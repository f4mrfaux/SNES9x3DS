# Getting Started - SNES 3D Layer Separation Project

**Quick start guide for development setup and first steps**

---

## Prerequisites

### Hardware Required

- **New Nintendo 3DS XL** with custom firmware
  - Luma3DS or similar CFW installed
  - Homebrew Launcher working
  - SD card with at least 2GB free space

- **Development PC** (Linux, macOS, or Windows)
  - We'll use Linux for this guide

### Knowledge Prerequisites

- Basic C/C++ programming
- Familiarity with 3DS homebrew (helpful but not required)
- Understanding of emulation concepts
- Git version control

---

## Step 1: Install devkitPro Toolchain

### On Linux (Recommended)

```bash
# Download and install devkitPro pacman
wget https://github.com/devkitPro/pacman/releases/latest/download/devkitpro-pacman.amd64.deb
sudo dpkg -i devkitpro-pacman.amd64.deb

# Update package database
sudo dkp-pacman -Sy

# Install 3DS development packages
sudo dkp-pacman -S 3ds-dev

# This installs:
# - devkitARM (ARM compiler toolchain)
# - libctru (3DS system library)
# - citro3d (GPU library)
# - citro2d (2D helper library)
# - 3dstools (makerom, etc.)
```

### Set Environment Variables

Add to `~/.bashrc` or `~/.zshrc`:

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
export PATH=$DEVKITPRO/tools/bin:$PATH
```

Then reload:
```bash
source ~/.bashrc
```

### Verify Installation

```bash
# Check ARM compiler
arm-none-eabi-gcc --version
# Should show: arm-none-eabi-gcc (devkitARM ...) 14.x.x

# Check 3DS tools
3dsxtool --help
makerom --help
```

---

## Step 2: Clone the Project

```bash
# Navigate to your projects directory
cd ~/projects/3ds\ hax/

# Clone this repository (if you haven't already)
git clone <your-repo-url> 3ds-conversion-project
cd 3ds-conversion-project

# Verify structure
ls -la repos/
# Should see: matbo87-snes9x_3ds, devkitpro-3ds-examples, etc.
```

---

## Step 3: Build Baseline SNES9x

Let's verify we can build the existing emulator:

```bash
cd repos/matbo87-snes9x_3ds

# Clean any previous builds
make clean

# Build 3dsx (Homebrew Launcher version)
make

# Or build CIA (installed to home menu)
make cia
```

**Expected Output:**
- `snes9x_3ds.3dsx` - Homebrew launcher executable
- `snes9x_3ds.cia` - CIA installer file
- `snes9x_3ds.smdh` - Icon/metadata

**If Build Fails:**

Common issues:
1. **Missing dependencies:** Run `sudo dkp-pacman -S 3ds-dev` again
2. **Old toolchain:** Update with `sudo dkp-pacman -Syu`
3. **Environment vars not set:** Check `echo $DEVKITPRO`

---

## Step 4: Test on 3DS Hardware

### Method 1: 3dslink (fastest for testing)

```bash
# Connect 3DS to same network as PC
# Start Homebrew Launcher on 3DS
# Run on PC:
make 3dslink

# This sends .3dsx directly to 3DS over network
```

### Method 2: Manual SD Card Copy

```bash
# Copy to SD card
cp snes9x_3ds.3dsx /path/to/sd/3ds/snes9x_3ds/
cp snes9x_3ds.smdh /path/to/sd/3ds/snes9x_3ds/

# Eject SD, insert in 3DS
# Launch from Homebrew Launcher
```

### Method 3: Install CIA

```bash
# Copy CIA to SD card
cp snes9x_3ds.cia /path/to/sd/

# On 3DS:
# Open FBI homebrew app
# Navigate to SD card
# Install snes9x_3ds.cia
# Launch from home menu
```

### Test a Game

```bash
# Copy a SNES ROM to SD card
cp /path/to/game.sfc /path/to/sd/roms/snes/

# Launch SNES9x on 3DS
# Browse to game.sfc
# Play to verify everything works
```

**Expected:** Game runs at 60 FPS on New 3DS XL, controls work, audio plays.

---

## Step 5: Build Citro3D Stereo Example

Let's verify stereoscopic 3D rendering works:

```bash
cd ../devkitpro-3ds-examples/graphics/gpu/stereoscopic_2d

# Build the example
make

# Send to 3DS
make 3dslink
# Or copy stereoscopic_2d.3dsx to SD card
```

### Test Stereo Example

1. Launch `stereoscopic_2d` on 3DS
2. Move 3D slider up
3. You should see Lenny face pop out in 3D!
4. Press D-Pad to adjust depth offset
5. Press START to exit

**This proves:**
- Citro3D stereo rendering works
- 3D slider integration works
- Parallax offset calculation works

---

## Step 6: Explore the Codebase

Now let's understand what we're working with:

### Key Files in SNES9x Fork

```bash
cd repos/matbo87-snes9x_3ds

# Main entry point
less source/3dsmain.cpp

# GPU abstraction
less source/3dsgpu.h
less source/3dsgpu.cpp

# SNES GPU implementation
less source/3dsimpl_gpu.h
less source/3dsimpl_gpu.cpp

# Tile caching
less source/3dsimpl_tilecache.cpp

# SNES9x core (read-only, understand but don't modify)
ls source/Snes9x/
```

### Look for Key Concepts

**Search for layer compositing:**
```bash
grep -r "DrawBackground" source/
grep -r "RenderSprite" source/
grep -r "BG[0-9]" source/ | head -20
```

**Find rendering loop:**
```bash
grep -r "RenderFrame" source/
grep -r "StartNewFrame" source/
```

**GPU texture usage:**
```bash
grep -r "gpu3dsCreateTexture" source/
grep -r "gpu3dsBindTexture" source/
```

---

## Step 7: Create Development Branch

```bash
cd repos/matbo87-snes9x_3ds

# Create our development branch
git checkout -b stereo-3d-layers

# Make a trivial change to test
echo "// Stereo 3D layer separation - WIP" >> source/3dsmain.cpp

# Commit
git add .
git commit -m "Initial stereo 3D development branch"

# Build to verify
make clean && make
```

---

## Step 8: First Code Modification - Add 3D Slider Check

Let's make our first modification to understand the build/test cycle:

### Edit source/3dsmain.cpp

Find the main loop (around line 200-300) and add:

```cpp
// In the main emulation loop:

// Read 3D slider state
float sliderValue = osGet3DSliderState();

// Print to bottom screen (for debugging)
static int frameCount = 0;
if (frameCount++ % 60 == 0) {  // Print once per second
    printf("\x1b[10;1H3D Slider: %.2f", sliderValue);
}
```

### Build and Test

```bash
make clean && make
make 3dslink  # Or copy to SD card

# On 3DS:
# Launch SNES9x
# Load a game
# Move 3D slider
# Watch bottom screen - slider value should update
```

**If this works:** You've successfully modified the emulator! 🎉

---

## Step 9: Study Stereoscopic Example Code

Now let's understand how to implement stereo:

```bash
cd ../devkitpro-3ds-examples/graphics/gpu/stereoscopic_2d

# Open the main file
less source/main.cpp
```

### Key Takeaways from Example

```cpp
// 1. Enable stereoscopic mode
gfxSet3D(true);

// 2. Create render targets for each eye
C3D_RenderTarget* left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
C3D_RenderTarget* right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);

// 3. Read slider
float slider = osGet3DSliderState();

// 4. Render left eye with positive offset
C2D_SceneBegin(left);
C2D_DrawImageAt(image, baseX + offset * slider, y, 0);

// 5. Render right eye with negative offset
C2D_SceneBegin(right);
C2D_DrawImageAt(image, baseX - offset * slider, y, 0);

// 6. Frame ends automatically with both eyes rendered
C3D_FrameEnd(0);
```

**Formula:** `leftX = baseX + (depth * slider)`, `rightX = baseX - (depth * slider)`

---

## Step 10: Plan Your First Prototype

### Goal: Render current SNES output in stereo (no layer separation yet)

**Strategy:**
1. Add Citro3D to SNES9x Makefile
2. Create left/right render targets
3. In rendering loop, duplicate current output twice
4. Apply small parallax offset to test
5. Integrate 3D slider

**Next file to create:**
```
repos/matbo87-snes9x_3ds/source/3dsstereo.cpp
repos/matbo87-snes9x_3ds/source/3dsstereo.h
```

**Skeleton code:**
```cpp
// 3dsstereo.h
#ifndef _3DSSTEREO_H_
#define _3DSSTEREO_H_

#include <citro3d.h>

bool stereo3dsInitialize();
void stereo3dsFinalize();
void stereo3dsRenderFrame(void* frameBuffer, int width, int height);
float stereo3dsGetSliderValue();

#endif

// 3dsstereo.cpp
#include "3dsstereo.h"

static C3D_RenderTarget* leftTarget = NULL;
static C3D_RenderTarget* rightTarget = NULL;

bool stereo3dsInitialize() {
    gfxSet3D(true);
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    leftTarget = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, -1);
    rightTarget = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, -1);

    C3D_RenderTargetSetOutput(leftTarget, GFX_TOP, GFX_LEFT,
                              DISPLAY_TRANSFER_FLAGS);
    C3D_RenderTargetSetOutput(rightTarget, GFX_TOP, GFX_RIGHT,
                              DISPLAY_TRANSFER_FLAGS);

    return true;
}

void stereo3dsRenderFrame(void* frameBuffer, int width, int height) {
    float slider = osGet3DSliderState();

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    // Render left eye (TODO: implement)
    C3D_FrameDrawOn(leftTarget);
    // ... render frame with offset

    // Render right eye (TODO: implement)
    C3D_FrameDrawOn(rightTarget);
    // ... render frame with opposite offset

    C3D_FrameEnd(0);
}
```

---

## Troubleshooting

### Build Errors

**Error: "citro3d.h: No such file or directory"**
```bash
# Install citro3d
sudo dkp-pacman -S citro3d
```

**Error: "undefined reference to C3D_Init"**
- Add to Makefile: `LIBS := -lcitro3d -lctru -lm`

**Error: "make: arm-none-eabi-gcc: Command not found"**
- Environment variables not set
- Run: `export DEVKITARM=/opt/devkitpro/devkitARM`

### Runtime Errors

**Black screen on 3DS**
- Check if ROM is loaded
- Verify .3dsx and .smdh copied together
- Try different ROM

**Crash on startup**
- Check debug logs (if Luma3DS exception handler enabled)
- Try Old 3DS mode (disable New 3DS features)

**3D effect not working**
- Verify `gfxSet3D(true)` called
- Check 3D slider isn't at 0
- Confirm on New 3DS (Old 3DS has weaker effect)

---

## Resources for Learning

### 3DS Development

- [devkitPro Getting Started](https://devkitpro.org/wiki/Getting_Started)
- [3DBrew Wiki](https://www.3dbrew.org/wiki/Main_Page)
- [Citro3D Documentation](https://github.com/devkitPro/citro3d/wiki)

### SNES Architecture

- [SNES Development Wiki](https://wiki.superfamicom.org/)
- [Fullsnes Documentation](https://problemkaputt.de/fullsnes.htm)

### OpenGL/3D Graphics (transferable concepts)

- [Learn OpenGL](https://learnopengl.com/) - Many concepts apply to Citro3D
- [OpenGL Tutorial](https://www.opengl-tutorial.org/)

---

## Next Steps

Once you have:
- ✅ devkitPro installed and working
- ✅ SNES9x building successfully
- ✅ Tested on New 3DS hardware
- ✅ Studied stereoscopic example
- ✅ Made your first code modification

**You're ready for:**
1. Implementing stereo rendering prototype
2. Analyzing layer extraction requirements
3. Beginning actual stereoscopic layer separation

See [PROJECT_OUTLINE.md](PROJECT_OUTLINE.md) Phase 1 for detailed next steps.

---

## Quick Command Reference

```bash
# Build SNES9x
cd repos/matbo87-snes9x_3ds && make clean && make

# Send to 3DS over network
make 3dslink

# Build CIA
make cia

# Clean build
make clean

# Check 3DS tools
arm-none-eabi-gcc --version
3dsxtool --help

# Update devkitPro packages
sudo dkp-pacman -Syu
```

---

**Last Updated:** 2025-10-20
**Status:** Development Environment Setup Guide
