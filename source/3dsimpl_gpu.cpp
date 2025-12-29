
#include "snes9x.h"
#include "ppu.h"

#include <3ds.h>
#include "3dsgpu.h"
#include "3dsimpl.h"
#include "3dsimpl_gpu.h"
#include "3dsopt.h"  // For t3dsStartTiming/t3dsEndTiming
#include "3dslog.h"  // For LOG_INFO macro
#include <fstream>
#include <sstream>
#include <ctime>

SGPU3DSExtended GPU3DSExt;

// Stereo clear flag - tracks whether we've cleared stereo targets this frame
static bool g_stereoClearedThisFrame = false;

// Reset stereo clear flag at start of each frame (called after stereo transfer)
void gpu3dsResetStereoClearFlag()
{
    g_stereoClearedThisFrame = false;
}

// Utility: clear color+depth on current render target (simple fullscreen quad)
void gpu3dsClearColorAndDepth(int width, int height)
{
    gpu3dsDisableAlphaBlending();
    gpu3dsSetTextureEnvironmentReplaceColor();
    GPU_SetDepthTestAndWriteMask(true, GPU_ALWAYS, GPU_WRITE_ALL);
    gpu3dsDrawRectangle(0, 0, width, height, 0, 0x000000ff);
    GPU_SetDepthTestAndWriteMask(true, GPU_GEQUAL, GPU_WRITE_ALL);
}

// Stereoscopic 3D layer offset system (Plan E: Per-eye vertex buffers)
// [eye][layer] - Eye 0 = LEFT, Eye 1 = RIGHT, Layers 0-3 = BG0-3, 4 = Sprites
float g_stereoLayerOffsets[2][5] = {
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // LEFT eye offsets
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}   // RIGHT eye offsets
};
int g_currentLayerIndex = 0;  // Which layer we're currently rendering (0-4)

void gpu3dsSetMode7UpdateFrameCountUniform();

void gpu3dsInitializeMode7Vertex(int idx, int x, int y)
{
    int x0 = 0;
    int y0 = 0;

    if (x < 64)
    {
        x0 = x * 8;
        y0 = (y * 2 + 1) * 8;
    }
    else
    {
        x0 = (x - 64) * 8;
        y0 = (y * 2) * 8;
    }

    int x1 = x0 + 8;
    int y1 = y0 + 8;

    if (GPU3DS.isReal3DS)
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, -1};
        //m7vertices[1].Position = (SVector4i){x1, y1, 0, -1};

        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        //m7vertices[1].TexCoord = (STexCoord2i){8, 8};

    }
    else
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx * 6];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, -1};
        m7vertices[1].Position = (SVector4i){x1, y0, 0, -1};
        m7vertices[2].Position = (SVector4i){x0, y1, 0, -1};

        m7vertices[3].Position = (SVector4i){x1, y1, 0, -1};
        m7vertices[4].Position = (SVector4i){x0, y1, 0, -1};
        m7vertices[5].Position = (SVector4i){x1, y0, 0, -1};

        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        m7vertices[1].TexCoord = (STexCoord2i){8, 0};
        m7vertices[2].TexCoord = (STexCoord2i){0, 8};

        m7vertices[3].TexCoord = (STexCoord2i){8, 8};
        m7vertices[4].TexCoord = (STexCoord2i){0, 8};
        m7vertices[5].TexCoord = (STexCoord2i){8, 0};

    }
}

void gpu3dsInitializeMode7VertexForTile0(int idx, int x, int y)
{
    int x0 = x;
    int y0 = y;

    int x1 = x0 + 8;
    int y1 = y0 + 8;

    if (GPU3DS.isReal3DS)
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, 0x3fff};
        //m7vertices[1].Position = (SVector4i){x1, y1, 0, 0};

        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        //m7vertices[1].TexCoord = (STexCoord2i){8, 8};

    }
    else
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx * 6];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, 0x3fff};
        m7vertices[1].Position = (SVector4i){x1, y0, 0, 0x3fff};
        m7vertices[2].Position = (SVector4i){x0, y1, 0, 0x3fff};

        m7vertices[3].Position = (SVector4i){x1, y1, 0, 0x3fff};
        m7vertices[4].Position = (SVector4i){x0, y1, 0, 0x3fff};
        m7vertices[5].Position = (SVector4i){x1, y0, 0, 0x3fff};

        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        m7vertices[1].TexCoord = (STexCoord2i){8, 0};
        m7vertices[2].TexCoord = (STexCoord2i){0, 8};

        m7vertices[3].TexCoord = (STexCoord2i){8, 8};
        m7vertices[4].TexCoord = (STexCoord2i){0, 8};
        m7vertices[5].TexCoord = (STexCoord2i){8, 0};

    }
}

void gpu3dsInitializeMode7Vertexes()
{
    GPU3DSExt.mode7FrameCount = 3;
    gpu3dsSetMode7UpdateFrameCountUniform();
    for (int f = 0; f < 2; f++)
    {
        int idx = 0;
        for (int section = 0; section < 4; section++)
        {
            for (int y = 0; y < 32; y++)
                for (int x = 0; x < 128; x++)
                    gpu3dsInitializeMode7Vertex(idx++, x, y);
        }

        gpu3dsInitializeMode7VertexForTile0(16384, 0, 0);
        gpu3dsInitializeMode7VertexForTile0(16385, 0, 8);
        gpu3dsInitializeMode7VertexForTile0(16386, 8, 0);
        gpu3dsInitializeMode7VertexForTile0(16387, 8, 8);

        gpu3dsSwapVertexListForNextFrame(&GPU3DSExt.mode7TileVertexes);
    }
}


void gpu3dsDrawRectangle(int x0, int y0, int x1, int y1, int depth, u32 color)
{
    gpu3dsAddRectangleVertexes (x0, y0, x1, y1, depth, color);
    gpu3dsDrawVertexList(&GPU3DSExt.rectangleVertexes, GPU_GEOMETRY_PRIM, false, -1, -1);
}


void gpu3dsAddRectangleVertexes(int x0, int y0, int x1, int y1, int depth, u32 color)
{
    if (GPU3DS.isReal3DS)
    {
        SVertexColor *vertices = &((SVertexColor *) GPU3DSExt.rectangleVertexes.List)[GPU3DSExt.rectangleVertexes.Count];

        vertices[0].Position = (SVector4i){x0, y0, depth, 1};
        vertices[1].Position = (SVector4i){x1, y1, depth, 1};

        u32 swappedColor = ((color & 0xff) << 24) | ((color & 0xff00) << 8) | ((color & 0xff0000) >> 8) | ((color & 0xff000000) >> 24);
        vertices[0].Color = swappedColor;
        vertices[1].Color = swappedColor;

        GPU3DSExt.rectangleVertexes.Count += 2;
    }
    else
    {
        SVertexColor *vertices = &((SVertexColor *) GPU3DSExt.rectangleVertexes.List)[GPU3DSExt.rectangleVertexes.Count];

        vertices[0].Position = (SVector4i){x0, y0, depth, 1};
        vertices[1].Position = (SVector4i){x1, y0, depth, 1};
        vertices[2].Position = (SVector4i){x0, y1, depth, 1};
        vertices[3].Position = (SVector4i){x1, y1, depth, 1};
        vertices[4].Position = (SVector4i){x1, y0, depth, 1};
        vertices[5].Position = (SVector4i){x0, y1, depth, 1};

        u32 swappedColor = ((color & 0xff) << 24) | ((color & 0xff00) << 8) | ((color & 0xff0000) >> 8) | ((color & 0xff000000) >> 24);
        vertices[0].Color = swappedColor;
        vertices[1].Color = swappedColor;
        vertices[2].Color = swappedColor;
        vertices[3].Color = swappedColor;
        vertices[4].Color = swappedColor;
        vertices[5].Color = swappedColor;

        GPU3DSExt.rectangleVertexes.Count += 6;
    }
}


void gpu3dsDrawVertexes(bool repeatLastDraw, int storeIndex)
{
    t3dsStartTiming(11, "DrawVertexes");

    // Plan E: Check if stereo is enabled (slider check handled in offset calculation)
    bool stereoEnabled = stereo3dsIsEnabled();

    if (!stereoEnabled)
    {
        t3dsStartTiming(12, "DrawVtx-Mono");
        // Mono path: Draw to single target (unchanged from original)
        gpu3dsDrawVertexList(&GPU3DSExt.quadVertexes, GPU_TRIANGLES, repeatLastDraw, 0, storeIndex);
        gpu3dsDrawVertexList(&GPU3DSExt.tileVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw, 1, storeIndex);
        gpu3dsDrawVertexList(&GPU3DSExt.rectangleVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw, 2, storeIndex);
        t3dsEndTiming(12);
    }
    else
    {
        t3dsStartTiming(13, "DrawVtx-Stereo");
        // Plan E: Stereo path - draw BOTH eyes to separate render targets
        // Safety: Check if targets exist before attempting to render
        if (!stereo3dsAreTargetsCreated()) {
            // Fallback: Targets don't exist, render to main screen (mono mode)
            // This prevents black screen when stereo is enabled but targets failed to create
            gpu3dsDrawVertexList(&GPU3DSExt.quadVertexes, GPU_TRIANGLES, repeatLastDraw, 0, storeIndex);
            gpu3dsDrawVertexList(&GPU3DSExt.tileVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw, 1, storeIndex);
            gpu3dsDrawVertexList(&GPU3DSExt.rectangleVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw, 2, storeIndex);
            return;  // Early return - don't try to render to non-existent stereo targets
        }

        // If slider is effectively zero, render only left eye to save work (matches devkitPro pattern)
        bool rightEyeActive = stereo3dsGetSliderValue() >= 0.01f;

        // DEBUG: Log vertex counts before drawing (first 10 stereo frames only)
        static int stereoDrawCount = 0;
        if (stereoDrawCount < 10) {
            LOG_INFO("STEREO-DBG", ">>> STEREO DRAW #%d: rightEyeActive=%d slider=%.2f",
                     stereoDrawCount, rightEyeActive, stereo3dsGetSliderValue());
            LOG_INFO("STEREO-DBG", "    LEFT:  tileVerts=%d quadVerts=%d rectVerts=%d",
                     GPU3DSExt.stereoTileVertexes[0].Count,
                     GPU3DSExt.stereoQuadVertexes[0].Count,
                     GPU3DSExt.rectangleVertexes.Count);
            LOG_INFO("STEREO-DBG", "    RIGHT: tileVerts=%d quadVerts=%d rectVerts=%d",
                     GPU3DSExt.stereoTileVertexes[1].Count,
                     GPU3DSExt.stereoQuadVertexes[1].Count,
                     GPU3DSExt.rectangleVertexes.Count);
            stereoDrawCount++;
        }

        // CRITICAL FIX: Only clear stereo targets ONCE per frame, not on every PPU layer draw
        // PPU calls gpu3dsDrawVertexes() multiple times (BG0, BG1, sprites, etc)
        // Clearing every time would erase previous layers!

        // Clear BOTH stereo targets before rendering (only on first draw call of frame)
        if (!g_stereoClearedThisFrame) {
            static int clearLogCount = 0;
            if (clearLogCount < 3) {
                LOG_INFO("CLEAR-DBG", "=== CLEARING STEREO TARGETS (frame clear #%d) ===", clearLogCount);
            }

            // Clear LEFT eye target (256x256 to match mono screen target)
            if (stereo3dsSetActiveRenderTarget(STEREO_EYE_LEFT)) {
                gpu3dsDisableAlphaBlending();
                gpu3dsSetTextureEnvironmentReplaceColor();
                gpu3dsClearColorAndDepth(256, 256);
                if (clearLogCount < 3) {
                    LOG_INFO("CLEAR-DBG", "  LEFT eye cleared to BLACK (256x256)");
                }
            }
            // Clear RIGHT eye target (256x256 to match mono screen target)
            if (stereo3dsSetActiveRenderTarget(STEREO_EYE_RIGHT)) {
                gpu3dsDisableAlphaBlending();
                gpu3dsSetTextureEnvironmentReplaceColor();
                gpu3dsClearColorAndDepth(256, 256);
                if (clearLogCount < 3) {
                    LOG_INFO("CLEAR-DBG", "  RIGHT eye cleared to BLACK (256x256)");
                    clearLogCount++;
                }
            }
            g_stereoClearedThisFrame = true;  // Mark as cleared for this frame
        }

        for (int eye = 0; eye < (rightEyeActive ? 2 : 1); ++eye)
        {
            // Switch to this eye's render target (returns false if targets don't exist)
            bool targetSwitchOk = stereo3dsSetActiveRenderTarget(eye == 0 ? STEREO_EYE_LEFT : STEREO_EYE_RIGHT);
            if (!targetSwitchOk) {
                // Critical: Target switch failed - fallback to main screen
                // This should never happen if stereo3dsAreTargetsCreated() returned true, but safety first
                gpu3dsSetRenderTargetToMainScreenTexture();
                // Use mono buffers as fallback (only render once)
                if (eye == 0) {
                    gpu3dsDrawVertexList(&GPU3DSExt.quadVertexes, GPU_TRIANGLES, repeatLastDraw, 0, storeIndex);
                    gpu3dsDrawVertexList(&GPU3DSExt.tileVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw, 1, storeIndex);
                    gpu3dsDrawVertexList(&GPU3DSExt.rectangleVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw, 2, storeIndex);
                }
                break;  // Exit loop - can't render stereo without targets
            }

            // Restore rendering state (targets already cleared above)
            gpu3dsEnableAlphaBlending();
            gpu3dsEnableDepthTest();

            // DEBUG: Log first 5 draw calls per eye
            static int drawLogCountLeft = 0, drawLogCountRight = 0;
            if ((eye == 0 && drawLogCountLeft < 5) || (eye == 1 && drawLogCountRight < 5)) {
                LOG_INFO("DRAW-DBG", ">>> DRAW eye=%s quads=%d tiles=%d rects=%d repeatLast=%d",
                         eye == 0 ? "LEFT " : "RIGHT",
                         GPU3DSExt.stereoQuadVertexes[eye].Count,
                         GPU3DSExt.stereoTileVertexes[eye].Count,
                         GPU3DSExt.rectangleVertexes.Count,
                         repeatLastDraw);
                if (eye == 0) drawLogCountLeft++; else drawLogCountRight++;
            }

            // Draw this eye's geometry (with per-layer depth offsets already baked into vertices)
            gpu3dsDrawVertexList(&GPU3DSExt.stereoQuadVertexes[eye], GPU_TRIANGLES, repeatLastDraw, 0, storeIndex);
            gpu3dsDrawVertexList(&GPU3DSExt.stereoTileVertexes[eye], GPU_GEOMETRY_PRIM, repeatLastDraw, 1, storeIndex);

            // Rectangles (UI elements) stay at screen depth - draw from mono buffer for both eyes
            // This ensures UI doesn't have parallax and remains comfortable to view
            gpu3dsDrawVertexList(&GPU3DSExt.rectangleVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw, 2, storeIndex);

        }

#ifdef DEBUG_STEREO_VERTEX_COUNTS
        // Debug: Log vertex counts for both eyes
        printf("[STEREO-DRAW] L_tiles=%d L_quads=%d | R_tiles=%d R_quads=%d\n",
               GPU3DSExt.stereoTileVertexes[0].Count,
               GPU3DSExt.stereoQuadVertexes[0].Count,
               GPU3DSExt.stereoTileVertexes[1].Count,
               GPU3DSExt.stereoQuadVertexes[1].Count);
#endif
        t3dsEndTiming(13);
    }

    t3dsEndTiming(11);
}


void gpu3dsDrawMode7Vertexes(int fromIndex, int tileCount)
{
    if (GPU3DS.isReal3DS)
        gpu3dsDrawVertexList(&GPU3DSExt.mode7TileVertexes, GPU_GEOMETRY_PRIM, fromIndex, tileCount);
    else
        gpu3dsDrawVertexList(&GPU3DSExt.mode7TileVertexes, GPU_TRIANGLES, fromIndex, tileCount);

}

void gpu3dsDrawMode7LineVertexes(bool repeatLastDraw, int storeIndex)
{
    bool stereoEnabled = stereo3dsIsEnabled();
    bool targetsReady = stereoEnabled ? stereo3dsAreTargetsCreated() : false;

    if (stereoEnabled && targetsReady)
    {
        for (int eye = 0; eye < 2; ++eye)
        {
            stereo3dsSetActiveRenderTarget(eye == 0 ? STEREO_EYE_LEFT : STEREO_EYE_RIGHT);
            if (GPU3DS.isReal3DS)
                gpu3dsDrawVertexList(&GPU3DSExt.stereoMode7LineVertexes[eye], GPU_GEOMETRY_PRIM, repeatLastDraw, 3, storeIndex);
            else
                gpu3dsDrawVertexList(&GPU3DSExt.stereoMode7LineVertexes[eye], GPU_TRIANGLES, repeatLastDraw, 3, storeIndex);
        }
    }
    else
    {
        if (GPU3DS.isReal3DS)
            gpu3dsDrawVertexList(&GPU3DSExt.mode7LineVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw, 3, storeIndex);
        else
            gpu3dsDrawVertexList(&GPU3DSExt.mode7LineVertexes, GPU_TRIANGLES, repeatLastDraw, 3, storeIndex);
    }
}


// Changes the texture pixel format (but it must be the same 
// size as the original pixel format). No errors will be thrown
// if the format is incorrect.
//
void gpu3dsSetMode7TexturesPixelFormatToRGB5551()
{
	snesMode7FullTexture->PixelFormat = GPU_RGBA5551;
    snesMode7Tile0Texture->PixelFormat = GPU_RGBA5551;
    snesMode7TileCacheTexture->PixelFormat = GPU_RGBA5551;
}

void gpu3dsSetMode7TexturesPixelFormatToRGB4444()
{
	snesMode7FullTexture->PixelFormat = GPU_RGBA4;
    snesMode7Tile0Texture->PixelFormat = GPU_RGBA4;
    snesMode7TileCacheTexture->PixelFormat = GPU_RGBA4;
}

void gpu3dsSetRenderTargetToMainScreenTexture()
{
    gpu3dsSetRenderTargetToTexture(snesMainScreenTarget, snesDepthForScreens);
}

void gpu3dsSetRenderTargetToSubScreenTexture()
{
    gpu3dsSetRenderTargetToTexture(snesSubScreenTarget, snesDepthForScreens);
}

void gpu3dsSetRenderTargetToDepthTexture()
{
    gpu3dsSetRenderTargetToTexture(snesDepthForScreens, snesDepthForOtherTextures);
}

void gpu3dsSetRenderTargetToMode7FullTexture(int pixelOffset, int width, int height)
{
    gpu3dsSetRenderTargetToTextureSpecific(snesMode7FullTexture, snesDepthForOtherTextures,
        pixelOffset * gpu3dsGetPixelSize(snesMode7FullTexture->PixelFormat), width, height);
}

void gpu3dsSetRenderTargetToMode7Tile0Texture()
{
    gpu3dsSetRenderTargetToTexture(snesMode7Tile0Texture, snesDepthForOtherTextures);
}

void gpu3dsSetMode7UpdateFrameCountUniform()
{
    int updateFrame = GPU3DSExt.mode7FrameCount;
    GPU3DSExt.mode7UpdateFrameCount[0] = ((float)updateFrame) - 0.5f;      // set 'w' to updateFrame

    GPU_SetFloatUniform(GPU_VERTEX_SHADER, 5, (u32 *)GPU3DSExt.mode7UpdateFrameCount, 1);
    GPU_SetFloatUniform(GPU_GEOMETRY_SHADER, 15, (u32 *)GPU3DSExt.mode7UpdateFrameCount, 1);
}


void gpu3dsCopyVRAMTilesIntoMode7TileVertexes(uint8 *VRAM)
{
    for (int i = 0; i < 16384; i++)
    {
        gpu3dsSetMode7TileTexturePos(i, VRAM[i * 2]);
        gpu3dsSetMode7TileModifiedFlag(i);
    }
    IPPU.Mode7CharDirtyFlagCount = 1;
    for (int i = 0; i < 256; i++)
    {
        IPPU.Mode7CharDirtyFlag[i] = 2;
    }
}

void gpu3dsIncrementMode7UpdateFrameCount()
{
    gpu3dsSwapVertexListForNextFrame(&GPU3DSExt.mode7TileVertexes);
    GPU3DSExt.mode7FrameCount ++;

    if (GPU3DSExt.mode7FrameCount == 0x3fff)
    {
        GPU3DSExt.mode7FrameCount = 1;
    }

    // Bug fix: Clears the updateFrameCount of both sets
    // of mode7TileVertexes!
    //
    if (GPU3DSExt.mode7FrameCount <= 2)
    {
        for (int i = 0; i < 16384; )
        {
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);

            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
        }
    }

    gpu3dsSetMode7UpdateFrameCountUniform();
}


void gpu3dsBindTextureDepthForScreens(GPU_TEXUNIT unit)
{
    gpu3dsBindTexture(snesDepthForScreens, unit);
}


void gpu3dsBindTextureSnesMode7TileCache(GPU_TEXUNIT unit)
{
    gpu3dsBindTexture(snesMode7TileCacheTexture, unit);
}

void gpu3dsBindTextureSnesMode7Tile0CacheRepeat(GPU_TEXUNIT unit)
{
    gpu3dsBindTextureWithParams(snesMode7Tile0Texture, unit,
        GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_WRAP_S(GPU_REPEAT)
        | GPU_TEXTURE_WRAP_T(GPU_REPEAT));
}

void gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT unit)
{
    gpu3dsBindTextureWithParams(snesMode7FullTexture, unit,
        GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_BORDER)
        | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_BORDER));
}

void gpu3dsBindTextureSnesMode7FullRepeat(GPU_TEXUNIT unit)
{
    gpu3dsBindTextureWithParams(snesMode7FullTexture, unit,
        GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_WRAP_S(GPU_REPEAT)
        | GPU_TEXTURE_WRAP_T(GPU_REPEAT));
}


void gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT unit)
{
    gpu3dsBindTexture(snesTileCacheTexture, unit);
}

void gpu3dsBindTextureSnesTileCacheForHires(GPU_TEXUNIT unit)
{
    gpu3dsBindTextureWithParams(snesTileCacheTexture, unit,
	    GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
		| GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_BORDER)
		| GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_BORDER)
    );
}

void gpu3dsBindTextureMainScreen(GPU_TEXUNIT unit)
{
    GPU_TEXTURE_FILTER_PARAM filter = settings3DS.ScreenStretch == 0 ? GPU_NEAREST : GPU_TEXTURE_FILTER_PARAM(settings3DS.ScreenFilter);

    gpu3dsBindTextureWithParams(snesMainScreenTarget, unit,        
        GPU_TEXTURE_MAG_FILTER(filter)
        | GPU_TEXTURE_MIN_FILTER(filter)
        | GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_BORDER)
        | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_BORDER));
}

void gpu3dsBindTextureSubScreen(GPU_TEXUNIT unit)
{
    gpu3dsBindTexture(snesSubScreenTarget, unit);
}
