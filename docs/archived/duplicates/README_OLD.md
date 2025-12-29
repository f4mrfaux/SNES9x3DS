# SNES 3D Layer Separation Project

**Stereoscopic 3D for SNES Games on New Nintendo 3DS XL**

Inspired by M2's groundbreaking work on 3D Classics (GigaDrive), this project aims to bring stereoscopic 3D depth to SNES games by separating and rendering background and sprite layers at different depths on the Nintendo 3DS's autostereoscopic display.

---

## Project Status

🚧 **Planning Phase** - Setting up foundation and analyzing existing codebases

### Completed
- ✅ Project outline and architecture design
- ✅ Technical analysis of SNES9x 3DS codebase
- ✅ Research on M2's GigaDrive techniques
- ✅ Repository cloning and setup
- ✅ Citro3D stereoscopic rendering research

### Next Steps
- [ ] Set up development environment (devkitPro)
- [ ] Build baseline SNES9x fork
- [ ] Create stereo rendering prototype
- [ ] Implement layer extraction

---

## What This Project Does

### The Vision

Transform classic SNES games into stereoscopic 3D experiences by:

1. **Extracting SNES Graphics Layers**
   - Separate BG1, BG2, BG3, BG4 (background layers)
   - Isolate sprite layer
   - Preserve window/UI elements

2. **Rendering in Stereoscopic 3D**
   - Render each layer at configurable depth
   - Use 3DS's autostereoscopic display
   - Respond to 3D slider for intensity control

3. **Game-Specific Configuration**
   - Per-game depth profiles
   - Manual depth adjustment UI
   - Special handling for Mode 7 and effects

### Example: Super Mario World in 3D

**Before (2D):**
```
[All layers flat on screen]
```

**After (Stereoscopic 3D):**
```
[Status bar]     ← At screen plane (depth 0.0)
[Mario/enemies]  ← Slightly forward (depth 1.5)
[Level tiles]    ← Mid-depth (depth 2.0)
[Sky/clouds]     ← Far background (depth 4.0)
```

Result: A diorama-like effect where backgrounds recede into the screen and characters pop out!

---

## Project Structure

```
3ds-conversion-project/
├── README.md                    # This file
├── docs/                        # Documentation
│   ├── PROJECT_OUTLINE.md       # Comprehensive project plan
│   └── TECHNICAL_ANALYSIS.md    # Technical implementation details
├── repos/                       # Cloned repositories
│   ├── matbo87-snes9x_3ds/      # Base SNES9x fork
│   ├── bubble2k16-snes9x_3ds/   # Original GPU implementation
│   ├── devkitpro-3ds-examples/  # Citro3D examples (stereo!)
│   └── devkitpro-citro3d/       # Citro3D library source
├── research/                    # Research notes and findings
├── tools/                       # Development utilities
└── assets/                      # Graphics, configs, etc.
```

---

## Technical Approach

### Base: matbo87/snes9x_3ds

**Why this fork?**
- Already uses GPU rendering (fast!)
- Modern toolchain (devkitARM r62, libctru 2.2.2)
- Active development (v1.52 - August 2025)
- Stable, well-tested
- New 3DS optimizations

### Architecture: Hybrid GPU + Citro3D

```
SNES9x Core Emulation
        ↓
Layer Extraction (modify existing GPU code)
        ↓
Stereo Layer Manager (NEW)
        ↓
Citro3D Stereoscopic Renderer (NEW)
   ↙          ↘
Left Eye    Right Eye
   ↓            ↓
3DS Autostereoscopic Display
```

### Key Technologies

- **PICA200 GPU** - Nintendo 3DS graphics processor
- **Citro3D** - Homebrew GPU wrapper library
- **libctru** - Core 3DS system functions
- **devkitARM** - ARM toolchain for 3DS

---

## Inspiration: M2's GigaDrive

M2's approach for Sega 3D Classics:

1. **Virtual Enhanced Hardware**
   - Created "GigaDrive" - imaginary 3D-capable Genesis
   - Added 4 extra background layers (6 total vs 2 original)
   - Assigned Z-depth to each layer and scanline

2. **GPU Rendering**
   - Drew each layer as textured quad at different depth
   - Left/right eye views with parallax offset
   - Hardware acceleration for performance

3. **Per-Game Tuning**
   - Manual depth assignment for each stage
   - Split UI elements to screen plane
   - Preserved original visuals when 3D off

**Our Advantage:** SNES has 4 native BG layers vs Genesis's 2 - more natural for depth separation!

---

## Features

### Planned for v1.0 (MVP)

- [x] Stereoscopic 3D rendering
- [ ] BG layer separation (BG1-4)
- [ ] Sprite layer rendering
- [ ] 3D slider integration
- [ ] Default depth profiles per BG mode
- [ ] Per-game depth configuration
- [ ] Depth adjustment UI
- [ ] 60 FPS on New 3DS XL
- [ ] Savestates (10 slots)
- [ ] Fast-forward (2x-4x)

### Planned for v2.0 (Advanced)

- [ ] Per-scanline depth (Mode 7 gradients)
- [ ] Sprite depth separation
- [ ] Automatic UI/HUD detection
- [ ] 50+ game depth profiles
- [ ] Game-specific handlers
- [ ] Community depth profile sharing

---

## Requirements

### Hardware

- **New Nintendo 3DS XL** (required)
  - 804 MHz quad-core CPU (vs 268 MHz on Old 3DS)
  - 256 MB RAM
  - 10 MB VRAM
  - Stereoscopic display

**Note:** Old 3DS NOT supported due to performance constraints

### Software

- Custom firmware (Luma3DS recommended)
- Homebrew Launcher
- devkitPro toolchain (for development)

---

## Development Timeline

### Realistic (Part-Time)

- **Month 1:** Foundation + Layer Separation
- **Month 2:** Stereo Rendering + Configuration
- **Month 3:** Additional Features + Polish

**Total:** ~3 months to MVP

### Aggressive (Full-Time)

- **Weeks 1-2:** Foundation through layer separation
- **Weeks 3-4:** Stereo rendering engine
- **Week 5:** Depth config + testing
- **Week 6:** Advanced features + polish

**Total:** ~6 weeks to MVP

---

## Documentation

### For Developers

- [**Project Outline**](docs/PROJECT_OUTLINE.md) - Complete implementation plan
- [**Technical Analysis**](docs/TECHNICAL_ANALYSIS.md) - GPU architecture & migration strategy

### For Users (Coming Soon)

- Installation guide
- Depth tuning guide
- Game compatibility list
- FAQ

---

## References & Resources

### M2's GigaDrive Research

- [Siliconera Interview Series](https://www.siliconera.com/) - M2's development process
- [Nintendo Life Articles](https://www.nintendolife.com/) - 3D Classics coverage
- [Sega Dev Forums](https://gendev.spritesmind.net/) - GigaDrive technical discussions

### 3DS Homebrew Development

- [devkitPro](https://devkitpro.org/) - Toolchain and libraries
- [3DBrew](https://www.3dbrew.org/) - 3DS technical wiki
- [Citro3D GitHub](https://github.com/devkitPro/citro3d) - GPU library
- [3DS Examples](https://github.com/devkitPro/3ds-examples) - Code samples

### SNES Development

- [SNESdev.org](https://snesdev.org/) - SNES hardware documentation
- [Super Famicom Wiki](https://wiki.superfamicom.org/) - PPU and graphics info

### Community

- [GBAtemp Forums](https://gbatemp.net/) - 3DS homebrew discussion
- Reddit: [r/3dshacks](https://reddit.com/r/3dshacks), [r/EmuDev](https://reddit.com/r/EmuDev)

---

## Contributing

🚧 **Project is in early planning stage**

Once we have a working prototype, we'll welcome:

- Depth profile submissions for games
- Bug reports and testing
- Performance optimization
- Documentation improvements
- Code contributions

---

## Credits & Acknowledgments

### This Project

- **Your Other AI** - Comprehensive GigaDrive research
- **Claude (Anthropic)** - Project planning and technical analysis

### Standing on the Shoulders of Giants

- **M2** - Pioneering stereoscopic layer separation techniques
- **bubble2k16** - Original SNES9x 3DS port with GPU rendering
- **matbo87** - Updated fork with modern features
- **SNES9x Team** - Core SNES emulation
- **devkitPro Team** - Toolchain, libctru, Citro3D
- **3DS Homebrew Community** - Knowledge, tools, inspiration

---

## License

**TBD** - Will respect licenses of:
- SNES9x (custom non-commercial license)
- Citro3D (zlib license)
- libctru (zlib license)

Likely GPLv3 or similar for our modifications.

---

## Contact

🚧 **Project Lead:** TBD

**Project Start Date:** October 20, 2025
**Current Phase:** Planning & Setup
**Target Platform:** New Nintendo 3DS XL

---

*"Making classic games pop with the magic of stereoscopic 3D!"* 🎮✨

---

**Last Updated:** 2025-10-20
