# OSF Chart Format Specification

**Version**: 1.0  
**Format Name**: OSF (OpenItUp Step File)  
**File Extension**: `.osf`  
**Encoding**: UTF-8 JSON

## Overview

OSF is the canonical chart format for the OpenItUp engine. It is:
- **Human-readable**: JSON text format suitable for version control
- **Comprehensive**: Represents all features from legacy formats (KSF, SSC, SMA, etc.)
- **Extensible**: Supports future features without breaking compatibility
- **Deterministic**: One chart per file, stable content hashing

Unlike multi-chart formats (SSC, SMA), each `.osf` file contains exactly one difficulty chart. Song packs with multiple difficulties use multiple `.osf` files.

## File Structure

An OSF file is a JSON object with four top-level sections:

```json
{
  "version": "1.0",
  "metadata": { ... },
  "timing_events": [ ... ],
  "notes": [ ... ]
}
```

## Schema

### Top Level

| Field           | Type     | Required | Description                                    |
|-----------------|----------|----------|------------------------------------------------|
| `version`       | string   | Yes      | Format version (currently "1.0")               |
| `metadata`      | object   | Yes      | Chart and song metadata                        |
| `timing_events` | array    | Yes      | BPM changes, stops (can be empty)              |
| `notes`         | array    | Yes      | Note events (can be empty for metadata-only)   |

### Metadata Object

| Field                    | Type    | Required | Description                                        |
|--------------------------|---------|----------|----------------------------------------------------|
| `title`                  | string  | Yes      | Song title (UTF-8)                                 |
| `artist`                 | string  | No       | Artist name (UTF-8)                                |
| `genre`                  | string  | No       | Music genre                                        |
| `charter_name`           | string  | No       | Chart author                                       |
| `difficulty_name`        | string  | No       | "Easy", "Normal", "Hard", "Crazy", etc.            |
| `difficulty_rating`      | int     | No       | Numeric difficulty (1-28 classic, higher modern)   |
| `mode`                   | string  | Yes      | "SINGLE" or "DOUBLE"                               |
| `audio_path`             | string  | No       | Relative path to audio file                        |
| `intro_path`             | string  | No       | Relative path to intro/demo audio                  |
| `banner_path`            | string  | No       | Relative path to banner image                      |
| `background_path`        | string  | No       | Relative path to background image                  |
| `display_bpm`            | number  | No       | Display BPM for UI (may differ from timing)        |
| `start_time_ms`          | number  | No       | Audio offset in milliseconds (default 0.0)         |
| `preview_start_seconds`  | number  | No       | Preview start position in seconds (default -1.0)   |
| `preview_length_seconds` | number  | No       | Preview duration in seconds (default -1.0 = 15.0)  |

**Mode Values**:
- `"SINGLE"` — 5 columns (indices 0-4)
- `"DOUBLE"` — 10 columns (indices 0-9)

### Timing Events Array

Each timing event is an object:

| Field           | Type    | Required | Description                                    |
|-----------------|---------|----------|------------------------------------------------|
| `type`          | string  | Yes      | "BPM_CHANGE" or "STOP"                         |
| `beat`          | number  | Yes      | Beat position (double precision)               |
| `bpm`           | number  | If type=BPM_CHANGE | New BPM value (beats per minute) |
| `stop_duration` | number  | If type=STOP | Stop duration in seconds              |

Events are sorted by beat position. BPM changes at the same beat appear before stops.

**Example**:
```json
"timing_events": [
  { "type": "BPM_CHANGE", "beat": 0.0, "bpm": 120.0 },
  { "type": "BPM_CHANGE", "beat": 8.0, "bpm": 180.0 },
  { "type": "STOP", "beat": 16.0, "stop_duration": 1.0 }
]
```

### Notes Array

Each note event is an object:

| Field    | Type   | Required | Description                                 |
|----------|--------|----------|---------------------------------------------|
| `beat`   | number | Yes      | Beat position (double precision)            |
| `column` | int    | Yes      | Column index (0-4 for SINGLE, 0-9 for DOUBLE) |
| `type`   | string | Yes      | Note type (see below)                       |

**Note Types**:
- `"TAP"` — Standard tap note
- `"HOLD_HEAD"` — Start of a hold note
- `"HOLD_TAIL"` — End of a hold note
- `"MINE"` — Mine (penalty for hitting)
- `"FAKE"` — Fake note (no judgment)
- `"LIFT"` — Lift note (release panel)

Notes are sorted by beat, then column, then type.

**Example**:
```json
"notes": [
  { "beat": 0.0, "column": 2, "type": "TAP" },
  { "beat": 1.0, "column": 1, "type": "HOLD_HEAD" },
  { "beat": 3.0, "column": 1, "type": "HOLD_TAIL" },
  { "beat": 4.0, "column": 3, "type": "TAP" }
]
```

## Minimal Example

```json
{
  "version": "1.0",
  "metadata": {
    "title": "Pumptris",
    "artist": "BanYa",
    "mode": "SINGLE",
    "difficulty_name": "Normal",
    "difficulty_rating": 5,
    "audio_path": "pumptris.ogg"
  },
  "timing_events": [
    { "type": "BPM_CHANGE", "beat": 0.0, "bpm": 140.0 }
  ],
  "notes": [
    { "beat": 0.0, "column": 2, "type": "TAP" },
    { "beat": 1.0, "column": 1, "type": "TAP" },
    { "beat": 2.0, "column": 3, "type": "TAP" }
  ]
}
```

## Validation Rules

1. **Version**: Must be "1.0" (future versions will increment)
2. **Metadata**:
   - `title` must not be empty
   - `mode` must be "SINGLE" or "DOUBLE"
3. **Timing Events**:
   - At least one BPM_CHANGE event required (typically at beat 0.0)
   - BPM values must be > 0
   - Beats must be >= 0.0
4. **Notes**:
   - Column indices must be valid for the mode (0-4 for SINGLE, 0-9 for DOUBLE)
   - HOLD_HEAD notes should have corresponding HOLD_TAIL notes
   - Beats must be >= 0.0

Validation warnings are logged but do not prevent loading. See `chart_validator.h` for implementation.

## Serialization Format

- **Pretty-printed**: 2-space indentation for human readability
- **Field order**: Fixed order within objects (version first, metadata second, etc.)
- **Floating-point precision**: Preserved from source (no rounding)
- **UTF-8 encoding**: All string fields support international characters

## Content Hashing

OSF files participate in the chart content hash system (see `chart-hashing-spec.md`). The hash is computed from notes and timing events only — metadata changes do not affect the hash. This allows scores to persist across format conversions and metadata updates.

## Future Extensions

Future versions may add:
- `scroll_speed_changes` array for visual scroll speed effects
- `per_note_metadata` for custom note properties (e.g., Lua hooks)
- `gameplay_modifiers` for chart-specific mods (e.g., forced noteskin)

The `version` field will increment when incompatible changes are introduced. Parsers should check the version and reject unsupported formats.

## See Also

- `bga-json-format-spec.md` — BGA animation format (`.bgaj`)
- `sprite-json-format-spec.md` — Sprite format (`.sprj`)
- `chart-hashing-spec.md` — Chart content hashing algorithm
