# Complete Guide to Stereoscopic 3D Rendering on Nintendo 3DS
## Educational Resource for SNES Layer Separation Project

**Last Updated:** 2025-10-21
**Author:** Research compilation from devkitPro examples, libctru source, and SNES9x analysis
**Purpose:** Comprehensive technical reference for implementing M2-style stereoscopic 3D

---

## Table of Contents

1. [Hardware Overview](#hardware-overview)
2. [Stereo Rendering Fundamentals](#stereo-rendering-fundamentals)
3. [3DS Autostereoscopic Display](#3ds-autostereoscopic-display)
4. [Citro3D/Citro2D API Reference](#citro3dcitro2d-api-reference)
5. [Stereo Projection Mathematics](#stereo-projection-mathematics)
6. [Implementation Patterns](#implementation-patterns)
7. [Performance Optimization](#performance-optimization)
8. [Common Pitfalls and Solutions](#common-pitfalls-and-solutions)

---

## Hardware Overview

### Nintendo 3DS Top Screen Specifications

| Property | Value | Notes |
|----------|-------|-------|
| **Display Type** | Autostereoscopic LCD | Parallax barrier technology |
| **Physical Resolution** | 800×240 pixels | Landscape orientation (hardware) |
| **Logical Resolution** | 240×400 pixels (per eye) | Portrait orientation (software) |
| **Screen Rotation** | 90° counterclockwise | Width/height are swapped |
| **Aspect Ratio** | 5:3 (400:240) | C3D_AspectRatioTop = 1.667 |
| **Stereo Method** | Parallax barrier | No glasses required |
| **Eye Separation** | 400 pixel columns each | Interlaced left/right columns |
| **3D Slider** | Hardware input | 0.0 (off) to 1.0 (max depth) |
| **Color Formats** | RGB565, RGB8, RGBA8 | GPU_RB_RGBA8 recommended |
| **Depth Buffer** | 16/24/24+8 bit | GPU_RB_DEPTH24_STENCIL8 |

### 3DS Bottom Screen (For Reference)

- **Resolution:** 240×320 pixels (portrait)
- **NO stereo support** - Always 2D
- **Aspect Ratio:** 4:3 (320:240) = C3D_AspectRatioBot

### Memory Architecture

- **VRAM:** 6 MB (at 0x1F000000)
- **Per-Frame Cost (RGBA8):** 240×400×4×2 eyes = 768 KB
- **Per-Frame Cost (RGB565):** 240×400×2×2 eyes = 384 KB
- **Depth Buffer (24-bit):** 240×400×3×2 = 576 KB
- **Total Stereo Frame:** ~1.3 MB (RGBA8) or ~950 KB (RGB565)

---

## Stereo Rendering Fundamentals

### Binocular Disparity

Human stereo vision works through **horizontal disparity** - each eye sees a slightly different view due to their physical separation (~65mm between pupils).

**Key Concepts:**

1. **Interocular Distance (IOD):**
   - Real human IOD ≈ 65mm
   - Virtual IOD in 3D graphics = camera separation
   - Formula: `iod = osGet3DSliderState() / 3.0f`
   - Typical range: 0.0 to 0.33 units

2. **Parallax:**
   - Positive parallax (crossed eyes) = object closer than screen
   - Zero parallax = object at screen plane
   - Negative parallax (diverged eyes) = object behind screen

3. **Focal Length (Screen Distance):**
   - Distance where objects appear at the screen plane
   - Objects closer → pop out
   - Objects farther → recede into screen
   - Typical value: 2.0 to 3.0 units

### Stereoscopic Depth Perception

```
                    Near Plane (0.01)
                         |
                         |  Objects here pop OUT
      Screen Plane ------*------ (focal length = 2.0)
                         |  Objects here go INTO screen
                         |
                    Far Plane (1000.0)
```

**Depth Cues:**

- **Convergence:** Eye rotation to focus on objects
- **Accommodation:** Eye lens focus adjustment (not simulated)
- **Motion Parallax:** Movement reveals depth
- **Occlusion:** Closer objects block farther ones

---

## 3DS Autostereoscopic Display

### Parallax Barrier Technology

The 3DS uses a **parallax barrier** - a physical LCD layer with vertical slits that directs light:

```
Left Eye    Right Eye
    \        /
     \      /
      \    /
       \  /
    [Parallax Barrier]
    [LCD Pixels]
    L R L R L R L R ...
```

- Odd pixel columns → Left eye only
- Even pixel columns → Right eye only
- Effective horizontal resolution halved (400 → 200 per eye)
- Vertical resolution unchanged (240 pixels)

### How gfxSet3D() Works

```c
gfxSet3D(true);  // Enable stereoscopic mode
```

**What this does:**

1. Reconfigures LCD controller to split framebuffer
2. Activates parallax barrier LCD layer
3. Maps framebuffer memory:
   - First 240×400 region → Left eye (GFX_LEFT)
   - Second 240×400 region → Right eye (GFX_RIGHT)
4. Hardware automatically interlaces pixel columns during display

**Critical:** Must call BEFORE creating render targets!

### 3D Slider Hardware

```c
float slider = osGet3DSliderState();  // Returns 0.0 to 1.0
```

**Implementation:**

- Physical slider position read from shared memory: `OS_SharedConfig->slider_3d`
- Updated by hardware automatically
- Poll every frame for dynamic adjustment
- Affected by auto-brightness (can reduce max slider value)

**Usage Pattern:**

```c
float slider = osGet3DSliderState();
float iod = slider / 3.0f;  // Scale to reasonable IOD

// Left eye: negative IOD (left camera position)
Mtx_PerspStereoTilt(&projLeft, fov, aspect, near, far, -iod, screen, false);

// Right eye: positive IOD (right camera position)
if (slider > 0.0f) {
    Mtx_PerspStereoTilt(&projRight, fov, aspect, near, far, iod, screen, false);
} else {
    // Slider at 0 = 2D mode, skip right eye rendering
}
```

---

## Citro3D/Citro2D API Reference

### Core Initialization Functions

#### gfxInitDefault()
```c
void gfxInitDefault(void);
```
- Initializes LCD framebuffers with defaults (BGR8_OES format)
- Equivalent to: `gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, false)`
- Stereo OFF by default, double buffering ON

#### gfxInit()
```c
void gfxInit(GSPGPU_FramebufferFormat topFormat,
             GSPGPU_FramebufferFormat bottomFormat,
             bool vramBuffers);
```
- **topFormat:** Color format for top screen (e.g., GPU_RB_RGBA8, GSP_BGR8_OES)
- **bottomFormat:** Color format for bottom screen
- **vramBuffers:** Allocate in VRAM (true) or LINEAR RAM (false)
- Note: Internally calls gspInit()

#### gfxSet3D()
```c
void gfxSet3D(bool enable);
```
- **enable:** true = stereo mode, false = mono mode
- **CRITICAL:** Call BEFORE creating render targets
- Reconfigures LCD and framebuffer mapping

#### gfxIs3D()
```c
bool gfxIs3D(void);
```
- Returns current stereo mode state
- Useful for conditional rendering logic

### Render Target Management

#### C3D_RenderTargetCreate()
```c
C3D_RenderTarget* C3D_RenderTargetCreate(int width, int height,
                                         GPU_COLORBUF colorFmt,
                                         GPU_DEPTHBUF depthFmt);
```
**Parameters:**
- **width, height:** Target dimensions (240×400 for top screen)
- **colorFmt:** GPU_RB_RGB565, GPU_RB_RGB8, GPU_RB_RGBA8
- **depthFmt:** GPU_RB_DEPTH16, GPU_RB_DEPTH24, GPU_RB_DEPTH24_STENCIL8

**Example:**
```c
C3D_RenderTarget* left = C3D_RenderTargetCreate(240, 400,
                                                 GPU_RB_RGBA8,
                                                 GPU_RB_DEPTH24_STENCIL8);
C3D_RenderTarget* right = C3D_RenderTargetCreate(240, 400,
                                                  GPU_RB_RGBA8,
                                                  GPU_RB_DEPTH24_STENCIL8);
```

#### C3D_RenderTargetSetOutput()
```c
void C3D_RenderTargetSetOutput(C3D_RenderTarget* target,
                               gfxScreen_t screen,
                               gfx3dSide_t side,
                               u32 transferFlags);
```
**Parameters:**
- **target:** Render target to configure
- **screen:** GFX_TOP or GFX_BOTTOM
- **side:** GFX_LEFT or GFX_RIGHT (top screen only)
- **transferFlags:** GPU transfer flags (usually 0 or GX_TRANSFER_FLIP_VERT)

**Example:**
```c
C3D_RenderTargetSetOutput(left, GFX_TOP, GFX_LEFT,
                          GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0));
C3D_RenderTargetSetOutput(right, GFX_TOP, GFX_RIGHT,
                          GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0));
```

#### Citro2D Simplified Targets
```c
C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);   // Left eye
C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);  // Right eye
```
- Higher-level API, automatically configures output
- Returns C3D_RenderTarget*
- Handles screen rotation internally

### Stereo Projection Functions

#### Mtx_PerspStereoTilt() - PRIMARY FUNCTION
```c
void Mtx_PerspStereoTilt(C3D_Mtx* mtx,
                         float fovy,
                         float aspect,
                         float near,
                         float far,
                         float iod,
                         float screen,
                         bool isLeftHanded);
```

**Parameters:**

- **mtx:** Output projection matrix (4x4)
- **fovy:** Vertical field of view in **radians**
  - Use `C3D_AngleFromDegrees(40.0f)` to convert from degrees
  - Typical values: 40-60 degrees
- **aspect:** Aspect ratio (width/height)
  - Top screen: `C3D_AspectRatioTop` = 400/240 = 1.667
- **near:** Near clip plane distance
  - Typical: 0.01 to 0.1
  - Objects closer than this are culled
- **far:** Far clip plane distance
  - Typical: 100.0 to 1000.0
  - Objects farther than this are culled
- **iod:** Interocular distance (eye separation)
  - **Left eye:** Use **negative** value (e.g., -iod)
  - **Right eye:** Use **positive** value (e.g., +iod)
  - Typical: `slider / 3.0f` = 0.0 to 0.33
- **screen:** Focal length (screen plane distance)
  - Objects at this depth appear at screen plane
  - Objects closer pop out, farther recede
  - Typical: 2.0 to 3.0
- **isLeftHanded:** Coordinate system handedness
  - false = Right-handed (OpenGL style) - **RECOMMENDED**
  - true = Left-handed (Direct3D style)

**Example:**
```c
C3D_Mtx projection;
float fov = C3D_AngleFromDegrees(40.0f);
float aspect = C3D_AspectRatioTop;
float slider = osGet3DSliderState();
float iod = slider / 3.0f;

// Left eye projection
Mtx_PerspStereoTilt(&projection, fov, aspect, 0.01f, 1000.0f,
                    -iod,  // Negative for left eye
                    2.0f, false);

// Right eye projection
Mtx_PerspStereoTilt(&projection, fov, aspect, 0.01f, 1000.0f,
                    iod,   // Positive for right eye
                    2.0f, false);
```

#### Mtx_PerspStereo() - Without Tilt
```c
void Mtx_PerspStereo(C3D_Mtx* mtx, float fovy, float aspect,
                     float near, float far, float iod,
                     float screen, bool isLeftHanded);
```
- Same as `Mtx_PerspStereoTilt()` but **without 90° screen rotation**
- **DO NOT USE** for top screen (screen is rotated 90°!)
- Only use for non-3DS displays or special cases

### Frame Rendering Loop

#### C3D_FrameBegin()
```c
bool C3D_FrameBegin(u8 flags);
```
**Flags:**
- **C3D_FRAME_SYNCDRAW:** Synchronize with display refresh (60 Hz)
- **C3D_FRAME_NONBLOCK:** Don't wait if GPU busy
- **0:** No sync, immediate return

**Returns:** true if frame started successfully

**Typical:**
```c
C3D_FrameBegin(C3D_FRAME_SYNCDRAW);  // Wait for VBlank
```

#### C3D_FrameDrawOn()
```c
void C3D_FrameDrawOn(C3D_RenderTarget* target);
```
- Switch rendering to specified target
- Call before rendering to each eye

#### C3D_FrameEnd()
```c
void C3D_FrameEnd(u8 flags);
```
- Submit frame to GPU for display
- **flags:** Usually 0

### Performance Monitoring

#### C3D_GetDrawingTime()
```c
float C3D_GetDrawingTime(void);
```
- Returns GPU time for last frame (as fraction of frame time)
- Multiply by 6.0 for percentage at 60 FPS
- Example: `0.5 * 6.0 = 30%` GPU utilization

#### C3D_GetProcessingTime()
```c
float C3D_GetProcessingTime(void);
```
- Returns CPU time for last frame (as fraction)
- Multiply by 6.0 for percentage

#### C3D_GetCmdBufUsage()
```c
float C3D_GetCmdBufUsage(void);
```
- Returns command buffer usage (0.0 to 1.0)
- Multiply by 100 for percentage
- If > 90%, may need to increase buffer size

---

## Stereo Projection Mathematics

### Perspective Projection Matrix

Standard perspective projection (no stereo):

```
[ f/aspect   0       0           0        ]
[    0       f       0           0        ]
[    0       0    (far+near)/(near-far)   2*far*near/(near-far) ]
[    0       0      -1           0        ]

where f = 1 / tan(fov/2)
```

### Stereo Perspective Projection

Stereo adds **asymmetric frustum** (off-axis projection):

```
Left Eye:
- Camera shifted LEFT by iod/2
- Frustum shifted RIGHT to keep screen centered
- Creates positive parallax for near objects

Right Eye:
- Camera shifted RIGHT by iod/2
- Frustum shifted LEFT to keep screen centered
- Creates negative parallax for near objects
```

**Parallax Formula:**

```
parallax_offset = (iod * depth) / (screen + depth)

where:
  - depth = distance from camera to object
  - screen = focal length (distance to screen plane)
  - iod = interocular distance
```

**For 2D Sprites (Citro2D approach):**

```
x_left = x_base + (layer_depth * slider)
x_right = x_base - (layer_depth * slider)

where:
  - layer_depth = arbitrary depth value (e.g., 0-50 pixels)
  - slider = osGet3DSliderState() (0.0-1.0)
```

### Depth Calculation Examples

For a game with multiple SNES layers:

```
BG4 (farthest):   depth = 0    → No parallax, at screen plane
BG3:              depth = 1    → Slight depth
BG2:              depth = 2    → Medium depth
BG1:              depth = 3    → More depth
Sprites (near):   depth = 5    → Closest layer, pops out

Left eye X positions (with slider = 1.0):
  BG4:  x + 0*1.0 = x
  BG3:  x + 1*1.0 = x+1
  BG2:  x + 2*1.0 = x+2
  BG1:  x + 3*1.0 = x+3
  Sprites: x + 5*1.0 = x+5

Right eye: Same but subtract
```

---

## Implementation Patterns

### Pattern 1: Simple 2D Stereo (Citro2D)

**Use Case:** 2D sprites, UI elements, simple layer separation

```c
#include <citro2d.h>
#include <3ds.h>

int main() {
    // Init
    gfxInitDefault();
    gfxSet3D(true);  // CRITICAL: Enable stereo
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create render targets
    C3D_RenderTarget* left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);

    // Load assets
    C2D_SpriteSheet sheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    C2D_Image bgLayer = C2D_SpriteSheetGetImage(sheet, 0);
    C2D_Image fgLayer = C2D_SpriteSheetGetImage(sheet, 1);

    // Main loop
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;

        float slider = osGet3DSliderState();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        {
            // Left eye
            C2D_TargetClear(left, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(left);
            C2D_DrawImageAt(bgLayer, 100 + 0*slider, 50, 0);   // Far layer
            C2D_DrawImageAt(fgLayer, 100 + 10*slider, 100, 0); // Near layer

            // Right eye
            C2D_TargetClear(right, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(right);
            C2D_DrawImageAt(bgLayer, 100 - 0*slider, 50, 0);   // Far layer
            C2D_DrawImageAt(fgLayer, 100 - 10*slider, 100, 0); // Near layer
        }
        C3D_FrameEnd(0);
    }

    // Cleanup
    C2D_SpriteSheetFree(sheet);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}
```

**Pros:**
- Simple, minimal code
- Low GPU overhead
- Perfect for 2D layer separation

**Cons:**
- No true 3D depth
- Limited parallax range
- Manual offset calculation

### Pattern 2: Full 3D Stereo (Citro3D)

**Use Case:** 3D scenes, complex geometry, proper depth

```c
#include <citro3d.h>
#include <3ds.h>

int main() {
    // Init
    gfxInitDefault();
    gfxSet3D(true);
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    // Create targets
    C3D_RenderTarget* targetLeft = C3D_RenderTargetCreate(240, 400,
                                        GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTarget* targetRight = C3D_RenderTargetCreate(240, 400,
                                        GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(targetLeft, GFX_TOP, GFX_LEFT,
                              GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0));
    C3D_RenderTargetSetOutput(targetRight, GFX_TOP, GFX_RIGHT,
                              GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0));

    // Load shaders and models (omitted for brevity)

    // Main loop
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;

        float slider = osGet3DSliderState();
        float iod = slider / 3.0f;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        {
            // Left eye
            C3D_FrameDrawOn(targetLeft);
            C3D_FVec clearColor = FVec4_New(0.0f, 0.0f, 0.0f, 1.0f);
            C3D_RenderTargetClear(targetLeft, C3D_CLEAR_ALL, __builtin_bswap32(0x000000FF), 0);

            C3D_Mtx projection;
            Mtx_PerspStereoTilt(&projection,
                               C3D_AngleFromDegrees(40.0f),
                               C3D_AspectRatioTop,
                               0.01f, 1000.0f,
                               -iod,  // Left eye
                               2.0f, false);

            // Render scene with projection matrix...

            // Right eye (only if slider > 0)
            if (iod > 0.0f) {
                C3D_FrameDrawOn(targetRight);
                C3D_RenderTargetClear(targetRight, C3D_CLEAR_ALL, __builtin_bswap32(0x000000FF), 0);

                Mtx_PerspStereoTilt(&projection,
                                   C3D_AngleFromDegrees(40.0f),
                                   C3D_AspectRatioTop,
                                   0.01f, 1000.0f,
                                   iod,   // Right eye
                                   2.0f, false);

                // Render scene again with different projection...
            }
        }
        C3D_FrameEnd(0);
    }

    // Cleanup
    C3D_RenderTargetDelete(targetLeft);
    C3D_RenderTargetDelete(targetRight);
    C3D_Fini();
    gfxExit();
}
```

### Pattern 3: Hybrid (RECOMMENDED FOR SNES9X)

**Use Case:** Emulators with GPU-rendered tiles, stereo depth separation

```c
// Keep existing SNES9x GPU tile rendering
// Add stereo offset at layer composition stage

void RenderLayerStereo(Layer layer, float depthOffset, bool isRightEye) {
    float slider = osGet3DSliderState();
    float parallax = depthOffset * slider;

    // Modify layer X position based on eye
    float xOffset = isRightEye ? -parallax : parallax;

    // Render layer with existing GPU code + offset
    RenderLayerToGPU(layer, xOffset);
}

void RenderFrame() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    // Left eye
    C3D_FrameDrawOn(targetLeft);
    RenderLayerStereo(BG4, 0.0f, false);   // Farthest
    RenderLayerStereo(BG3, 5.0f, false);
    RenderLayerStereo(BG2, 10.0f, false);
    RenderLayerStereo(BG1, 15.0f, false);
    RenderLayerStereo(SPRITES, 20.0f, false); // Closest

    // Right eye
    C3D_FrameDrawOn(targetRight);
    RenderLayerStereo(BG4, 0.0f, true);
    RenderLayerStereo(BG3, 5.0f, true);
    RenderLayerStereo(BG2, 10.0f, true);
    RenderLayerStereo(BG1, 15.0f, true);
    RenderLayerStereo(SPRITES, 20.0f, true);

    C3D_FrameEnd(0);
}
```

---

## Performance Optimization

### Target Frame Rate

**60 FPS (Ideal):**
- 16.67ms frame budget
- Render both eyes in < 16ms
- Requires efficient rendering

**30 FPS (Realistic for Stereo):**
- 33.33ms frame budget
- More comfortable for complex scenes
- Less eye strain

### Optimization Strategies

#### 1. Skip Right Eye When Slider = 0

```c
float slider = osGet3DSliderState();
if (slider > 0.01f) {
    // Render both eyes
} else {
    // Render left eye only (2D mode)
    // 50% performance gain!
}
```

#### 2. Use RGB565 Instead of RGBA8

```c
// RGBA8: 4 bytes/pixel = 768 KB per stereo frame
// RGB565: 2 bytes/pixel = 384 KB per stereo frame
// 50% memory bandwidth reduction!

C3D_RenderTargetCreate(240, 400, GPU_RB_RGB565, GPU_RB_DEPTH16);
```

#### 3. Share Depth Buffer (If Possible)

Some scenes allow sharing the depth buffer between eyes:
- Same geometry
- Only camera position differs
- Saves 576 KB per frame!

#### 4. Reduce Draw Calls

- Batch sprites into sprite sheets
- Use texture atlases
- Minimize state changes

#### 5. Optimize Shader Complexity

- Simpler shaders = faster rendering
- Consider LOD (Level of Detail) for distant objects

#### 6. Use New 3DS XL Features

```c
if (osIsNew3DS()) {
    // Use 804 MHz CPU mode
    // Access L2 cache
    // More VRAM available
}
```

### Performance Monitoring

```c
float gpu_time = C3D_GetDrawingTime() * 6.0f;      // GPU %
float cpu_time = C3D_GetProcessingTime() * 6.0f;   // CPU %
float cmd_usage = C3D_GetCmdBufUsage() * 100.0f;   // Buffer %

printf("GPU: %.1f%%  CPU: %.1f%%  Buf: %.1f%%\n",
       gpu_time, cpu_time, cmd_usage);

// Target: < 100% for all metrics at 60 FPS
// If > 100%, consider 30 FPS or optimization
```

---

## Common Pitfalls and Solutions

### Pitfall 1: Forgot gfxSet3D(true)

**Symptom:** No stereo effect, or garbled display

**Solution:**
```c
gfxInitDefault();
gfxSet3D(true);  // MUST be here!
C3D_Init(...);
```

### Pitfall 2: Wrong IOD Sign

**Symptom:** Reversed depth, eye strain

**Solution:**
```c
// Left eye: NEGATIVE iod
Mtx_PerspStereoTilt(&proj, ..., -iod, ...);

// Right eye: POSITIVE iod
Mtx_PerspStereoTilt(&proj, ..., +iod, ...);
```

### Pitfall 3: Excessive Parallax

**Symptom:** Eye strain, can't fuse stereo

**Solution:**
```c
// Keep parallax under 25 pixels for 400px width
float maxDepth = 25.0f;
float depth = min(layer_depth, maxDepth);
```

### Pitfall 4: Not Clearing Right Eye Buffer

**Symptom:** Ghosting, old frames visible

**Solution:**
```c
C2D_TargetClear(right, ...);  // Clear EVERY frame!
```

### Pitfall 5: Using Mtx_PerspStereo() Instead of Mtx_PerspStereoTilt()

**Symptom:** Display rotated 90°, incorrect aspect ratio

**Solution:**
```c
// ALWAYS use Mtx_PerspStereoTilt() for 3DS top screen!
Mtx_PerspStereoTilt(&proj, ...);  // Accounts for screen rotation
```

### Pitfall 6: Rendering Only When Slider > 0

**Symptom:** Blank screen when 3D off

**Solution:**
```c
// ALWAYS render left eye
C3D_FrameDrawOn(targetLeft);
renderScene(-iod);

// CONDITIONALLY render right eye
if (slider > 0.01f) {
    C3D_FrameDrawOn(targetRight);
    renderScene(+iod);
}
```

---

## Additional Resources

### Official Documentation

- **libctru API Docs:** https://libctru.devkitpro.org/
- **Citro3D Docs:** https://github.com/devkitPro/citro3d
- **Citro2D Docs:** https://citro2d.devkitpro.org/

### Example Code

- **devkitPro 3DS Examples:** `<PROJECT_ROOT>/repos/devkitpro-3ds-examples/`
- **stereoscopic_2d:** Simple 2D stereo example
- **particles:** Full 3D stereo with depth
- **lenny:** Complete stereo scene with lighting

### Community Resources

- **GBATemp Forum:** https://gbatemp.net/ (3DS homebrew section)
- **devkitPro Discord:** Active developer community
- **Reddit /r/3dshacks:** Homebrew discussions

---

## Conclusion

Stereoscopic 3D on the Nintendo 3DS combines:
1. Hardware autostereoscopic display (parallax barrier)
2. Software dual-view rendering (left/right eyes)
3. Dynamic depth control (3D slider)

Key takeaways:
- Call `gfxSet3D(true)` before creating targets
- Use `Mtx_PerspStereoTilt()` for 3D, or manual X-offset for 2D
- Respect the 3D slider value for comfort
- Optimize for 30-60 FPS with dual rendering load
- Test on hardware (emulators may not support stereo)

For SNES layer separation, **hybrid 2D offset approach** is recommended:
- Keep existing GPU tile rendering
- Add per-layer parallax offset
- Simple, performant, and effective

Good luck with your stereoscopic 3D implementation!
