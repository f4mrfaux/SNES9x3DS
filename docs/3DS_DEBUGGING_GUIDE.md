# 3DS XL Debugging Guide for SNES9x 3DS

## Overview

This guide explains how to capture debug logs from SNES9x running on your 3DS XL hardware. There are multiple methods depending on your setup.

---

## Method 1: On-Screen Console Output (Easiest)

SNES9x 3DS already has console output enabled on the bottom screen.

### How It Works

The code already initializes a console on the secondary screen:
```cpp
// source/3dsgpu.cpp:461
consoleInit(screenSettings.SecondScreen, NULL);
```

All `printf()` statements in the code will appear on the **bottom screen** while the game runs on the top screen.

### Current Debug Output

When you enable 3D mode, you should see messages like:

**On first frame with 3D enabled:**
```
[MAIN-RENDER] First frame: Taking STEREO path (enabled=1, slider=0.XX)
[DUAL-RENDER] === DUAL RENDERING ENABLED ===
[DUAL-RENDER] Slider: 0.XX (0.0=off, 1.0=max)
[DUAL-RENDER] Layer depths: BG0=X.X, BG1=X.X, BG2=X.X, BG3=X.X, Sprites=X.X
[DUAL-RENDER] LEFT eye render to offscreen texture complete
[DUAL-RENDER] LEFT eye framebuffer transfer complete
[DUAL-RENDER] RIGHT eye render to offscreen texture complete
[DUAL-RENDER] RIGHT eye framebuffer transfer complete
[DUAL-RENDER] === PER-LAYER 3D DEPTH SEPARATION ACTIVE ===
```

**Every 600 frames (10 seconds at 60fps):**
```
[MAIN-RENDER] Frame 600: Using STEREO rendering (slider=0.XX)
[DUAL-RENDER] Frame 600: Per-layer stereo active (slider=0.XX)
```

### What to Look For

**If you see:**
- `[MAIN-RENDER] First frame: Taking STEREO path` → Code is detecting slider correctly
- `[DUAL-RENDER] === DUAL RENDERING ENABLED ===` → `stereo3dsRenderSNESFrame()` is being called
- Both LEFT and RIGHT eye messages → Dual rendering is executing

**If you DON'T see:**
- No `[DUAL-RENDER]` messages → Old broken function `stereo3dsRenderSNESTexture()` is still being called
- Only `[MAIN-RENDER]` messages → Stereo path is not being taken (slider may be 0)

---

## Method 2: Luma3DS Rosalina Debugger (Advanced)

If you have Luma3DS custom firmware installed, you can use the Rosalina debugger for advanced debugging.

### Prerequisites

- Luma3DS CFW installed on your 3DS XL
- PC on the same network as your 3DS
- GDB installed on your PC: `pacman -S arm-none-eabi-gdb` (Arch Linux)

### Step 1: Enable Debugger on 3DS

1. While SNES9x is running, press **L + Down + Select** to open Rosalina menu
2. Navigate to **"Debugger options..."**
3. Select **"Enable debugger"**
4. Note the IP address and port shown (usually `<3DS-IP>:4003`)
5. Press **B** to close menu and return to game

**WARNING:** You must disable the debugger when finished, or your 3DS will not cleanly shutdown/reboot.

### Step 2: Connect GDB from PC

On your PC:
```bash
arm-none-eabi-gdb

# Inside GDB:
target remote <3DS-IP>:4003

# Set breakpoint example:
break stereo3dsRenderSNESFrame

# Continue execution:
continue

# When breakpoint hits, inspect variables:
print settings3DS.StereoSliderValue
print g_stereoLayerOffsets[0]

# Step through code:
step
next
```

### Step 3: Disconnect and Disable

In GDB:
```
quit
```

On 3DS:
1. Press **L + Down + Select** for Rosalina menu
2. Navigate to **"Debugger options..."**
3. Select **"Disable debugger"**

---

## Method 3: svcOutputDebugString (System-Level Logging)

Luma3DS allows `svcOutputDebugString` syscalls to work even on retail units.

### How to Add

In your code:
```cpp
#include <3ds.h>

void debugLog(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    svcOutputDebugString(buffer, strlen(buffer));
}

// Usage:
debugLog("[STEREO] Slider value: %.2f\n", settings3DS.StereoSliderValue);
```

### Capture Logs

These logs can be captured via:
- Luma3DS exception dumps (if crash occurs)
- GDB session (logs appear in debugger output)
- Custom log viewers (if using homebrew logging tools)

**Note:** This is more complex than on-screen `printf()` and usually not needed unless debugging crashes.

---

## Method 4: SD Card Log File (Custom Implementation)

You can add file logging to write to SD card.

### Implementation

```cpp
#include <stdio.h>

FILE* logFile = NULL;

void initLogging() {
    logFile = fopen("sdmc:/snes9x_3ds_debug.log", "w");
    if (logFile) {
        fprintf(logFile, "=== SNES9x 3DS Debug Log ===\n");
        fflush(logFile);
    }
}

void logToFile(const char* format, ...) {
    if (!logFile) return;

    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);
    fflush(logFile);  // Important: flush immediately
}

void closeLogging() {
    if (logFile) {
        fclose(logFile);
        logFile = NULL;
    }
}

// In main():
initLogging();

// In stereo3dsRenderSNESFrame():
logToFile("[DUAL-RENDER] Frame rendered with slider=%.2f\n", slider);

// On exit:
closeLogging();
```

### Retrieve Logs

1. Exit SNES9x
2. Power off 3DS
3. Remove SD card
4. Mount on PC
5. Read `snes9x_3ds_debug.log`

**Warning:** Frequent file writes can slow down emulation. Use sparingly or add frame interval checks.

---

## Recommended Debug Workflow

### For Hardware Testing Issues (Frame Freeze, No 3D Effect, etc.)

**Best approach: Use on-screen console (Method 1)**

1. Build the version you want to test
2. Install on 3DS XL
3. Launch SNES9x with a game loaded
4. Look at **bottom screen** (console output)
5. Enable 3D slider
6. Check if you see `[DUAL-RENDER]` messages
7. Take a photo of the bottom screen with your phone if needed

### What the Logs Will Tell You

**Scenario A: Frame freeze persists**
- If you see `[MAIN-RENDER] Taking STEREO path` but NO `[DUAL-RENDER]` messages:
  - Fix was not applied or old binary is running
  - Check build/install process

**Scenario B: Frame updates but no 3D effect**
- If you see both `[MAIN-RENDER]` and `[DUAL-RENDER]` messages:
  - Dual rendering is working
  - Problem is in the offset application (`g_stereoLayerOffsets[]`)
  - Need to verify vertex shader is applying offsets

**Scenario C: Crash when enabling 3D**
- If 3DS crashes/hangs when slider moves:
  - Use Luma3DS debugger (Method 2) to get crash dump

- Stereo alloc safety:
  - `gpu3dsCreateTextureInVRAM` now null-checks allocations; if a stereo target fails to allocate, we stay in mono instead of crashing.
  - Stereo targets are auto-freed after ~5s with the slider off to keep VRAM available; they reallocate on-demand when the slider is raised.
  - If stereo transfer fails (targets missing), the renderer automatically falls back to the mono transfer path to avoid a blank screen.
  - Check for null pointers or invalid memory access

---

## Performance Monitoring

### Frame Time Logging

The code already has timing instrumentation:
```cpp
t3dsEndTiming(5);  // End timing measurement
```

To see frame times, look for timing output on console (if enabled in build).

### Expected Performance

- **Mono (slider=0):** 60 FPS (16.67ms/frame)
- **Stereo (slider>0):** 50-60 FPS (~18ms/frame with dual rendering)

If frame time exceeds 33ms, you'll see stuttering.

---

## Troubleshooting Common Issues

### "I don't see any console output on bottom screen"

**Check:**
1. Is `consoleInit()` being called? (should be in `source/3dsgpu.cpp:461`)
2. Is the game using the top screen for gameplay? (console should be on bottom)
3. Is there a setting to hide/show console? (check in-game menu)

**Solution:**
Add this to force console visibility:
```cpp
// In 3dsmain.cpp or 3dsgpu.cpp
consoleSelect(&bottomScreen);  // Make console active
printf("DEBUG: Console output test\n");
```

### "Rosalina menu won't open"

**Possible causes:**
- Wrong button combo (ensure L + Down + Select pressed simultaneously)
- Luma3DS not installed or outdated
- Button combo disabled in Luma config

**Solution:**
Hold SELECT during boot → Luma configuration → Enable Rosalina

### "GDB connection refused"

**Check:**
1. Are 3DS and PC on same network?
2. Can you ping the 3DS IP? `ping <3DS-IP>`
3. Is port 4003 open? (no firewall blocking)
4. Did you enable debugger in Rosalina first?

---

## Quick Reference: Log Message Meanings

| Log Message | Meaning |
|-------------|---------|
| `[MAIN-RENDER] Taking STEREO path` | Main render loop detected slider > 0 |
| `[MAIN-RENDER] Taking MONO path` | Slider at 0, using normal rendering |
| `[DUAL-RENDER] === DUAL RENDERING ENABLED ===` | `stereo3dsRenderSNESFrame()` called successfully |
| `[DUAL-RENDER] LEFT eye render complete` | Left eye rendered to offscreen texture |
| `[DUAL-RENDER] LEFT eye framebuffer transfer complete` | Left eye copied to GFX_LEFT buffer |
| `[DUAL-RENDER] RIGHT eye render complete` | Right eye rendered to offscreen texture |
| `[DUAL-RENDER] RIGHT eye framebuffer transfer complete` | Right eye copied to GFX_RIGHT buffer |
| `[DUAL-RENDER] PER-LAYER 3D DEPTH SEPARATION ACTIVE` | Full stereo pipeline completed |
| `[DUAL-RENDER] Frame XXX: Per-layer stereo active` | Periodic confirmation (every 10 seconds) |

---

## Next Steps After Getting Logs

Once you have debug output visible:

1. **Verify the fix worked:**
   - Confirm `[DUAL-RENDER]` messages appear when slider > 0
   - If not, double-check `3dsimpl.cpp:654` has `stereo3dsRenderSNESFrame()` call

2. **Test 3D effect:**
   - If frames update but no 3D: Check offset application in vertex functions
   - Look at `g_stereoLayerOffsets[]` values in logs
   - Verify `gpu3dsAddTileVertexes()` uses offsets

3. **Test performance:**
   - Check if frame rate drops below 50 FPS
   - If too slow: May need to optimize or reduce layer count

4. **Test different games:**
   - Mode 7 games (F-Zero, Super Mario Kart)
   - Layer-heavy games (Super Mario World, Yoshi's Island)
   - Simple games (Tetris Attack, Dr. Mario)

---

## Files Modified for Logging

All debug logging is already in place:

| File | Lines | Purpose |
|------|-------|---------|
| `source/3dsimpl.cpp` | 646-662 | Main render path selection logging |
| `source/3dsstereo.cpp` | 327-343 | Dual render initialization logging |
| `source/3dsstereo.cpp` | 390-470 | Per-eye render completion logging |

No additional changes needed unless you want more verbose output.

---

## Additional Resources

- **Luma3DS Documentation:** https://github.com/LumaTeam/Luma3DS/wiki
- **3DS Homebrew Development:** https://www.3dbrew.org/
- **devkitPro Documentation:** https://devkitpro.org/wiki/devkitARM
- **GDB Debugging Guide:** https://www.3dbrew.org/wiki/Debugging

---

**Last Updated:** November 13, 2025
**Project:** SNES9x 3DS Stereoscopic 3D Layer Separation
