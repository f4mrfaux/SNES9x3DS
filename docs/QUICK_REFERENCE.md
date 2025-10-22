# Stereoscopic 3D Quick Reference
## Essential Info at a Glance

**Last Updated:** 2025-10-21

---

## Critical API Functions

### Stereo Initialization

```c
gfxInitDefault();                    // Init graphics
gfxSet3D(true);                      // ★ CRITICAL: Enable stereo FIRST
C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);   // Init Citro3D
```

### 3D Slider

```c
float slider = osGet3DSliderState();  // Returns 0.0-1.0
```

### Render Targets (Citro2D)

```c
C3D_RenderTarget* left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
C3D_RenderTarget* right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
```

### Stereo Projection (Citro3D)

```c
Mtx_PerspStereoTilt(&projection,
    C3D_AngleFromDegrees(40.0f),     // FOV
    C3D_AspectRatioTop,              // Aspect (400/240)
    0.01f,                           // Near plane
    1000.0f,                         // Far plane
    -iod,                            // IOD (negative for left eye)
    2.0f,                            // Focal length
    false);                          // Right-handed coords
```

### Rendering Loop

```c
C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
  // Left eye
  C2D_SceneBegin(left);
  C2D_DrawImageAt(sprite, x + depth*slider, y, 0);

  // Right eye
  C2D_SceneBegin(right);
  C2D_DrawImageAt(sprite, x - depth*slider, y, 0);
C3D_FrameEnd(0);
```

---

## Parallax Formula

### 2D Sprite Offset (Citro2D)

```
Left Eye:   x_left = base_x + (depth × slider)
Right Eye:  x_right = base_x - (depth × slider)

where:
  depth = layer depth value (0-25 pixels recommended)
  slider = osGet3DSliderState() (0.0-1.0)
```

### Depth Perception

- **depth > 0:** Object pops OUT of screen (positive parallax)
- **depth = 0:** Object AT screen plane (no parallax)
- **depth < 0:** Object goes INTO screen (negative parallax)

---

## SNES9x Injection Points

### Main Files

| File | Line | Function | Purpose |
|------|------|----------|---------|
| **gfxhw.cpp** | 3415 | S9xRenderScreenHardware() | Main render loop |
| **gfxhw.cpp** | 1509 | S9xDrawBackgroundHardwarePriority0() | BG depth calc |
| **gfxhw.cpp** | 2578 | S9xDrawOBJSHardware() | Sprite depth calc |
| **3dsgpu.cpp** | 152 | gpu3dsCheckSlider() | 3D slider polling |
| **3dsimpl.cpp** | 502 | impl3dsRunOneFrame() | Main loop |

### Depth Modification

**Current:**
```cpp
u16 depth = priority * 256 + alpha;
```

**Stereo:**
```cpp
u16 depth = priority * 256;
if (settings3DS.enableStereo3D) {
    float offset = settings3DS.layerDepth[layerType] * slider;
    depth += (u16)clamp(offset, 0, 255);
}
depth += alpha;
```

---

## Default Depth Profiles

### Conservative (Safe)

```
BG3 (far):  0.0   ← At screen plane
BG2:        5.0
BG1:       10.0
BG0:       15.0
Sprites:   20.0   ← Pop out
```

### Moderate

```
BG3:        0.0
BG2:       10.0
BG1:       15.0
BG0:       20.0
Sprites:   30.0
```

### Aggressive (Eye strain risk!)

```
BG3:        0.0
BG2:       20.0
BG1:       30.0
BG0:       40.0
Sprites:   50.0
```

**Recommendation:** Start conservative, tune per-game

---

## Per-Game Profiles

### Super Mario World
```
{15.0, 10.0, 5.0, 0.0, 20.0}  // BG0-3, Sprites
Strength: 1.0
Notes: BG3=clouds(far), sprites=Mario/enemies(near)
```

### Donkey Kong Country
```
{20.0, 15.0, 5.0, 0.0, 25.0}
Strength: 1.2
Notes: Prerendered graphics, can handle strong depth
```

### Chrono Trigger
```
{10.0, 8.0, 5.0, 0.0, 15.0}
Strength: 0.8
Notes: Conservative for battle backgrounds
```

### F-Zero (Mode 7)
```
Special: Per-scanline depth
Y-based: depth = (y / 224.0) * 30.0
Notes: Road recession effect
```

---

## Hardware Specs

### 3DS Top Screen

- **Resolution:** 240×400 pixels (per eye)
- **Aspect Ratio:** 5:3 (C3D_AspectRatioTop = 1.667)
- **Rotation:** 90° counterclockwise (portrait mode)
- **Stereo:** Parallax barrier (interlaced columns)
- **VRAM:** 6 MB total
- **Frame Cost (RGBA8):** 768 KB per stereo frame
- **Frame Cost (RGB565):** 384 KB per stereo frame

### Performance Targets

- **Old 3DS:** 30 FPS (stereo may struggle)
- **New 3DS XL:** 60 FPS (recommended platform)

---

## Common Pitfalls

### ❌ Forgot gfxSet3D(true)

**Symptom:** No stereo effect

**Fix:**
```c
gfxInitDefault();
gfxSet3D(true);  // ← MUST BE HERE!
C3D_Init(...);
```

### ❌ Wrong IOD Sign

**Symptom:** Reversed depth

**Fix:**
```c
// Left eye: NEGATIVE iod
Mtx_PerspStereoTilt(&proj, ..., -iod, ...);

// Right eye: POSITIVE iod
Mtx_PerspStereoTilt(&proj, ..., +iod, ...);
```

### ❌ Excessive Parallax

**Symptom:** Eye strain

**Fix:**
```c
// Keep depth under 25 pixels for 400px width
float maxDepth = 25.0f;
```

### ❌ Not Clearing Right Eye Buffer

**Symptom:** Ghosting

**Fix:**
```c
C2D_TargetClear(right, color);  // ← Every frame!
```

### ❌ Used Mtx_PerspStereo() Instead of Tilt

**Symptom:** Wrong aspect ratio

**Fix:**
```c
// ALWAYS use Tilt version for 3DS
Mtx_PerspStereoTilt(&proj, ...);  // ← Correct
```

---

## Performance Tips

### 1. Skip Right Eye When Slider = 0

```c
if (slider > 0.01f) {
    // Render both eyes
} else {
    // Render left eye only (2D mode)
    // 50% performance gain!
}
```

### 2. Use RGB565 Instead of RGBA8

```c
// 50% memory bandwidth reduction
C3D_RenderTargetCreate(240, 400, GPU_RB_RGB565, ...);
```

### 3. Precompute Per-Frame

```c
// Don't calculate per-tile:
float mult = slider * strength;  // ← Once per frame

// Then use:
offset = layerDepth[i] * mult;  // ← Per tile
```

### 4. Profile Performance

```c
float gpuTime = C3D_GetDrawingTime() * 6.0f;    // GPU %
float cpuTime = C3D_GetProcessingTime() * 6.0f; // CPU %
```

**Target:** < 100% for 60 FPS

---

## Build Commands

### Build SNES9x

```bash
cd <PROJECT_ROOT>/repos/matbo87-snes9x_3ds
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
make clean && make -j4
```

**Output:** `output/matbo87-snes9x_3ds.3dsx`

### Build stereoscopic_2d Example

```bash
cd <PROJECT_ROOT>/repos/devkitpro-3ds-examples/graphics/gpu/stereoscopic_2d
make clean && make
```

**Output:** `stereoscopic_2d.3dsx`

---

## Testing Checklist

### Basic Functionality

- [ ] 3D slider controls depth
- [ ] Slider at 0 = 2D mode (no stereo)
- [ ] Slider at 1.0 = max depth
- [ ] No eye strain at moderate depths
- [ ] No rendering artifacts

### Per-Game Testing

- [ ] Backgrounds separated by depth
- [ ] Sprites pop out (or at correct depth)
- [ ] No flickering or ghosting
- [ ] 60 FPS maintained (or stable 30 FPS)
- [ ] HUD readable (at screen plane or slight popout)

### Edge Cases

- [ ] Mode 7 games (F-Zero, Mario Kart)
- [ ] Transparency effects (Super Metroid)
- [ ] Per-scanline effects (water waves)
- [ ] High sprite count (Contra 3 boss fights)

---

## File Paths Reference

### Project Structure

```
<PROJECT_ROOT>/
├── repos/
│   ├── matbo87-snes9x_3ds/           ← Main SNES9x codebase
│   │   ├── source/
│   │   │   ├── Snes9x/gfxhw.cpp     ← Rendering pipeline ★
│   │   │   ├── 3dsgpu.cpp            ← GPU functions
│   │   │   ├── 3dsimpl.cpp           ← Main loop
│   │   │   └── 3dssettings.h         ← Config ★
│   │   └── output/
│   │       └── *.3dsx                ← Build output
│   ├── devkitpro-3ds-examples/       ← Example code
│   │   └── graphics/gpu/
│   │       └── stereoscopic_2d/      ← 2D stereo example ★
│   └── devkitpro/citro3d/            ← Citro3D library source
└── docs/
    ├── STEREO_RENDERING_COMPLETE_GUIDE.md    ← API reference
    ├── SNES9X_GPU_ARCHITECTURE.md            ← Architecture deep-dive
    ├── IMPLEMENTATION_ROADMAP.md             ← Step-by-step plan
    └── QUICK_REFERENCE.md                    ← This file
```

### Header Files

```
/opt/devkitpro/libctru/include/
├── 3ds/gfx.h         ← gfxSet3D(), GFX_LEFT/RIGHT
├── 3ds/os.h          ← osGet3DSliderState()
├── citro3d.h         ← C3D_* functions
├── citro2d.h         ← C2D_* functions
└── c3d/maths.h       ← Mtx_PerspStereoTilt()
```

---

## Useful Constants

```c
// Screen dimensions
#define SCREEN_TOP_WIDTH  400
#define SCREEN_TOP_HEIGHT 240
#define SCREEN_BOT_WIDTH  320
#define SCREEN_BOT_HEIGHT 240

// Aspect ratios
#define C3D_AspectRatioTop (400.0f / 240.0f)  // 1.667
#define C3D_AspectRatioBot (320.0f / 240.0f)  // 1.333

// Angle conversion
#define C3D_AngleFromDegrees(deg) ((deg) * M_TAU / 360.0f)

// Color formats
GPU_RB_RGBA8       // 32-bit RGBA (4 bytes/pixel)
GPU_RB_RGB8        // 24-bit RGB (3 bytes/pixel)
GPU_RB_RGB565      // 16-bit RGB (2 bytes/pixel)

// Depth formats
GPU_RB_DEPTH16            // 16-bit depth
GPU_RB_DEPTH24            // 24-bit depth
GPU_RB_DEPTH24_STENCIL8   // 24-bit depth + 8-bit stencil
```

---

## Documentation Cross-Reference

### For API Details
→ `STEREO_RENDERING_COMPLETE_GUIDE.md`

### For Architecture Understanding
→ `SNES9X_GPU_ARCHITECTURE.md`

### For Implementation Steps
→ `IMPLEMENTATION_ROADMAP.md`

### For Original Project Plan
→ `PROJECT_OUTLINE.md`

---

## Contacts & Resources

### Official Documentation

- **libctru:** https://libctru.devkitpro.org/
- **Citro3D:** https://github.com/devkitPro/citro3d
- **Citro2D:** https://citro2d.devkitpro.org/

### Community

- **GBATemp (3DS Homebrew):** https://gbatemp.net/categories/nintendo-3ds-homebrew-development-and-emulators.275/
- **devkitPro Discord:** https://discord.gg/dWeHxdc
- **Reddit /r/3dshacks:** https://reddit.com/r/3dshacks

### Example Repositories

- **SNES9x 3DS (matbo87):** https://github.com/matbo87/snes9x_3ds
- **devkitPro Examples:** https://github.com/devkitPro/3ds-examples

---

## Quick Start (Absolute Minimum)

**For 2D Stereo (Citro2D):**

```c
#include <citro2d.h>

gfxInitDefault();
gfxSet3D(true);  // ← Enable stereo
C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
C2D_Prepare();

auto left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
auto right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);

// Load sprite...
while (aptMainLoop()) {
    float slider = osGet3DSliderState();

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    {
        // Left eye
        C2D_TargetClear(left, BLACK);
        C2D_SceneBegin(left);
        C2D_DrawImageAt(sprite, 100 + 10*slider, 50, 0);

        // Right eye
        C2D_TargetClear(right, BLACK);
        C2D_SceneBegin(right);
        C2D_DrawImageAt(sprite, 100 - 10*slider, 50, 0);
    }
    C3D_FrameEnd(0);
}
```

**That's it!** Compile and run. Adjust 3D slider to see depth effect.

---

## Troubleshooting Decision Tree

```
NO STEREO EFFECT?
├─ Is gfxSet3D(true) called?
│  ├─ NO → Add it before C3D_Init()
│  └─ YES → Continue
├─ Is 3D slider > 0?
│  ├─ NO → Move slider up
│  └─ YES → Continue
├─ Are left/right targets created?
│  ├─ NO → Create both targets
│  └─ YES → Continue
└─ Is parallax offset applied?
   ├─ NO → Add x ± depth*slider
   └─ YES → Check offset calculation

EYE STRAIN?
├─ Reduce depth values by 50%
├─ Lower stereoStrength to 0.5
└─ Test at lower slider positions

ARTIFACTS/GHOSTING?
├─ Clear both targets every frame
├─ Check depth values (0-1000 range)
└─ Verify alpha blending disabled for opaque layers

POOR PERFORMANCE?
├─ Skip right eye when slider=0
├─ Use RGB565 instead of RGBA8
├─ Profile with C3D_GetDrawingTime()
└─ Consider 30 FPS target
```

---

## Version History

- **v1.0** (2025-10-21): Initial quick reference
  - Based on comprehensive research
  - Ready for prototype development

---

**Next Step:** Follow `IMPLEMENTATION_ROADMAP.md` Week 1 to start building!
