# SNES9x 3DS Rendering Pipeline - Architecture Summary

## Quick Reference

### Key Files
- **Main Loop:** `/source/3dsimpl.cpp` (line 502)
- **PPU Rendering:** `/source/Snes9x/gfxhw.cpp` (line 3415)
- **GPU Interface:** `/source/3dsgpu.cpp` and `3dsgpu.h`
- **Extended GPU:** `/source/3dsimpl_gpu.cpp` and `3dsimpl_gpu.h`
- **Tile Cache:** `/source/3dsimpl_tilecache.cpp` and `3dsimpl_tilecache.h`

### Quick Stats
- **Frame Size:** 256x256 RGBA8 (no stereoscopic support currently)
- **Tile Cache:** 1024x1024 (SNES tiles converted to GPU format)
- **GPU Command Buffer:** 1MB (2x 512KB double-buffered)
- **Vertex Buffers:** Multiple double-buffered lists for quads, tiles, rectangles
- **Shaders:** 3 (copy-to-screen, tile drawing, Mode 7)

---

## Complete Rendering Flow

```
FRAME START
│
├─→ gpu3dsStartNewFrame()
│   └─→ impl3dsPrepareForNewFrame()
│       ├─→ gpu3dsSwapVertexListForNextFrame(&quadVertexes)
│       ├─→ gpu3dsSwapVertexListForNextFrame(&tileVertexes)
│       └─→ gpu3dsSwapVertexListForNextFrame(&rectangleVertexes)
│
├─→ gpu3dsSetRenderTargetToMainScreenTexture()
│
├─→ gpu3dsUseShader(1)  // Tile drawing shader
│
├─→ S9xMainLoop()
│   │
│   ├─→ (CPU 6502 emulation for 1 frame)
│   │
│   └─→ IPPU.RenderThisFrame = true triggers:
│       │
│       └─→ S9xRenderScreenHardware(bool8 sub, bool8 force_no_add, uint8 D)
│           │
│           ├─→ S9xDrawBackdropHardware(sub, depth)
│           │   └─→ gpu3dsAddRectangleVertexes() - Add backdrop rectangles
│           │
│           ├─→ For Each Priority Level (0-3):
│           │   │
│           │   ├─→ S9xDrawOBJSHardware(sub, depth, priority)
│           │   │   └─→ S9xDrawOBJTileHardware2()  // Sprite tiles
│           │   │
│           │   ├─→ S9xDrawBackgroundHardware*()
│           │   │   └─→ S9xDrawBackgroundHardwarePriority0Inline_4Color()
│           │   │   └─→ S9xDrawBackgroundHardwarePriority0Inline_16Color()
│           │   │   └─→ S9xDrawBackgroundHardwarePriority0Inline_256Color()
│           │   │
│           │   └─→ Handle Mode 7:
│           │       └─→ S9xDrawBackgroundMode7Hardware()
│           │
│           └─→ Handle color math / blending modes
│
├─→ gpu3dsSetRenderTargetToFrameBuffer()
│
├─→ gpu3dsUseShader(0)  // Copy-to-screen shader
│
├─→ Render border (if enabled)
│   └─→ gpu3dsBindTexture(borderTexture)
│   └─→ gpu3dsAddQuadVertexes()  // Position border
│
├─→ Render main screen
│   └─→ gpu3dsBindTextureMainScreen()
│   └─→ gpu3dsAddQuadVertexes()  // Position & scale game
│
├─→ gpu3dsDrawVertexes()
│   └─→ gpu3dsDrawVertexList()  // All vertex lists
│
├─→ gpu3dsFlush()
│   └─→ GPUCMD_Split()
│   └─→ GX_ProcessCommandList()  // Submit to GPU
│
├─→ gpu3dsTransferToScreenBuffer()
│   └─→ gpu3dsWaitForPreviousFlush()
│   └─→ GX_DisplayTransfer()  // Transfer frame to LCD
│
├─→ gpu3dsSwapScreenBuffers()
│
└─→ FRAME END
```

---

## Layer Rendering Hierarchy

```
Rendering Priority (Back to Front):
═════════════════════════════════════

1. BACKDROP COLOR (always drawn first)
   └─→ Full-screen color rectangle

2. PRIORITY 0 LAYERS (farthest back)
   ├─→ OBJ Sprites (priority 0)
   ├─→ BG Layer 0 (priority 0)
   ├─→ BG Layer 1 (priority 0)
   ├─→ BG Layer 2 (priority 0)
   └─→ BG Layer 3 (priority 0)

3. PRIORITY 1 LAYERS
   ├─→ OBJ Sprites (priority 1)
   ├─→ BG Layer 0 (priority 1)
   ├─→ BG Layer 1 (priority 1)
   ├─→ BG Layer 2 (priority 1)
   └─→ BG Layer 3 (priority 1)

4. PRIORITY 2 LAYERS
   └─→ (same structure)

5. PRIORITY 3 LAYERS (closest to viewer)
   └─→ (same structure)

Note: Rendered using GPU depth test (GPU_GEQUAL)
Depth = priority * 256 + alpha_component
```

---

## Data Structures for Rendering

### Vertex Types

```c
// Regular tiles/quads
struct STileVertex {
    SVector3i Position;     // x, y, z (z = depth)
    STexCoord2i TexCoord;   // Texture coordinates
};

// Mode 7 tiles
struct SMode7TileVertex {
    SVector4i Position;     // x, y, z, w (w = update frame)
    STexCoord2i TexCoord;   // Texture coordinates
};

// Mode 7 scanlines
struct SMode7LineVertex {
    SVector4i Position;     // x, y, z, w
    STexCoord2f TexCoord;   // Float texture coords
};

// Rectangles (for backdrops, etc.)
struct SVertexColor {
    SVector4i Position;     // x, y, z, w
    u32 Color;             // RGBA packed
};
```

### GPU Texture Types

```
Tile Cache (1024x1024, RGBA5551, Linear Memory)
├─→ Stores converted SNES tiles
├─→ One tile = 8x8 pixels
├─→ Up to 16,384 tiles can fit
└─→ CPU converts bitplane→GPU format here

Main Screen Target (256x256, RGBA8, VRAM)
├─→ Rendered scene for SNES resolution
└─→ Scaled to 3DS screen resolution

Sub Screen Target (256x256, RGBA8, VRAM)
└─→ For sub-screen rendering

Mode 7 Textures:
├─→ Mode 7 Full (1024x1024, RGBA4, VRAM) - Full Mode 7 scene
├─→ Mode 7 Tile Cache (128x128, RGBA4, Linear) - Individual tiles
└─→ Mode 7 Tile 0 (16x16, RGBA4, VRAM) - Special tile 0 repeat

Depth Buffers (256x256, RGBA8, VRAM)
├─→ Depth for screens
└─→ Depth for other textures (512x512)
```

---

## Tile Caching System

```
SNES VRAM
    ↓ (tile address + palette)
Hash Calculation: COMPOSE_HASH(vramAddr, pal)
    ↓
Lookup: vramCacheHashToTexturePosition[hash]
    ↓
├─→ HIT: Found in cache
│   └─→ Return texture position
│
└─→ MISS: New tile
    ├─→ Allocate new position
    ├─→ Store hash→position mapping
    ├─→ Invalidate palette frame count
    └─→ Schedule for conversion
        ↓
Tile Conversion (CPU):
├─→ Extract 8x8 tile from SNES VRAM (bitplane format)
├─→ Convert using palette
├─→ Store as RGBA5551 in tile cache texture
└─→ Ready for GPU rendering

GPU Rendering:
├─→ Bind tile cache texture
├─→ Create quads with texture coordinates
├─→ Depth test sorts by layer priority
└─→ Alpha blend for transparency
```

---

## Background Layer Rendering Detail

```
For BG Layer (0-3):

1. DETERMINE VISIBILITY
   ├─→ Check if layer enabled on main/sub screen
   ├─→ Check if layer is forced off (PPU.BG_Forced)
   └─→ Skip if not visible

2. DETERMINE BIT DEPTH
   └─→ Based on PPU.BGMode and layer config:
       ├─→ 2-bit (4 colors)
       ├─→ 4-bit (16 colors)
       └─→ 8-bit (256 colors)

3. PROCESS EACH TILE
   ├─→ tile_address = BG_VRAM_BASE + (x*y*2)
   ├─→ tile_palette = Get palette index from tile
   ├─→ texturePos = cache3dsGetTexturePositionFast(tile_address, pal)
   │   ├─→ Check if tile is cached
   │   └─→ Queue conversion if not
   │
   └─→ RENDER TILE
       ├─→ Calculate screen coordinates
       ├─→ Create vertex quad:
       │   ├─→ Position: screen_x, screen_y, depth
       │   └─→ TexCoord: texturePos_x, texturePos_y
       ├─→ Add to vertex list
       └─→ GPU will render via geometry shader

4. LAYER COMPLETE
   └─→ Submit all vertex data to GPU for rendering
```

---

## Sprite Rendering Detail

```
For Each OBJ (0-127):

1. CHECK VISIBILITY
   ├─→ Get sprite X, Y, size, priority
   ├─→ Check if on screen
   └─→ Skip if off-screen or disabled

2. GET TILE LIST FOR SCANLINE
   └─→ OBJ can span multiple tiles
       ├─→ Determine which scanlines overlap
       └─→ Get list of tiles for current line

3. FOR EACH TILE IN SPRITE
   ├─→ Get tile data from OAM (sprite attribute memory)
   ├─→ tile_address = OBJ_VRAM_BASE + tile_num
   ├─→ tile_palette = 128 + (sprite_palette * 16)  // OBJ palette
   │
   └─→ HANDLE FLIPPING & SIZE
       ├─→ Check horizontal flip bit
       ├─→ Check vertical flip bit
       ├─→ Adjust texture coordinates accordingly
       └─→ Size: 8x8, 16x16, 32x32, or 64x64
   
   └─→ RENDER TILE
       ├─→ texturePos = cache3dsGetTexturePositionFast()
       ├─→ S9xDrawOBJTileHardware2()
       │   └─→ Create vertex quad with sprite tile
       └─→ Depth = (priority + 1) * 3 * 256 + depth_offset

4. SPRITE COMPLETE
   └─→ All sprite tiles queued for GPU rendering
```

---

## Mode 7 Rendering

```
MODE 7 SPECIAL RENDERING:

Initial Setup:
├─→ S9xPrepareMode7()
│   ├─→ Convert Mode 7 tileset to GPU format
│   └─→ Store in Mode 7 tile cache (128x128)
│
└─→ gpu3dsInitializeMode7Vertexes()
    └─→ Create vertex grid for Mode 7 scene

Rendering:
├─→ S9xDrawBackgroundMode7Hardware()
│   ├─→ For each tile in viewport:
│   │   ├─→ Calculate transformed coordinates
│   │   ├─→ S9xDrawBackgroundMode7HardwareRepeatTile0()
│   │   └─→ Add tile vertices with rotation/scaling
│   │
│   └─→ gpu3dsDrawMode7Vertexes()
│       └─→ Submit Mode 7 geometry shader rendering
│
└─→ Special geometry shader handles transformation matrix

Performance Note:
└─→ Mode 7 uses different buffer/shader than normal BG layers
    └─→ Separate Mode 7 line vertex list
    └─→ Geometry shader applies scaling/rotation
```

---

## Blending & Transparency

```
TRANSPARENCY HANDLING:

1. ALPHA TEST (Per-pixel rejection)
   ├─→ gpu3dsEnableAlphaTestNotEqualsZero()
   │   └─→ Discard pixels with alpha=0 (transparent)
   │
   ├─→ gpu3dsEnableAlphaTestEqualsOne()
   │   └─→ Discard pixels with alpha≠1 (semi-transparent)
   │
   └─→ gpu3dsEnableAlphaTestGreaterThanEquals()
       └─→ Custom alpha threshold

2. ALPHA BLENDING (Output compositing)
   ├─→ gpu3dsEnableAlphaBlending()
   │   └─→ blend = src_alpha * src + (1-src_alpha) * dst
   │
   ├─→ gpu3dsEnableAdditiveBlending()
   │   └─→ blend = src + dst (screen/additive blend)
   │
   └─→ gpu3dsEnableSubtractiveBlending()
       └─→ blend = dst - src

3. STENCIL TEST (Window masking)
   ├─→ gpu3dsEnableStencilTest()
   │   └─→ Apply SNES window masks
   │
   └─→ Stencil operations:
       ├─→ GPU_STENCIL_KEEP    - No change
       ├─→ GPU_STENCIL_ZERO    - Clear stencil
       ├─→ GPU_STENCIL_REPLACE - Set reference value
       └─→ GPU_STENCIL_INCR    - Increment

4. DEPTH TEST (Layer priority)
   ├─→ gpu3dsEnableDepthTest()
   │   └─→ GPU_GEQUAL: Keep pixel if depth >= buffer
   │
   └─→ Depth encoding:
       └─→ depth = (priority * 256) + alpha_value
           ├─→ Higher priority = larger depth (rendered later)
           └─→ Alpha component for sub-pixel ordering
```

---

## 3D Hardware Integration Points

```
3DS HARDWARE FEATURES:

1. PARALLAX BARRIER (3D Slider)
   ├─→ gpu3dsSetParallaxBarrier(bool enable)
   │   └─→ GSPGPU_WriteHWRegs(0x202000, enable)
   │
   └─→ gpu3dsCheckSlider()
       ├─→ Read slider value: *(float*)0x1FF81080
       ├─→ If < 0.6: Disable parallax (2D mode)
       └─→ If >= 0.6: Enable parallax (3D mode)

2. SCREEN SELECTION
   ├─→ gfxSet3D(bool enable)
   │   └─→ Enable/disable 3D processing
   │
   └─→ gfxScreenSwapBuffers(gfxScreen_t screen, bool vblank)
       └─→ Swap top/bottom screen buffers

3. DISPLAY TRANSFER
   └─→ GX_DisplayTransfer()
       ├─→ Format conversion (RGBA8 → screen format)
       ├─→ Scaling (256x256 → 400x240)
       └─→ Copy to screen framebuffer

Current 3D Support: LIMITED
├─→ Parallax barrier can toggle on/off
├─→ NO per-layer depth offset
├─→ NO dual-render for stereo separation
└─→ NO game-specific stereo configuration
```

---

## Performance-Critical Sections

```
TIMING INSTRUMENTATION:

gpu3dsCheckSlider()           - Check 3D slider position
  ↓ (if changed)
S9xMainLoop()                 - Full CPU frame
  ├─→ S9xRenderScreenHardware() [CRITICAL]
  │   ├─→ S9xDrawBackdropHardware()
  │   ├─→ S9xDrawBackgroundHardware*() [CRITICAL - BG0-3]
  │   ├─→ S9xDrawOBJSHardware() [CRITICAL - Sprites]
  │   └─→ Color math / blending
  │
  └─→ Timing breaks down by layer/component

impl3dsRunOneFrame()
  ├─→ gpu3dsSetRenderTargetToMainScreenTexture()
  ├─→ S9xMainLoop()
  │
  ├─→ gpu3dsSetRenderTargetToFrameBuffer() [CRITICAL]
  ├─→ Render border & scale game to screen [CRITICAL]
  │
  ├─→ gpu3dsFlush() [CRITICAL - GPU sync point]
  ├─→ gpu3dsWaitForPreviousFlush() [CRITICAL - GPU wait]
  │
  └─→ gpu3dsTransferToScreenBuffer() [CRITICAL - DMA]

Bottlenecks:
├─→ Tile caching (CPU conversion: bitplane→GPU)
├─→ Layer composition (many tiles per layer)
├─→ GPU synchronization (waiting for previous frame)
└─→ Display transfer (512KB framebuffer DMA)
```

---

## Where Stereoscopic Code Could Go

```
RECOMMENDED INJECTION POINT:

Option A: Layer-by-Layer Depth Offset (RECOMMENDED)
  
  Location: /source/Snes9x/gfxhw.cpp:3415
  Function: S9xRenderScreenHardware()
  
  Implementation:
  ├─→ Add config: enable_stereo_3d, depth_offset_amount
  ├─→ For each layer rendered:
  │   ├─→ Calculate base_depth (priority * 256)
  │   ├─→ Add stereo_offset = depth_offset * (layer_type + priority)
  │   ├─→ final_depth = base_depth + stereo_offset
  │   └─→ Pass to gpu3dsSetDepthValue(final_depth)
  │
  └─→ Effects:
      ├─→ Backgrounds pop out with parallax effect
      ├─→ Sprites get varying depth
      └─→ Minimal performance impact

Option B: Parallax Barrier Control Enhancement
  
  Location: /source/3dsgpu.cpp:152
  Function: gpu3dsCheckSlider()
  
  Implementation:
  ├─→ Read slider as continuous value (0.0-1.0)
  ├─→ Map to depth offset (-MAX_OFFSET to +MAX_OFFSET)
  ├─→ Update all layer depths dynamically
  └─→ Real-time 3D depth adjustment

Option C: Dual-Eye Rendering (COMPLEX)
  
  Location: /source/3dsgpu.cpp:926
  Function: gpu3dsSetRenderTargetToFrameBuffer()
  
  Implementation:
  ├─→ Create left/right framebuffers
  ├─→ Render scene twice with camera offsets
  ├─→ Combine into interleaved output for parallax
  └─→ HIGH PERFORMANCE COST (2x GPU work)

RECOMMENDED: Option A (Layer Depth Offset)
```

---

## Summary of Key Concepts

| Concept | Implementation | Location |
|---------|-----------------|----------|
| **Main Loop** | impl3dsRunOneFrame() | 3dsimpl.cpp:502 |
| **Layer Priority** | Depth encoding in vertices | gfxhw.cpp:3415+ |
| **Transparency** | GPU alpha test/blend | 3dsgpu.cpp:565+ |
| **Tile Cache** | Hash-based lookup system | 3dsimpl_tilecache.cpp |
| **GPU Commands** | Command buffer API | 3dsgpu.cpp:1-100 |
| **3D Support** | Parallax barrier toggle | 3dsgpu.cpp:140+ |
| **Sprite Rendering** | Per-scanline rendering | gfxhw.cpp:2578+ |
| **Mode 7** | Separate geometry shader | gfxhw.cpp:3146+ |
| **Screen Output** | DisplayTransfer DMA | 3dsgpu.cpp:1049+ |

---

## Files by Importance for Stereo Implementation

1. **CRITICAL:**
   - `/source/3dsgpu.cpp` - GPU state management
   - `/source/Snes9x/gfxhw.cpp` - Layer rendering
   - `/source/3dsimpl.cpp` - Main render loop

2. **IMPORTANT:**
   - `/source/3dsimpl_gpu.cpp` - Vertex generation
   - `/source/3dsgpu.h` - GPU API headers
   - `/source/3dsimpl_gpu.h` - Vertex structures

3. **REFERENCE:**
   - `/source/3dsimpl_tilecache.cpp` - Tile management
   - `/source/Snes9x/ppu.h` - PPU structure
   - `/source/Snes9x/gfx.h` - Graphics structures

