# Stereoscopic 3D Status - December 29, 2025

## Current Status: 🔴 DEBUGGING - Root Cause Identified

Black screen / green artifacts when 3D slider is activated. Comprehensive logging revealed the root cause.

## Issue Description

When user moves 3D slider up, screen shows:
- Black screen (no game visible)
- Green checkerboard artifacts (uninitialized VRAM)
- Audio continues normally (emulation is running)
- Rotation appears 90 degrees off

## Root Cause Analysis (From Logs)

### What's Working ✅
1. **Render targets ARE created correctly**
   - LEFT eye: 0x1f43b800 (256×256 RGBA8)
   - RIGHT eye: 0x1f13ba00 (256×256 RGBA8)
   - Depth buffers allocated correctly

2. **Target switching IS working**
   - Switches between LEFT/RIGHT render targets correctly
   - Addresses and dimensions verified in logs

3. **Clearing IS working**
   - Both eyes cleared to BLACK (256×256)
   - Frame-once clear pattern working correctly

### The Problem ❌
4. **Vertex counts are ZERO!**
   ```
   LEFT:  tileVerts=0 quadVerts=0 rectVerts=2
   RIGHT: tileVerts=0 quadVerts=0 rectVerts=2
   ```
   - Only UI rectangles have vertices (2)
   - Game tiles/quads have ZERO vertices
   - **No geometry is being generated for stereo buffers**

### Root Cause

**Stereo enable/disable is toggling during frame rendering:**

```
Log sequence:
stereo3dsIsEnabled=1 ← Enabled
stereo3dsIsEnabled=0 ← Disabled!
stereo3dsIsEnabled=1 ← Enabled again
```

**The bug flow:**
1. Slider drops below 0.01f threshold (even momentarily)
2. `stereo3dsSetEnabled(false)` called in main loop (3dsimpl.cpp:598)
3. `stereo3dsIsEnabled()` returns false during vertex generation
4. Vertex generation skips stereo buffers (3dsimpl_gpu.h:171)
5. Stereo targets exist and are cleared/rendered
6. **Result:** Empty stereo buffers → black screen/green artifacts

**Key code locations:**
- `3dsimpl.cpp:597-598` - Stereo toggling based on slider threshold
- `3dsimpl_gpu.h:169-221` - Vertex generation checks `stereo3dsIsEnabled()`
- `3dsstereo.cpp:125` - Returns `stereoConfig.enabled && stereoInitialized`

## Architecture (Plan E)

### Per-Eye Dual Rendering
- **Vertex buffers:** Separate for each eye (`stereoTileVertexes[2]`, `stereoQuadVertexes[2]`)
- **Depth offsets:** Per-layer, per-eye horizontal shifts
- **Render targets:** 256×256 RGBA8 textures (LEFT/RIGHT)
- **Lazy allocation:** Targets created when slider > 0 (saves VRAM when 3D off)

### Current Implementation
```
Main Loop (3dsimpl.cpp):
  ├─ Read hardware slider → calculate effective depth
  ├─ stereo3dsSetEnabled(stereoActive) ← TOGGLES BASED ON SLIDER!
  ├─ Update layer offsets
  ├─ Ensure stereo targets created
  └─ S9xMainLoop() calls PPU:
       └─ gfxhw.cpp calls gpu3dsAddTileVertexes():
            └─ Checks stereo3dsIsEnabled() ← MAY BE FALSE!
                 ├─ If true: Generate vertices for stereo buffers
                 └─ If false: Generate vertices for MONO buffers
```

## Debug Logging Added

Comprehensive logging in current build:
1. **Render target switches** (3dsstereo.cpp:265-296)
   - First 10 switches logged
   - Shows eye, texture address, dimensions

2. **Clear operations** (3dsimpl_gpu.cpp:264-291)
   - First 3 frame clears logged
   - Confirms both eyes cleared to BLACK

3. **Draw calls** (3dsimpl_gpu.cpp:314-324)
   - First 5 draws per eye logged
   - Shows vertex counts (all ZERO for tiles/quads)

4. **Stereo mode changes** (3dsimpl.cpp:600-612)
   - Tracks ACTIVE/INACTIVE toggles
   - Shows slider values when changes occur

## Attempted Fixes

### ❌ Removed stereo toggle (Rejected by user)
- Initially tried removing `stereo3dsSetEnabled(stereoActive)` call
- User correctly pointed out slider is for 3D intensity control
- Slider should remain functional (not binary on/off)

## Next Steps

### Investigation Needed
1. **Why are vertices not being generated?**
   - Is `stereo3dsIsEnabled()` false during S9xMainLoop()?
   - Timing issue between enable/disable and vertex generation?
   - Check if `stereoInitialized` flag is unexpectedly false

2. **Vertex buffer initialization**
   - When are stereo buffers allocated?
   - Are they being properly initialized before first use?

3. **Alternative solutions**
   - Decouple slider value from stereo capability
   - Always generate to stereo buffers when targets exist
   - Only use slider for offset intensity, not enable/disable

### Testing Plan
1. Add logging to vertex generation functions
2. Log `stereo3dsIsEnabled()` state during PPU rendering
3. Trace exact sequence: enable → vertex gen → draw
4. Identify where toggle causes vertex generation to fail

## Files Modified (Current Build)

- `source/3dsstereo.cpp/h` - Stereo rendering system
- `source/3dslog.cpp/h` - Logging infrastructure
- `source/3dsimpl_gpu.cpp/h` - Vertex generation and drawing
- `source/3dsimpl.cpp` - Main loop integration
- `source/3dsgpu.cpp/h` - Render target management
- `source/Snes9x/gfxhw.cpp` - Layer tracking

## Build Info

- **Debug build MD5:** `00d191a3ecd66d7112a717f2aea90018`
- **Pushed to:** https://github.com/f4mrfaux/SNES9x3DS (master branch)
- **Log location:** SD card `/snes9x_3ds_stereo.log`

## Previous Status

See `STEREO_STATUS_DEC22_2025.md` for previous hardware slider integration work.

---
**Last Updated:** December 29, 2025
**Status:** Debugging vertex generation failure
**Next Milestone:** Fix vertex buffer population to enable hardware testing
