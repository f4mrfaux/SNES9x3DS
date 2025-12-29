# Stereo Status – Dec 22, 2025

**Context:** Stereo Plan E code is merged and builds; first hardware push shows heavy artifacting. Below is the current code reality (ignoring older “complete” docs) and the live-debug plan.

## What’s in the code now
- Dual eye render targets created in `3dsstereo.cpp`; transfers happen via `stereo3dsTransferToScreenBuffers()` every frame when stereo is active.
- Per-layer offsets applied in `gpu3dsAddTileVertexes()` (see `3dsimpl_gpu.h`): uses `g_stereoLayerOffsets` and `g_currentLayerIndex` (set in `Snes9x/gfxhw.cpp` for BG0-3 and sprites).
- Slider handling in `impl3dsRunOneFrame()`: effective depth = hardware slider × `StereoSliderValue` × `StereoDepthStrength`; enables stereo if effective ≥ 0.01.
- UI rectangles stay mono (no parallax). Mode7 now uses per-eye offsets (treated as BG0) with optional gradient (`Mode7DepthNear/Far`, `Mode7UseGradient`); no per-sprite depth; geometry shader path used where available.
- Logging to SD: `sdmc:/snes9x_3ds_stereo.log` via `log3dsInit(true)` (falls back to printf if file fails).
- Depth format: DEPTH24_STENCIL8 via `GPU_SetViewport`; explicit color+depth clears per eye, main, and sub each frame; right eye skipped when slider ≈ 0.

## Known gaps / risks
- Stereo settings were not persisted or exposed in UI (fixed: global/game cfg + menu). Defaults softened (depth strength/slider/layer depths).
- `stereo3dsBeginFrame/EndFrame()` are unused, so their counters/logging never run.
- GDB script previously pointed to a non-existent `stereo3dsRenderSNESFrame`; fixed now, but IP must match your console.
- Artifacting previously observed; depth format now correct and depth clears added—needs hardware validation.

## Live debugging plan (hardware)
1) **Deploy build:** copy `repos/matbo87-snes9x_3ds/output/matbo87-snes9x_3ds.3dsx` (and `.smdh` if desired) to SD and run.
2) **Attach GDB:** update IP in `debug-stereo.gdb` (default 192.168.1.2) and run `./connect-gdb.sh` or `arm-none-eabi-gdb -x debug-stereo.gdb`.
3) **Watch breakpoints:** script now hits:
   - `impl3dsRunOneFrame` (1/sec heartbeat with slider/effective depth)
   - `stereo3dsUpdateLayerOffsetsFromSlider` (per-layer offsets/plane)
   - `gpu3dsDrawVertexes` (first 10 calls: current layer + per-eye offsets)
   - `stereo3dsTransferToScreenBuffers` (per-eye copy into framebuffers)
4) **Inspect state on pause (Ctrl+C):**
   - `p settings3DS.EnableStereo3D`, `p osGet3DSliderState()`, `p settings3DS.StereoSliderValue`
   - `p g_stereoLayerOffsets[0]`, `p g_stereoLayerOffsets[1]`, `p g_currentLayerIndex`
   - `p stereoLeftEyeTarget->Width`, `p stereoLeftEyeTarget->Height`, `p screenSettings.GameScreenWidth`
5) **Collect logs:** grab `sdmc:/snes9x_3ds_stereo.log` after a run to cross-check with GDB output.

## Quick hypotheses to poke first
- Per-eye targets not bound when drawing: confirm `gpu3dsDrawVertexes` is called twice and `stereo3dsSetActiveRenderTarget` succeeded (right eye skipped when slider≈0).
- Offsets too large or wrong sign: inspect `g_stereoLayerOffsets` values vs configured `LayerDepth`/`ScreenPlaneLayer`.
- Transfer mismatch: ensure `stereo3dsTransferToScreenBuffers` uses the expected dimensions and isn’t racing with previous flush.
- Slider/effective depth mis-scaling: check `effective` vs on-screen parallax; try forcing `StereoDepthStrength` to 0.5 in code for a sanity run.

## Next steps (post-Dec 22 changes)
- Hardware validation: confirm artifact-free stereo with depth clears; verify slider → parallax response and layer offsets.
- (Optional) Provide a clearer depth-buffer creation helper name to avoid confusion over RGBA8-allocated depth storage; size stays 4 B/px for DEPTH24_STENCIL8.
- Mode7: test gradient settings on real games (e.g., Star Fox, Yoshi’s Island, CV IV Mode7 rooms); tune defaults if needed; per-sprite depth still not implemented.
