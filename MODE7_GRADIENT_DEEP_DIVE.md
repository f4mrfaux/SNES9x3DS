# Mode 7 Per-Scanline Gradient - Technical Deep Dive

**Date:** November 11, 2025
**Status:** Research & Planning
**Potential:** 🌟 GAME-CHANGING for racing games!

---

## 🎯 The Vision

Imagine playing **F-Zero** or **Super Mario Kart** where:
- The road far ahead (horizon) is deep into the screen
- The road right in front of you is at screen plane
- Your racer pops out toward you
- **Perfect depth gradient matching the perspective!**

This is what M2 did with **3D After Burner II** and **3D Out Run** - and it's STUNNING.

---

## 🎮 How Mode 7 Works

### SNES Mode 7 Mechanics

Mode 7 creates pseudo-3D by:
1. Rotating and scaling a single 2D layer
2. **Different transformation per scanline**
3. Far scanlines: Heavily scaled down (small)
4. Near scanlines: Less scaled (large)
5. Creates perspective illusion

**Example: F-Zero Road**
```
Scanline   0 (top):     Tiny, far away  (horizon)
Scanline  50:           Smaller
Scanline 100:           Medium
Scanline 150:           Larger
Scanline 200 (bottom):  Huge, close up  (in front of racer)
```

### Current Implementation Concern

Our current system applies **uniform horizontal offset** to entire layer:
```cpp
x0 += offset;  // Same offset for all vertices
x1 += offset;
```

For Mode 7, this means:
- Horizon gets same offset as foreground
- Breaks the perspective illusion
- May look "sheared" or wrong

---

## 💡 Per-Scanline Gradient Solution

### The Concept

Apply **variable offset based on Y position**:

```
Far scanlines (top):    Small offset   (less parallax)
Near scanlines (bottom): Large offset  (more parallax)
```

This matches how real depth works:
- Distant objects have small parallax
- Close objects have large parallax
- **Matches Mode 7's existing perspective!**

### Visual Effect

**Without gradient (current):**
```
         [Road]          ← Uniform offset
         [Road]          ← Looks sheared
        [Road]           ← Breaks perspective
       [Road]
      [Road]
     [Road]
```

**With gradient (proposed):**
```
         [Road]          ← Small offset (far)
          [Road]         ← Gradual increase
           [Road]        ← Medium offset
            [Road]       ← More offset
             [Road]      ← Larger offset
              [Road]     ← Max offset (near)
```

**Result:** Road maintains perspective while having proper depth!

---

## 🔧 Implementation Strategy

### Approach 1: Vertex-Level Y-Based Offset

**Modify vertex generation to check Y coordinate:**

```cpp
inline void gpu3dsAddTileVertexes(
    int x0, int y0, int x1, int y1, ...)
{
    // Base offset for this layer
    float baseOffset = g_stereoLayerOffsets[g_currentLayerIndex];

    // If Mode 7, apply gradient
    if (PPU.BGMode == 7 && g_currentLayerIndex == 0)
    {
        // Calculate gradient factor based on Y position
        float screenHeight = 240.0f;  // 3DS screen height
        float nearY = 200.0f;  // Bottom of screen (near)
        float farY = 40.0f;    // Top of screen (far)

        // Gradient: 0.0 (far) to 1.0 (near)
        float gradient = (y0 - farY) / (nearY - farY);
        gradient = clamp(gradient, 0.0f, 1.0f);

        // Apply gradient to offset
        // Far: 20% of offset
        // Near: 100% of offset
        float minScale = 0.2f;  // 20% offset at horizon
        float maxScale = 1.0f;  // 100% offset at foreground
        float scaledOffset = baseOffset * (minScale + gradient * (maxScale - minScale));

        x0 += (int)scaledOffset;
        x1 += (int)scaledOffset;
    }
    else
    {
        // Normal layers: uniform offset
        int offset = (int)baseOffset;
        x0 += offset;
        x1 += offset;
    }

    // ... rest of vertex generation
}
```

**Pros:**
- Simple to implement
- Works with existing vertex system
- Automatic per-scanline gradient

**Cons:**
- Y position might not perfectly match Mode 7 perspective
- May need tuning

### Approach 2: Hook Mode 7 Rendering Directly

**Find where Mode 7 tiles are generated:**

File: `source/3dsimpl_gpu.cpp` - `gpu3dsInitializeMode7Vertex()`

This function already sets up Mode 7 vertices. We could:
1. Track which scanline each vertex belongs to
2. Apply gradient based on scanline number
3. More accurate to actual Mode 7 math

**Pros:**
- More accurate
- Can use Mode 7's own perspective calculations
- Perfect gradient

**Cons:**
- More complex
- Need to understand Mode 7 vertex setup
- Potential for bugs

### Approach 3: Use Mode 7 Depth Values

**SNES9x already has Mode 7 depth settings!**

From `3dssettings.h`:
```cpp
float   Mode7DepthNear = 1.0f;
float   Mode7DepthFar = 8.0f;
bool    Mode7UseGradient = false;
```

These were added but **never implemented**! We can use them:

```cpp
if (PPU.BGMode == 7 && settings3DS.Mode7UseGradient)
{
    // Use Y position to interpolate between near and far
    float t = (y0 - farY) / (nearY - farY);
    float depthScale = lerp(settings3DS.Mode7DepthFar,
                            settings3DS.Mode7DepthNear,
                            t);

    float scaledOffset = baseOffset * depthScale;
    x0 += (int)scaledOffset;
    x1 += (int)scaledOffset;
}
```

**Pros:**
- Uses existing settings infrastructure
- User-configurable (Near/Far values)
- Can toggle on/off
- Clean design

**Cons:**
- Still need to determine correct Y range

---

## 📐 Math Deep Dive

### Gradient Formula

**Goal:** Map Y position to offset scale

**Inputs:**
- `y` = Current vertex Y position (0-240)
- `nearY` = Bottom of road on screen (e.g., 200)
- `farY` = Top of road/horizon (e.g., 40)
- `baseOffset` = Layer offset from slider (e.g., 15px at 100%)

**Calculation:**
```
1. Normalize Y to 0.0-1.0:
   gradient = (y - farY) / (nearY - farY)

2. Apply non-linear curve (optional):
   gradient = pow(gradient, 1.5)  // More depth near camera

3. Map to scale range:
   scale = minScale + gradient * (maxScale - minScale)
   Example: 0.2 + gradient * (1.0 - 0.2) = 0.2 to 1.0

4. Apply to offset:
   finalOffset = baseOffset * scale
```

**Example with baseOffset = 15px:**
- Horizon (gradient = 0.0): offset = 15 * 0.2 = **3px**
- Mid-road (gradient = 0.5): offset = 15 * 0.6 = **9px**
- Foreground (gradient = 1.0): offset = 15 * 1.0 = **15px**

### Non-Linear Curves

**Linear gradient:**
```cpp
scale = gradient;  // Straight line
```

**Quadratic gradient (more depth near camera):**
```cpp
scale = gradient * gradient;  // Emphasize foreground
```

**Square root gradient (more depth at horizon):**
```cpp
scale = sqrt(gradient);  // Emphasize background
```

**Exponential gradient:**
```cpp
scale = pow(gradient, 1.5f);  // Smooth curve
```

**Which to use?** Start with **linear**, adjust if needed based on testing.

---

## 🎨 Visual Examples

### F-Zero: Mute City Track

**Road perspective:**
```
Horizon line (Y=40):   ▁▁▁▁▁▁   (far, small)
                      ▂▂▂▂▂▂▂▂
                     ▃▃▃▃▃▃▃▃▃
                    ▄▄▄▄▄▄▄▄▄▄
Near player (Y=200): ██████████  (close, large)
```

**With gradient offset:**
```
LEFT EYE:                RIGHT EYE:
    ▁▁▁▁▁▁                  ▁▁▁▁▁▁
   ▂▂▂▂▂▂▂▂              ▂▂▂▂▂▂▂▂
    ▃▃▃▃▃▃▃▃▃          ▃▃▃▃▃▃▃▃▃
      ▄▄▄▄▄▄▄▄▄▄    ▄▄▄▄▄▄▄▄▄▄
        ██████████ ██████████

Brain fuses → Perfect depth gradient!
```

### Super Mario Kart: Rainbow Road

**Curved track with items:**
```
Sky (BG):      At screen        (0px offset)
Far track:     Slight depth     (3px offset)
Mid track:     Medium depth     (8px offset)
Near track:    Deep depth       (15px offset)
Kart sprite:   Pop out!         (20px offset)
Items:         Pop out more!    (25px offset)
```

**Result:** Track recedes naturally, kart and items pop forward!

---

## 🔍 Where to Hook In

### Option A: Modify Existing Vertex Function

**File:** `source/3dsimpl_gpu.h` (line 146+)

```cpp
inline void gpu3dsAddTileVertexes(
    int x0, int y0, int x1, int y1,
    int tx0, int ty0, int tx1, int ty1,
    int data)
{
    // Apply stereoscopic horizontal offset based on current layer
    int offset = (int)g_stereoLayerOffsets[g_currentLayerIndex];

    // NEW: Apply gradient for Mode 7
    if (PPU.BGMode == 7 && g_currentLayerIndex == 0 &&
        settings3DS.Mode7UseGradient)
    {
        offset = calculateMode7GradientOffset(y0, offset);
    }

    x0 += offset;
    x1 += offset;

    // ... rest of function
}
```

### Option B: Add Helper Function

**File:** `source/3dsimpl_gpu.cpp`

```cpp
int calculateMode7GradientOffset(int y, int baseOffset)
{
    // Screen Y range for Mode 7 road
    const float farY = 40.0f;   // Horizon
    const float nearY = 200.0f; // Foreground

    // Clamp to valid range
    float yf = clamp((float)y, farY, nearY);

    // Calculate gradient (0.0 = far, 1.0 = near)
    float gradient = (yf - farY) / (nearY - farY);

    // Apply curve (optional)
    gradient = pow(gradient, settings3DS.Mode7DepthCurve);

    // Scale offset
    float minScale = 1.0f / settings3DS.Mode7DepthFar;   // e.g., 1/8 = 0.125
    float maxScale = settings3DS.Mode7DepthNear;         // e.g., 1.0
    float scale = minScale + gradient * (maxScale - minScale);

    return (int)(baseOffset * scale);
}
```

### Option C: Mode 7-Specific Rendering Path

**Create separate Mode 7 handling:**

```cpp
if (PPU.BGMode == 7)
{
    // Use specialized Mode 7 gradient system
    renderMode7WithGradient();
}
else
{
    // Normal layer rendering
    S9xRenderScreenHardware();
}
```

---

## 📊 Settings Configuration

### Add to UI Menu

```
MODE 7 SETTINGS:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Use Gradient:      [ON]     ◄ ►
Horizon Depth:     [25%]    ◄ ►
Foreground Depth:  [100%]   ◄ ►
Curve:             [Linear] ◄ ►

PRESETS:
[Subtle] [Balanced] [Aggressive]
```

### Configuration Values

```cpp
struct Mode7Settings {
    bool   useGradient;       // Enable gradient
    float  depthFar;          // Horizon scale (0.1-1.0)
    float  depthNear;         // Foreground scale (0.5-2.0)
    float  depthCurve;        // Curve exponent (0.5-2.0)
    float  minY;              // Start of gradient (pixels)
    float  maxY;              // End of gradient (pixels)
};

// Presets
Mode7Settings MODE7_SUBTLE = {
    true, 0.5f, 0.8f, 1.0f, 40.0f, 200.0f
};

Mode7Settings MODE7_BALANCED = {
    true, 0.25f, 1.0f, 1.0f, 40.0f, 200.0f
};

Mode7Settings MODE7_AGGRESSIVE = {
    true, 0.1f, 1.5f, 1.5f, 40.0f, 200.0f
};
```

---

## 🧪 Testing Strategy

### Phase 1: Proof of Concept (2-3 hours)
1. Implement Option A (simple vertex modification)
2. Use hardcoded values (farY=40, nearY=200, scale 0.2-1.0)
3. Test with F-Zero
4. Document visual quality

### Phase 2: Refinement (2-3 hours)
1. Add settings variables
2. Make Y range configurable
3. Add curve options
4. Test with multiple Mode 7 games

### Phase 3: UI Integration (2-3 hours)
1. Add Mode 7 section to settings menu
2. Add presets
3. Add per-game save
4. Polish and test

**Total Time:** 6-9 hours

---

## 🎯 Expected Results

### F-Zero

**Before (uniform offset):**
- Road looks sheared
- Perspective broken
- Doesn't feel right

**After (gradient offset):**
- Road recedes naturally into horizon
- Perspective enhanced
- Racers pop out dramatically
- **LOOKS AMAZING!**

### Super Mario Kart

**Before:**
- Track looks flat or weird
- Items and karts at same depth as track

**After:**
- Track has proper depth gradient
- Items and karts pop out from track
- Rainbow Road looks *spectacular*
- **Game-changing!**

### Pilotwings

**Before:**
- Ground looks uniform
- No sense of altitude

**After:**
- Ground recedes to horizon
- Plane pops out above ground
- Real sense of flying
- **Immersive!**

---

## 💪 Feasibility Assessment

### Technical Feasibility: 🟢 HIGH

**Reasons:**
- Simple math (linear interpolation)
- Vertex system already in place
- Y coordinate readily available
- No GPU changes needed
- Estimated 6-9 hours work

### Performance Impact: 🟡 MEDIUM

**Additional Cost:**
- Per-vertex: 1 float subtraction, 1 division, 1 multiply
- Negligible on ARM11 CPU
- Only affects Mode 7 games
- Can be toggled off

**Estimate:** < 5% additional overhead for Mode 7 games

### Visual Impact: 🟢 SPECTACULAR

**This could be THE KILLER FEATURE:**
- Makes racing games shine
- Unique to this implementation
- No other SNES emulator has this
- M2-level quality

---

## 🚀 Recommendation

### Priority: HIGH

**Why implement this:**
1. **Wow factor** - This will blow people away
2. **Technical showcase** - Demonstrates deep understanding
3. **Racing game support** - Critical genre for 3D
4. **Differentiation** - Sets us apart from other ports
5. **Relatively easy** - 6-9 hours for major feature

### Implementation Order

1. **First:** Test current implementation with Mode 7 (might be fine!)
2. **If issues:** Implement basic uniform disable (Option 1 from UI_AND_MODE7_PLAN)
3. **Then:** Implement gradient system (this document)
4. **Polish:** Add UI controls and presets

### Success Criteria

**Minimum:**
- Mode 7 games don't crash
- Visual quality acceptable

**Target:**
- Mode 7 has proper depth gradient
- Racing games look amazing
- Configurable per-game

**Stretch:**
- Auto-detect optimal Y range
- Per-scanline optimization
- Perfect visual quality

---

## 📝 Next Steps

1. **Test on hardware** - See how current implementation handles Mode 7
2. **If broken** - Implement basic disable first
3. **Then** - Implement gradient system
4. **Polish** - Add to UI menu
5. **Showcase** - Make demo video of F-Zero in 3D!

---

## 💡 Future Enhancements

### Per-Game Auto-Calibration

**Detect road Y range automatically:**
```cpp
// During gameplay, track which Y values Mode 7 uses
// Auto-calculate farY and nearY
// Save per-game
```

### Advanced Curves

**Multiple curve types:**
- Linear
- Quadratic
- Cubic
- S-curve
- Custom Bézier

### Per-Scanline Effects

**More complex gradients:**
- Different curves for left/right halves
- Handling for curved tracks
- Adaptive based on rotation

---

## 🎬 Demo Scenario

**Imagine showing this to the 3DS homebrew community:**

> "Check out F-Zero in stereoscopic 3D with per-scanline depth gradient!
> The track naturally recedes into the horizon while your racer pops out toward you.
> This is the same technique M2 used for 3D After Burner II, now working for SNES!"

**Result:** 🤯 Mind blown!

---

**Document Version:** 1.0
**Created:** November 11, 2025
**Status:** Planned - Awaiting Hardware Testing
**Potential Impact:** 🌟🌟🌟🌟🌟 **GAME-CHANGING**
**Implementation Difficulty:** 🟡 Medium (6-9 hours)
**Visual Impact:** 🔥🔥🔥🔥🔥 **SPECTACULAR**

---

*"Per-scanline gradient: The feature that will make racing games LEGENDARY on 3DS!"*
