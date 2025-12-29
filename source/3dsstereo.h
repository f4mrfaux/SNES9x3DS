#ifndef _3DSSTEREO_H_
#define _3DSSTEREO_H_

#include <3ds.h>
#include <citro3d.h>

//=============================================================================
// 3DS Stereoscopic 3D Rendering Module
//=============================================================================
// This module adds stereoscopic 3D support to SNES9x by rendering
// the SNES output to both left and right eye targets with parallax offset.
//
// Phase 1: Minimal prototype - Render existing output in stereo
// Future: Layer separation (BG1-4, sprites) with configurable depths
//=============================================================================

//-----------------------------------------------------------------------------
// Stereo eye enum
//-----------------------------------------------------------------------------
typedef enum {
    STEREO_EYE_LEFT = 0,
    STEREO_EYE_RIGHT = 1
} StereoEye;

//-----------------------------------------------------------------------------
// Stereo configuration
//-----------------------------------------------------------------------------
typedef struct {
    bool enabled;                   // Enable/disable stereo rendering
} SStereoConfig;

//-----------------------------------------------------------------------------
// Initialization and cleanup
//-----------------------------------------------------------------------------
bool stereo3dsInitialize();
void stereo3dsFinalize();

//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------
void stereo3dsSetEnabled(bool enabled);
bool stereo3dsIsEnabled();
float stereo3dsGetSliderValue();

//-----------------------------------------------------------------------------
// Phase 1: Dual Render Target Management
//-----------------------------------------------------------------------------
bool stereo3dsCreateRenderTargets();
void stereo3dsDestroyRenderTargets();
bool stereo3dsSetActiveRenderTarget(StereoEye eye);  // Returns false if targets don't exist
bool stereo3dsAreTargetsCreated();
bool stereo3dsEnsureTargetsCreated();  // Lazy allocation: create if not exists

//-----------------------------------------------------------------------------
// Phase 2: Layer Tracking Globals
//-----------------------------------------------------------------------------
// Current layer being rendered (-1 = unknown, 0-3 = BG0-3, 4 = sprites)
extern int g_currentLayerIndex;

// Plan E: Per-eye, per-layer horizontal offsets for depth (pixels)
// [eye][layer] where eye: 0=LEFT, 1=RIGHT; layer: 0-3=BG0-3, 4=Sprites
extern float g_stereoLayerOffsets[2][5];

//-----------------------------------------------------------------------------
// Phase 3: Layer Offset Management (Plan E)
//-----------------------------------------------------------------------------
void stereo3dsUpdateLayerOffsetsFromSlider(float sliderValue);

//-----------------------------------------------------------------------------
// Phase 4: Screen Transfer (Plan E)
//-----------------------------------------------------------------------------
bool stereo3dsTransferToScreenBuffers();

//-----------------------------------------------------------------------------
// Logging helper (called from main loop)
//-----------------------------------------------------------------------------
void stereo3dsLogFrameUpdate(float effectiveSlider, bool stereoActive);

#endif // _3DSSTEREO_H_
