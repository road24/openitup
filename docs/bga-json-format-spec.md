# BGA JSON File Format Specification

Version: 1.0


---

## Overview

This document defines a JSON representation of BGA animations as an alternative to the [binary BGA format](bga-binary-format-spec.md). The animation model, keyframe semantics, interpolation rules, and rendering behavior are identical — see the [BGA Animation Specification](bga-animation-spec.md).

The JSON format uses the `.bgaj` file extension.

---

## Schema

The complete JSON Schema (Draft 2020-12):

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "BGA Animation",
  "description": "A BGA animation file containing layers with keyframed sprite animations.",
  "type": "object",
  "required": ["version", "layers"],
  "additionalProperties": false,
  "properties": {
    "version": {
      "const": 2,
      "description": "Format version. Must be 2."
    },
    "layers": {
      "type": "array",
      "description": "Animation layers, ordered back-to-front. Layer 0 renders first (behind), last layer renders on top.",
      "minItems": 0,
      "maxItems": 50,
      "items": {
        "$ref": "#/$defs/layer"
      }
    }
  },
  "$defs": {
    "layer": {
      "type": "object",
      "description": "A single animation layer bound to one sprite.",
      "required": ["sprite", "keyframes"],
      "additionalProperties": false,
      "properties": {
        "sprite": {
          "type": "string",
          "description": "Sprite filename (.spr, .sp2, or .tga). Empty string for inactive layers.",
          "maxLength": 63
        },
        "keyframes": {
          "type": "array",
          "description": "Keyframe events ordered by ascending tick value.",
          "minItems": 0,
          "maxItems": 300,
          "items": {
            "$ref": "#/$defs/keyframe"
          }
        }
      }
    },
    "keyframe": {
      "type": "object",
      "description": "A keyframe capturing the complete state of a layer at a specific tick.",
      "required": [
        "tick",
        "translate",
        "pivot",
        "scale",
        "rotate",
        "color",
        "display",
        "effect"
      ],
      "additionalProperties": false,
      "properties": {
        "tick": {
          "type": "integer",
          "minimum": 0,
          "maximum": 65535,
          "description": "Timeline position."
        },
        "translate": {
          "type": "array",
          "description": "Position offset [x, y] in 640x480 coordinate space.",
          "items": { "type": "number" },
          "minItems": 2,
          "maxItems": 2
        },
        "pivot": {
          "type": "array",
          "description": "Rotation/scale pivot point [x, y] relative to the sprite's origin. Not interpolated — value is taken from the start keyframe of each interval.",
          "items": { "type": "number" },
          "minItems": 2,
          "maxItems": 2
        },
        "scale": {
          "type": "array",
          "description": "Scale factors [x, y]. 1.0 = original size.",
          "items": { "type": "number" },
          "minItems": 2,
          "maxItems": 2
        },
        "rotate": {
          "type": "number",
          "description": "Rotation angle in degrees."
        },
        "color": {
          "type": "array",
          "description": "Color and opacity [r, g, b, a]. Each component 0.0–1.0. Values outside this range are undefined behavior.",
          "items": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          },
          "minItems": 4,
          "maxItems": 4
        },
        "display": {
          "type": "boolean",
          "description": "Visibility flag. false = layer hidden from this keyframe until the next keyframe that sets it to true. Not interpolated — value is taken from the start keyframe of each interval."
        },
        "effect": {
          "type": "string",
          "description": "Blend mode for compositing. Not interpolated — value is taken from the start keyframe of each interval.",
          "enum": ["normal", "screen", "multiply", "dodge", "difference"]
        }
      }
    }
  }
}
```

---

## Differences from Binary Format

| Aspect | Binary (.bga) | JSON (.bgaj) |
|--------|---------------|---------------|
| Layer count | Fixed 50 (inactive layers stored with empty/space-prefixed names) | Variable array (omit inactive layers) |
| Inactive layers | Stored with empty or space-prefixed sprite name (event count irrelevant) | Simply absent from the array |
| Display flag | int16 (0 = hidden, non-zero = visible) | Boolean (`true`/`false`) |
| Effect/blend mode | Integer 0–4 | String enum: `"normal"`, `"screen"`, `"multiply"`, `"dodge"`, `"difference"` |
| 2D properties | Flat fields (`translate_x`, `translate_y`) | Arrays (`"translate": [x, y]`) |
| Color | Four separate floats | Array (`"color": [r, g, b, a]`) |
| Reserved fields | 14 bytes per event, 12 bytes in header | Not present |
| Byte order | Little-endian | N/A (text) |

### Key Design Decisions

**Variable layer count**: The binary format always stores 50 layers, using empty sprite names for inactive ones. The JSON format omits inactive layers entirely. Layer index in the array determines rendering order (index 0 = back, last = front).

**String enums for effect**: Numeric blend mode IDs are replaced with readable string constants. The mapping is:

| Binary | JSON |
|--------|------|
| 0 | `"normal"` |
| 1 | `"screen"` |
| 2 | `"multiply"` |
| 3 | `"dodge"` |
| 4 | `"difference"` |

**Boolean display**: The binary int16 visibility flag is replaced with a JSON boolean.

**Grouped vectors**: Related x/y pairs are grouped into arrays for readability and to reduce field count.

---

## Example

```json
{
  "version": 2,
  "layers": [
    {
      "sprite": "title_bg.tga",
      "keyframes": [
        {
          "tick": 0,
          "translate": [0, 0],
          "pivot": [0, 0],
          "scale": [1, 1],
          "rotate": 0,
          "color": [1, 1, 1, 1],
          "display": true,
          "effect": "normal"
        },
        {
          "tick": 600,
          "translate": [0, 0],
          "pivot": [0, 0],
          "scale": [1, 1],
          "rotate": 0,
          "color": [1, 1, 1, 1],
          "display": true,
          "effect": "normal"
        }
      ]
    },
    {
      "sprite": "logo.spr",
      "keyframes": [
        {
          "tick": 30,
          "translate": [200, 100],
          "pivot": [0, 0],
          "scale": [1, 1],
          "rotate": 0,
          "color": [1, 1, 1, 0],
          "display": true,
          "effect": "normal"
        },
        {
          "tick": 90,
          "translate": [200, 100],
          "pivot": [0, 0],
          "scale": [1, 1],
          "rotate": 0,
          "color": [1, 1, 1, 1],
          "display": true,
          "effect": "normal"
        },
        {
          "tick": 600,
          "translate": [200, 100],
          "pivot": [0, 0],
          "scale": [1, 1],
          "rotate": 0,
          "color": [1, 1, 1, 1],
          "display": true,
          "effect": "normal"
        }
      ]
    },
    {
      "sprite": "press_start.sp2",
      "keyframes": [
        {
          "tick": 120,
          "translate": [220, 400],
          "pivot": [0, 0],
          "scale": [1, 1],
          "rotate": 0,
          "color": [1, 1, 1, 0.5],
          "display": true,
          "effect": "screen"
        },
        {
          "tick": 180,
          "translate": [220, 400],
          "pivot": [0, 0],
          "scale": [1, 1],
          "rotate": 0,
          "color": [1, 1, 1, 1],
          "display": true,
          "effect": "screen"
        },
        {
          "tick": 240,
          "translate": [220, 400],
          "pivot": [0, 0],
          "scale": [1, 1],
          "rotate": 0,
          "color": [1, 1, 1, 0.5],
          "display": true,
          "effect": "screen"
        },
        {
          "tick": 300,
          "translate": [220, 400],
          "pivot": [0, 0],
          "scale": [1, 1],
          "rotate": 0,
          "color": [1, 1, 1, 1],
          "display": true,
          "effect": "screen"
        },
        {
          "tick": 360,
          "translate": [220, 400],
          "pivot": [0, 0],
          "scale": [1, 1],
          "rotate": 0,
          "color": [1, 1, 1, 0.5],
          "display": true,
          "effect": "screen"
        }
      ]
    }
  ]
}
```

This example matches the title screen composition from the [BGA Animation Specification](bga-animation-spec.md#example-composing-an-animation): a static background, a logo that fades in via alpha interpolation, and a pulsing text with screen blending.

---

## Conversion

### Binary to JSON

```
function bga_to_json(binary_animation):
    effect_names = ["normal", "screen", "multiply", "dodge", "difference"]
    
    json = { "version": 2, "layers": [] }

    for layer in binary_animation.layers:
        // Skip inactive layers (empty name or starts with space)
        if layer.sprite_name is empty or layer.sprite_name[0] == ' ': continue

        json_layer = {
            "sprite": layer.sprite_name,
            "keyframes": []
        }

        for event in layer.events:
            json_keyframe = {
                "tick":      event.tick,
                "translate": [event.translate_x, event.translate_y],
                "pivot":     [event.pivot_x, event.pivot_y],
                "scale":     [event.scale_x, event.scale_y],
                "rotate":    event.rotate,
                "color":     [event.color_r, event.color_g, event.color_b, event.color_a],
                "display":   event.display != 0,
                "effect":    effect_names[event.effect]
            }
            json_layer.keyframes.append(json_keyframe)

        json.layers.append(json_layer)

    return json
```

### JSON to Binary

```
function json_to_bga(json):
    effect_ids = { "normal": 0, "screen": 1, "multiply": 2, "dodge": 3, "difference": 4 }
    
    binary = new BinaryAnimation()

    // Binary format requires exactly 50 layers
    for i from 0 to 49:
        if i < json.layers.length:
            // Copy JSON layer to binary layer i
            json_layer = json.layers[i]
            binary.layers[i].sprite_name = json_layer.sprite
            binary.layers[i].num_events  = json_layer.keyframes.length

            for j, kf in enumerate(json_layer.keyframes):
                event = new Event()
                event.translate_x = kf.translate[0]
                event.translate_y = kf.translate[1]
                event.pivot_x     = kf.pivot[0]
                event.pivot_y     = kf.pivot[1]
                event.scale_x     = kf.scale[0]
                event.scale_y     = kf.scale[1]
                event.rotate      = kf.rotate
                event.color_r     = kf.color[0]
                event.color_g     = kf.color[1]
                event.color_b     = kf.color[2]
                event.color_a     = kf.color[3]
                event.tick        = kf.tick
                event.display     = 1 if kf.display else 0
                event.effect      = effect_ids[kf.effect]
                binary.layers[i].events[j] = event
        else:
            // Fill remaining slots with inactive layers
            binary.layers[i].sprite_name = ""
            binary.layers[i].num_events  = 0

    return binary
```

---

## Limits

| Limit | Value |
|-------|-------|
| Max layers per animation | 50 |
| Max keyframes per layer | 300 |
| Sprite name length | 63 characters |
| Tick range | 0 to 65535 |
| Color component range | 0.0–1.0. Values outside this range are undefined behavior. |
