# SP2 File Format Specification

Version: 1.0


---

## Overview

SP2 is an extended version of the [SPR format](spr-format-spec.md). Like SPR, it is a plain-text sprite descriptor that maps regions of image files to screen-space rectangles. SP2 adds two features:

1. **Named pictures** -- each picture has an identifier that can be referenced by external systems.
2. **Fixed-grid texture coordinates** -- texture UVs are computed against a 256x256 virtual atlas, and specified as position + size offsets rather than absolute edges.

SP2 shares the same rendering modes (TILE, ANI, PATTERN), coordinate system, state parameter semantics, and texture lookup rules as SPR. This document only covers what differs. For shared concepts, refer to the [SPR specification](spr-format-spec.md).

---

## File Structure

SP2 files are line-oriented plain text, identical in structure to SPR:

```
NUM <count>
TYPE <mode> [mode_params...]
T <pic_name> <texture_file> <x> <y> <width> <height> <tex_x> <tex_y> <tex_w> <tex_h>
```

Whitespace delimiters: space, tab, carriage return, newline.

Directives are **case-insensitive**.

### Directive Order

Files follow a fixed structure:

```
NUM <count>
TYPE <mode> [...]
T ...
T ...
T ...
```

`NUM` is always the first directive. `TYPE` follows, then all `T` entries. Each directive appears once per file (except `T`, which repeats per picture).

---

## Directives

### NUM

Identical to SPR. See [SPR: NUM](spr-format-spec.md#num).

### TYPE

Identical to SPR. See [SPR: TYPE](spr-format-spec.md#type).

### T (Picture Entry)

```
T <pic_name> <texture_file> <x> <y> <width> <height> <tex_x> <tex_y> <tex_w> <tex_h>
```

Defines one named picture. Each `T` line appends a picture to the sprite's list.

| Parameter | Type | Unit | Description |
|-----------|------|------|-------------|
| `pic_name` | string | -- | Identifier for this picture (e.g., `arrow_left`, `frame_top`) |
| `texture_file` | string | -- | Texture name with `.tga` extension. See [SPR: Texture Lookup](spr-format-spec.md#texture-lookup). |
| `x` | integer | pixels | Screen X position (left edge) |
| `y` | integer | pixels | Screen Y position (top edge) |
| `width` | integer | pixels | Width of the screen rectangle |
| `height` | integer | pixels | Height of the screen rectangle |
| `tex_x` | integer | atlas units | X offset of the source region in the texture atlas |
| `tex_y` | integer | atlas units | Y offset of the source region in the texture atlas |
| `tex_w` | integer | atlas units | Width of the source region in the texture atlas |
| `tex_h` | integer | atlas units | Height of the source region in the texture atlas |

#### Picture Name

The `pic_name` field is the key difference from SPR. It provides a stable identifier for each picture, allowing external systems (animation engines, scripting, tooling) to reference individual pictures by name rather than by index.

Picture names are limited to 30 characters.

#### Screen Rectangle

Screen rectangles are computed identically to SPR. Both formats parse position and size, then compute the far edge as an offset from the base:

```
rect.x1 = x
rect.y1 = y
rect.x2 = x + width
rect.y2 = y + height
```

All values are in a **640x480 virtual coordinate space** with top-left origin.

#### Texture Coordinates

This is the major difference from SPR. SP2 texture coordinates use a **256x256 virtual atlas grid** and are specified as **position + size offsets**, not absolute edges.

The four texture parameters define a rectangle within the atlas:

```
u1 = tex_x / 256.0
v1 = tex_y / 256.0
u2 = u1 + tex_w / 256.0
v2 = v1 + tex_h / 256.0
```

Note the additive relationship: `u2` and `v2` are computed by adding the width/height offsets to the base coordinates, not parsed independently.

#### Why 256?

The 256x256 atlas grid is a fixed convention. All SP2 texture coordinates are authored against this grid regardless of the actual texture dimensions. This means:

- A 256x256 texture maps 1:1 with atlas coordinates.
- A 512x512 texture would only use the top-left quadrant if coordinates stay within 0-255.
- The actual texture dimensions do not affect UV computation.

This differs from SPR, where texture coordinates are absolute pixel positions divided by the actual texture size.

---

## Differences from SPR

| Aspect | SPR | SP2 |
|--------|-----|-----|
| `T` line first field | `texture_file` | `pic_name` (then `texture_file`) |
| Picture names | Not supported | Each picture has a name identifier |
| Texture coord encoding | Absolute edge positions (x1, y1, x2, y2) | Position + size offsets (x, y, w, h) |
| Texture coord parsing | Four independent values | Base position + additive width/height |
| UV normalization | Divide by actual texture dimensions | Divide by fixed 256 |
| UV computation | `u1 = tex_x1 / tex_width`<br>`u2 = tex_x2 / tex_width` | `u1 = tex_x / 256`<br>`u2 = u1 + tex_w / 256` |

---

## Example Files

### Named multi-layer sprite (TILE)

```
NUM 3
TYPE TILE
T bg_layer background.tga 0 0 640 480 0 0 256 256
T frame_layer frame.tga 100 50 440 380 0 0 128 128
T icon_center icon.tga 280 200 80 80 0 0 64 64
```

Three named layers. External systems can reference `bg_layer`, `frame_layer`, or `icon_center` by name.

### Named animation frames (ANI)

```
NUM 4
TYPE ANI
T walk_0 character.tga 0 0 64 96 0 0 64 96
T walk_1 character.tga 0 0 64 96 64 0 64 96
T walk_2 character.tga 0 0 64 96 128 0 64 96
T walk_3 character.tga 0 0 64 96 192 0 64 96
```

Four frames from a horizontal strip in a 256x256 atlas. Each frame is 64x96 atlas units, starting at `tex_x` = 0, 64, 128, 192.

UV for `walk_1`:

```
u1 = 64 / 256 = 0.25
v1 = 0 / 256  = 0.0
u2 = 0.25 + 64 / 256 = 0.5
v2 = 0.0 + 96 / 256  = 0.375
```

---

## Implementation Guide

### Parsing

```
function load_sp2(file_path, texture_loader):
    sprite = new Sprite()
    sprite.mode = TILE          // default
    sprite.num_pictures = 0

    for each line in file:
        tokens = split(line, whitespace)
        if tokens is empty: continue

        directive = to_upper(tokens[0])

        if directive == "NUM":
            // informational only; actual count comes from T lines
            continue

        else if directive == "TYPE":
            // identical to SPR — see SPR spec
            mode_str = to_upper(tokens[1])
            if mode_str == "TILE":
                sprite.mode = TILE
            else if mode_str == "ANI":
                sprite.mode = ANI
            else if mode_str == "PATTERN":
                sprite.mode = PATTERN
                sprite.direction = parse_int(tokens[2])
                sprite.grid_y    = parse_int(tokens[3])  // rows
                sprite.grid_x    = parse_int(tokens[4])  // columns
            else:
                sprite.mode = TILE  // default for unrecognized modes

        else if directive == "T":
            pic = new Picture()
            pic.name    = tokens[1]                        // picture name
            texture     = texture_loader.load(tokens[2])   // texture file

            pic.rect.x1 = parse_int(tokens[3])
            pic.rect.y1 = parse_int(tokens[4])
            pic.rect.x2 = pic.rect.x1 + parse_int(tokens[5])
            pic.rect.y2 = pic.rect.y1 + parse_int(tokens[6])

            pic.uv.u1 = parse_int(tokens[7])  / 256.0
            pic.uv.v1 = parse_int(tokens[8])  / 256.0
            pic.uv.u2 = pic.uv.u1 + parse_int(tokens[9])  / 256.0
            pic.uv.v2 = pic.uv.v1 + parse_int(tokens[10]) / 256.0

            pic.texture = texture
            sprite.pictures[sprite.num_pictures] = pic
            sprite.num_pictures += 1

    return sprite
```

### Rendering

Rendering is identical to SPR. See [SPR: Rendering](spr-format-spec.md#rendering).

### Porting Checklist

1. **Texture loading**: Same `.tga` lookup rules as SPR. See [SPR: Texture Lookup](spr-format-spec.md#texture-lookup).
2. **UV computation**: Divide by 256, not by actual texture dimensions. Coordinates are position + size offsets, not absolute edges.
3. **Picture names**: Store the `pic_name` field if your system needs to reference pictures by name. If not needed, it can be ignored.
4. **Coordinate mapping**: Same 640x480 virtual space as SPR. See [SPR: Coordinate System](spr-format-spec.md#coordinate-system).
5. **State parameter**: Same semantics as SPR. See [SPR: Rendering Modes](spr-format-spec.md#rendering-modes).
6. **Draw order**: Same as SPR -- TILE mode draws back-to-front.
7. **Alpha blending**: Sprites are designed to be rendered with alpha blending enabled.

---

## Limits

| Limit | Value |
|-------|-------|
| Max pictures per sprite | 64 |
| Picture name length | 30 characters |
| Texture filename length | 31 characters |
| Max line length | 256 characters |
