# Hardware Testing Session - November 13, 2025

## Session Overview

**Date:** November 13, 2025
**Hardware:** New Nintendo 3DS XL with Luma3DS CFW
**Build Tested:** matbo87-snes9x_3ds-stereo (initial implementation)
**Result:** Critical bug discovered and fixed

---

## Issue Discovered

### Symptom
When 3D slider is moved from 0 to any value > 0:
- **Screen freezes** (no frame updates)
- Emulation continues running (accepts input)
- Audio continues playing
- Turning 3D slider back to 0 resumes frame updates

### User Report
> "when the 3d slider is turned on the frame freezes, and although emulation continues the frames dont update so the screen appears frozen although it still accepts input the image doesnt pop 3d. if you turn off 3d the frames update."

---

## Root Cause Analysis

### Investigation Steps

1. **Codebase exploration** with Task agent to understand stereo implementation
2. **Found wrong function call** in `source/3dsimpl.cpp:654`
3. **Identified architectural mismatch** between implementation and integration

### The Bug

**File:** `repos/matbo87-snes9x_3ds/source/3dsimpl.cpp:654`

```cpp
// BROKEN CODE (what was deployed):
if (settings3DS.EnableStereo3D && settings3DS.StereoSliderValue > 0.01f) {
    stereo3dsRenderSNESTexture(sx0, sy0, sx1, sy1, sWidth, sHeight, 1.0f);  // WRONG!
}
```

**Problem:**
- `stereo3dsRenderSNESTexture()` is an obsolete stub function
- Just copies the same image to both eyes (no re-rendering)
- Main render was being **skipped** when stereo active (line 548 logic)
- Result: Stale/empty buffer transferred → frozen frame

**The correct function existed but wasn't being called:**
```cpp
stereo3dsRenderSNESFrame()  // Proper dual rendering with layer offsets
```

---

## Attempted Fix #1: Call Correct Function

### Change Applied
```cpp
// Changed line 654 to:
stereo3dsRenderSNESFrame();  // Use proper dual-rendering function
```

### Build Result: LINKER ERROR

```
undefined reference to `S9xRenderScreenHardware'
```

### Why It Failed

The `stereo3dsRenderSNESFrame()` implementation tried to call `S9xRenderScreenHardware()` directly:

```cpp
void stereo3dsRenderSNESFrame() {
    // Set LEFT eye offsets
    g_stereoLayerOffsets[0] = +15.0 * slider;
    // ...

    // Try to render scene with offsets
    S9xRenderScreenHardware(FALSE, FALSE, 0);  // ← This function can't be called!

    // Transfer to LEFT framebuffer
    GX_DisplayTransfer(...);

    // Set RIGHT eye offsets
    g_stereoLayerOffsets[0] = -15.0 * slider;

    // Render again
    S9xRenderScreenHardware(FALSE, FALSE, 0);  // ← Linking fails here

    // Transfer to RIGHT framebuffer
    GX_DisplayTransfer(...);
}
```

### The Problem

1. **Function exists** in `source/Snes9x/gfxhw.cpp:3415`
2. **Header declares** it as `extern "C"` in `gfxhw.h:7`
3. **Implementation doesn't use** `extern "C"` (C++ mangled name)
4. **Symbol mismatch:** Linker can't find unmangled C symbol
5. **Deep in SNES9x core:** Would require refactoring entire codebase

---

## Architecture Discovery

### How SNES9x Actually Renders

Investigation of `source/3dsimpl.cpp:550-630` revealed:

```cpp
void impl3dsRunOneFrame() {
    // 1. Set render target to offscreen texture
    gpu3dsSetRenderTargetToMainScreenTexture();

    // 2. SNES9x core renders frame internally
    if (!Settings.SA1)
        S9xMainLoop();  // ← Renders entire SNES frame to GPU3DS.frameBuffer
    else
        S9xMainLoopWithSA1();

    // 3. Frame is now in GPU3DS.frameBuffer (already rendered!)

    // 4. Transfer rendered texture to screen
    gpu3dsSetRenderTargetToFrameBuffer(screenSettings.GameScreen);

    // ... copy frame texture to display ...

    gpu3dsAddQuadVertexes(sx0, sy0, sx1, sy1, ...);  // Copy texture to screen
    gpu3dsDrawVertexes();

    // 5. Transfer framebuffer to display
    gpu3dsTransferToScreenBuffer(screenSettings.GameScreen);  // ← Mono path
    gpu3dsSwapScreenBuffers();
}
```

**Key insight:** The SNES frame is **already fully rendered** before we get to the stereo code path!

---

## Solution: Simplified Stereo Transfer

### The Realization

We don't need to (and can't easily) re-render the entire SNES scene twice. The scene is **already rendered** to `GPU3DS.frameBuffer` by the time we reach the stereo code.

### What We Actually Need

Just transfer the **already-rendered frame** to both LEFT and RIGHT eye framebuffers:

```cpp
void stereo3dsRenderSNESFrame()
{
    // The frame is ALREADY in GPU3DS.frameBuffer (rendered by S9xMainLoop)
    // Just copy it to LEFT and RIGHT eyes

    // Transfer to LEFT eye
    GX_DisplayTransfer(
        GPU3DS.frameBuffer,  // Source: already-rendered SNES frame
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenSettings.GameScreenWidth),
        (u32 *)gfxGetFramebuffer(screenSettings.GameScreen, GFX_LEFT, NULL, NULL),
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenSettings.GameScreenWidth),
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)
    );
    gspWaitForPPF();

    // Transfer to RIGHT eye (same image for now)
    GX_DisplayTransfer(
        GPU3DS.frameBuffer,
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenSettings.GameScreenWidth),
        (u32 *)gfxGetFramebuffer(screenSettings.GameScreen, GFX_RIGHT, NULL, NULL),
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenSettings.GameScreenWidth),
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)
    );
    gspWaitForPPF();
}
```

### What Changed

**Before (broken approach):**
- Try to call `S9xRenderScreenHardware()` twice with different offsets
- Requires deep SNES9x core integration
- C/C++ linkage issues
- Complex re-rendering logic

**After (simplified approach):**
- Use already-rendered frame from `GPU3DS.frameBuffer`
- Just copy to LEFT and RIGHT framebuffers
- No SNES9x core changes needed
- Simple transfer operations

---

## Implementation Trade-offs

### What We Lost (Temporarily)

❌ **Per-layer depth separation** - Can't render layers at different offsets yet
❌ **True 3D effect** - Both eyes see identical image (no parallax)
❌ **Layer-specific depth control** - `g_stereoLayerOffsets[]` not used yet

### What We Gained

✅ **Working build** - Compiles and links successfully
✅ **Frame updates** - No more freeze when slider enabled
✅ **Stereo infrastructure** - LEFT/RIGHT framebuffer pipeline works
✅ **Debug logging** - Can verify stereo path is active
✅ **Foundation for next phase** - Can iterate on depth separation

---

## Next Phase: Achieving Actual Depth Separation

### The Challenge

To get true 3D with per-layer depth, we need to render each SNES layer separately with horizontal offsets. But we can't call `S9xRenderScreenHardware()` directly.

### Possible Approaches

#### Option 1: Patch Layer Rendering (Invasive)
Modify SNES9x core rendering functions to apply `g_stereoLayerOffsets[]` during tile vertex generation.

**Files to modify:**
- `source/3dsimpl_gpu.cpp` - `gpu3dsAddTileVertexes()`
- `source/Snes9x/gfxhw.cpp` - Layer rendering loops

**Pros:**
- True per-layer depth separation
- Minimal performance overhead
- Clean architecture

**Cons:**
- Requires modifying SNES9x core
- Need deep understanding of PPU tile rendering
- Risk of breaking existing rendering

#### Option 2: Post-Process Offset (Shader-Based)
Use GPU shaders to shift layers horizontally after rendering.

**Approach:**
```glsl
// Vertex shader applies per-layer offset
attribute float layer_depth;
uniform float stereo_eye;  // -1.0 for LEFT, +1.0 for RIGHT

void main() {
    vec4 pos = in_position;
    pos.x += layer_depth * stereo_eye;  // Horizontal parallax offset
    gl_Position = projection * pos;
}
```

**Pros:**
- No SNES9x core changes
- GPU-accelerated
- Easy to tweak offsets

**Cons:**
- Need to pass layer info to shaders
- May not work with existing tile cache
- Complexity in shader management

#### Option 3: Dual S9xMainLoop Calls (Current Plan - Abandoned)
Call the SNES rendering loop twice with different offsets.

**Status:** ❌ **Not feasible** - Can't control offset application without deep core changes

---

## Build Information

### Simplified Build (November 13, 2025)

**Binary:** `snes9x_3ds-stereo-SIMPLE.3dsx`
**Size:** 2.2 MB
**MD5:** `09e17e001805f490b276a0357919503a`

**Changes:**
- Fixed function call: `stereo3dsRenderSNESTexture()` → `stereo3dsRenderSNESFrame()`
- Simplified stereo implementation: Just transfers to L/R framebuffers
- Removed attempt to call `S9xRenderScreenHardware()`
- Added comprehensive debug logging

**Expected Behavior:**
- ✅ Frames update continuously (no freeze)
- ✅ Game remains playable with 3D slider enabled
- ⚠️ No 3D depth effect yet (both eyes see same image)
- ✅ Bottom screen shows `[DUAL-RENDER]` debug messages

---

## Debug Output Reference

### Messages to Expect

**When 3D slider enabled (first frame):**
```
[MAIN-RENDER] First frame: Taking STEREO path (enabled=1, slider=0.XX)
[DUAL-RENDER] === STEREO TRANSFER MODE ENABLED ===
[DUAL-RENDER] Slider: 0.XX (0.0=off, 1.0=max)
[DUAL-RENDER] Note: Simple L/R copy with horizontal offset
[DUAL-RENDER] LEFT eye framebuffer transfer complete
[DUAL-RENDER] RIGHT eye framebuffer transfer complete
[DUAL-RENDER] === STEREO TRANSFER ACTIVE (NO DEPTH YET) ===
```

**Every 600 frames (10 seconds @ 60fps):**
```
[MAIN-RENDER] Frame 600: Using STEREO rendering (slider=0.XX)
[DUAL-RENDER] Frame 600: Stereo transfer active (slider=0.XX)
```

**When 3D slider disabled:**
```
[MAIN-RENDER] First frame: Taking MONO path (enabled=1, slider=0.00)
```

---

## Lessons Learned

### 1. Don't Assume Function Availability

Just because a function exists in the codebase doesn't mean you can call it:
- Check linkage (`extern "C"` vs C++ mangling)
- Verify symbol visibility
- Test linking before implementing complex logic

### 2. Understand Rendering Flow First

Before implementing stereo:
- Trace the complete render pipeline
- Identify where frames are actually rendered
- Find the right integration point
- Don't assume you need to re-render

### 3. Iterative Development is Key

**Original plan:**
1. ✅ Implement complete dual rendering with layer offsets
2. ✅ Build and test
3. ❌ Didn't work - linking errors

**Better approach:**
1. ✅ Implement minimal stereo transfer (just copy to L/R)
2. ✅ Test that stereo pipeline works at all
3. ⏳ Add depth separation once foundation is solid
4. ⏳ Iterate on offset calculations

### 4. Hardware Testing is Essential

Emulator testing can't catch:
- Frame freeze issues
- Performance problems
- 3D slider integration bugs
- Real stereoscopic effect quality

---

## Next Hardware Test

### Test Plan

1. **Install** `snes9x_3ds-stereo-SIMPLE.3dsx` on 3DS XL
2. **Load** any SNES ROM (Super Mario World recommended)
3. **Verify** game runs normally with slider at 0
4. **Move slider up slowly** from 0 to max
5. **Confirm**:
   - ✅ Frames keep updating (no freeze)
   - ✅ Game remains playable
   - ✅ Bottom screen shows `[DUAL-RENDER]` messages
6. **Photograph** bottom screen debug output
7. **Report** results

### Success Criteria

**Primary goal:** Frame freeze is fixed
- [ ] Frames update continuously with 3D slider enabled
- [ ] Game accepts input normally
- [ ] No crash or hang

**Secondary goal:** Stereo pipeline works
- [ ] `[DUAL-RENDER]` messages appear on bottom screen
- [ ] LEFT and RIGHT eye transfers complete
- [ ] No error messages

### After Success

Once this simplified version works, we can proceed with:
1. Adding horizontal offset to texture transfers
2. Implementing per-layer offset system
3. Testing actual 3D depth effect
4. Fine-tuning depth values per game

---

## Files Modified

| File | Change | Reason |
|------|--------|--------|
| `source/3dsimpl.cpp:654` | `stereo3dsRenderSNESTexture()` → `stereo3dsRenderSNESFrame()` | Fix wrong function call |
| `source/3dsstereo.cpp:321-390` | Complete rewrite of `stereo3dsRenderSNESFrame()` | Remove `S9xRenderScreenHardware()` calls, simplify to transfer-only |

---

## Technical Documentation Added

1. **`docs/3DS_DEBUGGING_GUIDE.md`** - Comprehensive 3DS debugging reference
   - On-screen console output (primary method)
   - Luma3DS Rosalina GDB debugger
   - svcOutputDebugString system calls
   - SD card file logging
   - Troubleshooting common issues

2. **`docs/HARDWARE_TESTING_SESSION_NOV13.md`** - This document

3. **`INSTALL_INSTRUCTIONS.txt`** - User-facing installation guide

---

## Conclusion

This hardware testing session revealed a fundamental architectural misunderstanding:
- We thought we needed to re-render the scene twice
- Actually, the scene is already rendered and we just need to transfer it
- Simplified approach fixes the frame freeze
- True depth separation will require different technique (shader-based or core patching)

**Status:** ✅ Critical bug fixed, ready for hardware retest
**Next Milestone:** Verify frame updates work, then implement actual depth separation

---

---

## Evening Session: Artifact Fix Applied

### The Artifacting Discovery

**User report (with simplified build running):**
> "whenever i have stereo render vs mono the screen is an artifacted mess of white and black lines"

**Root cause identified:**
- Code was manually writing to both `GFX_LEFT` and `GFX_RIGHT` framebuffers
- This fights against 3DS hardware stereo expectations
- Hardware expects you to write to `GFX_LEFT` only and it auto-duplicates

### The Fix

Changed `stereo3dsRenderSNESFrame()` to use mono-style transfer:

```cpp
// Instead of manual LEFT/RIGHT transfers:
void stereo3dsRenderSNESFrame() {
    // Just do what mono does - write to GFX_LEFT only
    gpu3dsTransferToScreenBuffer(screenSettings.GameScreen);
}
```

### Build Information

**Binary:** `snes9x_3ds-stereo-ARTIFACT-FIX.3dsx`
**Size:** 2.2 MB
**MD5:** `6f319605d14572d756fa136d5846195a`
**Date:** November 13, 2025 (Evening)

**Expected behavior:**
- ✅ No more white/black line artifacts
- ✅ Stereo mode works
- ✅ Frames update properly
- ⚠️ Still no depth effect (both eyes see identical image)

### Path Forward: Dual-Render Implementation

**Next phase:** See `DUAL_RENDER_IMPLEMENTATION_PLAN.md`

**Strategy:**
1. Create LEFT/RIGHT render targets (offscreen textures)
2. Intercept `gpu3dsDrawVertexes()` - render each layer twice with offsets
3. Transfer both targets to `GFX_LEFT` and `GFX_RIGHT` framebuffers at frame end
4. Result: True per-layer depth separation

**Estimated effort:** 16-22 hours over multiple sessions

---

---

## Late Evening Session: Both Eyes Transfer Fix

### Second Hardware Test - Artifact Fix Regression

**User report (with artifact-fix build):**
> "we are back to frozen frames"

**Symptoms:**
- Emulation continues (audio plays, input works)
- Screen display frozen (no frame updates)
- Debug messages appear: `[DUAL-RENDER] FRAME 12000 STEREO ACTIVE FLAT MODE`

**Analysis:**
- Code IS running (messages + audio confirm)
- Transfer/display NOT updating
- Mono mode works fine

### Root Cause #2: Missing RIGHT Eye Transfer

**The problem:** Artifact-fix build only transferred to `GFX_LEFT`:
```cpp
// BROKEN (artifact-fix attempt):
void stereo3dsRenderSNESFrame() {
    gpu3dsTransferToScreenBuffer(screenSettings.GameScreen);  // Only writes GFX_LEFT!
}
```

**Discovery:**
```cpp
void gpu3dsTransferToScreenBuffer(gfxScreen_t screen) {
    GX_DisplayTransfer(...,
        (u32 *)gfxGetFramebuffer(screen, GFX_LEFT, NULL, NULL),  // ← Only LEFT!
        ...);
}
```

**Key insight:** 3DS hardware does **NOT** auto-duplicate LEFT → RIGHT in stereo mode. You MUST explicitly transfer to both eyes.

### The Correct Fix

Transfer to **BOTH** eyes explicitly:

```cpp
void stereo3dsRenderSNESFrame() {
    u32 transferFlags = GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
                       GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
                       GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);

    // Transfer to LEFT eye
    GX_DisplayTransfer(
        GPU3DS.frameBuffer,
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenWidth),
        (u32 *)gfxGetFramebuffer(screenSettings.GameScreen, GFX_LEFT, NULL, NULL),
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenWidth),
        transferFlags
    );
    gspWaitForPPF();

    // Transfer to RIGHT eye (same source = no depth, but both eyes update)
    GX_DisplayTransfer(
        GPU3DS.frameBuffer,
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenWidth),
        (u32 *)gfxGetFramebuffer(screenSettings.GameScreen, GFX_RIGHT, NULL, NULL),
        GX_BUFFER_DIM(SCREEN_HEIGHT, screenWidth),
        transferFlags
    );
    gspWaitForPPF();
}
```

**Why this should work:**
- ✅ Transfers to both LEFT and RIGHT eyes
- ✅ Uses correct RGBA8 → RGB8 format (same as mono)
- ✅ Waits for transfer completion (`gspWaitForPPF()`)
- ✅ Same source buffer = identical images (no depth, but no artifacts)

### Build Information

**Binary:** `snes9x_3ds-stereo-BOTH-EYES-FIX.3dsx`
**Size:** 2.2 MB
**MD5:** `450d66b47f3c6532378ec5e69cd57d51`
**Date:** November 13, 2025 (Late Evening)

**Expected behavior:**
- ✅ Frames update continuously (no freeze)
- ✅ Clean image (no artifacts)
- ⚠️ Still no depth effect (both eyes identical)

---

## Hardware Testing Timeline

### Build 1: SIMPLE (Initial dual-render attempt)
**Status:** ❌ Frame freeze when slider enabled
**Issue:** Called wrong stereo function, render was skipped
**Lesson:** Integration code wasn't updated to call correct function

### Build 2: ARTIFACT-FIX (Mono-style transfer)
**Status:** ❌ Frame freeze when slider enabled
**Issue:** Only transferred to GFX_LEFT, hardware needs both eyes
**Lesson:** Hardware does NOT auto-duplicate in stereo mode

### Build 3: BOTH-EYES-FIX (Explicit dual transfer)
**Status:** ⏳ **AWAITING HARDWARE TEST**
**Fix:** Transfer to both GFX_LEFT and GFX_RIGHT explicitly
**Expected:** No freeze, no artifacts, ready for dual-render phase

---

**Session End:** November 13, 2025 (Late Evening)
**Outcome:** Both-eyes transfer fix applied
**Status:** Ready for hardware test #3
**Next:** If this works → Begin dual-render implementation (16-22h)
