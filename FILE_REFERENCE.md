# SNES9x 3DS - Complete File Reference for Rendering Pipeline

## Absolute File Paths

### Core 3DS GPU Implementation
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsgpu.cpp` - Main GPU implementation
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsgpu.h` - GPU interface header
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/gpulib.cpp` - Deprecated GPU functions
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/gpulib.h` - Legacy GPU API

### Extended GPU (Vertex Generation & Rendering)
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl_gpu.cpp` - Vertex generation
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl_gpu.h` - Vertex structures
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl.cpp` - Main emulation implementation
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl.h` - Implementation header

### Tile Caching System
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl_tilecache.cpp` - Tile conversion
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl_tilecache.h` - Tile cache API

### SNES9x Core - Graphics Rendering
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfxhw.cpp` - **[KEY FILE]** Hardware graphics rendering
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfxhw.h` - Graphics hardware header
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfx.cpp` - Graphics core
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfx.h` - Graphics structure

### SNES9x Core - PPU
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/ppu.cpp` - PPU emulation
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/ppu.h` - **[KEY FILE]** PPU structure
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/ppuvsect.cpp` - PPU vertical section handling
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/ppuvsect.h` - PPU vsect header

### SNES9x Core - CPU
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/cpuexec.cpp` - CPU execution
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/cpuexec.h` - CPU header

### Main Loop & Display
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsmain.cpp` - Main application loop
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsmenu.cpp` - Menu system
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsui.cpp` - UI rendering
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsui.h` - UI header

### Configuration & Settings
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsconfig.cpp` - Config loading
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsconfig.h` - Config header
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dssettings.h` - Settings structure

---

## Key Functions by Location

### Main Rendering Loop
```
File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl.cpp
Line 502:   void impl3dsRunOneFrame(bool firstFrame, bool skipDrawingFrame)
Line 490:   void impl3dsPrepareForNewFrame()
Line 280:   void impl3dsFinalize()
Line 110:   bool impl3dsInitializeCore()
```

### PPU Graphics Rendering (CRITICAL FOR STEREO)
```
File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfxhw.cpp
Line 3415:  void S9xRenderScreenHardware(bool8 sub, bool8 force_no_add, uint8 D)
Line 3146:  void S9xDrawBackgroundMode7Hardware(int bg, bool8 sub, int depth, int alphaTest)
Line 3023:  void S9xPrepareMode7(bool sub)
Line 2578:  void S9xDrawOBJSHardware(bool8 sub, int depth = 0, int priority = 0)
Line 2479:  void S9xDrawOBJTileHardware2(...)
Line 1509:  void S9xDrawBackgroundHardwarePriority0Inline(...)
Line 87:    void S9xDrawBackdropHardware(bool sub, int depth)
```

### GPU Interface
```
File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsgpu.cpp
Line 994:   void gpu3dsFlush()
Line 1015:  void gpu3dsWaitForPreviousFlush()
Line 1049:  void gpu3dsTransferToScreenBuffer(gfxScreen_t screen)
Line 1060:  void gpu3dsSwapScreenBuffers()
Line 926:   void gpu3dsSetRenderTargetToFrameBuffer(gfxScreen_t targetScreen)
Line 948:   void gpu3dsSetRenderTargetToTexture(SGPUTexture *texture, SGPUTexture *depthTexture)
Line 809:   void gpu3dsEnableAlphaBlending()
Line 565:   void gpu3dsEnableAlphaTestNotEqualsZero()
Line 172:   void gpu3dsEnableDepthTest()
Line 152:   void gpu3dsCheckSlider()    // 3D SLIDER - KEY FOR STEREO
Line 140:   void gpu3dsSetParallaxBarrier(bool enable)
```

### 3D Display Features
```
File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsgpu.cpp
Line 152-170: gpu3dsCheckSlider() and gpu3dsSetParallaxBarrier() - 3D SLIDER CONTROL

File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsmain.cpp
Line 2241:  gfxSet3D(true)      - Enable 3D mode
Line 2242:  gpu3dsCheckSlider() - Check slider position
```

### Vertex Generation
```
File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl_gpu.h
Line 114-139: gpu3dsAddQuadVertexes() - Add quad vertices
Line 142-194: gpu3dsAddTileVertexes() - Add tile vertices
Line 197-248: gpu3dsAddMode7LineVertexes() - Mode 7 lines
```

### Tile Caching
```
File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl_tilecache.h
Line 20-66: cache3dsGetTexturePositionFast() - Tile lookup
Line 59-66: cacheGetSwapTexturePositionForAltFrameFast()
Line 73-76: cache3dsCacheSnesTileToTexturePosition() - Tile conversion
```

---

## Critical Code Sections for Stereo Implementation

### 1. Layer Depth Encoding (BEST INJECTION POINT)
```
File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfxhw.cpp
Line 3415-3950: S9xRenderScreenHardware()

Current depth calculation:
  depth = priority * 256 + alpha_value

For stereo, modify to:
  depth = priority * 256 + alpha_value + stereo_offset
  where stereo_offset = base_offset * (layer_type + priority)
```

### 2. 3D Slider Control
```
File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsgpu.cpp
Line 152-170: gpu3dsCheckSlider()

Current: Only toggles parallax barrier on/off
New: Could map slider value to depth offset amount
```

### 3. Background Layer Rendering
```
File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfxhw.cpp
Line 1509-1810: S9xDrawBackgroundHardwarePriority0Inline()

For each layer, add stereo depth calculation before:
  gpu3dsAddTileVertexes(x0, y0, x1, y1, tx0, ty0, tx1, ty1, 
                        depth + stereo_offset)  // NEW
```

### 4. Sprite Rendering
```
File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfxhw.cpp
Line 2707: S9xDrawOBJTileHardware2(sub, 
           (PPU.OBJ[S].Priority + 1) * 3 * 256 + depth,  // DEPTH HERE
           ...)

Modify sprite depth to include stereo offset
```

---

## Data Flow Files

### CPU Emulation
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/cpuexec.cpp` - S9xMainLoop()
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/cpuexec.h` - CPU state

### PPU State
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/ppu.h` - PPU registers and state
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/ppu.cpp` - PPU implementation

### Graphics State
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfx.h` - Graphics state (SGFX)
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfx.cpp` - Graphics functions

### GPU State
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsgpu.h` - GPU state (SGPU3DS)
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl_gpu.h` - Extended GPU state (SGPU3DSExtended)

---

## Configuration & Game Settings Files

### Emulator Settings
```
File: <PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dssettings.h
Contains: S9xSettings3DS structure with:
  - settings3DS.Disable3DSlider
  - settings3DS.GameScreen (TOP or BOTTOM)
  - Screen stretching options
  - Display crop options
```

### Suggested New Configuration
```
Add to S9xSettings3DS:
  - stereo_3d_enabled (bool)
  - stereo_depth_offset (int, 0-100)
  - stereo_mode (per-game config)
```

---

## Testing & Debugging Files

### Main Application
- `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsmain.cpp` - Application entry point

### Performance Timing
```
Timing markers throughout code:
  t3dsStartTiming(id, name)
  t3dsEndTiming(id)

Critical timing points:
  - Line 3417: S9xRenderScreenHardware() timing
  - Line 534: CopyFB timing
  - Line 601: Transfer timing
  - Line 615: Flush timing
```

---

## Summary: Files to Modify for Stereo Implementation

### Modification Priority

**MUST MODIFY:**
1. `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfxhw.cpp`
   - Function: S9xRenderScreenHardware() [Line 3415]
   - Add stereo depth offset to all layer rendering

2. `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dssettings.h`
   - Add stereo configuration structure
   - Stereo enable/disable flag
   - Depth offset parameters

**SHOULD MODIFY:**
3. `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsgpu.cpp`
   - Function: gpu3dsCheckSlider() [Line 152]
   - Map slider to stereo depth instead of just on/off

4. `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl.cpp`
   - Function: impl3dsRunOneFrame() [Line 502]
   - Pass stereo configuration to rendering

**REFERENCE ONLY:**
5. `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/ppu.h` - Understand layer structure
6. `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsgpu.h` - Understand GPU API

