# Project Status Report

**SNES 3D Layer Separation for New 3DS XL**

**Date:** October 20, 2025
**Phase:** Initial Planning and Setup
**Overall Progress:** 15% (Foundation Complete)

---

## ✅ Completed Tasks

### Research & Planning
- [x] Comprehensive research on M2's GigaDrive techniques
- [x] Analysis of SNES PPU layer architecture
- [x] Review of homebrew community efforts
- [x] Identification of best base emulator (matbo87/snes9x_3ds)
- [x] Citro3D stereoscopic rendering research

### Documentation
- [x] PROJECT_OUTLINE.md - 33KB comprehensive plan
- [x] TECHNICAL_ANALYSIS.md - 21KB technical deep-dive
- [x] GETTING_STARTED.md - Developer setup guide
- [x] README.md - Project overview
- [x] This STATUS.md

### Repository Setup
- [x] Cloned matbo87/snes9x_3ds (v1.52 - latest)
- [x] Cloned bubble2k16/snes9x_3ds (original reference)
- [x] Cloned devkitpro/3ds-examples (includes stereoscopic_2d!)
- [x] Cloned devkitpro/citro3d (GPU library source)
- [x] Organized project directory structure

### Analysis
- [x] Identified GPU rendering architecture (custom wrapper)
- [x] Located layer compositing code paths
- [x] Found stereoscopic example code (perfect reference!)
- [x] Analyzed shader system (separate for Mode 7)
- [x] Documented memory budget (plenty of headroom)
- [x] Assessed performance targets (60 FPS achievable)

---

## 🚧 In Progress

### Current Focus
- [ ] Setting up development environment
- [ ] Building baseline SNES9x fork
- [ ] Testing on New 3DS XL hardware

### Immediate Next Steps (Week 1)
1. Install devkitPro toolchain
2. Build matbo87 fork successfully
3. Test on hardware
4. Study `3dsimpl_gpu.cpp` rendering code
5. Create first stereo prototype

---

## 📋 Upcoming Milestones

### Sprint 1: Foundation (Week 1)
**Goal:** Stereo rendering prototype (no layer separation)

**Tasks:**
- [ ] Set up dev environment
- [ ] Build and test baseline
- [ ] Add Citro3D to build system
- [ ] Create `3dsstereo.cpp` module
- [ ] Render current output in stereo with test offset
- [ ] Integrate 3D slider

**Deliverable:** SNES game displays in basic stereo

### Sprint 2: Layer Extraction (Weeks 2-3)
**Goal:** Separate SNES layers into individual buffers

**Tasks:**
- [ ] Deep dive into `3dsimpl_gpu.cpp`
- [ ] Identify BG layer rendering functions
- [ ] Modify to output separate buffers instead of composite
- [ ] Handle sprite layer separately
- [ ] Preserve transparency/blending
- [ ] Test with Super Mario World

**Deliverable:** Layer buffers extracted successfully

### Sprint 3: Stereo Rendering (Week 4)
**Goal:** Render layers at different depths

**Tasks:**
- [ ] Implement layer-based stereo renderer
- [ ] Apply depth-based parallax offsets
- [ ] Sort layers back-to-front
- [ ] Handle alpha blending correctly
- [ ] Test depth effect

**Deliverable:** Convincing 3D effect with separated layers

### Sprint 4: Configuration (Week 5)
**Goal:** Make depth configurable

**Tasks:**
- [ ] Create default depth profiles per BG mode
- [ ] Implement JSON game profile loader
- [ ] Add in-emulator depth adjustment UI
- [ ] Test with 10+ games
- [ ] Performance optimization pass

**Deliverable:** Polished stereo 3D for multiple games

### Sprint 5: Features (Week 6)
**Goal:** Add missing emulator features

**Tasks:**
- [ ] Implement savestates (10 slots)
- [ ] Add fast-forward (2x-4x)
- [ ] Create game depth profile database
- [ ] Documentation for users

**Deliverable:** Feature-complete v1.0 release

---

## 📊 Key Metrics

### Project Scope
- **Estimated Total Effort:** 8-10 weeks to v1.0
- **Complexity:** Medium-High
- **Risk Level:** Medium (proven technique, solid base)

### Technical Metrics
- **Target FPS:** 60 FPS on New 3DS XL
- **Target Games (v1.0):** 20+ with depth profiles
- **Memory Usage:** ~15 MB (well within 256 MB budget)
- **VRAM Usage:** ~2-3 MB (within 10 MB budget)

### Code Statistics
- **Base Codebase:** ~50,000 lines (matbo87 fork)
- **Expected New Code:** ~2,000-3,000 lines
- **Modified Existing:** ~500-1,000 lines

---

## 🎯 Success Criteria

### Minimum Viable Product (MVP)
- [ ] 5 games with convincing 3D effect
- [ ] 60 FPS on New 3DS XL
- [ ] Pixel-perfect accuracy when 3D disabled
- [ ] 3D slider works smoothly
- [ ] No crashes or graphical corruption

### Version 1.0 Release
- [ ] 20+ games with depth profiles
- [ ] All SNES modes supported (0-7)
- [ ] Savestates working
- [ ] Fast-forward working
- [ ] User depth adjustment UI
- [ ] Stable and polished

### Version 2.0 (Stretch)
- [ ] 50+ game profiles
- [ ] Per-scanline depth (Mode 7)
- [ ] Sprite depth separation
- [ ] Community profile sharing

---

## 🔬 Technical Decisions Made

### 1. Base Emulator: matbo87/snes9x_3ds ✅
**Rationale:**
- Modern, actively maintained (v1.52 August 2025)
- GPU rendering already implemented
- Good performance baseline
- Clean codebase

### 2. Rendering Approach: Hybrid (Custom GPU + Citro3D) ✅
**Rationale:**
- Lower risk than full rewrite
- Leverage existing optimizations
- Citro3D handles stereo complexity
- Incremental migration path

**Alternative Considered:** Full Citro3D migration
- **Rejected:** Too risky, longer timeline

### 3. Target Platform: New 3DS XL Only ✅
**Rationale:**
- Old 3DS too slow for stereo (268 MHz)
- New 3DS has 3x CPU (804 MHz) + L2 cache
- Stereo rendering = 2x workload minimum
- 60 FPS requires headroom

### 4. Missing Features: Add to Project ✅
**Savestates + Fast-Forward:**
- **Rationale:** User convenience, complete package
- Adds ~1 week but delivers polished v1.0

---

## 📚 Repository Status

### Cloned Repositories

| Repository | Purpose | Status | Version |
|------------|---------|--------|---------|
| matbo87/snes9x_3ds | Base emulator | ✅ Cloned | v1.52 |
| bubble2k16/snes9x_3ds | Reference | ✅ Cloned | v1.42 |
| devkitpro/3ds-examples | Stereo example | ✅ Cloned | Latest |
| devkitpro/citro3d | GPU library | ✅ Cloned | Latest |

### Project Files

| File | Size | Status | Purpose |
|------|------|--------|---------|
| PROJECT_OUTLINE.md | 33 KB | ✅ Complete | Full implementation plan |
| TECHNICAL_ANALYSIS.md | 21 KB | ✅ Complete | GPU architecture analysis |
| GETTING_STARTED.md | 13 KB | ✅ Complete | Developer setup guide |
| README.md | 8 KB | ✅ Complete | Project overview |
| STATUS.md | This file | 🚧 Live | Current status |

---

## 🔍 Key Findings from Analysis

### Good News 👍

1. **GPU Rendering Already Implemented**
   - bubble2k16 did the hard work
   - Tiles rendered via GPU, not software
   - Performance is excellent

2. **Stereoscopic Example Available**
   - devkitpro has working stereo demo
   - Clear reference code
   - Proven technique

3. **New 3DS Performance Headroom**
   - 804 MHz CPU plenty fast
   - 256 MB RAM more than enough
   - VRAM budget comfortable

4. **Active Base Codebase**
   - matbo87 fork maintained (last update Aug 2025)
   - Bugs being fixed
   - Clean code to work with

### Challenges Identified ⚠️

1. **Custom GPU Wrapper**
   - Not using Citro3D natively
   - Need hybrid approach
   - Requires understanding custom API

2. **Layer Compositing Buried in Code**
   - Need to locate exact compositing point
   - Must preserve special effects
   - Mode 7 is special case

3. **Missing Features (Savestates, Fast-Forward)**
   - Not in current version
   - Need to implement from scratch
   - Or port from other fork

4. **Per-Game Tuning Required**
   - Every game needs depth config
   - Time-consuming to optimize
   - Need community contribution

### Risks Assessed 🎲

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Performance < 60 FPS | Medium | High | Profile early, optimize, New 3DS only |
| Visual artifacts | High | Medium | Extensive testing, per-game fixes |
| Complexity underestimated | Medium | High | Incremental dev, prototype early |
| Citro3D integration issues | Medium | Medium | Hybrid approach, careful API use |

---

## 💡 Lessons from M2's GigaDrive

### What M2 Did (Genesis 3D Classics)

1. **Virtual Enhanced Hardware**
   - Simulated "GigaDrive" with extra layers
   - Added depth registers to virtual VDP
   - Game code ran unmodified

2. **GPU Rendering**
   - Each layer = textured quad
   - Left/right eye with parallax
   - 60 FPS maintained

3. **Manual Tuning**
   - Per-stage depth assignment
   - Hand-placed depth values
   - Took significant time per game

4. **Aggressive Optimization**
   - Assembly for critical paths
   - Disabled extra features for performance
   - Memory/CPU budget carefully managed

### What We'll Do (SNES Adaptation)

1. **SNES Native Layers**
   - Use real BG1-4 layers (no simulation needed!)
   - SNES already has more layers than Genesis
   - Simpler extraction

2. **Citro3D + Existing GPU**
   - Hybrid approach
   - Leverage bubble2k16's optimizations
   - Add Citro3D stereo on top

3. **Community Tuning**
   - Start with defaults
   - User-adjustable depths
   - Community-contributed profiles

4. **New 3DS Power**
   - Target faster hardware from start
   - Don't need extreme optimization
   - Can add more features

---

## 📞 Communication

### For Future Collaboration

**When project is public:**
- GitHub Issues for bugs
- GBAtemp thread for discussion
- Discord server for real-time chat
- Wiki for depth profile submissions

**Currently:** Solo development / small team

---

## 🎮 Test Game List (Planned)

### Tier 1: Must Work Perfectly
- [ ] Super Mario World
- [ ] The Legend of Zelda: A Link to the Past
- [ ] Super Metroid
- [ ] Chrono Trigger
- [ ] Final Fantasy VI

### Tier 2: Should Work Well
- [ ] F-Zero (Mode 7)
- [ ] Super Mario Kart (Mode 7)
- [ ] Donkey Kong Country
- [ ] Mega Man X
- [ ] Street Fighter II Turbo

### Tier 3: Nice to Have
- [ ] Yoshi's Island (SFX chip)
- [ ] Star Fox (SFX chip)
- [ ] Contra III
- [ ] Castlevania IV
- [ ] Super Ghouls 'n Ghosts

---

## 📈 Progress Tracking

### Week 1 Goals (Current)
- [x] ~~Research and planning~~ ✅ DONE
- [x] ~~Repository setup~~ ✅ DONE
- [x] ~~Documentation~~ ✅ DONE
- [ ] Dev environment setup ⏳ NEXT
- [ ] Baseline build ⏳ NEXT
- [ ] Hardware testing ⏳ NEXT

### Week 2-3 Goals
- [ ] Layer extraction implementation
- [ ] Test with multiple games
- [ ] Performance baseline

### Week 4 Goals
- [ ] Stereo rendering engine
- [ ] Depth configuration
- [ ] UI for adjustment

### Week 5-6 Goals
- [ ] Additional features
- [ ] Polish and testing
- [ ] v1.0 release prep

---

## 🚀 Ready to Start Coding

**Prerequisites Met:**
- ✅ Research complete
- ✅ Architecture designed
- ✅ Base code analyzed
- ✅ Documentation written
- ✅ Risks assessed

**Next Action:**
👉 **Install devkitPro and build baseline SNES9x**

See [GETTING_STARTED.md](docs/GETTING_STARTED.md) for step-by-step instructions.

---

## 📝 Notes for Future Sessions

### Things to Remember

1. **Layer Extraction Key Files:**
   - `source/3dsimpl_gpu.cpp` - Main rendering
   - `source/3dsimpl_gpu.h` - API definitions
   - Search for: `DrawBackground`, `RenderSprite`

2. **Stereo Reference Code:**
   - `repos/devkitpro-3ds-examples/graphics/gpu/stereoscopic_2d/source/main.cpp`
   - Lines 27, 36-37 (enable 3D, create targets)
   - Lines 66-83 (render loop with parallax)

3. **Performance Critical:**
   - Enable New 3DS boost: `osSetSpeedupEnable(true)`
   - Use existing tile cache system
   - Minimize GPU state changes

4. **Missing in Base:**
   - No savestates (need to implement)
   - No fast-forward (need to implement)
   - No stereo 3D (obviously!)

---

**Last Updated:** 2025-10-20 18:50 UTC
**Next Update:** After dev environment setup and baseline build
**Overall Status:** 🟢 ON TRACK
