# Dual-Render Implementation Plan for Per-Layer 3D Depth

**Date:** November 13, 2025 (Evening)
**Status:** Artifact fix applied, dual-render plan documented
**Build:** `snes9x_3ds-stereo-ARTIFACT-FIX.3dsx` (MD5: 6f319605d14572d756fa136d5846195a)

---

## Problem Recap

**What we tried:** Manually writing to both `GFX_LEFT` and `GFX_RIGHT` framebuffers
**Result:** White/black line artifacts (corrupted display)
**Why it failed:** Fighting against 3DS hardware expectations for stereo mode

---

## Immediate Fix (Applied)

**Solution:** Use mono-style transfer - write to `GFX_LEFT` only
```cpp
void stereo3dsRenderSNESFrame() {
    // Use same transfer as mono - hardware auto-duplicates to RIGHT
    gpu3dsTransferToScreenBuffer(screenSettings.GameScreen);
}
```

**Result:**
- ✅ No more artifacts
- ✅ Stereo mode works
- ✅ Frames update properly
- ❌ No depth effect (both eyes see identical image)

---

## The Path to True Per-Layer 3D Depth

### Core Insight (ChatGPT + Claude Agreement)

**You CAN'T call `S9xMainLoop()` twice, but you CAN render geometry twice.**

The key is to intercept at the **draw call level** and render each layer twice with different offsets.

### How SNES9x Rendering Works

```cpp
S9xMainLoop() {
    // 1. Game logic updates (PPU state, sprites, etc.)

    // 2. For each layer (BG0, BG1, BG2, BG3, Sprites):
    gpu3dsAddTileVertexes(x, y, w, h, ...);  // Build geometry
    gpu3dsDrawVertexes();                     // Submit to GPU

    // 3. Result accumulates in GPU3DS.frameBuffer
}
```

**Key point:** Each layer is drawn separately via `gpu3dsDrawVertexes()` calls.

---

## Dual-Render Architecture

### High-Level Flow

```
For each frame:
  1. S9xMainLoop() runs ONCE (game logic advances once)
  2. Each time gpu3dsDrawVertexes() is called:
     - Draw geometry to LEFT eye target with +offset
     - Draw same geometry to RIGHT eye target with -offset
  3. At frame end:
     - Transfer LEFT target → GFX_LEFT framebuffer
     - Transfer RIGHT target → GFX_RIGHT framebuffer
  4. Swap buffers → stereo display!
```

### Why This Works

- ✅ Game logic runs **only once** (no state duplication)
- ✅ Every draw call produces output for **both eyes**
- ✅ Each layer can have **different depth** (per-layer offsets)
- ✅ Minimal performance overhead (~90% as predicted)
- ✅ Works with existing SNES9x architecture

---

## Implementation Phases

### Phase 1: Add Dual Render Targets (2-3 hours)

**File:** `source/3dsgpu.cpp`

```cpp
// Global state
static C3D_RenderTarget* leftEyeTarget = NULL;
static C3D_RenderTarget* rightEyeTarget = NULL;
static bool stereoRenderingActive = false;

void gpu3dsInitialize() {
    // ...existing init...

    if (settings3DS.EnableStereo3D) {
        // Create offscreen render targets for stereo
        leftEyeTarget = gpu3dsCreateTexture(
            screenSettings.GameScreenWidth,
            SCREEN_HEIGHT,
            GPU_RGBA8
        );

        rightEyeTarget = gpu3dsCreateTexture(
            screenSettings.GameScreenWidth,
            SCREEN_HEIGHT,
            GPU_RGBA8
        );

        printf("[STEREO] Dual render targets created\n");
    }
}

void gpu3dsFinalize() {
    if (leftEyeTarget) {
        gpu3dsDestroyTexture(leftEyeTarget);
        leftEyeTarget = NULL;
    }
    if (rightEyeTarget) {
        gpu3dsDestroyTexture(rightEyeTarget);
        rightEyeTarget = NULL;
    }
}
```

**Test:** Verify targets are created without crashing.

---

### Phase 2: Track Current Layer (3-4 hours)

**File:** `source/3dsimpl_gpu.h`

```cpp
// Global layer tracking
extern int g_currentLayerIndex;  // 0=BG0, 1=BG1, 2=BG2, 3=BG3, 4=Sprites
extern float g_stereoLayerOffsets[5];
```

**File:** `source/Snes9x/gfxhw.cpp` (or wherever layers are drawn)

Find where each layer renders and add:

```cpp
// Before drawing BG0:
g_currentLayerIndex = 0;
gpu3dsDrawVertexes();

// Before drawing BG1:
g_currentLayerIndex = 1;
gpu3dsDrawVertexes();

// Before drawing BG2:
g_currentLayerIndex = 2;
gpu3dsDrawVertexes();

// Before drawing BG3:
g_currentLayerIndex = 3;
gpu3dsDrawVertexes();

// Before drawing sprites:
g_currentLayerIndex = 4;
gpu3dsDrawVertexes();
```

**Test:** Add debug prints to verify layer index is set correctly.

---

### Phase 3: Modify gpu3dsDrawVertexes() for Dual Rendering (4-5 hours)

**File:** `source/3dsgpu.cpp`

```cpp
void gpu3dsDrawVertexes() {
    float slider = settings3DS.StereoSliderValue;

    if (!settings3DS.EnableStereo3D || slider < 0.01f || !leftEyeTarget) {
        // MONO MODE: Normal rendering
        GPU_DrawElements(...);
        return;
    }

    // STEREO MODE: Render twice with layer-specific offsets
    float offset = g_stereoLayerOffsets[g_currentLayerIndex] * slider;

    // === LEFT EYE (positive offset = shift RIGHT) ===
    gpu3dsSetRenderTarget(leftEyeTarget);

    // Apply +offset to all vertex X coordinates
    for (int i = 0; i < currentVertexList->numVertices; i++) {
        currentVertexList->vertices[i].position.x += offset;
    }

    GPU_DrawElements(...);  // Draw with offset

    // === RIGHT EYE (negative offset = shift LEFT) ===
    gpu3dsSetRenderTarget(rightEyeTarget);

    // Apply -offset (from current position, which is already +offset)
    for (int i = 0; i < currentVertexList->numVertices; i++) {
        currentVertexList->vertices[i].position.x -= (offset * 2.0f);
    }

    GPU_DrawElements(...);  // Draw with opposite offset

    // === RESTORE vertices ===
    for (int i = 0; i < currentVertexList->numVertices; i++) {
        currentVertexList->vertices[i].position.x += offset;  // Back to original
    }

    // Restore render target
    gpu3dsSetRenderTarget(mainScreenTarget);
}
```

**Test:**
1. First test with **all offsets = 0** (should produce flat stereo, no depth)
2. Then enable **small offsets** (e.g., BG0=5px, sprites=10px)
3. Verify left/right eyes show different horizontal positions

---

### Phase 4: Transfer Both Eyes at Frame End (2-3 hours)

**File:** `source/3dsstereo.cpp`

```cpp
void stereo3dsRenderSNESFrame() {
    float slider = settings3DS.StereoSliderValue;

    if (!settings3DS.EnableStereo3D || slider < 0.01f) {
        // Fallback to mono
        gpu3dsTransferToScreenBuffer(screenSettings.GameScreen);
        return;
    }

    // Transfer LEFT eye texture to left framebuffer
    GX_DisplayTransfer(
        leftEyeTarget->texture->data,
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenSettings.GameScreenWidth),
        (u32 *)gfxGetFramebuffer(screenSettings.GameScreen, GFX_LEFT, NULL, NULL),
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenSettings.GameScreenWidth),
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)
    );
    gspWaitForPPF();

    // Transfer RIGHT eye texture to right framebuffer
    GX_DisplayTransfer(
        rightEyeTarget->texture->data,
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenSettings.GameScreenWidth),
        (u32 *)gfxGetFramebuffer(screenSettings.GameScreen, GFX_RIGHT, NULL, NULL),
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenSettings.GameScreenWidth),
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)
    );
    gspWaitForPPF();

    if (frameCount == 1) {
        printf("[DUAL-RENDER] === PER-LAYER 3D DEPTH ACTIVE ===\n");
    }
}
```

**Test:** Check for artifacts, verify both eyes transfer cleanly.

---

### Phase 5: Configure Per-Layer Depths (2-3 hours)

**File:** `source/3dssettings.h`

```cpp
struct S9xSettings3DS {
    // ...existing settings...

    // Stereoscopic 3D
    bool EnableStereo3D;
    float StereoSliderValue;  // Updated from osGet3DSliderState()
    float LayerDepth[5];      // Depth for BG0, BG1, BG2, BG3, Sprites
};
```

**Default depths:**
```cpp
settings3DS.LayerDepth[0] = 15.0f;  // BG0 (far background)
settings3DS.LayerDepth[1] = 10.0f;  // BG1
settings3DS.LayerDepth[2] = 5.0f;   // BG2
settings3DS.LayerDepth[3] = 0.0f;   // BG3 (at screen plane)
settings3DS.LayerDepth[4] = 20.0f;  // Sprites (pop out!)
```

**Initialize in `stereo3dsInitialize()`:**
```cpp
for (int i = 0; i < 5; i++) {
    g_stereoLayerOffsets[i] = settings3DS.LayerDepth[i];
}
```

---

### Phase 6: Add In-Game Menu (3-4 hours)

Allow users to adjust depth per-layer in real-time.

**File:** `source/3dsmain.cpp` (menu system)

```cpp
void AddStereoSettingsMenu() {
    AddMenuSlider("BG0 Depth", 0, 30, settings3DS.LayerDepth[0]);
    AddMenuSlider("BG1 Depth", 0, 30, settings3DS.LayerDepth[1]);
    AddMenuSlider("BG2 Depth", 0, 30, settings3DS.LayerDepth[2]);
    AddMenuSlider("BG3 Depth", 0, 30, settings3DS.LayerDepth[3]);
    AddMenuSlider("Sprite Depth", 0, 30, settings3DS.LayerDepth[4]);
}
```

**Save per-game profiles:** Store depth settings in game-specific config files.

---

## Estimated Total Effort

| Phase | Task | Time | Difficulty |
|-------|------|------|------------|
| 1 | Dual render targets | 2-3h | Medium |
| 2 | Layer tracking | 3-4h | Medium |
| 3 | Dual draw calls | 4-5h | Hard |
| 4 | Frame-end transfers | 2-3h | Medium |
| 5 | Depth configuration | 2-3h | Easy |
| 6 | In-game menu | 3-4h | Medium |
| **Total** | | **16-22 hours** | **Medium-Hard** |

---

## Testing Strategy

### Step 0: Sanity Check (Before any coding)

Test with **current artifact-fix build**:
- ✅ Verify stereo mode works (no artifacts)
- ✅ Confirm frames update
- ✅ Validate it's just flat (no depth)

### Step 1: Dual Targets Only

Implement Phase 1, then test:
- Create LEFT/RIGHT targets
- Don't render to them yet, just verify no crash
- Check memory usage (targets should be ~2-3 MB each)

### Step 2: Flat Dual Render

Implement Phases 2-3 with **all offsets = 0**:
- Should produce identical LEFT/RIGHT images
- Verify no performance drop beyond expected 2x rendering

### Step 3: Tiny Offsets

Enable small offsets (5-10px):
- BG0 = 5px
- Sprites = 10px
- Others = 0
- Should see subtle depth separation

### Step 4: Full Depth

Enable full depth values and tune:
- Adjust per-layer until it "feels right"
- Test with multiple games
- Create per-game profiles

---

## Performance Expectations

**Mono (slider=0):**
- Overhead: <1% (fast path)
- FPS: 60

**Stereo (slider>0):**
- Overhead: ~90% (render 2x)
- FPS: 50-60 on New 3DS @ 804MHz
- FPS: 35-45 on Old 3DS @ 268MHz

**Optimizations:**
- Early-out when slider=0
- Reuse vertex buffers (don't reallocate)
- Cache layer offsets (don't recalculate per draw)

---

## Fallback Plan

If GPU dual-rendering becomes too complex:

**CPU-side compositor approach:**
1. Extract each layer to separate textures
2. CPU composites LEFT eye: layer0 + offset0, layer1 + offset1, etc.
3. CPU composites RIGHT eye: layer0 - offset0, layer1 - offset1, etc.
4. Transfer composited images to framebuffers

**Pros:** Simpler to reason about, easy to debug
**Cons:** Slower (CPU-bound), more memory

---

## Next Steps

### Immediate (Tonight):

1. **Test artifact-fix build on hardware:**
   - File: `snes9x_3ds-stereo-ARTIFACT-FIX.3dsx`
   - Expected: No artifacts, flat stereo, frames update
   - Report: Does it work?

### After Confirmation:

2. **Begin Phase 1:** Add dual render targets
3. **Document findings:** Update this plan based on hardware results

---

## Related Documentation

- `HARDWARE_TESTING_SESSION_NOV13.md` - Session log
- `3DS_DEBUGGING_GUIDE.md` - Debugging reference
- `STEREO_RENDERING_COMPLETE_GUIDE.md` - Original research
- `tech-portfolio/docs/snes9x-3ds-stereoscopic-3d.md` - Project overview

---

**Last Updated:** November 13, 2025 (Evening)
**Status:** Artifact fix applied, ready for hardware test
**Next:** Test on 3DS, then begin dual-render implementation
