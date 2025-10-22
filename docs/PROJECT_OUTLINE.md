# SNES 3D Layer Separation Project
## Stereoscopic 3D for SNES Games on New 3DS XL

### Project Overview
This project aims to implement M2-style stereoscopic 3D layer separation for SNES games running on the New Nintendo 3DS XL. By extracting and rendering the SNES's native background and sprite layers at different depths, we'll create a convincing 3D effect that enhances classic games without modifying their core logic.

---

## Project Goals

### Primary Objectives
- [ ] Implement stereoscopic 3D rendering for SNES games on New 3DS XL
- [ ] Separate SNES PPU layers (BG1-4, sprites) and render at configurable depths
- [ ] Achieve 60 FPS performance on New 3DS XL
- [ ] Support 3D slider for dynamic depth adjustment
- [ ] Maintain pixel-perfect accuracy when 3D is disabled

### Secondary Objectives
- [ ] Create per-game depth configuration system
- [ ] Implement advanced features (per-scanline depth, sprite depth separation)
- [ ] Add savestates and fast-forward features to emulator
- [ ] Support special SNES modes (Mode 7, transparency effects)
- [ ] Build user-friendly depth adjustment UI

---

## Technical Approach

### Architecture: Standalone SNES9x Fork

**Why Standalone vs RetroArch:**
- Direct hardware access for maximum performance
- SNES-specific PPU optimizations
- Easier per-game configuration
- Full control over New 3DS XL features (804 MHz CPU, L2 cache)
- Simpler debugging and iteration

### Base Emulator Selection

**Candidates:**
1. **bubble2k16/snes9x_3ds** - Original major fork, GPU-accelerated
2. **matbo87/snes9x_3ds** - Updated fork with newer devkitARM/libctru
3. **Universal-DB version** - Latest on Universal-Team updater

**Decision Criteria:**
- GPU rendering already implemented? (essential)
- Active maintenance and modern toolchain
- Performance characteristics
- Code quality and documentation
- Missing features (savestates, fast-forward)

---

## System Architecture

### Layer Extraction Module
```
SNES PPU Emulation
       ↓
[Layer Separator]
   • BG1 → Buffer
   • BG2 → Buffer
   • BG3 → Buffer
   • BG4 → Buffer
   • Sprites → Buffer
   • Windows/UI → Buffer
       ↓
[Layer Metadata]
   • Depth values
   • Scroll offsets
   • Transparency flags
   • Priority order
```

### Stereoscopic Rendering Engine
```
[Frame Layers] → [Depth Config] → [Citro3D Renderer]
                                         ↓
                              [Left Eye View] [Right Eye View]
                                         ↓
                              [Parallax Calculation]
                                   • depth * slider
                                         ↓
                              [GPU Texture Quads]
                                   • Back to front
                                   • Alpha blending
                                         ↓
                              [3DS Stereoscopic Display]
```

---

## Implementation Phases

### Phase 1: Foundation & Setup (Weeks 1-2)

#### 1.1 Development Environment
- [x] Install devkitPro toolchain (devkitARM r64+)
- [x] Install libctru (latest)
- [x] Install Citro3D
- [ ] Set up build system
- [ ] Configure New 3DS XL test environment

#### 1.2 Codebase Analysis
- [ ] Clone all SNES9x 3DS fork candidates
- [ ] Build and test each on New 3DS XL
- [ ] Document GPU rendering implementation
- [ ] Map PPU rendering pipeline
- [ ] Identify layer compositing code
- [ ] Analyze performance characteristics

#### 1.3 Stereoscopic Prototype
- [ ] Create minimal Citro3D stereo test app
- [ ] Implement dual framebuffer rendering
- [ ] Test 3D slider integration
- [ ] Verify left/right eye separation
- [ ] Measure baseline performance

**Deliverables:**
- Working dev environment
- Base emulator fork selected
- Simple stereo rendering demo
- Performance baseline metrics

---

### Phase 2: Layer Separation System (Weeks 3-4)

#### 2.1 Data Structures

**Core Types:**
```c
typedef struct {
    u8* pixelData;          // RGBA8 or indexed color
    C3D_Tex* gpuTexture;    // GPU texture handle
    u16 width, height;
    s16 scrollX, scrollY;
    float depth;            // 0.0 = screen plane, + = into screen
    bool transparent;
    u8 priority;
    bool enabled;
    bool dirty;             // Needs GPU texture update
} SnesLayer;

typedef struct {
    SnesLayer bg[4];        // BG1, BG2, BG3, BG4
    SnesLayer sprites;      // All sprites composited
    SnesLayer window;       // Window/UI overlay
    u8 bgMode;              // SNES background mode (0-7)
    u8 activeLayerCount;
    float globalDepthScale; // From 3D slider
} SnesFrameLayers;
```

**Configuration Types:**
```c
typedef struct {
    char gameName[64];
    u32 crc32;
    float bgDepth[4];
    float spriteDepth;
    float windowDepth;
    bool enablePerSpriteDepth;
    bool enableScanlineDepth;
} GameDepthProfile;

typedef struct {
    GameDepthProfile* profiles;
    u32 profileCount;
    GameDepthProfile* currentProfile;
    float defaultDepths[7]; // Mode 0-6 defaults
} DepthConfiguration;
```

#### 2.2 PPU Modification Strategy

**Current Flow (bubble2k16):**
```
PPU Render → Software composite → GPU upload → Display
```

**New Flow:**
```
PPU Render → Layer extraction → GPU per-layer → Stereo composite → Display
```

**Code Locations to Modify:**
- `source/snes9x/ppu.cpp` - Main PPU rendering
- `source/snes9x/tile.cpp` - Tile rendering
- `source/snes9x/gfx.cpp` - Graphics compositing
- `source/3dsgpu.cpp` - GPU interface (likely name)

**Extraction Points:**
```c
// In PPU render loop
void S9xUpdateScreen() {
    // MODIFY: Instead of composite, extract layers

    // Old: DrawBackground(BG1) → mainScreen
    // New: DrawBackground(BG1) → frameLayers.bg[0].pixelData

    ExtractLayer_BG1(&frameLayers.bg[0]);
    ExtractLayer_BG2(&frameLayers.bg[1]);
    ExtractLayer_BG3(&frameLayers.bg[2]);
    ExtractLayer_BG4(&frameLayers.bg[3]);
    ExtractLayer_Sprites(&frameLayers.sprites);

    // Pass to stereo renderer
    RenderStereoscopicFrame(&frameLayers);
}
```

#### 2.3 Special Cases Handling

**Mode 7 (3D perspective mode):**
- Render as single layer with depth gradient
- Optional: per-scanline depth for road recession effect
- Games: F-Zero, Super Mario Kart, Pilotwings

**Transparency/Color Math:**
- Use GPU alpha blending instead of software
- Map SNES blend modes to OpenGL blend equations
- Preserve half-transparency effects

**Window Effects:**
- Render window masks as separate pass
- Apply as stencil or clip rectangle

**Hi-Res Mode (512px):**
- Handle doubled horizontal resolution
- May need separate texture format

**Interlace Mode:**
- Rare in games, handle as special case

**Deliverables:**
- Layer extraction code integrated
- All BG modes supported (0-7)
- Special effects preserved
- Layer buffers output correctly

---

### Phase 3: Citro3D Stereo Rendering Engine (Weeks 5-6)

#### 3.1 Core Rendering Loop

```c
// Global state
C3D_RenderTarget* leftTarget;
C3D_RenderTarget* rightTarget;
C3D_Mtx projection;

void InitStereoRenderer() {
    // Initialize Citro3D
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    // Enable stereoscopic mode
    gfxSet3D(true);

    // Create render targets
    leftTarget = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    rightTarget = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);

    C3D_RenderTargetSetOutput(leftTarget, GFX_TOP, GFX_LEFT,
                              DISPLAY_TRANSFER_FLAGS);
    C3D_RenderTargetSetOutput(rightTarget, GFX_TOP, GFX_RIGHT,
                              DISPLAY_TRANSFER_FLAGS);

    // Set up orthographic projection
    Mtx_OrthoTilt(&projection, 0.0, 400.0, 0.0, 240.0, 0.0, 100.0, true);
}

void RenderStereoscopicFrame(SnesFrameLayers* layers) {
    // Read 3D slider (0.0 to 1.0)
    float sliderValue = osGet3DSliderState();
    layers->globalDepthScale = sliderValue;

    // Begin frame
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    // Render left eye
    C3D_RenderTargetClear(leftTarget, C3D_CLEAR_ALL, 0x000000FF, 0);
    C3D_FrameDrawOn(leftTarget);
    RenderLayersForEye(layers, -1.0f); // Left = negative offset

    // Render right eye
    C3D_RenderTargetClear(rightTarget, C3D_CLEAR_ALL, 0x000000FF, 0);
    C3D_FrameDrawOn(rightTarget);
    RenderLayersForEye(layers, +1.0f); // Right = positive offset

    // End frame
    C3D_FrameEnd(0);
}

void RenderLayersForEye(SnesFrameLayers* layers, float eyeSign) {
    // Sort layers by depth (back to front)
    SnesLayer* sortedLayers[6];
    SortLayersByDepth(layers, sortedLayers);

    // Render each layer
    for (int i = 0; i < layers->activeLayerCount; i++) {
        SnesLayer* layer = sortedLayers[i];
        if (!layer->enabled) continue;

        // Calculate parallax offset
        float parallax = layer->depth * layers->globalDepthScale * eyeSign;

        // Update GPU texture if dirty
        if (layer->dirty) {
            UpdateLayerTexture(layer);
            layer->dirty = false;
        }

        // Render textured quad
        RenderLayerQuad(layer, parallax);
    }
}
```

#### 3.2 Parallax Calculation

**Depth Mapping:**
```
Depth Value    Meaning              Parallax at Max Slider
-----------    -------              ----------------------
-2.0           Pop-out extreme      -16 pixels
-1.0           Pop-out moderate     -8 pixels
 0.0           Screen plane         0 pixels (UI layer)
 1.0           Slight depth         +4 pixels
 2.0           Moderate depth       +8 pixels
 4.0           Far depth            +16 pixels
 8.0           Very far             +32 pixels
```

**Formula:**
```c
float parallaxPixels = depthValue * sliderValue * PARALLAX_SCALE;
// PARALLAX_SCALE = 4.0 (tunable constant)
```

**Eye Offset:**
```c
// Left eye: shift layers RIGHT (positive X) for far layers
// Right eye: shift layers LEFT (negative X) for far layers
// This creates the illusion of depth INTO screen

float xOffset = baseX + (parallaxPixels * eyeSign);
```

#### 3.3 GPU Texture Management

```c
void UpdateLayerTexture(SnesLayer* layer) {
    if (layer->gpuTexture == NULL) {
        // Allocate new texture
        layer->gpuTexture = malloc(sizeof(C3D_Tex));
        C3D_TexInit(layer->gpuTexture, layer->width, layer->height, GPU_RGBA8);
    }

    // Upload pixel data to VRAM
    GSPGPU_FlushDataCache(layer->pixelData, layer->width * layer->height * 4);
    C3D_SyncDisplayTransfer(
        (u32*)layer->pixelData, GX_BUFFER_DIM(layer->width, layer->height),
        (u32*)layer->gpuTexture->data, GX_BUFFER_DIM(layer->width, layer->height),
        TEXTURE_TRANSFER_FLAGS
    );
}

void RenderLayerQuad(SnesLayer* layer, float parallaxOffset) {
    // Bind texture
    C3D_TexBind(0, layer->gpuTexture);

    // Calculate position (centered, SNES is 256x224 typically)
    float x = (400 - layer->width) / 2.0f + parallaxOffset;
    float y = (240 - layer->height) / 2.0f;
    float z = layer->depth; // Z-depth for proper sorting

    // Set up vertices (textured quad)
    // ... vertex buffer setup ...

    // Draw
    C3D_DrawArrays(GPU_TRIANGLES, 0, 6); // Two triangles = quad
}
```

#### 3.4 Performance Optimizations

**Texture Caching:**
- Only update textures when SNES VRAM changes
- Use dirty flags per tile/sprite
- Batch updates within vblank

**Vertex Batching:**
- Render all tiles of one layer in single draw call
- Use instancing if available

**New 3DS Boost:**
```c
void EnableNew3DSPerformance() {
    // Enable 804 MHz CPU
    osSetSpeedupEnable(true);

    // Enable L2 cache
    // (Handled by OS when speedup enabled)

    // Set thread priority
    svcSetThreadPriority(CUR_THREAD_HANDLE, 0x18);
}
```

**GPU State Management:**
- Minimize state changes between layers
- Group layers with same blend mode
- Reuse shaders and buffers

**Deliverables:**
- Working stereo renderer
- 60 FPS on New 3DS XL
- Proper depth parallax
- Texture management system

---

### Phase 4: Depth Configuration System (Week 7)

#### 4.1 Default Depth Profiles

```c
// depth_profiles.c

const float DEFAULT_DEPTHS_MODE[8][4] = {
    // Mode 0: 4 color layers (BG1, BG2, BG3, BG4)
    {1.0f, 2.0f, 3.0f, 4.0f},

    // Mode 1: 3 layers, BG1&2 = 16 color, BG3 = 4 color
    {1.0f, 2.0f, 4.0f, 0.0f},

    // Mode 2: Offset-per-tile mode
    {1.0f, 3.0f, 0.0f, 0.0f},

    // Mode 3: 256 color + 16 color
    {1.0f, 3.0f, 0.0f, 0.0f},

    // Mode 4: Offset-per-tile + 256 color
    {1.0f, 3.0f, 0.0f, 0.0f},

    // Mode 5: Hi-res 512x224
    {1.0f, 3.0f, 0.0f, 0.0f},

    // Mode 6: Hi-res interlace
    {1.0f, 3.0f, 0.0f, 0.0f},

    // Mode 7: Rotation/scaling
    {2.0f, 0.0f, 0.0f, 0.0f}  // Special handling
};

const float DEFAULT_SPRITE_DEPTH = 0.5f;  // In front of most BGs
const float DEFAULT_WINDOW_DEPTH = 0.0f;  // At screen plane
```

#### 4.2 Game-Specific Profiles

**JSON Configuration File:**
```json
{
  "format_version": "1.0",
  "games": [
    {
      "name": "Super Mario World",
      "crc32": "0xA31BEAD4",
      "depths": {
        "bg1": 0.8,
        "bg2": 2.0,
        "bg3": 4.0,
        "bg4": 0.0,
        "sprites": 1.5,
        "window": 0.0
      },
      "notes": "BG1 has status bar, keep near screen"
    },
    {
      "name": "Chrono Trigger",
      "crc32": "0x2D206BF7",
      "depths": {
        "bg1": 1.5,
        "bg2": 3.0,
        "bg3": 5.0,
        "bg4": 0.0,
        "sprites": 1.0,
        "window": 0.0
      },
      "notes": "Beautiful parallax backgrounds"
    },
    {
      "name": "F-Zero",
      "crc32": "0x7A717F97",
      "depths": {
        "bg1": 1.0,
        "bg2": 0.0,
        "bg3": 0.0,
        "bg4": 0.0,
        "sprites": 0.5,
        "window": 0.0
      },
      "mode7_gradient": true,
      "mode7_depth_near": 1.0,
      "mode7_depth_far": 8.0,
      "notes": "Mode 7 racing, use scanline depth gradient"
    },
    {
      "name": "Super Metroid",
      "crc32": "0x12B77885",
      "depths": {
        "bg1": 1.0,
        "bg2": 2.5,
        "bg3": 5.0,
        "bg4": 0.0,
        "sprites": 1.2,
        "window": 0.0
      },
      "notes": "Atmospheric layering, subtle effect works best"
    },
    {
      "name": "Yoshi's Island",
      "crc32": "0x2A9F390E",
      "depths": {
        "bg1": 1.0,
        "bg2": 2.0,
        "bg3": 4.0,
        "bg4": 0.0,
        "sprites": 1.5,
        "window": 0.0
      },
      "sprite_depth_separation": true,
      "notes": "Complex sprite effects, may need special handling"
    }
  ]
}
```

**Loading System:**
```c
#include "json.h" // Use jsmn or similar lightweight parser

DepthConfiguration* LoadDepthConfigs(const char* configPath) {
    // Load JSON file
    // Parse game profiles
    // Store in hash table by CRC32
    // Return configuration struct
}

GameDepthProfile* GetProfileForROM(u32 crc32) {
    // Look up in hash table
    // Return profile or NULL for default
}

void ApplyDepthProfile(SnesFrameLayers* layers, GameDepthProfile* profile) {
    if (profile == NULL) {
        // Use defaults based on BG mode
        ApplyDefaultDepths(layers);
        return;
    }

    layers->bg[0].depth = profile->bgDepth[0];
    layers->bg[1].depth = profile->bgDepth[1];
    layers->bg[2].depth = profile->bgDepth[2];
    layers->bg[3].depth = profile->bgDepth[3];
    layers->sprites.depth = profile->spriteDepth;
    layers->window.depth = profile->windowDepth;
}
```

#### 4.3 Runtime Configuration UI

**In-Emulator Menu:**
```
┌─────────────────────────────────────┐
│    3D DEPTH CONFIGURATION           │
├─────────────────────────────────────┤
│ Game: Super Mario World             │
│ Profile: Custom                     │
├─────────────────────────────────────┤
│ BG1 Depth:  [====·····] 1.0        │
│ BG2 Depth:  [========··] 2.0       │
│ BG3 Depth:  [============] 4.0     │
│ Sprite Depth: [======····] 1.5     │
├─────────────────────────────────────┤
│ [A] Adjust  [B] Back  [X] Save      │
│ [Y] Reset to Default                │
└─────────────────────────────────────┘
```

**Implementation:**
```c
typedef struct {
    float* targetDepth;
    char label[32];
    float min, max, step;
} DepthSlider;

void ShowDepthConfigMenu() {
    DepthSlider sliders[] = {
        {&currentLayers.bg[0].depth, "BG1 Depth", 0.0f, 8.0f, 0.1f},
        {&currentLayers.bg[1].depth, "BG2 Depth", 0.0f, 8.0f, 0.1f},
        {&currentLayers.bg[2].depth, "BG3 Depth", 0.0f, 8.0f, 0.1f},
        {&currentLayers.sprites.depth, "Sprite Depth", 0.0f, 8.0f, 0.1f},
    };

    int selectedSlider = 0;

    while (menuActive) {
        // Read input
        // Adjust selected slider
        // Render menu
        // Show live preview of changes
    }
}

void SaveCustomProfile(const char* romName, u32 crc32) {
    // Serialize current depths to JSON
    // Append to config file
    // Reload configuration
}
```

**Deliverables:**
- Default depth profiles for all BG modes
- Game-specific profile database (start with ~20 popular games)
- JSON parser and loader
- In-emulator configuration UI
- Profile save/load system

---

### Phase 5: Advanced Features (Weeks 8-9)

#### 5.1 Per-Scanline Depth

**Use Case:** Mode 7 games with perspective (racing games, SNES Pilotwings)

**Concept:**
- Top of screen (horizon) = far depth
- Bottom of screen (near road) = close depth
- Gradient between = smooth depth transition

**Implementation:**
```c
typedef struct {
    float depthGradient[224]; // One depth per scanline
    bool useGradient;
    float nearDepth;
    float farDepth;
} ScanlineDepthConfig;

void CalculateMode7DepthGradient(ScanlineDepthConfig* config) {
    // Linear gradient from top to bottom
    for (int y = 0; y < 224; y++) {
        float t = (float)y / 224.0f;
        config->depthGradient[y] = config->farDepth * (1.0f - t) +
                                   config->nearDepth * t;
    }
}

void RenderLayerQuadWithGradient(SnesLayer* layer, ScanlineDepthConfig* config) {
    // Instead of one quad, render multiple horizontal strips
    // Each strip at different depth

    for (int y = 0; y < layer->height; y += STRIP_HEIGHT) {
        float depth = config->depthGradient[y];
        float parallax = depth * globalDepthScale * eyeSign;

        // Render horizontal strip
        RenderQuadStrip(layer, y, STRIP_HEIGHT, parallax);
    }
}
```

**Games to Support:**
- F-Zero
- Super Mario Kart
- Pilotwings
- HyperZone

#### 5.2 Sprite Depth Separation

**Concept:** Give individual sprites different depths based on position or priority

**Strategies:**

**1. Y-Position Based:**
```c
// Sprites lower on screen appear closer (RPG depth illusion)
float GetSpriteDepth_YBased(int spriteY, float baseDepth) {
    float t = (float)spriteY / 224.0f;
    return baseDepth + (t * 2.0f); // Closer at bottom
}
```

**2. Priority-Based:**
```c
// SNES sprites have 4 priority levels
float GetSpriteDepth_Priority(int priority, float baseDepth) {
    float priorityOffset[] = {0.0f, 0.5f, 1.0f, 1.5f};
    return baseDepth + priorityOffset[priority];
}
```

**3. Size-Based:**
```c
// Larger sprites appear closer (common perspective trick)
float GetSpriteDepth_Size(int width, int height, float baseDepth) {
    int size = width * height;
    if (size > 2048) return baseDepth - 0.5f; // Large = closer
    if (size < 256) return baseDepth + 1.0f;  // Small = farther
    return baseDepth;
}
```

**Implementation Challenge:**
- Current approach renders all sprites as single layer
- Need to separate sprites and render individually
- Performance impact: more draw calls

**Optimization:**
```c
// Sort sprites into depth buckets
SpriteLayer spriteLayers[4]; // 4 depth levels

void SeparateSprites(SnesSprites* sprites) {
    for (int i = 0; i < sprites->count; i++) {
        int bucket = CalculateSpriteBucket(&sprites->sprite[i]);
        AddSpriteToLayer(&spriteLayers[bucket], &sprites->sprite[i]);
    }
}

// Render each bucket as one layer
void RenderSpriteDepthSeparation() {
    for (int i = 0; i < 4; i++) {
        RenderLayerQuad(&spriteLayers[i], spriteLayers[i].depth);
    }
}
```

#### 5.3 Automatic Layer Detection

**Goal:** Automatically detect which layers are UI vs gameplay

**UI Detection Heuristics:**
- Layer doesn't scroll (scrollX/Y = 0 every frame)
- High contrast text rendering
- Always enabled (never disabled)
- Typically on BG1 or sprites

**Implementation:**
```c
typedef struct {
    int framesWithoutScroll;
    bool likelyUI;
} LayerAnalysis;

void AnalyzeLayer(SnesLayer* layer, LayerAnalysis* analysis) {
    if (layer->scrollX == 0 && layer->scrollY == 0) {
        analysis->framesWithoutScroll++;
    } else {
        analysis->framesWithoutScroll = 0;
    }

    // If static for 60 frames, probably UI
    if (analysis->framesWithoutScroll > 60) {
        analysis->likelyUI = true;
        layer->depth = 0.0f; // Keep at screen plane
    }
}
```

#### 5.4 Special Game Handlers

**Examples:**

**Super Mario World - Status Bar:**
```c
void SpecialHandler_SMW(SnesFrameLayers* layers) {
    // BG1 has both status bar (top 32 pixels) and level graphics
    // Split into two sub-layers

    SplitLayerHorizontal(&layers->bg[0], 32, &statusBar, &levelGraphics);
    statusBar.depth = 0.0f;        // UI at screen
    levelGraphics.depth = 2.0f;    // Level has depth
}
```

**Chrono Trigger - Text Boxes:**
```c
void SpecialHandler_ChronoTrigger(SnesFrameLayers* layers) {
    // Detect dialog boxes (specific sprite patterns)
    if (DialogBoxActive()) {
        // Bring dialog sprites to screen plane
        SetSpriteDepthRange(DIALOG_SPRITE_START, DIALOG_SPRITE_END, 0.0f);
    }
}
```

**Deliverables:**
- Per-scanline depth for Mode 7
- Sprite depth separation system
- Automatic UI detection
- Special handlers for 5+ popular games

---

### Phase 6: Additional Emulator Features (Week 10)

#### 6.1 Savestates

**Why Missing?** Many SNES9x 3DS forks lack savestates due to complexity of serializing state.

**Implementation Plan:**
```c
typedef struct {
    // SNES state
    u8 cpuState[CPU_STATE_SIZE];
    u8 ppuState[PPU_STATE_SIZE];
    u8 ramState[128 * 1024];  // 128KB WRAM
    u8 vramState[64 * 1024];  // 64KB VRAM
    u8 saveramState[SAVERAM_SIZE];

    // Emulator state
    u32 frameCount;
    u64 timestamp;
    char romName[256];
    u32 romCRC;

    // 3D state (optional)
    SnesFrameLayers layerState;
    GameDepthProfile depthProfile;
} SaveState;

bool SaveStateToFile(const char* path) {
    SaveState state;
    SerializeSNESState(&state);

    FILE* f = fopen(path, "wb");
    fwrite(&state, sizeof(SaveState), 1, f);
    fclose(f);

    // Save screenshot for slot preview
    SaveScreenshot(path);

    return true;
}

bool LoadStateFromFile(const char* path) {
    SaveState state;

    FILE* f = fopen(path, "rb");
    fread(&state, sizeof(SaveState), 1, f);
    fclose(f);

    DeserializeSNESState(&state);

    return true;
}
```

**UI:**
```
F1 = Quick Save
F2 = Quick Load
Select + L/R = Change save slot (0-9)
```

#### 6.2 Fast Forward

**Implementation:**
```c
bool fastForwardEnabled = false;
const int FAST_FORWARD_SPEED = 2; // 2x speed

void UpdateEmulation() {
    int framesToRun = fastForwardEnabled ? FAST_FORWARD_SPEED : 1;

    for (int i = 0; i < framesToRun; i++) {
        S9xMainLoop(); // Run one SNES frame

        // Only render last frame in fast-forward
        if (i == framesToRun - 1) {
            RenderFrame();
        }
    }

    // Skip audio during fast-forward
    if (!fastForwardEnabled) {
        UpdateAudio();
    }
}
```

**Controls:**
```
Hold R shoulder = 2x fast forward
Hold R + A = 4x fast forward (New 3DS only)
```

#### 6.3 Enhanced UI

**Features:**
- ROM browser with thumbnails
- Per-game settings memory
- Cheat code support
- Screenshot capture
- FPS counter / performance overlay

**Deliverables:**
- Working savestate system (10 slots)
- Fast-forward (2x-4x)
- Enhanced menu system
- Quality of life improvements

---

## Technical Specifications

### Hardware Requirements
- **New Nintendo 3DS XL** (minimum)
  - 804 MHz quad-core ARM11 MPCore
  - 256 MB RAM (FCRAM)
  - 10 MB VRAM
  - PICA200 GPU
- **Old 3DS NOT recommended** (268 MHz too slow for stereo rendering)

### Performance Targets
- **Frame Rate:** 60 FPS locked (NTSC), 50 FPS (PAL)
- **Input Lag:** <1 frame (<16.7ms)
- **Audio Latency:** <100ms
- **Frame Pacing:** ±0.5ms variance

### Memory Budget
```
SNES RAM:          128 KB
SNES VRAM:          64 KB
SNES Save RAM:      Up to 128 KB (varies)
Emulator Code:      ~2 MB
Layer Buffers:      1.5 MB (6 layers × 256×224×4 bytes)
GPU Textures:       ~2 MB (VRAM)
Sound Buffer:       64 KB
Savestate:          ~512 KB per slot
Total:              ~6-8 MB (plenty of headroom in 256 MB)
```

### SNES PPU Capabilities
- **Background Modes:** 0-7 (varying layer counts and color depths)
- **Max Layers:** 4 backgrounds + sprites + window
- **Resolution:** 256×224 (NTSC), 256×239 (PAL), up to 512×448 (hi-res interlace)
- **Colors:** 32,768 (15-bit), up to 256 on-screen
- **Sprites:** Up to 128, max 32 per scanline
- **Tile Size:** 8×8 or 16×16 pixels

---

## Development Tools

### Required Software
- [x] **devkitPro** - Toolchain for 3DS homebrew
  - devkitARM r64 or later
  - libctru 2.3.1+
  - Citro3D 1.7.1+
- [ ] **Build Tools**
  - make / cmake
  - git
- [ ] **Development**
  - VS Code / IDE of choice
  - GDB for debugging

### Recommended Libraries
- **3DS Libraries:**
  - libctru - Core 3DS functions
  - Citro3D - GPU rendering
  - Citro2D - 2D helper (for UI)
  - sf2d / sftd - Font rendering
- **Utility:**
  - jsmn - JSON parser (for config files)
  - stb_image - Image loading (for screenshots)
  - minizip - Savestate compression

### Testing Tools
- **Citra Emulator** - Test on PC before hardware
- **3DS-GDB** - Remote debugging on hardware
- **Luma3DS** - CFW with exception handling
- **Checkpoint** - Savestate management

---

## Repository Structure

```
snes9x-3d/
├── README.md
├── PROJECT_OUTLINE.md (this file)
├── LICENSE
├── Makefile
├── source/
│   ├── snes9x/              # Base SNES9x core
│   │   ├── cpu/
│   │   ├── ppu.cpp          # [MODIFY] Layer extraction
│   │   ├── gfx.cpp          # [MODIFY] Remove compositing
│   │   └── ...
│   ├── 3ds/                 # 3DS-specific code
│   │   ├── main.cpp
│   │   ├── stereo_renderer.c    # [NEW] Stereoscopic engine
│   │   ├── layer_manager.c      # [NEW] Layer extraction
│   │   ├── depth_config.c       # [NEW] Depth configuration
│   │   ├── ui_menu.c            # [ENHANCE] Add 3D menu
│   │   ├── savestate.c          # [NEW] Savestate system
│   │   └── input.c
│   └── shaders/             # [NEW] GPU shaders
│       ├── layer_vertex.glsl
│       └── layer_fragment.glsl
├── include/
│   ├── stereo_types.h       # [NEW] Layer structs
│   ├── depth_profiles.h
│   └── ...
├── data/
│   ├── depth_profiles.json  # [NEW] Game configurations
│   ├── icon.png
│   └── shaders/             # Compiled shaders
├── docs/
│   ├── DEVELOPMENT.md
│   ├── DEPTH_TUNING.md
│   └── API.md
└── tools/
    ├── profile_generator.py  # [NEW] Helper to create profiles
    └── crc32_calculator.py
```

---

## Testing Plan

### Phase 1: Unit Tests
- Layer extraction correctness
- Depth calculation accuracy
- Texture upload/update
- 3D slider response

### Phase 2: Integration Tests
- Full frame rendering
- Mode switching (0-7)
- Special effects preservation
- Performance profiling

### Phase 3: Game Testing

**Tier 1 - Must Work Perfectly:**
- Super Mario World
- The Legend of Zelda: A Link to the Past
- Super Metroid
- Chrono Trigger
- Final Fantasy VI

**Tier 2 - Should Work Well:**
- F-Zero (Mode 7)
- Super Mario Kart (Mode 7)
- Donkey Kong Country (pre-rendered graphics)
- Street Fighter II Turbo
- Mega Man X

**Tier 3 - Nice to Have:**
- Yoshi's Island (Super FX chip)
- Star Fox (Super FX chip)
- Contra III
- Castlevania IV
- Super Ghouls 'n Ghosts

### Phase 4: User Testing
- Beta release to homebrew community
- Collect feedback on depth settings
- Identify problem games
- Iterate on configurations

---

## Known Challenges

### Technical Challenges

1. **Performance:**
   - Stereo rendering = 2x pixel fill (left + right eye)
   - Must maintain 60 FPS on New 3DS
   - Mitigation: GPU acceleration, texture caching, New 3DS boost

2. **Transparency Effects:**
   - SNES has complex color math modes
   - Must preserve visual accuracy in 3D
   - Mitigation: GPU blend modes, special shaders

3. **Mode 7:**
   - Rotation/scaling layer needs special handling
   - Per-scanline depth adds complexity
   - Mitigation: Gradient depth, strip rendering

4. **Memory Constraints:**
   - Multiple layer buffers increase RAM usage
   - GPU VRAM is limited (10 MB)
   - Mitigation: Texture compression, efficient allocation

### Design Challenges

1. **Depth Configuration:**
   - Every game needs tuning for best effect
   - Too much depth = eye strain
   - Too little = no effect
   - Mitigation: Conservative defaults, per-game profiles, user adjustment

2. **UI/Gameplay Separation:**
   - Hard to auto-detect status bars, menus
   - These should stay at screen plane
   - Mitigation: Per-game handlers, heuristics

3. **Special Effects:**
   - Some games use tricks that break layering
   - Mid-frame changes, pseudo-transparency
   - Mitigation: Game-specific workarounds

4. **User Expectation:**
   - Users expect M2-level quality
   - But we're homebrew with limited resources
   - Mitigation: Clear documentation, realistic goals

---

## Success Metrics

### Minimum Viable Product (MVP)
- [ ] 5 games with convincing 3D effect
- [ ] 60 FPS on New 3DS XL
- [ ] Accurate rendering (matches 2D mode)
- [ ] 3D slider works smoothly
- [ ] No crashes or corruption

### Version 1.0 Release
- [ ] 20+ games with depth profiles
- [ ] All SNES modes supported (0-7)
- [ ] Savestates working
- [ ] Fast-forward working
- [ ] User depth adjustment UI
- [ ] Stable and polished

### Version 2.0 (Stretch Goals)
- [ ] 50+ game profiles
- [ ] Per-scanline depth for Mode 7
- [ ] Sprite depth separation
- [ ] Community profile sharing
- [ ] Auto-update depth database

---

## Timeline

### Realistic Timeline (Part-Time Development)

**Month 1:**
- Weeks 1-2: Foundation & setup
- Weeks 3-4: Layer separation system

**Month 2:**
- Weeks 5-6: Stereo rendering engine
- Week 7: Depth configuration
- Week 8: Testing & debugging

**Month 3:**
- Weeks 9-10: Advanced features
- Weeks 11-12: Polish & documentation

**Total: 3 months to MVP**

### Aggressive Timeline (Full-Time Development)

**Weeks 1-2:** Foundation through layer separation
**Weeks 3-4:** Stereo rendering engine
**Week 5:** Depth config + testing
**Week 6:** Advanced features + polish

**Total: 6 weeks to MVP**

---

## Resources & References

### Documentation
- **SNES Development:**
  - https://snesdev.org/
  - https://wiki.superfamicom.org/
- **3DS Homebrew:**
  - https://devkitpro.org/wiki/
  - https://www.3dbrew.org/
- **Citro3D:**
  - https://github.com/devkitPro/citro3d
  - https://github.com/devkitPro/3ds-examples

### Inspiration
- **M2 GigaDrive Interviews:**
  - Siliconera articles on 3D Classics development
  - NintendoLife interviews
- **Existing 3DS Homebrew:**
  - Red Viper (Virtual Boy emulator with 3D)
  - 3DSNES (current SNES emulator)

### Community
- **GBAtemp Forums** - 3DS homebrew discussion
- **Reddit:** r/3dshacks, r/EmuDev
- **Discord:** 3DS Homebrew servers

---

## License & Credits

### Licensing
- SNES9x core: Custom non-commercial license
- Citro3D: zlib license
- Our modifications: TBD (likely GPLv3 or similar)

### Credits
- **SNES9x Team** - Original emulator
- **bubble2k16** - SNES9x 3DS port
- **devkitPro Team** - Toolchain and libraries
- **M2** - Inspiration and pioneering techniques
- **Community** - Testing and feedback

---

## Contact & Contribution

### Project Lead
- TBD

### How to Contribute
- Report bugs via GitHub Issues
- Submit depth profiles for games
- Contribute code via Pull Requests
- Join discussion on GBAtemp/Discord

### Support
- Documentation: `docs/` folder
- FAQs: Wiki
- Discord: TBD

---

## Changelog

### Version 0.1 (Planning Phase)
- Initial project outline created
- Architecture designed
- Development phases defined

### Future Versions
- TBD as development progresses

---

**Last Updated:** 2025-10-20
**Document Version:** 1.0
**Status:** Planning Phase
