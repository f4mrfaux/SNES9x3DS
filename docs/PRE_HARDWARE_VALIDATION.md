# Pre-Hardware Testing Validation

**Date:** November 15, 2025 (Updated: November 25, 2025)
**Build MD5:** `4afa913c0c994705ed6cc4f07795c3ca` (P1-P6 fixes applied Nov 25, 2025)
**Previous MD5:** `c7ccc93fd2b31c2696b364c89c9d511c` (Pre-fixes)

---

## ⚠️ IMPORTANT: P1-P6 Bug Fixes Applied (Nov 25, 2025)

**This document describes the pre-fix validation. The following issues identified here have been FIXED:**
- **P1 (CRITICAL):** Hardware slider integration - NOW FIXED (see POST_FIX_VERIFICATION_NOV25.md)
- **P2:** Dead code removed - 292 lines of obsolete stereo functions deleted
- **P3:** Unused VRAM buffers removed - ~200KB savings
- **P5:** Render target initialization - verified correct
- **P6:** Variable naming - clarified in documentation

**For current status, see:** `/POST_FIX_VERIFICATION_NOV25.md`

---

## Code Validation Results

Based on 3DS homebrew best practices and canonical stereo rendering patterns, the following items were checked:

### ✅ 1. `gfxSet3D(true)` Called Correctly

**Status:** VERIFIED CORRECT

**Findings:**
```cpp
// 3dsgpu.cpp:444 - Initialization
gfxSet3D(!settings3DS.Disable3DSlider);

// 3dsstereo.cpp:70 - Stereo init
gfxSet3D(true);

// 3dsstereo.cpp:272-274 - Enable/disable
if (enabled) {
    gfxSet3D(true);
} else {
    gfxSet3D(false);
}
```

**Conclusion:** Properly enables right-eye framebuffer. No action needed.

---

### ⚠️ 2. Hardware Slider Integration (`osGet3DSliderState()`)

**Status:** PARTIALLY IMPLEMENTED - Needs Fix

**Current Implementation:**
```cpp
// 3dsstereo.cpp:121 - Slider is READ
currentSliderValue = osGet3DSliderState();

// BUT: Not actually USED for depth calculation!
// 3dsimpl.cpp:595-598
if (settings3DS.EnableStereo3D)
    stereo3dsUpdateLayerOffsetsFromSlider(settings3DS.StereoSliderValue);  // ← Menu setting!
else
    stereo3dsUpdateLayerOffsetsFromSlider(0.0f);
```

**Problem:**
- Hardware slider value is read but ignored
- Depth offsets use menu setting (`settings3DS.StereoSliderValue`)
- User must close game and change menu to adjust depth
- **Expected behavior:** Hardware slider should modulate effect in real-time

**Recommended Fix:**
```cpp
// In 3dsimpl.cpp main loop (around line 595)
float hwSlider = osGet3DSliderState();  // 0.0 - 1.0 from hardware
float userMax = settings3DS.StereoSliderValue;  // User's max strength setting
float effective = hwSlider * userMax;  // Combined value

if (settings3DS.EnableStereo3D)
    stereo3dsUpdateLayerOffsetsFromSlider(effective);
else
    stereo3dsUpdateLayerOffsetsFromSlider(0.0f);
```

**Benefits:**
- Slider down → instant 2D (even if menu has 3D enabled)
- Slider up → gradual depth increase up to user's max
- Matches standard 3DS game behavior
- Better user experience

**Priority:** MEDIUM - Game works without it, but UX is suboptimal

---

### ✅ 3. Transfer Flags Consistency

**Status:** VERIFIED CORRECT

**Mono transfer (`gpu3dsTransferToScreenBuffer`):**
```cpp
GX_DisplayTransfer(
    GPU3DS.frameBuffer,
    GX_BUFFER_DIM(SCREEN_HEIGHT, screenWidth),
    (u32 *)gfxGetFramebuffer(screen, GFX_LEFT, NULL, NULL),
    GX_BUFFER_DIM(SCREEN_HEIGHT, screenWidth),
    GX_TRANSFER_IN_FORMAT(...) | GX_TRANSFER_OUT_FORMAT(...));
```

**Stereo transfer (`stereo3dsTransferToScreenBuffers`):**
```cpp
GX_DisplayTransfer(
    (u32*)stereoLeftEyeTarget->PixelData,
    GX_BUFFER_DIM(texHeight, texWidth),
    (u32 *)gfxGetFramebuffer(screenSettings.GameScreen, GFX_LEFT, NULL, NULL),
    GX_BUFFER_DIM(SCREEN_HEIGHT, screenWidth),
    GX_TRANSFER_IN_FORMAT(...) | GX_TRANSFER_OUT_FORMAT(...));
```

**Analysis:**
- Same format arrays used: `GX_TRANSFER_FRAMEBUFFER_FORMAT_VALUES`, `GX_TRANSFER_SCREEN_FORMAT_VALUES`
- No additional flags (FLIP_VERT, TILED, SCALING) in either path
- Consistent approach

**Conclusion:** Transfer flags are identical. No action needed.

**Note:** Both paths are minimal - they don't use flags like `GX_TRANSFER_FLIP_VERT`, `GX_TRANSFER_SCALING`, etc. This is fine as long as it works for mono rendering (which it does).

---

### ✅ 4. Screen Orientation & Dimensions

**Status:** VERIFIED CORRECT

**Texture Dimensions:**
```cpp
// 3dsstereo.cpp:420,428 - Render targets created
stereoLeftEyeTarget = gpu3dsCreateTextureInVRAM(256, 240, GPU_RGBA8);
stereoRightEyeTarget = gpu3dsCreateTextureInVRAM(256, 240, GPU_RGBA8);
```

**Transfer Dimensions:**
```cpp
// Source: 256×240 (SNES native)
int texWidth = stereoLeftEyeTarget->Width;   // 256
int texHeight = stereoLeftEyeTarget->Height; // 240
GX_BUFFER_DIM(texHeight, texWidth)           // 240×256

// Dest: 400×240 (3DS top screen, rotated)
int screenWidth = screenSettings.GameScreenWidth; // 400
GX_BUFFER_DIM(SCREEN_HEIGHT, screenWidth)        // 240×400
```

**Analysis:**
- SNES renders 256×240 internally
- 3DS screen is 400×240 (rotated 90° from physical 240×400 LCD)
- GPU scales 256→400 width automatically
- Consistent with mono path approach

**Conclusion:** Dimensions correct. No action needed.

---

## Summary & Recommendations

### Must Fix Before Hardware Test
**None** - Build will run and display stereo

### Should Fix for Better UX
1. **Hardware slider integration** (Priority: MEDIUM)
   - Implement `osGet3DSliderState()` × `settings3DS.StereoSliderValue` combination
   - Allows real-time depth adjustment without menu
   - 5-10 minute fix

### Nice to Have
1. **Transfer flag audit**
   - Both mono and stereo use minimal flags
   - Could investigate if FLIP_VERT or SCALING flags improve quality
   - Low priority - current approach matches existing mono path

---

## Code Quality Assessment

**Overall Grade: A-**

**Strengths:**
- ✅ Architecture matches canonical 3DS stereo patterns
- ✅ Depth math correct (convergence/divergence signs)
- ✅ Source/dest dimensions properly separated
- ✅ `gfxSet3D()` integration correct
- ✅ Mono path preservation excellent
- ✅ Error checking (render target existence, etc.)

**Weaknesses:**
- ⚠️ Hardware slider read but not used (easy fix)
- ⚠️ No comfort scaling (slider/3 for max depth comfort)

**Risks:**
- Low: Depth values [-15, -10, -5, 0, +20] might be aggressive at full slider
- Mitigation: Can adjust in 30 seconds if needed

---

## Hardware Test Readiness

**Ready to test:** YES

**Expected Results:**
1. **Slider OFF**: Perfect 2D rendering (mono path)
2. **Slider ON** (with current code):
   - Depth will be constant (menu setting)
   - Moving hardware slider won't change depth in real-time
   - But stereo effect should be visible and correct
3. **After slider fix**:
   - Depth adjusts smoothly with hardware slider
   - Slider down → instant 2D
   - Slider up → gradual depth increase

**Test Procedure:**
1. Flash build to 3DS
2. Load Super Mario World
3. Enable stereo in menu (set StereoSliderValue to 1.0)
4. Move hardware slider up
5. Observe depth separation (backgrounds in, Mario out)
6. If depth doesn't change with slider: Confirms this validation
7. Apply slider integration fix
8. Rebuild and re-test

---

## Suggested Next Steps

1. **Option A: Test as-is**
   - Flash current build
   - Verify stereo works
   - Then add slider integration

2. **Option B: Fix slider first**
   - 5-10 minute code change
   - Rebuild
   - Flash and test complete experience

**Recommendation:** Option B - The slider fix is trivial and will make testing more pleasant.

---

## Implementation Notes for Slider Fix

**File:** `source/3dsimpl.cpp` (around line 595)

**Before:**
```cpp
// Plan E: Set layer offsets for BOTH eyes before rendering
if (settings3DS.EnableStereo3D)
    stereo3dsUpdateLayerOffsetsFromSlider(settings3DS.StereoSliderValue);
else
    stereo3dsUpdateLayerOffsetsFromSlider(0.0f);
```

**After:**
```cpp
// Plan E: Set layer offsets for BOTH eyes before rendering
// Combine hardware slider with user's max depth setting
float hwSlider = osGet3DSliderState();        // 0.0-1.0 from hardware
float userMax = settings3DS.StereoSliderValue; // User's max strength
float effective = hwSlider * userMax;         // Combined

if (settings3DS.EnableStereo3D && effective > 0.01f)
    stereo3dsUpdateLayerOffsetsFromSlider(effective);
else
    stereo3dsUpdateLayerOffsetsFromSlider(0.0f);
```

**Optional Enhancement (comfort scaling):**
```cpp
float effective = (hwSlider / 3.0f) * userMax;  // Divide slider by 3 for comfort
```

This reduces maximum depth to 1/3 of values, common practice in 3DS homebrew to prevent eye strain.

---

**Validation Complete**
**Ready for Hardware Testing with Optional Slider Enhancement**
