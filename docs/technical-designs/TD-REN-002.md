# TD-REN-002: Sprite-Based Note Skins — NoteSkin Loading, Sprite Note Rendering, and Receptor Compositing

**Stories**: US-REN-021, US-REN-022, US-REN-023
**Phase**: 2
**Author**: technical-architect agent
**Status**: Draft

## Overview

This design replaces the Phase 1 placeholder rectangles in `NoteRenderer` and `JudgmentDisplay` with sprite-based rendering driven by a `NoteSkin` data class. The `NoteSkin` owns all `Sprite` objects for a single noteskin directory (loaded via the existing `load_sprj` / `TextureCache` infrastructure). The `NoteRenderer` is modified to accept an optional `NoteSkin*` and render notes as animated sprites instead of colored rectangles. The `JudgmentDisplay` gains per-column judge overlay sprites from the same skin. Receptor rendering becomes a 3-layer composite (receptor background + press overlay + judge overlay).

The existing `Sprite::draw()` already handles ANI-mode frame selection via a normalized `t` parameter. Noteskin animations are 6-frame loops at 50ms/frame (300ms cycle). The design introduces a thin `NoteSkinAnimTimer` utility that converts a wall-clock millisecond timestamp to the `[0,1)` normalized `t` value for looping sprites, and a separate one-shot variant for press/judge overlays.

## Architecture

### Component Diagram

```
GameplayScene (TD-SCN-001)
  |
  |  at init:
  |    noteskin_ = NoteSkinLoader::load("default", cache_)
  |    note_renderer_ = NoteRenderer(note_data, timing_data, config, &noteskin_)
  |
  |  per-frame:
  |    1. note_renderer_.render(sdl_renderer, song_position_ms, global_time_ms)
  |    2. note_renderer_.render_receptors(sdl_renderer, global_time_ms, pressed, judge_active)
  |    3. judgment_display_.render(sdl_renderer, dt)
  |
  v

NoteSkinLoader (src/openitup/render/noteskin_loader.h)
  |  uses
  ├── load_sprj() (src/openitup/sprite/sprite_loader.h)
  └── TextureCache (src/openitup/gfx/texture_cache.h)
  |
  |  produces
  v

NoteSkin (src/openitup/render/noteskin.h)
  |  owns (unique_ptr<Sprite> for each loaded SPRJ)
  |  48 possible sprites per spec, graceful null for missing
  |
  |  consumed by (non-owning pointer)
  v

NoteRenderer (src/openitup/render/note_renderer.h)
  |  reads (const ref, non-owning)
  ├── NoteData (TD-CHT-001)
  ├── TimingData (TD-CHT-001)
  ├── NoteFieldConfig (pixel layout)
  └── NoteSkin* (nullable — falls back to placeholder rectangles if null)

JudgmentDisplay (src/openitup/render/judgment_display.h)
  |  reads (non-owning pointer)
  └── NoteSkin* (uses JUDGE sprites per column for overlay)
```

### New Types

#### `NoteSkin` (`src/openitup/render/noteskin.h`)

Pure data holder. Owns all loaded Sprite objects for one noteskin directory. Provides typed accessors for each arrow type. Missing sprites are represented as null unique_ptrs — callers check before drawing.

```cpp
// src/openitup/render/noteskin.h
#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>

#include <openitup/sprite/sprite.h>

namespace openitup {

// Number of tracks in a single side (always 5 for PIU).
inline constexpr int NUM_TRACKS = 5;

// Hold sub-parts.
enum class HoldPart : uint8_t { HEAD = 0, BODY = 1, TAIL = 2 };

class NoteSkin {
public:
    // The directory this skin was loaded from.
    std::filesystem::path directory;

    // The skin name (directory stem, e.g. "default").
    std::string name;

    // --- Per-track sprites (5 entries each, indexed by track 0-4) ---

    // TAP arrows.
    const Sprite* tap(int track) const;

    // FAKETAP arrows.
    const Sprite* faketap(int track) const;

    // LONG hold note parts.
    const Sprite* hold(int track, HoldPart part) const;

    // OTHER division mode notes.
    const Sprite* other_w(int track) const;
    const Sprite* other_g(int track) const;

    // PRESS overlay (one-shot, triggered on panel press).
    const Sprite* press(int track) const;

    // JUDGE overlay (one-shot, triggered on Perfect/Great/Good).
    const Sprite* judge(int track) const;

    // --- Receptor sprites (one per play mode) ---

    enum class PlayMode : uint8_t { SINGLE = 0, DOUBLE = 1, HALF = 2 };

    const Sprite* receptor(PlayMode mode) const;

    // --- Completeness query ---

    // True if all Phase 2 required sprites are loaded (all TAP, LONG, etc.).
    bool is_complete() const;

    // Count of successfully loaded sprites.
    int loaded_count() const;

    // Total expected for Phase 2 (48).
    static constexpr int EXPECTED_COUNT = 48;

private:
    // NoteSkinLoader is the only class that populates these.
    friend class NoteSkinLoader;

    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> tap_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> faketap_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> hold_head_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> hold_body_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> hold_tail_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> other_w_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> other_g_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> press_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> judge_;
    std::array<std::unique_ptr<Sprite>, 3> receptor_;  // SINGLE, DOUBLE, HALF
};

} // namespace openitup
```

**Key decisions**:

- Fixed `std::array` storage rather than a map. There are exactly 5 tracks and exactly N known types. Arrays give O(1) lookup and zero allocation overhead.
- `const Sprite*` return type (non-owning). Returns `nullptr` when the sprite was not loaded, allowing callers to skip rendering for that element. The `unique_ptr` stays inside `NoteSkin`.
- The `friend class NoteSkinLoader` pattern keeps the mutation surface minimal: only the loader writes to the arrays, all other access is const.
- `HoldPart` enum avoids magic integers when indexing hold sub-parts.

---

#### `NoteSkinLoader` (`src/openitup/render/noteskin_loader.h`)

Stateless factory. Discovers and loads SPRJ files from a noteskin directory using the naming convention from `docs/noteskin-format-spec.md`. Uses the existing `load_sprj()` function and `TextureCache`.

```cpp
// src/openitup/render/noteskin_loader.h
#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <openitup/gfx/texture_cache.h>
#include <openitup/render/noteskin.h>

namespace openitup {

class NoteSkinLoader {
public:
    // Load a noteskin from "noteskin/<skin_name>/".
    // base_dir: root directory containing the "noteskin/" folder (project root).
    // skin_name: subdirectory name (e.g. "default").
    // cache: TextureCache for loading textures referenced by SPRJs.
    //
    // Returns a populated NoteSkin. Missing individual SPRJ files produce
    // warnings but do not fail — the corresponding sprite pointer will be null.
    // Throws if the skin directory does not exist AND fallback to "default" also fails.
    static std::unique_ptr<NoteSkin> load(
        const std::filesystem::path& base_dir,
        const std::string& skin_name,
        TextureCache& cache);

    // Load with automatic fallback: tries skin_name first, then "default".
    static std::unique_ptr<NoteSkin> load_with_fallback(
        const std::filesystem::path& base_dir,
        const std::string& skin_name,
        TextureCache& cache);

private:
    // Try to load a single SPRJ from the skin directory.
    // Returns nullptr (with warning log) if file does not exist.
    static std::unique_ptr<Sprite> try_load_sprj(
        const std::filesystem::path& skin_dir,
        const std::string& filename,
        TextureCache& cache);
};

} // namespace openitup
```

**Key decisions**:

- `static` methods — no instance state. The loader is a pure factory.
- `try_load_sprj` returns `nullptr` on missing file (with spdlog warning). This implements the spec's "log warning, render nothing for that element" behavior for individual missing assets.
- `load_with_fallback` encapsulates the spec's fallback logic: try requested skin, then `default/`, then throw if both fail.
- The loader constructs filenames from the naming convention (e.g., `fmt::format("ARROW{:02d}_TAP.sprj", track)`). No filesystem scanning needed — the names are deterministic.

---

#### `NoteSkinAnimTimer` (`src/openitup/render/noteskin_anim_timer.h`)

Lightweight utility for converting wall-clock time to sprite `t` parameter. Two modes: looping (for scrolling notes and receptors) and one-shot (for press/judge overlays).

```cpp
// src/openitup/render/noteskin_anim_timer.h
#pragma once

#include <cmath>

namespace openitup {

// Animation constants from noteskin-format-spec.md.
inline constexpr double NOTESKIN_FRAME_DURATION_MS = 50.0;
inline constexpr int    NOTESKIN_FRAME_COUNT = 6;
inline constexpr double NOTESKIN_LOOP_DURATION_MS =
    NOTESKIN_FRAME_DURATION_MS * NOTESKIN_FRAME_COUNT;  // 300.0

// Compute the looping normalized t for a global timestamp.
// Returns a value in [0.0, 1.0) that cycles every 300ms.
// Pure function — no state.
inline float noteskin_loop_t(double global_time_ms) {
    double phase = std::fmod(global_time_ms, NOTESKIN_LOOP_DURATION_MS);
    if (phase < 0.0) phase += NOTESKIN_LOOP_DURATION_MS;
    return static_cast<float>(phase / NOTESKIN_LOOP_DURATION_MS);
}

// Compute the one-shot normalized t for an animation that started at trigger_time_ms.
// Returns [0.0, 1.0] where 1.0 means the animation has finished.
// After the full duration (300ms), returns values >= 1.0 (caller checks for expiry).
inline float noteskin_oneshot_t(double global_time_ms, double trigger_time_ms) {
    double elapsed = global_time_ms - trigger_time_ms;
    if (elapsed < 0.0) return 0.0f;
    return static_cast<float>(elapsed / NOTESKIN_LOOP_DURATION_MS);
}

// True if a one-shot animation triggered at trigger_time_ms is still active.
inline bool noteskin_oneshot_active(double global_time_ms, double trigger_time_ms) {
    return (global_time_ms - trigger_time_ms) < NOTESKIN_LOOP_DURATION_MS;
}

} // namespace openitup
```

**Key decisions**:

- Header-only with `inline` functions. These are trivial computations — no need for a .cpp file.
- `noteskin_loop_t` uses `std::fmod` with positive correction. This gives a clean `[0,1)` sawtooth that the existing `Sprite::ani_frame()` consumes directly.
- `noteskin_oneshot_t` can exceed 1.0, which `Sprite::ani_frame()` clamps to the last frame via `std::clamp`. The caller checks `noteskin_oneshot_active()` to know when to stop drawing.
- Global wall-clock time (not song time) drives all noteskin animations. The spec says "global animation timer" for looping and "300ms from trigger" for one-shots. This avoids coupling animation speed to BPM or scroll speed.

---

### Modified Types

#### `NoteRenderer` (`src/openitup/render/note_renderer.h`)

- **Add parameter**: Constructor gains optional `const NoteSkin* skin` (nullable, non-owning). When non-null, `render()` draws sprites; when null, falls back to colored rectangles (Phase 1 behavior preserved).
- **Modify method**: `render(SDL_Renderer*, double song_position_ms)` gains a second parameter `double global_time_ms` to drive noteskin animation. Signature becomes: `void render(SDL_Renderer* renderer, double song_position_ms, double global_time_ms) const;`
- **Modify method**: `render_receptors(SDL_Renderer*)` gains parameters for animation and interactive state. New signature: `void render_receptors(SDL_Renderer* renderer, double global_time_ms, const bool* pressed_columns, const double* judge_trigger_times) const;` where `pressed_columns` is a bool array of size `num_columns` (from input system), and `judge_trigger_times` is a double array of per-column timestamps when the last judge animation was triggered (negative or 0 = inactive).
- **Add member**: `const NoteSkin* skin_` (non-owning pointer, may be null).
- **Add member**: `NoteFieldConfig` gains `float note_sprite_size = 64.0f` field for the logical sprite size from the noteskin spec (64x64 in 640x480 space).
- **Reason**: The renderer must be able to render sprites for TAP, FAKETAP, HOLD_HEAD, and FAKE note types using the noteskin, and must composite the 3-layer receptor area. The nullable pointer preserves backward compatibility with Phase 1 tests that construct NoteRenderer without a skin.

The render loop changes from:
```
SDL_SetRenderDrawColor(color); SDL_RenderFillRect(rect);
```
to:
```
const Sprite* sprite = select_sprite(note.type, note.column);
if (sprite) {
    float t = noteskin_loop_t(global_time_ms);
    LayerTransform xform;
    xform.translate_x = x;
    xform.translate_y = y;
    sprite->draw(renderer, cache, t, xform, {}, SDL_BLENDMODE_BLEND);
} else {
    // Phase 1 fallback: colored rectangle
}
```

#### `NoteFieldConfig` (`src/openitup/render/note_renderer.h`)

- **Add field**: `float note_sprite_size = 64.0f` — logical pixel size of noteskin sprites. Used when skin is active to position sprites. The existing `note_width`/`note_height` (48.0f) continue to be used for placeholder rectangles.

#### `JudgmentDisplay` (`src/openitup/render/judgment_display.h`)

- **Add member**: `const NoteSkin* skin_` — optional noteskin pointer for sprite-based judge display.
- **Modify constructor**: Accept optional `const NoteSkin*`.
- **Reason**: When a noteskin is available, the judgment display can use the noteskin's JUDGE sprites for per-column visual feedback instead of a single colored rectangle. However, the central judgment tier display (PERFECT/GREAT/etc. text) is a separate concern from the per-column judge overlay. In Phase 2, the JudgmentDisplay continues showing the colored rectangle for the tier label (sprite-based judgment *text* is a future story). The per-column judge overlay moves to NoteRenderer's receptor compositing layer.

This is a minimal change: the JudgmentDisplay class itself does not need deep modification for Phase 2. The per-column judge animation overlay is rendered by `NoteRenderer::render_receptors()` using the noteskin's JUDGE sprites. The JudgmentDisplay remains the central tier indicator.

## File Plan

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/openitup/render/noteskin.h` | NoteSkin data class — owns all Sprite objects for one skin |
| Create | `src/openitup/render/noteskin.cpp` | NoteSkin accessor implementations, `is_complete()`, `loaded_count()` |
| Create | `src/openitup/render/noteskin_loader.h` | NoteSkinLoader factory — loads a skin directory |
| Create | `src/openitup/render/noteskin_loader.cpp` | NoteSkinLoader implementation — file discovery, try_load_sprj |
| Create | `src/openitup/render/noteskin_anim_timer.h` | Animation timing utilities (header-only, inline functions) |
| Modify | `src/openitup/render/note_renderer.h` | Add `NoteSkin*` member, update render signatures, add `note_sprite_size` to config |
| Modify | `src/openitup/render/note_renderer.cpp` | Sprite rendering path in `render()`, 3-layer receptor compositing |
| Modify | `src/openitup/render/judgment_display.h` | Add optional `NoteSkin*` member (minimal, see Architecture above) |
| Modify | `CMakeLists.txt` | Add `noteskin.cpp`, `noteskin_loader.cpp` to `openitup_engine` sources |
| Create | `test/test_noteskin.cpp` | Unit tests for NoteSkin accessors, NoteSkinLoader file discovery, animation timer math |
| Modify | `CMakeLists.txt` | Add `test_noteskin.cpp` to `openitup_tests` |

## Data Flow

### Skin Loading at Scene Init

```
1. GameplayScene init:
   base_dir = project_root  (contains "noteskin/" directory)
   skin_name = "default"    (from config or hardcoded Phase 2)

2. NoteSkinLoader::load_with_fallback(base_dir, "default", cache):
   a. skin_dir = base_dir / "noteskin" / "default"
   b. Verify skin_dir exists (is_directory)
   c. For each track 0-4:
      try_load_sprj(skin_dir, "ARROW00_TAP.sprj", cache)      -> skin.tap_[0]
      try_load_sprj(skin_dir, "ARROW00_FAKETAP.sprj", cache)   -> skin.faketap_[0]
      try_load_sprj(skin_dir, "ARROW00_LONG_HEAD.sprj", cache) -> skin.hold_head_[0]
      try_load_sprj(skin_dir, "ARROW00_LONG_BODY.sprj", cache) -> skin.hold_body_[0]
      try_load_sprj(skin_dir, "ARROW00_LONG_TAIL.sprj", cache) -> skin.hold_tail_[0]
      try_load_sprj(skin_dir, "ARROW00_OTHER_W.sprj", cache)   -> skin.other_w_[0]
      try_load_sprj(skin_dir, "ARROW00_OTHER_G.sprj", cache)   -> skin.other_g_[0]
      try_load_sprj(skin_dir, "ARROW00_PRESS.sprj", cache)     -> skin.press_[0]
      try_load_sprj(skin_dir, "ARROW00_JUDGE.sprj", cache)     -> skin.judge_[0]
      ... (repeat for tracks 01-04)
   d. try_load_sprj(skin_dir, "ARROW_RECEPTOR_SINGLE.sprj", cache) -> skin.receptor_[0]
      try_load_sprj(skin_dir, "ARROW_RECEPTOR_DOUBLE.sprj", cache) -> skin.receptor_[1]
      try_load_sprj(skin_dir, "ARROW_RECEPTOR_HALF.sprj", cache)   -> skin.receptor_[2]
   e. Log summary: "NoteSkin 'default' loaded: 30/48 sprites"
   f. Return unique_ptr<NoteSkin>

3. NoteRenderer constructed with &noteskin:
   NoteRenderer(note_data, timing_data, config, noteskin_.get())
```

### Per-Frame Note Rendering

```
NoteRenderer::render(sdl_renderer, song_position_ms=5000.0, global_time_ms=12345.0):

1. current_beat = timing_data_.beat_at_time(5.0)   // existing logic unchanged

2. Compute visible beat range (existing logic unchanged)

3. For each visible note in range:
   a. y = beat_to_y(note.beat, current_beat)        // existing, unchanged
   b. x = column_x[note.column]                     // existing
   c. track = note.column % NUM_TRACKS              // map column to track 0-4

   d. Select sprite by note type:
      NoteType::TAP       -> skin_->tap(track)
      NoteType::FAKE      -> skin_->faketap(track)
      NoteType::HOLD_HEAD -> skin_->hold(track, HoldPart::HEAD)
      (HOLD_TAIL, MINE handled in future phases)

   e. If sprite is non-null:
      float t = noteskin_loop_t(global_time_ms)     // 300ms loop -> [0,1)
      LayerTransform xform{};
      xform.translate_x = x - 32.0f;               // center 64px sprite on column
      xform.translate_y = y - 32.0f;               // center vertically
      sprite->draw(sdl_renderer, cache, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND);

   f. Else (no skin or missing sprite):
      Fall back to colored rectangle (Phase 1 behavior)
```

### Per-Frame Receptor 3-Layer Compositing

```
NoteRenderer::render_receptors(sdl_renderer, global_time_ms, pressed_columns, judge_trigger_times):

For each column col (0 to num_columns-1):
  track = col % NUM_TRACKS
  x = column_x[col] - 32.0f          // center 64px sprite
  y = receptor_y - 32.0f

  // Layer 1: Receptor background (always looping)
  // Note: receptor SPRJ is a full bar for the mode. For Phase 2,
  // if per-track receptor sprites are not available, we draw per-column.
  // The spec says receptor is one SPRJ for the full bar, but the actual
  // noteskin/default/ has no RECEPTOR files yet. For now, this layer is
  // deferred until receptor SPRJs are created.

  // Layer 2: Press overlay (one-shot, while panel pressed)
  if (pressed_columns && pressed_columns[col]):
    const Sprite* press_sprite = skin_->press(track)
    if (press_sprite):
      // While held: loop the press animation
      float t = noteskin_loop_t(global_time_ms)
      press_sprite->draw(sdl_renderer, cache, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND)

  // Layer 3: Judge overlay (one-shot, 300ms from trigger)
  if (judge_trigger_times && noteskin_oneshot_active(global_time_ms, judge_trigger_times[col])):
    const Sprite* judge_sprite = skin_->judge(track)
    if (judge_sprite):
      float t = noteskin_oneshot_t(global_time_ms, judge_trigger_times[col])
      judge_sprite->draw(sdl_renderer, cache, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND)
```

### NoteType-to-Sprite Mapping

The mapping from `NoteType` (from `note_type.h`) to noteskin sprite arrays:

| NoteType | NoteSkin Accessor | Phase |
|----------|-------------------|-------|
| `TAP` | `tap(track)` | 2 (this TD) |
| `FAKE` | `faketap(track)` | 2 (this TD) |
| `HOLD_HEAD` | `hold(track, HEAD)` | 2 (this TD, head only) |
| `HOLD_TAIL` | `hold(track, TAIL)` | 3 (US-REN-025) |
| `MINE` | future ITEM sprites | future |
| `LIFT` | future | future |

The `column % NUM_TRACKS` operation maps double-mode columns (0-9) to tracks (0-4), matching the spec: "Double mode reuses the same 5 arrow assets for both sides."

## Dependencies

### Internal
- **Sprite** (`src/openitup/sprite/sprite.h`) — The core renderable. `NoteSkin` owns `Sprite` instances. `Sprite::draw()` with ANI mode handles frame selection via `t`.
- **load_sprj** (`src/openitup/sprite/sprite_loader.h`) — Loads SPRJ files, resolves textures. Called by `NoteSkinLoader`.
- **TextureCache** (`src/openitup/gfx/texture_cache.h`) — Caches textures loaded by SPRJ files. Passed through from scene to loader.
- **NoteData / TimingData** (`src/openitup/chart/note_data.h`, `timing_data.h`, TD-CHT-001) — Unchanged. NoteRenderer reads these as before.
- **NoteType** (`src/openitup/chart/note_type.h`) — Existing enum. The `FAKE` value maps to FAKETAP sprites.
- **LayerTransform / ColorMod** (`src/openitup/math/types.h`) — Used to position sprites at note screen coordinates.

### External (new libraries)
None. All dependencies are already in the project.

## Architectural Decisions

### ADR-1: NoteSkin as Data Class, Loader as Separate Factory

- **Context**: The noteskin could be a self-loading class (constructor takes a path) or a passive data holder populated by a separate loader.
- **Decision**: Separate `NoteSkin` (data) and `NoteSkinLoader` (factory). `NoteSkin` has no filesystem or TextureCache dependency in its header.
- **Alternatives considered**: (a) Self-loading class — couples data lifetime to I/O, makes testing harder (need real filesystem for construction). (b) Free function returning NoteSkin — similar to the chosen approach but `NoteSkinLoader` as a class allows the `friend` pattern for private member population.
- **Consequences**: Tests can construct a `NoteSkin` with manually-inserted sprites (via a test helper or by making the friend relationship available to tests). The loader can be tested separately against real noteskin directories.

### ADR-2: Nullable NoteSkin Pointer Instead of NoteSkin Interface

- **Context**: NoteRenderer needs to work both with and without a noteskin (Phase 1 rectangle fallback must still work for tests and for the missing-skin error case).
- **Decision**: `NoteRenderer` accepts `const NoteSkin*` which may be null. When null, the existing colored-rectangle path executes.
- **Alternatives considered**: (a) `NoteSkinInterface` with a `RectangleNoteSkin` placeholder implementation — over-engineered for a temporary fallback. (b) Always require a noteskin, provide a "built-in" one — requires shipping placeholder SPRJ files.
- **Consequences**: Existing Phase 1 tests continue to work without modification (pass `nullptr` for skin). The `render()` method has a single `if (skin_)` branch at the point where it selects how to draw each note.

### ADR-3: Global Wall-Clock Time for Animation, Not Song Time

- **Context**: Noteskin animations need a time source. Options are song position (ms from audio), beat position, or wall-clock time.
- **Decision**: Use `global_time_ms` (wall-clock time since application start, typically `SDL_GetTicks()`). The `noteskin_loop_t()` function wraps this into a 300ms cycle.
- **Alternatives considered**: (a) Song position — would freeze animations during song stops/pauses. The spec says arrows animate continuously. (b) Beat position — would change animation speed with BPM. The spec says 50ms/frame regardless of BPM.
- **Consequences**: Noteskin animations run at a constant rate independent of gameplay timing. Pausing the game pauses wall-clock updates from the scene, which naturally pauses animations.

### ADR-4: Receptor Background Deferred Within Phase 2

- **Context**: The spec defines 3 receptor layers. The receptor background is a single SPRJ for the full bar (e.g., ARROW_RECEPTOR_SINGLE.sprj). The current noteskin/default/ directory does not contain receptor SPRJs. Press and judge overlays use per-track sprites which do exist.
- **Decision**: Implement press overlay and judge overlay in this TD. The receptor background layer renders using the existing outlined-rectangle placeholder until receptor SPRJs are created. The architecture supports adding receptor background rendering with zero code changes — just create the SPRJ files and the `skin_->receptor()` call will return non-null.
- **Alternatives considered**: Block on receptor SPRJ asset creation — would delay the rest of the sprite note rendering work.
- **Consequences**: The 3-layer receptor system is structurally complete but layer 1 (background) renders as a placeholder rectangle when the receptor SPRJ is missing. This matches the spec's "render nothing for that element" rule for missing assets.

### ADR-5: TextureCache Reference Stored in NoteRenderer

- **Context**: `Sprite::draw()` requires a `const TextureCache&`. The `NoteRenderer` currently has no reference to TextureCache because Phase 1 did not use sprites.
- **Decision**: Add `TextureCache& cache_` reference to `NoteRenderer`, passed via constructor.
- **Alternatives considered**: (a) Pass cache through every render call — clutters the per-frame API. (b) Embed cache in NoteSkin — violates single responsibility and creates ownership issues.
- **Consequences**: NoteRenderer's constructor gains one more parameter. The cache must outlive the renderer (same lifetime management as NoteData and TimingData).

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Noteskin directory missing at runtime | Med | Med | `load_with_fallback` tries "default" then throws with clear error. Scene handles the exception. |
| Individual SPRJ files missing (incomplete skin) | Low | High | `try_load_sprj` returns nullptr with warning log. Callers skip draw for null sprites. Current default/ skin has 30 of 48 files. |
| Performance: 48 SPRJ loads at scene init | Low | Low | Each SPRJ is a small JSON file (~1KB) with 6 texture references. TextureCache deduplicates. Loading 48 files is well under the 500ms NFR. |
| Sprite positioning incorrect (offset by half-sprite) | Med | Med | Unit test `SpritePositionCenteredOnColumn` verifies the centering math. Integration test renders one note and checks pixel position. |
| Animation timer drift over long play sessions | Low | Low | `std::fmod` resets every 300ms. No accumulation error. |
| Double mode column-to-track mapping incorrect | Med | Low | Unit test verifies `column % NUM_TRACKS` for columns 0-9. |
| Breaking existing Phase 1 tests | Med | Low | NoteSkin pointer is nullable. Existing tests pass nullptr and get rectangle rendering. New constructor signature adds a defaulted parameter. |

## Traceability Matrix

| Requirement | Story | Acceptance Criterion | Test Case | Source File |
|-------------|-------|---------------------|-----------|-------------|
| REQ-REN-009 | US-REN-021 | SC1: Note skin loads | `NoteSkinTest::LoadDefaultSkin` | `noteskin_loader.cpp` |
| REQ-REN-009 | US-REN-021 | SC1: All sprites resolved and cached | `NoteSkinTest::LoadedCountMatchesFiles` | `noteskin_loader.cpp` |
| REQ-REN-009 | US-REN-021 | SC2: Each column uses corresponding sprite | `NoteSkinTest::TapSpritePerTrack` | `noteskin.cpp` |
| REQ-REN-009 | US-REN-021 | SC3: Double mode 10 columns | `NoteSkinTest::DoubleColumnToTrackMapping` | `note_renderer.cpp` |
| REQ-REN-009 | US-REN-021 | SC4: Missing skin fallback | `NoteSkinTest::FallbackToDefault` | `noteskin_loader.cpp` |
| REQ-REN-009 | US-REN-021 | SC4: Placeholder used when no skin | `NoteRendererTest::NullSkinRendersRectangles` | `note_renderer.cpp` |
| REQ-REN-020 | US-REN-021 | Animation: 6 frames at 50ms | `NoteSkinAnimTest::LoopTCycles300ms` | `noteskin_anim_timer.h` |
| REQ-REN-020 | US-REN-021 | Animation: 64x64 sprites | `NoteSkinTest::SpriteDimensions64x64` | `noteskin_loader.cpp` |
| REQ-REN-009 | US-REN-022 | SC1: Receptors at judgment line | `NoteRendererTest::ReceptorPositionAtReceptorY` | `note_renderer.cpp` |
| REQ-REN-009 | US-REN-022 | SC2: Receptor count matches mode | `NoteRendererTest::FiveReceptorsInSingleMode` | `note_renderer.cpp` |
| REQ-REN-009 | US-REN-022 | SC3: Press overlay on input | `NoteRendererTest::PressOverlayWhenColumnPressed` | `note_renderer.cpp` |
| REQ-REN-014 | US-REN-023 | SC1: Perfect sprite appears | `NoteRendererTest::JudgeOverlayOnPerfect` | `note_renderer.cpp` |
| REQ-REN-014 | US-REN-023 | SC2: Miss does not trigger overlay | `NoteRendererTest::NoJudgeOverlayOnMiss` | `note_renderer.cpp` |
| REQ-REN-014 | US-REN-023 | SC3: Judge overlay positioning | `NoteRendererTest::JudgeOverlayAtReceptorPosition` | `note_renderer.cpp` |
| REQ-REN-020 | US-REN-021 | Missing SPRJ logs warning | `NoteSkinTest::MissingSprjLogsWarning` | `noteskin_loader.cpp` |
| REQ-REN-020 | US-REN-021 | Missing default/ is fatal | `NoteSkinTest::MissingDefaultThrows` | `noteskin_loader.cpp` |

## Testing Strategy

### Unit Tests (`test/test_noteskin.cpp`) — Pure Logic, No SDL

These tests verify the data structures, loading logic, and animation math without requiring SDL or a renderer.

**NoteSkin accessors:**
- `TapReturnsNullForUnloaded` — default-constructed NoteSkin returns nullptr for all accessors.
- `TapSpritePerTrack` — after loading, `tap(0)` through `tap(4)` return non-null for existing SPRJs.
- `HoldPartsDistinct` — `hold(0, HEAD)`, `hold(0, BODY)`, `hold(0, TAIL)` return different Sprite pointers.
- `DoubleColumnToTrackMapping` — verify that `column % NUM_TRACKS` correctly maps columns 5-9 to tracks 0-4.

**NoteSkinLoader:**
- `LoadDefaultSkin` — loads the committed noteskin/default/ directory, verifies `loaded_count() == 30` (matching the 30 SPRJ files currently present).
- `LoadedCountMatchesFiles` — loaded_count matches actual SPRJ file count on disk.
- `MissingSprjLogsWarning` — requesting a nonexistent SPRJ produces a warning log and null sprite (use spdlog test sink).
- `MissingDefaultThrows` — calling load with a nonexistent base_dir throws std::runtime_error.
- `FallbackToDefault` — `load_with_fallback("nonexistent_skin")` falls back to "default" successfully.
- `SpriteDimensions64x64` — loaded sprites have pictures with 64x64 rect dimensions.

**NoteSkinAnimTimer:**
- `LoopTCycles300ms` — `noteskin_loop_t(0) == 0.0`, `noteskin_loop_t(150) ≈ 0.5`, `noteskin_loop_t(300) == 0.0`.
- `LoopTWrapsNegative` — negative timestamps handled correctly.
- `OneshotTLinear` — `noteskin_oneshot_t(100, 0) ≈ 0.333`, `noteskin_oneshot_t(300, 0) == 1.0`.
- `OneshotActiveWithin300ms` — `noteskin_oneshot_active(200, 0) == true`, `noteskin_oneshot_active(301, 0) == false`.
- `OneshotBeforeTriggerReturnsZero` — `noteskin_oneshot_t(50, 100) == 0.0`.

**NoteRenderer sprite selection (pure logic, no SDL):**
- `NullSkinRendersRectangles` — constructing NoteRenderer with nullptr skin does not crash; render logic uses fallback.
- `SelectSpriteTapType` — TAP note on column 2 selects `skin->tap(2)`.
- `SelectSpriteFakeType` — FAKE note selects `skin->faketap(track)`.
- `SelectSpriteHoldHead` — HOLD_HEAD note selects `skin->hold(track, HEAD)`.

### Integration Tests — SDL-Dependent

These require a live SDL_Renderer and real noteskin files from `noteskin/default/`.

- `RenderTapNoteWithSprite` — render a single TAP note with the default skin, verify a non-black pixel appears at the expected column position.
- `ReceptorPressOverlayRenders` — trigger a press overlay, verify non-black pixels at receptor position.
- `ReceptorJudgeOverlayRenders` — trigger a judge overlay, verify it disappears after 300ms.
- `AnimationFrameChanges` — render at t=0 and t=50ms, verify different pixels (different animation frame).

### What Is NOT Tested in This TD

- Hold body tiling (Phase 3, US-REN-025)
- Combo number sprites (US-REN-024, separate TD)
- Sprite-based judgment text (future phase)
- Receptor background SPRJ (deferred within Phase 2 until asset creation)
- Double mode column layout (Phase 5, US-REN-031)

---

*Generated from stories in docs/stories/06-visual-rendering.md (Phase 2 subset)*
*Spec reference: docs/noteskin-format-spec.md*
*Last updated: 2026-04-29*
