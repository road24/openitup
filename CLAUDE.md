# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

openitup is an open-source Pump It Up engine reimplementation targeting the Exceed (2003) era. C++ game engine using SDL3. Cross-platform target, Linux-first development.

## Project Structure

```
docs/           — format specifications and documentation
src/            — engine source code
test/           — tests
test/fixtures/  — SPRJ/BGAJ fixtures, PNG textures, reference snapshots
tools/          — standalone tools (bga_player)
```

## Architecture: 2D Compositing Animation Stack

The engine's visual system is a three-layer stack. Understanding the ownership boundaries is essential:

1. **Sprites** (SPR/SP2 → unified JSON `.sprj`) — Stateless picture collections. A sprite maps texture regions to screen rectangles in 640×480 space. Sprites receive a normalized `t` parameter but own no concept of time. Three rendering modes:
   - **TILE**: draw all pictures back-to-front, `t` ignored
   - **ANI**: `frame = floor(num_pictures * t)`, single frame selection
   - **PATTERN**: tile pictures into a grid, `t` shifts which picture fills each cell

2. **BGA Animations** (binary `.bga` → JSON `.bgaj`) — Keyframed compositions of up to 50 layers, each bound to one sprite. Layers are rendered in index order (painter's algorithm). The keyframe interpolation factor `dt` is passed directly to the sprite as its `t` parameter — sprite animation speed is entirely dictated by keyframe spacing, not by the sprite itself.

3. **Texture loading** — `.tga` in all format files is a base name hint, not a literal path. Loaders strip the extension and probe `.tga`, `.png`, `.dds` in order.

### Critical details easy to get wrong

- **SPR vs SP2 UV encoding**: SPR uses absolute pixel coords divided by actual texture dimensions. SP2 uses position+size offsets divided by fixed 256. The JSON format normalizes both to 0.0–1.0.
- **PATTERN TYPE line param order**: `TYPE PATTERN <direction> <grid_y> <grid_x>` — rows before columns.
- **TILE draw order**: reverse iteration (last picture drawn first, first picture appears on top).
- **Non-interpolated keyframe properties**: `pivot`, `effect` (blend mode), and `display` (visibility) are taken from the **start** keyframe of each interval — they snap, not blend.
- **Layer visibility window**: a layer is invisible before its first keyframe tick AND at/past its last keyframe tick. To hold visible through tick N, the last keyframe must be at tick N+1 or later.
- **BGA binary format**: always exactly 50 layers (inactive = empty or space-prefixed sprite name); little-endian; 16-byte header with `BGA2` magic; 64-byte keyframe event records with 14 bytes reserved padding.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DSDL_X11_XSCRNSAVER=OFF
cmake --build build -j$(nproc)
```

All dependencies (SDL3, SDL3_image, nlohmann/json, GoogleTest) are fetched via CMake FetchContent. The `-DSDL_X11_XSCRNSAVER=OFF` flag is needed unless `libxss-dev` is installed.

### Run tests

```bash
cd build && ctest --output-on-failure
```

### Run a single test

```bash
cd build && ctest --output-on-failure -R "TestName"
```

### BGA Player tool

```bash
# Interactive playback
./build/bga_player test/fixtures/title_screen.bgaj

# Render a single frame to PNG
./build/bga_player test/fixtures/title_screen.bgaj --snapshot 60 output.png
```

### Format converters

```bash
# SPR -> SPRJ (requires texture files present for UV normalization)
./build/spr2sprj input.spr output.sprj
./build/spr2sprj input.spr --asset-dir /path/to/textures output.sprj

# SP2 -> SPRJ (no texture files needed — fixed 256 divisor)
./build/spr2sprj input.sp2 output.sprj

# BGA binary -> BGAJ (pure binary parsing, no dependencies)
./build/bga2bgaj input.bga output.bgaj
```

SPR conversion will fail with an error if referenced textures cannot be found — this is intentional, since UV normalization requires actual texture dimensions and a fallback would produce incorrect data.

### Regenerate test textures

```bash
cmake --build build --target generate_samples
cd test/fixtures && ../../build/generate_samples
```

## Testing

Three test tiers:

1. **Unit tests** — Pure math/logic, no SDL: keyframe interpolation, lerp, sprite mode frame selection, texture probe order, JSON round-trips.

2. **Integration tests** (`test_integration.cpp`) — Generate textures and fixtures in `/tmp` at runtime, render to offscreen targets, verify pixel values. Tests basic rendering pipeline correctness.

3. **Regression tests** (`test_regression.cpp`) — Load committed BGAJ/SPRJ fixtures from `test/fixtures/`, render at specific ticks, compare pixel-by-pixel against 30 reference PNGs in `test/fixtures/reference/` (tolerance ≤ 2 per channel). Covers every animation property:

   **Interpolated (should change smoothly):** translate, scale, rotate, color RGB, color alpha

   **Not interpolated (should snap at keyframe boundary):** pivot, display, effect/blend mode

   **Spec rules:** visibility window (invisible before first tick, invisible at/past last tick), ANI frame selection by dt, layer compositing order (painter's algorithm)

   To update references after an intentional rendering change:
   ```bash
   cd test/fixtures
   # Re-run bga_player --snapshot for each (bgaj, tick) pair
   # See the generate commands in the commit that added the references
   ```

## Project Tracking

**`docs/stories/STATUS.md`** is the master tracking matrix for the entire project. It lists every user story with its phase, status, points, and dependencies. **Update this file whenever a story's status changes** (e.g., PLANNED → DONE).

Related documents:
- `docs/engine-roadmap.md` — Architecture plan and implementation phases
- `docs/requirements/` — Business requirements by subsystem (136 total)
- `docs/stories/` — User stories by subsystem (290 total, ~832 story points)

## Format Specs

All specs live in `docs/`:
- `spr-format-spec.md` — SPR text format (original sprite format)
- `sp2-format-spec.md` — SP2 text format (named pictures, 256-grid UVs)
- `sprite-json-format-spec.md` — Unified JSON sprite format (.sprj)
- `bga-binary-format-spec.md` — BGA binary layout
- `bga-animation-spec.md` — BGA animation model (keyframes, interpolation, rendering)
- `bga-json-format-spec.md` — BGA JSON format (.bgaj)
