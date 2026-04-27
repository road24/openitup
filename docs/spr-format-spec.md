# SPR File Format Specification

Version: 1.0


---

## Overview

SPR is a plain-text sprite descriptor format that maps regions of image files to screen-space rectangles. It does not contain image data itself -- it references external texture files (PNG, DDS, TGA) and defines how to cut and position them on screen.

A single SPR file describes one **sprite**, which is a collection of up to 64 **pictures**. Each picture maps a rectangular region of a texture to a rectangular region on screen. How the pictures are drawn depends on the sprite's **rendering mode** (all-at-once, frame animation, or tiled grid).

### Design Intent

SPR was designed for a 2D compositing animation system in an arcade rhythm game. It serves as the bridge between authored texture atlases and the keyframe animation engine (BGA). An SPR file tells the engine: "here are the pieces you can animate."

---

## File Structure

SPR files are line-oriented plain text. Each line contains a **directive** followed by whitespace-separated parameters. Lines are parsed top-to-bottom. Blank lines and unrecognized directives are ignored.

Whitespace delimiters: space, tab, carriage return, newline.

Directives are **case-insensitive**.

```
NUM <count>
TYPE <mode> [mode_params...]
T <texture_file> <x> <y> <width> <height> <tex_x1> <tex_y1> <tex_x2> <tex_y2>
```

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

```
NUM <count>
```

Declares the number of pictures in the sprite. Must be the **first directive** in the file, before any `TYPE` or `T` lines.

| Parameter | Type | Description |
|-----------|------|-------------|
| `count` | integer | Declared picture count. **Informational only** -- the actual count is determined by how many `T` lines follow. |

The `count` value is not enforced; parsers should track the actual picture count by counting `T` directives. It exists for human readability and tooling hints.

### TYPE

```
TYPE <mode> [direction] [grid_y] [grid_x]
```

Sets the sprite's rendering mode, which controls how pictures are drawn at render time.

**Parameter order note**: For PATTERN mode, the third parameter is grid_y (rows) and the fourth is grid_x (columns).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `mode` | string | yes | One of: `TILE`, `ANI`, `PATTERN` |
| `direction` | integer | PATTERN only | `0` = horizontal-first iteration, `1` = vertical-first |
| `grid_y` | integer | PATTERN only | Number of rows in the grid |
| `grid_x` | integer | PATTERN only | Number of columns in the grid |

If `TYPE` is omitted or the mode string is unrecognized, the sprite defaults to **TILE** mode.

### T (Picture Entry)

```
T <texture_file> <x> <y> <width> <height> <tex_x1> <tex_y1> <tex_x2> <tex_y2>
```

Defines one picture. Each `T` line appends a picture to the sprite's list.

| Parameter | Type | Unit | Description |
|-----------|------|------|-------------|
| `texture_file` | string | -- | Texture name with `.tga` extension (e.g., `arrow.tga`). See [Texture Lookup](#texture-lookup) below. |
| `x` | integer | pixels | Screen X position (left edge) |
| `y` | integer | pixels | Screen Y position (top edge) |
| `width` | integer | pixels | Width of the screen rectangle |
| `height` | integer | pixels | Height of the screen rectangle |
| `tex_x1` | integer | pixels | Left edge of the source region in the texture |
| `tex_y1` | integer | pixels | Top edge of the source region in the texture |
| `tex_x2` | integer | pixels | Right edge of the source region in the texture |
| `tex_y2` | integer | pixels | Bottom edge of the source region in the texture |

#### Texture Lookup

Texture filenames in SPR files always use the `.tga` extension. The actual image may be stored in any supported format.

When loading a texture, strip the extension from the filename and search for the image by trying each supported format in order until one is found:

```
function load_texture(name):
    base = strip_extension(name)       // "arrow.tga" -> "arrow"
    for ext in [".tga", ".png", ".dds"]:
        if exists(base + ext):
            return load_image(base + ext)
    error("texture not found: " + name)
```

The fallback order and set of supported formats are up to the implementing engine. The key requirement is that `.tga` in the SPR file is treated as a base name hint, not a literal file path.

#### Screen Rectangle

The screen rectangle is computed as:

```
rect.x1 = x
rect.y1 = y
rect.x2 = x + width
rect.y2 = y + height
```

All values are in a **640x480 virtual coordinate space** with top-left origin. Regardless of actual display resolution, coordinates are authored against this fixed grid.

#### Texture Coordinates

Texture coordinates are specified as **absolute pixel positions** within the source image. To convert to normalized UV coordinates (0.0 to 1.0), divide by the actual dimensions of the loaded texture:

```
u1 = tex_x1 / texture_width
v1 = tex_y1 / texture_height
u2 = tex_x2 / texture_width
v2 = tex_y2 / texture_height
```

This means the texture must be loaded (or its dimensions known) before UV coordinates can be resolved. The SPR file encodes pixel positions, not normalized values.

---

## Rendering Modes

The `TYPE` directive determines how the sprite's pictures are drawn. All modes receive a **state parameter** `t`, a normalized value where `0.0` represents the initial state and `1.0` represents the final state.

Sprites have no concept of time, frames, or duration. The `t` parameter is an abstract progress value -- what it maps to is entirely up to the calling system. The animation layer above the sprite decides how to produce `t`. For example:

- A keyframe animation system might compute `t` as the interpolation factor between two keyframes: `t = (current_tick - keyframe_start) / (keyframe_end - keyframe_start)`.
- A gameplay system might pass `0.0` to always show the first state, or cycle `t` based on a game clock.
- A tool or editor might let the user scrub `t` manually.

This separation means sprite rendering is stateless and deterministic -- the same `t` always produces the same visual output. All timing, easing, looping, and sequencing belong to the layer that drives the sprite.

> **Implementation note**: A keyframe-based animation engine can exploit this by computing `t` relative to the interval between two keyframes. This way the sprite's frame selection is tied to animation progress rather than wall-clock time, enabling variable playback speed, reverse playback, and time-remapping without any changes to the sprite system itself.

### TILE

Draw **all pictures simultaneously**, layered back-to-front. The state parameter is ignored.

Pictures are drawn in reverse order (last defined picture drawn first, first defined picture drawn last), producing a natural back-to-front layering where the first picture in the file appears on top.

```
function draw_tile(sprite, t):
    for i from (sprite.num_pictures - 1) down to 0:
        draw_picture(sprite.pictures[i])
```

Use cases: static multi-layer compositions, UI elements with overlapping parts, base displays.

### ANI

Draw **one picture** selected by the state parameter. This is frame-by-frame animation.

```
function draw_ani(sprite, t):
    frame = floor(sprite.num_pictures * t)
    frame = min(frame, sprite.num_pictures - 1)
    draw_picture(sprite.pictures[frame])
```

At `t=0.0`, the first picture is drawn. At `t=1.0`, the last picture is drawn. The state parameter maps linearly across all pictures.

Use cases: animated arrows, blinking indicators, cycling effects.

### PATTERN

Tile pictures in a **grid**, with state-based cycling through the picture list. Requires three additional parameters from the `TYPE` directive: `direction`, `grid_x`, `grid_y`.

Each grid cell draws one picture, offset from the origin by the first picture's dimensions. The state parameter shifts which picture appears in each cell.

```
function draw_pattern(sprite, t):
    cell_width  = sprite.pictures[0].rect.x2 - sprite.pictures[0].rect.x1
    cell_height = sprite.pictures[0].rect.y2 - sprite.pictures[0].rect.y1

    state_offset = floor(sprite.num_pictures * (1.0 - t))

    if direction == 0:   // horizontal-first
        for row from 0 to grid_y - 1:
            for col from 0 to grid_x - 1:
                index = (col + grid_x * row + state_offset) mod sprite.num_pictures
                draw_picture_at(sprite.pictures[index],
                                offset_x = cell_width * col,
                                offset_y = cell_height * row)

    else:                // vertical-first
        for col from 0 to grid_x - 1:
            for row from 0 to grid_y - 1:
                index = (col * grid_y + row + state_offset) mod sprite.num_pictures
                draw_picture_at(sprite.pictures[index],
                                offset_x = cell_width * col,
                                offset_y = cell_height * row)
```

Use cases: scrolling tile backgrounds, pattern-fill effects.

---

## Coordinate System

### Authoring Space

SPR files are authored in a **640x480 top-left origin** coordinate space:

```
(0,0) ---- X+ ----> (640,0)
  |
  Y+
  |
  v
(0,480)              (640,480)
```

Screen rectangle values (`x`, `y`, `width`, `height`) are in this space.

### Rendering Adaptation

If your engine uses a different coordinate convention (e.g., bottom-left origin), flip the Y axis when rendering:

```
rendered_y = 480 - authored_y
```

If your engine uses a different resolution, scale all screen coordinates proportionally:

```
scale_x = your_width  / 640.0
scale_y = your_height / 480.0
rendered_x = authored_x * scale_x
rendered_y = authored_y * scale_y
```

---

## Limits

| Limit | Value | Notes |
|-------|-------|-------|
| Max pictures per sprite | 64 | |
| Texture filename length | 31 characters | |
| Max line length | 256 characters | |

---

## Example Files

### Static multi-layer sprite (TILE)

```
NUM 3
TYPE TILE
T background.tga 0 0 640 480 0 0 512 512
T frame.tga 100 50 440 380 0 0 256 256
T icon.tga 280 200 80 80 0 0 64 64
```

This sprite draws three layers simultaneously: a background, a frame on top, and an icon centered over the frame.

### Animated sprite (ANI)

```
NUM 6
TYPE ANI
T arrows.tga 0 0 49 60 0 0 49 60
T arrows.tga 0 0 49 60 49 0 98 60
T arrows.tga 0 0 49 60 98 0 147 60
T arrows.tga 0 0 49 60 147 0 196 60
T arrows.tga 0 0 49 60 196 0 245 60
T arrows.tga 0 0 49 60 245 0 294 60
```

Six frames cut from a horizontal strip in `arrows.png`. At `t=0.0`, frame 0 is shown. At `t=0.5`, frame 3 is shown.

### Grid pattern (PATTERN)

```
NUM 4
TYPE PATTERN 0 2 3
T tile_a.tga 0 0 64 64 0 0 64 64
T tile_b.tga 0 0 64 64 0 0 64 64
T tile_c.tga 0 0 64 64 0 0 64 64
T tile_d.tga 0 0 64 64 0 0 64 64
```

A 3x2 grid of 64x64 tiles (2 rows, 3 columns), iterated horizontally. As `t` changes, tiles cycle through the four pictures.

---

## Implementation Guide

### Parsing

```
function load_spr(file_path, texture_loader):
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
            texture = texture_loader.load(tokens[1])

            pic.rect.x1 = parse_int(tokens[2])
            pic.rect.y1 = parse_int(tokens[3])
            pic.rect.x2 = pic.rect.x1 + parse_int(tokens[4])
            pic.rect.y2 = pic.rect.y1 + parse_int(tokens[5])

            tex_w = texture.width
            tex_h = texture.height

            pic.uv.u1 = parse_int(tokens[6])  / tex_w
            pic.uv.v1 = parse_int(tokens[7])  / tex_h
            pic.uv.u2 = parse_int(tokens[8])  / tex_w
            pic.uv.v2 = parse_int(tokens[9])  / tex_h

            pic.texture = texture
            sprite.pictures[sprite.num_pictures] = pic
            sprite.num_pictures += 1

    return sprite
```

### Rendering

```
function draw_sprite(sprite, t):
    if sprite.mode == TILE:
        draw_tile(sprite, t)
    else if sprite.mode == ANI:
        draw_ani(sprite, t)
    else if sprite.mode == PATTERN:
        draw_pattern(sprite, t)

function draw_picture(pic):
    bind_texture(pic.texture)
    draw_textured_quad(
        screen_rect = pic.rect,
        uv_rect     = pic.uv
    )
```

### Porting Checklist

1. **Texture loading**: Texture filenames in SPR always use `.tga`. Your loader must strip the extension and search for the actual image across supported formats (`.tga`, `.png`, `.dds`, etc.). Loaded textures must provide their pixel dimensions for UV normalization.
2. **Coordinate mapping**: Map the 640x480 authored coordinates to your engine's coordinate system and resolution.
3. **Y-axis flip**: If your engine uses bottom-left origin, invert Y coordinates (`480 - y`).
4. **Draw order**: TILE mode draws back-to-front (last picture in the list drawn first).
5. **State parameter**: ANI and PATTERN modes expect a normalized `t` in `[0.0, 1.0]`. Your animation or keyframe system computes this value -- the sprite itself has no concept of time.
6. **Texture atlas support**: Multiple pictures can reference the same texture file with different source regions. Avoid loading the same texture multiple times.
7. **Alpha blending**: Sprites are designed to be rendered with alpha blending enabled. Ensure your renderer handles transparency.
