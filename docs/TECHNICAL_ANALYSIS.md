# Technical Analysis: SNES9x 3DS for Stereoscopic Layer Separation

**Date:** 2025-10-20
**Purpose:** Analyze existing SNES9x 3DS implementation to plan stereoscopic 3D layer separation
**Base Project:** matbo87/snes9x_3ds (fork of bubble2k16/snes9x_3ds)

---

## Executive Summary

**Good News:**
- ✅ matbo87's SNES9x 3DS fork already uses GPU rendering (bubble2k16's implementation)
- ✅ Built with modern toolchain (devkitARM r62, libctru 2.2.2)
- ✅ Active development (v1.52 released August 2025)
- ✅ Stable, well-tested codebase
- ✅ New 3DS optimization support already present

**Gaps Identified:**
- ❌ No savestates (major missing feature)
- ❌ No fast-forward
- ❌ No stereoscopic 3D rendering (our target feature)
- ❌ Uses custom GPU wrapper instead of Citro3D (migration needed)

**Recommendation:** Use matbo87's fork as base, migrate to Citro3D, add stereo rendering + missing features.

---

## Repository Analysis

### 1. matbo87/snes9x_3ds Fork

**Latest Version:** v1.52 (August 15, 2025)

**Key Features:**
- Game thumbnails/boxart support
- Themes (Dark mode, RetroArch style, Original)
- Improved cheat menu
- GPU-accelerated rendering
- Multiple display modes (pixel perfect, 4:3, cropped, TV style)
- SNES enhancement chips (SFX1/2, SA-1, DSP, SDD1, CX4)
- Sound emulation at 32KHz

**Recent Improvements (v1.52):**
- Thread safety fixes
- Improved shutdown stability
- ROM mapping fixes (Mega Man X)
- 8:7 fit scaling option
- Linear filtering option

**Known Issues:**
- Citra emulation broken (libctru 1.5.x+ issue)
- Deprecated CSND service (should migrate to ndsp)
- Minor sound emulation errors
- Poor performance in some SFX games (Doom)
- Mosaics not implemented
- In-frame palette changes imperfect

---

## GPU Architecture Analysis

### Current Implementation: Custom GPU Wrapper

**File:** `source/3dsgpu.cpp` / `source/3dsgpu.h`

**Architecture:**
```
SNES9x Core → 3dsgpu wrapper → gpulib.cpp → PICA200 GPU
```

**Key Components:**

#### 1. SGPU3DS Global State Structure
```c
typedef struct {
    GSPGPU_FramebufferFormat    screenFormat;
    GPU_TEXCOLOR                frameBufferFormat;

    u32                 *frameBuffer;
    u32                 *frameDepthBuffer;

    float               projectionTopScreen[16];
    float               projectionBottomScreen[16];
    float               textureOffset[4];

    SStoredVertexList   vertexesStored[4][10];

    SGPUTexture         *currentTexture;
    SGPUTexture         *currentRenderTarget;
    SGPUTexture         *currentRenderTargetDepth;

    SGPUShader          shaders[3];
    int                 currentShader = -1;

    bool                isReal3DS = false;
    bool                isNew3DS  = false;
    bool                enableDebug = false;
    int                 emulatorState = 0;
} SGPU3DS;
```

**Important Notes:**
- Has New 3DS detection (`isNew3DS`)
- Manages multiple shaders (likely for different SNES modes)
- Uses orthographic projection matrices
- Stores vertex lists for rendering

#### 2. SGPUTexture Structure
```c
typedef struct {
    int             Memory;         // 0 = linear memory, 1 = VRAM
    GPU_TEXCOLOR    PixelFormat;
    u32             Params;
    int             Width;
    int             Height;
    void            *PixelData;
    int             BufferSize;
    float           Projection[4*4];
    float           TextureScale[4];
} SGPUTexture;
```

**Memory Management:**
- Supports both linear RAM and VRAM textures
- Multiple pixel formats (RGBA8, RGB565, etc.)
- Custom projection per texture (interesting for our use case!)

#### 3. Shader System

**Shader Files Found:**
- `shaderfast2.v.pica` / `shaderfast2.g.pica` - Main fast shader (vertex + geometry)
- `shaderfastm7.v.pica` / `shaderfastm7.g.pica` - Mode 7 shader
- `shaderslow.v.pica` - Slow mode shader

**Shader Loading:**
```c
void gpu3dsLoadShader(int shaderIndex, u32 *shaderBinary, int size, int geometryShaderStride);
void gpu3dsUseShader(int shaderIndex);
```

**Analysis:** Separate shaders for Mode 7 vs regular tiles - this will be important for our layer separation.

#### 4. Rendering Pipeline

**Key Functions:**
```c
void gpu3dsStartNewFrame();
void gpu3dsSetRenderTargetToFrameBuffer(gfxScreen_t screen);
void gpu3dsSetRenderTargetToTexture(SGPUTexture *texture, SGPUTexture *depthTexture);
void gpu3dsFlush();
void gpu3dsFrameEnd();
void gpu3dsTransferToScreenBuffer(gfxScreen_t screen);
void gpu3dsSwapScreenBuffers();
```

**Current Flow:**
```
1. gpu3dsStartNewFrame()
2. Render SNES tiles/sprites → vertex lists
3. gpu3dsDrawVertexList() → GPU
4. gpu3dsFlush()
5. gpu3dsTransferToScreenBuffer()
6. gpu3dsSwapScreenBuffers()
```

#### 5. Vertex List System

**Buffer Size:** 2MB for GPU command buffer (increased for heavy effects)

**Vertex Lists:**
- Uses custom `SVertexList` structure
- Double buffering via `gpu3dsSwapVertexListForNextFrame()`
- Stores previous frame data for optimization

---

## SNES Implementation Files

### Critical Files for Layer Separation

#### 1. `source/3dsimpl_gpu.cpp` / `3dsimpl_gpu.h`
**Purpose:** SNES-specific GPU implementation

**Key Functions to Analyze:**
- Tile rendering
- Sprite rendering
- Background layer handling
- Mode 7 handling

**Expected Contents:**
- BG layer compositing logic (what we need to SPLIT)
- Sprite batching
- Window/clipping implementation

#### 2. `source/3dsimpl.cpp` / `3dsimpl.h`
**Purpose:** Main 3DS implementation layer

**Likely Contains:**
- Frame timing
- Input handling
- Menu system integration
- Settings management

#### 3. `source/3dsimpl_tilecache.cpp` / `3dsimpl_tilecache.h`
**Purpose:** Tile caching system

**Importance:** Critical for performance - we need to understand how tiles are cached to avoid redundant texture uploads in stereo mode.

#### 4. `source/Snes9x/` directory
**Purpose:** Core SNES9x emulation

**Files of Interest:**
- PPU rendering code
- Graphics modes
- Transparency/color math

---

## Stereoscopic 3D Implementation Path

### Comparison: Custom GPU vs Citro3D

| Aspect | Current (Custom GPU) | Citro3D |
|--------|---------------------|---------|
| **API Level** | Low-level GPU commands | Higher-level abstraction |
| **Stereoscopic Support** | Manual implementation | Built-in stereo targets |
| **Maintenance** | Custom code to maintain | Maintained by devkitPro |
| **Documentation** | Limited | Well-documented |
| **Learning Curve** | Steep (GPU internals) | Moderate (3D API) |
| **Performance** | Potentially faster | Very good |
| **3D Slider** | Manual polling | Integrated |

### Migration Strategy: Hybrid Approach

**Recommendation:** Keep existing GPU code for now, add Citro3D layer on top for stereo.

**Rationale:**
1. Existing GPU implementation is highly optimized
2. Full migration is risky and time-consuming
3. Citro3D can wrap existing rendering
4. Hybrid allows incremental development

**Hybrid Architecture:**
```
SNES9x Core
    ↓
3dsimpl_gpu (extract layers instead of composite)
    ↓
[NEW] Stereo Layer Manager
    ↓
[NEW] Citro3D Stereo Renderer (left/right eye)
    ↓
Existing 3dsgpu for per-layer rendering
    ↓
PICA200 GPU
```

### Alternative: Full Citro3D Migration

**If we decide to fully migrate:**

**Pros:**
- Cleaner codebase
- Better stereo integration
- Easier to understand
- Modern API

**Cons:**
- Major rewrite required
- Risk of performance regression
- Longer development time
- Potential bugs during migration

**Estimate:** 4-6 weeks additional work vs 1-2 weeks for hybrid.

---

## Stereoscopic Reference: devkitPro Example

**File:** `repos/devkitpro-3ds-examples/graphics/gpu/stereoscopic_2d/source/main.cpp`

### Key Learnings from Example:

#### 1. Enable Stereoscopic Mode
```cpp
gfxSet3D(true); // Activate stereoscopic 3D
```

#### 2. Create Separate Render Targets
```cpp
C3D_RenderTarget* left;
C3D_RenderTarget* right;

left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
```

#### 3. Read 3D Slider
```cpp
float slider = osGet3DSliderState(); // Returns 0.0 to 1.0
```

#### 4. Render Each Eye with Parallax Offset
```cpp
// Left eye view
C2D_SceneBegin(left);
C2D_DrawImageAt(face, 100 + offset * slider, 30, 0);

// Right eye view
C2D_SceneBegin(right);
C2D_DrawImageAt(face, 100 - offset * slider, 30, 0);
```

**Key Insight:** Positive offset for left eye, negative for right eye creates depth INTO screen.

#### 5. Parallax Formula
```
leftX  = baseX + (depth * slider)
rightX = baseX - (depth * slider)

// Where:
// - depth = how far the layer should appear (higher = further)
// - slider = 3D intensity (0.0 = 2D, 1.0 = max 3D)
```

---

## Implementation Plan for SNES9x 3D

### Phase 1: Minimal Stereo Prototype

**Goal:** Render current SNES output in stereo (no layer separation yet)

**Steps:**
1. Add Citro3D/Citro2D to build system
2. Create left/right render targets
3. Duplicate current rendering for both eyes
4. Add small parallax offset to test
5. Integrate 3D slider

**Expected Result:** SNES games display in 3D with single-layer depth

**Complexity:** Low
**Time Estimate:** 1-2 days

### Phase 2: Layer Extraction

**Goal:** Separate SNES layers into individual buffers

**Modify:** `source/3dsimpl_gpu.cpp`

**Current (pseudo-code):**
```cpp
void RenderSNESFrame() {
    // Composite all layers into one buffer
    RenderBG3();
    RenderBG2();
    RenderBG1();
    RenderSprites();
    RenderWindow();

    UploadToGPU(compositeBuffer);
}
```

**New (pseudo-code):**
```cpp
void RenderSNESFrame() {
    // Render each layer to separate buffer
    ExtractLayer(BG3, &layerBuffers[3]);
    ExtractLayer(BG2, &layerBuffers[2]);
    ExtractLayer(BG1, &layerBuffers[1]);
    ExtractLayer(BG0, &layerBuffers[0]);
    ExtractSprites(&spriteBuffer);

    // Pass to stereo renderer
    RenderStereoLayers(layerBuffers, 4, spriteBuffer);
}
```

**Challenges:**
- SNES layers have different properties (scroll, priority, transparency)
- Mode 7 is special (rotation/scaling)
- Window effects clip layers
- Color math (transparency) affects blending

**Complexity:** Medium-High
**Time Estimate:** 1-2 weeks

### Phase 3: Stereo Layer Rendering

**Goal:** Render extracted layers in stereo with depth

**New File:** `source/3dsstereo.cpp` / `3dsstereo.h`

**Structure:**
```cpp
typedef struct {
    SGPUTexture* texture;
    float depth;
    bool enabled;
    bool transparent;
    int priority;
} StereoLayer;

typedef struct {
    StereoLayer layers[6];  // BG0-3 + sprites + window
    int activeCount;
    float sliderValue;
} StereoFrameData;

void RenderStereoFrame(StereoFrameData* data) {
    // Sort layers by depth (back to front)
    SortLayersByDepth(data);

    // Render left eye
    C3D_FrameDrawOn(leftTarget);
    for (each layer) {
        float offsetX = layer.depth * data->sliderValue;
        RenderLayerWithOffset(layer, +offsetX);
    }

    // Render right eye
    C3D_FrameDrawOn(rightTarget);
    for (each layer) {
        float offsetX = layer.depth * data->sliderValue;
        RenderLayerWithOffset(layer, -offsetX);
    }
}
```

**Complexity:** Medium
**Time Estimate:** 1 week

### Phase 4: Depth Configuration

**Goal:** Per-game and per-mode depth settings

**Files:**
- `data/depth_profiles.json` - Game-specific settings
- `source/3dsdepthconfig.cpp` - Loading/management

**Complexity:** Low
**Time Estimate:** 3-4 days

### Phase 5: Advanced Features

**Features:**
- Per-scanline depth (Mode 7 gradients)
- Sprite depth separation
- UI/HUD detection and handling
- Game-specific handlers

**Complexity:** High
**Time Estimate:** 2-3 weeks

---

## Missing Features Analysis

### 1. Savestates

**Current Status:** Not implemented

**Why Missing:** Complex state serialization, especially with GPU state

**Implementation Plan:**

**Files to Create:**
- `source/3dssavestate.cpp` / `3dssavestate.h`

**State to Save:**
```cpp
typedef struct {
    // SNES state (from Snes9x core)
    uint8 cpu[CPU_STATE_SIZE];
    uint8 ppu[PPU_STATE_SIZE];
    uint8 wram[128 * 1024];
    uint8 vram[64 * 1024];
    uint8 saveram[SAVERAM_SIZE];

    // Audio state
    uint8 apu[APU_STATE_SIZE];

    // Frame counter
    uint32 frameNumber;

    // Timestamp
    uint64 timestamp;

    // ROM identity
    char romName[256];
    uint32 romCRC;

    // Screenshot (for UI)
    uint8 screenshot[256 * 224 * 3]; // RGB

    // Optional: 3D depth settings
    float layerDepths[6];
} SaveState;
```

**Savestate Slots:** 10 slots (F1-F10 or menu-based)

**Storage Location:** `sd:/3ds/snes9x_3ds/states/<ROM_NAME>/state_<N>.sav`

**Functions:**
```cpp
bool SaveState_Save(int slot);
bool SaveState_Load(int slot);
bool SaveState_Exists(int slot);
void SaveState_GetPreviewImage(int slot, uint8* outBuffer);
```

**Complexity:** Medium
**Time Estimate:** 1 week

### 2. Fast Forward

**Current Status:** Not implemented

**Implementation:**

**Simple Approach:**
```cpp
bool fastForwardEnabled = false;
int fastForwardMultiplier = 2; // 2x by default

void EmulationLoop() {
    int framesToRun = fastForwardEnabled ? fastForwardMultiplier : 1;

    for (int i = 0; i < framesToRun; i++) {
        RunSNESFrame();

        // Only render last frame
        if (i == framesToRun - 1) {
            RenderFrame();
        }
    }

    // Skip audio during fast forward
    if (!fastForwardEnabled) {
        PlayAudio();
    }
}
```

**Controls:**
- Hold R = 2x fast forward
- Hold R + A = 4x fast forward (New 3DS only)
- Or configurable hotkey

**UI Indicator:** "FF 2x" or "FF 4x" overlay

**Complexity:** Low
**Time Estimate:** 2-3 days

---

## Performance Considerations

### Current Performance (matbo87 fork)

**Old 3DS:** Variable (268 MHz CPU)
- Most games: 50-60 FPS
- SFX chip games: <30 FPS (Doom, Yoshi's Island)
- Mode 7 games: 50-60 FPS

**New 3DS:** Excellent (804 MHz CPU)
- Most games: 60 FPS locked
- SFX chip games: 45-60 FPS
- Mode 7 games: 60 FPS locked

### Expected Performance with Stereo Rendering

**Old 3DS:** Not recommended
- Stereo = 2x pixel fill + layer overhead
- Likely <30 FPS on most games
- Not worth supporting

**New 3DS:** Target platform
- With optimization: 60 FPS achievable
- May need reduced complexity on heavy scenes
- Layer caching will be critical

### Optimization Strategies

#### 1. New 3DS CPU Boost
```cpp
// Enable in main.cpp
osSetSpeedupEnable(true); // 804 MHz + L2 cache
```

#### 2. Texture Caching
**Current System:** `3dsimpl_tilecache.cpp` already caches tiles

**Enhancement for Stereo:**
- Cache per-layer textures
- Only update changed tiles
- Use dirty flags from SNES VRAM writes

#### 3. GPU State Management
**Minimize state changes between layers:**
- Group layers by shader
- Batch draw calls
- Reuse textures where possible

#### 4. Layer Culling
**Optimization:**
```cpp
if (!layer.enabled) continue;           // Layer disabled this frame
if (layer.fullyTransparent) continue;   // Layer is invisible
if (layer.offscreen) continue;          // Layer scrolled off screen
```

#### 5. Frame Pacing
**Use VSync:**
```cpp
gfxWaitForVSync(); // Ensure smooth 60 FPS
```

**Monitor frame time:**
```cpp
#ifdef DEBUG
    u64 frameStart = svcGetSystemTick();
    RenderFrame();
    u64 frameEnd = svcGetSystemTick();
    float ms = (frameEnd - frameStart) / CPU_TICKS_PER_MSEC;
    if (ms > 16.7f) {
        printf("Warning: Frame took %.2fms\n", ms);
    }
#endif
```

---

## Memory Budget

### Available Resources (New 3DS)

**Total RAM:** 256 MB
**VRAM:** 10 MB (FCRAM used as VRAM)
**Linear Memory:** ~40 MB available for homebrew

### Current Memory Usage (Estimated)

```
Emulator Code:          ~2 MB
SNES RAM:               128 KB
SNES VRAM:              64 KB
SNES Save RAM:          Up to 128 KB
Sound Buffer:           64 KB
GPU Command Buffers:    2 MB (COMMAND_BUFFER_SIZE)
Tile Cache:             ~1 MB (estimate)
UI/Textures:            ~1 MB
Total Current:          ~6-7 MB
```

### Additional Memory for Stereo (Estimated)

```
Per-Layer Buffers:
- BG0: 256x224x4 = 224 KB
- BG1: 256x224x4 = 224 KB
- BG2: 256x224x4 = 224 KB
- BG3: 256x224x4 = 224 KB
- Sprites: 256x224x4 = 224 KB
- Window: 256x224x4 = 224 KB
Total Layer Buffers: 1.3 MB

GPU Textures (VRAM):
- 6 textures @ ~256KB each = 1.5 MB

Depth Config Data: <1 MB

Savestate Slots (optional):
- 10 slots @ ~512KB = 5 MB

Total Additional: ~8 MB
Grand Total: ~15 MB
```

**Conclusion:** Plenty of headroom. Memory is not a constraint.

---

## Risk Assessment

### High Risks

**1. Performance Regression**
- **Risk:** Stereo rendering too slow for 60 FPS
- **Mitigation:** Profile early, optimize aggressively, New 3DS only
- **Likelihood:** Medium
- **Impact:** High

**2. Visual Artifacts**
- **Risk:** Layer separation breaks certain visual effects
- **Mitigation:** Extensive testing, per-game fixes
- **Likelihood:** High
- **Impact:** Medium

**3. Complexity Underestimation**
- **Risk:** Layer extraction harder than expected
- **Mitigation:** Incremental development, prototype early
- **Likelihood:** Medium
- **Impact:** High

### Medium Risks

**4. Citro3D Integration Issues**
- **Risk:** Conflicts with existing GPU code
- **Mitigation:** Hybrid approach, careful API boundaries
- **Likelihood:** Medium
- **Impact:** Medium

**5. Game-Specific Edge Cases**
- **Risk:** Some games use unique tricks that break
- **Mitigation:** Whitelist/blacklist, per-game handlers
- **Likelihood:** High
- **Impact:** Low (affects few games)

### Low Risks

**6. Savestate Implementation**
- **Risk:** Complex state to serialize
- **Mitigation:** Use existing Snes9x savestate code as reference
- **Likelihood:** Low
- **Impact:** Medium

---

## Recommended Development Order

### Sprint 1: Foundation (Week 1)
1. Set up build environment
2. Fork matbo87's repository
3. Build and test on New 3DS XL
4. Add Citro3D to build system
5. Create minimal stereo test (duplicate current output)

**Deliverable:** SNES game displays in stereo (no layer separation yet)

### Sprint 2: Layer Extraction (Weeks 2-3)
1. Analyze `3dsimpl_gpu.cpp` rendering
2. Identify BG layer rendering code
3. Modify to output separate layer buffers
4. Test with simple game (Super Mario World)

**Deliverable:** Layer buffers extracted successfully

### Sprint 3: Stereo Rendering (Week 4)
1. Implement `3dsstereo.cpp` module
2. Create left/right eye rendering loop
3. Apply depth offsets based on layer
4. Integrate 3D slider
5. Test depth effect

**Deliverable:** Convincing 3D effect with separated layers

### Sprint 4: Configuration & Polish (Week 5)
1. Create depth configuration system
2. Add default depth profiles
3. Implement in-emulator depth adjustment UI
4. Test with 10+ games
5. Optimize performance

**Deliverable:** Playable stereo 3D for multiple games

### Sprint 5: Additional Features (Week 6)
1. Implement savestates
2. Add fast-forward
3. Create per-game depth database
4. Documentation

**Deliverable:** Feature-complete v1.0

### Sprint 6: Advanced Features (Weeks 7-8)
1. Per-scanline depth for Mode 7
2. Sprite depth separation
3. Special game handlers
4. Community testing

**Deliverable:** Polished v2.0

---

## Next Steps

### Immediate Actions

1. **Set Up Development Environment**
   - Install devkitPro pacman
   - Install devkitARM, libctru, Citro3D
   - Configure New 3DS XL for development

2. **Build Baseline**
   ```bash
   cd repos/matbo87-snes9x_3ds
   make
   # Test .3dsx on New 3DS XL
   ```

3. **Study GPU Implementation**
   - Read `3dsimpl_gpu.cpp` in detail
   - Map out rendering pipeline
   - Identify layer compositing code

4. **Create Stereo Prototype**
   - Add Citro3D to Makefile
   - Implement basic stereo output
   - Test with single game

### Questions to Answer Through Code Analysis

1. **Where are layers composited?**
   - Likely in `3dsimpl_gpu.cpp`
   - Need to find exact function

2. **How are tiles cached?**
   - Review `3dsimpl_tilecache.cpp`
   - Understand cache invalidation

3. **How does Mode 7 work?**
   - Special shader: `shaderfastm7.*`
   - Different rendering path?

4. **Where is the frame rendered?**
   - Find main rendering loop
   - Identify entry point for stereo injection

### Documentation Needed

- [ ] GPU rendering call graph
- [ ] Layer rendering sequence diagram
- [ ] Memory layout diagram
- [ ] Shader analysis document

---

## Conclusion

**Feasibility:** High
**Complexity:** Medium-High
**Time to MVP:** 5-6 weeks
**Time to v1.0:** 8-10 weeks
**Time to v2.0:** 12-14 weeks

**Key Success Factors:**
1. Base on solid matbo87 fork
2. Incremental development (prototype → MVP → v1.0 → v2.0)
3. New 3DS XL only (performance critical)
4. Hybrid GPU approach (minimize risk)
5. Extensive testing on diverse games

**This is achievable!** The existing codebase provides a strong foundation, and the stereoscopic technique is well-proven by M2's GigaDrive work.

---

**Last Updated:** 2025-10-20
**Author:** Technical Analysis for SNES 3D Layer Separation Project
**Status:** Ready to Begin Development
