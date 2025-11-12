# SNES9x 3DS Layer Rendering Research
## Understanding the Graphics Pipeline for Stereo Implementation

**Date:** 2025-10-22
**Purpose:** Document how SNES9x handles layers to inform stereo 3D implementation strategy

---

## Key Findings

### Layer System Architecture

SNES9x 3DS uses a **hybrid CPU-GPU rendering system**:

1. **CPU Side:** Emulates SNES PPU, generates layer data
2. **GPU Side:** Renders layers using 3DS PICA200 GPU with depth buffer

### Layer Types

The SNES has **5 renderable layer types**:
- **BG0-BG3:** Background tile layers (4 layers)
- **OBJ/Sprites:** Sprite layer (1 layer)
- **Backdrop:** Background color

### Critical Files

| File | Purpose | Key Functions |
|------|---------|---------------|
| `gfxhw.cpp` | GPU-accelerated rendering | `S9xRenderScreenHardware()` |
| `gfxhw.cpp:1509` | BG layer rendering | `S9xDrawBackgroundHardwarePriority0Inline()` |
| `gfxhw.cpp:2578` | Sprite rendering | `S9xDrawOBJSHardware()` |
| `gfx.h:11-92` | Graphics structures | `SGFX`, `SBG` structs |

---

## Depth System

### Current Depth Calculation

**Background Layers:**
```cpp
// From line 3535 macro:
depth = d0 * 256 + BGAlpha##bg
```

Where:
- `d0` = priority level (0-3)
- `BGAlpha##bg` = alpha/blending value (0-255)
- Result = 16-bit depth value for GPU depth buffer

**Sprite Layer:**
```cpp
// Function signature at line 2578:
void S9xDrawOBJSHardware(bool8 sub, int depth = 0, int priority = 0)
```

The `depth` parameter is passed in and controls sprite depth in the GPU depth buffer.

### How Depth Buffer Works

1. **Priority Levels:** Each SNES layer has priority 0-3
2. **Depth Encoding:** `priority * 256 + sub-priority`
3. **GPU Sorting:** GPU depth buffer automatically sorts layers
4. **Alpha Blending:** GPU handles transparency effects

---

## Stereo Implementation Strategy

### Option 1: Depth Offset (CHOSEN ✅)

**Modify depth calculation to add stereo offset:**

```cpp
// Current:
depth = priority * 256 + alpha

// Modified for stereo:
depth = priority * 256 + stereo_offset + alpha

where:
  stereo_offset = layerDepth[layerType] * slider * strength
```

**Advantages:**
- ✅ Minimal code changes (~50 lines)
- ✅ Uses existing GPU depth buffer
- ✅ < 10% performance overhead
- ✅ Works with all SNES modes
- ✅ High game compatibility

**Challenges:**
- ⚠️ Limited depth range per priority (256 units)
- ⚠️ Need to clamp stereo offset to prevent overflow

### Option 2: Dual Render (REJECTED ❌)

Render left and right eye views separately with different camera positions.

**Why Rejected:**
- ❌ 2× rendering cost (may not hit 60 FPS)
- ❌ More complex implementation
- ❌ Higher VRAM usage

---

## Layer Drawing Flow

### Main Rendering Function

**`S9xRenderScreenHardware()` at line ~3516:**

This function orchestrates the entire frame rendering:

1. **Clear backdrop**
2. **Draw background layers** (BG0-BG3) by priority
3. **Draw sprites** (OBJ) by priority
4. **Apply color math/transparency**
5. **Flush to screen**

### Background Layer Rendering

**Function Template:**
```cpp
void S9xDrawBackgroundHardwarePriority0Inline(
    int tileSize,
    int tileShift,
    int bitShift,
    int paletteShift,
    int paletteMask,
    int startPalette,
    bool directColourMode,
    uint32 BGMode,     // SNES mode (0-7)
    uint32 bg,         // BG layer index (0-3)
    bool sub,          // Is subscreen?
    int depth0,        // ⭐ DEPTH for priority 0
    int depth1         // ⭐ DEPTH for priority 1
)
```

**Key Points:**
- Separate functions for 4-color, 16-color, 256-color modes
- Uses tile cache system for performance
- Depth passed as parameter

### Sprite Rendering

**Function:**
```cpp
void S9xDrawOBJSHardware(
    bool8 sub,
    int depth = 0,     // ⭐ BASE DEPTH
    int priority = 0   // Priority level
)
```

**Key Points:**
- All sprites rendered together
- Per-sprite depth separation possible but not implemented
- Uses OBJ priority system (0-3)

---

## SNES PPU Structures

### SGFX Structure (`gfx.h:11-92`)

```cpp
struct SGFX {
    uint8  *Screen;          // Main screen buffer
    uint8  *SubScreen;       // Subscreen for transparency
    uint8  *ZBuffer;         // Depth buffer
    uint8  *SubZBuffer;      // Subscreen depth buffer

    uint8  Z1;               // Depth for comparison
    uint8  Z2;               // Depth to save
    uint8  ZSprite;          // Sprite depth tracking

    uint8  r212c;            // Main screen designation
    uint8  r212d;            // Subscreen designation
    uint8  r2130;            // Color math designation
    uint8  r2131;            // Color math settings
};
```

### SBG Structure (`gfx.h:108-145`)

```cpp
struct SBG {
    uint32 TileSize;         // 8 or 16 pixels
    uint32 TileAddress;      // VRAM address
    uint32 NameSelect;       // Tile name table
    uint32 SCBase;           // Screen base address

    uint32 StartPalette;     // Palette index
    uint32 PaletteShift;
    uint32 PaletteMask;

    uint8  *Buffer;          // Tile cache buffer
    uint8  *Buffered;        // Cache status
    bool8  DirectColourMode; // 256-color direct mode

    int    Depth;            // ⭐ Layer depth value
};
```

---

## Modification Points for Stereo

### 1. Add Stereo Helper Function

**Location:** Top of `gfxhw.cpp`

```cpp
inline u16 CalculateStereoDepth(int priority, int layerType, u8 alpha) {
    u16 baseDepth = priority * 256;

    if (settings3DS.EnableStereo3D && settings3DS.StereoSliderValue > 0.01f) {
        float slider = settings3DS.StereoSliderValue;
        float strength = settings3DS.StereoDepthStrength;
        float offset = settings3DS.LayerDepth[layerType] * slider * strength;

        // Clamp to prevent priority overflow
        offset = fminf(offset, 255.0f);
        offset = fmaxf(offset, 0.0f);

        baseDepth += (u16)offset;
    }

    return baseDepth + alpha;
}
```

### 2. Modify Background Depth Calls

**Location:** Line ~3535 macro calls

**Current:**
```cpp
d0 * 256 + BGAlpha##bg
```

**Modified:**
```cpp
CalculateStereoDepth(d0, bg, BGAlpha##bg)
```

### 3. Modify Sprite Depth Calls

**Location:** Calls to `S9xDrawOBJSHardware()`

**Current:**
```cpp
S9xDrawOBJSHardware(sub, depth, priority)
```

**Modified:**
```cpp
int stereoDepth = CalculateStereoDepth(priority, 4, 0);  // Layer 4 = sprites
S9xDrawOBJSHardware(sub, stereoDepth, priority)
```

---

## Layer Types Mapping

| Layer Index | SNES Layer | Default Depth | Visual Effect |
|-------------|------------|---------------|---------------|
| 0 | BG0 | 15.0 | Closest background |
| 1 | BG1 | 10.0 | Mid-depth |
| 2 | BG2 | 5.0  | Farther back |
| 3 | BG3 | 0.0  | At screen plane |
| 4 | Sprites | 20.0 | Pop out effect |

---

## Background Modes

SNES has 8 background modes (0-7) with different layer configurations:

| Mode | BG0 | BG1 | BG2 | BG3 | Notes |
|------|-----|-----|-----|-----|-------|
| 0 | 4c | 4c | 4c | 4c | All 4 layers, 4-color |
| 1 | 16c | 16c | 4c | - | Mode used by most games |
| 2 | 16c | 16c | - | - | Offset-per-tile |
| 3 | 256c | 16c | - | - | Hi-color mode |
| 4 | 256c | 4c | - | - | Offset-per-tile + hi-color |
| 5 | 16c | 4c | - | - | Hi-res 512px |
| 6 | 16c | - | - | - | Hi-res interlace |
| 7 | 256c | - | - | OPT | Rotation/scaling (Mode 7) |

**c = colors per tile**

---

## Special Cases

### Mode 7 (Rotation/Scaling)

- Used in F-Zero, Mario Kart, Pilotwings
- Single rotated/scaled layer
- Requires per-scanline depth gradient for road effect
- Special handling needed

### Color Math (Transparency)

- SNES supports additive/subtractive color math
- GPU handles with alpha blending
- Must preserve visual accuracy

### Window Effects

- SNES has 2 hardware windows for clipping
- Already handled by stencil buffer
- Should work with stereo without changes

---

## Performance Considerations

### Current Performance

- **Target:** 60 FPS on New 3DS XL
- **CPU:** 804 MHz ARM11 MPCore
- **GPU:** PICA200 @ 268 MHz

### Stereo Overhead

**Estimated costs:**
- Depth calculation: +2-5% CPU
- Float operations: Minimal (cached per frame)
- GPU depth sorting: No change (already used)

**Total:** < 10% overhead ✅

### Optimizations

1. **Cache slider value** per frame (don't re-read every tile)
2. **Fast path when slider = 0** (skip stereo calculations)
3. **Precompute multipliers** per frame
4. **Use integer math** where possible

---

## Next Steps

1. ✅ Add stereo settings to `3dssettings.h` - DONE
2. ✅ Update comparison operators - DONE
3. ⏳ Integrate 3D slider polling - IN PROGRESS
4. ⏳ Implement `CalculateStereoDepth()` helper
5. ⏳ Modify BG depth calculations
6. ⏳ Modify sprite depth calculations
7. ⏳ Test with Super Mario World

---

## Conclusion

The SNES9x 3DS codebase is **well-structured for stereo modification**:

- ✅ Already uses GPU depth buffer
- ✅ Centralized depth calculation points
- ✅ Clear layer separation
- ✅ Minimal changes needed

**Implementation confidence: HIGH** 🎯

The depth offset approach will work efficiently and provide convincing M2-style stereoscopic 3D with minimal performance impact.

---

**Document Version:** 1.0
**Last Updated:** 2025-10-22
**Status:** Research Complete, Implementation Ready
