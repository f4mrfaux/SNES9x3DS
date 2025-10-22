# SNES9x 3DS Rendering Pipeline - Complete Analysis

## 1. RENDERING ARCHITECTURE OVERVIEW

### 1.1 High-Level Flow
```
S9xMainLoop() (CPU emulation)
    ↓
IPPU.RenderThisFrame = true
    ↓
S9xRenderScreenHardware(bool8 sub, bool8 force_no_add, uint8 D)
    ↓
GPU Rendering to Texture (VRAM)
    ↓
Screen Transfer & Display (Frame Buffer)
    ↓
gfxScreenSwapBuffers()
```

### 1.2 Frame Timing
- **Target:** NTSC 60fps (16,667 µs per frame)
- **Implementation:** GPU command buffers (1MB allocated for heavy effects)
- **Current:** CPU rendering with GPU acceleration for compositing

## 2. CURRENT RENDERING PIPELINE

### 2.1 Main Rendering Loop (`impl3dsRunOneFrame()`)
**File:** `/source/3dsimpl.cpp:502`

```
impl3dsRunOneFrame(bool firstFrame, bool skipDrawingFrame)
├── Apply speed hack patches
├── Set Main Screen Texture as render target
├── Use Shader 1 (tile drawing shader)
├── S9xMainLoop() - CPU emulation with PPU graphics rendering
├── Set Frame Buffer as render target
├── Copy rendered textures to 3DS frame buffer
│   ├── Border (if enabled)
│   └── Main screen content
├── Use Shader 0 (copy-to-screen shader)
├── Transfer to screen buffer
├── Swap screen buffers
└── gpu3dsFlush() - Submit GPU commands
```

### 2.2 GPU Infrastructure

**File:** `/source/3dsgpu.cpp` and `/source/3dsgpu.h`

**Key Components:**
- **Vertex Lists:** Double-buffered for frame-by-frame rendering
- **Command Buffer:** 1MB for GPU commands (2 x 512KB buffers)
- **Shaders:**
  - Shader 0: Copy to screen (slow shader)
  - Shader 1: Draw tiles (fast 2 shader with geometry shader)
  - Shader 2: Mode 7 rendering (fastm7 with geometry shader)

**Texture Management:**
- **Frame Buffer:** 256x256 RGBA8 (256KB) in VRAM
- **Depth Buffer:** 256x256 RGBA8 (256KB) in VRAM
- **Tile Cache:** 1024x1024 RGBA5551 (2MB) in linear memory
- **Mode 7 Full:** 1024x1024 RGBA4 (2MB) in VRAM
- **Mode 7 Tile Cache:** 128x128 RGBA4 (32KB) in linear memory

## 3. PPU EMULATION & LAYER RENDERING

### 3.1 PPU Structure
**File:** `/source/Snes9x/ppu.h`

**Background Layers (BG 0-3):**
- Support for different bit depths (2-bit, 4-bit, 8-bit tiles)
- Tile cache system for performance
- Palette management (16 palettes for 4-color/16-color BGs)

**Sprite System (OBJ):**
- Up to 128 sprites per frame
- Variable sizes (8x8, 16x16, 32x32, etc.)
- Priority ordering with background layers

### 3.2 Graphics Rendering Entry Point
**File:** `/source/Snes9x/gfxhw.cpp:3415`

```
S9xRenderScreenHardware(bool8 sub, bool8 force_no_add, uint8 D)
├── Determine which layers are visible on main/sub screens
├── Calculate blending alphas
├── For each layer priority level:
│   ├── DRAW_OBJS: S9xDrawOBJSHardware() - Render sprites
│   ├── DRAW_4COLOR_BG: S9xDrawBackgroundHardwarePriority0Inline_4Color()
│   ├── DRAW_16COLOR_BG: S9xDrawBackgroundHardwarePriority0Inline_16Color()
│   ├── DRAW_256COLOR_BG: S9xDrawBackgroundHardwarePriority0Inline_256Color()
│   └── (Handle offset/Mode7 variants)
└── Handle backdrops and color math
```

### 3.3 Background Layer Rendering
**File:** `/source/Snes9x/gfxhw.cpp`

**Process:**
1. **Tile Lookup:** Convert tile address + palette to VRAM cache position
2. **Cache Check:** `cache3dsGetTexturePositionFast()`
3. **Tile Rendering:** `S9xDrawBackgroundHardwarePriority0Inline()`
   - Uses GPU_GEOMETRY_PRIM (geometry shader on real 3DS)
   - Falls back to GPU_TRIANGLES with 6 vertices per tile on Citra

**Key Functions:**
- `S9xDrawBackgroundHardwarePriority0Inline_4Color()` - 4-color BGs
- `S9xDrawBackgroundHardwarePriority0Inline_16Color()` - 16-color BGs
- `S9xDrawBackgroundHardwarePriority0Inline_256Color()` - 256-color BGs
- `S9xDrawBackgroundHardwarePriority0Inline()` - Generic handler

### 3.4 Sprite Rendering
**File:** `/source/Snes9x/gfxhw.cpp:2578`

```
S9xDrawOBJSHardware(bool8 sub, int depth = 0, int priority = 0)
├── Get OBJ list for current scanline
├── For each sprite on scanline:
│   ├── S9xDrawOBJTileHardware2() - Render individual sprite tiles
│   └── Handle priority levels
└── Build vertex data for GPU rendering
```

**Sprite Features:**
- OBJ priority bits determine render order relative to backgrounds
- Horizontal/vertical flipping support
- Size attributes for variable sprite dimensions

### 3.5 Mode 7 Rendering
**File:** `/source/Snes9x/gfxhw.cpp`

- **Mode 7 Preparation:** `S9xPrepareMode7()` - Prepare Mode 7 tiles/texture
- **Tile Drawing:** `S9xDrawBackgroundMode7Hardware()` - Render Mode 7 tiles
- **Scanline Drawing:** `S9xDrawBackgroundMode7HardwareRepeatTile0()` - Draw scanlines

## 4. TRANSPARENCY & BLENDING

### 4.1 Alpha Blending Modes
**File:** `/source/3dsgpu.cpp`

```c
gpu3dsEnableAlphaBlending()      // Default: SRC_ALPHA, ONE_MINUS_SRC_ALPHA
gpu3dsEnableAdditiveBlending()   // DST_ALPHA, ONE
gpu3dsEnableSubtractiveBlending()// Reverse subtract
gpu3dsEnableAdditiveDiv2Blending()
gpu3dsEnableSubtractiveDiv2Blending()
gpu3dsDisableAlphaBlending()     // ONE, ZERO (no blend)
```

### 4.2 Alpha Test
```c
gpu3dsEnableAlphaTestNotEqualsZero()   // Skip alpha=0 pixels (transparency)
gpu3dsEnableAlphaTestEqualsOne()       // Skip non-opaque pixels
gpu3dsEnableAlphaTestGreaterThanEquals(uint8 alpha)
```

### 4.3 Stencil Testing
- Used for window masks and clipping
- `gpu3dsEnableStencilTest()` / `gpu3dsDisableStencilTest()`
- Supports various test functions (NEVER, ALWAYS, EQUAL, NOTEQUAL, etc.)

### 4.4 Depth Testing
- Used for layer priority ordering
- `gpu3dsEnableDepthTest()` / `gpu3dsDisableDepthTest()`
- Depth values encode layer priority (depth = priority * 256 + alpha_value)

## 5. SCREEN UPDATE & DISPLAY PIPELINE

### 5.1 Screen Rendering (`impl3dsRunOneFrame`)
**File:** `/source/3dsimpl.cpp:502`

```
Main Screen Target Texture (256x256) VRAM
    ↓ [All layers rendered here]
    ↓
Frame Buffer (256x256) VRAM
    ↓ [Scale/crop and display]
    ↓
3DS Screen (Top 400x240 or Bottom 320x240)
```

### 5.2 Buffer Transfer Pipeline
**File:** `/source/3dsgpu.cpp:1049`

```
gpu3dsTransferToScreenBuffer(gfxScreen_t screen)
├── Wait for previous flush with gspWaitForPreviousFlush()
└── GX_DisplayTransfer()
    ├── Source: Frame Buffer (RGBA8)
    ├── Destination: Screen buffer (RGBA8/RGB565/RGBA4)
    └── Format conversion applied
```

### 5.3 Screen Swap
```c
gpu3dsSwapScreenBuffers()
├── gfxScreenSwapBuffers(GFX_TOP, false)
└── gfxScreenSwapBuffers(GFX_BOTTOM, false)
```

## 6. 3DS-SPECIFIC FEATURES

### 6.1 Parallax Barrier / 3D Slider Support
**File:** `/source/3dsgpu.cpp:137-170`

```c
void gpu3dsSetParallaxBarrier(bool enable)
{
    u32 reg_state = enable ? 0x00010001: 0x0;
    GSPGPU_WriteHWRegs(0x202000, &reg_state, 4);
}

void gpu3dsCheckSlider()
{
    float sliderVal = *(float*)0x1FF81080;  // Read 3D slider position
    if (sliderVal < 0.6)
        gpu3dsSetParallaxBarrier(false);    // 2D mode
    else
        gpu3dsSetParallaxBarrier(true);     // 3D mode active
}
```

**Current Status:** 
- Only enables/disables parallax barrier
- No actual stereoscopic layer separation implemented
- Top/Bottom screen rendering to separate LCD views not implemented

### 6.2 Screen Selection
```c
// Projection matrices for top/bottom screens
matrix3dsInitOrthographic(GPU3DS.projectionTopScreen, 0, 400, 0, 240, 0, 1)
matrix3dsInitOrthographic(GPU3DS.projectionBottomScreen, 0, 320, 0, 240, 0, 1)
```

## 7. TILE CACHE SYSTEM

### 7.1 Tile Caching Mechanism
**File:** `/source/3dsimpl_tilecache.cpp` and `/source/3dsimpl_tilecache.h`

**Purpose:** Convert SNES tiles from bitplane format to GPU-ready textures

**Process:**
1. **Hash Lookup:** `cache3dsGetTexturePositionFast(tileAddr, palette)`
   - Create hash from tile VRAM address + palette
   - Look up cached texture position
2. **Cache Miss:** Allocate new texture position
   - Store hash-to-position mapping
   - Invalidate palette cache
3. **Tile Conversion:** `cache3dsCacheSnesTileToTexturePosition()`
   - Convert bitplane format → RGBA5551
   - Store in tile cache texture (1024x1024)

**Optimization Features:**
- Palette tracking prevents redundant conversions
- LRU-like replacement for cache positions
- Mode 7 tiles use separate cache (128x128)

## 8. PERFORMANCE OPTIMIZATION TECHNIQUES

### 8.1 GPU-Accelerated Features
1. **Geometry Shaders:**
   - Real 3DS: Geometry shaders expand 2-vertex lines to quads (2 points → 6 vertices)
   - Citra: Falls back to CPU vertex expansion

2. **Tiling Optimization:**
   - Adjacent tiles rendered as multi-quads in single draw call
   - Reduces CPU overhead for background composition

3. **Depth-Based Priority:**
   - Layer ordering handled by GPU depth buffer
   - Avoids sorting on CPU

### 8.2 Memory Management
- **Dual Command Buffers:** Double-buffering reduces GPU stalls
- **Linear vs VRAM Memory:**
  - Tile cache: Linear memory (CPU reads for conversion)
  - Frame/Depth buffers: VRAM (fast GPU access)
- **Vertex List Flipping:** 1MB allocated but halved per frame (504KB used, 504KB waiting)

### 8.3 Timing & Synchronization
**File:** `/source/3dsgpu.cpp:1015`

```c
void gpu3dsWaitForPreviousFlush()
{
    if (somethingWasFlushed)
    {
        if (GPU3DS.isReal3DS)
            gspWaitForP3D();  // Wait for GPU to complete
        somethingWasFlushed = false;
    }
}
```

### 8.4 Performance Timing Infrastructure
**Timing markers placed at:**
- RenderScnHW (overall screen render)
- DrawBKClr (backdrop)
- DrawBG0-3 (individual backgrounds)
- DrawOBJS (sprites)
- CopyFB (frame buffer copy)
- Transfer (screen transfer)
- Flush (GPU flush)

## 9. CURRENT GPU RENDERING STATUS

### 9.1 GPU Rendering Implementation
**Current Status:** GPU-ACCELERATED (Partial)

**What's GPU-Rendered:**
- Tile assembly and quads (geometry shader accelerated)
- Blending operations
- Depth/priority ordering
- Screen transfer/scaling

**What's CPU-Rendered:**
- Tile data conversion (SNES bitplane → GPU format)
- Layer composition logic (which tiles to draw)
- Sprite/OBJ ordering
- Palette management

### 9.2 Rendering Pipeline (Hybrid)
```
CPU: Tile conversion (bitplane → texture)
     ↓
GPU: Vertex assembly (2-point tiles → quads)
     ↓
GPU: Depth testing & priority
     ↓
GPU: Alpha blending
     ↓
GPU: Screen compositing
     ↓
GPU: Frame buffer scaling/transfer
```

## 10. WHERE TO INJECT STEREO LAYER SEPARATION

### 10.1 Potential Injection Points

#### **Option 1: Parallax Barrier Direct Control (Easiest)**
**Location:** `/source/3dsgpu.cpp:152-170`

```c
void gpu3dsCheckSlider()
{
    float sliderVal = *(float*)0x1FF81080;
    
    // Current: Only enables/disables parallax barrier
    // NEW: Could map slider value to depth offset amount
    
    if (sliderVal > 0.6)
    {
        // Calculate depth offset based on slider value
        float depthOffset = sliderVal * MAX_DEPTH_OFFSET;
        gpu3dsSetLeftEyeOffset(-depthOffset);
        gpu3dsSetRightEyeOffset(+depthOffset);
    }
}
```

#### **Option 2: Per-Layer Depth Manipulation (Medium Difficulty)**
**Location:** `/source/Snes9x/gfxhw.cpp:3415-3950`

```c
void S9xRenderScreenHardware(bool8 sub, ...)
{
    // For each layer being rendered:
    // Apply stereo depth based on:
    // - Layer type (BG0-3, OBJ)
    // - Priority
    // - Game-specific configuration
    
    int stereoDepth = calculateStereoDepth(layerType, priority);
    gpu3dsSetLayerDepthOffset(stereoDepth);
}
```

#### **Option 3: Render to Left/Right Framebuffers (Most Complex)**
**Location:** `/source/3dsgpu.cpp:926-974`

```c
// Render scene twice - once for each eye
gpu3dsSetRenderTargetToFrameBuffer(GFX_LEFT_EYE);  // NEW
gpu3dsRenderScene();

gpu3dsSetRenderTargetToFrameBuffer(GFX_RIGHT_EYE); // NEW
gpu3dsRenderScene();

gpu3dsDisplayStereoFramebuffers(); // NEW - combine views
```

### 10.2 Recommended Approach: Option 2

**Advantages:**
- Minimal code changes
- Works with existing rendering pipeline
- Per-layer control enables game-specific tuning
- Can use depth encoding already in place

**Implementation Steps:**
1. Add stereo depth configuration parameters
2. Modify depth calculations in layer rendering
3. Add per-layer depth offset based on:
   - Layer priority
   - Layer type (background vs sprite)
   - Game/mode settings
4. Pass modified depth to GPU

## 11. POTENTIAL CHALLENGES & CONSIDERATIONS

### 11.1 Technical Challenges
1. **Framebuffer Limitations:**
   - 3DS top screen is 800x240 effective (2 views @ 400x240)
   - Current rendering uses 256x256 texture
   - May need to adjust or create dual framebuffers

2. **Geometry Shader Constraints:**
   - Real 3DS geometry shaders do 2→6 vertex expansion
   - Would need modification for stereo projection

3. **Parallax Barrier Interaction:**
   - Physical barrier has fixed interleaving pattern
   - Pixel offset precision matters

4. **Mode 7 Complexity:**
   - Uses different rendering path entirely
   - May require separate stereo implementation

### 11.2 Performance Considerations
1. **Double Rendering:**
   - Drawing for both eyes increases GPU workload by ~50-100%
   - Limited GPU bandwidth on 3DS

2. **Precision Issues:**
   - 3DS LCD has fixed ~20-30 pixel separation at design distance
   - Incorrect offset breaks parallax effect

3. **Memory Usage:**
   - Dual framebuffers would use additional VRAM
   - Already at tight memory constraints

### 11.3 Software Compatibility
1. **Game-Specific Tuning:**
   - Different games need different depth offsets
   - May need per-game configuration database

2. **Aspect Ratio Issues:**
   - Stereo can distort aspect ratio perception
   - Horizontal scaling affects eye separation

3. **Fast-Scrolling Issues:**
   - Rapid movement with stereo can cause artifacts
   - May need depth smoothing filters

## 12. KEY FILE LOCATIONS & FUNCTIONS

| Component | File | Line | Function |
|-----------|------|------|----------|
| Main Loop | 3dsimpl.cpp | 502 | `impl3dsRunOneFrame()` |
| Frame Preparation | 3dsimpl.cpp | 490 | `impl3dsPrepareForNewFrame()` |
| PPU Rendering | gfxhw.cpp | 3415 | `S9xRenderScreenHardware()` |
| BG Rendering | gfxhw.cpp | 1509 | `S9xDrawBackgroundHardwarePriority0Inline()` |
| Sprite Rendering | gfxhw.cpp | 2578 | `S9xDrawOBJSHardware()` |
| Mode 7 Rendering | gfxhw.cpp | 3146 | `S9xDrawBackgroundMode7Hardware()` |
| 3D Slider | 3dsgpu.cpp | 152 | `gpu3dsCheckSlider()` |
| Parallax Barrier | 3dsgpu.cpp | 140 | `gpu3dsSetParallaxBarrier()` |
| Tile Caching | 3dsimpl_tilecache.cpp | - | `cache3dsGetTexturePositionFast()` |
| Screen Transfer | 3dsgpu.cpp | 1049 | `gpu3dsTransferToScreenBuffer()` |
| GPU Flush | 3dsgpu.cpp | 994 | `gpu3dsFlush()` |
| Depth Test | 3dsgpu.cpp | 172 | `gpu3dsEnableDepthTest()` |

## 13. DATA FLOW DIAGRAM

```
Game ROM
  ↓
CPU (6502 emulation)
  ├─→ PPU Register Writes
  ├─→ VRAM Updates
  └─→ Sprite Position/State
      ↓
PPU (Picture Processing Unit)
  ├─→ Layer Enable/Priority
  ├─→ Window Masking
  └─→ Color Math Settings
      ↓
Graphics Rendering (S9xRenderScreenHardware)
  ├─→ Backdrop Color
  ├─→ BG0-3 Tiles → Tile Cache System
  ├─→ Sprites/OBJ
  └─→ Color Math/Blending
      ↓
Vertex Generation
  └─→ Position/TexCoord/Depth/Alpha
      ↓
GPU Rendering (3DS GPU)
  ├─→ Vertex Assembly (Real 3DS: Geometry Shader)
  ├─→ Alpha Test
  ├─→ Stencil Test
  ├─→ Depth Test
  ├─→ Alpha Blending
  └─→ Render to Frame Buffer
      ↓
Screen Transfer (GX_DisplayTransfer)
  ├─→ Format Conversion
  ├─→ Scaling
  └─→ Copy to Screen Buffer
      ↓
3DS LCD Display
  ├─→ Parallax Barrier (if enabled)
  └─→ Screen Output
```

## 14. SUMMARY

The SNES9x 3DS rendering pipeline is a **hybrid CPU-GPU system** that:

1. **CPU Side:**
   - Runs full SNES emulation including PPU
   - Converts SNES tiles from bitplane format to GPU textures
   - Determines layer visibility and blending parameters
   - Builds vertex lists for GPU rendering

2. **GPU Side:**
   - Assembles and transforms vertices
   - Applies depth/alpha/stencil tests
   - Performs alpha blending with various modes
   - Transfers final image to LCD screen

3. **Current 3D Support:**
   - Parallax barrier control exists but only turns on/off
   - No actual stereoscopic rendering implemented
   - Single 256x256 framebuffer (not stereo-aware)

4. **Optimization Techniques:**
   - Geometry shaders for vertex expansion (real 3DS)
   - Tile caching system for performance
   - Dual command buffers for GPU pipelining
   - Depth-based layer ordering

5. **Stereo Injection Opportunity:**
   - Best approach: Modify depth calculations per layer
   - Minimal impact on existing rendering pipeline
   - Can leverage existing depth buffer infrastructure
   - Key entry point: `S9xRenderScreenHardware()` depth parameter

