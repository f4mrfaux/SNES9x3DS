# Nintendo 3DS Stereoscopic 3D Rendering API Reference

## Complete Analysis of Citro3D and Citro2D Stereoscopic Capabilities

### 1. STEREO PROJECTION FUNCTIONS (Citro3D Mathematics)

#### 1.1 Stereo Perspective Projection

**Core API Functions** in `/opt/devkitpro/libctru/include/c3d/maths.h`:

```c
// Basic stereo perspective projection (without screen tilt)
void Mtx_PerspStereo(C3D_Mtx* mtx, float fovy, float aspect, 
                     float near, float far, float iod, float screen, 
                     bool isLeftHanded);

// Stereo perspective projection with 3DS screen tilt (RECOMMENDED)
void Mtx_PerspStereoTilt(C3D_Mtx* mtx, float fovy, float aspect,
                         float near, float far, float iod, float screen, 
                         bool isLeftHanded);
```

**Parameters:**
- `fovy`: Vertical field of view in radians (use `C3D_AngleFromDegrees(angle)` helper)
- `aspect`: Aspect ratio (width/height). Use `C3D_AspectRatioTop` (400/240 = 1.667)
- `near`: Near clipping plane (e.g., 0.01f)
- `far`: Far clipping plane (e.g., 1000.0f)
- `iod`: Interocular distance (distance between the eyes)
  - **KEY INSIGHT**: Pass negative value for LEFT eye, positive for RIGHT eye
  - Typical range: -0.3 to 0.3 (depends on 3D slider)
  - Directly tied to `osGet3DSliderState()` return value
- `screen`: Focal length (convergence distance)
  - This is the depth plane where objects appear AT the screen surface
  - Objects closer pop OUT, objects farther go IN
  - Typical value: 2.0f (experimentation recommended)
- `isLeftHanded`: Whether to use left-handed or right-handed coordinate system

**Tilt Functions:**
The "Tilt" variants (`Mtx_PerspStereoTilt`) account for the 3DS's physical screen rotation
(screens are rotated 90 degrees counterclockwise) and should be used for all top-screen rendering.

#### 1.2 Non-Stereo Perspective (for comparison)

```c
void Mtx_Persp(C3D_Mtx* mtx, float fovy, float aspect, 
               float near, float far, bool isLeftHanded);

void Mtx_PerspTilt(C3D_Mtx* mtx, float fovy, float aspect,
                   float near, float far, bool isLeftHanded);
```

---

### 2. 3D SLIDER STATE API

**Location**: `/opt/devkitpro/libctru/include/3ds/os.h`

```c
// Gets the current state of the physical 3D slider
static inline float osGet3DSliderState(void)
{
    return OS_SharedConfig->slider_3d;  // Returns 0.0 to 1.0
}
```

**Characteristics:**
- **Range**: 0.0 (no 3D) to 1.0 (maximum 3D intensity)
- **Source**: Hardware register updated in real-time (polling)
- **Update Rate**: Continuously available via shared system configuration page
- **Typical Usage Pattern**:
  ```c
  float slider = osGet3DSliderState();
  float iod = slider / 3.0f;  // Scale to eye separation value
  // Then pass -iod and +iod to separate left/right projections
  ```

**Implementation Detail from Lenny Example**:
```c
float slider = osGet3DSliderState();
float iod = slider / 3;  // Typical scaling factor
// Render left eye with -iod, right eye with +iod
sceneRender(-iod);  // LEFT eye
sceneRender(iod);   // RIGHT eye (only if iod > 0)
```

---

### 3. FRAMEBUFFER & RENDER TARGET API

#### 3.1 Enable/Disable Stereoscopic Mode

**Location**: `/opt/devkitpro/libctru/include/3ds/gfx.h`

```c
// Enable or disable stereoscopic 3D on the top screen
void gfxSet3D(bool enable);

// Check if 3D is currently enabled
bool gfxIs3D(void);

// Initialize graphics (should be done before gfxSet3D)
void gfxInitDefault(void);
void gfxInit(GSPGPU_FramebufferFormat topFormat,
             GSPGPU_FramebufferFormat bottomFormat,
             bool vrambuffers);
```

**Key Notes:**
- 3D stereoscopic mode is **mutually exclusive with wide mode** (800px tall)
- Only affects the TOP screen
- Bottom screen cannot be used for stereoscopic rendering (fixed single framebuffer)
- Must enable BEFORE creating render targets

#### 3.2 Render Target Management

**Location**: `/opt/devkitpro/libctru/include/c3d/renderqueue.h`

```c
// Create a render target for a specific screen and side
C3D_RenderTarget* C3D_RenderTargetCreate(int width, int height,
                                         GPU_COLORBUF colorFmt,
                                         C3D_DEPTHTYPE depthFmt);

// Link render target to screen output (for stereo: create separate targets)
void C3D_RenderTargetSetOutput(C3D_RenderTarget* target,
                               gfxScreen_t screen,    // GFX_TOP or GFX_BOTTOM
                               gfx3dSide_t side,      // GFX_LEFT or GFX_RIGHT
                               u32 transferFlags);

// Clean up
void C3D_RenderTargetDelete(C3D_RenderTarget* target);
```

#### 3.3 Stereo Framebuffer Sides

**Location**: `/opt/devkitpro/libctru/include/3ds/gfx.h`

```c
typedef enum {
    GFX_LEFT  = 0,  // Left eye framebuffer
    GFX_RIGHT = 1,  // Right eye framebuffer
} gfx3dSide_t;
```

**Citro2D Helper**:
```c
// Convenience function to create a screen render target
C3D_RenderTarget* C2D_CreateScreenTarget(gfxScreen_t screen,
                                         gfx3dSide_t side);
```

#### 3.4 Render Target Structure

```c
typedef struct C3D_RenderTarget_tag {
    C3D_RenderTarget *next, *prev;
    C3D_FrameBuf frameBuf;
    
    bool used;
    bool ownsColor, ownsDepth;
    
    bool linked;
    gfxScreen_t screen;     // Which screen (TOP/BOTTOM)
    gfx3dSide_t side;       // Which eye (LEFT/RIGHT)
    u32 transferFlags;      // Transfer configuration
} C3D_RenderTarget;
```

---

### 4. DEPTH BUFFER FORMATS

**Location**: `/opt/devkitpro/libctru/include/3ds/gpu/enums.h`

```c
typedef enum {
    GPU_RB_DEPTH16          = 0,  // 16-bit Depth (lighter, faster)
    GPU_RB_DEPTH24          = 2,  // 24-bit Depth (standard)
    GPU_RB_DEPTH24_STENCIL8 = 3,  // 24-bit Depth + 8-bit Stencil (for advanced effects)
} GPU_DEPTHBUF;
```

**Stereo Recommendations:**
- **Default**: `GPU_RB_DEPTH24_STENCIL8` for full precision Z-buffering
- **Performance**: Use `GPU_RB_DEPTH16` if performance is critical
- **Testing**: `GPU_RB_DEPTH24` for balance

**Typical Stereo Render Target Creation**:
```c
C3D_RenderTarget* targetLeft = C3D_RenderTargetCreate(
    240, 400,                           // Width x Height (top screen)
    GPU_RB_RGBA8,                       // Color format
    GPU_RB_DEPTH24_STENCIL8             // Depth format
);
```

---

### 5. FRAME SYNCHRONIZATION & PERFORMANCE APIs

**Location**: `/opt/devkitpro/libctru/include/c3d/renderqueue.h`

#### 5.1 Frame Control

```c
// Set target frame rate (returns actual achievable rate)
float C3D_FrameRate(float fps);

// Wait for GPU to finish (synchronize)
void C3D_FrameSync(void);

// Begin frame rendering with optional sync
bool C3D_FrameBegin(u8 flags);
    // flags: C3D_FRAME_SYNCDRAW (sync before checking GPU status)
    //        C3D_FRAME_NONBLOCK (return false if GPU busy instead of waiting)

// Switch drawing context between render targets
bool C3D_FrameDrawOn(C3D_RenderTarget* target);

// Split frame rendering (advanced)
void C3D_FrameSplit(u8 flags);

// End frame and queue for display
void C3D_FrameEnd(u8 flags);
```

#### 5.2 Performance Monitoring

```c
// Get timing information
float C3D_GetDrawingTime(void);     // Time spent in GPU drawing commands
float C3D_GetProcessingTime(void);  // Time spent processing (CPU side)

// Get frame counter
u32 C3D_FrameCounter(int id);       // Count frames since start
```

#### 5.3 Stereo Rendering Frame Pattern

```c
C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
{
    // Render LEFT eye
    C3D_RenderTargetClear(targetLeft, C3D_CLEAR_ALL, clearColor, 0);
    C3D_FrameDrawOn(targetLeft);
    sceneRender(-iod);  // Negative IOD offset for left eye
    
    // Render RIGHT eye (if 3D intensity > 0)
    if (iod > 0.0f) {
        C3D_RenderTargetClear(targetRight, C3D_CLEAR_ALL, clearColor, 0);
        C3D_FrameDrawOn(targetRight);
        sceneRender(iod);   // Positive IOD offset for right eye
    } else {
        // When slider is at 0, just show left eye on both
        // (or duplicate left to right)
    }
}
C3D_FrameEnd(0);
```

---

### 6. CITRO2D STEREO RENDERING (SIMPLIFIED)

**Location**: `/opt/devkitpro/libctru/include/c2d/base.h`

#### 6.1 Simple 2D Stereo Setup

```c
// Initialize citro2d
C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
C2D_Prepare();

// Enable 3D mode
gfxSet3D(true);

// Create targets for each eye
C3D_RenderTarget* left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
C3D_RenderTarget* right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);

// Main loop
while (aptMainLoop()) {
    float slider = osGet3DSliderState();
    
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    
    // Render left eye
    C2D_TargetClear(left, C2D_Color32(255, 255, 255, 255));
    C2D_SceneBegin(left);
    C2D_DrawImageAt(image, 100 + offsetUpper * slider, 30, 0);
    
    // Render right eye
    C2D_TargetClear(right, C2D_Color32(255, 255, 255, 255));
    C2D_SceneBegin(right);
    C2D_DrawImageAt(image, 100 - offsetUpper * slider, 30, 0);
    
    C3D_FrameEnd(0);
}
```

#### 6.2 Citro2D Drawing Functions

```c
// Clear a render target
void C2D_TargetClear(C3D_RenderTarget* target, u32 color);

// Begin drawing to a target
void C2D_SceneBegin(C3D_RenderTarget* target);

// Configure scene size (handles tilting automatically)
void C2D_SceneSize(u32 width, u32 height, bool tilt);

// Drawing primitives
bool C2D_DrawImageAt(C2D_Image img, float x, float y, float depth, ...);
bool C2D_DrawRectSolid(float x, float y, float z, float w, float h, u32 clr);
bool C2D_DrawTriangle(...);
bool C2D_DrawLine(...);
```

---

### 7. REAL-WORLD EXAMPLE: LENNY (CITRO3D STEREO)

**File**: `<PROJECT_ROOT>/repos/devkitpro-3ds-examples/graphics/gpu/lenny/source/main.c`

```c
int main() {
    // Initialize
    gfxInitDefault();
    gfxSet3D(true);  // Enable stereoscopic 3D
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    
    // Create separate render targets for left and right eyes
    C3D_RenderTarget* targetLeft = C3D_RenderTargetCreate(
        240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTarget* targetRight = C3D_RenderTargetCreate(
        240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    
    // Bind to screen (LEFT and RIGHT framebuffers)
    C3D_RenderTargetSetOutput(targetLeft, GFX_TOP, GFX_LEFT, 
                              DISPLAY_TRANSFER_FLAGS);
    C3D_RenderTargetSetOutput(targetRight, GFX_TOP, GFX_RIGHT,
                              DISPLAY_TRANSFER_FLAGS);
    
    sceneInit();
    
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
        
        // KEY: Read 3D slider and convert to IOD (interocular distance)
        float slider = osGet3DSliderState();
        float iod = slider / 3;
        
        // Rotate model
        angleY += 1.0f/256;
        
        // RENDER FRAME
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        {
            // LEFT EYE - pass NEGATIVE iod
            C3D_RenderTargetClear(targetLeft, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
            C3D_FrameDrawOn(targetLeft);
            sceneRender(-iod);
            
            // RIGHT EYE - pass POSITIVE iod (only if slider > 0)
            if (iod > 0.0f) {
                C3D_RenderTargetClear(targetRight, C3D_CLEAR_ALL, 
                                      CLEAR_COLOR, 0);
                C3D_FrameDrawOn(targetRight);
                sceneRender(iod);
            }
        }
        C3D_FrameEnd(0);
    }
    
    C3D_Fini();
    gfxExit();
}

// Scene rendering function - uses stereo projection
static void sceneRender(float iod) {
    // Create stereo projection matrix with Tilt for 3DS screen orientation
    Mtx_PerspStereoTilt(&projection,
        C3D_AngleFromDegrees(40.0f),  // FOV
        C3D_AspectRatioTop,            // 400/240 aspect ratio
        0.01f, 1000.0f,                // near/far planes
        iod,                           // ← IOD causes camera shift!
        2.0f,                          // screen focal length
        false);                        // right-handed
    
    // Create model-view matrix and update uniforms
    C3D_Mtx modelView;
    Mtx_Identity(&modelView);
    Mtx_Translate(&modelView, 0, 0, -3.0f, true);
    Mtx_RotateY(&modelView, C3D_Angle(angleY), true);
    Mtx_Scale(&modelView, 2.0f, 2.0f, 2.0f);
    
    // Upload to GPU
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);
    
    // Draw geometry
    C3D_DrawArrays(GPU_TRIANGLES, 0, vertex_list_count);
}
```

---

### 8. REAL-WORLD EXAMPLE: STEREOSCOPIC_2D (CITRO2D STEREO)

**File**: `<PROJECT_ROOT>/repos/devkitpro-3ds-examples/graphics/gpu/stereoscopic_2d/source/main.cpp`

```cpp
int main() {
    romfsInit();
    gfxInitDefault();
    gfxSet3D(true);  // Enable stereoscopic 3D
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    consoleInit(GFX_BOTTOM, NULL);
    
    // Create 2D render targets
    C3D_RenderTarget* left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    
    // Load sprite
    C2D_SpriteSheet sheet = C2D_SpriteSheetLoad("romfs:/gfx/lenny.t3x");
    C2D_Image face = C2D_SpriteSheetGetImage(sheet, 0);
    
    int offsetUpper = 0;
    int offsetLower = 0;
    
    while (aptMainLoop()) {
        hidScanInput();
        int keysD = hidKeysDown();
        int keysH = hidKeysHeld();
        
        if (keysD & KEY_START) break;
        
        // User input adjusts stereo offset
        if (keysD & KEY_DRIGHT || keysH & KEY_DUP) offsetUpper++;
        if (keysD & KEY_DLEFT || keysH & KEY_DDOWN) offsetUpper--;
        
        // Get 3D slider position
        float slider = osGet3DSliderState();
        
        // RENDER
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        
        // LEFT eye - shift right
        {
            C2D_TargetClear(left, C2D_Color32(255, 255, 255, 255));
            C2D_SceneBegin(left);
            C2D_DrawImageAt(face, 100 + offsetUpper * slider, 30, 0);
            C2D_DrawImageAt(face, 100 + offsetLower * slider, 140, 0);
        }
        
        // RIGHT eye - shift left
        {
            C2D_TargetClear(right, C2D_Color32(255, 255, 255, 255));
            C2D_SceneBegin(right);
            C2D_DrawImageAt(face, 100 - offsetUpper * slider, 30, 0);
            C2D_DrawImageAt(face, 100 - offsetLower * slider, 140, 0);
        }
        
        C3D_FrameEnd(0);
    }
    
    C2D_SpriteSheetFree(sheet);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
}
```

---

### 9. PARALLAX BARRIER & LCD CONTROL

**Location**: `/opt/devkitpro/libctru/include/3ds/services/gsplcd.h`

```c
// Power control (basic)
Result GSPLCD_PowerOnBacklight(u32 screen);
Result GSPLCD_PowerOffBacklight(u32 screen);
Result GSPLCD_PowerOnAllBacklights(void);
Result GSPLCD_PowerOffAllBacklights(void);

// 3D LED control (hardware indicator)
Result GSPLCD_SetLedForceOff(bool disable);

// Brightness control
Result GSPLCD_GetBrightness(u32 screen, u32* brightness);
Result GSPLCD_SetBrightness(u32 screen, u32 brightness);
Result GSPLCD_SetBrightnessRaw(u32 screen, u32 brightness);
```

**Important Note**: 
- The parallax barrier itself cannot be controlled directly from user code
- Barrier is automatically enabled/disabled based on `gfxSet3D()` state
- The 3D LED is just a visual indicator of 3D mode
- Eye tracking (QTM service) can be used with `osGet3DSliderState()` for advanced head-tracking

---

### 10. LOW-LEVEL GPU FRAMEBUFFER API

**Location**: `/opt/devkitpro/libctru/include/3ds/services/gspgpu.h`

```c
// Presents a buffer to the screen (used internally by gfx.h)
bool gspPresentBuffer(unsigned screen, unsigned swap,
                      const void* fb_a,  // Left eye or main framebuffer
                      const void* fb_b,  // Right eye framebuffer (or duplicate fb_a)
                      u32 stride,        // Framebuffer width in bytes
                      u32 mode);         // LCD mode register value

// Check if presentation is pending
bool gspIsPresentPending(unsigned screen);

// Get framebuffer info
typedef struct {
    u32 active_framebuf;      // 0 or 1
    u32 *framebuf0_vaddr;     // Left eye (stereo) / main (mono)
    u32 *framebuf1_vaddr;     // Right eye (stereo)
    u32 framebuf_widthbytesize;
    u32 format;
    u32 framebuf_dispselect;
    u32 unk;
} GSPGPU_FramebufferInfo;
```

---

### 11. COLOR BUFFER FORMATS

**Location**: `/opt/devkitpro/libctru/include/3ds/gpu/enums.h`

```c
typedef enum {
    GPU_RB_RGBA8    = 0,  // 8-8-8-8 (32-bit, fullcolor)
    GPU_RB_RGB8     = 1,  // 8-8-8 (24-bit, no alpha)
    GPU_RB_RGBA5551 = 2,  // 5-5-5-1 (16-bit)
    GPU_RB_RGB565   = 3,  // 5-6-5 (16-bit, common)
    GPU_RB_RGBA4    = 4,  // 4-4-4-4 (16-bit, low quality)
} GPU_COLORBUF;
```

**Stereo Recommendation**: `GPU_RB_RGBA8` for best quality (both eyes get full color fidelity)

---

### 12. COMPARISON: CITRO2D vs CITRO3D FOR STEREO

| Feature | Citro2D | Citro3D |
|---------|---------|---------|
| **Ease of Use** | Very simple, just shift positions | More complex, requires matrices |
| **Stereo Projection** | Manual X-offset per eye | `Mtx_PerspStereoTilt()` handles parallax |
| **Best For** | 2D overlays, simple stereoscopic effects | Full 3D geometry, complex scenes |
| **Performance** | Fast, lightweight | More overhead but GPU-accelerated |
| **Depth Perception** | Based on X-axis parallax only | Full 3D with proper depth cues |
| **Setup Code** | <50 lines | 100-200 lines |
| **Target Example** | `C2D_CreateScreenTarget()` | `C3D_RenderTargetCreate()` |
| **Per-Eye Rendering** | Draw same scene, different X offsets | Render with different projection matrices |
| **3D Slider Integration** | `offset * osGet3DSliderState()` | `iod = slider / 3` → `Mtx_PerspStereo(iod)` |

---

### 13. BEST PRACTICES FOR STEREO RENDERING

#### 13.1 Initialization Checklist
```c
1. gfxInitDefault();                    // Initialize framebuffers
2. gfxSet3D(true);                      // Enable stereoscopic mode BEFORE creating targets
3. C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);  // Initialize GPU (citro3d)
4. // Create render targets AFTER gfxSet3D
5. C3D_RenderTarget* targetLeft = C3D_RenderTargetCreate(...);
6. C3D_RenderTarget* targetRight = C3D_RenderTargetCreate(...);
7. C3D_RenderTargetSetOutput(targetLeft, GFX_TOP, GFX_LEFT, ...);
8. C3D_RenderTargetSetOutput(targetRight, GFX_TOP, GFX_RIGHT, ...);
```

#### 13.2 Per-Frame Rendering Pattern
```c
float slider = osGet3DSliderState();  // Poll every frame
float iod = slider / 3.0f;             // Scale factor (experiment!)

C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
{
    // LEFT eye
    C3D_RenderTargetClear(targetLeft, C3D_CLEAR_ALL, color, 0);
    C3D_FrameDrawOn(targetLeft);
    renderWithProjection(-iod);
    
    // RIGHT eye (only if iod > 0 to avoid redundant rendering)
    if (iod > 0.0f) {
        C3D_RenderTargetClear(targetRight, C3D_CLEAR_ALL, color, 0);
        C3D_FrameDrawOn(targetRight);
        renderWithProjection(iod);
    }
}
C3D_FrameEnd(0);
```

#### 13.3 Projection Matrix Setup
```c
static void updateProjection(float iod) {
    // Always use the "Tilt" version for top screen!
    Mtx_PerspStereoTilt(
        &projMatrix,
        C3D_AngleFromDegrees(40.0f),    // Adjust FOV to taste
        C3D_AspectRatioTop,             // 400/240 = 1.667
        0.01f, 1000.0f,                 // Near/far - adjust for your scene
        iod,                            // ← Key parameter that changes per-eye
        2.0f,                           // Screen focal distance (experiment!)
        false                           // Right-handed coordinate system
    );
}
```

#### 13.4 Optimization Tips
- **Frame Rate**: Use `C3D_FrameRate(30.0f)` for 30 FPS stereo to reduce load
- **Depth Buffer**: `GPU_RB_DEPTH24` sufficient; `GPU_RB_DEPTH16` for low-power devices
- **Conditional Rendering**: Skip right-eye rendering when slider = 0
- **Viewport**: Both eyes are 240x400 on top screen (portrait)
- **Memory**: Each eye needs separate framebuffer (0.9 MB for RGBA8, less for RGB565)

#### 13.5 Common Mistakes to Avoid
1. **Forgetting `gfxSet3D(true)`** - Mode won't activate
2. **Using non-Tilt projection** - Stereo will be misaligned on screen
3. **Not polling 3D slider** - Static stereo effect won't follow user adjustment
4. **Creating targets BEFORE `gfxSet3D()`** - May use wrong framebuffer layout
5. **Using `GFX_LEFT` for both eyes** - Right eye won't render
6. **IOD too large** - Causes eye strain; typically scale by slider/3
7. **Forgetting to clear depth buffer** - Z-fighting between frames

---

### 14. ADDITIONAL STEREO EXAMPLES IN DEVKITPRO

The following examples use stereo rendering (GFX_LEFT/GFX_RIGHT):
- `lenny` - Full 3D stereo with lighting
- `particles` - Particle system with stereo
- `normal_mapping` - Normal-mapped stereo geometry
- `fragment_light` - Fragment lighting stereo
- `composite_scene` - Complex scene stereo
- `stereoscopic_2d` - 2D sprite stereo (citro2d)
- `toon_shading` - Cartoon shading stereo

---

### 15. HARDWARE CONSTRAINTS & LIMITATIONS

**Top Screen Specifications**:
- Resolution: 240 pixels wide × 400 pixels tall (portrait orientation)
- Physical Orientation: 90° CCW rotation (hence "Tilt" functions)
- Stereo Method: Parallax barrier (active when gfxSet3D enabled)
- Each Eye: 240×400 framebuffer
- Refresh Rate: 60 Hz (each eye updated at 60 Hz)
- Aspect Ratio: 400/240 = 1.667

**Bottom Screen Specifications**:
- Resolution: 240 pixels wide × 320 pixels tall
- Stereo: **NOT SUPPORTED** (single framebuffer only)
- Use for UI/debug text while rendering stereo on top

**Memory Per Frame**:
- RGBA8: 240×400×4 = 384 KB per eye = 768 KB total
- RGB8: 240×400×3 = 288 KB per eye = 576 KB total
- RGB565: 240×400×2 = 192 KB per eye = 384 KB total
- Depth24_Stencil8: 240×400×4 = 384 KB

**GPU Constraints**:
- Command buffer: 256 KB default (adjust with `C3D_DEFAULT_CMDBUF_SIZE`)
- Rendering two full scenes doubles fill-rate demand
- Consider 30 FPS for complex scenes

---

### 16. ADVANCED TOPICS

#### 16.1 Eye Tracking Integration (QTM Service)
```c
// In addition to slider, can read actual eye position
// See: /opt/devkitpro/libctru/include/3ds/services/qtm.h
// This enables head-tracking for more immersive stereo
```

#### 16.2 Capture to Stereo Image
```c
// GPU can capture both eyes to external memory
// Useful for screenshots or recording stereo video
// Located in gspgpu.h: GSPGPU_CaptureInfo structures
```

#### 16.3 Screen Transfer Flags
```c
#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | \
     GX_TRANSFER_OUT_TILED(0) | \
     GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
     GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
// These control color format conversion between GPU and LCD
```

---

## Summary Tables

### Stereo API Quick Reference

| Task | API | Function |
|------|-----|----------|
| Enable 3D Mode | gfx.h | `gfxSet3D(true)` |
| Get 3D Slider | os.h | `osGet3DSliderState()` |
| Create Targets | renderqueue.h | `C3D_RenderTargetCreate()` |
| Set Eye Output | renderqueue.h | `C3D_RenderTargetSetOutput(..., GFX_LEFT/RIGHT)` |
| Stereo Projection | maths.h | `Mtx_PerspStereoTilt(iod, screen)` |
| Frame Sync | renderqueue.h | `C3D_FrameBegin/End()` |
| Switch Eye | renderqueue.h | `C3D_FrameDrawOn(target)` |

### Typical IOD (Interocular Distance) Values

| 3D Slider Position | IOD Value | Effect |
|-------------------|-----------|--------|
| 0.0 (off) | 0.0 | No 3D effect, render once |
| 0.5 (medium) | 0.167 | Moderate depth |
| 1.0 (maximum) | 0.333 | Maximum 3D intensity |

*(iod = slider / 3.0 is typical; adjust divisor experimentally)*

