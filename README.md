# openitup

An open-source rhythm game engine compatible with 5-panel dance game formats.

openitup is a community-driven engine built from scratch for 5-panel rhythm gameplay, with support for multiple chart formats and eras.

This is not a theme or skin — it's a standalone engine with its own rendering pipeline, audio system, and judgment logic.

> **Important: Want to contribute?**
> Please read [Contributing](#contributing) and [Support This Project](#support-this-project) before starting any work. **Open a discussion first** — this project uses AI-driven development with a specific architecture, and uncoordinated PRs will likely be rejected.

> **Status: Early Development**
> The animation and rendering foundation is complete and tested. Gameplay systems (audio, input, judge, note scrolling) are next. See [Current Progress](#current-progress) below.

## What It Does (Eventually)

- Reads chart formats: KSF, SSC, SMA, STX, SEE, NX, and a new open format (OSF)
- Supports keyboard, USB dance pads, and arcade I/O boards
- Single, Double, Co-op, and Battle play modes
- Per-version judge rules (timing windows, scoring, life gauge) loaded from JSON profiles
- Low-latency audio with sample-accurate sync for precise judgment timing
- Lua scripting for screen flow and UI, keeping the core engine in C++ for performance
- Online leaderboards and score submission (planned)
- Cross-platform: Linux-first, Windows supported

## What It Does Right Now

The **BGA (Background Animation)** subsystem is fully implemented. This is the visual foundation — every screen in openitup is built from BGA animations and sprite compositions.

- Load and render sprite sheets (SPR, SP2, SPRJ formats) and keyframed animations (BGA binary, BGAJ)
- Three sprite rendering modes: TILE (layered), ANI (frame animation), PATTERN (tiled grid)
- Keyframe interpolation for translate, scale, rotate, color, and alpha at 60 ticks/sec with sub-tick precision
- Five blend modes (normal, screen/additive, multiply, color dodge, difference)
- Case-insensitive texture probing (.tga, .png, .dds)
- 82 automated tests including pixel-level visual regression against reference images

### Tools

- **bga_player** — Interactive BGA animation viewer with pause, frame stepping, speed control, and PNG snapshot export for automated testing
- **spr2sprj** — Converts SPR and SP2 sprite files to the JSON-based SPRJ format
- **bga2bgaj** — Converts BGA binary animation files to the JSON-based BGAJ format

## Building

Requirements: a C++20 compiler (GCC 12+, Clang 15+, or MSVC 2022+), CMake 3.24+, and a working internet connection for the first build (dependencies are downloaded automatically).

```bash
git clone https://github.com/road24/openitup.git
cd openitup
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DSDL_X11_XSCRNSAVER=OFF
cmake --build build -j$(nproc)
```

The `-DSDL_X11_XSCRNSAVER=OFF` flag avoids a dependency on `libxss-dev`. If you have it installed, you can omit this flag.

All dependencies are fetched automatically via CMake FetchContent:

| Dependency | Purpose |
|-----------|---------|
| SDL3 | Windowing, rendering, input |
| SDL3_image | Image format loading (PNG, TGA, etc.) |
| nlohmann/json | JSON parsing for SPRJ/BGAJ formats |
| spdlog | Structured logging |
| CLI11 | Command-line argument parsing |
| GoogleTest | Testing framework |

### Running Tests

```bash
cd build && ctest --output-on-failure
```

### Using the BGA Player

```bash
# Interactive playback (Space=pause, Left/Right=step, Home=restart, Escape=quit)
./build/bga_player path/to/animation.bgaj

# Render a single frame to PNG
./build/bga_player path/to/animation.bgaj --snapshot 60 output.png

# Load original binary BGA files directly
./build/bga_player path/to/animation.BGA

# Verbose logging for debugging
./build/bga_player path/to/animation.bgaj --log-level trace
```

### Using the Format Converters

```bash
# SPR to SPRJ (texture files must be present for UV normalization)
./build/spr2sprj input.spr output.sprj --asset-dir /path/to/textures

# SP2 to SPRJ (no texture files needed)
./build/spr2sprj input.sp2 output.sprj

# BGA binary to BGAJ
./build/bga2bgaj input.BGA output.bgaj
```

## Current Progress

| Phase | Goal | Status |
|-------|------|--------|
| **BGA Stack** | Animation rendering, sprite system, regression tests, tools | **Done** |
| **Phase 1** | Play one song with keyboard (engine loop, audio, input, chart parser, judge) | Planned |
| **Phase 2** | Proper visuals (note skins, BGA during gameplay, title screen) | Planned |
| **Phase 3** | Full gameplay loop (song select, hold notes, life gauge, SFX, results) | Planned |
| **Phase 4** | All chart formats, multi-version judge rules, .osf format | Planned |
| **Phase 5** | Double mode, speed mods, pause, calibration, Lua scripting begins | Planned |
| **Phase 6** | USB dance pad support | Planned |
| **Phase 7** | Full Lua game definitions, multiple PIU versions selectable | Planned |
| **Phase 8** | Arcade I/O hardware, online score submission | Planned |
| **Phase 9** | Leaderboards, accounts, web portal | Planned |

Detailed tracking: [docs/stories/STATUS.md](docs/stories/STATUS.md) (290 stories, ~832 story points)

## Project Structure

```
docs/           Format specs, architecture roadmap, requirements, user stories
src/            Engine source code (C++)
test/           Tests and fixtures (unit, integration, visual regression)
test/fixtures/  Sample SPRJ/BGAJ files, PNG textures, reference snapshots
tools/          Standalone tools (bga_player, spr2sprj, bga2bgaj)
```

## Documentation

- [Engine Roadmap](docs/engine-roadmap.md) — Full architecture plan with 11 subsystems and 9 implementation phases
- [Business Requirements](docs/requirements/) — 136 formal requirements organized by subsystem
- [User Stories](docs/stories/) — 290 stories with acceptance criteria, story points, and dependency tracking
- [Format Specifications](docs/) — Complete specs for SPR, SP2, SPRJ, BGA binary, BGAJ, and the BGA animation model

## Contributing

The project is in early development and follows an AI-driven development workflow with a specific architecture. Before starting any work:

1. **Open a discussion first.** Describe what you want to work on and wait for alignment. This prevents wasted effort on PRs that don't fit the architecture or duplicate work already in progress.
2. **Read the [Engine Roadmap](docs/engine-roadmap.md)** to understand the subsystem boundaries and implementation phases.
3. **Check the [project tracking matrix](docs/stories/STATUS.md)** for available stories and their dependencies.

The codebase has strong test coverage. New code should maintain this standard — unit tests for logic, integration tests for rendering, and visual regression tests for any change that affects rendered output.

## Support This Project

The initial effort behind openitup took about 8 months to materialize — most of that time went into understanding the file formats, building the rendering pipeline, and designing the engine architecture. It has been a passion project in my free time, which left little room for actual development.

That's why I jumped into this with an AI-driven development approach in mind. To make the most of my limited time, I've been using a per-token billing solution (no artificial subscription limits), which means I can get the best output per hour of work — but it also means this project has a real cost attached to it.

If you'd like to help keep this going:

**[Buy me a coffee](https://buymeacoffee.com/road24)**

Or, if you can afford to drop some tokens of your own — I've already had Claude generate a full set of user stories for the engine (see the [project tracking matrix](docs/stories/STATUS.md)). If you want to contribute code, please align in a discussion first so you don't spend your tokens on a PR that won't make it in. Cooperative agentic workflows are still new territory for everyone, and I want to make sure contributions fit the architecture before work begins.

## License

GPLv3. See [LICENSE](LICENSE) for details.

## Disclaimer

openitup is an independent, community-driven project. This engine does not include any copyrighted game assets — users must provide their own.
