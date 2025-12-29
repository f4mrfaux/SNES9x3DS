#include "3dssnes9x.h"
#include "3dsgpu.h"
#include "3dsstereo.h"  // Plan E: For SStereoConfig
#include "3dssettings.h"
#include <fstream>
#include <sstream>
#include <ctime>

#ifndef _3DSIMPL_GPU_H_
#define _3DSIMPL_GPU_H_

#define COMPOSE_HASH(vramAddr, pal)   ((vramAddr) << 4) + ((pal) & 0xf)

// Stereoscopic 3D layer offset system (defined in 3dsimpl_gpu.cpp)
// Plan E: Per-eye offsets [eye][layer]
// Eye 0 = LEFT, Eye 1 = RIGHT
// Layers: 0-3 = BG0-3, 4 = Sprites
extern float g_stereoLayerOffsets[2][5];  // [eye][layer] pixel offsets
extern int g_currentLayerIndex;           // Which layer we're currently rendering (0-4)

struct SVector3i
{
    int16 x, y, z;
};

struct SVector4i
{
    int16 x, y, z, w;
};


struct SVector3f
{
    float x, y, z;
};

struct STexCoord2i
{
    int16 u, v;
};

struct STexCoord2f
{
    float u, v;
};


struct STileVertex {
    struct SVector3i    Position;
	struct STexCoord2i  TexCoord;
};


struct SMode7TileVertex {
    struct SVector4i    Position;
	struct STexCoord2i  TexCoord;
};

struct SMode7LineVertex {
    struct SVector4i    Position;
	struct STexCoord2f  TexCoord;
};


struct SVertexColor {
    struct SVector4i    Position;
	u32                 Color;
};


#define STILEVERTEX_ATTRIBFORMAT        GPU_ATTRIBFMT (0, 3, GPU_SHORT) | GPU_ATTRIBFMT (1, 2, GPU_SHORT)
#define SMODE7TILEVERTEX_ATTRIBFORMAT    GPU_ATTRIBFMT (0, 4, GPU_SHORT) | GPU_ATTRIBFMT (1, 2, GPU_SHORT)
#define SMODE7LINEVERTEX_ATTRIBFORMAT    GPU_ATTRIBFMT (0, 4, GPU_SHORT) | GPU_ATTRIBFMT (1, 2, GPU_FLOAT)
#define SVERTEXCOLOR_ATTRIBFORMAT       GPU_ATTRIBFMT(0, 4, GPU_SHORT) | GPU_ATTRIBFMT(1, 4, GPU_UNSIGNED_BYTE)


#define MAX_TEXTURE_POSITIONS		16383
#define MAX_HASH					(65536 * 16 / 8)


typedef struct
{
    // Mono/shared vertex buffers (used when stereo is disabled)
    SVertexList         quadVertexes;
    SVertexList         tileVertexes;
    SVertexList         mode7TileVertexes;
    SVertexList         mode7LineVertexes;
    SVertexList         rectangleVertexes;

    // Plan E: Per-eye vertex buffers for stereoscopic 3D
    // [0] = LEFT eye, [1] = RIGHT eye
    // Note: Rectangles use mono buffer (rectangleVertexes) for both eyes (UI stays flat)
    SVertexList         stereoQuadVertexes[2];
    SVertexList         stereoTileVertexes[2];
    SVertexList         stereoMode7LineVertexes[2];

    int                 mode7FrameCount = 0;
    float               mode7UpdateFrameCount[4];

    // Memory Usage = 0.25 MB (for hashing of the texture position)
    uint16  vramCacheHashToTexturePosition[MAX_HASH + 1];

    // Memory Usage = 0.06 MB
    int     vramCacheTexturePositionToHash[MAX_TEXTURE_POSITIONS];

    int     newCacheTexturePosition = 2;
} SGPU3DSExtended;

extern SGPU3DSExtended GPU3DSExt;


void gpu3dsSetRenderTargetToMainScreenTexture();
void gpu3dsSetRenderTargetToSubScreenTexture();
void gpu3dsSetRenderTargetToDepthTexture();
void gpu3dsSetRenderTargetToMode7FullTexture(int pixelOffset, int width, int height);
void gpu3dsSetRenderTargetToMode7Tile0Texture();

void gpu3dsSetMode7TexturesPixelFormatToRGB5551();
void gpu3dsSetMode7TexturesPixelFormatToRGB4444();

void gpu3dsBindTextureDepthForScreens(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesTileCacheForHires(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesMode7TileCache(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesMode7Tile0CacheRepeat(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesMode7FullRepeat(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT unit);
void gpu3dsBindTextureSubScreen(GPU_TEXUNIT unit);
void gpu3dsBindTextureMainScreen(GPU_TEXUNIT unit);

void gpu3dsInitializeMode7Vertexes();
void gpu3dsSetMode7UpdateFrameCountUniform();

inline void __attribute__((always_inline)) gpu3dsAddQuadVertexes(
    int x0, int y0, int x1, int y1,
    int tx0, int ty0, int tx1, int ty1,
    int data)
{
    STileVertex *vertices = &((STileVertex *) GPU3DSExt.quadVertexes.List)[GPU3DSExt.quadVertexes.Count];

	vertices[0].Position = (SVector3i){x0, y0, data};
	vertices[1].Position = (SVector3i){x1, y0, data};
	vertices[2].Position = (SVector3i){x0, y1, data};

	vertices[3].Position = (SVector3i){x1, y1, data};
	vertices[4].Position = (SVector3i){x0, y1, data};
	vertices[5].Position = (SVector3i){x1, y0, data};

	vertices[0].TexCoord = (STexCoord2i){tx0, ty0};
	vertices[1].TexCoord = (STexCoord2i){tx1, ty0};
	vertices[2].TexCoord = (STexCoord2i){tx0, ty1};

	vertices[3].TexCoord = (STexCoord2i){tx1, ty1};
	vertices[4].TexCoord = (STexCoord2i){tx0, ty1};
	vertices[5].TexCoord = (STexCoord2i){tx1, ty0};

    //GPU3DSExt.vertexCount += 6;
    GPU3DSExt.quadVertexes.Count += 6;
}


inline void __attribute__((always_inline)) gpu3dsAddTileVertexes(
    int x0, int y0, int x1, int y1,
    int tx0, int ty0, int tx1, int ty1,
    int data)
{
    // Plan E: Generate vertices for BOTH eyes if stereo is enabled
    // Slider check is handled in offset calculation, so we only need to check if stereo is enabled
    bool stereoEnabled = stereo3dsIsEnabled();

    if (stereoEnabled) {
        // Plan E: Emit to per-eye buffers
        static int vertexGenCounter = 0;
        vertexGenCounter++;
        for (int eye = 0; eye < 2; ++eye) {
            // Apply per-eye, per-layer offset
            int offset = (int)g_stereoLayerOffsets[eye][g_currentLayerIndex];
            int ex0 = x0 + offset;
            int ex1 = x1 + offset;

#ifndef RELEASE
            if (GPU3DS.isReal3DS)
            {
#endif
                STileVertex *vertices = &((STileVertex *) GPU3DSExt.stereoTileVertexes[eye].List)[GPU3DSExt.stereoTileVertexes[eye].Count];

                vertices[0].Position = (SVector3i){ex0, y0, data};
                vertices[0].TexCoord = (STexCoord2i){tx0, ty0};

                vertices[1].Position = (SVector3i){ex1, y1, data};
                vertices[1].TexCoord = (STexCoord2i){tx1, ty1};

                GPU3DSExt.stereoTileVertexes[eye].Count += 2;

#ifndef RELEASE
            }
            else
            {
                STileVertex *vertices = &((STileVertex *) GPU3DSExt.stereoQuadVertexes[eye].List)[GPU3DSExt.stereoQuadVertexes[eye].Count];

                vertices[0].Position = (SVector3i){ex0, y0, data};
                vertices[1].Position = (SVector3i){ex1, y0, data};
                vertices[2].Position = (SVector3i){ex0, y1, data};

                vertices[3].Position = (SVector3i){ex1, y1, data};
                vertices[4].Position = (SVector3i){ex0, y1, data};
                vertices[5].Position = (SVector3i){ex1, y0, data};

                vertices[0].TexCoord = (STexCoord2i){tx0, ty0};
                vertices[1].TexCoord = (STexCoord2i){tx1, ty0};
                vertices[2].TexCoord = (STexCoord2i){tx0, ty1};

                vertices[3].TexCoord = (STexCoord2i){tx1, ty1};
                vertices[4].TexCoord = (STexCoord2i){tx0, ty1};
                vertices[5].TexCoord = (STexCoord2i){tx1, ty0};

                GPU3DSExt.stereoQuadVertexes[eye].Count += 6;
            }
#endif
        }
    } else {
        // Mono mode: Use original mono buffers
#ifndef RELEASE
        if (GPU3DS.isReal3DS)
        {
#endif
            STileVertex *vertices = &((STileVertex *) GPU3DSExt.tileVertexes.List)[GPU3DSExt.tileVertexes.Count];

            vertices[0].Position = (SVector3i){x0, y0, data};
            vertices[0].TexCoord = (STexCoord2i){tx0, ty0};

            vertices[1].Position = (SVector3i){x1, y1, data};
            vertices[1].TexCoord = (STexCoord2i){tx1, ty1};

            GPU3DSExt.tileVertexes.Count += 2;

#ifndef RELEASE
        }
        else
        {
            STileVertex *vertices = &((STileVertex *) GPU3DSExt.quadVertexes.List)[GPU3DSExt.quadVertexes.Count];

            vertices[0].Position = (SVector3i){x0, y0, data};
            vertices[1].Position = (SVector3i){x1, y0, data};
            vertices[2].Position = (SVector3i){x0, y1, data};

            vertices[3].Position = (SVector3i){x1, y1, data};
            vertices[4].Position = (SVector3i){x0, y1, data};
            vertices[5].Position = (SVector3i){x1, y0, data};

            vertices[0].TexCoord = (STexCoord2i){tx0, ty0};
            vertices[1].TexCoord = (STexCoord2i){tx1, ty0};
            vertices[2].TexCoord = (STexCoord2i){tx0, ty1};

            vertices[3].TexCoord = (STexCoord2i){tx1, ty1};
            vertices[4].TexCoord = (STexCoord2i){tx0, ty1};
            vertices[5].TexCoord = (STexCoord2i){tx1, ty0};

            GPU3DSExt.quadVertexes.Count += 6;
        }
#endif
    }

}


inline void __attribute__((always_inline)) gpu3dsAddMode7LineVertexes(
    int x0, int y0, int x1, int y1,
    float tx0, float ty0, float tx1, float ty1)
{
    // Stereo path: emit per-eye Mode7 line vertices with horizontal offsets (Mode7 treated as BG0)
    if (stereo3dsIsEnabled()) {
        // Compute per-line depth based on gradient settings
        float t = (float)y0 / (float)SCREEN_HEIGHT; // 0 = top, 1 = bottom
        float depthLine = settings3DS.Mode7UseGradient
            ? (settings3DS.Mode7DepthNear + (settings3DS.Mode7DepthFar - settings3DS.Mode7DepthNear) * (1.0f - t))  // near at bottom
            : settings3DS.Mode7DepthNear;
        int plane = settings3DS.ScreenPlaneLayer;
        float planeDepth = (plane >= 0 && plane < 5) ? settings3DS.LayerDepth[plane] : 0.0f;
        float slider = stereo3dsGetSliderValue();
        float strength = settings3DS.StereoDepthStrength;
        float offsetBase = (depthLine - planeDepth) * strength * slider;
        // Clamp to comfort (~34px total)
        if (offsetBase > 17.0f) offsetBase = 17.0f;
        if (offsetBase < -17.0f) offsetBase = -17.0f;

        for (int eye = 0; eye < 2; ++eye) {
            float eyeOffset = (eye == 0) ? offsetBase : -offsetBase;
            int offset = (int)eyeOffset;
            int ex0 = x0 + offset;
            int ex1 = x1 + offset;
#ifndef RELEASE
            if (GPU3DS.isReal3DS)
            {
#endif
                SMode7LineVertex *vertices = &((SMode7LineVertex *) GPU3DSExt.stereoMode7LineVertexes[eye].List)[GPU3DSExt.stereoMode7LineVertexes[eye].Count];

                vertices[0].Position = (SVector4i){ex0, y0, 0, 1};
                vertices[0].TexCoord = (STexCoord2f){tx0, ty0};

                vertices[1].Position = (SVector4i){ex1, -16384, 0, 1};
                vertices[1].TexCoord = (STexCoord2f){tx1, ty1};

                GPU3DSExt.stereoMode7LineVertexes[eye].Count += 2;
#ifndef RELEASE
            }
            else
            {
                // Citra/test path without geometry shader
                SMode7LineVertex *vertices = &((SMode7LineVertex *) GPU3DSExt.stereoMode7LineVertexes[eye].List)[GPU3DSExt.stereoMode7LineVertexes[eye].Count];

                vertices[0].Position = (SVector4i){ex0, y0, 0, 1};
                vertices[0].TexCoord = (STexCoord2f){tx0, ty0};

                vertices[1].Position = (SVector4i){ex1, y0, 0, 1};
                vertices[1].TexCoord = (STexCoord2f){tx1, ty1};

                vertices[2].Position = (SVector4i){ex0, y1, 0, 1};
                vertices[2].TexCoord = (STexCoord2f){tx0, ty0};

                vertices[3].Position = (SVector4i){ex1, y0, 0, 1};
                vertices[3].TexCoord = (STexCoord2f){tx1, ty1};

                vertices[4].Position = (SVector4i){ex1, y1, 0, 1};
                vertices[4].TexCoord = (STexCoord2f){tx1, ty1};

                vertices[5].Position = (SVector4i){ex0, y1, 0, 1};
                vertices[5].TexCoord = (STexCoord2f){tx0, ty0};

                GPU3DSExt.stereoMode7LineVertexes[eye].Count += 6;
            }
#endif
        }
        return;
    }

#ifndef RELEASE
    if (GPU3DS.isReal3DS)
    {
#endif
        SMode7LineVertex *vertices = &((SMode7LineVertex *) GPU3DSExt.mode7LineVertexes.List)[GPU3DSExt.mode7LineVertexes.Count];

        vertices[0].Position = (SVector4i){x0, y0, 0, 1};
        vertices[0].TexCoord = (STexCoord2f){tx0, ty0};

        // yes we will use a special value for the geometry shader to detect detect mode 7
        vertices[1].Position = (SVector4i){x1, -16384, 0, 1};
        vertices[1].TexCoord = (STexCoord2f){tx1, ty1};

        GPU3DSExt.mode7LineVertexes.Count += 2;

#ifndef RELEASE
    }
    else
    {
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        SMode7LineVertex *vertices = &((SMode7LineVertex *) GPU3DSExt.mode7LineVertexes.List)[GPU3DSExt.mode7LineVertexes.Count];

        vertices[0].Position = (SVector4i){x0, y0, 0, 1};
        vertices[0].TexCoord = (STexCoord2f){tx0, ty0};

        vertices[1].Position = (SVector4i){x1, y0, 0, 1};
        vertices[1].TexCoord = (STexCoord2f){tx1, ty1};

        vertices[2].Position = (SVector4i){x0, y1, 0, 1};
        vertices[2].TexCoord = (STexCoord2f){tx0, ty0};


        vertices[3].Position = (SVector4i){x1, y0, 0, 1};
        vertices[3].TexCoord = (STexCoord2f){tx1, ty1};

        vertices[4].Position = (SVector4i){x1, y1, 0, 1};
        vertices[4].TexCoord = (STexCoord2f){tx1, ty1};

        vertices[5].Position = (SVector4i){x0, y1, 0, 1};
        vertices[5].TexCoord = (STexCoord2f){tx0, ty0};

        GPU3DSExt.mode7LineVertexes.Count += 6;
    }
#endif

}




inline void __attribute__((always_inline)) gpu3dsSetMode7TileTexturePos(int idx, int data)
{
#ifndef RELEASE
    if (GPU3DS.isReal3DS)
    {
#endif
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx];

        m7vertices[0].Position.z = data;
        //m7vertices[1].Position.z = data;
#ifndef RELEASE
    }
    else
    {
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx * 6];

        m7vertices[0].Position.z = data;
        m7vertices[1].Position.z = data;
        m7vertices[2].Position.z = data;
        m7vertices[3].Position.z = data;
        m7vertices[4].Position.z = data;
        m7vertices[5].Position.z = data;
    }
#endif
}



inline void __attribute__((always_inline)) gpu3dsSetMode7TileModifiedFlag(int idx)
{
    int updateFrame = GPU3DSExt.mode7FrameCount;

#ifndef RELEASE
    if (GPU3DS.isReal3DS)
    {
#endif
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx];

        m7vertices[0].Position.w = updateFrame;
        //m7vertices[1].Position.w = updateFrame;
#ifndef RELEASE
    }
    else
    {
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx * 6];

        m7vertices[0].Position.w = updateFrame;
        m7vertices[1].Position.w = updateFrame;
        m7vertices[2].Position.w = updateFrame;
        m7vertices[3].Position.w = updateFrame;
        m7vertices[4].Position.w = updateFrame;
        m7vertices[5].Position.w = updateFrame;
    }
#endif
}


inline void __attribute__((always_inline)) gpu3dsSetMode7TileModifiedFlag(int idx, int updateFrame)
{
#ifndef RELEASE
    if (GPU3DS.isReal3DS)
    {
#endif
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx];

        m7vertices[0].Position.w = updateFrame;
        //m7vertices[1].Position.w = updateFrame;
#ifndef RELEASE
    }
    else
    {
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx * 6];

        m7vertices[0].Position.w = updateFrame;
        m7vertices[1].Position.w = updateFrame;
        m7vertices[2].Position.w = updateFrame;
        m7vertices[3].Position.w = updateFrame;
        m7vertices[4].Position.w = updateFrame;
        m7vertices[5].Position.w = updateFrame;
    }
#endif
}

inline void __attribute__((always_inline)) gpu3dsAddMode7ScanlineVertexes(
    int x0, int y0, int x1, int y1,
    int tx0, int ty0, int tx1, int ty1,
    int data)
{
#ifndef RELEASE
    if (GPU3DS.isReal3DS)
    {
#endif
        STileVertex *vertices = &((STileVertex *) GPU3DSExt.tileVertexes.List)[GPU3DSExt.tileVertexes.Count];

        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[0].TexCoord = (STexCoord2i){tx0, ty0};

        // yes we will use a special value for the geometry shader to detect detect mode 7
        vertices[1].Position = (SVector3i){x1, -16384, data};
        vertices[1].TexCoord = (STexCoord2i){tx1, ty1};

        //GPU3DSExt.tileCount += 2;
        GPU3DSExt.tileVertexes.Count += 2;

#ifndef RELEASE
    }
    else
    {
        //STileVertex *vertices = &GPU3DSExt.vertexList[GPU3DSExt.vertexCount];
        STileVertex *vertices = &((STileVertex *) GPU3DSExt.quadVertexes.List)[GPU3DSExt.quadVertexes.Count];

        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[0].TexCoord = (STexCoord2i){tx0, ty0};

        vertices[1].Position = (SVector3i){x1, y0, data};
        vertices[1].TexCoord = (STexCoord2i){tx1, ty1};

        vertices[2].Position = (SVector3i){x0, y1, data};
        vertices[2].TexCoord = (STexCoord2i){tx0, ty0};


        vertices[3].Position = (SVector3i){x1, y0, data};
        vertices[3].TexCoord = (STexCoord2i){tx1, ty1};

        vertices[4].Position = (SVector3i){x1, y1, data};
        vertices[4].TexCoord = (STexCoord2i){tx1, ty1};

        vertices[5].Position = (SVector3i){x0, y1, data};
        vertices[5].TexCoord = (STexCoord2i){tx0, ty0};

        //GPU3DSExt.vertexCount += 6;
        GPU3DSExt.quadVertexes.Count += 6;
    }
#endif
}


void gpu3dsDrawRectangle(int x0, int y0, int x1, int y1, int depth, u32 color);
void gpu3dsAddRectangleVertexes(int x0, int y0, int x1, int y1, int depth, u32 color);
void gpu3dsClearColorAndDepth(int width, int height);
void gpu3dsResetStereoClearFlag();  // Reset flag at start of each frame
void gpu3dsDrawVertexes(bool repeatLastDraw = false, int storeIndex = -1);
void gpu3dsDrawMode7Vertexes(int fromIndex, int tileCount);
void gpu3dsDrawMode7LineVertexes(bool repeatLastDraw = false, int storeIndex = -1);


#endif
