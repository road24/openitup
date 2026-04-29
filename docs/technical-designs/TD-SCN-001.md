# TD-SCN-001: Minimal Gameplay Scene — Phase 1 Integration Point

**Stories**: US-SCN-007a
**Phase**: 1
**Author**: technical-architect agent
**Status**: Draft

## Overview

This design introduces the Phase 1 `MinimalGameplayScene` class — the integration point that wires all Phase 1 subsystems together into a playable experience. It loads a chart via the KSF parser (TD-CHT-001), starts audio playback (TD-AUD-001), captures keyboard input (TD-INP-001), runs the judge each tick (TD-JDG-001), renders placeholder notes (TD-REN-001), and shows judgment feedback. There is no scene stack in Phase 1 — the Engine runs this scene directly via a `run_gameplay()` method.

The scene is deliberately thin — it orchestrates existing subsystems but contains minimal logic of its own. Each subsystem remains independently testable via its own TD's injection points. The scene's unique responsibility is the per-frame "tick script": query audio position, poll input, feed both to the judge, pass events to GameplayState, and render everything in the correct order.

## Architecture

### Component Diagram

```
Engine (TD-ENG-001)
  |  owns (unique_ptr)
  ├── Clock
  ├── Renderer
  ├── AudioSystem (TD-AUD-001)
  └── InputSystem (TD-INP-001)
  |
  |  Engine::run_gameplay(data_dir, chart_path)
  |    constructs MinimalGameplayScene
  |    runs main loop calling scene.update()/scene.render()
  v

MinimalGameplayScene (src/openitup/scene/minimal_gameplay_scene.h)
  |  owns (value or unique_ptr)
  ├── Chart (from KsfParser, TD-CHT-001)
  ├── Judge (TD-JDG-001, references Chart's NoteData + TimingData)
  ├── GameplayState (TD-JDG-001)
  ├── NoteRenderer (TD-REN-001, references Chart's NoteData + TimingData)
  └── JudgmentDisplay (TD-REN-001)
  |
  |  Non-owning references (from Engine):
  ├── AudioSystem*
  ├── InputSystem*
  └── Renderer*
```

### New Types

#### `MinimalGameplayScene` (`src/openitup/scene/minimal_gameplay_scene.h`)

The Phase 1 gameplay scene. Owns the chart, judge, gameplay state, and renderers. References the Engine's audio, input, and renderer subsystems via non-owning pointers.

```cpp
// src/openitup/scene/minimal_gameplay_scene.h
#pragma once

#include <memory>
#include <filesystem>

#include <openitup/chart/chart.h>
#include <openitup/chart/ksf_parser.h>
#include <openitup/judge/judge.h>
#include <openitup/judge/gameplay_state.h>
#include <openitup/render/note_renderer.h>
#include <openitup/render/judgment_display.h>

namespace openitup {

class AudioSystem;
class InputSystem;
class Renderer;

class MinimalGameplayScene {
public:
    // Construct the scene. Loads chart, resolves audio, initializes subsystems.
    // Throws ChartLoadException if chart cannot be loaded.
    // audio_system may be nullptr (gameplay proceeds without audio).
    MinimalGameplayScene(
        const std::filesystem::path& chart_path,
        const std::filesystem::path& data_dir,
        AudioSystem* audio_system,
        InputSystem* input_system,
        Renderer* renderer);

    ~MinimalGameplayScene();

    MinimalGameplayScene(const MinimalGameplayScene&) = delete;
    MinimalGameplayScene& operator=(const MinimalGameplayScene&) = delete;

    // Called once per fixed-step tick (60 Hz).
    // dt: fixed step duration (1/60th second).
    void update(double dt);

    // Called once per render frame (display refresh rate).
    // alpha: interpolation factor [0.0, 1.0) for sub-tick smoothing.
    void render(double alpha);

    // True if the song has completed (audio stopped or all notes judged).
    bool is_complete() const;

    // Access gameplay state (for future result screen or testing).
    const GameplayState& gameplay_state() const;

    // Access judge (for testing).
    const Judge& judge() const;

private:
    // Owned subsystems — constructed at scene init.
    Chart chart_;
    Judge judge_;
    GameplayState gameplay_state_;
    NoteRenderer note_renderer_;
    JudgmentDisplay judgment_display_;

    // Non-owning references to Engine subsystems.
    AudioSystem* audio_;
    InputSystem* input_;
    Renderer* renderer_;

    // Scene state.
    bool audio_started_ = false;
    bool complete_ = false;
    double last_song_ms_ = 0.0;

    // Start audio playback (called on first update).
    void start_audio();
};

} // namespace openitup
```

**Key decisions**:

- The scene stores `Chart` by value (moved in from the parser). `Judge` and `NoteRenderer` hold const references to `chart_.note_data()` and `chart_.timing_data()` — both are stable because `Chart` is owned by the scene and does not move after construction.
- `AudioSystem*` is a raw non-owning pointer (nullable). If the audio system failed to initialize (TD-AUD-001 allows graceful degradation), the scene proceeds without audio. `get_position_ms()` returns 0.0 when audio is null, which means the judge auto-misses all notes — acceptable for a "no audio" degraded state.
- `InputSystem*` is a raw non-owning pointer. It is polled by Engine before `update()` is called, so the scene reads the snapshot via `input_->snapshot()`.
- `Renderer*` is a raw non-owning pointer. The scene uses `renderer_->get()` to obtain the `SDL_Renderer*` for NoteRenderer and JudgmentDisplay.
- `audio_started_` defers audio `play()` to the first `update()` call. This ensures the audio system is fully initialized before playback begins and allows a brief startup delay for the renderer to show the first frame.
- No scene stack operations. `is_complete()` signals to Engine that the song is over, and Engine simply continues the loop (Phase 1 has no result screen).

---

### Modified Types

#### `Engine` (`src/openitup/core/engine.h`)

- Add method: `int run_gameplay(const std::filesystem::path& chart_path, const std::filesystem::path& data_dir)` — Creates a `MinimalGameplayScene`, runs the main loop calling `scene.update()` and `scene.render()`, and returns when the scene is complete or quit is requested.
- Reason: Phase 1 has no scene stack. The Engine needs a direct way to run the gameplay scene. This method is removed in Phase 2 when the scene stack is introduced.

#### `Engine` (`src/openitup/core/engine.cpp`)

The `run_gameplay()` method replaces the stub `update()` and `render()` calls in `run()` with scene-aware logic:

```cpp
int Engine::run_gameplay(const std::filesystem::path& chart_path,
                         const std::filesystem::path& data_dir) {
    try {
        auto scene = std::make_unique<MinimalGameplayScene>(
            chart_path, data_dir,
            audio_.get(), input_system_.get(), renderer_.get());

        running_ = true;
        clock_->reset();
        accumulator_ = 0.0;
        tick_count_ = 0;

        while (running_ && !scene->is_complete()) {
            process_events();

            double delta = clock_->tick();
            auto result = compute_fixed_steps(delta, accumulator_);
            accumulator_ = result.new_accumulator;
            render_alpha_ = result.alpha;

            if (result.spiral_guard_triggered) {
                spdlog::warn("Spiral-of-death guard triggered");
            }

            for (int i = 0; i < result.num_steps; i++) {
                try {
                    if (input_system_) input_system_->poll(tick_count_);
                    scene->update(FIXED_STEP);
                } catch (const std::exception& e) {
                    spdlog::error("Exception in update: {}", e.what());
                }
                tick_count_++;
            }

            renderer_->begin_frame();
            try {
                scene->render(render_alpha_);
            } catch (const std::exception& e) {
                spdlog::error("Exception in render: {}", e.what());
            }
            renderer_->end_frame();
        }

        spdlog::info("Gameplay complete. Score: {}, Max combo: {}",
                     scene->gameplay_state().score(),
                     scene->gameplay_state().max_combo());
        return 0;

    } catch (const std::exception& e) {
        spdlog::error("Failed to start gameplay: {}", e.what());
        return 1;
    }
}
```

#### `main.cpp` (`src/openitup/main.cpp`)

- After CLI parsing and data directory resolution, call `engine.run_gameplay(chart_path, data_dir)` instead of `engine.run()`.
- Reason: Phase 1 entry point goes directly to gameplay.

## File Plan

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/openitup/scene/minimal_gameplay_scene.h` | MinimalGameplayScene class declaration |
| Create | `src/openitup/scene/minimal_gameplay_scene.cpp` | Scene implementation (init, update, render, audio start) |
| Modify | `src/openitup/core/engine.h` | Add run_gameplay() method declaration |
| Modify | `src/openitup/core/engine.cpp` | Implement run_gameplay() with scene-aware main loop |
| Modify | `src/openitup/main.cpp` | Wire CLI args to engine.run_gameplay() |
| Modify | `CMakeLists.txt` | Add minimal_gameplay_scene.cpp to openitup_engine |
| Create | `test/test_minimal_gameplay_scene.cpp` | Unit tests for scene update/render orchestration |
| Modify | `CMakeLists.txt` | Add test file to openitup_tests |

## Data Flow

### Scene Construction

```
MinimalGameplayScene::MinimalGameplayScene(chart_path, data_dir, audio, input, renderer):

1. Parse chart:
   KsfParser parser;
   chart_ = parser.parse(chart_path);
   spdlog::info("Loaded chart: '{}', {} notes, BPM {}",
                chart_.metadata().title,
                chart_.note_count(),
                chart_.metadata().display_bpm);

2. Initialize judge:
   judge_ = Judge(chart_.note_data(), chart_.timing_data(),
                  default_timing_profile());

3. Initialize gameplay state:
   gameplay_state_ = GameplayState(judge_.total_judgable());

4. Initialize note renderer:
   auto config = NoteRenderer::default_single_config();
   note_renderer_ = NoteRenderer(chart_.note_data(), chart_.timing_data(), config);

5. Initialize judgment display:
   judgment_display_ = JudgmentDisplay();

6. Load audio:
   if (audio_) {
       std::string audio_file = chart_.metadata().audio_path;
       if (!audio_file.empty()) {
           auto audio_path = data_dir / audio_file;
           if (!audio_->load_music(audio_path)) {
               spdlog::warn("Failed to load audio: {}", audio_path.string());
               audio_ = nullptr;  // proceed without audio
           }
       } else {
           spdlog::warn("Chart has no audio file reference");
           audio_ = nullptr;
       }
   }

7. Store non-owning references:
   audio_ = audio;
   input_ = input;
   renderer_ = renderer;
```

### Per-Tick Update (60 Hz)

```
MinimalGameplayScene::update(double dt):

1. Start audio on first update:
   if (!audio_started_ && audio_) {
       audio_->play();
       audio_started_ = true;
   }

2. Get song position:
   double song_ms = 0.0;
   if (audio_ && audio_->get_state() == AudioState::PLAYING) {
       song_ms = audio_->get_position_ms();
   }
   last_song_ms_ = song_ms;

3. Get input:
   uint32_t pressed = 0;
   if (input_) {
       pressed = input_->snapshot().pressed_mask() & 0x03FF;
       // Mask to 10 panel bits (P1 bits 0-4, P2 bits 5-9)
   }

4. Run judge:
   auto events = judge_.update(song_ms, pressed);

5. Update gameplay state:
   for (const auto& event : events) {
       judgment_display_.on_judgment(event.tier());
   }
   gameplay_state_.apply(events);

6. Check completion:
   if (audio_ && audio_->get_state() == AudioState::STOPPED && audio_started_) {
       auto remaining = judge_.flush_remaining();
       gameplay_state_.apply(remaining);
       complete_ = true;
       spdlog::info("Song complete. Final score: {}", gameplay_state_.score());
   }
   if (judge_.is_complete()) {
       complete_ = true;
   }
```

### Per-Frame Render

```
MinimalGameplayScene::render(double alpha):

1. Render note field:
   note_renderer_.render_receptors(renderer_->get());
   note_renderer_.render(renderer_->get(), last_song_ms_);

2. Render judgment feedback:
   judgment_display_.render(renderer_->get(), Engine::FIXED_STEP);
   // Note: using FIXED_STEP as dt approximation. The judgment display
   // fade timer doesn't need sub-frame precision.
```

### End-to-End: Player Hits a Note

```
Frame N (60 Hz tick):
  1. Engine::process_events() → SDL_PollEvent drains queue
  2. input_system_->poll(tick) → KeyboardDriver reads SDL_GetKeyboardState
     Player pressed E key → P1_CENTER bit set in pressed_mask
  3. scene->update(1/60):
     song_ms = audio_->get_position_ms() → 5016.0
     pressed = 0x0004 (P1_CENTER)
     judge_.update(5016.0, 0x0004):
       Note at beat 20.0 on column 2 (P1_CENTER)
       note_time = timing_data.time_at_beat(20.0) * 1000.0 = 5000.0
       error = 5016.0 - 5000.0 = +16.0ms
       classify(16.0) → PERFECT (boundary inclusive)
       emit JudgmentEvent(idx, 2, 20.0, PERFECT, +16.0, false)
     judgment_display_.on_judgment(PERFECT) → green rectangle queued
     gameplay_state_.apply([PERFECT]) → combo++, score += 1000

Frame N render:
  4. renderer->begin_frame() → clear black
  5. scene->render(alpha):
     note_renderer_.render_receptors() → 5 gray rectangles at y=400
     note_renderer_.render(5016.0) → colored rectangles for visible notes
     judgment_display_.render() → green rectangle at (260, 200)
  6. renderer->end_frame() → present
```

## Dependencies

### Internal
- **Chart** (`src/openitup/chart/chart.h`, TD-CHT-001) — Owned by the scene. Provides NoteData + TimingData.
- **KsfParser** (`src/openitup/chart/ksf_parser.h`, TD-CHT-001) — Used in constructor to parse the chart file.
- **Judge** (`src/openitup/judge/judge.h`, TD-JDG-001) — Owned by the scene. Called per-tick with song position and pressed columns.
- **GameplayState** (`src/openitup/judge/gameplay_state.h`, TD-JDG-001) — Owned by the scene. Accumulates score/combo.
- **NoteRenderer** (`src/openitup/render/note_renderer.h`, TD-REN-001) — Owned by the scene. Renders placeholder note rectangles.
- **JudgmentDisplay** (`src/openitup/render/judgment_display.h`, TD-REN-001) — Owned by the scene. Shows judgment feedback.
- **AudioSystem** (`src/openitup/audio/audio_system.h`, TD-AUD-001) — Non-owning pointer. Provides `get_position_ms()`, `play()`, `load_music()`.
- **InputSystem** (`src/openitup/input/input_system.h`, TD-INP-001) — Non-owning pointer. Provides `snapshot().pressed_mask()`.
- **Renderer** (`src/openitup/gfx/renderer.h`) — Non-owning pointer. Provides `SDL_Renderer*` for drawing.
- **DataDirectory** (`src/openitup/asset/data_directory.h`, TD-AST-001) — Used by main.cpp to resolve paths before scene construction.

### External (new libraries)
None.

## Architectural Decisions

### ADR-1: Direct Scene Execution, No Scene Stack

- **Context**: Phase 1 has one screen (gameplay). A scene stack (US-SCN-001/002) is Phase 2.
- **Decision**: Engine gets a temporary `run_gameplay()` method that constructs and runs the gameplay scene directly. No push/pop/replace operations.
- **Alternatives considered**: (a) Build a minimal scene stack now — adds complexity for one scene. (b) Put all gameplay logic in Engine — violates separation of concerns and makes the gameplay scene untestable.
- **Consequences**: Phase 2 removes `run_gameplay()` and introduces the scene stack. The `MinimalGameplayScene` class either becomes the foundation for the full `GameplayScene` or is replaced entirely. The internal subsystem wiring (judge + renderer + audio) carries forward regardless.

### ADR-2: Scene Owns Chart by Value

- **Context**: The chart must outlive the judge and note renderer (which hold const references to NoteData and TimingData). Who owns the chart?
- **Decision**: The scene owns `Chart` as a value member. It is moved in from the parser during construction.
- **Alternatives considered**: (a) Engine owns the chart — adds a member to Engine that only gameplay needs. (b) `shared_ptr<Chart>` — unnecessary shared ownership when the scene is the sole consumer. (c) Pass NoteData/TimingData separately — loses the Chart abstraction.
- **Consequences**: The chart's lifetime matches the scene's lifetime. Judge and NoteRenderer hold references to `chart_.note_data()` and `chart_.timing_data()`, which are stable because the chart is a value member that does not move after the constructor's member initializer list.

### ADR-3: Audio Nullable for Graceful Degradation

- **Context**: TD-AUD-001 specifies that audio init can fail gracefully. The gameplay scene must handle "no audio" without crashing.
- **Decision**: `AudioSystem* audio_` is a nullable raw pointer. When null, `song_ms` defaults to 0.0 and the judge auto-misses all notes.
- **Alternatives considered**: (a) Require audio — makes the game unplayable on headless systems or when audio device is unavailable. (b) Mock audio with a clock-based position — adds complexity for an edge case.
- **Consequences**: The game technically "runs" without audio but is unplayable (all notes auto-miss). This is acceptable for Phase 1 — the goal is "first playable build," and audio is expected to work on development machines. Phase 2 can add a clock-based fallback if needed.

### ADR-4: Judgment Display dt Approximation

- **Context**: `JudgmentDisplay::render()` needs `dt` to advance its fade timer. The render method doesn't know the exact time between frames.
- **Decision**: Pass `FIXED_STEP` (1/60) as an approximation. The judgment display is a cosmetic timer (0.5 second fade) where 16ms accuracy is more than sufficient.
- **Alternatives considered**: (a) Track actual render dt from Clock — adds complexity for a visual fade timer. (b) Use `alpha` to compute fractional dt — over-engineering. (c) Use wall-clock time inside JudgmentDisplay — couples it to a time source.
- **Consequences**: The judgment fade may be slightly longer or shorter on high-refresh displays (the timer advances once per logic tick, not once per render frame). This is imperceptible for a 0.5-second fade.

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Chart references audio file that doesn't exist | Med | Med | Constructor checks file existence after resolving path. Falls back to null audio with warning. |
| Judge and NoteRenderer hold dangling references if Chart moves | High | Low | Chart is a value member of the scene, constructed in the initializer list. It does not move after construction. Judge and NoteRenderer are constructed after Chart. |
| Audio position jumps backward (e.g., due to seek or underrun) | Med | Low | Judge's cursor-based design (TD-JDG-001) handles this correctly — cursor only advances forward, so backward position doesn't re-judge notes. |
| Phase 2 scene stack requires significant refactoring of this class | Med | High (by design) | This is expected. The class is intentionally minimal. The internal wiring (chart→judge→state, chart→renderer) carries forward. Only the Engine integration changes. |
| First frame renders before audio starts (visual-audio desync) | Low | Med | Audio starts on first update, not in constructor. First frame shows notes at position 0. This is 1/60th of a second before audio begins — imperceptible. |

## Testing Strategy

### Unit Tests (`test/test_minimal_gameplay_scene.cpp`) — With Mock Subsystems

The scene orchestration can be tested using mock audio (from TD-AUD-001's `MockAudioSystem`) and a mock input driver.

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `SceneConstructsFromKsf` | Chart loads, judge initializes, note renderer created | US-SCN-007a SC1 |
| `UpdateQueriesAudioPosition` | After update, judge receives audio position | US-SCN-007a SC2 |
| `UpdateProcessesInput` | Pressed column produces judgment event | US-SCN-007a SC4 |
| `JudgmentFeedbackDisplayed` | After judgment, judgment_display shows tier | US-SCN-007a SC5 |
| `SongCompletionDetected` | When audio stops, is_complete() returns true | US-SCN-007a SC6 |
| `NullAudioDoesNotCrash` | Scene with nullptr audio runs update without crash | US-AST-032 |
| `GameplayStateAccumulates` | Score and combo track correctly across updates | US-SCN-007a SC4 |

### Integration Test

One manual integration test validates the full pipeline:

```bash
# Requires a song directory with pumptris.ksf and pumptris.ogg
./build/openitup --data-dir /path/to/pumptris/
# Expected: window opens, notes scroll, audio plays, keys produce judgments
```

---

*Generated from stories in docs/stories/07-screen-flow.md (Phase 1: US-SCN-007a)*
*Last updated: 2026-04-28*
