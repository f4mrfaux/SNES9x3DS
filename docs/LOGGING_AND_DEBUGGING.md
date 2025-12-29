# SNES9x 3DS Stereo - Logging and Debugging Guide

**Date:** November 25, 2025
**Purpose:** How to gather logs and live-debug the current stereo build (with known artifacts)

---

## Overview

The SNES9x 3DS Stereo build now includes a comprehensive logging system that writes to both:
1. **SD card file** (`sdmc:/snes9x_3ds_stereo.log`) - persistent logs
2. **Bottom screen console** - real-time visual feedback
3. **GDB remote debugging** - live variable inspection

This solves the problem of trying to read verbose output from the secondary screen.

---

## Logging System Features

### Automatic SD Card Logging

All debug output is automatically written to:
```
sdmc:/snes9x_3ds_stereo.log
```

**Location:** Root of your SD card
**Format:** Timestamped entries with log level and tag
**Persistence:** Appends to file across sessions

### Log Levels

- `DEBUG` - Detailed trace information (layer tracking, draw calls)
- `INFO` - General information (initialization, state changes)
- `WARN` - Warning messages (non-critical issues)
- `ERROR` - Error messages (critical problems)

### Log Tags

Logs are tagged by subsystem for easy filtering:
- `[INIT]` - System initialization
- `[STEREO]` - Stereo rendering events
- `[LAYER]` - Per-layer rendering details
- `[DRAW]` - GPU draw call tracking
- `[GPU]` - GPU state and operations

---

## Usage

### Reading Logs After Testing

**On 3DS:**
1. Run SNES9x and test stereo 3D
2. Exit the emulator
3. Remove SD card

**On PC:**
1. Insert SD card
2. Open `snes9x_3ds_stereo.log` in text editor
3. Search for tags like `[STEREO]` or `[ERROR]`

### Example Log Output

```
========================================
SNES9x 3DS Stereo - New Session
========================================
[0.123] [INFO:INIT] SNES9x 3DS Stereo - Starting initialization
[0.125] [INFO:INIT] Version: 1.0.0
[0.256] [INFO:INIT] Sound initialized: 32000 Hz, 256 samples
[1.234] [INFO:STEREO] Initializing stereoscopic 3D system
[2.456] [INFO:STEREO] === DUAL RENDERING STARTED ===
[2.457] [INFO:STEREO] Slider value: 0.85 (0.0=off, 1.0=max)
[2.458] [INFO:STEREO] Transfer mode: Flat stereo (both eyes see same image)
[12.789] [INFO:STEREO] Frame 600: Stereo active (slider=0.85)
```

---

## GDB Remote Debugging

For live debugging with variable inspection and breakpoints.

### Setup on 3DS

1. **Enable Rosalina debugger:**
   - Press: `L + Down + Select`
   - Navigate to: `Debugger options`
   - Select: `Enable debugger`

2. **Enable required services:**
   - Press: `L + Down + Select`
   - Navigate to: `Process list`
   - Enable these services:
     - `3dsx_app` (port 4025) - Your SNES9x app
     - `gsp` (port 4015) - Graphics service
     - `hid` (port 4013) - 3D slider input

3. **Note your 3DS IP address:**
   - Rosalina will show IP when debugger is enabled
   - Usually: `192.168.1.x`

### Connect from PC

**Option 1: Use the automated script**
```bash
cd /home/bob/projects/3ds-hax/3ds-conveersion-project
./connect-gdb.sh
```

This will:
- Connect to your 3DS
- Load symbols from the ELF file
- Set up automatic breakpoints
- Start logging to `snes9x_stereo_debug.log`

**Option 2: Manual connection**
```bash
arm-none-eabi-gdb repos/matbo87-snes9x_3ds/output/matbo87-snes9x_3ds.elf

# In GDB:
(gdb) target remote 192.168.1.2:4025
(gdb) set logging file snes9x_gdb.log
(gdb) set logging on
(gdb) continue
```

### GDB Breakpoints (current)

1. **`impl3dsRunOneFrame()`** – heartbeat every 60 frames (1s): shows render active flag and slider state.
2. **`stereo3dsUpdateLayerOffsetsFromSlider()`** – logs input slider value + LayerDepth/ScreenPlane/Strength config.
3. **`gpu3dsDrawVertexes()`** – first 10 calls: logs stereo enabled flag, current layer, and per-eye offsets.
4. **`stereo3dsTransferToScreenBuffers()`** – confirms per-eye transfer is called and shows texture/screen sizes.

### Manual Variable Inspection

While GDB is paused (press Ctrl+C):

```gdb
# Check stereo state
(gdb) print settings3DS.EnableStereo3D
(gdb) print settings3DS.StereoSliderValue

# Check rendering state
(gdb) print IPPU.RenderThisFrame

# Check layer offsets (Plan E)
(gdb) print g_stereoLayerOffsets[0]   # left eye
(gdb) print g_stereoLayerOffsets[1]   # right eye
(gdb) print g_currentLayerIndex

# Resume execution
(gdb) continue
```

---

## Troubleshooting

### "Log file is empty"

**Cause:** SNES9x crashed before closing log file
**Solution:** Logs are flushed on every write, but check:
```bash
# On PC with SD card:
ls -lh snes9x_3ds_stereo.log
```

### "GDB can't connect"

**Check:**
1. Rosalina debugger is enabled
2. `3dsx_app` service is enabled in process list
3. 3DS IP address is correct in `debug-stereo.gdb` (line 5)
4. 3DS and PC are on same network

**Test connection:**
```bash
ping 192.168.1.2  # Your 3DS IP
```

### "No symbols in GDB"

**Cause:** ELF file not found or not built with debug symbols
**Solution:**
```bash
# Rebuild with debug symbols (already enabled in Makefile)
cd repos/matbo87-snes9x_3ds
make clean
make

# Verify ELF exists
ls -lh output/matbo87-snes9x_3ds.elf
```

### "Bottom screen shows old logs"

**Rosalina console scrolling:**
- D-pad Up/Down: Scroll through console
- B button: Exit console view

---

## What to Log When Debugging Stereo

### Current Issue: Frame Freeze

**Key variables to check:**
```
IPPU.RenderThisFrame          # Should be TRUE (1) when not skipping
settings3DS.EnableStereo3D    # Should be TRUE (1) when stereo enabled
settings3DS.StereoSliderValue # Should be > 0.01 when slider is on
```

**Expected log pattern (working):**
```
[INFO:STEREO] === DUAL RENDERING STARTED ===
[INFO:STEREO] Frame 600: Stereo active (slider=0.85)
[INFO:STEREO] Frame 1200: Stereo active (slider=0.85)
...
```

**Problem log pattern (frame freeze):**
```
[INFO:STEREO] === DUAL RENDERING STARTED ===
[INFO:STEREO] Frame 600: Stereo active (slider=0.85)
# No more frames logged = freeze!
```

### Future: Dual-Render Implementation

When implementing per-layer depth separation, add:

```cpp
LOG_LAYER("Drawing layer %d with offset %.2f", g_currentLayerIndex, offset);
LOG_DRAW("gpu3dsDrawVertexes() - %d vertices for layer %d", count, g_currentLayerIndex);
LOG_GPU("Switched render target to %s eye", eyeName);
```

---

## Log File Management

### Clearing Old Logs

The log file appends across sessions. To clear:

**On 3DS:**
1. Use FBI or another file manager
2. Navigate to `sdmc:/`
3. Delete `snes9x_3ds_stereo.log`

**On PC:**
```bash
# When SD card is mounted
rm /path/to/sd/snes9x_3ds_stereo.log
```

### Log Rotation

Logs include session markers:
```
========================================
SNES9x 3DS Stereo - New Session
========================================
```

Each time you launch SNES9x, a new session is logged. To find the latest session:
```bash
# On PC:
grep -n "New Session" snes9x_3ds_stereo.log
# Shows line numbers of each session start
```

---

## Performance Impact

**SD card logging overhead:**
- ~0.1ms per log entry
- Logs are buffered and flushed immediately
- Negligible impact on 60 FPS target

**GDB remote debugging overhead:**
- Breakpoints pause execution
- `continue` with `silent` commands has minimal impact
- Printing variables adds ~1-5ms per breakpoint hit

**Recommendation:**
- Use SD card logging for normal testing
- Use GDB only when you need to inspect specific variables

---

## Quick Reference

### Enable Logging in Code

```cpp
#include "3dslog.h"

// In impl3dsInitializeCore():
log3dsInit(true);  // Enable SD card logging

// Use logging macros:
LOG_INFO("TAG", "Message with %s", variable);
LOG_STEREO("Slider: %.2f", slider);
LOG_LAYER("Layer %d offset: %.2f", layer, offset);
LOG_DEBUG("TAG", "Detailed trace info");
LOG_ERROR("TAG", "Critical error!");

// On shutdown:
log3dsClose();  // Close log file
```

### Connect GDB

```bash
./connect-gdb.sh
```

### Read Logs

```bash
# View latest session on PC
tail -100 /path/to/sd/snes9x_3ds_stereo.log

# Search for errors
grep ERROR /path/to/sd/snes9x_3ds_stereo.log

# Count stereo frames
grep "Stereo active" /path/to/sd/snes9x_3ds_stereo.log | wc -l
```

---

## Next Steps

1. **Build with logging enabled:**
   ```bash
   cd repos/matbo87-snes9x_3ds
   make clean && make
   ```

2. **Copy to 3DS and test**

3. **Read log file to diagnose frame freeze issue**

4. **Use GDB if you need to inspect variables live**

---

**Last Updated:** November 14, 2025
