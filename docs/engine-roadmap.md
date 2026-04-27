# openitup Engine Roadmap

This document describes the full architecture and implementation plan for openitup, an open-source Pump It Up engine reimplementation. It covers the major subsystems, their interactions, key design decisions, and a phased implementation plan from first playable song to online leaderboards.

---

## Current State

The BGA (Background Animation) subsystem is complete and tested. This provides the foundation for all 2D rendering in the engine, since every visual element in Pump It Up — from song backgrounds to UI screens — is built from BGA animations and sprite compositions.

What has been built so far:

- **Sprite and BGA loading**: Both the JSON formats (SPRJ/BGAJ) and the original binary formats (SPR/SP2/BGA) can be loaded directly. A texture cache handles case-insensitive file probing across .tga, .png, and .dds formats.
- **Animation rendering**: Full keyframe interpolation for translate, scale, rotate, color, and alpha at 60 ticks per second with sub-tick precision for high-refresh-rate displays. Painter's algorithm compositing across up to 50 layers. Five blend modes (normal, screen, multiply, with dodge and difference approximations).
- **Renderer**: A `Renderer` class that owns the SDL_Window and SDL_Renderer, sets up 640×480 logical presentation with letterboxing, and provides `begin_frame()`/`end_frame()` for the render loop.
- **Tools**: A BGA player for interactive playback and automated snapshot export, plus format converters (spr2sprj, bga2bgaj) for converting original game assets to JSON.
- **Testing**: 82 tests across three tiers — 24 unit tests (math, interpolation, JSON round-trips), 41 integration tests (sprite modes, texture cache, rendering pipeline), and 17 visual regression tests comparing rendered frames pixel-by-pixel against 30 committed reference images.
- **Build system**: CMake with FetchContent pulling SDL3, SDL3_image, nlohmann/json, spdlog, CLI11, and GoogleTest. Cross-platform, Linux-first.

---

## Subsystems

### 1. Core Engine Loop

The `Engine` class will formalize the current prototype loop in `main.cpp`. It owns the renderer (which already exists), the scene stack, the audio system, and the input system — serving as the root of the entire object graph.

The game logic runs at a fixed 60 Hz tick rate, matching the BGA animation timeline. The rendering runs at the display's refresh rate (or uncapped). A time accumulator drives fixed-step updates: each frame, the engine accumulates wall-clock time, then steps the logic in 1/60th-second increments. This gives deterministic judge timing regardless of whether the display runs at 60, 120, or 144 Hz.

A `Clock` utility wraps `SDL_GetPerformanceCounter` to provide delta time, fixed step tracking, and elapsed time accessors.

**Dependencies**: Renderer (exists).
**Phase**: 1 — this is the prerequisite for everything else.

### 2. Input System

The input system abstracts all physical input devices behind a common vocabulary. A `PadInput` enum represents the 10 PIU dance panels (5 per player: down-left, up-left, center, up-right, down-right) plus menu actions (start, back, select, coin).

Each tick, the input system produces an `InputSnapshot` containing a bitmask of currently held panels and edge events (pressed-this-frame, released-this-frame). The gameplay judge consumes the edge events for timing. Screen logic consumes the full snapshot via Lua bindings.

Three backends sit behind a common `InputDriver` interface:

- **KeyboardDriver**: Maps SDL key events to PadInput via a configurable keymap. This is the development and casual play backend.
- **HidPadDriver**: Uses SDL3's gamepad API for USB dance pads. Supports axis thresholding and button mapping, configurable per device VID/PID.
- **ArcadeIODriver**: Talks to PIUIO arcade I/O boards over raw USB. This is for real arcade cabinets.

Input polling happens once per fixed-step update, before the active scene's `update()` call.

For co-op and battle modes, the input system produces separate `InputSnapshot` instances per player. The `KeyboardDriver` and `HidPadDriver` support binding physical inputs to either P1 or P2. The `ArcadeIODriver` natively separates the two sides of the cabinet.

**Dependencies**: Engine loop (subsystem 1).
**Phase**: 1 (keyboard), 6 (dance pad), 8 (arcade I/O).

### 3. Audio System

Audio is the most latency-sensitive part of a rhythm game. The player's sense of timing depends entirely on the relationship between what they hear and when they press.

The audio system has two channel categories:

- **Music**: One seekable, pausable stream per song. Supports play, pause, seek to position, and — critically — `get_position_ms()` which returns the current playback position based on samples consumed by the audio hardware. This is the authoritative time source for gameplay; the judge reads audio position, not wall-clock time. This eliminates drift between what the player hears and what the game judges.
- **SFX**: Fire-and-forget short samples for key sounds (panel press feedback), judgment sounds, and UI effects. Loaded entirely into memory for instant playback.

The initial implementation uses SDL3's audio API (`SDL_AudioStream`), which supports push-based streaming and multiple simultaneous streams. If output latency proves perceptible in practice, the backend can be swapped to SoLoud or miniaudio behind the same interface without changing any calling code.

A configurable global offset (positive or negative milliseconds) allows users to calibrate for their specific hardware's audio latency. This offset is stored in the user's profile (subsystem 9).

**Dependencies**: Engine loop (subsystem 1), asset management (subsystem 11) for file discovery.
**Phase**: 1 (basic music playback), 3 (SFX and key sounds), 5 (calibration screen).

### 4. Chart / Step System

This subsystem handles parsing all supported chart formats into a unified internal representation, and defines the new `.osf` format.

The internal `Chart` structure contains:

- **Metadata**: Title, artist, BPM, banner path, and other display information.
- **TimingData**: A sorted list of timing events — BPM changes, stops, warps, speed changes, and delays. The two critical functions are `time_at_beat(beat) → seconds` and `beat_at_time(seconds) → beat`, which convert between musical time (beats) and real time (seconds). Every other system that cares about "when" uses these. The implementation follows the same sorted-vector + binary-search pattern already established in the keyframe interpolation system (`evaluate_keyframes` in `keyframe.h`).
- **NoteData**: A flat, time-sorted vector of `NoteEvent` structs. Each event has a beat position (as a rational or double), a column index (0–4 for single, 0–9 for double), and a type (tap, hold head, hold tail, mine, fake, lift).

One parser per format, all producing the same `Chart` struct:

| Format | Source | Notes |
|--------|--------|-------|
| `.ksf` | Kick It Up | Simplest format, good for initial development |
| `.ssc` | StepMania | Most widely used in the community |
| `.sma` | StepMania variant | Older StepMania format |
| `.stx` | PIU Pro/Pro2 | |
| `.see` | PIU Exceed/Exceed2 | Encrypted, requires key |
| `.nx`  | PIU NX through Phoenix | Multiple sub-variants |
| `.osf` | openitup | New JSON-based format, spec TBD in Phase 4 |

The `.osf` (OpenItUp Step File) format is designed to represent everything that any of the legacy formats can express, plus features none of them support individually (arbitrary scroll speed changes, per-note metadata, Lua hooks). It serves as the canonical save format for any future chart editor.

**Dependencies**: None for the data model. Asset management (subsystem 11) for directory scanning.
**Phase**: 1 (KSF parser), 4 (remaining parsers and .osf spec).

### 5. Gameplay Judge

The judge is a pure logic module with no rendering or audio dependencies. Given a note's beat position, the player's input timestamp, and the current song position, it returns a judgment (Perfect, Great, Good, Bad, Miss) with a signed timing error in milliseconds.

Timing windows are data-driven, stored in a `JudgeProfile` JSON file. Each Pump It Up version (Exceed, Zero, NX, NX2, Fiesta, XX, Phoenix) has its own profile specifying window widths, life gauge drain rates, grade thresholds, and scoring formulas. Adding support for a new version means adding a new JSON file, not writing new code.

Hold note processing: a hold is considered "active" once its head is judged and the player keeps the panel held. Releasing early gives a partial score. Releasing and re-pressing within a version-dependent grace window can recover the hold.

Each tick, the judge scans notes within the judgable range around the current song time, matches them against input edge events, and emits `JudgmentEvent`s. Notes that pass beyond the latest timing window without being hit are automatically judged as Miss.

A separate `GameplayState` object subscribes to these events and maintains the running combo, score, life gauge, and grade. This separation means the judge never needs to know about score display or life bar rendering.

For co-op mode, a single `GameplayState` is shared between two judge instances (one per player), each receiving its own `InputSnapshot`. The life gauge can be shared (both players drain/recover the same bar) or separate, depending on the judge profile.

**Dependencies**: Chart system (subsystem 4), input system (subsystem 2), audio system (subsystem 3).
**Phase**: 1 (single profile, taps only), 3 (holds, life gauge), 4 (multi-version profiles), 5 (co-op judge configuration).

### 6. Note Renderer

Notes scroll in beat-space, not time-space. The renderer converts each note's beat position to a vertical screen position using the current scroll speed and BPM. This correctly handles BPM changes, stops, and speed modifiers without special-casing.

In Phase 1, the note renderer draws simple colored rectangles to prove the scroll math works. In Phase 2, it switches to sprite-based rendering using the existing SPRJ loading system. Each note skin is a directory containing a manifest file plus sprites: one per column direction (5 for single, 10 for double), receptor sprites, hold body and cap sprites, and judgment/combo number sprites.

The note field renders into the 640×480 virtual coordinate space. In single mode, it occupies the center. In double mode, it spans the full width. The receptor line position, note scale, and column spacing are configurable per skin.

**Speed modifiers** are applied at render time, not stored in the chart. This is an important architectural boundary — the chart data is always the ground truth, and visual presentation modifiers live in the renderer:

- **C-mod (constant)**: Notes scroll at a fixed rate regardless of BPM.
- **M-mod (multiply)**: Base scroll speed multiplied by a factor.
- **R-mod (random)**: Random per-note speed (cosmetic only, doesn't affect timing).

The BGA animation (the existing animation stack) plays behind the note field during gameplay, synchronized to the audio position via `get_position_ms()`. The note field composites on top, fitting naturally into the painter's algorithm rendering model already in place.

**Dependencies**: Chart system (subsystem 4), judge (subsystem 5), sprite system (exists), BGA animation (exists).
**Phase**: 1 (placeholder rectangles), 2 (sprite-based note skins), 3 (holds, hit effects, judgments), 5 (speed mods, double mode).

### 7. Screen System

The screen system uses a scene stack with push, pop, and replace operations. This naturally supports overlays — pushing a pause screen on top of gameplay doesn't destroy the gameplay scene; it just pauses it.

Each scene is a class implementing `on_enter()`, `on_exit()`, `on_pause()`, `on_resume()`, `update(dt)`, `render()`, and `handle_input(InputSnapshot)`. The engine renders the full stack bottom-to-top (so overlays are visible on top of the scenes below them), but only the topmost scene receives input and update calls.

The standard screens follow the Pump It Up game flow:

| Screen | Purpose |
|--------|---------|
| BootScene | Logo splash, initialization |
| TitleScene | Attract mode / title loop |
| ModeSelectScene | Single / Double / Co-op / Battle |
| SongSelectScene | Music wheel, banner, difficulty selector |
| GameplayScene | The main play screen — owns judge, note renderer, audio, BGA |
| ResultScene | Grade, score, timing breakdown |
| NameEntryScene | High score name input |

Scene transitions use BGA animations for fade and wipe effects, consistent with how the original game handles them. A `TransitionScene` wraps the outgoing and incoming scenes, plays a transition animation, and completes the switch.

The `GameplayScene` is the most complex screen, as it orchestrates all gameplay subsystems. Each frame, `GameplayScene::update()` queries the audio position, advances the BGA tick counter, calls `judge.update()` with the input snapshot, and updates the note renderer's scroll position. The `render()` method draws the BGA first, then the note field, then UI overlays (combo, judgment, life gauge).

**Dependencies**: Engine loop (subsystem 1), input system (subsystem 2).
**Phase**: 1 (minimal GameplayScene), 2 (stack infrastructure, boot, title), 3 (select, result), 5 (transitions, pause overlay).

### 8. Lua Scripting

The engine exposes a stable C++ API to Lua using sol2, a header-only binding library that integrates cleanly with the existing CMake FetchContent pattern.

The C++ side exposes: input queries, audio control, sprite and BGA rendering commands, scene stack navigation, timer utilities, text rendering, and profile/score access. Lua scripts cannot bypass the judge or audio sync — those remain C++ for performance and correctness.

Each Pump It Up game version (Exceed, Zero, NX, etc.) is defined as a Lua "game" — a directory containing Lua scripts for each screen (title.lua, select.lua, result.lua) plus references to the appropriate assets and judge profile. The engine loads one game definition at startup. Switching between game versions means loading a different directory.

This means openitup is not a single game with themes — it's a platform that hosts multiple distinct rhythm games, each defined in Lua, sharing the same C++ engine for the performance-critical parts (rendering, audio, judging).

Lua is explicitly **not** used for per-frame note rendering, judge logic, or audio mixing. These stay in C++ to guarantee frame-budget and timing accuracy.

**Dependencies**: Scene system (subsystem 7), input (subsystem 2), sprite/BGA (exists), audio (subsystem 3).
**Phase**: 5 (initial bindings, convert one screen), 7 (full Lua game definitions, multi-version).

### 9. Profile / Save System

Player profiles are stored as JSON files in a platform-appropriate user data directory (`~/.local/share/openitup/profiles/` on Linux). Each profile contains a display name, per-chart high scores, preferred speed mod, note skin selection, input and audio calibration offsets, and play statistics.

Charts are identified by a content hash (SHA-256 of the note data and timing data in a canonical binary representation, excluding metadata like title or artist). This means the same chart in different file formats, or moved to a different directory, retains its score history. The hash is computed once at chart load time. The exact canonical form will be specified when the hashing is implemented in Phase 4.

Engine settings (video resolution, audio device, key bindings, global offset) are stored in a separate `settings.json` file. Settings load at startup and save on change.

The profile system is a service object owned by the `Engine`, accessible from both C++ screens and Lua scripts.

**Dependencies**: Chart system (subsystem 4) for hashing, engine (subsystem 1) for lifecycle.
**Phase**: 3 (settings, single default profile), 5 (multiple profiles, high scores), 7 (statistics).

### 10. Network

The network system enables online score submission and leaderboards. It follows a client-server architecture where the server is a separate project (likely Rust or Go), and the engine includes a thin HTTP client layer.

After completing a song, the client posts the score, chart hash, judge profile version, and a replay hash (for anti-cheat verification) to the server. Leaderboards are queried per chart hash.

The account system uses simple username/password authentication with token-based sessions. Tokens are stored in the player's profile.

All network operations run asynchronously on a background thread with callbacks. The game never blocks on network I/O. Failed submissions are queued locally and retried on the next successful connection.

The game is fully functional offline. Network features are purely additive — they enhance the experience but are never required.

**Dependencies**: Profile system (subsystem 9), chart system (subsystem 4), judge (subsystem 5).
**Phase**: 8 (score submission, server prototype), 9 (leaderboards, accounts, web portal).

### 11. Asset Management

The engine does not ship game assets. It discovers them from one or more configured game data directories at startup.

The asset manager scans for known directory structures: song folders (containing chart files, audio, and BGA data), system assets (note skins, UI sprites, fonts, sound effects), and version-specific layouts. Version detection uses a combination of directory structure heuristics and the presence of version-specific file formats (e.g., `.see` files indicate Exceed-era data, `.nx` files indicate NX-era data).

A `SongDatabase` is built during startup by scanning all song directories. Each entry stores resolved paths to the chart file, audio, banner, and BGA. This database is cached to disk to avoid full re-scans on every launch.

The existing `TextureCache` with case-insensitive probing forms the foundation. The asset manager sits above it, providing higher-level queries: "give me the song list," "load the note skin for this column," "find the BGA for this song."

Resource loading is lazy where possible. Song BGAs and audio are loaded when the player selects a song, not at startup. Note skins are loaded when gameplay begins.

**Dependencies**: Texture cache (exists), sprite/BGA loaders (exist), chart system (subsystem 4).
**Phase**: 1 (hardcoded path to one song directory), 3 (recursive directory scan and song database), 5 (version detection, note skin loading), 7 (multi-version data coexistence).

---

## Implementation Phases

### Phase 1: Play One Song (Keyboard)

The goal is the minimum playable loop: load a chart, play its audio, scroll notes on screen, accept keyboard input, judge timing, and show feedback. Everything is minimal — rectangles instead of sprites, hardcoded paths, one chart format.

- [ ] `Engine` class with fixed-step game loop, replacing the prototype loop in `main.cpp`
- [ ] `Clock` utility for frame timing (delta time, fixed step, elapsed)
- [ ] `InputSystem` with `KeyboardDriver` producing `InputSnapshot` per tick
- [ ] `AudioSystem` with music loading (OGG/MP3), playback, position tracking, seek, and pause
- [ ] `TimingData` and `NoteData` internal representation
- [ ] KSF chart parser
- [ ] `Judge` with a single timing profile (tap notes only, no holds)
- [ ] Minimal `GameplayScene` wiring the chart, audio, judge, and input together
- [ ] Minimal note renderer using colored rectangles (proving the scroll math works)
- [ ] Minimal asset discovery (command-line path to a song directory)

**Exit criteria**: Launch the engine, point it at a song folder, hear the music, see notes scrolling in time, press keys, see timing feedback on screen.

### Phase 2: It Looks Like a Game

Replace the placeholder visuals with proper sprites, add BGA backgrounds during gameplay, and introduce a basic screen flow so the game has a beginning and not just a gameplay screen.

- [ ] Scene stack infrastructure (push, pop, replace)
- [ ] Sprite-based note rendering using the existing SPRJ system
- [ ] Note skin loading from a directory with a manifest file
- [ ] BGA playback during gameplay, synchronized to audio position (the animation stack already works; wire it behind the note field)
- [ ] Boot and title screens driven by BGA animations
- [ ] Judgment and combo display using sprite-based numbers and text

**Exit criteria**: The game boots to a title screen, transitions to gameplay, plays a song with proper note graphics and BGA background, and shows judgment feedback.

### Phase 3: Full Gameplay Loop

Complete the single-player loop from song selection through results, adding hold notes, a life gauge, sound effects, and settings persistence.

- [ ] Song database built by scanning song directories
- [ ] Song select screen with a music wheel, banner display, and difficulty selector
- [ ] Hold note processing in the judge (active while held, grace window on release)
- [ ] Hold note rendering (body and cap sprites)
- [ ] Life gauge with HP drain on misses and fail detection
- [ ] SFX system for key sounds on panel press and judgment feedback sounds
- [ ] Results screen showing grade, score, and timing breakdown
- [ ] Settings file for key bindings, audio offset, and display preferences

**Exit criteria**: A complete single-player session from title screen through song select, gameplay with hold notes and life gauge, to a result screen. Scores are saved locally.

### Phase 4: Multi-Format, Multi-Version

Expand chart format support to cover all Pump It Up versions, define the custom .osf format, and introduce version-specific judge rules.

- [ ] Additional chart parsers: SSC, SMA, STX, SEE, and NX
- [ ] `.osf` format specification document and parser/writer implementation
- [ ] Multiple judge profiles for each PIU version (Exceed through Phoenix)
- [ ] Version selection in the UI
- [ ] Chart content hash computation (SHA-256) for stable score identity

**Exit criteria**: The engine can load songs from any supported PIU version's data. The player can select which version's judge rules to use. Scores are correctly attributed per chart hash.

### Phase 5: Polish and Double Mode

Add double play, speed modifiers, pause functionality, screen transitions, input calibration, and multi-profile support.

- [ ] Double mode note field layout (10 columns spanning the full width)
- [ ] Speed modifiers: C-mod (constant scroll speed) and M-mod (multiplied scroll speed)
- [ ] Co-op mode (2 players with per-player input snapshots and shared or separate life gauge)
- [ ] Pause overlay (pushed on top of gameplay without destroying it)
- [ ] Scene transitions using BGA animations for fades and wipes
- [ ] Audio and input calibration screen
- [ ] Multiple local profiles with per-profile high scores
- [ ] Initial Lua bindings (sol2): convert one screen (e.g., title) as proof of concept

**Exit criteria**: Double mode is playable. Speed mods work correctly. The game can be paused. Transitions between screens are smooth. Users can calibrate their setup and maintain separate profiles.

### Phase 6: HID Dance Pad

Enable playing on real dance pads connected via USB.

- [ ] `HidPadDriver` using SDL3's gamepad API
- [ ] Device mapping configuration (USB VID/PID to column mapping)
- [ ] Input mapping screen (press each panel to configure)
- [ ] Simultaneous keyboard and pad support
- [ ] Per-device calibration settings

**Exit criteria**: Plug in a USB dance pad, configure it through the in-game screen, and play songs. Works alongside keyboard input.

### Phase 7: Lua Scripting and Game Versions

Move all screen logic to Lua, enabling multiple PIU versions to coexist as separate Lua "game" packages on the same engine.

- [ ] C++ API bindings for input, audio, rendering, scene stack, and profile access
- [ ] Define the Lua game directory structure and manifest format
- [ ] Convert all screens to Lua (boot, title, mode select, song select, gameplay overlay, result, name entry)
- [ ] Create one complete Lua game definition (e.g., Exceed-style)
- [ ] Create a second Lua game definition (e.g., NX-style) to validate the multi-version architecture
- [ ] Multi-version switching at startup or from a menu

**Exit criteria**: All screen flow is controlled by Lua. At least two distinct game versions are selectable, each with its own screen behavior and asset references.

### Phase 8: Arcade I/O and Score Submission

Support arcade cabinet hardware and begin online functionality with score submission.

- [ ] `ArcadeIODriver` for PIUIO (USB bulk transfer protocol for sensors and lamps)
- [ ] Lamp output control during gameplay (panel lighting)
- [ ] HTTP client library integration (libcurl or cpp-httplib)
- [ ] Score submission endpoint (client side)
- [ ] Server prototype with REST API, database, and authentication

**Exit criteria**: The engine runs on arcade hardware with PIUIO input and lamp output. Scores are successfully posted to a server after each play.

### Phase 9: Online Community

Complete the online feature set with leaderboards, accounts, replays, and a web portal.

- [ ] Account creation and authentication flow in the game client
- [ ] Leaderboard query and display per chart (in-game and global)
- [ ] Replay data capture during gameplay and upload to server
- [ ] Server hardening: rate limiting, input validation, anti-cheat verification
- [ ] Web portal for browsing leaderboards outside the game

**Exit criteria**: Users can create accounts, submit scores, and view leaderboards both in-game and on the web. Basic anti-cheat measures are in place.

---

## Key Architecture Decisions

| Decision | Rationale |
|----------|-----------|
| Fixed 60 Hz logic step | Matches the BGA animation timeline. Gives deterministic judge timing regardless of display refresh rate. Rendering interpolates between logic steps for smooth visuals at any frame rate. |
| Audio position as the song time source | The audio callback's consumed-sample count is the authoritative clock for gameplay. This eliminates drift between what the player hears and what the game judges. A global offset setting handles hardware-specific audio latency. |
| Data-driven judge profiles in JSON | Timing windows, scoring formulas, life gauge rules, and grade thresholds are configuration, not code. Supporting a new PIU version means adding a JSON file, not modifying the judge. |
| Lua for screen logic, C++ for core | The judge, audio synchronization, note renderer, and animation stack stay in C++ for performance and timing guarantees. Lua handles the parts that differ between game versions: screen flow, UI layout, input routing, and cosmetic behavior. This makes the engine a platform, not a single game. |
| BGA animation stack as the UI rendering primitive | Every screen in Pump It Up is built from BGA animations — title screens, song select backgrounds, result screens. The existing animation system is the UI framework. Building on it avoids inventing a separate UI toolkit. |
| Chart content hash for score identity | A SHA-256 hash of the note data and timing data (excluding metadata) means scores survive file renames, format conversions, and directory reorganization. The same chart parsed from KSF or SSC produces the same hash and shares the same leaderboard. |
| Server as a separate project | The server has a different language (Rust or Go), different deployment model, and different release lifecycle than the game client. Keeping it separate avoids coupling their development. |

---

## Scope Notes

- **Localization**: English-only for the initial implementation. Internationalization support (multi-language strings, font fallbacks) is not planned for any phase but can be added later without architectural changes.
- **Chart editing**: A chart editor is out of scope for this roadmap. The `.osf` format is designed to support one, but the editor itself would be a separate project or a post-Phase 9 effort.
- **Target platforms**: Desktop Linux and Windows are the primary targets. The architecture does not preclude running on lower-power hardware (Steam Deck, Raspberry Pi), but no specific optimization targets are set for those platforms.
- **Error handling**: Asset loading failures (missing textures, unparseable charts) are logged at error level via spdlog and surface as exceptions. The engine continues running when possible (e.g., a missing BGA doesn't prevent gameplay), but missing critical assets (chart file, audio) are fatal for that song.
