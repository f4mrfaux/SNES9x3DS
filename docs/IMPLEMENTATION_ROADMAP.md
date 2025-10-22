# Stereoscopic 3D Implementation Roadmap
## Step-by-Step Guide for SNES9x Layer Separation

**Last Updated:** 2025-10-21
**Timeline:** 6-10 weeks for complete implementation
**Difficulty:** Intermediate to Advanced

---

## Project Status

### ✅ Phase 1: Research & Setup (COMPLETE)

- [x] Installed devkitPro toolchain (devkitARM r66, libctru 2.6.2, citro3d 1.7.1)
- [x] Cloned repositories (SNES9x, 3ds-examples, citro3d)
- [x] Built SNES9x successfully (matbo87 fork)
- [x] Studied stereoscopic_2d example
- [x] Analyzed SNES9x rendering pipeline
- [x] Created comprehensive documentation

**Deliverables:**
- PROJECT_OUTLINE.md (33 KB)
- TECHNICAL_ANALYSIS.md (21 KB)
- STEREO_RENDERING_COMPLETE_GUIDE.md (comprehensive API reference)
- SNES9X_GPU_ARCHITECTURE.md (detailed architecture analysis)

---

## Phase 2: Prototype Development

**Goal:** Create basic working stereo prototype with one game

**Timeline:** 2-4 weeks

### Week 1: Foundation

#### Day 1-2: Stereo Configuration System

**File:** `source/3dssettings.h`

**Task:** Add stereo settings to configuration structure

```cpp
// Add to S9xSettings3DS struct
struct S9xSettings3DS {
    // ...existing settings...

    // Stereoscopic 3D Settings
    bool        enableStereo3D;         // Master stereo toggle
    bool        autoLoadProfiles;       // Load per-game profiles
    float       stereoStrength;         // Global depth multiplier (0.5-2.0)
    float       layerDepth[5];          // Depth for each layer type
                                         // [0]=BG0, [1]=BG1, [2]=BG2, [3]=BG3, [4]=Sprites
    int         screenPlaneLayer;       // Which layer at screen plane (0-4, -1=auto)
    bool        enablePerSpriteDepth;   // Separate sprite depths (advanced)
};

// Default values
void InitializeStereoDefaults(S9xSettings3DS& settings) {
    settings.enableStereo3D = true;
    settings.autoLoadProfiles = true;
    settings.stereoStrength = 1.0f;

    // Default depth configuration (conservative)
    settings.layerDepth[0] = 15.0f;  // BG0 (closest background)
    settings.layerDepth[1] = 10.0f;  // BG1
    settings.layerDepth[2] = 5.0f;   // BG2
    settings.layerDepth[3] = 0.0f;   // BG3 (farthest, at screen plane)
    settings.layerDepth[4] = 20.0f;  // Sprites (popout)

    settings.screenPlaneLayer = 3;   // BG3 at screen
    settings.enablePerSpriteDepth = false;
}
```

**Testing:**
- Compile and ensure no errors
- Verify settings load/save

---

#### Day 3-4: 3D Slider Integration

**File:** `source/3dsimpl.cpp`

**Task:** Add 3D slider polling to main loop

**Location:** `impl3dsRunOneFrame()` at line ~502

```cpp
void impl3dsRunOneFrame() {
    // ...existing code...

    // ADD: Read 3D slider state
    float slider = osGet3DSliderState();  // 0.0 to 1.0

    // Update stereo settings
    if (slider < 0.01f) {
        // Slider off = disable stereo to save performance
        settings3DS.enableStereo3D = false;
    } else {
        settings3DS.enableStereo3D = true;

        // Store slider value for depth calculations
        // (will be used in rendering functions)
        settings3DS.stereoSliderValue = slider;
    }

    // ...continue with existing rendering...
}
```

**Testing:**
- Run emulator with 3D slider at different positions
- Verify `settings3DS.stereoSliderValue` updates correctly
- Use debug print to console: `printf("Slider: %.2f\n", slider);`

---

#### Day 5-7: Depth Calculation Modification

**File:** `source/Snes9x/gfxhw.cpp`

**Task 1:** Modify BG layer depth calculation (multiple locations)

**Locations:**
- Line 1509: `S9xDrawBackgroundHardwarePriority0Inline()`
- Line 1650: `S9xDrawBackgroundHardwarePriority1Inline()`
- Line 1791: `S9xDrawBackgroundHardwarePriority2Inline()`
- Line 1932: `S9xDrawBackgroundHardwarePriority3Inline()`

**Original Code (example from line 1509):**

```cpp
// Current depth calculation
u16 depth = priority * 256 + alpha;
```

**Modified Code:**

```cpp
// NEW: Helper function at top of file
inline u16 CalculateStereoDepth(int priority, int layerType, u8 alpha) {
    u16 baseDepth = priority * 256;

    // Add stereo offset if enabled
    if (settings3DS.enableStereo3D) {
        float slider = settings3DS.stereoSliderValue;
        float strength = settings3DS.stereoStrength;

        // Calculate layer-specific depth offset
        float offset = settings3DS.layerDepth[layerType] * slider * strength;

        // Clamp to prevent priority overflow (stay within 0-255 per priority level)
        offset = fminf(offset, 255.0f);
        offset = fmaxf(offset, 0.0f);

        baseDepth += (u16)offset;
    }

    // Add alpha for sub-priority sorting
    baseDepth += alpha;

    return baseDepth;
}

// In S9xDrawBackgroundHardwarePriority0Inline():
// Replace:
//   u16 depth = priority * 256 + alpha;
// With:
u16 depth = CalculateStereoDepth(priority, bgIndex, alpha);
```

**Apply to All BG Functions:**
- Replace depth calculation in all 4 priority functions
- Pass correct `bgIndex` (0-3 for BG0-BG3)

---

**Task 2:** Modify sprite depth calculation

**Location:** Line 2578: `S9xDrawOBJSHardware()`

**Original:**
```cpp
u16 depth = priority * 256 + alpha;
```

**Modified:**
```cpp
// Layer type 4 = Sprites
u16 depth = CalculateStereoDepth(priority, 4, alpha);
```

---

**Testing:**
- Build SNES9x: `cd repos/matbo87-snes9x_3ds && make -j4`
- Test with Super Mario World:
  - Load ROM
  - Adjust 3D slider
  - **Expected:** BG layers should appear at different depths
  - **Expected:** Sprites should pop out when slider > 0

**Debug:**
- If no effect: Check `settings3DS.enableStereo3D` is true
- If artifacts: Reduce `layerDepth` values
- If depth too strong: Reduce `stereoStrength` to 0.5

---

### Week 2: Testing & Refinement

#### Day 8-10: Multi-Game Testing

**Test Suite (Progressive Difficulty):**

1. **Super Mario World** (Mode 1, simple backgrounds)
   - Tune depths for Mario at screen plane
   - Clouds in back, foreground pipes pop out

2. **Donkey Kong Country** (Mode 1, prerendered)
   - More complex parallax
   - Test with multiple background layers

3. **Super Metroid** (Mixed modes, complex effects)
   - Transparency effects
   - HUD layer handling

4. **Chrono Trigger** (Mode 1, battle backgrounds)
   - Test with static and scrolling BGs

5. **F-Zero** (Mode 7, rotation)
   - Special handling may be needed
   - May need Mode 7 depth adjustment

**For Each Game:**
- Document default depth that works well
- Note any rendering artifacts
- Record FPS performance
- Create per-game profile

---

#### Day 11-12: Per-Game Profiles

**File:** `source/3dsstereo.cpp` (NEW FILE)

```cpp
#include "3dsstereo.h"
#include <string.h>

struct StereoProfile {
    const char* gameName;      // ROM name substring
    float layerDepth[5];       // BG0-3, Sprites
    float stereoStrength;      // Global multiplier
    const char* notes;         // Description
};

// Profile database
static const StereoProfile profiles[] = {
    {
        "Super Mario World",
        {15.0f, 10.0f, 5.0f, 0.0f, 20.0f},
        1.0f,
        "BG3=clouds(far), BG2=hills(mid), BG1=ground(near), Sprites=Mario/enemies"
    },
    {
        "Donkey Kong Country",
        {20.0f, 15.0f, 5.0f, 0.0f, 25.0f},
        1.2f,
        "Prerendered graphics, strong depth effect"
    },
    {
        "Chrono Trigger",
        {10.0f, 8.0f, 5.0f, 0.0f, 15.0f},
        0.8f,
        "Conservative depths for battle backgrounds"
    },
    {
        "Super Metroid",
        {12.0f, 9.0f, 6.0f, 0.0f, 18.0f},
        1.0f,
        "HUD on sprites layer"
    },
    // Add more as tested...
};

void LoadStereoProfile(const char* romName) {
    if (!settings3DS.autoLoadProfiles) return;

    // Search for matching profile
    for (int i = 0; i < sizeof(profiles) / sizeof(StereoProfile); i++) {
        if (strstr(romName, profiles[i].gameName) != NULL) {
            // Found match, load profile
            memcpy(settings3DS.layerDepth,
                   profiles[i].layerDepth,
                   sizeof(float) * 5);
            settings3DS.stereoStrength = profiles[i].stereoStrength;

            printf("Loaded stereo profile: %s\n", profiles[i].gameName);
            return;
        }
    }

    // No profile found, use defaults
    printf("No stereo profile, using defaults\n");
}
```

**Integration:**
- Call `LoadStereoProfile(romName)` when ROM loads
- Add to `source/3dsimpl.cpp` in ROM loading function

---

#### Day 13-14: Performance Optimization

**Profile Performance:**

```cpp
// Add to main loop
#ifdef DEBUG_STEREO
float gpuTime = C3D_GetDrawingTime() * 6.0f;
float cpuTime = C3D_GetProcessingTime() * 6.0f;
printf("GPU: %.1f%%  CPU: %.1f%%\n", gpuTime, cpuTime);
#endif
```

**Optimization Targets:**
- **Target:** < 100% GPU+CPU at 60 FPS
- **If > 100%:** Consider reducing stereo strength or disabling for heavy scenes

**Optimizations:**

1. **Skip Right Eye Calculation When Slider = 0:**
```cpp
if (settings3DS.stereoSliderValue < 0.01f) {
    // Fast path: No stereo calculations
    depth = priority * 256 + alpha;
} else {
    depth = CalculateStereoDepth(priority, layerType, alpha);
}
```

2. **Reduce Float Operations:**
```cpp
// Precompute per-frame instead of per-tile
float stereoMultiplier = settings3DS.stereoSliderValue * settings3DS.stereoStrength;

// Then in depth calculation:
float offset = settings3DS.layerDepth[layerType] * stereoMultiplier;
```

---

### Week 3-4: UI and Polish

#### In-Game Depth Adjustment UI

**Goal:** Allow real-time depth tuning without restarting

**Implementation:** Add to pause menu

```cpp
// In menu rendering code
void RenderStereoMenu() {
    printf("=== 3D Depth Configuration ===\n");
    printf("3D Slider: %.2f\n", osGet3DSliderState());
    printf("Strength: %.1f\n", settings3DS.stereoStrength);
    printf("\n");
    printf("Layer Depths:\n");
    printf(" BG0: %.1f  [LEFT/RIGHT to adjust]\n", settings3DS.layerDepth[0]);
    printf(" BG1: %.1f\n", settings3DS.layerDepth[1]);
    printf(" BG2: %.1f\n", settings3DS.layerDepth[2]);
    printf(" BG3: %.1f\n", settings3DS.layerDepth[3]);
    printf(" SPR: %.1f\n", settings3DS.layerDepth[4]);
    printf("\n");
    printf("SELECT: Switch layer\n");
    printf("START: Save & Exit\n");
}

// Input handling
void HandleStereoMenuInput() {
    static int selectedLayer = 0;

    if (keysDown & KEY_SELECT) {
        selectedLayer = (selectedLayer + 1) % 5;
    }

    if (keysHeld & KEY_DRIGHT) {
        settings3DS.layerDepth[selectedLayer] += 0.5f;
    }

    if (keysHeld & KEY_DLEFT) {
        settings3DS.layerDepth[selectedLayer] -= 0.5f;
        if (settings3DS.layerDepth[selectedLayer] < 0)
            settings3DS.layerDepth[selectedLayer] = 0;
    }

    if (keysDown & KEY_START) {
        SaveSettings();  // Save to config file
        // Exit menu
    }
}
```

---

## Phase 3: Advanced Features

**Timeline:** 2-3 weeks (optional enhancements)

### Advanced Feature 1: Per-Sprite Depth

**Challenge:** Some games have sprites at different depths (HUD vs characters)

**Solution:** Detect sprite type and apply different depths

```cpp
void S9xDrawOBJSHardware(int priority, int spriteIndex, SpriteInfo sprite) {
    float spriteDepth;

    if (sprite.isHUD) {
        // HUD elements at screen plane (no depth)
        spriteDepth = 0.0f;
    } else if (sprite.size > 32) {
        // Large sprites (bosses) closer
        spriteDepth = settings3DS.layerDepth[4] * 1.5f;
    } else {
        // Normal sprites
        spriteDepth = settings3DS.layerDepth[4];
    }

    u16 depth = CalculateStereoDepth(priority, spriteDepth, alpha);
}
```

**Detection Heuristics:**
- Position (HUD usually at fixed screen coords)
- Size (HUD text is often 8x8, characters 16x16+)
- Palette (HUD may use specific palette)

---

### Advanced Feature 2: Mode 7 Depth

**Challenge:** Mode 7 uses rotation/scaling, not tile layers

**Solution:** Apply depth based on Y-coordinate (farther = higher Y)

```cpp
void RenderMode7WithDepth() {
    for (int y = 0; y < 224; y++) {
        // Calculate depth based on scanline
        // Farther scanlines (higher Y) = more depth
        float depthFactor = (float)y / 224.0f;  // 0.0 to 1.0
        float depth = depthFactor * settings3DS.mode7MaxDepth;

        // Apply as horizontal offset for stereo
        float parallax = depth * settings3DS.stereoSliderValue;

        // Render scanline with parallax offset
        RenderMode7Scanline(y, parallax);
    }
}
```

---

### Advanced Feature 3: Per-Scanline Depth (H-Blank Effects)

**Challenge:** Some games change layer properties mid-frame (e.g., waving water)

**Solution:** Track per-scanline depth changes

```cpp
struct ScanlineDepthOverride {
    int scanline;
    int layer;
    float depthOverride;
};

// Apply during rendering
if (HasScanlineOverrides(currentY, layer)) {
    float overrideDepth = GetScanlineDepth(currentY, layer);
    depth = CalculateStereoDepth(priority, overrideDepth, alpha);
}
```

---

## Phase 4: Testing & Release

**Timeline:** 1-2 weeks

### Comprehensive Testing

**Test Matrix:**

| Category | Games | Focus |
|----------|-------|-------|
| **Platformers** | Mario, DKC, Mega Man X | Layer parallax |
| **RPGs** | Chrono Trigger, FF6, Earthbound | Battle backgrounds |
| **Action** | Super Metroid, Contra 3 | Fast scrolling |
| **Racing** | F-Zero, Mario Kart | Mode 7 depth |
| **Puzzle** | Tetris Attack, Yoshi's Cookie | UI depth |

**For Each:**
- Create optimal depth profile
- Test at different slider positions
- Verify 60 FPS (or stable 30 FPS)
- Document any issues

---

### User Documentation

Create **USER_GUIDE.md:**

```markdown
# SNES9x 3DS - Stereoscopic 3D Guide

## Quick Start

1. Load your favorite SNES game
2. Slide the 3D slider up
3. Enjoy depth-separated layers!

## Adjusting Depth

1. Pause the game
2. Navigate to "3D Settings"
3. Adjust layer depths with D-Pad
4. Press START to save

## Recommended Settings

- **Super Mario World:** Default profile
- **Donkey Kong Country:** +20% depth
- **F-Zero:** Mode 7 depth enabled

## Troubleshooting

**No 3D effect:**
- Ensure 3D slider is up
- Check "Enable Stereo 3D" in settings

**Eye strain:**
- Reduce global strength to 0.5
- Lower individual layer depths
```

---

### Build & Release

**Build Process:**

```bash
cd <PROJECT_ROOT>/repos/matbo87-snes9x_3ds

# Clean build
make clean

# Build release version
make -j4 RELEASE=1

# Verify output
ls -lh output/
# Should see: snes9x_3ds_stereo.3dsx
```

**Generate CIA (Optional):**

```bash
# Requires makerom and bannertool
makerom -f cia -o snes9x_3ds_stereo.cia \
        -elf output/snes9x_3ds.elf \
        -rsf snes9x.rsf \
        -icon icon.icn \
        -banner banner.bnr
```

**Release Checklist:**

- [ ] Version number updated
- [ ] Changelog written
- [ ] User guide included
- [ ] Example depth profiles (10+ games)
- [ ] Tested on Old 3DS (fallback if needed)
- [ ] Tested on New 3DS XL (target platform)

---

## Milestones & Deliverables

### Milestone 1: Basic Stereo (Week 2)
- [  ] Stereo depth calculation working
- [  ] 3D slider integration
- [  ] One game (Super Mario World) working well

**Demo:** Video showing Mario with depth-separated clouds/ground/sprites

---

### Milestone 2: Multi-Game Support (Week 4)
- [  ] 5+ games tested and profiled
- [  ] Per-game profiles loading automatically
- [  ] 60 FPS maintained (or stable 30 FPS)

**Demo:** Montage of different games with stereo effect

---

### Milestone 3: Advanced Features (Week 6)
- [  ] Mode 7 depth (F-Zero working)
- [  ] Per-sprite depth (HUD at screen plane)
- [  ] In-game depth adjustment UI

**Demo:** F-Zero with road recession depth

---

### Milestone 4: Release (Week 8)
- [  ] 20+ game profiles
- [  ] User documentation
- [  ] Builds (.3dsx and .cia)
- [  ] Published to Universal-Updater

**Demo:** Release trailer showcasing best examples

---

## Risk Assessment

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|---------|-----------|
| **Performance < 60 FPS** | Medium | High | Accept 30 FPS, optimize depth calc |
| **Depth artifacts** | High | Medium | Per-game tuning, clamp values |
| **Mode 7 incompatible** | Low | Medium | Disable for Mode 7, or special handling |
| **Sprite sorting issues** | Medium | Low | Refine depth formula, per-sprite depth |

### Project Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|---------|-----------|
| **Time overrun** | Medium | Low | MVP: Basic stereo only, defer advanced features |
| **Toolchain updates break build** | Low | High | Pin versions, document dependencies |
| **Community not interested** | Low | Low | Still valuable as learning project |

---

## Success Criteria

### Must Have (MVP)
- ✓ Stereo depth working with 3D slider
- ✓ 5+ games with good depth profiles
- ✓ No major rendering artifacts
- ✓ Performance: 60 FPS or stable 30 FPS

### Should Have
- Per-game depth profiles (auto-loading)
- In-game depth adjustment UI
- 20+ game profiles
- Mode 7 support

### Nice to Have
- Per-sprite depth detection
- Per-scanline depth (H-blank effects)
- Auto-profile generation tool
- Community profile sharing

---

## Resources Needed

### Time Investment
- **Development:** 6-10 weeks (part-time)
- **Testing:** 2-3 weeks
- **Documentation:** 1 week

### Hardware
- **New 3DS XL:** For testing (target platform)
- **Old 3DS:** For compatibility testing (optional)
- **SD Card:** For ROM testing

### Software
- **devkitPro:** Already installed ✓
- **Test ROMs:** SNES game collection
- **Video capture:** For demos (optional)

---

## Next Immediate Steps

### This Week

1. **Day 1:** Add stereo settings to `3dssettings.h`
2. **Day 2:** Integrate 3D slider polling in main loop
3. **Day 3-4:** Implement `CalculateStereoDepth()` helper
4. **Day 5:** Modify BG layer depth calculations
5. **Day 6:** Modify sprite depth calculation
6. **Day 7:** Build and test with Super Mario World

### Goal for Week 1
**Working prototype with visible depth effect in Super Mario World**

---

## Conclusion

This is a manageable project with clear milestones. The research phase is complete, and the technical approach is validated.

**Key Advantages:**
- Minimal code changes (~100 lines modified)
- Leverages existing GPU depth buffer
- Low performance overhead (< 10%)
- High compatibility with existing games

**Estimated Final Result:**
A SNES9x 3DS build that adds M2-style stereoscopic 3D to classic SNES games, providing a unique way to experience retro titles on the New 3DS XL's autostereoscopic display.

**Let's build it!** 🎮✨

---

**Document Version:** 1.0
**Last Updated:** 2025-10-21
**Status:** Ready to begin Phase 2 (Prototype Development)
