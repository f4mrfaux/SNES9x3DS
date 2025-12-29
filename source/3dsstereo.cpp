#include <3ds.h>
#include <citro3d.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <sstream>
#include <ctime>

#include "3dsstereo.h"
#include "3dsgpu.h"
#include "3dssettings.h"
#include "3dsimpl_gpu.h"
#include "3dslog.h"
#include "3dsimpl.h"  // For accessing snesDepthForScreens
#include "3dsopt.h"   // For t3dsStartTiming/t3dsEndTiming
#include "Snes9x/gfxhw.h"
#include "Snes9x/ppu.h"

// External GPU state from 3dsgpu.cpp
extern SGPU3DS GPU3DS;
extern ScreenSettings screenSettings;

//=============================================================================
// Phase 2: Layer Tracking Globals
//=============================================================================
// NOTE: These globals are defined in 3dsimpl_gpu.cpp and declared extern in 3dsstereo.h
// They track which SNES layer is currently being rendered and per-layer depth offsets
// See 3dsimpl_gpu.cpp:14-15 for actual definitions

//=============================================================================
// Stereoscopic 3D Module Implementation
//=============================================================================

//-----------------------------------------------------------------------------
// Global state
//-----------------------------------------------------------------------------
static SStereoConfig stereoConfig;
static bool stereoInitialized = false;
static float currentSliderValue = 0.0f;

// Logging state (for meaningful debug output)
static int frameCounter = 0;
static float lastLoggedSlider = -1.0f;
static int lastLoggedFrame = 0;
static int stereoFrameCount = 0;
static int monoFrameCount = 0;
static int sliderZeroFrames = 0;

// Phase 1: Dual render targets
// Using SGPUTexture (SNES9x's existing texture system) instead of C3D_RenderTarget
// This matches how snesMainScreenTarget and snesSubScreenTarget work
static SGPUTexture* stereoLeftEyeTarget = NULL;
static SGPUTexture* stereoRightEyeTarget = NULL;
static SGPUTexture* stereoLeftEyeDepth = NULL;
static SGPUTexture* stereoRightEyeDepth = NULL;
static bool stereoDepthShared = false;

//-----------------------------------------------------------------------------
// Initialize stereoscopic rendering
//-----------------------------------------------------------------------------
bool stereo3dsInitialize()
{
    if (stereoInitialized)
        return true;

    LOG_INFO("STEREO", "Initializing stereoscopic 3D system (lazy allocation mode)");

    // Initialize config with defaults
    stereoConfig.enabled = true;

    // Enable stereoscopic 3D mode (so hardware supports it when needed)
    gfxSet3D(true);

    // Phase 1: SKIP render target creation - defer until slider > 0 to save VRAM
    // Render targets will be created on-demand in stereo3dsEnsureTargetsCreated()
    // This saves ~1MB VRAM when 3D slider is off

    stereoInitialized = true;
    frameCounter = 0;
    lastLoggedSlider = -1.0f;
    lastLoggedFrame = 0;
    stereoFrameCount = 0;
    monoFrameCount = 0;
    sliderZeroFrames = 0;
    
    LOG_INFO("STEREO", "Stereo 3D system ready - targets will allocate on first use");

    return true;
}

//-----------------------------------------------------------------------------
// Cleanup stereoscopic rendering
//-----------------------------------------------------------------------------
void stereo3dsFinalize()
{
    if (!stereoInitialized)
        return;

    LOG_INFO("STEREO", "Cleaning up stereo 3D system");

    // Clean up Phase 1 render targets
    stereo3dsDestroyRenderTargets();

    stereoInitialized = false;
}

//=============================================================================
// Configuration Functions (Plan E)
//=============================================================================

void stereo3dsSetEnabled(bool enabled)
{
    stereoConfig.enabled = enabled;

    if (enabled) {
        gfxSet3D(true);
    } else {
        gfxSet3D(false);
    }
}

bool stereo3dsIsEnabled()
{
    return stereoConfig.enabled && stereoInitialized;
}

float stereo3dsGetSliderValue()
{
    return currentSliderValue;
}

//-----------------------------------------------------------------------------
// Logging helper: Update frame counter and log meaningful events
//-----------------------------------------------------------------------------
void stereo3dsLogFrameUpdate(float effectiveSlider, bool stereoActive)
{
    frameCounter++;
    
    // Log slider changes (when changed by >5% or every 300 frames if active)
    if (fabs(effectiveSlider - lastLoggedSlider) > 0.05f || 
        (stereoActive && (frameCounter - lastLoggedFrame) >= 300)) {
        LOG_INFO("STEREO", "Slider: %.2f | Active: %s | Frames: Stereo=%d Mono=%d",
                 effectiveSlider, stereoActive ? "YES" : "NO", 
                 stereoFrameCount, monoFrameCount);
        lastLoggedSlider = effectiveSlider;
        lastLoggedFrame = frameCounter;
    }
    
    if (stereoActive) {
        stereoFrameCount++;
        sliderZeroFrames = 0;
    } else {
        monoFrameCount++;
        if (stereo3dsAreTargetsCreated()) {
            sliderZeroFrames++;
            if (sliderZeroFrames == 300) { // ~5 seconds at 60fps
                LOG_INFO("STEREO", "Slider off for 5s - releasing stereo targets to free VRAM");
                stereo3dsDestroyRenderTargets();
            }
        }
    }
}

//=============================================================================
// Phase 1: Dual Render Target Management
//=============================================================================

//-----------------------------------------------------------------------------
// Create stereo render targets
//-----------------------------------------------------------------------------
bool stereo3dsCreateRenderTargets()
{
    // Destroy any existing targets first
    stereo3dsDestroyRenderTargets();

    LOG_INFO("STEREO", "=== CREATE RENDER TARGETS START ===");
    LOG_INFO("STEREO", "Creating dual render targets (256x256 RGBA8 - matching mono)...");

    // Create LEFT eye render target (MUST match snesMainScreenTarget: 256x256)
    // Using 240 height caused 90-degree rotation artifacts
    LOG_DEBUG("STEREO", "Allocating LEFT eye color buffer...");
    stereoLeftEyeTarget = gpu3dsCreateTextureInVRAM(256, 256, GPU_RGBA8);
    if (!stereoLeftEyeTarget) {
        LOG_ERROR("STEREO", "VRAM allocation FAILED for LEFT eye render target!");
        return false;
    }
    LOG_DEBUG("STEREO", "LEFT eye created at %p", stereoLeftEyeTarget->PixelData);

    // Create RIGHT eye render target (must match LEFT: 256x256)
    LOG_DEBUG("STEREO", "Allocating RIGHT eye color buffer...");
    stereoRightEyeTarget = gpu3dsCreateTextureInVRAM(256, 256, GPU_RGBA8);
    if (!stereoRightEyeTarget) {
        LOG_ERROR("STEREO", "VRAM allocation FAILED for RIGHT eye render target!");
        stereo3dsDestroyRenderTargets();  // Clean up LEFT target
        return false;
    }
    LOG_DEBUG("STEREO", "RIGHT eye created at %p", stereoRightEyeTarget->PixelData);

    // Create per-eye depth buffers to avoid cross-eye depth artifacts.
    LOG_DEBUG("STEREO", "Allocating LEFT eye depth buffer...");
    stereoLeftEyeDepth = gpu3dsCreateTextureInVRAM(256, 240, GPU_RGBA8);
    if (!stereoLeftEyeDepth) {
        LOG_ERROR("STEREO", "VRAM allocation FAILED for LEFT eye depth buffer!");
        stereo3dsDestroyRenderTargets();
        return false;
    }
    LOG_DEBUG("STEREO", "LEFT depth created at %p", stereoLeftEyeDepth->PixelData);

    LOG_DEBUG("STEREO", "Allocating RIGHT eye depth buffer...");
    stereoRightEyeDepth = gpu3dsCreateTextureInVRAM(256, 240, GPU_RGBA8);
    if (!stereoRightEyeDepth) {
        LOG_ERROR("STEREO", "VRAM allocation FAILED for RIGHT eye depth buffer!");
        stereo3dsDestroyRenderTargets();
        return false;
    }
    LOG_DEBUG("STEREO", "RIGHT depth created at %p", stereoRightEyeDepth->PixelData);
    stereoDepthShared = false;

    LOG_INFO("STEREO", "=== ALL RENDER TARGETS CREATED SUCCESSFULLY ===");
    LOG_INFO("STEREO", "LEFT color: %p, RIGHT color: %p",
             stereoLeftEyeTarget->PixelData, stereoRightEyeTarget->PixelData);
    LOG_INFO("STEREO", "LEFT depth: %p, RIGHT depth: %p",
             stereoLeftEyeDepth->PixelData, stereoRightEyeDepth->PixelData);

    return true;
}

//-----------------------------------------------------------------------------
// Destroy stereo render targets
//-----------------------------------------------------------------------------
void stereo3dsDestroyRenderTargets()
{
    if (stereoLeftEyeTarget) {
        LOG_INFO("STEREO", "Destroying LEFT eye render target");
        gpu3dsDestroyTextureFromVRAM(stereoLeftEyeTarget);
        stereoLeftEyeTarget = NULL;
    }

    if (stereoRightEyeTarget) {
        LOG_INFO("STEREO", "Destroying RIGHT eye render target");
        gpu3dsDestroyTextureFromVRAM(stereoRightEyeTarget);
        stereoRightEyeTarget = NULL;
    }

    if (stereoLeftEyeDepth) {
        LOG_INFO("STEREO", "Destroying LEFT eye depth buffer%s", stereoDepthShared ? " (shared)" : "");
        gpu3dsDestroyTextureFromVRAM(stereoLeftEyeDepth);
        stereoLeftEyeDepth = NULL;
    }
    if (stereoRightEyeDepth && (!stereoDepthShared || stereoRightEyeDepth != stereoLeftEyeDepth)) {
        LOG_INFO("STEREO", "Destroying RIGHT eye depth buffer");
        gpu3dsDestroyTextureFromVRAM(stereoRightEyeDepth);
        stereoRightEyeDepth = NULL;
    }
    stereoDepthShared = false;
}

//-----------------------------------------------------------------------------
// Set active render target for drawing
// Returns false if targets don't exist (caller should fallback to mono)
//-----------------------------------------------------------------------------
bool stereo3dsSetActiveRenderTarget(StereoEye eye)
{
    // DEBUG: Log first 10 target switches
    static int switchLogCount = 0;
    bool shouldLog = (switchLogCount < 10);

    if (eye == STEREO_EYE_LEFT) {
        if (stereoLeftEyeTarget && stereoLeftEyeDepth) {
            gpu3dsSetRenderTargetToTexture(stereoLeftEyeTarget, stereoLeftEyeDepth);
            if (shouldLog) {
                LOG_INFO("TARGET-DBG", "Switch #%d -> LEFT (tex=%p depth=%p dim=%dx%d)",
                         switchLogCount, stereoLeftEyeTarget->PixelData, stereoLeftEyeDepth->PixelData,
                         stereoLeftEyeTarget->Width, stereoLeftEyeTarget->Height);
                switchLogCount++;
            }
            return true;
        }
    } else {
        if (stereoRightEyeTarget && stereoRightEyeDepth) {
            gpu3dsSetRenderTargetToTexture(stereoRightEyeTarget, stereoRightEyeDepth);
            if (shouldLog) {
                LOG_INFO("TARGET-DBG", "Switch #%d -> RIGHT (tex=%p depth=%p dim=%dx%d)",
                         switchLogCount, stereoRightEyeTarget->PixelData, stereoRightEyeDepth->PixelData,
                         stereoRightEyeTarget->Width, stereoRightEyeTarget->Height);
                switchLogCount++;
            }
            return true;
        }
    }
    // Targets don't exist - this is a critical error
    LOG_ERROR("STEREO", "stereo3dsSetActiveRenderTarget(%s) failed - targets not created!",
              eye == STEREO_EYE_LEFT ? "LEFT" : "RIGHT");
    return false;
}

//-----------------------------------------------------------------------------
// Check if render targets are created
//-----------------------------------------------------------------------------
bool stereo3dsAreTargetsCreated()
{
    return (stereoLeftEyeTarget != NULL &&
            stereoRightEyeTarget != NULL &&
            stereoLeftEyeDepth != NULL &&
            stereoRightEyeDepth != NULL);
}

//=============================================================================
// Phase 3: Layer Offset Management
//=============================================================================

//-----------------------------------------------------------------------------
// Update layer offsets for BOTH eyes based on 3D slider (Plan E)
//-----------------------------------------------------------------------------
void stereo3dsUpdateLayerOffsetsFromSlider(float sliderValue)
{
    currentSliderValue = sliderValue;

    // If slider is "off", just zero everything
    if (sliderValue < 0.01f)
    {
        for (int eye = 0; eye < 2; ++eye)
            for (int i = 0; i < 5; ++i)
                g_stereoLayerOffsets[eye][i] = 0.0f;
        return;
    }

    // Anchor depths relative to the configured screen-plane layer so the chosen layer stays at 0.
    const int plane = settings3DS.ScreenPlaneLayer;
    const float planeDepth = (plane >= 0 && plane < 5) ? settings3DS.LayerDepth[plane] : 0.0f;

    // Use configured per-layer depths (positive = into screen, negative = pop-out), scaled by slider.
    // CORRECTED SIGN CONVENTION (Dec 2025):
    // For INTO screen (positive depth): LEFT shifts RIGHT (+), RIGHT shifts LEFT (-) = convergence
    // For POP OUT (negative depth): LEFT shifts LEFT (-), RIGHT shifts RIGHT (+) = divergence
    // The offset sign convention: offset = depth * slider, then NEGATE for eye offsets
    //   - Positive depth → LEFT=-, RIGHT=+ → convergence (into screen) ✅
    //   - Negative depth → LEFT=+, RIGHT=- → divergence (pop out) ✅
    const float depthStrength = settings3DS.StereoDepthStrength;
    const float maxPerEye = 17.0f;  // ~34px total parallax comfort limit

    for (int i = 0; i < 5; ++i)
    {
        float depth = (settings3DS.LayerDepth[i] - planeDepth) * depthStrength;
        float offset = depth * sliderValue;

        // Clamp per-eye shift to comfort range
        if (offset >  maxPerEye) offset =  maxPerEye;
        if (offset < -maxPerEye) offset = -maxPerEye;

        // CRITICAL FIX (Dec 2025): Negate offsets to match depth semantics
        // Positive depth (backgrounds) → convergence → into screen
        // Negative depth (sprites) → divergence → pop out
        // Example: BG0 depth=+8.0 → offset=+8.0 → LEFT=-8, RIGHT=+8 → convergence ✅
        g_stereoLayerOffsets[0][i] = -offset;  // LEFT eye: negated offset
        g_stereoLayerOffsets[1][i] = offset;   // RIGHT eye: positive offset
    }


    // Log offset changes periodically (every 300 frames when active)
    if (sliderValue >= 0.01f && (frameCounter % 300 == 0)) {
        LOG_DEBUG("STEREO", "Offsets (slider=%.2f): L[BG0=%.1f SPR=%.1f] R[BG0=%.1f SPR=%.1f]",
                  sliderValue,
                  g_stereoLayerOffsets[0][0], g_stereoLayerOffsets[0][4],
                  g_stereoLayerOffsets[1][0], g_stereoLayerOffsets[1][4]);
    }
}

//-----------------------------------------------------------------------------
// Transfer stereo eye textures to framebuffers
//-----------------------------------------------------------------------------
// Lazy allocation: Create render targets on first use (when slider > 0)
bool stereo3dsEnsureTargetsCreated()
{
    if (stereo3dsAreTargetsCreated())
        return true;  // Already created

    LOG_INFO("STEREO", "Allocating render targets on-demand (lazy allocation)");

    if (!stereo3dsCreateRenderTargets()) {
        LOG_ERROR("STEREO", "Failed to allocate render targets! VRAM may be exhausted - staying in mono mode");
        return false;
    }

    LOG_INFO("STEREO", "Render targets allocated - stereo 3D now active");
    return true;
}

bool stereo3dsTransferToScreenBuffers()
{
    t3dsStartTiming(14, "StereoTransfer");

    // Lazy allocation: Create targets if not already created
    bool targetsOk = stereo3dsEnsureTargetsCreated();
    if (!targetsOk) {
        LOG_ERROR("STEREO", "Render targets not available, falling back to mono");
        t3dsEndTiming(14);
        return false;
    }

    // Get dimensions - source is texture size, dest is screen size
    int screenWidth  = screenSettings.GameScreenWidth;    // e.g. 400
    int texWidth     = stereoLeftEyeTarget->Width;        // e.g. 256
    int texHeight    = stereoLeftEyeTarget->Height;       // e.g. 240

    gpu3dsWaitForPreviousFlush();

    // Get framebuffer pointers
    u32 *leftFB = (u32 *)gfxGetFramebuffer(screenSettings.GameScreen, GFX_LEFT, NULL, NULL);
    u32 *rightFB = (u32 *)gfxGetFramebuffer(screenSettings.GameScreen, GFX_RIGHT, NULL, NULL);

    // CRITICAL DEBUG: Log framebuffer pointers (first 10 frames only)
    static int dbgTransferCount = 0;
    if (dbgTransferCount < 10) {
        LOG_INFO("STEREO-DBG", "Transfer #%d: LEFT_SRC=%p LEFT_DST=%p RIGHT_SRC=%p RIGHT_DST=%p",
                 dbgTransferCount,
                 stereoLeftEyeTarget->PixelData, leftFB,
                 stereoRightEyeTarget->PixelData, rightFB);
        LOG_INFO("STEREO-DBG", "  Dims: src=%dx%d dst=%dx%d fmt_in=%d fmt_out=%d",
                 texWidth, texHeight, screenWidth, SCREEN_HEIGHT,
                 GPU3DS.frameBufferFormat, GPU3DS.screenFormat);
        LOG_INFO("STEREO-DBG", "  FB pointers same? %s", leftFB == rightFB ? "YES-ERROR!" : "NO-OK");
        dbgTransferCount++;
    }

    // LEFT eye: stereoLeftEyeTarget → GFX_LEFT framebuffer
    GX_DisplayTransfer(
        (u32*)stereoLeftEyeTarget->PixelData,
        GX_BUFFER_DIM(texHeight, texWidth),               // ✅ Source = texture dims
        leftFB,
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenWidth),        // ✅ Dest = screen dims
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FRAMEBUFFER_FORMAT_VALUES[GPU3DS.frameBufferFormat]) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_SCREEN_FORMAT_VALUES[GPU3DS.screenFormat]));

    gpu3dsWaitForPreviousFlush();

    // RIGHT eye: stereoRightEyeTarget → GFX_RIGHT framebuffer
    GX_DisplayTransfer(
        (u32*)stereoRightEyeTarget->PixelData,
        GX_BUFFER_DIM(texHeight, texWidth),               // ✅ Source = texture dims
        rightFB,
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenWidth),        // ✅ Dest = screen dims
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FRAMEBUFFER_FORMAT_VALUES[GPU3DS.frameBufferFormat]) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_SCREEN_FORMAT_VALUES[GPU3DS.screenFormat]));


    // Log transfer periodically (every 600 frames = ~10 seconds at 60fps)
    static int transferLogCounter = 0;
    transferLogCounter++;
    if (transferLogCounter % 600 == 0) {
        LOG_DEBUG("STEREO", "Transfer: %dx%d -> %dx%d (L/R framebuffers)",
                  texWidth, texHeight, screenWidth, SCREEN_HEIGHT);
    }

    // Reset stereo clear flag for next frame
    gpu3dsResetStereoClearFlag();

    t3dsEndTiming(14);
    return true;
}
