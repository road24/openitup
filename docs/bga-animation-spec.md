# BGA Animation Specification

Version: 1.0


---

## Overview

BGA (Background Animation) is a 2D compositing animation system. A BGA animation is a stack of independently animated **layers**, each bound to a **sprite** (SPR or SP2). Each layer has a **keyframe timeline** that controls transform, color, visibility, and blend mode over time.

BGA is used across multiple game screens (logo, title, music select, high score, name input) and as song background visuals during play.

### Core Concepts

- **Animation**: A composition of up to 50 layers, played back against a tick-based timeline.
- **Layer**: One visual element in the composition. Each layer references a sprite and has its own keyframe timeline.
- **Keyframe**: A snapshot of a layer's state at a specific tick. Between keyframes, all interpolable properties are linearly interpolated.
- **Sprite**: The visual content of a layer — an SPR or SP2 file. The sprite's state parameter is driven by the keyframe interpolation factor.
- **Tick**: The unit of time in the animation timeline.

---

## Layer Model

A BGA animation contains up to **50 layers**, indexed 0 through 49. Layers are rendered in index order (0 first, 49 last), producing natural back-to-front compositing — higher-index layers draw on top of lower-index layers.

Each layer has:

| Property | Description |
|----------|-------------|
| Sprite reference | Filename of the visual content bound to this layer (`.spr`, `.sp2`, or `.tga`) |
| Keyframe timeline | Ordered sequence of up to 300 keyframes |

A layer with an empty sprite reference or zero keyframes is inactive and skipped during rendering.

### Layer Sprite Binding

Each layer is bound to exactly one sprite for its entire lifetime. The sprite is loaded when the animation loads — there is no mechanism to swap sprites mid-animation. All visual variation within a layer comes from the keyframe-driven transforms and the sprite's own state parameter (which selects frames in ANI-mode sprites or tiles in PATTERN-mode sprites).

The sprite reference is a filename with one of three extensions, which determines how it is loaded:

| Extension | Loaded as | Description |
|-----------|-----------|-------------|
| `.spr` | SPR sprite | Multi-picture sprite descriptor. See [SPR specification](spr-format-spec.md). |
| `.sp2` | SP2 sprite | Named-picture sprite descriptor. See [SP2 specification](sp2-format-spec.md). |
| `.tga` | Raw texture | A single image loaded directly as a one-picture TILE sprite. |

#### Raw Texture Layers

When a layer references a `.tga` file directly (instead of an SPR/SP2 descriptor), the image is loaded and automatically wrapped into a single-picture TILE sprite:

- One picture covering the full texture dimensions
- Screen rectangle set to `(0, 0, texture_width, texture_height)`
- Texture coordinates set to the full image `(0.0, 0.0, 1.0, 1.0)`
- Rendering mode set to TILE

This means the layer displays the raw image at its native size, positioned at the top-left corner. All keyframe transforms (translate, scale, rotate, color) apply normally. Since it is a TILE sprite with one picture, the state parameter `dt` has no effect.

The `.tga` extension follows the same lookup convention as SPR texture references — the actual image file may be stored in any supported format (`.tga`, `.png`, `.dds`). See [SPR: Texture Lookup](spr-format-spec.md#texture-lookup).

---

## Keyframes

A keyframe captures the complete state of a layer at a specific point in the timeline. Every keyframe contains the following properties:

### Keyframe Properties

| Property | Type | Range | Description |
|----------|------|-------|-------------|
| `tick` | integer | 0+ | Position in the timeline |
| `translate_x` | float | any | Horizontal position offset (pixels, 640x480 space) |
| `translate_y` | float | any | Vertical position offset (pixels, 640x480 space) |
| `pivot_x` | float | any | X coordinate of the rotation/scale pivot point |
| `pivot_y` | float | any | Y coordinate of the rotation/scale pivot point |
| `scale_x` | float | any | Horizontal scale factor |
| `scale_y` | float | any | Vertical scale factor |
| `rotate` | float | any | Rotation angle in degrees |
| `color_r` | float | 0.0–1.0 | Red tint multiplier |
| `color_g` | float | 0.0–1.0 | Green tint multiplier |
| `color_b` | float | 0.0–1.0 | Blue tint multiplier |
| `color_a` | float | 0.0–1.0 | Alpha (opacity) multiplier |
| `display` | boolean | on/off | Visibility flag — if off, the layer is not rendered |
| `effect` | integer | 0–4 | Blend mode for compositing (see below) |

### Blend Modes

| Value | Name | Description |
|-------|------|-------------|
| 0 | Normal | Standard alpha compositing |
| 1 | Screen | Additive blending — brightens the image |
| 2 | Multiply | Darkens — multiplies source and destination colors |
| 3 | Color Dodge | Brightens destination based on source color |
| 4 | Difference | Inverts colors based on source/destination difference |

### Which Properties Interpolate

Not all keyframe properties are interpolated between keyframes. This distinction is critical for correct playback:

**Interpolated** (linearly blended between keyframes):

- `translate_x`, `translate_y`
- `scale_x`, `scale_y`
- `rotate`
- `color_r`, `color_g`, `color_b`, `color_a`

**Not interpolated** (held from the previous keyframe):

- `pivot_x`, `pivot_y` — the pivot point is always taken from the **start** keyframe of the current interval
- `effect` — the blend mode is always taken from the **start** keyframe of the current interval
- `display` — the visibility flag is always taken from the **start** keyframe of the current interval

This means pivot, blend mode, and visibility change **instantly** at the keyframe where they are set, with no gradual transition.

---

## Interpolation

Between any two consecutive keyframes, interpolable properties are blended using **linear interpolation**:

```
dt = (current_tick - keyframe_start.tick) / (keyframe_end.tick - keyframe_start.tick)
value = start_value + (end_value - start_value) * dt
```

Where `dt` is a normalized factor from `0.0` (at the start keyframe) to `1.0` (at the end keyframe).

### Timeline Evaluation Rules

For a given `tick`, the layer evaluates as follows:

1. **Before the first keyframe** (`tick < keyframes[0].tick`): The layer is invisible. Nothing is rendered.

2. **Between two keyframes** (`keyframes[i-1].tick <= tick < keyframes[i].tick`):
   - Check `display` on the **start** keyframe (`keyframes[i-1]`). If off, the layer is invisible.
   - Compute `dt` as the interpolation factor between the two keyframes.
   - Interpolate all interpolable properties.
   - Use pivot, effect, and display from the **start** keyframe.

3. **At or past the last keyframe** (`tick >= keyframes[last].tick`): The layer is invisible. Nothing is rendered.

This means every layer has a finite visible lifetime defined by its first and last keyframe ticks. The layer becomes invisible **exactly at** the last keyframe's tick. To hold a layer visible through tick N, you must add a keyframe at tick N+1 (or later) with the desired held state.

---

## Sprite State and Keyframe Interaction

The interpolation factor `dt` computed between keyframes is passed directly to the sprite as its **state parameter** `t` (see [SPR: Rendering Modes](spr-format-spec.md#rendering-modes)).

This creates a direct coupling between keyframe timing and sprite animation:

- **TILE sprites**: `dt` is ignored. The sprite draws all its pictures regardless of keyframe position. Use TILE sprites for static layers or layers where only the transform/color changes.

- **ANI sprites**: `dt` selects which frame to display. As the timeline progresses from one keyframe to the next, the sprite cycles through its frames. A keyframe interval that spans the full animation plays all frames once. Shorter intervals play partial frame ranges.

- **PATTERN sprites**: `dt` shifts the tile pattern. The pattern scrolls as the timeline progresses between keyframes.

### Implications

- The sprite's animation is **not independent** — it is locked to keyframe intervals. A sprite does not play at its own speed; it plays at whatever speed the surrounding keyframes dictate.

- To play a sprite animation over a specific duration, place two keyframes at the start and end of that duration. The sprite will receive `dt` from `0.0` to `1.0` over that interval, playing its full frame range.

- To hold a sprite on a specific frame, use two keyframes with the same properties. `dt` will sweep from `0.0` to `1.0`, but if the sprite is TILE mode or has only one picture, the visual stays the same.

- To play a sprite animation multiple times, create multiple keyframe intervals. Each interval resets `dt` to `0.0` at its start keyframe.

---

## Transform Order

When rendering a layer, the transform properties are applied in a specific order. In the logical 640x480 coordinate space (origin at top-left, Y-axis down), the result is equivalent to:

1. Translate to position (`translate_x`, `translate_y`)
2. Move to pivot point (`pivot_x`, `pivot_y`)
3. Apply rotation (`rotate` degrees around the Z axis)
4. Apply scale (`scale_x`, `scale_y`)
5. Move back from pivot point (negate pivot)

The pivot point defines the center of rotation and scaling. A pivot at `(0, 0)` means the sprite rotates and scales around its top-left corner. A pivot at the center of the sprite rotates and scales around its center.


### Pseudocode

```
function render_layer(layer, tick):
    keyframes = layer.keyframes
    if tick < keyframes[0].tick: return  // before first keyframe: invisible
    
    // find the first keyframe with tick > current_tick
    for i from 0 to keyframes.length - 1:
        if keyframes[i].tick > tick:
            // tick is between keyframes[i-1] and keyframes[i]
            // (or if i==0, this shouldn't happen due to the check above,
            //  but would mean exact match at first keyframe)
            
            if i == 0:
                // exact match at first keyframe (tick == keyframes[0].tick)
                if not keyframes[0].display: return
                render_at_keyframe(layer, keyframes[0], state=0.0)
            else:
                // between keyframes[i-1] and keyframes[i]
                if not keyframes[i-1].display: return
                
                dt = (tick - keyframes[i-1].tick) / (keyframes[i].tick - keyframes[i-1].tick)
                
                props = interpolate(keyframes[i-1], keyframes[i], dt)
                props.pivot  = keyframes[i-1].pivot     // not interpolated
                props.effect = keyframes[i-1].effect     // not interpolated

                render_with_props(layer.sprite, props, state=dt)
            return
    
    // if we reach here, tick >= keyframes[last].tick: invisible (past last keyframe)

function interpolate(kf_start, kf_end, dt):
    result.translate_x = lerp(kf_start.translate_x, kf_end.translate_x, dt)
    result.translate_y = lerp(kf_start.translate_y, kf_end.translate_y, dt)
    result.scale_x     = lerp(kf_start.scale_x,     kf_end.scale_x,     dt)
    result.scale_y     = lerp(kf_start.scale_y,     kf_end.scale_y,     dt)
    result.rotate      = lerp(kf_start.rotate,       kf_end.rotate,       dt)
    result.color_r     = lerp(kf_start.color_r,      kf_end.color_r,      dt)
    result.color_g     = lerp(kf_start.color_g,      kf_end.color_g,      dt)
    result.color_b     = lerp(kf_start.color_b,      kf_end.color_b,      dt)
    result.color_a     = lerp(kf_start.color_a,      kf_end.color_a,      dt)
    return result

function render_with_props(sprite, props, state):
    save_transform()
    
    translate(props.translate_x, props.translate_y)
    translate(props.pivot_x, props.pivot_y)
    rotate(props.rotate)
    scale(props.scale_x, props.scale_y)
    translate(-props.pivot_x, -props.pivot_y)
    
    set_color(props.color_r, props.color_g, props.color_b, props.color_a)
    set_blend_mode(props.effect)
    
    sprite.draw(state)
    
    set_blend_mode(NORMAL)
    restore_transform()

function lerp(a, b, t):
    return a + (b - a) * t
```

---

## Rendering Order

Each frame, the animation evaluates all 50 layers in index order:

```
function render_animation(animation, tick):
    if tick < 0: tick = 0
    for layer_index from 0 to 49:
        render_layer(animation.layers[layer_index], tick)
```

This produces a strict painter's algorithm compositing:

- Layer 0 is drawn first (furthest back).
- Layer 49 is drawn last (on top of everything).
- Each layer's blend mode affects how it composites with whatever has already been drawn beneath it.

There is no Z-ordering or dynamic layer sorting. The layer index determines the visual stacking order absolutely.

---

## Limits

| Limit | Value |
|-------|-------|
| Max layers per animation | 50 |
| Max keyframes per layer | 300 |
| Sprite name length | 63 characters |
| Color component range | 0.0–1.0. Values outside this range are undefined behavior. This range was chosen to match OpenGL color calls directly, avoiding extra normalization. |

---

## Example: Composing an Animation

Consider a simple title screen with a background, a logo that fades in, and a pulsing "press start" text:

**Layer 0 — Background** (TILE sprite, static):
```
Keyframe 0: tick=0,   translate=(0,0), scale=(1,1), rotate=0, color=(1,1,1,1), display=on, effect=normal
Keyframe 1: tick=600, translate=(0,0), scale=(1,1), rotate=0, color=(1,1,1,1), display=on, effect=normal
```
Visible for 600 ticks with no changes. The sprite is TILE mode, so `dt` is ignored.

**Layer 1 — Logo** (TILE sprite, fades in):
```
Keyframe 0: tick=30,  translate=(200,100), scale=(1,1), rotate=0, color=(1,1,1,0),   display=on, effect=normal
Keyframe 1: tick=90,  translate=(200,100), scale=(1,1), rotate=0, color=(1,1,1,1),   display=on, effect=normal
Keyframe 2: tick=600, translate=(200,100), scale=(1,1), rotate=0, color=(1,1,1,1),   display=on, effect=normal
```
Invisible before tick 30. Alpha interpolates from 0 to 1 between ticks 30–90 (fade in). Holds at full opacity until tick 600.

**Layer 2 — "Press Start"** (ANI sprite, pulses with additive blend):
```
Keyframe 0: tick=120, translate=(220,400), scale=(1,1), rotate=0, color=(1,1,1,0.5), display=on, effect=screen
Keyframe 1: tick=180, translate=(220,400), scale=(1,1), rotate=0, color=(1,1,1,1),   display=on, effect=screen
Keyframe 2: tick=240, translate=(220,400), scale=(1,1), rotate=0, color=(1,1,1,0.5), display=on, effect=screen
Keyframe 3: tick=300, translate=(220,400), scale=(1,1), rotate=0, color=(1,1,1,1),   display=on, effect=screen
Keyframe 4: tick=360, translate=(220,400), scale=(1,1), rotate=0, color=(1,1,1,0.5), display=on, effect=screen
```
Appears at tick 120 with screen blending. Alpha oscillates between 0.5 and 1.0 across keyframe intervals, creating a pulsing glow. The ANI sprite cycles frames within each 60-tick interval.
