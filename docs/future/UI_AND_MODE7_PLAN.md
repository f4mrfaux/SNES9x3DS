# User Interface & Mode 7 Plan
## In-Game Depth Customization System

**Date:** November 11, 2025
**Status:** Planning Phase
**Priority:** High (for v1.0 release)

---

## 🎮 UI System: In-Game Depth Adjustment

### The Problem with Per-Game Profiles

**Original Plan:**
- Create JSON files with depth settings per game
- Auto-load by ROM CRC/name
- Requires manual profile creation for each game

**Issues:**
- Hundreds of SNES games = hundreds of profiles needed
- Users can't customize on the fly
- Different users have different depth preferences
- Some games have multiple modes requiring different settings

### Better Solution: Real-Time UI Adjustment

**User-Controlled Depth Assignment**

Allow users to adjust depth values in real-time through an in-game menu accessible via pause/SELECT button.

---

## 📐 Proposed UI Design

### Main Menu (Press SELECT during gameplay)

```
╔═══════════════════════════════════════╗
║     STEREOSCOPIC 3D SETTINGS          ║
╠═══════════════════════════════════════╣
║ 3D Effect: [ENABLED]    (A to toggle)║
║                                       ║
║ LAYER DEPTH ASSIGNMENT:               ║
║                                       ║
║ BG0 (Background):  [DEEP]    ◄ ►     ║
║ BG1 (Midground):   [MID]     ◄ ►     ║
║ BG2 (Foreground):  [NEAR]    ◄ ►     ║
║ BG3 (Sky/HUD):     [SCREEN]  ◄ ►     ║
║ Sprites (Objects): [POPOUT]  ◄ ►     ║
║                                       ║
║ PRESETS:                              ║
║ [Standard] [Aggressive] [Subtle]      ║
║ [Custom]   [Save]       [Reset]       ║
║                                       ║
║ B: Close   SELECT: Save & Resume      ║
╚═══════════════════════════════════════╝
```

### Depth Presets

**Screen Distances (in pixels at 100% slider):**

| Preset | BG0 | BG1 | BG2 | BG3 | Sprites | Use Case |
|--------|-----|-----|-----|-----|---------|----------|
| **POPOUT** | +25 | +20 | +15 | 0 | +30 | Max depth, sprites pop out |
| **DEEP** | +20 | +15 | +10 | 0 | +25 | Deep background |
| **MID** | +15 | +10 | +5 | 0 | +20 | Balanced (default) |
| **NEAR** | +10 | +5 | +2 | 0 | +15 | Slight depth |
| **SUBTLE** | +5 | +3 | +1 | 0 | +8 | Minimal effect |
| **SCREEN** | 0 | 0 | 0 | 0 | 0 | At screen plane (2D) |
| **INSCREEN** | -5 | -3 | -1 | 0 | +5 | Into screen only |

### Per-Layer Preset Options

Each layer can be individually set to:
- **POPOUT** (+30px) - Maximum pop-out
- **CLOSE** (+20px) - Close to viewer
- **NEAR** (+10px) - Slightly forward
- **SCREEN** (0px) - At screen plane
- **MID** (-5px) - Slightly back
- **DEEP** (-15px) - Deep into screen
- **FAR** (-25px) - Maximum depth

### Quick Access (During Gameplay)

**Shoulder Button Shortcuts:**
- **L + D-Pad Up/Down:** Quick adjust all layers together (increase/decrease depth)
- **R + D-Pad Up/Down:** Quick adjust sprite depth only
- **L + R + SELECT:** Toggle 3D on/off
- **L + R + START:** Reset to defaults

---

## 🎯 Implementation Plan

### Phase 1: Settings Infrastructure ✅ (Already Exists)
- [x] `settings3DS.LayerDepth[5]` array exists
- [x] `settings3DS.EnableStereo3D` toggle exists
- [x] `settings3DS.StereoSliderValue` tracking exists

### Phase 2: Add UI State Variables
**File:** `source/3dssettings.h`

```cpp
// UI state for depth customization
bool    StereoMenuOpen = false;           // Is the 3D menu currently open?
int     StereoMenuSelection = 0;           // Currently selected menu item (0-4 for layers)
bool    StereoSettingsModified = false;   // Have settings changed since last save?

// Quick preset values
enum StereoPreset {
    PRESET_STANDARD,    // Default balanced
    PRESET_AGGRESSIVE,  // Maximum depth
    PRESET_SUBTLE,      // Minimal depth
    PRESET_CUSTOM       // User-customized
};
StereoPreset CurrentPreset = PRESET_STANDARD;
```

### Phase 3: Implement Menu Rendering
**File:** `source/3dsmenu.cpp` (add new function)

```cpp
void menu3dsShowStereoSettings()
{
    // Pause emulation
    // Draw semi-transparent overlay
    // Render menu UI
    // Handle input (D-Pad, A, B, SELECT)
    // Update LayerDepth[5] values in real-time
    // Resume emulation when closed
}
```

**UI Libraries Available:**
- Use existing 3DS menu system (`3dsmenu.cpp`)
- citro2D for text rendering
- Simple button-based navigation

### Phase 4: Input Handling
**File:** `source/3dsinput.cpp` (modify)

```cpp
// In input polling loop:
if (keysDown() & KEY_SELECT)
{
    if (!emulationPaused && !menuOpen)
    {
        settings3DS.StereoMenuOpen = true;
        pauseEmulation();
        menu3dsShowStereoSettings();
    }
}

// Quick shortcuts (during gameplay):
if ((keysHeld() & KEY_L) && (keysDown() & KEY_DUP))
{
    // Increase all layer depths by 1px
    for (int i = 0; i < 5; i++)
        settings3DS.LayerDepth[i] = min(30.0f, settings3DS.LayerDepth[i] + 1.0f);
}
```

### Phase 5: Settings Persistence
**File:** `source/3dsconfig.cpp` (already handles settings save/load)

Settings are already saved/loaded automatically. Just need to ensure `LayerDepth[5]` values persist.

### Phase 6: Visual Feedback
**File:** `source/3dsui.cpp` (add HUD overlay)

```cpp
void ui3dsShowDepthIndicator()
{
    // Small HUD overlay showing current depth preset
    // Example: "3D: STANDARD" in corner of screen
    // Fades out after 2 seconds
}
```

---

## 🎢 Mode 7 Handling

### What is Mode 7?

**SNES Mode 7:**
- Rotation and scaling of a single background layer
- Used for: Road racing (F-Zero, Mario Kart), flying (Pilotwings), world maps (FF6)
- Creates pseudo-3D effect by scaling scanlines
- Each scanline can have different rotation/scale

**Visual Effect:**
```
Far away:  Small, compressed  (top of screen)
              ↓
              ↓ (perspective)
              ↓
Close up:  Large, expanded   (bottom of screen)
```

### Current Implementation Concerns

**Our Vertex Offset System:**
```cpp
// We apply horizontal offset uniformly:
x0 += offset;  // Same offset for entire layer
x1 += offset;
```

**Mode 7 Problem:**
- Mode 7 already has per-scanline transformations
- Each scanline has different X/Y offset for perspective
- Our horizontal offset might conflict or look wrong

### How Mode 7 Works in SNES9x

**File:** `source/Snes9x/gfxhw.cpp` (Mode 7 rendering)

Looking at the implementation log, Mode 7 uses different rendering:
- Special vertex setup: `gpu3dsInitializeMode7Vertex()`
- Per-tile transformations already applied
- Different shader: `shaderfastm7.v.pica`

**Key Question:** Does our `g_currentLayerIndex` get set during Mode 7 rendering?

### Potential Issues

1. **Uniform Offset Problem:**
   - Mode 7 creates depth illusion via scaling
   - Adding uniform horizontal offset might break the perspective
   - Far scanlines need less offset, near scanlines need more

2. **Conflicting Effects:**
   - Mode 7 simulates depth via vertical position
   - Our stereo simulates depth via horizontal offset
   - These might fight each other visually

3. **Layer Index:**
   - Mode 7 is technically BG0, but rendered differently
   - Need to verify `g_currentLayerIndex` is set correctly

### Solution Options

#### Option 1: Disable Stereo for Mode 7 (Simple)
```cpp
// In Mode 7 rendering:
if (PPU.BGMode == 7)
{
    g_currentLayerIndex = 3;  // Force to SCREEN plane (0 offset)
    // or
    float oldOffset = g_stereoLayerOffsets[0];
    g_stereoLayerOffsets[0] = 0.0f;  // Disable offset for Mode 7
    // ... render Mode 7 ...
    g_stereoLayerOffsets[0] = oldOffset;  // Restore
}
```

**Pros:**
- Simple, safe
- No visual conflicts
- Mode 7 games still work perfectly

**Cons:**
- No 3D effect for Mode 7 backgrounds
- Less impressive for racing games

#### Option 2: Per-Scanline Depth Gradient (Complex)
```cpp
// Apply depth offset based on Y position:
float depthGradient = (y - nearY) / (farY - nearY);  // 0.0 (near) to 1.0 (far)
float scaledOffset = baseOffset * (1.0f - depthGradient * 0.5f);
// Far scanlines: 50% offset
// Near scanlines: 100% offset
```

**Pros:**
- Creates proper depth for roads/horizons
- Enhances Mode 7 perspective
- Very impressive for racing games

**Cons:**
- Complex to implement
- Requires per-scanline offset (not per-layer)
- May not work with current vertex system

#### Option 3: Sprites-Only 3D for Mode 7 Games (Practical)
```cpp
// When Mode 7 active:
if (PPU.BGMode == 7)
{
    // Disable offset for BG layers
    g_stereoLayerOffsets[0] = 0.0f;
    g_stereoLayerOffsets[1] = 0.0f;
    g_stereoLayerOffsets[2] = 0.0f;
    g_stereoLayerOffsets[3] = 0.0f;

    // Keep sprite offset (characters pop out from road)
    g_stereoLayerOffsets[4] = settings3DS.LayerDepth[4] * slider;
}
```

**Pros:**
- Simple to implement
- Sprites (racers, objects) still pop out
- Background looks correct (no conflict)
- Good compromise

**Cons:**
- No depth for the road itself

### Recommended Approach

**Phase 1 (MVP): Option 1 - Disable for Mode 7**
- Safest approach
- Ensures Mode 7 games look correct
- Can enhance later

**Phase 2 (Enhancement): Option 3 - Sprites-Only**
- Better than nothing
- Characters/objects pop out
- Background stays flat

**Phase 3 (Advanced): Option 2 - Per-Scanline Gradient**
- Research M2's approach to Mode 7
- Implement if technically feasible
- Could be game-changing for racing games

### Mode 7 Detection

**File:** `source/Snes9x/gfxhw.cpp`

```cpp
// Check for Mode 7:
if (PPU.BGMode == 7)
{
    // Apply Mode 7-specific handling
}
```

**Existing Mode 7 Settings:**
```cpp
// From 3dssettings.h (already exists):
float   Mode7DepthNear = 1.0f;
float   Mode7DepthFar = 8.0f;
bool    Mode7UseGradient = false;
```

These were added in the original settings but never implemented!

---

## 📋 Implementation Checklist

### UI System
- [ ] Add UI state variables to settings
- [ ] Create depth preset definitions
- [ ] Implement menu rendering function
- [ ] Add input handlers (SELECT, D-Pad, shoulder buttons)
- [ ] Add visual feedback (HUD indicator)
- [ ] Test menu navigation
- [ ] Add settings save/load integration
- [ ] Add quick shortcuts (L+R combos)

### Mode 7 Handling
- [ ] Test current implementation with Mode 7 games (F-Zero, Mario Kart)
- [ ] Check if `g_currentLayerIndex` set correctly
- [ ] Implement Option 1 (disable for Mode 7) as MVP
- [ ] Test with multiple Mode 7 games
- [ ] Document behavior in user guide
- [ ] Plan future enhancement (per-scanline gradient)

### Testing Games
- [ ] **Mode 7 Required:** F-Zero, Super Mario Kart, Pilotwings, Final Fantasy 6 (world map)
- [ ] **Standard Modes:** Super Mario World, Donkey Kong Country, Mega Man X
- [ ] **Hi-Res Modes:** Seiken Densetsu 3, Star Ocean

---

## 🎨 UI Mockup Flow

### Step 1: User presses SELECT during gameplay
```
[Game pauses]
[Semi-transparent overlay]
[Menu appears]
```

### Step 2: Navigate with D-Pad
```
▶ BG0 (Background):  [DEEP]    ◄ ►
  BG1 (Midground):   [MID]     ◄ ►
  BG2 (Foreground):  [NEAR]    ◄ ►
  BG3 (Sky/HUD):     [SCREEN]  ◄ ►
  Sprites (Objects): [POPOUT]  ◄ ►
```

### Step 3: Adjust with Left/Right
```
  BG0 (Background):  [DEEP] ◄ [MID] ► [NEAR]
```

### Step 4: See changes in real-time
```
[Background preview updates]
[Current values shown]
```

### Step 5: Save or Cancel
```
SELECT: Save & Resume
B: Cancel & Revert
```

---

## 💡 User Experience Benefits

### Immediate Feedback
- See depth changes in real-time
- No need to exit game
- Experiment freely

### Per-Game Customization
- Users can optimize for each game
- Different preferences accommodated
- Save custom settings

### Accessibility
- Easy to disable if causing eye strain
- Quick reset to defaults
- Simple preset system

### Educational
- Users learn what each layer does
- Understanding of SNES hardware
- Appreciation for depth effect

---

## 🔮 Future Enhancements

### Advanced Features (Post-MVP)
1. **Per-Scene Profiles:**
   - Save different settings for different game areas
   - Auto-switch based on game state
   - Example: Different depths for world map vs battles in RPGs

2. **Community Profiles:**
   - Share depth settings online
   - Download popular community presets
   - Rating system for profiles

3. **Auto-Optimization:**
   - Analyze game visuals
   - Suggest optimal depth values
   - Machine learning approach

4. **Mode 7 Enhancement:**
   - Implement per-scanline gradient
   - Research M2's approach
   - Create dedicated Mode 7 presets

5. **HUD Detection:**
   - Auto-detect UI elements
   - Keep HUD at screen plane
   - Separate gameplay depth from UI depth

---

## 📊 Estimated Effort

| Task | Complexity | Time Estimate | Priority |
|------|-----------|---------------|----------|
| **UI Menu System** | Medium | 4-6 hours | High |
| **Input Handlers** | Low | 1-2 hours | High |
| **Preset System** | Low | 1-2 hours | High |
| **HUD Indicator** | Low | 1 hour | Medium |
| **Settings Persistence** | Low | 1 hour | High |
| **Mode 7 Testing** | Low | 2 hours | High |
| **Mode 7 Disable** | Low | 1 hour | High |
| **Mode 7 Sprites-Only** | Medium | 2-3 hours | Medium |
| **Documentation** | Low | 2 hours | High |

**Total Estimate:** 15-20 hours for full UI + Mode 7 handling

**MVP (UI + Mode 7 disable):** 10-12 hours

---

## 🎯 Recommendation

### Phase 1: Hardware Testing (Current)
1. Test current implementation on real hardware
2. Verify basic 3D effect works
3. Test with standard 2D games (Super Mario World)
4. **Test with Mode 7 game (F-Zero)** ← CRITICAL

### Phase 2: Mode 7 Handling (If needed)
1. Observe how current implementation handles Mode 7
2. Implement Option 1 (disable) if issues found
3. Document behavior

### Phase 3: UI System (v1.0 requirement)
1. Design menu layout
2. Implement menu system
3. Add preset system
4. Add quick shortcuts
5. Test extensively

### Phase 4: Polish (v1.0+)
1. Mode 7 sprites-only enhancement
2. HUD detection
3. Community sharing features
4. Per-scanline gradient research

---

## 🤔 Questions to Answer via Hardware Testing

1. **Does current implementation work with Mode 7?**
   - Does it look correct?
   - Does it look broken?
   - Does it crash?

2. **What do different games need?**
   - Do platformers need different settings than RPGs?
   - Do racing games work at all?

3. **What's the sweet spot for default values?**
   - Current defaults: {15, 10, 5, 0, 20}
   - Too much? Too little? Just right?

4. **Do users want more or less depth?**
   - Some people have depth perception issues
   - Some want maximum pop-out
   - UI flexibility is key

---

**Document Version:** 1.0
**Created:** November 11, 2025
**Status:** Planning - Awaiting Hardware Testing Results
**Priority:** High for v1.0 Release
