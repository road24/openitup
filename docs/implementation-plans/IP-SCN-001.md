# IP-SCN-001: Minimal Gameplay Scene Phase 1 Implementation Plan

**Design**: TD-SCN-001
**Stories**: US-SCN-007a
**Total Steps**: 5
**Estimated Total**: ~3.5 hours
**Author**: technical-lead agent
**Status**: Draft

## Prerequisites

ALL of the following must be implemented before this plan starts. This is the final Phase 1 integration — it wires everything together.

- **TD-ENG-001 / IP-ENG-001**: `Engine`, `Clock`, `EngineConfig`, `compute_fixed_steps()` — the game loop
- **TD-INP-001 / IP-INP-001**: `InputSystem`, `KeyboardDriver`, `InputSnapshot`, `PadInput` — keyboard input
- **TD-AUD-001 / IP-AUD-001**: `AudioSystem`, `SDL3AudioSystem`, decoders — audio playback + position tracking
- **TD-CHT-001 / IP-CHT-001**: `Chart`, `NoteData`, `TimingData`, `KsfParser`, `ChartBuilder` — chart loading
- **TD-JDG-001 / IP-JDG-001**: `Judge`, `GameplayState`, `JudgmentTier`, `TimingProfile` — gameplay judge
- **TD-REN-001 / IP-REN-001**: `NoteRenderer`, `JudgmentDisplay`, `NoteFieldConfig` — placeholder rendering
- **TD-AST-001 / IP-AST-001**: `DataDirectory`, CLI parsing, env var support — data directory resolution

---

## Step 1: Create MinimalGameplayScene Class Skeleton

**Files**:
- Create `src/openitup/scene/minimal_gameplay_scene.h` — Class declaration
- Create `src/openitup/scene/minimal_gameplay_scene.cpp` — Constructor, destructor, accessors, stub update/render
- Modify `CMakeLists.txt` — Add `src/openitup/scene/minimal_gameplay_scene.cpp` to `openitup_engine`

**What to implement**:

The class skeleton that compiles against all prerequisite headers but has stub implementations for `update()` and `render()`.

Constructor logic:
```cpp
MinimalGameplayScene::MinimalGameplayScene(
    const std::filesystem::path& chart_path,
    const std::filesystem::path& data_dir,
    AudioSystem* audio_system,
    InputSystem* input_system,
    Renderer* renderer)
    : chart_(KsfParser().parse(chart_path)),
      judge_(chart_.note_data(), chart_.timing_data(), default_timing_profile()),
      gameplay_state_(judge_.total_judgable()),
      note_renderer_(chart_.note_data(), chart_.timing_data(),
                     NoteRenderer::default_single_config()),
      judgment_display_(),
      audio_(audio_system),
      input_(input_system),
      renderer_(renderer)
{
    spdlog::info("Loaded chart '{}': {} notes, BPM {:.0f}",
                 chart_.metadata().title,
                 chart_.note_count(),
                 chart_.metadata().display_bpm);

    // Load audio
    if (audio_) {
        std::string audio_file = chart_.metadata().audio_path;
        if (!audio_file.empty()) {
            auto audio_path = data_dir / std::filesystem::path(audio_file).filename();
            if (!audio_->load_music(audio_path)) {
                spdlog::warn("Failed to load audio '{}', proceeding without",
                             audio_path.string());
                audio_ = nullptr;
            }
        } else {
            spdlog::warn("Chart has no audio file reference");
            audio_ = nullptr;
        }
    }
}
```

Key implementation notes:
- The member initializer list order must match declaration order in the header: `chart_`, `judge_`, `gameplay_state_`, `note_renderer_`, `judgment_display_`, then the pointers.
- `chart_` is move-constructed from the parser's return value.
- `judge_` and `note_renderer_` take const references to `chart_`'s NoteData and TimingData. This is safe because `chart_` is initialized first in the initializer list.
- Audio path resolution: take the filename from the chart's audio_path and look for it in data_dir. This handles the common case where KSF lists `#AUDIOFILE:song.ogg;` and the file is in the same directory.

Stub methods:
```cpp
void MinimalGameplayScene::update(double dt) {
    // Implemented in Step 2
}

void MinimalGameplayScene::render(double alpha) {
    // Implemented in Step 3
}

bool MinimalGameplayScene::is_complete() const {
    return complete_;
}

const GameplayState& MinimalGameplayScene::gameplay_state() const {
    return gameplay_state_;
}

const Judge& MinimalGameplayScene::judge() const {
    return judge_;
}
```

**Tests**:

No tests yet — construction requires a real chart file. Testing happens in Step 4.

Compile verification:
```bash
cmake --build build -j$(nproc)
# Must succeed — verifies all includes resolve and link correctly
```

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] All existing tests still pass
- [ ] Scene compiles against all subsystem headers

**Expected commit message**:
`feat(scene): add MinimalGameplayScene skeleton with chart loading and subsystem wiring`

**Estimated time**: ~45 minutes

---

## Step 2: Implement update() — Judge + Input + Audio Tick

**Files**:
- Modify `src/openitup/scene/minimal_gameplay_scene.cpp` — Implement update()

**What to implement**:

The per-tick update method following TD-SCN-001's data flow:

```cpp
void MinimalGameplayScene::update(double dt) {
    // Start audio on first tick
    if (!audio_started_ && audio_) {
        audio_->play();
        audio_started_ = true;
        spdlog::info("Audio playback started");
    }

    // Get song position from audio (authoritative time source)
    double song_ms = 0.0;
    if (audio_ && audio_->get_state() == AudioState::PLAYING) {
        song_ms = audio_->get_position_ms();
    }
    last_song_ms_ = song_ms;

    // Get input (InputSystem was already polled by Engine before this call)
    uint32_t pressed = 0;
    if (input_) {
        pressed = input_->snapshot().pressed_mask() & 0x03FF;
    }

    // Run judge
    auto events = judge_.update(song_ms, pressed);

    // Feed events to displays and state
    for (const auto& event : events) {
        judgment_display_.on_judgment(event.tier());

        if (spdlog::should_log(spdlog::level::debug)) {
            spdlog::debug("Judgment: {} col={} error={:.1f}ms {}",
                         judgment_tier_to_string(event.tier()),
                         event.column(),
                         event.timing_error_ms(),
                         event.is_auto_miss() ? "(auto-miss)" : "");
        }
    }
    gameplay_state_.apply(events);

    // Check song completion
    if (audio_started_ && audio_ && audio_->get_state() == AudioState::STOPPED) {
        if (!complete_) {
            auto remaining = judge_.flush_remaining();
            gameplay_state_.apply(remaining);
            complete_ = true;
            spdlog::info("Song complete. Score: {}, Max combo: {}, Perfects: {}",
                         gameplay_state_.score(),
                         gameplay_state_.max_combo(),
                         gameplay_state_.count(JudgmentTier::PERFECT));
        }
    }

    // Also complete if all notes judged (edge case: chart with no audio)
    if (!complete_ && judge_.is_complete()) {
        complete_ = true;
    }
}
```

Key implementation notes:
- Audio starts on first tick, not in constructor. This gives the renderer one frame to show before audio begins.
- `pressed_mask() & 0x03FF` extracts the 10 panel bits (P1 bits 0-4 + P2 bits 5-9). Phase 1 only uses P1 bits 0-4 from the keyboard driver.
- Debug logging is guarded by `should_log(debug)` to avoid string formatting overhead in release builds.
- Completion is detected by audio state (STOPPED) or all notes judged. Both paths call `flush_remaining()` to ensure all notes get a judgment.

**Tests**:

No new tests — update() depends on subsystem interactions tested in Step 4.

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] All existing tests still pass

**Expected commit message**:
`feat(scene): implement MinimalGameplayScene::update with judge, input, and audio integration`

**Estimated time**: ~30 minutes

---

## Step 3: Implement render() — Notes + Judgment Display

**Files**:
- Modify `src/openitup/scene/minimal_gameplay_scene.cpp` — Implement render()

**What to implement**:

```cpp
void MinimalGameplayScene::render(double alpha) {
    if (!renderer_) return;

    SDL_Renderer* sdl_renderer = renderer_->get();

    // Render note field
    note_renderer_.render_receptors(sdl_renderer);
    note_renderer_.render(sdl_renderer, last_song_ms_);

    // Render judgment feedback
    // Use FIXED_STEP as dt approximation for the fade timer
    judgment_display_.render(sdl_renderer, 1.0 / 60.0);
}
```

Key implementation notes:
- `last_song_ms_` was stored during `update()`. Using it here (rather than re-querying audio) ensures the note positions match the judge's timing for this tick.
- Render order: receptors first (background), then notes (over receptors), then judgment display (overlay). This matches the painter's algorithm used throughout the codebase.
- No BGA rendering in Phase 1. The background is black (cleared by `renderer_->begin_frame()`).
- `alpha` parameter is available for future sub-tick interpolation but unused in Phase 1 placeholder rendering.

**Tests**:

No automated render tests — visual verification only. This is consistent with the project's three-tier testing strategy where rendering is verified via integration and regression tests.

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] All existing tests still pass

**Expected commit message**:
`feat(scene): implement MinimalGameplayScene::render with notes and judgment display`

**Estimated time**: ~15 minutes

---

## Step 4: Add Engine::run_gameplay() and Wire main.cpp

**Files**:
- Modify `src/openitup/core/engine.h` — Add `run_gameplay()` method declaration
- Modify `src/openitup/core/engine.cpp` — Implement `run_gameplay()` with scene-aware loop
- Modify `src/openitup/main.cpp` — Wire CLI args to engine.run_gameplay()

**What to implement**:

Add to `Engine`:
```cpp
// In engine.h:
int run_gameplay(const std::filesystem::path& chart_path,
                 const std::filesystem::path& data_dir);
```

Implementation in engine.cpp:
```cpp
int Engine::run_gameplay(const std::filesystem::path& chart_path,
                         const std::filesystem::path& data_dir) {
    try {
        auto scene = std::make_unique<MinimalGameplayScene>(
            chart_path, data_dir,
            audio_.get(),
            input_system_ ? input_system_.get() : nullptr,
            renderer_.get());

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
                    spdlog::error("Exception in gameplay update: {}", e.what());
                }
                tick_count_++;
            }

            renderer_->begin_frame();
            try {
                scene->render(render_alpha_);
            } catch (const std::exception& e) {
                spdlog::error("Exception in gameplay render: {}", e.what());
            }
            renderer_->end_frame();
        }

        // Log final results
        const auto& state = scene->gameplay_state();
        spdlog::info("=== Results ===");
        spdlog::info("Score: {}", state.score());
        spdlog::info("Max combo: {}", state.max_combo());
        spdlog::info("Perfect: {}, Great: {}, Good: {}, Bad: {}, Miss: {}",
                     state.count(JudgmentTier::PERFECT),
                     state.count(JudgmentTier::GREAT),
                     state.count(JudgmentTier::GOOD),
                     state.count(JudgmentTier::BAD),
                     state.count(JudgmentTier::MISS));

        return 0;

    } catch (const ChartLoadException& e) {
        spdlog::error("Failed to load chart: {}", e.what());
        return 1;
    } catch (const std::exception& e) {
        spdlog::error("Gameplay error: {}", e.what());
        return 1;
    }
}
```

Update `main.cpp` to use run_gameplay:
```cpp
// After CLI parsing and data directory validation:
auto data_dir = openitup::resolve_data_directory(data_dir_arg);
if (!data_dir || !data_dir->validate()) return 1;

// Find chart file
std::filesystem::path chart_file;
if (!chart_arg.empty()) {
    chart_file = chart_arg;
} else {
    auto found = data_dir->find_file_by_extension(".ksf");
    if (!found) {
        spdlog::error("No .ksf chart found in '{}'", data_dir->path().string());
        return 1;
    }
    chart_file = *found;
}

spdlog::info("Chart: {}", chart_file.string());
spdlog::info("Data dir: {}", data_dir->path().string());

openitup::EngineConfig config;
openitup::Engine engine(config);
return engine.run_gameplay(chart_file, data_dir->path());
```

**Tests**:

Manual integration test (the "first playable" milestone):
```bash
# Prepare a song directory with a KSF chart and audio file
./build/openitup --data-dir /path/to/pumptris/

# Expected behavior:
# 1. Window opens with black background
# 2. Audio starts playing
# 3. Colored rectangles scroll downward
# 4. Pressing Q/W/E/A/S produces judgment feedback (colored rect)
# 5. When song ends, log shows final score
# 6. ESC or window close exits

# Verify with explicit chart:
./build/openitup --chart /path/to/pumptris/pumptris.ksf --data-dir /path/to/pumptris/
```

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] All existing tests still pass
- [ ] `./build/openitup --data-dir <song_dir>` launches gameplay
- [ ] Notes scroll, audio plays, input produces judgments
- [ ] Song completion logs final score

**Expected commit message**:
`feat(scene): wire MinimalGameplayScene into Engine with run_gameplay() for Phase 1 first-playable`

**Estimated time**: ~1 hour

---

## Step 5: Add Unit Tests for Scene Orchestration

**Files**:
- Create `test/test_minimal_gameplay_scene.cpp` — Unit tests with mock subsystems
- Modify `CMakeLists.txt` — Add test file to `openitup_tests`

**What to implement**:

Test the scene's orchestration logic using the MockAudioSystem from TD-AUD-001 and a MockDriver from TD-INP-001. These tests construct a scene with a known chart fixture (inline KSF content via injectable FileReaderFn) and verify behavior without SDL audio/rendering.

Test helpers:
```cpp
// Build a minimal chart for testing
Chart make_test_chart() {
    ChartBuilder builder;
    builder.set_title("Test");
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.set_audio_path("test.ogg");
    // 4 quarter notes, one per beat on columns 0-3
    builder.add_note(1.0, 0, NoteType::TAP);
    builder.add_note(2.0, 1, NoteType::TAP);
    builder.add_note(3.0, 2, NoteType::TAP);
    builder.add_note(4.0, 3, NoteType::TAP);
    return builder.build();
}
```

Note: Since MinimalGameplayScene's constructor calls KsfParser, we need to either:
(a) Create a temporary KSF fixture file in /tmp for each test, or
(b) Add a constructor overload that accepts a pre-built Chart.

**Option (b) is preferred** — add a test-friendly constructor:
```cpp
// Test-only constructor: accepts a pre-built Chart (no file parsing)
MinimalGameplayScene(Chart chart, AudioSystem* audio, InputSystem* input, Renderer* renderer);
```

Test cases:
- `ConstructWithChart` — Build chart, construct scene, verify note_count and judge.total_judgable match
- `UpdateQueriesAudioPosition` — Mock audio at 1000ms, update, verify judge processes notes near beat 2.0
- `UpdateProcessesInput` — Mock input with P1_CENTER pressed at note time, verify PERFECT judgment
- `JudgmentUpdatesDisplay` — After input match, judgment_display.current_tier() matches
- `SongCompletionWhenAudioStops` — Set mock audio state to STOPPED, update, verify is_complete()
- `NullAudioNoExceptions` — Construct with nullptr audio, call update 60 times — no crash
- `GameplayStateAccumulates` — Hit 3 notes as PERFECT, verify score==3000 and combo==3
- `FlushOnComplete` — 4 notes, hit 2, audio stops -> remaining 2 are MISS, total judged==4

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R MinimalGameplay` passes all tests
- [ ] Tests do not require SDL audio device or display (mock subsystems only)

**Expected commit message**:
`test(scene): add MinimalGameplayScene unit tests with mock audio and input`

**Estimated time**: ~1 hour

---

## Summary

| Step | What | Files Created/Modified | Stories Covered | Est. |
|------|------|----------------------|-----------------|------|
| 1 | Scene skeleton + chart loading | 2 new + 1 modified | US-SCN-007a SC1 | 45m |
| 2 | update() implementation | 1 modified | US-SCN-007a SC2, SC4 | 30m |
| 3 | render() implementation | 1 modified | US-SCN-007a SC3, SC5 | 15m |
| 4 | Engine wiring + main.cpp | 3 modified | US-SCN-007a (all) | 1h |
| 5 | Unit tests with mocks | 1 new + 1 modified | US-SCN-007a (verification) | 1h |

**Total new source files**: 2 (1 header + 1 .cpp in `src/openitup/scene/`)
**Total new test files**: 1 (`test/test_minimal_gameplay_scene.cpp`)
**CMakeLists.txt modifications**: Add 1 .cpp to `openitup_engine`, add 1 test file to `openitup_tests`

Steps 1-3 build the scene incrementally. Step 4 is the "first playable" milestone — the moment you can launch the game and play a song. Step 5 adds automated verification.

## Acceptance Verification

| Story ID | How to verify |
|----------|--------------|
| US-SCN-007a SC1 | `./build/openitup --chart /path/to/song.ksf --data-dir /path/` loads chart successfully |
| US-SCN-007a SC2 | Log output shows audio position advancing; judge processes notes at correct times |
| US-SCN-007a SC3 | Visual: colored rectangles scroll downward at BPM-appropriate speed |
| US-SCN-007a SC4 | Pressing Q/W/E/A/S while notes are at receptor produces "PERFECT"/"GREAT" in logs |
| US-SCN-007a SC5 | Visual: colored judgment rectangle appears on screen after hitting a note |
| US-SCN-007a SC6 | When audio ends, log shows "Song complete" with final score and judgment breakdown |

## Phase 1 Exit Criteria (from engine-roadmap.md)

After this plan completes:
> "Launch the engine, point it at a song folder, hear the music, see notes scrolling in time, press keys, see timing feedback on screen."

This is achieved by:
```bash
./build/openitup --data-dir /path/to/song/
```
