/**
 * Nintendo 3DS Stereoscopic 3D Rendering - Code Snippets
 * Quick reference implementations for common stereo tasks
 */

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

/* ============================================================================
 * SNIPPET 1: Minimal Citro3D Stereo Setup
 * ============================================================================ */

void setup_stereo_citro3d(void) {
    // Initialize graphics
    gfxInitDefault();
    gfxSet3D(true);  // CRITICAL: Enable 3D before creating render targets
    
    // Initialize GPU
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    
    // Create render targets for left and right eyes
    C3D_RenderTarget* targetLeft = C3D_RenderTargetCreate(
        240, 400,                    // 240 wide x 400 tall (top screen portrait)
        GPU_RB_RGBA8,                // Color format
        GPU_RB_DEPTH24_STENCIL8      // Depth format with stencil
    );
    
    C3D_RenderTarget* targetRight = C3D_RenderTargetCreate(
        240, 400,
        GPU_RB_RGBA8,
        GPU_RB_DEPTH24_STENCIL8
    );
    
    // Bind targets to screen outputs
    C3D_RenderTargetSetOutput(targetLeft, GFX_TOP, GFX_LEFT,
        GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | 
        GX_TRANSFER_RAW_COPY(0) | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | 
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)
    );
    
    C3D_RenderTargetSetOutput(targetRight, GFX_TOP, GFX_RIGHT,
        GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) |
        GX_TRANSFER_RAW_COPY(0) | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)
    );
}

/* ============================================================================
 * SNIPPET 2: Render Both Eyes in Main Loop
 * ============================================================================ */

void render_stereo_frame(C3D_RenderTarget* targetLeft, C3D_RenderTarget* targetRight) {
    // Get current 3D slider position (0.0 to 1.0)
    float slider = osGet3DSliderState();
    
    // Convert to interocular distance
    // Negative for left eye, positive for right eye
    float iod = slider / 3.0f;
    
    // Start frame rendering
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    {
        // === LEFT EYE ===
        C3D_RenderTargetClear(targetLeft, C3D_CLEAR_ALL, 0x68B0D8FF, 0);
        C3D_FrameDrawOn(targetLeft);
        render_scene(-iod);  // Pass NEGATIVE iod
        
        // === RIGHT EYE ===
        // Only render if slider position indicates 3D effect desired
        if (iod > 0.0f) {
            C3D_RenderTargetClear(targetRight, C3D_CLEAR_ALL, 0x68B0D8FF, 0);
            C3D_FrameDrawOn(targetRight);
            render_scene(iod);   // Pass POSITIVE iod
        }
    }
    C3D_FrameEnd(0);
}

/* ============================================================================
 * SNIPPET 3: Stereo Projection Matrix Creation
 * ============================================================================ */

void create_stereo_projection(C3D_Mtx* projection, float iod) {
    // Use PerspStereoTilt for 3DS top screen (accounts for 90° rotation)
    Mtx_PerspStereoTilt(
        projection,
        C3D_AngleFromDegrees(40.0f),  // Vertical field of view
        C3D_AspectRatioTop,            // 400/240 = 1.667 aspect ratio
        0.01f,                         // Near clipping plane
        1000.0f,                       // Far clipping plane
        iod,                           // Interocular distance (eye separation)
        2.0f,                          // Screen focal length (convergence depth)
        false                          // Right-handed coordinate system
    );
}

/* ============================================================================
 * SNIPPET 4: Complete Scene Render with Stereo
 * ============================================================================ */

static C3D_Mtx g_projection;
static int g_projectionLoc, g_modelViewLoc;

void render_scene(float iod) {
    // Update projection matrix for this eye
    create_stereo_projection(&g_projection, iod);
    
    // Create model-view matrix
    C3D_Mtx modelView;
    Mtx_Identity(&modelView);
    Mtx_Translate(&modelView, 0.0f, 0.0f, -3.0f, true);
    Mtx_RotateY(&modelView, C3D_Angle(0.1f), true);
    Mtx_Scale(&modelView, 2.0f, 2.0f, 2.0f);
    
    // Upload matrices to GPU
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_projectionLoc, &g_projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_modelViewLoc, &modelView);
    
    // Draw geometry
    C3D_DrawArrays(GPU_TRIANGLES, 0, 36);  // Example: draw 36 vertices
}

/* ============================================================================
 * SNIPPET 5: Minimal Citro2D Stereo Setup
 * ============================================================================ */

void setup_stereo_citro2d(void) {
    // Initialize
    gfxInitDefault();
    gfxSet3D(true);
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    
    // Targets are created during main loop or separately
}

/* ============================================================================
 * SNIPPET 6: Citro2D Main Loop Pattern
 * ============================================================================ */

void render_2d_stereo_frame(C3D_RenderTarget* left, C3D_RenderTarget* right,
                            C2D_Image img, int offset) {
    // Get slider state
    float slider = osGet3DSliderState();
    
    // Start frame
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    
    // LEFT eye: shift image right
    {
        C2D_TargetClear(left, C2D_Color32(255, 255, 255, 255));
        C2D_SceneBegin(left);
        // Shift position: +offset for left eye
        C2D_DrawImageAt(img, 100.0f + offset * slider, 50.0f, 0.0f);
    }
    
    // RIGHT eye: shift image left
    {
        C2D_TargetClear(right, C2D_Color32(255, 255, 255, 255));
        C2D_SceneBegin(right);
        // Shift position: -offset for right eye
        C2D_DrawImageAt(img, 100.0f - offset * slider, 50.0f, 0.0f);
    }
    
    C3D_FrameEnd(0);
}

/* ============================================================================
 * SNIPPET 7: Check Stereo Availability
 * ============================================================================ */

bool is_stereo_available(void) {
    // Check if 3D mode is enabled
    return gfxIs3D();
}

float get_3d_intensity(void) {
    // Get slider position (0.0 = no 3D, 1.0 = max 3D)
    return osGet3DSliderState();
}

/* ============================================================================
 * SNIPPET 8: Cleanup
 * ============================================================================ */

void cleanup_stereo(C3D_RenderTarget* targetLeft, C3D_RenderTarget* targetRight) {
    // Clean up render targets
    C3D_RenderTargetDelete(targetLeft);
    C3D_RenderTargetDelete(targetRight);
    
    // Clean up GPU
    C3D_Fini();
    
    // Clean up graphics
    gfxExit();
}

/* ============================================================================
 * SNIPPET 9: Advanced - Conditional Right Eye Rendering
 * ============================================================================ */

void render_stereo_optimized(C3D_RenderTarget* targetLeft,
                             C3D_RenderTarget* targetRight,
                             void (*renderFunc)(float iod)) {
    float slider = osGet3DSliderState();
    float iod = slider / 3.0f;
    
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    {
        // Always render left eye
        C3D_RenderTargetClear(targetLeft, C3D_CLEAR_ALL, 0x68B0D8FF, 0);
        C3D_FrameDrawOn(targetLeft);
        renderFunc(-iod);
        
        // Only render right eye if slider indicates 3D is active
        if (iod > 0.001f) {  // Use small epsilon for float comparison
            C3D_RenderTargetClear(targetRight, C3D_CLEAR_ALL, 0x68B0D8FF, 0);
            C3D_FrameDrawOn(targetRight);
            renderFunc(iod);
        }
    }
    C3D_FrameEnd(0);
}

/* ============================================================================
 * SNIPPET 10: Stereo Parameter Definitions
 * ============================================================================ */

// Typical constants for stereo rendering
#define TOP_SCREEN_WIDTH       240
#define TOP_SCREEN_HEIGHT      400
#define STEREO_FOV_DEGREES     40.0f
#define STEREO_ASPECT_RATIO    C3D_AspectRatioTop  // 400/240
#define STEREO_NEAR_PLANE      0.01f
#define STEREO_FAR_PLANE       1000.0f
#define STEREO_SCREEN_FOCAL    2.0f    // Convergence distance
#define STEREO_IOD_SCALE       3.0f    // Divide slider by this

// Get IOD from slider
static inline float calculate_iod(float slider) {
    return slider / STEREO_IOD_SCALE;
}

/* ============================================================================
 * SNIPPET 11: Performance Monitoring for Stereo
 * ============================================================================ */

void monitor_stereo_performance(void) {
    // Get timing information
    float drawTime = C3D_GetDrawingTime();       // GPU drawing time (ms)
    float processTime = C3D_GetProcessingTime(); // CPU processing time (ms)
    
    // Note: Stereo rendering takes ~2x the GPU time (rendering 2 scenes)
    // but may only take 1.3-1.5x total time due to parallelization
    
    // Set target frame rate (important for smooth stereo)
    float actualFps = C3D_FrameRate(30.0f);  // Target 30 FPS for stereo
}

/* ============================================================================
 * SNIPPET 12: Debug - Verify Stereo Setup
 * ============================================================================ */

void verify_stereo_setup(void) {
    // Check 3D mode
    if (!gfxIs3D()) {
        // Error: 3D mode not enabled!
        return;
    }
    
    // Check slider position
    float slider = osGet3DSliderState();
    if (slider > 0.0f) {
        // 3D slider is active - stereo should be visible
    }
    
    // Check framebuffer info (internal use)
    // Both fb0 and fb1 should be different when in stereo mode
}

/* ============================================================================
 * SNIPPET 13: IOD Scaling Experiments
 * ============================================================================ */

// Different IOD scaling factors to experiment with
void test_iod_scaling(float slider) {
    // Conservative (less eye strain)
    float iod_conservative = slider / 4.0f;
    
    // Standard (recommended)
    float iod_standard = slider / 3.0f;
    
    // Aggressive (more dramatic 3D)
    float iod_aggressive = slider / 2.0f;
    
    // Custom factor based on game needs
    float custom_factor = 3.5f;
    float iod_custom = slider / custom_factor;
}

/* ============================================================================
 * SNIPPET 14: Screen Focal Distance Experiments
 * ============================================================================ */

// Screen focal distance affects where objects appear relative to the screen
void test_screen_focal_distances(float iod) {
    C3D_Mtx proj;
    
    // Objects at this distance appear AT the screen surface
    // Closer objects pop OUT, farther objects go IN
    
    // Close convergence (objects pop out more easily)
    Mtx_PerspStereoTilt(&proj, C3D_AngleFromDegrees(40.0f),
                        C3D_AspectRatioTop, 0.01f, 1000.0f,
                        iod, 1.0f, false);  // Objects pop out at z=1
    
    // Standard convergence (balanced)
    Mtx_PerspStereoTilt(&proj, C3D_AngleFromDegrees(40.0f),
                        C3D_AspectRatioTop, 0.01f, 1000.0f,
                        iod, 2.0f, false);  // Objects pop out at z=2
    
    // Far convergence (objects stay inside screen more)
    Mtx_PerspStereoTilt(&proj, C3D_AngleFromDegrees(40.0f),
                        C3D_AspectRatioTop, 0.01f, 1000.0f,
                        iod, 3.0f, false);  // Objects pop out at z=3
}

