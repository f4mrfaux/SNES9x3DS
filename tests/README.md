# Stereo 3D Test Harness

Quick validation tests for stereo 3D implementation without rebuilding the full emulator.

## Quick Start

```bash
# Run all tests
cd tests
make test-stereo

# Or compile and run manually
make
./test_stereo_math
```

## What It Tests

1. **Zero Slider**: All offsets should be zero when slider is at 0
2. **Sprite Pop-Out**: Sprites should have negative LEFT offset, positive RIGHT offset
3. **Background Into Screen**: Backgrounds should have positive LEFT offset, negative RIGHT offset
4. **Screen Plane**: Screen plane layer (BG3) should always have zero offset
5. **Depth Gradient**: BG0 > BG1 > BG2 > BG3 (decreasing depth)
6. **Slider Scaling**: 0.5 slider = half the offsets of 1.0 slider
7. **Clamping**: Offsets should never exceed maxPerEye (17.0)

## Expected Output

```
========================================
Stereo 3D Math Test Harness
========================================

Test 1: Zero slider (should zero all offsets)
  PASS
Test 2: Sprite pop-out (negative depth)
  PASS: L=-15.00 R=15.00 (sprites pop out)
Test 3: Background into screen (positive depth)
  PASS: L=12.00 R=-12.00 (background goes into screen)
Test 4: Screen plane layer (BG3) should have zero offset
  PASS: Screen plane at zero offset
Test 5: Depth gradient (BG0 > BG1 > BG2 > BG3)
  PASS: Gradient correct (BG0=12.00 > BG1=8.00 > BG2=4.00 > BG3=0.00)
Test 6: Slider scaling (0.5 slider = half offsets)
  PASS: Slider scaling correct (ratio=0.50)
Test 7: Offset clamping (maxPerEye = 17.0)
  PASS: All offsets clamped to max 17.0

========================================
Results: 7/7 tests passed
========================================
✅ All tests PASSED - Stereo math is correct!
```

## Adding New Tests

Add test functions to `test_stereo_math.cpp`:

```cpp
int test_my_feature() {
    printf("Test N: My feature test\n");
    // ... test logic ...
    if (/* condition */) {
        printf("  PASS\n");
        return 1;
    } else {
        printf("  FAIL: reason\n");
        return 0;
    }
}
```

Then call it in `main()` and increment the total count.

## Integration with Main Build

To run tests before building:

```bash
# In main Makefile, add:
test: 
	$(MAKE) -C tests test-stereo

# Then:
make test && make
```


