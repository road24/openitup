# Sprite JSON File Format Specification

Version: 1.0


---

## Overview

This document defines a unified JSON representation for sprite descriptors, replacing both the [SPR](spr-format-spec.md) and [SP2](sp2-format-spec.md) text formats. The rendering model, state parameter semantics, and coordinate system are identical — see the [SPR specification](spr-format-spec.md) for those details.

The JSON format uses the `.sprj` file extension.

### Design Goals

- **Unified**: A single schema handles both SPR and SP2 data. Format-specific details (UV normalization method, picture names) are resolved at conversion time.
- **Direct coordinates**: Screen rectangles are stored as absolute pixel coordinates. Texture UVs are stored as normalized values (0.0–1.0), resolved from the source format's encoding at conversion time.
- **Lossless round-trip**: SPR and SP2 files can be converted to JSON and back without data loss. The `source_format` field preserves which text format the data originated from.

---

## Schema

The complete JSON Schema (Draft 2020-12):

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Sprite Descriptor",
  "description": "A sprite descriptor mapping texture regions to screen-space rectangles.",
  "type": "object",
  "required": ["source_format", "mode", "pictures"],
  "additionalProperties": false,
  "properties": {
    "source_format": {
      "type": "string",
      "description": "Original text format this was converted from. Used for lossless round-trip conversion.",
      "enum": ["spr", "sp2"]
    },
    "mode": {
      "type": "string",
      "description": "Rendering mode that controls how pictures are drawn.",
      "enum": ["tile", "ani", "pattern"]
    },
    "pattern": {
      "type": "object",
      "description": "PATTERN mode parameters. Must be present when mode is 'pattern', must be absent for other modes. JSON Schema does not support conditional requirements across properties, so validators should check this constraint separately.",
      "required": ["direction", "grid_x", "grid_y"],
      "additionalProperties": false,
      "properties": {
        "direction": {
          "type": "string",
          "description": "Grid iteration order.",
          "enum": ["horizontal", "vertical"]
        },
        "grid_x": {
          "type": "integer",
          "description": "Number of columns in the grid.",
          "minimum": 1
        },
        "grid_y": {
          "type": "integer",
          "description": "Number of rows in the grid.",
          "minimum": 1
        }
      }
    },
    "pictures": {
      "type": "array",
      "description": "Ordered list of pictures. Rendering order depends on the mode. May be empty, though source formats typically contain at least one picture.",
      "minItems": 0,
      "maxItems": 64,
      "items": {
        "$ref": "#/$defs/picture"
      }
    }
  },
  "$defs": {
    "picture": {
      "type": "object",
      "description": "A single picture mapping a texture region to a screen rectangle.",
      "required": ["texture", "rect", "uv"],
      "additionalProperties": false,
      "properties": {
        "name": {
          "type": "string",
          "description": "Picture identifier. Present when converted from SP2, absent when converted from SPR.",
          "maxLength": 30
        },
        "texture": {
          "type": "string",
          "description": "Texture filename with .tga extension. The .tga extension is a base name hint — the actual image may be in any supported format (.tga, .png, .dds).",
          "maxLength": 31
        },
        "rect": {
          "type": "array",
          "description": "Screen rectangle [x1, y1, x2, y2] in pixels. Origin is top-left.",
          "items": { "type": "integer" },
          "minItems": 4,
          "maxItems": 4
        },
        "uv": {
          "type": "array",
          "description": "Normalized texture coordinates [u1, v1, u2, v2] in 0.0–1.0 range.",
          "items": { "type": "number" },
          "minItems": 4,
          "maxItems": 4
        }
      }
    }
  }
}
```

---

## Differences from Text Formats

| Aspect | SPR | SP2 | JSON (.sprj) |
|--------|-----|-----|---------------|
| Picture names | Not supported | Required | Optional (`name` field) |
| Screen rect encoding | Position + size (must compute x2=x1+w, y2=y1+h) | Position + size (must compute x2=x1+w, y2=y1+h) | Absolute pixel coordinates `[x1, y1, x2, y2]` |
| Texture coord encoding | Absolute pixel positions (x1, y1, x2, y2) | Atlas units (position + size: x, y, w, h) | Normalized UVs `[u1, v1, u2, v2]` |
| UV normalization | Divide by actual texture dimensions | Divide by fixed 256 | Already normalized (0.0–1.0) |
| Rendering mode | String directive (`TILE`, `ANI`, `PATTERN`) | Same | Lowercase string enum (`"tile"`, `"ani"`, `"pattern"`) |
| PATTERN direction | Integer (0 = horizontal, 1 = vertical) | Same | String enum (`"horizontal"`, `"vertical"`) |
| PATTERN grid params | Packed into TYPE line | Same | Separate `grid_x`, `grid_y` fields |

### Key Design Decisions

**Direct coordinates**: Screen rectangles are absolute pixel coordinates with top-left origin. Texture UVs are normalized 0.0–1.0 values. The text formats encode these differently (position+size for rects, format-specific normalization for UVs), but the JSON format stores them directly, eliminating the need for texture dimensions or format knowledge at load time.

**Optional picture names**: SPR pictures have no names. SP2 pictures require names. The JSON format makes `name` optional — present when the data comes from SP2, absent when from SPR.

**Source format tracking**: The `source_format` field records the original text format. This enables lossless round-trip conversion: when converting back to a text format, the converter knows whether to write SPR (pixel-based UVs, no names) or SP2 (atlas-based UVs, with names).

**String enums**: Numeric direction values and uppercase mode strings are replaced with readable lowercase constants.

---

## Example

### Converted from SPR (TILE)

```json
{
  "source_format": "spr",
  "mode": "tile",
  "pictures": [
    {
      "texture": "background.tga",
      "rect": [0, 0, 640, 480],
      "uv": [0.0, 0.0, 1.0, 1.0]
    },
    {
      "texture": "frame.tga",
      "rect": [100, 50, 540, 430],
      "uv": [0.0, 0.0, 1.0, 1.0]
    },
    {
      "texture": "icon.tga",
      "rect": [280, 200, 360, 280],
      "uv": [0.0, 0.0, 1.0, 1.0]
    }
  ]
}
```

### Converted from SP2 (ANI)

```json
{
  "source_format": "sp2",
  "mode": "ani",
  "pictures": [
    {
      "name": "walk_0",
      "texture": "character.tga",
      "rect": [0, 0, 64, 96],
      "uv": [0.0, 0.0, 0.25, 0.375]
    },
    {
      "name": "walk_1",
      "texture": "character.tga",
      "rect": [0, 0, 64, 96],
      "uv": [0.25, 0.0, 0.5, 0.375]
    },
    {
      "name": "walk_2",
      "texture": "character.tga",
      "rect": [0, 0, 64, 96],
      "uv": [0.5, 0.0, 0.75, 0.375]
    },
    {
      "name": "walk_3",
      "texture": "character.tga",
      "rect": [0, 0, 64, 96],
      "uv": [0.75, 0.0, 1.0, 0.375]
    }
  ]
}
```

### PATTERN mode

```json
{
  "source_format": "spr",
  "mode": "pattern",
  "pattern": {
    "direction": "horizontal",
    "grid_x": 3,
    "grid_y": 2
  },
  "pictures": [
    {
      "texture": "tile_a.tga",
      "rect": [0, 0, 64, 64],
      "uv": [0.0, 0.0, 1.0, 1.0]
    },
    {
      "texture": "tile_b.tga",
      "rect": [0, 0, 64, 64],
      "uv": [0.0, 0.0, 1.0, 1.0]
    },
    {
      "texture": "tile_c.tga",
      "rect": [0, 0, 64, 64],
      "uv": [0.0, 0.0, 1.0, 1.0]
    },
    {
      "texture": "tile_d.tga",
      "rect": [0, 0, 64, 64],
      "uv": [0.0, 0.0, 1.0, 1.0]
    }
  ]
}
```

---

## Conversion

### SPR to JSON

```
function spr_to_json(spr, texture_loader):
    json = {
        "source_format": "spr",
        "mode": spr.mode_as_lowercase_string(),
        "pictures": []
    }

    if spr.mode == PATTERN:
        json.pattern = {
            "direction": "horizontal" if spr.direction == 0 else "vertical",
            "grid_x": spr.grid_x,
            "grid_y": spr.grid_y
        }

    for pic in spr.pictures:
        # pic.texture_file is the string from the SPR file
        # pic.x, pic.y, pic.width, pic.height are the screen rect values
        # pic.tex_x1, pic.tex_y1, pic.tex_x2, pic.tex_y2 are absolute pixel coords
        
        tex_w = texture_loader.get_width(pic.texture_file)
        tex_h = texture_loader.get_height(pic.texture_file)

        json_pic = {
            "texture": pic.texture_file,
            "rect": [pic.x, pic.y, pic.x + pic.width, pic.y + pic.height],
            "uv": [
                pic.tex_x1 / tex_w,
                pic.tex_y1 / tex_h,
                pic.tex_x2 / tex_w,
                pic.tex_y2 / tex_h
            ]
        }
        json.pictures.append(json_pic)

    return json
```

### SP2 to JSON

```
function sp2_to_json(sp2):
    json = {
        "source_format": "sp2",
        "mode": sp2.mode_as_lowercase_string(),
        "pictures": []
    }

    if sp2.mode == PATTERN:
        json.pattern = {
            "direction": "horizontal" if sp2.direction == 0 else "vertical",
            "grid_x": sp2.grid_x,
            "grid_y": sp2.grid_y
        }

    for pic in sp2.pictures:
        # pic.name is the picture identifier
        # pic.texture_file is the string from the SP2 file
        # pic.x, pic.y, pic.width, pic.height are the screen rect values
        # pic.tex_x, pic.tex_y, pic.tex_w, pic.tex_h are atlas units (0-255 range)
        
        u1 = pic.tex_x / 256.0
        v1 = pic.tex_y / 256.0

        json_pic = {
            "name": pic.name,
            "texture": pic.texture_file,
            "rect": [pic.x, pic.y, pic.x + pic.width, pic.y + pic.height],
            "uv": [
                u1,
                v1,
                u1 + pic.tex_w / 256.0,
                v1 + pic.tex_h / 256.0
            ]
        }
        json.pictures.append(json_pic)

    return json
```

### JSON to SPR

Converting back to SPR requires knowing texture dimensions to reverse the UV normalization. The texture must be loaded (or its dimensions known from metadata) to compute the absolute pixel coordinates that SPR expects.

```
function json_to_spr(json, texture_loader):
    spr = ""
    spr += "NUM " + str(len(json.pictures)) + "\n"

    mode = json.mode.upper()
    if mode == "PATTERN":
        dir = 0 if json.pattern.direction == "horizontal" else 1
        spr += "TYPE PATTERN " + str(dir) + " " + str(json.pattern.grid_y) + " " + str(json.pattern.grid_x) + "\n"
    else:
        spr += "TYPE " + mode + "\n"

    for pic in json.pictures:
        tex_w = texture_loader.get_width(pic.texture)
        tex_h = texture_loader.get_height(pic.texture)

        x      = pic.rect[0]
        y      = pic.rect[1]
        width  = pic.rect[2] - pic.rect[0]
        height = pic.rect[3] - pic.rect[1]

        tex_x1 = round(pic.uv[0] * tex_w)
        tex_y1 = round(pic.uv[1] * tex_h)
        tex_x2 = round(pic.uv[2] * tex_w)
        tex_y2 = round(pic.uv[3] * tex_h)

        spr += "T " + pic.texture + " "
        spr += str(x) + " " + str(y) + " " + str(width) + " " + str(height) + " "
        spr += str(tex_x1) + " " + str(tex_y1) + " " + str(tex_x2) + " " + str(tex_y2) + "\n"

    return spr
```

### JSON to SP2

```
function json_to_sp2(json):
    sp2 = ""
    sp2 += "NUM " + str(len(json.pictures)) + "\n"

    mode = json.mode.upper()
    if mode == "PATTERN":
        dir = 0 if json.pattern.direction == "horizontal" else 1
        sp2 += "TYPE PATTERN " + str(dir) + " " + str(json.pattern.grid_y) + " " + str(json.pattern.grid_x) + "\n"
    else:
        sp2 += "TYPE " + mode + "\n"

    for pic in json.pictures:
        # SP2 requires picture names. If converting from SPR-sourced JSON,
        # names will be absent — generate a synthetic name in that case.
        name   = pic.name or "unnamed"
        x      = pic.rect[0]
        y      = pic.rect[1]
        width  = pic.rect[2] - pic.rect[0]
        height = pic.rect[3] - pic.rect[1]

        tex_x = round(pic.uv[0] * 256.0)
        tex_y = round(pic.uv[1] * 256.0)
        tex_w = round((pic.uv[2] - pic.uv[0]) * 256.0)
        tex_h = round((pic.uv[3] - pic.uv[1]) * 256.0)

        sp2 += "T " + name + " " + pic.texture + " "
        sp2 += str(x) + " " + str(y) + " " + str(width) + " " + str(height) + " "
        sp2 += str(tex_x) + " " + str(tex_y) + " " + str(tex_w) + " " + str(tex_h) + "\n"

    return sp2
```

---

## Limits

These limits match the constraints from the SPR and SP2 source formats:

| Limit | Value | Notes |
|-------|-------|-------|
| Max pictures per sprite | 64 | Enforced by schema `maxItems` |
| Picture name length | 30 characters | Enforced by schema `maxLength` |
| Texture filename length | 31 characters | Enforced by schema `maxLength` |

The 256-character line length limit from the text formats does not apply to JSON.
