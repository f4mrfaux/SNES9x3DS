# Development Environment Setup

This guide will help you set up the development environment for the SNES 3D Layer Separation project.

---

## Prerequisites

### Required Software

1. **devkitPro** (devkitARM r64 or newer)
   - Installation: https://devkitpro.org/wiki/Getting_Started
   - Includes: devkitARM, libctru, citro3d

2. **Git** (for cloning repositories)
   - Linux: `sudo apt install git` or `sudo pacman -S git`
   - macOS: `brew install git`
   - Windows: https://git-scm.com/download/win

3. **Make** (build system)
   - Included with devkitPro

### Recommended Hardware

- **New Nintendo 3DS XL** (target platform)
  - Old 3DS may struggle with performance
- **SD Card** (4GB+ for homebrew and ROMs)

---

## Step-by-Step Setup

### 1. Install devkitPro

#### Linux (Arch-based)
```bash
# Add devkitPro repository
sudo pacman-key --recv BC26F752D25B92CE272E0F44F7FD5492264BB9D0
sudo pacman-key --lsign BC26F752D25B92CE272E0F44F7FD5492264BB9D0
echo '[dkp-libs]' | sudo tee -a /etc/pacman.conf
echo 'Server = https://pkg.devkitpro.org/packages' | sudo tee -a /etc/pacman.conf
echo '' | sudo tee -a /etc/pacman.conf
echo '[dkp-linux]' | sudo tee -a /etc/pacman.conf
echo 'Server = https://pkg.devkitpro.org/packages/linux/$arch/' | sudo tee -a /etc/pacman.conf

# Install packages
sudo pacman -Syu 3ds-dev
```

#### Linux (Debian/Ubuntu)
```bash
wget https://github.com/devkitPro/pacman/releases/latest/download/devkitpro-pacman.amd64.deb
sudo dpkg -i devkitpro-pacman.amd64.deb
sudo dkp-pacman -Syu 3ds-dev
```

#### macOS
```bash
brew install devkitpro/devkitpro/devkitARM
brew install devkitpro/devkitpro/libctru
brew install devkitpro/devkitpro/citro3d
```

#### Windows
Download and run the installer from: https://github.com/devkitPro/installer/releases

### 2. Set Environment Variables

Add to your `~/.bashrc` or `~/.zshrc`:

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
export PATH=$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH
```

Reload your shell:
```bash
source ~/.bashrc  # or source ~/.zshrc
```

### 3. Verify Installation

```bash
# Check devkitARM
arm-none-eabi-gcc --version
# Should show: gcc version 15.1.0 or newer

# Check environment variables
echo $DEVKITPRO
# Should show: /opt/devkitpro (or your install path)

# Check libctru
ls $DEVKITPRO/libctru/include/3ds.h
# Should exist

# Check citro3d
ls $DEVKITPRO/libctru/include/citro3d.h
# Should exist
```

### 4. Clone This Repository

```bash
git clone https://github.com/YOUR_USERNAME/snes-3d-layer-separation.git
cd snes-3d-layer-separation
```

### 5. Clone Required External Repositories

The project requires several external repositories (SNES9x forks, devkitPro examples, etc.). These are **NOT included** in this repo to keep it clean.

#### Option A: Use the Automated Script

```bash
chmod +x scripts/clone-repos.sh
./scripts/clone-repos.sh
```

#### Option B: Clone Manually

```bash
# Create repos directory
mkdir -p repos
cd repos

# Clone SNES9x forks
git clone https://github.com/matbo87/snes9x_3ds.git matbo87-snes9x_3ds
git clone https://github.com/bubble2k16/snes9x_3ds.git bubble2k16-snes9x_3ds

# Clone devkitPro examples
git clone https://github.com/devkitPro/3ds-examples.git devkitpro-3ds-examples

# Clone Citro3D library source (optional, for reference)
git clone https://github.com/devkitPro/citro3d.git devkitpro-citro3d

cd ..
```

### 6. Build SNES9x to Verify Setup

```bash
cd repos/matbo87-snes9x_3ds
make clean
make -j4

# Check output
ls output/matbo87-snes9x_3ds.3dsx
# Should exist if build succeeded
```

If you see `matbo87-snes9x_3ds.3dsx` (2.2 MB), the build was successful! ✅

### 7. Test Stereoscopic Example

```bash
cd ../devkitpro-3ds-examples/graphics/gpu/stereoscopic_2d
make clean
make

# Check output
ls stereoscopic_2d.3dsx
# Should exist if build succeeded
```

---

## Directory Structure After Setup

```
snes-3d-layer-separation/
├── docs/                              # Documentation
│   ├── STEREO_RENDERING_COMPLETE_GUIDE.md
│   ├── SNES9X_GPU_ARCHITECTURE.md
│   ├── IMPLEMENTATION_ROADMAP.md
│   └── QUICK_REFERENCE.md
├── repos/                             # External repositories (not in git)
│   ├── matbo87-snes9x_3ds/           # Main SNES9x fork
│   ├── bubble2k16-snes9x_3ds/        # Original SNES9x 3DS port
│   ├── devkitpro-3ds-examples/       # 3DS example code
│   └── devkitpro-citro3d/            # Citro3D library source
├── assets/                            # Project assets
├── research/                          # Research notes
├── tools/                             # Utility scripts
├── scripts/                           # Build/setup scripts
├── README.md                          # Main README
├── SETUP.md                           # This file
└── .gitignore                         # Git ignore rules
```

---

## Troubleshooting

### Build Errors: "arm-none-eabi-gcc: command not found"

**Problem:** devkitARM not in PATH

**Solution:**
```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
export PATH=$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH
```

### Build Errors: "3ds.h: No such file or directory"

**Problem:** libctru not installed

**Solution:**
```bash
sudo dkp-pacman -S libctru
```

### Build Errors: "citro3d.h: No such file or directory"

**Problem:** citro3d not installed

**Solution:**
```bash
sudo dkp-pacman -S citro3d
```

### SNES9x Build Fails with Linker Errors

**Problem:** Outdated devkitARM or libctru

**Solution:**
```bash
# Update all packages
sudo dkp-pacman -Syu

# Rebuild
cd repos/matbo87-snes9x_3ds
make clean
make -j4
```

### "repos/" Directory Not Found

**Problem:** External repos not cloned

**Solution:**
```bash
./scripts/clone-repos.sh
# Or manually clone as shown in step 5
```

---

## Testing on Hardware

### Prerequisites

- **Homebrew-enabled 3DS** (see https://3ds.hacks.guide/)
- **Homebrew Launcher** installed
- **SD Card** with homebrew folder

### Installing to 3DS

1. **Copy .3dsx file to SD card:**
   ```bash
   # Mount SD card, then:
   cp repos/matbo87-snes9x_3ds/output/matbo87-snes9x_3ds.3dsx /path/to/sd/3ds/
   ```

2. **Copy ROMs (optional, for testing):**
   ```bash
   mkdir -p /path/to/sd/3ds/snes9x/roms
   # Copy your legally-owned SNES ROMs to this folder
   ```

3. **Eject SD card, insert in 3DS**

4. **Launch Homebrew Launcher**

5. **Select SNES9x**

### Testing Stereo Effect

1. Load a SNES ROM (e.g., Super Mario World)
2. Adjust 3D slider on 3DS
3. **Currently:** No stereo effect (not yet implemented)
4. **After implementation:** Should see depth-separated layers!

---

## Next Steps

Once your environment is set up:

1. **Read the documentation:**
   - [Implementation Roadmap](docs/IMPLEMENTATION_ROADMAP.md) - Development plan
   - [Stereo Rendering Guide](docs/STEREO_RENDERING_COMPLETE_GUIDE.md) - API reference
   - [SNES9x GPU Architecture](docs/SNES9X_GPU_ARCHITECTURE.md) - Architecture details

2. **Start development:**
   - Follow Week 1 tasks in the Implementation Roadmap
   - Begin with stereo configuration system
   - Test with Super Mario World

3. **Join the community:**
   - Star this repo on GitHub
   - Report issues or ask questions
   - Share your progress!

---

## Additional Resources

### Documentation

- **devkitPro Wiki:** https://devkitpro.org/wiki/Main_Page
- **libctru Documentation:** https://libctru.devkitpro.org/
- **Citro3D Documentation:** https://github.com/devkitPro/citro3d
- **3DS Homebrew Guide:** https://3ds.hacks.guide/

### Community

- **GBATemp (3DS Homebrew):** https://gbatemp.net/categories/nintendo-3ds-homebrew-development-and-emulators.275/
- **devkitPro Discord:** https://discord.gg/dWeHxdc
- **Reddit /r/3dshacks:** https://reddit.com/r/3dshacks

---

**Setup complete!** You're ready to start developing stereoscopic 3D for SNES games! 🎮✨
