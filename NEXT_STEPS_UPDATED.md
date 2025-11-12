# SNES9x 3DS Stereoscopic - Next Steps (Updated)

**Status:** ✅ Implementation Complete - Hardware Testing & UI Development
**Date:** November 11, 2025 (Evening)
**Phase:** Hardware Testing → UI Development → Mode 7 Testing

---

## 🎯 Immediate Priority: Hardware Testing

### Step 1: Load on New 3DS XL
**Time:** 5 minutes
**Binary:** `~/Desktop/matbo87-snes9x_3ds.3dsx`

1. Copy to SD card `/3ds/` folder
2. Launch via Homebrew Launcher
3. Load test ROM (Super Mario World recommended)
4. Adjust 3D slider
5. Observe results

### Step 2: Initial Testing Checklist

**Basic Functionality:**
- [ ] Emulator launches successfully
- [ ] Game loads and runs
- [ ] Performance acceptable (count frames)
- [ ] No crashes or freezes

**3D Effect Testing:**
- [ ] 3D slider at 0%: Game looks normal (2D)
- [ ] 3D slider at 50%: Visible depth separation
- [ ] 3D slider at 100%: Maximum depth
- [ ] Layers separated correctly (clouds back, Mario forward)
- [ ] No double vision or ghosting
- [ ] No visual artifacts

**Performance Testing:**
- [ ] Mono (slider 0%): FPS measurement
- [ ] Stereo (slider 100%): FPS measurement
- [ ] FPS drop acceptable (target: > 50 FPS)
- [ ] No stuttering or frame pacing issues

### Step 3: Game-Specific Testing

**Standard 2D Games (Should work perfectly):**
- [ ] Super Mario World (platformer)
- [ ] Mega Man X (action)
- [ ] Chrono Trigger (RPG)

**Mode 7 Games (CRITICAL - may have issues):**
- [ ] F-Zero (racing)
- [ ] Super Mario Kart (racing)
- [ ] Pilotwings (flying)
- [ ] Final Fantasy VI (world map)

**Observations:**
```
Game: _______________
Slider: ___ %
Visual Quality: [Good / Acceptable / Broken]
FPS: ___ FPS
Notes: _______________
```

---

## 🎮 Phase 2: UI System Development

**Priority:** High (required for v1.0)
**Estimated Time:** 10-12 hours

### Why UI Instead of Per-Game Profiles?

**Benefits:**
- ✅ No need to create hundreds of JSON files
- ✅ Users customize on the fly
- ✅ Handles individual preferences
- ✅ Works for all games instantly
- ✅ More flexible and user-friendly

**Rationale:**
- Different users prefer different depth levels
- Same game may need different settings in different areas
- Real-time adjustment allows experimentation
- Saves development time (no profile creation)

### UI Design Goals

1. **Accessible:** Press SELECT to open during gameplay
2. **Simple:** D-Pad navigation, intuitive controls
3. **Instant:** See changes in real-time
4. **Persistent:** Settings save automatically
5. **Flexible:** Presets + custom adjustments

### Implementation Checklist

#### Part 1: Menu Infrastructure (3-4 hours)
- [ ] Add `StereoMenuOpen` state variable
- [ ] Create pause/resume functions
- [ ] Implement semi-transparent overlay
- [ ] Basic menu rendering (citro2D text)
- [ ] Input handling (SELECT to open/close)

#### Part 2: Depth Adjustment System (2-3 hours)
- [ ] Add layer selection (D-Pad Up/Down)
- [ ] Add value adjustment (D-Pad Left/Right)
- [ ] Preset definitions (Standard, Aggressive, Subtle)
- [ ] Apply changes to `LayerDepth[5]` in real-time
- [ ] Visual feedback (current values shown)

#### Part 3: Quick Shortcuts (1-2 hours)
- [ ] L + D-Pad Up/Down: Adjust all layers
- [ ] R + D-Pad Up/Down: Adjust sprites only
- [ ] L + R + SELECT: Toggle 3D on/off
- [ ] L + R + START: Reset to defaults

#### Part 4: Polish (2-3 hours)
- [ ] HUD indicator (shows current preset)
- [ ] Fade-out after 2 seconds
- [ ] Settings persistence (auto-save)
- [ ] Help text in menu
- [ ] Testing with multiple games

**Files to Modify:**
- `source/3dssettings.h` - Add UI state variables
- `source/3dsmenu.cpp` - Add `menu3dsShowStereoSettings()`
- `source/3dsinput.cpp` - Add input handlers
- `source/3dsui.cpp` - Add HUD indicator

**See:** `UI_AND_MODE7_PLAN.md` for complete design spec

---

## 🎢 Phase 3: Mode 7 Handling

**Priority:** High (required for racing game support)
**Estimated Time:** 3-5 hours

### Testing Required First

**Must test these games on hardware:**
1. F-Zero (road racing)
2. Super Mario Kart (kart racing)
3. Pilotwings (flying)
4. Final Fantasy VI (world map perspective)

**Possible Outcomes:**
1. ✅ **Works perfectly:** No action needed!
2. ⚠️ **Looks weird:** Need to adjust or disable
3. ❌ **Crashes:** Must fix before release

### Implementation Options

#### Option 1: Disable Stereo for Mode 7 (Safest)
**Time:** 1 hour
**Code:**
```cpp
if (PPU.BGMode == 7)
{
    // Force all offsets to 0
    for (int i = 0; i < 4; i++)
        g_stereoLayerOffsets[i] = 0.0f;
}
```

**Result:** Mode 7 games work correctly but stay 2D

#### Option 2: Sprites-Only 3D (Better)
**Time:** 2 hours
**Code:**
```cpp
if (PPU.BGMode == 7)
{
    // Disable BG offsets
    g_stereoLayerOffsets[0] = 0.0f;  // Mode 7 layer flat

    // Keep sprite offset
    g_stereoLayerOffsets[4] = settings3DS.LayerDepth[4] * slider;
}
```

**Result:** Road stays flat, racers/objects pop out

#### Option 3: Per-Scanline Gradient (Advanced)
**Time:** 8-12 hours (research + implementation)
**Complexity:** High
**Benefit:** Road has proper depth gradient

**Deferred:** Phase 4 (post-MVP)

### Checklist

- [ ] Test Mode 7 games with current implementation
- [ ] Document visual issues (if any)
- [ ] Implement Option 1 or 2 (based on results)
- [ ] Retest Mode 7 games
- [ ] Add Mode 7 toggle to UI menu
- [ ] Update documentation

---

## 📊 Development Roadmap

### v0.9 - MVP (Current)
**Status:** ✅ Complete - Ready for Testing
- [x] Vertex offset system
- [x] Layer tracking
- [x] Build successfully
- [ ] Hardware testing (in progress)

### v1.0 - Release Candidate
**Target:** 2-3 weeks
**Requirements:**
- [ ] Hardware testing complete
- [ ] UI system implemented
- [ ] Mode 7 handling complete
- [ ] Performance acceptable (>50 FPS)
- [ ] Documentation complete
- [ ] User guide created

### v1.1 - Enhanced
**Target:** 1-2 months
**Features:**
- [ ] Per-scanline Mode 7 gradient
- [ ] HUD auto-detection
- [ ] Advanced preset system
- [ ] Community profile sharing

### v2.0 - Advanced
**Target:** 3-6 months
**Features:**
- [ ] Per-sprite depth
- [ ] Game-specific optimizations
- [ ] Machine learning depth suggestions
- [ ] VR output support (experimental)

---

## 🔍 Testing Protocol

### Phase 1: Smoke Test (30 minutes)
**Goal:** Verify basic functionality

1. Load Super Mario World
2. Play for 5 minutes with slider at different positions
3. Check for crashes, artifacts, performance
4. Document FPS at 0%, 50%, 100% slider

**Pass Criteria:**
- No crashes
- 3D effect visible
- FPS > 40 at 100% slider

### Phase 2: Compatibility Test (2 hours)
**Goal:** Test diverse game types

**Test Suite:**
- Platform: Super Mario World, Donkey Kong Country
- Action: Mega Man X, Contra III
- RPG: Chrono Trigger, Final Fantasy VI
- Racing: F-Zero, Super Mario Kart
- Puzzle: Tetris Attack, Dr. Mario

**Document for each:**
- Visual quality (1-5 stars)
- Performance (FPS)
- Issues/artifacts
- Recommended depth settings

### Phase 3: Stress Test (1 hour)
**Goal:** Find edge cases

**Tests:**
- Rapid slider adjustment
- Game mode changes (battle to world map)
- Save/load states
- Menu transitions
- Multiple hours of play

### Phase 4: User Experience Test (2 hours)
**Goal:** Gather subjective feedback

**Questions:**
- Is depth effect pleasing?
- Any eye strain after 30 minutes?
- Preferred depth level?
- Suggested improvements?

---

## 📝 Documentation Tasks

### User Documentation
- [ ] Installation guide
- [ ] Quick start guide
- [ ] UI controls reference
- [ ] Troubleshooting FAQ
- [ ] Recommended settings per game type

### Developer Documentation
- [ ] Architecture overview
- [ ] Build instructions
- [ ] API reference
- [ ] Contributing guide
- [ ] Future enhancement ideas

### Media
- [ ] Screenshots (2D vs 3D comparison)
- [ ] Video demonstration
- [ ] GIF animations
- [ ] Social media posts

---

## 🎯 Success Criteria

### Minimum (v1.0 Release)
- [ ] Builds successfully
- [ ] Runs on New 3DS XL
- [ ] 3D effect visible and controllable
- [ ] Performance > 50 FPS (stereo mode)
- [ ] UI menu functional
- [ ] Mode 7 games work (even if 2D)
- [ ] No crashes in testing
- [ ] Documentation complete

### Desired (v1.1+)
- [ ] Performance > 55 FPS
- [ ] Mode 7 with depth
- [ ] HUD detection
- [ ] Community profiles
- [ ] Universal-Updater release

### Stretch (v2.0+)
- [ ] Per-sprite depth
- [ ] AI-assisted optimization
- [ ] Cross-platform (RetroArch port?)

---

## 🚀 Getting Started TODAY

### If you have New 3DS hardware:
1. ✅ Copy `matbo87-snes9x_3ds.3dsx` to SD card
2. ✅ Launch and test
3. ✅ Document results
4. ✅ Report back findings

### If no hardware yet:
1. ✅ Read `UI_AND_MODE7_PLAN.md`
2. ✅ Design UI mockups
3. ✅ Plan implementation approach
4. ✅ Prepare for UI development phase

### Either way:
- ✅ Celebrate successful implementation!
- ✅ Review documentation
- ✅ Plan next sprint
- ✅ Consider UI design decisions

---

## 💭 Open Questions

**Hardware Testing:**
- Does it work as expected?
- Is depth effect pleasing or too strong/weak?
- Any performance issues?
- Mode 7 behavior?

**UI Design:**
- Should preset be selectable or adjustable?
- How many presets to include?
- Shoulder button shortcuts intuitive?
- Help text needed?

**Mode 7:**
- Does current implementation work?
- Which approach to take?
- Worth the effort for gradient?

**Performance:**
- Can we hit 60 FPS?
- Is 50 FPS acceptable?
- Need optimization?

---

## 📞 Next Session Plan

**Session Goal:** Hardware testing and UI planning

**Agenda:**
1. Test on real hardware (30-60 min)
2. Document results
3. Decide on Mode 7 approach
4. Begin UI implementation or refine based on testing
5. Set v1.0 milestone

**Success Metric:** Clear path forward based on real hardware results

---

**Document Version:** 2.0
**Last Updated:** November 11, 2025 (Evening)
**Status:** ✅ Implementation Complete → Hardware Testing Phase
**Next Milestone:** Test on New 3DS XL, Plan UI System

---

*Made with ❤️ for the 3DS Homebrew Community*
*"From research to working code in 10.5 hours!"*
