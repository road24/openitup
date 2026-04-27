# BGA Binary File Format Specification

Version: 1.0


---

## Overview

This document describes the binary layout of `.BGA` files. For the animation model, keyframe semantics, interpolation rules, and rendering behavior, see the [BGA Animation Specification](bga-animation-spec.md).

A `.BGA` file stores one animation: a fixed array of 50 layers, each with a sprite reference and a variable-length sequence of keyframe events.

---

## Byte Order

All multi-byte values are stored in **little-endian** byte order.

---

## File Layout

```
+------------------+
| Header (16 bytes)|
+------------------+
| Layer 0          |
+------------------+
| Layer 1          |
+------------------+
| ...              |
+------------------+
| Layer 49         |
+------------------+
```

The file contains a 16-byte header followed by exactly **50 layers** in sequence. Layers are stored in index order (0–49), which corresponds to their rendering order (back to front).

---

## Header

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0 | 4 | char[4] | Magic number: `BGA2` (bytes `0x42 0x47 0x41 0x32`) |
| 4 | 12 | int32_t[3] | Reserved. Must be read and discarded (3 × 4-byte integers). |

**Total: 16 bytes**

The magic number identifies the file as BGA version 2. Files that do not begin with `BGA2` are not valid.

---

## Layer

Each layer is stored as a sprite name, a keyframe count, and then that many keyframe event records:

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0 | 64 | char[64] | Sprite filename, null-padded (e.g., `title_bg.spr`, `logo.tga`) |
| 64 | 4 | int32 | `num_events` — number of keyframe events that follow |
| 68 | num_events * 64 | event[num_events] | Keyframe event records |

**Total per layer: 68 + (num_events * 64) bytes**

### Sprite Filename

A 64-byte field containing the sprite filename as a null-terminated string, padded with zeros. The filename determines how the layer's visual content is loaded:

- `.spr` — loaded as an SPR sprite descriptor
- `.sp2` — loaded as an SP2 sprite descriptor
- `.tga` — loaded as a raw texture (see [BGA Animation Spec: Raw Texture Layers](bga-animation-spec.md#raw-texture-layers))

An inactive layer has no visual content and is skipped during rendering regardless of its keyframe data. A layer is inactive if the sprite filename is an empty string (first byte is `0x00`) or begins with a space character (`0x20`).

### Event Count

A signed 32-bit integer specifying the number of keyframe events. The parser clamps negative values to 1 (treating corrupted data gracefully). A value of 0 means the layer has no keyframes and will not render.

---

## Keyframe Event Record

Each keyframe event is a fixed **64-byte** record:

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 4 | float | `translate_x` | Horizontal position offset |
| 4 | 4 | float | `translate_y` | Vertical position offset |
| 8 | 4 | float | `pivot_x` | Rotation/scale pivot X |
| 12 | 4 | float | `pivot_y` | Rotation/scale pivot Y |
| 16 | 4 | float | `scale_x` | Horizontal scale factor |
| 20 | 4 | float | `scale_y` | Vertical scale factor |
| 24 | 4 | float | `rotate` | Rotation angle in degrees |
| 28 | 4 | float | `color_r` | Red tint (0.0–1.0) |
| 32 | 4 | float | `color_g` | Green tint (0.0–1.0) |
| 36 | 4 | float | `color_b` | Blue tint (0.0–1.0) |
| 40 | 4 | float | `color_a` | Alpha/opacity (0.0–1.0) |
| 44 | 2 | uint16 | `tick` | Timeline position |
| 46 | 2 | int16 | `display` | Visibility flag (0 = hidden, non-zero = visible) |
| 48 | 2 | int16 | `effect` | Blend mode (0–4, see below) |
| 50 | 14 | char[14] | `reserved` | Reserved, should be zeros |

**Total: 64 bytes per event**

### Field Details

**`translate_x`, `translate_y`** — Position offset in 640x480 coordinate space.

**`pivot_x`, `pivot_y`** — The point around which rotation and scaling are applied. Coordinates are relative to the sprite's origin.

**`scale_x`, `scale_y`** — Scale factors. `1.0` = original size.

**`rotate`** — Rotation angle in degrees.

**`color_r`, `color_g`, `color_b`, `color_a`** — Color and opacity multipliers. Values outside 0.0–1.0 are undefined behavior. The range matches direct OpenGL color calls, avoiding extra normalization.

**`tick`** — Unsigned 16-bit integer marking where this keyframe sits on the timeline. Tick values must be >= 0. Keyframes within a layer must be ordered by ascending tick value.

**`display`** — Visibility flag. When `0`, the layer is hidden from this keyframe until the next keyframe that sets it to non-zero.

**`effect`** — Blend mode selector:

| Value | Mode |
|-------|------|
| 0 | Normal |
| 1 | Screen |
| 2 | Multiply |
| 3 | Color Dodge |
| 4 | Difference |

See [BGA Animation Spec: Blend Modes](bga-animation-spec.md#blend-modes) for descriptions.

**`reserved`** — 14 bytes of padding. Should be zero-filled when writing.

---

## Parsing Pseudocode

```
function parse_bga(stream):
    magic = read_bytes(stream, 4)
    if magic != "BGA2":
        error("invalid BGA file")

    read_bytes(stream, 12)                  // skip reserved header

    animation = new Animation()

    for layer_index from 0 to 49:
        layer = new Layer()

        layer.sprite_name = read_string(stream, 64)     // null-padded
        layer.num_events  = read_int32(stream)

        if layer.num_events < 0:
            layer.num_events = 1            // clamp corrupted data to 1

        for event_index from 0 to layer.num_events - 1:
            event = new Event()

            event.translate_x = read_float(stream)
            event.translate_y = read_float(stream)
            event.pivot_x     = read_float(stream)
            event.pivot_y     = read_float(stream)
            event.scale_x     = read_float(stream)
            event.scale_y     = read_float(stream)
            event.rotate      = read_float(stream)
            event.color_r     = read_float(stream)
            event.color_g     = read_float(stream)
            event.color_b     = read_float(stream)
            event.color_a     = read_float(stream)
            event.tick        = read_uint16(stream)
            event.display     = read_int16(stream)
            event.effect      = read_int16(stream)
            read_bytes(stream, 14)                       // skip reserved

            layer.events[event_index] = event

        animation.layers[layer_index] = layer

    return animation
```

---

## Limits

| Limit | Value |
|-------|-------|
| Layers per file | 50 (fixed array, always present in file) |
| Max events per layer | 300 |
| Sprite name field size | 64 bytes |
| Event record size | 64 bytes |
| Tick range | 0 to 65535 (uint16) |

---

## Size Calculation

The minimum file size (all layers with 0 events):

```
16 + 50 * (64 + 4) = 16 + 3400 = 3416 bytes
```

The maximum file size (all 50 layers with 300 events each):

```
16 + 50 * (64 + 4 + 300 * 64) = 16 + 50 * 19268 = 963416 bytes (~941 KB)
```
