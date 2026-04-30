# Noteskin Format Specification

## Overview

A noteskin defines the visual representation of all arrows, receptors, and feedback animations used during gameplay. Each noteskin is a directory containing SPRJ sprite animation files following a deterministic naming convention. No manifest file is required — the engine discovers assets by filename.

Noteskins reuse the existing SPRJ sprite animation format. Each arrow graphic is a 6-frame looping animation at 64×64 pixels per frame, cycling every 300ms (50ms per frame).

## Directory Structure

Noteskins live under the `noteskin/` directory. Each subdirectory is a separate skin:

```
noteskin/
  default/
    ARROW00_TAP.sprj
    ARROW01_TAP.sprj
    ...
  my-awesome-ns/
    ARROW00_TAP.sprj
    ...
```

The engine discovers available skins by scanning `noteskin/` for subdirectories. The `default/` skin is required and used as fallback when a selected skin is missing.

## Naming Convention

All asset filenames follow this pattern:

```
ARROW##_TYPE[_SUBTYPE].sprj
```

Where:
- `##` — two-digit track number (see Track Mapping below)
- `TYPE` — the arrow category
- `SUBTYPE` — type-specific variant (omitted when not applicable)

### Track Mapping

| Track | Direction | Panel Position |
|-------|-----------|---------------|
| 00 | Down-Left | Bottom-left |
| 01 | Up-Left | Top-left |
| 02 | Center | Center |
| 03 | Up-Right | Top-right |
| 04 | Down-Right | Bottom-right |

Only tracks 00–04 exist. Double mode and half-double mode reuse the same 5 arrow assets for both sides.

## Arrow Types

### TAP — Regular Arrows

Standard tap notes. No subtype.

```
ARROW00_TAP.sprj
ARROW01_TAP.sprj
ARROW02_TAP.sprj
ARROW03_TAP.sprj
ARROW04_TAP.sprj
```

### FAKETAP — Fake Arrows

Arrows that should not be stepped on. Visually distinct from regular taps.

```
ARROW00_FAKETAP.sprj
ARROW01_FAKETAP.sprj
ARROW02_FAKETAP.sprj
ARROW03_FAKETAP.sprj
ARROW04_FAKETAP.sprj
```

### LONG — Hold Notes

Hold notes consist of three parts: the head (where the hold starts), the body (stretched between head and tail), and the tail (where the hold ends).

```
ARROW00_LONG_HEAD.sprj
ARROW00_LONG_BODY.sprj
ARROW00_LONG_TAIL.sprj
ARROW01_LONG_HEAD.sprj
ARROW01_LONG_BODY.sprj
ARROW01_LONG_TAIL.sprj
...
ARROW04_LONG_TAIL.sprj
```

The body sprite is tiled vertically between head and tail positions during rendering.

### OTHER — Division Mode Notes

Special notes used in division mode gameplay. Hitting W or G branches the chart difficulty up or down respectively. Typically rendered as the letters "W" and "G".

```
ARROW00_OTHER_W.sprj
ARROW00_OTHER_G.sprj
ARROW01_OTHER_W.sprj
ARROW01_OTHER_G.sprj
...
ARROW04_OTHER_G.sprj
```

### ITEM — Gameplay Items (Future)

Item notes (X1, X2, X3, X4, X8, POTION, ACTION, ROCKET, mine, etc.) are defined as a future nice-to-have. Their subtypes and gameplay effects will be specified in the gameplay mechanics specifications (REQ-GPD series). The naming convention extends naturally:

```
ARROW00_ITEM_X1.sprj
ARROW00_ITEM_MINE.sprj
ARROW00_ITEM_POTION.sprj
...
```

These are NOT required for Phase 2.

## Receptor Area

The receptor area is where arrows arrive and the player presses panels. It consists of three overlapping animation layers rendered in order (back to front):

1. **Receptor background** — always playing, looping animation
2. **Press overlay** — plays on the column when the user presses the corresponding button (visual input acknowledgment)
3. **Judge overlay** — plays on the column when an arrow is judged Perfect, Great, or Good

All three layers render simultaneously when active. They overlay, not replace each other.

### Receptor Background

Receptors are shared across all tracks (not per-arrow). Three variants exist for the three play modes:

```
ARROW_RECEPTOR_SINGLE.sprj
ARROW_RECEPTOR_DOUBLE.sprj
ARROW_RECEPTOR_HALF.sprj
```

Each receptor SPRJ contains the full receptor bar for its mode (5 panels for single, 10 for double, the appropriate subset for half-double). The receptor animation plays continuously regardless of gameplay state.

### Press Animation

Per-track animation triggered on button press:

```
ARROW00_PRESS.sprj
ARROW01_PRESS.sprj
ARROW02_PRESS.sprj
ARROW03_PRESS.sprj
ARROW04_PRESS.sprj
```

Plays once when the corresponding panel is pressed. Provides immediate visual feedback that the engine recognized the input.

### Judge Animation

Per-track animation triggered on successful judgment:

```
ARROW00_JUDGE.sprj
ARROW01_JUDGE.sprj
ARROW02_JUDGE.sprj
ARROW03_JUDGE.sprj
ARROW04_JUDGE.sprj
```

Plays once when an arrow on the corresponding column is judged Perfect, Great, or Good. Does NOT play on Bad or Miss judgments.

## Animation Properties

All SPRJ files in a noteskin follow these animation rules:

| Property | Value |
|----------|-------|
| Frame count | 6 |
| Frame duration | 50ms |
| Full loop duration | 300ms (6 × 50ms) |
| Frame dimensions | 64×64 pixels |
| Loop behavior | Continuous loop for receptors and scrolling arrows; single play for press and judge |

The SPRJ format handles frame selection via the normalized `t` parameter. For looping arrows, `t` is driven by a global animation timer. For one-shot animations (press, judge), `t` advances from 0.0 to 1.0 over 300ms and then the animation becomes inactive.

## Play Modes

### Single Mode

Uses tracks 00–04. Five columns mapped left to right:

```
Column 0: Track 00 (Down-Left)
Column 1: Track 01 (Up-Left)
Column 2: Track 02 (Center)
Column 3: Track 03 (Up-Right)
Column 4: Track 04 (Down-Right)
```

Receptor: `ARROW_RECEPTOR_SINGLE.sprj`

### Double Mode

Uses tracks 00–04 for both the P1 side (columns 0–4) and P2 side (columns 5–9). The same arrow SPRJ files are reused — column 5 uses ARROW00, column 6 uses ARROW01, etc.

```
Column 0: Track 00    Column 5: Track 00
Column 1: Track 01    Column 6: Track 01
Column 2: Track 02    Column 7: Track 02
Column 3: Track 03    Column 8: Track 03
Column 4: Track 04    Column 9: Track 04
```

Receptor: `ARROW_RECEPTOR_DOUBLE.sprj`

### Half-Double Mode

Uses a subset of columns from double mode (typically columns 1–3 from P1 and columns 5–7 from P2, skipping the outer panels). Same arrow SPRJ files reused.

Receptor: `ARROW_RECEPTOR_HALF.sprj`

## Complete File Listing (Phase 2 Required)

A minimal Phase 2 noteskin contains the following files:

**Tap arrows** (5 files):
`ARROW00_TAP.sprj` through `ARROW04_TAP.sprj`

**Fake arrows** (5 files):
`ARROW00_FAKETAP.sprj` through `ARROW04_FAKETAP.sprj`

**Hold notes** (15 files):
`ARROW00_LONG_HEAD.sprj` through `ARROW04_LONG_TAIL.sprj`

**Division mode** (10 files):
`ARROW00_OTHER_W.sprj` through `ARROW04_OTHER_G.sprj`

**Receptors** (3 files):
`ARROW_RECEPTOR_SINGLE.sprj`, `ARROW_RECEPTOR_DOUBLE.sprj`, `ARROW_RECEPTOR_HALF.sprj`

**Press feedback** (5 files):
`ARROW00_PRESS.sprj` through `ARROW04_PRESS.sprj`

**Judge feedback** (5 files):
`ARROW00_JUDGE.sprj` through `ARROW04_JUDGE.sprj`

**Total: 48 SPRJ files per noteskin.**

## Missing Asset Handling

| Missing Asset | Behavior |
|---------------|----------|
| Entire skin directory | Fall back to `default/` skin, log warning |
| Individual SPRJ file | Log warning with filename, render nothing for that element |
| `default/` skin missing | Fatal error at startup — cannot render gameplay |

## Example Directory

```
noteskin/default/
├── ARROW00_TAP.sprj
├── ARROW01_TAP.sprj
├── ARROW02_TAP.sprj
├── ARROW03_TAP.sprj
├── ARROW04_TAP.sprj
├── ARROW00_FAKETAP.sprj
├── ARROW01_FAKETAP.sprj
├── ARROW02_FAKETAP.sprj
├── ARROW03_FAKETAP.sprj
├── ARROW04_FAKETAP.sprj
├── ARROW00_LONG_HEAD.sprj
├── ARROW00_LONG_BODY.sprj
├── ARROW00_LONG_TAIL.sprj
├── ARROW01_LONG_HEAD.sprj
├── ARROW01_LONG_BODY.sprj
├── ARROW01_LONG_TAIL.sprj
├── ARROW02_LONG_HEAD.sprj
├── ARROW02_LONG_BODY.sprj
├── ARROW02_LONG_TAIL.sprj
├── ARROW03_LONG_HEAD.sprj
├── ARROW03_LONG_BODY.sprj
├── ARROW03_LONG_TAIL.sprj
├── ARROW04_LONG_HEAD.sprj
├── ARROW04_LONG_BODY.sprj
├── ARROW04_LONG_TAIL.sprj
├── ARROW00_OTHER_W.sprj
├── ARROW00_OTHER_G.sprj
├── ARROW01_OTHER_W.sprj
├── ARROW01_OTHER_G.sprj
├── ARROW02_OTHER_W.sprj
├── ARROW02_OTHER_G.sprj
├── ARROW03_OTHER_W.sprj
├── ARROW03_OTHER_G.sprj
├── ARROW04_OTHER_W.sprj
├── ARROW04_OTHER_G.sprj
├── ARROW_RECEPTOR_SINGLE.sprj
├── ARROW_RECEPTOR_DOUBLE.sprj
├── ARROW_RECEPTOR_HALF.sprj
├── ARROW00_PRESS.sprj
├── ARROW01_PRESS.sprj
├── ARROW02_PRESS.sprj
├── ARROW03_PRESS.sprj
├── ARROW04_PRESS.sprj
├── ARROW00_JUDGE.sprj
├── ARROW01_JUDGE.sprj
├── ARROW02_JUDGE.sprj
├── ARROW03_JUDGE.sprj
└── ARROW04_JUDGE.sprj
```
