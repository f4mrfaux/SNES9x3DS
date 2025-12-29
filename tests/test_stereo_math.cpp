//=============================================================================
// Stereo 3D Math Test Harness
// Tests core stereo logic without requiring full emulator rebuild
// Run: make test-stereo (or compile standalone)
//=============================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>

// Mock settings structure (matches 3dssettings.h)
struct MockSettings {
    float LayerDepth[5];
    float StereoSliderValue;
    float StereoDepthStrength;
    int ScreenPlaneLayer;
    bool EnableStereo3D;
};

// Mock stereo offsets (matches 3dsstereo.cpp)
float g_stereoLayerOffsets[2][5] = {{0}};

// Test configuration
static MockSettings testSettings = {
    .LayerDepth = {12.0f, 8.0f, 4.0f, 0.0f, -15.0f},  // BG0, BG1, BG2, BG3, Sprites
    .StereoSliderValue = 1.0f,
    .StereoDepthStrength = 1.0f,
    .ScreenPlaneLayer = 3,  // BG3 at screen plane
    .EnableStereo3D = true
};

// Mock offset calculation (matches stereo3dsUpdateLayerOffsetsFromSlider)
void testUpdateLayerOffsetsFromSlider(float sliderValue) {
    if (sliderValue < 0.01f) {
        for (int eye = 0; eye < 2; ++eye)
            for (int i = 0; i < 5; ++i)
                g_stereoLayerOffsets[eye][i] = 0.0f;
        return;
    }

    const int plane = testSettings.ScreenPlaneLayer;
    const float planeDepth = (plane >= 0 && plane < 5) ? testSettings.LayerDepth[plane] : 0.0f;
    const float depthStrength = testSettings.StereoDepthStrength;
    const float maxPerEye = 17.0f;

    for (int i = 0; i < 5; ++i) {
        float depth = (testSettings.LayerDepth[i] - planeDepth) * depthStrength;
        float offset = depth * sliderValue;

        if (offset >  maxPerEye) offset =  maxPerEye;
        if (offset < -maxPerEye) offset = -maxPerEye;

        g_stereoLayerOffsets[0][i] = offset;   // LEFT eye
        g_stereoLayerOffsets[1][i] = -offset;  // RIGHT eye
    }
}

// Test cases
int test_zero_slider() {
    printf("Test 1: Zero slider (should zero all offsets)\n");
    testUpdateLayerOffsetsFromSlider(0.0f);
    
    for (int eye = 0; eye < 2; ++eye) {
        for (int i = 0; i < 5; ++i) {
            if (fabs(g_stereoLayerOffsets[eye][i]) > 0.001f) {
                printf("  FAIL: Offset[%d][%d] = %.2f (expected 0.0)\n", eye, i, g_stereoLayerOffsets[eye][i]);
                return 0;
            }
        }
    }
    printf("  PASS\n");
    return 1;
}

int test_sprite_popout() {
    printf("Test 2: Sprite pop-out (negative depth)\n");
    testUpdateLayerOffsetsFromSlider(1.0f);
    
    // Sprites (layer 4) should have negative depth (-15.0)
    // LEFT eye should shift LEFT (negative offset)
    // RIGHT eye should shift RIGHT (positive offset)
    float leftOffset = g_stereoLayerOffsets[0][4];
    float rightOffset = g_stereoLayerOffsets[1][4];
    
    if (leftOffset >= 0.0f) {
        printf("  FAIL: LEFT eye offset = %.2f (expected negative for pop-out)\n", leftOffset);
        return 0;
    }
    if (rightOffset <= 0.0f) {
        printf("  FAIL: RIGHT eye offset = %.2f (expected positive for pop-out)\n", rightOffset);
        return 0;
    }
    if (fabs(leftOffset + rightOffset) > 0.001f) {
        printf("  FAIL: Offsets not symmetric: L=%.2f R=%.2f\n", leftOffset, rightOffset);
        return 0;
    }
    printf("  PASS: L=%.2f R=%.2f (sprites pop out)\n", leftOffset, rightOffset);
    return 1;
}

int test_background_into_screen() {
    printf("Test 3: Background into screen (positive depth)\n");
    testUpdateLayerOffsetsFromSlider(1.0f);
    
    // BG0 (layer 0) should have positive depth (12.0)
    // LEFT eye should shift RIGHT (positive offset for convergence)
    // RIGHT eye should shift LEFT (negative offset for convergence)
    float leftOffset = g_stereoLayerOffsets[0][0];
    float rightOffset = g_stereoLayerOffsets[1][0];
    
    if (leftOffset <= 0.0f) {
        printf("  FAIL: LEFT eye offset = %.2f (expected positive for into-screen)\n", leftOffset);
        return 0;
    }
    if (rightOffset >= 0.0f) {
        printf("  FAIL: RIGHT eye offset = %.2f (expected negative for into-screen)\n", rightOffset);
        return 0;
    }
    if (fabs(leftOffset + rightOffset) > 0.001f) {
        printf("  FAIL: Offsets not symmetric: L=%.2f R=%.2f\n", leftOffset, rightOffset);
        return 0;
    }
    printf("  PASS: L=%.2f R=%.2f (background goes into screen)\n", leftOffset, rightOffset);
    return 1;
}

int test_screen_plane_zero() {
    printf("Test 4: Screen plane layer (BG3) should have zero offset\n");
    testUpdateLayerOffsetsFromSlider(1.0f);
    
    // BG3 (layer 3) is the screen plane, should have zero offset
    float leftOffset = g_stereoLayerOffsets[0][3];
    float rightOffset = g_stereoLayerOffsets[1][3];
    
    if (fabs(leftOffset) > 0.001f || fabs(rightOffset) > 0.001f) {
        printf("  FAIL: Screen plane has offset L=%.2f R=%.2f (expected 0.0)\n", leftOffset, rightOffset);
        return 0;
    }
    printf("  PASS: Screen plane at zero offset\n");
    return 1;
}

int test_depth_gradient() {
    printf("Test 5: Depth gradient (BG0 > BG1 > BG2 > BG3)\n");
    testUpdateLayerOffsetsFromSlider(1.0f);
    
    // BG0 should have largest offset, BG1 medium, BG2 small, BG3 zero
    float bg0_left = fabs(g_stereoLayerOffsets[0][0]);
    float bg1_left = fabs(g_stereoLayerOffsets[0][1]);
    float bg2_left = fabs(g_stereoLayerOffsets[0][2]);
    float bg3_left = fabs(g_stereoLayerOffsets[0][3]);
    
    if (bg0_left <= bg1_left || bg1_left <= bg2_left || bg2_left <= bg3_left) {
        printf("  FAIL: Depth gradient incorrect: BG0=%.2f BG1=%.2f BG2=%.2f BG3=%.2f\n",
               bg0_left, bg1_left, bg2_left, bg3_left);
        return 0;
    }
    printf("  PASS: Gradient correct (BG0=%.2f > BG1=%.2f > BG2=%.2f > BG3=%.2f)\n",
           bg0_left, bg1_left, bg2_left, bg3_left);
    return 1;
}

int test_slider_scaling() {
    printf("Test 6: Slider scaling (0.5 slider = half offsets)\n");
    testUpdateLayerOffsetsFromSlider(0.5f);
    float half_left = fabs(g_stereoLayerOffsets[0][0]);
    
    testUpdateLayerOffsetsFromSlider(1.0f);
    float full_left = fabs(g_stereoLayerOffsets[0][0]);
    
    float ratio = half_left / full_left;
    if (ratio < 0.45f || ratio > 0.55f) {
        printf("  FAIL: Slider scaling incorrect: half=%.2f full=%.2f ratio=%.2f (expected ~0.5)\n",
               half_left, full_left, ratio);
        return 0;
    }
    printf("  PASS: Slider scaling correct (ratio=%.2f)\n", ratio);
    return 1;
}

int test_clamping() {
    printf("Test 7: Offset clamping (maxPerEye = 17.0)\n");
    // Set extreme slider value to test clamping
    testUpdateLayerOffsetsFromSlider(10.0f);  // Way beyond normal range
    
    for (int eye = 0; eye < 2; ++eye) {
        for (int i = 0; i < 5; ++i) {
            float absOffset = fabs(g_stereoLayerOffsets[eye][i]);
            if (absOffset > 17.1f) {  // Small epsilon for float comparison
                printf("  FAIL: Offset[%d][%d] = %.2f exceeds max (17.0)\n", eye, i, absOffset);
                return 0;
            }
        }
    }
    printf("  PASS: All offsets clamped to max 17.0\n");
    return 1;
}

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("Stereo 3D Math Test Harness\n");
    printf("========================================\n\n");
    
    int passed = 0;
    int total = 7;
    
    passed += test_zero_slider();
    passed += test_sprite_popout();
    passed += test_background_into_screen();
    passed += test_screen_plane_zero();
    passed += test_depth_gradient();
    passed += test_slider_scaling();
    passed += test_clamping();
    
    printf("\n========================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    printf("========================================\n");
    
    if (passed == total) {
        printf("✅ All tests PASSED - Stereo math is correct!\n");
        return 0;
    } else {
        printf("❌ Some tests FAILED - Check implementation\n");
        return 1;
    }
}

