# TD-REN-001: Note Renderer — Beat-to-Screen Conversion, Placeholder Notes, and Judgment Display

**Stories**: US-REN-019, US-REN-020, US-REN-036
**Phase**: 1
**Author**: technical-architect agent
**Status**: Draft

## Overview

This design introduces the Phase 1 note rendering subsystem: a `NoteRenderer` class that converts notes from beat-space to screen-space vertical positions, draws them as colored rectangles (one distinct color per column), and a `JudgmentDisplay` class that shows the most recent judgment as a colored rectangle with text label. The entire visual layer is intentionally placeholder — rectangles instead of sprites — to unblock gameplay testing without sprite asset dependencies.

The `NoteRenderer` reads `NoteData` and `TimingData` from TD-CHT-001 and queries the current song position (in milliseconds) to determine which notes are visible and where to draw them. It uses the existing `Renderer` class (`src/openitup/gfx/renderer.h`) for all SDL draw calls. The `JudgmentDisplay` reads `JudgmentEvent` objects from TD-JDG-001 and shows the most recent tier as colored feedback.

## Architecture

### Component Diagram

```
GameplayScene (TD-SCN-001)
  |
  |  per-frame:
  |    1. note_renderer_.render(renderer, song_position_ms)
  |    2. judgment_display_.render(renderer)
  |
  v

NoteRenderer (src/openitup/render/note_renderer.h)
  |  reads (const ref, non-owning)
  ├── NoteData (from TD-CHT-001)
  ├── TimingData (from TD-CHT-001)
  └── NoteFieldConfig (pixel layout parameters)
  |
  |  per-frame input:
  └── song_position_ms (double, from AudioSystem)

JudgmentDisplay (src/openitup/render/judgment_display.h)
  |  per-judgment input:
  └── on_judgment(JudgmentTier tier)
  |
  |  per-frame:
  └── render(SDL_Renderer*)
```

### New Types

#### `NoteFieldConfig` (`src/openitup/render/note_renderer.h`, defined alongside NoteRenderer)

A plain struct holding the pixel-space layout parameters for the note field. All values are in the 640x480 logical coordinate space.

```cpp
struct NoteFieldConfig {
    // Vertical position of the receptor line (where notes should be hit).
    float receptor_y = 400.0f;

    // Pixel width and height of each note rectangle.
    float note_width = 48.0f;
    float note_height = 48.0f;

    // Pixels per beat at 1.0x scroll speed. At 120 BPM, one beat = 0.5s.
    // 80 pixels/beat means 4 beats fill 320 pixels of scroll distance.
    float pixels_per_beat = 80.0f;

    // Scroll speed multiplier (1.0x default). Phase 5 adds C-mod/M-mod.
    float scroll_speed = 1.0f;

    // Number of columns (5 for single, 10 for double).
    int num_columns = 5;

    // X position of each column center. Computed at construction.
    // For single mode: centered in 640px with equal spacing.
    std::vector<float> column_x;
};
```

**Key decisions**:

- `pixels_per_beat` is the fundamental scroll parameter. Notes move `pixels_per_beat * scroll_speed` pixels per beat of distance from the receptor. This naturally handles BPM changes because the renderer works in beat-space, not time-space.
- `column_x` is a precomputed vector rather than a formula. This allows Phase 5 to support double mode (10 columns with different spacing) without changing the render loop.
- `receptor_y = 400` places the receptor near the bottom of the 480-pixel logical space, matching PIU's down-scroll convention (notes appear at the top and scroll toward the bottom).
- All float values use `float`, not `double`, because they are pixel coordinates in a 640x480 space where `float` precision is more than sufficient. This matches the existing `LayerTransform` in `types.h`.

---

#### `NoteRenderer` (`src/openitup/render/note_renderer.h`)

The Phase 1 note renderer. Converts beats to vertical screen positions and draws colored rectangles. Has no SDL dependency in its header beyond the forward-declared `SDL_Renderer*` in the render method.

```cpp
// src/openitup/render/note_renderer.h
#pragma once

#include <cstdint>
#include <vector>

#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>

struct SDL_Renderer;

namespace openitup {

struct NoteFieldConfig {
    float receptor_y = 400.0f;
    float note_width = 48.0f;
    float note_height = 48.0f;
    float pixels_per_beat = 80.0f;
    float scroll_speed = 1.0f;
    int num_columns = 5;
    std::vector<float> column_x;
};

class NoteRenderer {
public:
    // Construct for a chart. note_data and timing_data must outlive the renderer.
    NoteRenderer(const NoteData& note_data, const TimingData& timing_data,
                 const NoteFieldConfig& config);

    // Convert a beat position to a Y screen coordinate given the current song beat.
    // Pure function — testable without SDL.
    float beat_to_y(double note_beat, double current_beat) const;

    // Render all visible notes for the current song position.
    // song_position_ms: current audio playback position from AudioSystem.
    void render(SDL_Renderer* renderer, double song_position_ms) const;

    // Render receptor indicators at the receptor line.
    void render_receptors(SDL_Renderer* renderer) const;

    // Access config for external queries (e.g., judgment display positioning).
    const NoteFieldConfig& config() const;

    // Build a default single-mode config (5 columns centered in 640px).
    static NoteFieldConfig default_single_config();

private:
    const NoteData& note_data_;
    const TimingData& timing_data_;
    NoteFieldConfig config_;

    // Per-column colors for placeholder rectangles.
    struct Color { uint8_t r, g, b; };
    static const Color COLUMN_COLORS[];
};

} // namespace openitup
```

**Key decisions**:

- `beat_to_y()` is a public pure function. The judge, scene, and tests can call it without rendering. It computes: `y = receptor_y - (note_beat - current_beat) * pixels_per_beat * scroll_speed`. Notes above the receptor have a smaller beat (in the past relative to current), so they appear below the receptor (already scrolled past). Notes with a larger beat (in the future) appear above.
- `render()` accepts `song_position_ms` and internally converts to beat via `timing_data_.beat_at_time(song_position_ms / 1000.0)`. This means BPM changes and stops are handled automatically — during a stop, `beat_at_time` returns the same beat value, so notes freeze in place.
- The renderer draws only `NoteType::TAP` and `NoteType::HOLD_HEAD` notes in Phase 1. Hold bodies/tails are Phase 3.
- `COLUMN_COLORS` is a static array of 10 colors (5 unique + 5 repeated for double mode). Phase 1 uses columns 0-4 only.
- Forward-declared `SDL_Renderer*` in the render method keeps the header SDL-free. The `.cpp` includes `<SDL3/SDL.h>`.
- `default_single_config()` computes `column_x` for 5 columns centered in 640px with 56px spacing: `column_x = {208, 264, 320, 376, 432}`.

---

#### `JudgmentDisplay` (`src/openitup/render/judgment_display.h`)

Displays the most recent judgment as a colored rectangle with the tier name. This is the Phase 1 placeholder for the sprite-based judgment display in Phase 2 (US-REN-023).

```cpp
// src/openitup/render/judgment_display.h
#pragma once

#include <cstdint>

#include <openitup/judge/judgment_tier.h>

struct SDL_Renderer;

namespace openitup {

class JudgmentDisplay {
public:
    JudgmentDisplay();

    // Called when a new judgment is issued. Updates the displayed tier.
    void on_judgment(JudgmentTier tier);

    // Render the judgment indicator at a fixed screen position.
    // Shows a colored rectangle whose color corresponds to the tier.
    // Fades after display_duration_seconds.
    void render(SDL_Renderer* renderer, double dt) const;

    // The most recently displayed tier (for testing).
    JudgmentTier current_tier() const;

    // True if the display is currently visible (within fade duration).
    bool is_visible() const;

private:
    JudgmentTier current_tier_ = JudgmentTier::MISS;
    mutable double time_since_judgment_ = 999.0;  // start invisible

    static constexpr double DISPLAY_DURATION = 0.5;  // seconds

    // Screen position (640x480 logical space).
    static constexpr float DISPLAY_X = 260.0f;   // centered-ish
    static constexpr float DISPLAY_Y = 200.0f;   // above note field
    static constexpr float DISPLAY_W = 120.0f;
    static constexpr float DISPLAY_H = 40.0f;

    struct TierColor { uint8_t r, g, b; };
    static const TierColor TIER_COLORS[];
};

} // namespace openitup
```

**Key decisions**:

- `on_judgment()` stores the tier and resets the display timer. `render()` draws the colored rectangle if `time_since_judgment_ < DISPLAY_DURATION`. The timer advances via `dt` parameter passed from the scene.
- `time_since_judgment_` is `mutable` because `render()` updates it (accumulates dt). This follows the convention where render-only visual state (fade timers, animations) mutates during the render pass.
- Colors: PERFECT=green(0,255,0), GREAT=cyan(0,200,255), GOOD=yellow(255,255,0), BAD=orange(255,128,0), MISS=red(255,0,0). Distinct and readable against a black background.
- Position is hardcoded for Phase 1. Phase 2 replaces this entire class with sprite-based rendering (US-REN-023).
- No text rendering in Phase 1. SDL3 does not include text rendering without SDL3_ttf. The colored rectangle alone provides sufficient feedback. If the implementer finds a lightweight way to add text (e.g., SDL_RenderDebugText in newer SDL3 builds), that is a bonus, not a requirement.

---

### Modified Types

#### `Renderer` (`src/openitup/gfx/renderer.h`)

No modifications needed. `NoteRenderer` uses `renderer.get()` to obtain the raw `SDL_Renderer*` for drawing.

## File Plan

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/openitup/render/note_renderer.h` | NoteRenderer + NoteFieldConfig declarations |
| Create | `src/openitup/render/note_renderer.cpp` | NoteRenderer implementation (beat_to_y, render, column colors) |
| Create | `src/openitup/render/judgment_display.h` | JudgmentDisplay class declaration |
| Create | `src/openitup/render/judgment_display.cpp` | JudgmentDisplay implementation (on_judgment, render, tier colors) |
| Modify | `CMakeLists.txt` | Add note_renderer.cpp and judgment_display.cpp to openitup_engine |
| Create | `test/test_note_renderer.cpp` | Unit tests for beat_to_y conversion and config |
| Modify | `CMakeLists.txt` | Add test_note_renderer.cpp to openitup_tests |

## Data Flow

### Normal Frame: Notes Scrolling

```
1. GameplayScene::render(alpha):
   song_ms = audio_->get_position_ms()
   note_renderer_.render(renderer.get(), song_ms)
   judgment_display_.render(renderer.get(), dt)

2. NoteRenderer::render(sdl_renderer, 5000.0):
   a. current_beat = timing_data_.beat_at_time(5.0)  // 5000ms = 5s
      At 120 BPM: current_beat = 10.0

   b. Compute visible beat range:
      top_of_screen_beat = current_beat + (receptor_y / (pixels_per_beat * scroll_speed))
      bottom_of_screen_beat = current_beat - ((480 - receptor_y) / (pixels_per_beat * scroll_speed))

   c. Get notes in range via note_data_.notes_in_range(bottom_beat, top_beat)

   d. For each visible note:
      y = beat_to_y(note.beat, current_beat)
      x = column_x[note.column] - note_width/2
      color = COLUMN_COLORS[note.column]
      SDL_SetRenderDrawColor(r, g, b, 255)
      SDL_RenderFillRect(x, y - note_height/2, note_width, note_height)
```

### BPM Change: Notes Freeze During Stop

```
Given: 120 BPM, 2-second stop at beat 16.0

At song_position_ms = 7900ms (just before stop):
  current_beat = beat_at_time(7.9) = 15.8
  Notes near beat 16.0 are visible above receptor

At song_position_ms = 8000ms (stop begins):
  current_beat = beat_at_time(8.0) = 16.0
  Notes are at their positions

At song_position_ms = 9000ms (mid-stop):
  current_beat = beat_at_time(9.0) = 16.0  (still 16.0 — beat frozen during stop)
  Notes remain stationary — beat_to_y returns same values

At song_position_ms = 10000ms (stop ends):
  current_beat = beat_at_time(10.0) = 16.0  (just ending stop)

At song_position_ms = 10001ms (after stop):
  current_beat = beat_at_time(10.001) ≈ 16.002
  Notes resume scrolling
```

### Judgment Feedback

```
1. GameplayScene::update(dt):
   events = judge_->update(song_ms, pressed_columns)
   for (const auto& event : events):
     judgment_display_.on_judgment(event.tier())
   gameplay_state_->apply(events)

2. JudgmentDisplay::on_judgment(PERFECT):
   current_tier_ = PERFECT
   time_since_judgment_ = 0.0

3. JudgmentDisplay::render(renderer, dt):
   time_since_judgment_ += dt
   if time_since_judgment_ < 0.5:
     color = TIER_COLORS[PERFECT]  // green
     SDL_SetRenderDrawColor(0, 255, 0, 255)
     SDL_RenderFillRect(260, 200, 120, 40)
```

## Key Algorithms

### beat_to_y(note_beat, current_beat)

```
float NoteRenderer::beat_to_y(double note_beat, double current_beat) const {
    double beat_delta = note_beat - current_beat;
    // Positive beat_delta = note is in the future = above receptor
    // Negative beat_delta = note is in the past = below receptor
    float y = config_.receptor_y
            - static_cast<float>(beat_delta)
              * config_.pixels_per_beat
              * config_.scroll_speed;
    return y;
}
```

This is a single multiply-add. Notes scroll downward as `current_beat` increases (because `beat_delta` decreases, making `y` increase toward the bottom of the screen). At the moment the note should be hit (`note_beat == current_beat`), `y == receptor_y`.

### Visible Range Computation

```
// Beats visible above the receptor (from receptor to top of screen y=0):
double beats_above = config_.receptor_y / (config_.pixels_per_beat * config_.scroll_speed);
double top_beat = current_beat + beats_above;

// Beats visible below the receptor (from receptor to bottom of screen y=480):
double beats_below = (480.0f - config_.receptor_y) / (config_.pixels_per_beat * config_.scroll_speed);
double bottom_beat = current_beat - beats_below;
```

With defaults (receptor_y=400, pixels_per_beat=80, scroll_speed=1.0):
- beats_above = 400/80 = 5.0 beats visible above receptor
- beats_below = 80/80 = 1.0 beat visible below receptor
- At current_beat=10.0: visible range is [9.0, 15.0]

### Default Single-Mode Column Layout

```
NoteFieldConfig NoteRenderer::default_single_config() {
    NoteFieldConfig config;
    config.num_columns = 5;
    config.column_x.resize(5);

    float center_x = 320.0f;
    float spacing = 56.0f;  // pixels between column centers
    float start_x = center_x - 2.0f * spacing;

    for (int i = 0; i < 5; i++) {
        config.column_x[i] = start_x + static_cast<float>(i) * spacing;
    }
    // column_x = {208, 264, 320, 376, 432}

    return config;
}
```

## Dependencies

### Internal
- **NoteData** (`src/openitup/chart/note_data.h`, TD-CHT-001) — Read-only access to sorted note events via `notes_in_range()`.
- **TimingData** (`src/openitup/chart/timing_data.h`, TD-CHT-001) — `beat_at_time()` to convert song position to current beat.
- **NoteType** (`src/openitup/chart/note_type.h`, TD-CHT-001) — Filter visible notes to TAP and HOLD_HEAD.
- **JudgmentTier** (`src/openitup/judge/judgment_tier.h`, TD-JDG-001) — JudgmentDisplay shows tier feedback.
- **Renderer** (`src/openitup/gfx/renderer.h`) — Existing renderer provides `SDL_Renderer*` for draw calls.
- **SDL3** — `SDL_RenderFillRect`, `SDL_SetRenderDrawColor` for rectangle drawing. Already linked.

### External (new libraries)
None.

## Architectural Decisions

### ADR-1: Beat-Space Rendering, Not Time-Space

- **Context**: US-REN-019 requires notes to scroll in beat-space. BPM changes and stops must not cause visual discontinuities.
- **Decision**: The renderer converts song position (ms) to beat via `TimingData::beat_at_time()`, then positions notes using `(note_beat - current_beat) * pixels_per_beat`. All positioning math is in beat-space.
- **Alternatives considered**: (a) Time-space rendering (convert note beats to ms, compute pixel offsets from time differences) — fails during BPM changes because the visual spacing would change. (b) Pre-computed Y positions per frame — wastes memory and doesn't handle scroll speed changes.
- **Consequences**: BPM changes, stops, and future speed mods (Phase 5) are handled naturally. During a stop, `beat_at_time` returns a frozen beat, so notes freeze. During a BPM change, beats still advance linearly (TimingData handles the time-to-beat conversion), so note spacing remains proportional to beat distance.

### ADR-2: Colored Rectangles as Phase 1 Placeholder

- **Context**: US-REN-020 requires placeholder visuals that validate scroll math without sprite dependencies.
- **Decision**: Draw 48x48 colored rectangles, one distinct color per column.
- **Alternatives considered**: (a) Colored circles — slightly more complex rendering for no benefit. (b) Simple text labels — SDL3 has no built-in text rendering. (c) Colored lines — too small to see clearly.
- **Consequences**: The NoteRenderer's render method is ~30 lines of SDL draw calls. Replacing it with sprite-based rendering in Phase 2 (US-REN-021) changes only the draw calls, not the beat_to_y math or visible range computation.

### ADR-3: JudgmentDisplay as Separate Class

- **Context**: US-REN-036 requires minimal timing feedback. This could be part of NoteRenderer or a separate class.
- **Decision**: Separate `JudgmentDisplay` class with its own render method.
- **Alternatives considered**: Putting judgment display inside NoteRenderer — couples two concerns (note field rendering and feedback display) that have different lifecycles and different replacement schedules (judgment display is replaced in Phase 2, note renderer is extended).
- **Consequences**: GameplayScene calls `judgment_display_.render()` separately from `note_renderer_.render()`. The two can be independently replaced or extended.

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| beat_to_y precision issues with very high BPM (>300) or very small scroll_speed | Low | Low | All arithmetic uses double for beat calculations. Float is used only for final pixel coordinates where sub-pixel precision is meaningless. |
| NoteRenderer iterates all notes per frame instead of visible subset | Med | Low | `notes_in_range()` from TD-CHT-001 returns an iterator pair via binary search. Only visible notes are iterated. |
| Missing text rendering makes judgment display hard to read | Low | Med | Colored rectangles are distinct enough for Phase 1. Each tier has a unique color. Phase 2 replaces with sprites. If SDL_RenderDebugText is available, use it as bonus. |
| Column layout doesn't account for note overlap at high scroll speeds | Low | Low | Phase 1 uses fixed 48x48 rectangles. At default spacing (56px between columns), overlap requires notes to be on adjacent columns at the same beat — which is valid gameplay. No mitigation needed. |

## Testing Strategy

### Unit Tests (`test/test_note_renderer.cpp`) — Pure Logic, No SDL

The beat_to_y conversion and config generation are pure math, testable without SDL.

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `BeatToYAtReceptor` | note_beat == current_beat -> y == receptor_y | US-REN-019 SC1 |
| `BeatToYFourBeatsAbove` | note_beat = current+4.0, pixels_per_beat=80 -> y = receptor_y - 320 | US-REN-019 SC1 |
| `BeatToYBelowReceptor` | note_beat < current_beat -> y > receptor_y | US-REN-019 |
| `BeatToYScrollSpeedMultiplier` | scroll_speed=2.0 -> distance doubles | US-REN-019 SC4 |
| `BeatToYStopFreezesPosition` | During stop: same current_beat -> same y | US-REN-019 SC3 |
| `DefaultSingleConfigFiveColumns` | default_single_config has 5 column_x values | US-REN-020 SC1 |
| `DefaultSingleConfigCentered` | column_x[2] == 320 (center of 640px) | US-REN-020 SC1 |
| `DefaultSingleConfigEqualSpacing` | column_x spacing is uniform | US-REN-020 SC3 |
| `NoteSize48x48` | Default note_width/height == 48 | US-REN-020 SC2 |
| `ColumnColorsDistinct` | All 5 column colors are different | US-REN-020 SC1 |
| `JudgmentDisplayOnJudgment` | on_judgment(PERFECT) sets current_tier to PERFECT | US-REN-036 SC1 |
| `JudgmentDisplayVisible` | After on_judgment, is_visible() returns true | US-REN-036 SC2 |
| `JudgmentDisplayFades` | After DISPLAY_DURATION, is_visible() returns false | US-REN-036 SC2 |
| `JudgmentDisplayUpdatesOnNewJudgment` | Calling on_judgment again resets timer | US-REN-036 SC2 |
| `TierColorsDistinct` | All 5 tier colors are different | US-REN-036 SC3 |

---

*Generated from stories in docs/stories/06-visual-rendering.md (Phase 1 subset)*
*Last updated: 2026-04-28*
