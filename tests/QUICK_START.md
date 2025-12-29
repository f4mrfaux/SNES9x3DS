# Stereo 3D Test Harness - Quick Start

## ✅ Status: READY TO USE

The test harness validates stereo 3D math/logic **without rebuilding the full emulator**.

## Quick Commands

```bash
# From project root:
make test                    # Run all tests

# From tests directory:
cd tests
make test-stereo            # Run stereo tests
make clean                  # Clean test binaries
```

## What Gets Tested

✅ **7 core tests** covering:
1. Zero slider behavior
2. Sprite pop-out (negative depth)
3. Background into-screen (positive depth)
4. Screen plane zero offset
5. Depth gradient (BG0 > BG1 > BG2 > BG3)
6. Slider scaling (0.5 = half offsets)
7. Offset clamping (max 17.0 pixels)

## Typical Workflow

```bash
# 1. Make a change to stereo math/logic
vim source/3dsstereo.cpp

# 2. Run quick test (takes < 1 second)
make test

# 3. If tests pass, rebuild full emulator
make

# 4. Deploy to 3DS for hardware testing
```

## Adding New Tests

Edit `tests/test_stereo_math.cpp`:

```cpp
int test_my_new_feature() {
    printf("Test 8: My new feature\n");
    // ... test logic ...
    if (/* passes */) {
        printf("  PASS\n");
        return 1;
    }
    printf("  FAIL: reason\n");
    return 0;
}

// In main():
passed += test_my_new_feature();
total = 8;  // Update total
```

## Integration with CI/CD

The test harness can be integrated into automated builds:

```bash
# In CI script:
make test || exit 1  # Fail build if tests fail
make                 # Build only if tests pass
```

## Performance

- **Compile time**: ~0.5 seconds
- **Run time**: < 0.1 seconds
- **Total**: < 1 second vs. ~30-60 seconds for full rebuild

This enables **rapid iteration** on stereo logic fixes!


