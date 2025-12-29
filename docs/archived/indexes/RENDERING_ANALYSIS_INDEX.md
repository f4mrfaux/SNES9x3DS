# SNES9x 3DS Rendering Pipeline - Analysis Documentation Index

This directory contains comprehensive analysis of the SNES9x 3DS rendering pipeline, including architecture documentation, file references, and implementation guidance for stereoscopic 3D support.

## Generated Documentation Files

### 1. SNES9X_3DS_RENDERING_ANALYSIS.md (547 lines)
**Comprehensive Complete Analysis**

The main detailed document covering:
- Rendering architecture overview
- Complete rendering pipeline description
- PPU emulation and layer rendering (BG0-3, sprites, Mode 7)
- Transparency and blending implementation
- Screen update and display pipeline
- 3DS-specific features (parallax barrier, 3D slider)
- Tile cache system operation
- Performance optimization techniques
- Current GPU rendering status
- **CRITICAL:** Where to inject stereo layer separation (3 options analyzed)
- Potential challenges and considerations
- Key file locations and functions
- Data flow diagram
- Complete summary

**Use this for:** Deep understanding of how rendering works and where to implement stereoscopic features.

### 2. RENDERING_ARCHITECTURE_SUMMARY.md (528 lines)
**Quick Reference Architecture Guide**

Contains:
- Quick reference file locations and statistics
- Complete rendering flow (ASCII flowchart)
- Layer rendering hierarchy with priority ordering
- Data structures for rendering (vertex types, textures)
- Tile caching system operation flowchart
- Background layer rendering detail
- Sprite/OBJ rendering detail
- Mode 7 rendering special handling
- Blending and transparency implementation
- 3D hardware integration points
- Performance-critical sections with timing instrumentation
- Recommended stereo code injection points (3 options)
- Summary table of key concepts
- Files by importance for stereo implementation

**Use this for:** Quick lookup of how specific features work, ASCII flowcharts, and visual reference.

### 3. FILE_REFERENCE.md (251 lines)
**Complete File Path Reference with Functions**

Contains:
- Absolute file paths for all key source files
- Key functions by location with line numbers
- Critical code sections for stereo implementation
- Data flow files organized by component
- Configuration and settings files
- Testing and debugging files
- Summary of files to modify for stereo implementation with priority levels

**Use this for:** Finding specific code locations and understanding file organization.

## Quick Navigation

### I need to understand...

**...how the main rendering loop works**
- See: RENDERING_ARCHITECTURE_SUMMARY.md - "Complete Rendering Flow"
- Code: `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsimpl.cpp:502`

**...how layers are rendered (BG0-3)**
- See: SNES9X_3DS_RENDERING_ANALYSIS.md - "Section 3.3: Background Layer Rendering"
- Also: RENDERING_ARCHITECTURE_SUMMARY.md - "Background Layer Rendering Detail"
- Code: `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfxhw.cpp:1509`

**...how sprites are rendered**
- See: RENDERING_ARCHITECTURE_SUMMARY.md - "Sprite Rendering Detail"
- Code: `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/Snes9x/gfxhw.cpp:2578`

**...how GPU rendering works**
- See: SNES9X_3DS_RENDERING_ANALYSIS.md - "Section 9: Current GPU Rendering Status"
- Code: `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsgpu.cpp`

**...where to inject stereo code**
- See: SNES9X_3DS_RENDERING_ANALYSIS.md - "Section 10: Where to Inject Stereo Layer Separation"
- Recommended: Section 10.2 - "Option 2: Per-Layer Depth Manipulation"

**...what the 3D slider does**
- See: SNES9X_3DS_RENDERING_ANALYSIS.md - "Section 6.1: Parallax Barrier / 3D Slider Support"
- Also: RENDERING_ARCHITECTURE_SUMMARY.md - "3D Hardware Integration Points"
- Code: `<PROJECT_ROOT>/repos/matbo87-snes9x_3ds/source/3dsgpu.cpp:152`

**...how transparency and blending work**
- See: SNES9X_3DS_RENDERING_ANALYSIS.md - "Section 4: Transparency & Blending"
- Also: RENDERING_ARCHITECTURE_SUMMARY.md - "Blending & Transparency"

**...about the tile cache system**
- See: SNES9X_3DS_RENDERING_ANALYSIS.md - "Section 7: Tile Cache System"
- Also: RENDERING_ARCHITECTURE_SUMMARY.md - "Tile Caching System"

**...about performance bottlenecks**
- See: SNES9X_3DS_RENDERING_ANALYSIS.md - "Section 8: Performance Optimization Techniques"
- Also: RENDERING_ARCHITECTURE_SUMMARY.md - "Performance-Critical Sections"

## Architecture Summary

```
SNES9x 3DS Rendering Pipeline (Hybrid CPU-GPU):

SNES CPU Emulation
  ↓
PPU Graphics Rendering
  ├─→ Tile Caching (CPU: bitplane → GPU format)
  ├─→ Layer Composition (CPU: BG0-3, OBJ)
  └─→ Blending Configuration (CPU: alpha, priority)
      ↓
Vertex Generation
  └─→ Position, TexCoord, Depth, Alpha (GPU-ready)
      ↓
GPU Rendering
  ├─→ Vertex Assembly (Geometry Shader on Real 3DS)
  ├─→ Depth/Alpha/Stencil Tests
  ├─→ Alpha Blending
  └─→ Render to Frame Buffer
      ↓
Screen Output
  ├─→ Format Conversion
  ├─→ Scaling (256×256 → 400×240)
  └─→ Display on 3DS LCD (with optional parallax barrier)
```

## Current 3D Support Status

- **Parallax Barrier:** CAN toggle on/off
- **Stereoscopic Rendering:** NOT IMPLEMENTED
- **Per-Layer Depth Offset:** NOT IMPLEMENTED
- **3D Slider Integration:** Only binary on/off

## Recommended Stereo Implementation Approach

**Option A: Per-Layer Depth Offset (RECOMMENDED)**
- Modify: S9xRenderScreenHardware() in gfxhw.cpp:3415
- Add stereo depth offset to layer depth calculations
- Minimal performance impact
- Maximum game compatibility
- Works with existing parallax barrier infrastructure

**Key Modification Points:**
1. Add stereo configuration to settings
2. Calculate stereo offset = base_offset * (layer_priority + layer_type)
3. Pass modified depth to GPU: depth = base_depth + stereo_offset
4. Leverage existing depth buffer for layer ordering

## File Organization

```
<PROJECT_ROOT>/
├── SNES9X_3DS_RENDERING_ANALYSIS.md        [THIS - Complete analysis]
├── RENDERING_ARCHITECTURE_SUMMARY.md       [Reference - Flowcharts & architecture]
├── FILE_REFERENCE.md                       [Lookup - File paths & functions]
├── README.md                               [Project overview]
├── STATUS.md                               [Project status]
│
└── repos/matbo87-snes9x_3ds/
    └── source/
        ├── 3dsgpu.cpp                      [Main GPU implementation]
        ├── 3dsgpu.h                        [GPU interface]
        ├── 3dsimpl.cpp                     [Main rendering loop: impl3dsRunOneFrame()]
        ├── 3dsimpl_gpu.cpp                 [Vertex generation]
        ├── 3dsimpl_tilecache.cpp           [Tile caching]
        │
        └── Snes9x/
            ├── gfxhw.cpp                   [KEY: S9xRenderScreenHardware() - Layer rendering]
            ├── gfx.h/cpp                   [Graphics state]
            ├── ppu.h/cpp                   [PPU emulation]
            └── cpuexec.cpp                 [CPU emulation]
```

## Key Statistics

- **Main Rendering Loop:** impl3dsRunOneFrame() - 136 lines
- **PPU Graphics Rendering:** S9xRenderScreenHardware() - 536 lines
- **Background Layer Rendering:** S9xDrawBackgroundHardwarePriority0Inline() - 301+ lines
- **Sprite Rendering:** S9xDrawOBJSHardware() - 170+ lines
- **GPU Command Buffer:** 1MB (2x512KB double-buffered)
- **Tile Cache:** 1024×1024 RGBA5551 (2MB)
- **Frame Buffer:** 256×256 RGBA8 (256KB)
- **Target Framerate:** 60 FPS NTSC (16.667ms per frame)

## Implementation Checklist for Stereo Support

- [ ] Read SNES9X_3DS_RENDERING_ANALYSIS.md Section 10
- [ ] Understand current layer depth encoding
- [ ] Review S9xRenderScreenHardware() function
- [ ] Design stereo offset calculation
- [ ] Add stereo configuration to settings
- [ ] Modify layer depth calculations in gfxhw.cpp
- [ ] Test with various games
- [ ] Adjust 3D slider integration
- [ ] Game-specific tuning (optional)
- [ ] Performance benchmarking

## Performance Considerations

**Current Bottlenecks:**
1. Tile conversion (CPU: bitplane → GPU format)
2. Layer composition logic (many tiles per layer)
3. GPU synchronization (waiting for previous frame)
4. Display transfer (512KB DMA)

**Stereo Implementation Impact:**
- **Option A (Depth Offset):** ~5-10% GPU overhead
- **Option B (Slider Enhancement):** ~0% additional overhead
- **Option C (Dual Rendering):** ~50-100% GPU overhead (NOT RECOMMENDED)

## Referenced Technologies

- **3DS GPU:** 3D graphics processor with geometry shader support
- **Parallax Barrier:** Fixed physical LCD interleaving pattern
- **3D Slider:** Analog input (0.0-1.0) from hardware register 0x1FF81080
- **SNES GPU:** Original PPU emulation in software
- **Depth Testing:** GPU_GEQUAL test for layer priority ordering
- **Alpha Blending:** Multiple blend modes (additive, subtractive, etc.)

---

**Last Updated:** October 21, 2025
**Analysis Scope:** Complete rendering pipeline for SNES9x 3DS
**Focus:** Stereoscopic 3D layer separation implementation

For detailed implementation guidance, see SNES9X_3DS_RENDERING_ANALYSIS.md Section 10.

