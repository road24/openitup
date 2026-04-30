# IP-REN-002: Sprite-Based Note Skins Implementation Plan

**Design**: TD-REN-002
**Stories**: US-REN-021, US-REN-022, US-REN-023
**Total Steps**: 6
**Estimated Total**: ~3.0 hours
**Author**: technical-lead agent
**Status**: Draft

## Overview

This plan implements sprite-based noteskin rendering to replace Phase 1 placeholder rectangles. The core architecture is: `NoteSkin` (data holder) owns all sprites, `NoteSkinLoader` (factory) loads them from disk, `NoteRenderer` consumes a `NoteSkin*` to render animated arrows and 3-layer receptor compositing.

Key constraint: `NoteSkin` and `NoteSkinLoader` must exist before `NoteRenderer` can use them.

## Prerequisites

- **TD-CHT-001 / IP-CHT-001**: `NoteData`, `TimingData`, `NoteType` exist
- **Sprite system**: `Sprite`, `load_sprj`, `TextureCache` exist (already implemented)
- **NoteRenderer Phase 1**: `src/openitup/render/note_renderer.h` exists with placeholder rectangle rendering

Available for testing:
- `noteskin/default/` has 30 SPRJ files (TAP, LONG, JUDGE, PRESS for tracks 00-04)
- `noteskin/newextra/` has similar structure

---

## Step 1: NoteSkin Data Class with Sprite Accessors

**Files**:
- Create `src/openitup/render/noteskin.h` — NoteSkin class with fixed arrays and typed accessors
- Create `src/openitup/render/noteskin.cpp` — Accessor implementations, `is_complete()`, `loaded_count()`
- Modify `CMakeLists.txt` — Add `src/openitup/render/noteskin.cpp` to `openitup_engine` sources

**What to implement**:

`NoteSkin` class (from TD-REN-002 interface sketch):
- Public members: `directory` (path), `name` (string)
- Accessors (all return `const Sprite*`, nullable):
  - `tap(int track)` — TAP arrows (track 0-4)
  - `faketap(int track)` — FAKE arrows
  - `hold(int track, HoldPart part)` — LONG notes (HEAD, BODY, TAIL)
  - `other_w(int track)`, `other_g(int track)` — Division mode
  - `press(int track)` — Press overlay
  - `judge(int track)` — Judge overlay
  - `receptor(PlayMode mode)` — Receptor background (SINGLE, DOUBLE, HALF)
- Queries:
  - `bool is_complete() const` — returns true if all 48 expected sprites are loaded
  - `int loaded_count() const` — count of non-null sprites
  - `static constexpr int EXPECTED_COUNT = 48`

Private storage (friend `NoteSkinLoader` to populate):
- `std::array<std::unique_ptr<Sprite>, NUM_TRACKS>` for each arrow type (9 arrays)
- `std::array<std::unique_ptr<Sprite>, 3>` for receptors

Constants:
- `inline constexpr int NUM_TRACKS = 5`
- `enum class HoldPart : uint8_t { HEAD = 0, BODY = 1, TAIL = 2 }`
- `enum class PlayMode : uint8_t { SINGLE = 0, DOUBLE = 1, HALF = 2 }`

Implementation:
```cpp
const Sprite* NoteSkin::tap(int track) const {
    if (track < 0 || track >= NUM_TRACKS) return nullptr;
    return tap_[track].get();
}
// ... similar for other accessors

bool NoteSkin::is_complete() const {
    return loaded_count() == EXPECTED_COUNT;
}

int NoteSkin::loaded_count() const {
    int count = 0;
    for (const auto& arr : {tap_, faketap_, hold_head_, hold_body_, hold_tail_,
                             other_w_, other_g_, press_, judge_}) {
        for (const auto& ptr : arr) {
            if (ptr) ++count;
        }
    }
    for (const auto& ptr : receptor_) {
        if (ptr) ++count;
    }
    return count;
}
```

**Tests**:
- Create `test/test_noteskin.cpp` — Unit tests for NoteSkin accessors
- Modify `CMakeLists.txt` — Add `test/test_noteskin.cpp` to `openitup_tests`

Test cases (pure logic, no SDL):
- `DefaultConstructedReturnsNull` — default NoteSkin, `tap(0)` returns nullptr
- `TapOutOfBoundsReturnsNull` — `tap(-1)` and `tap(5)` return nullptr
- `HoldPartsAreIndependent` — head/body/tail use separate arrays
- `LoadedCountInitiallyZero` — default NoteSkin, `loaded_count() == 0`
- `IsCompleteInitiallyFalse` — default NoteSkin, `is_complete() == false`
- `ExpectedCountIs48` — `NoteSkin::EXPECTED_COUNT == 48`

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R NoteSkin` passes
- [ ] No SDL dependency in noteskin.h

**Expected commit message**:
`feat(render): add NoteSkin data class with typed sprite accessors`

**Estimated time**: ~30 minutes

---

## Step 2: NoteSkinLoader with File Discovery and SPRJ Loading

**Files**:
- Create `src/openitup/render/noteskin_loader.h` — NoteSkinLoader factory with `load()` and `load_with_fallback()`
- Create `src/openitup/render/noteskin_loader.cpp` — Implementation with deterministic filename construction
- Modify `CMakeLists.txt` — Add `src/openitup/render/noteskin_loader.cpp` to `openitup_engine` sources

**What to implement**:

`NoteSkinLoader` (from TD-REN-002 interface sketch):
- `static std::unique_ptr<NoteSkin> load(const std::filesystem::path& base_dir, const std::string& skin_name, TextureCache& cache)` — Throws if skin directory does not exist
- `static std::unique_ptr<NoteSkin> load_with_fallback(...)` — Tries skin_name, then "default", then throws
- `static std::unique_ptr<Sprite> try_load_sprj(const std::filesystem::path& skin_dir, const std::string& filename, TextureCache& cache)` — Returns nullptr with warning log if missing

Algorithm for `load()`:
1. Construct `skin_dir = base_dir / "noteskin" / skin_name`
2. Verify `skin_dir` exists and is a directory (throw if not)
3. Create `NoteSkin` instance, set `directory` and `name`
4. For each track 0-4:
   - `skin.tap_[t] = try_load_sprj(skin_dir, fmt::format("ARROW{:02d}_TAP.sprj", t), cache)`
   - Similar for FAKETAP, LONG_HEAD, LONG_BODY, LONG_TAIL, OTHER_W, OTHER_G, PRESS, JUDGE
5. Load receptors: `ARROW_RECEPTOR_SINGLE.sprj`, `ARROW_RECEPTOR_DOUBLE.sprj`, `ARROW_RECEPTOR_HALF.sprj`
6. Log: `spdlog::info("NoteSkin '{}' loaded: {}/{} sprites", name, loaded_count(), EXPECTED_COUNT)`
7. Return `unique_ptr<NoteSkin>`

`try_load_sprj()`:
```cpp
auto try_load_sprj(const fs::path& skin_dir, const string& filename, TextureCache& cache) {
    fs::path path = skin_dir / filename;
    if (!fs::exists(path)) {
        spdlog::warn("NoteSkin: missing {}", filename);
        return unique_ptr<Sprite>{};
    }
    return load_sprj(path, cache);
}
```

`load_with_fallback()`:
```cpp
try {
    return load(base_dir, skin_name, cache);
} catch (...) {
    if (skin_name == "default") throw;
    spdlog::warn("NoteSkin '{}' failed, falling back to 'default'", skin_name);
    return load(base_dir, "default", cache);
}
```

**Tests**:
- Modify `test/test_noteskin.cpp` — Add loader tests

Test cases (requires noteskin/default/ to exist):
- `LoadDefaultSkin` — load(".", "default", cache), verify `name == "default"`, `loaded_count() >= 30`
- `LoadedCountMatchesFilesOnDisk` — actual SPRJ count matches `loaded_count()`
- `TapSpritesPerTrack` — `tap(0)` through `tap(4)` return non-null after load
- `MissingSprjReturnsNull` — manually check a missing SPRJ slot (e.g., RECEPTOR) is null
- `LoadWithFallbackUsesDefault` — `load_with_fallback(".", "nonexistent", cache)` succeeds with default
- `LoadNonexistentDirectoryThrows` — `load("/tmp", "nosuchskin", cache)` throws

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R NoteSkin` passes
- [ ] Loader logs warnings for missing SPRJs (check with spdlog test sink)

**Expected commit message**:
`feat(render): add NoteSkinLoader with automatic file discovery and fallback`

**Estimated time**: ~40 minutes

---

## Step 3: Animation Timer Utilities (Header-Only)

**Files**:
- Create `src/openitup/render/noteskin_anim_timer.h` — Inline timing functions (no .cpp)

**What to implement**:

Header-only utilities (from TD-REN-002):
- Constants: `NOTESKIN_FRAME_DURATION_MS = 50.0`, `NOTESKIN_FRAME_COUNT = 6`, `NOTESKIN_LOOP_DURATION_MS = 300.0`
- `inline float noteskin_loop_t(double global_time_ms)` — Returns `[0.0, 1.0)` using `std::fmod`
- `inline float noteskin_oneshot_t(double global_time_ms, double trigger_time_ms)` — Returns `[0.0, ∞)` (caller clamps or checks expiry)
- `inline bool noteskin_oneshot_active(double global_time_ms, double trigger_time_ms)` — Returns `(elapsed < 300.0)`

Implementation:
```cpp
inline float noteskin_loop_t(double global_time_ms) {
    double phase = std::fmod(global_time_ms, NOTESKIN_LOOP_DURATION_MS);
    if (phase < 0.0) phase += NOTESKIN_LOOP_DURATION_MS;
    return static_cast<float>(phase / NOTESKIN_LOOP_DURATION_MS);
}

inline float noteskin_oneshot_t(double global_time_ms, double trigger_time_ms) {
    double elapsed = global_time_ms - trigger_time_ms;
    if (elapsed < 0.0) return 0.0f;
    return static_cast<float>(elapsed / NOTESKIN_LOOP_DURATION_MS);
}

inline bool noteskin_oneshot_active(double global_time_ms, double trigger_time_ms) {
    return (global_time_ms - trigger_time_ms) < NOTESKIN_LOOP_DURATION_MS;
}
```

**Tests**:
- Modify `test/test_noteskin.cpp` — Add animation timer tests

Test cases (pure math):
- `LoopTAtZero` — `noteskin_loop_t(0.0) == 0.0f`
- `LoopTAtHalfCycle` — `noteskin_loop_t(150.0) ≈ 0.5f`
- `LoopTWrapsAt300` — `noteskin_loop_t(300.0) == 0.0f`
- `LoopTWrapsAt600` — `noteskin_loop_t(600.0) == 0.0f`
- `LoopTHandlesNegative` — `noteskin_loop_t(-50.0)` in `[0.0, 1.0)`
- `OneshotTLinear` — `noteskin_oneshot_t(100.0, 0.0) ≈ 0.333f`
- `OneshotTReaches1` — `noteskin_oneshot_t(300.0, 0.0) == 1.0f`
- `OneshotTBeforeTriggerReturnsZero` — `noteskin_oneshot_t(50.0, 100.0) == 0.0f`
- `OneshotActiveWithin300` — `noteskin_oneshot_active(200.0, 0.0) == true`
- `OneshotInactiveAfter300` — `noteskin_oneshot_active(301.0, 0.0) == false`

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R NoteSkin` passes
- [ ] No .cpp file needed (header-only)

**Expected commit message**:
`feat(render): add noteskin animation timing utilities`

**Estimated time**: ~25 minutes

---

## Step 4: NoteRenderer Sprite Integration (Replace Rectangles)

**Files**:
- Modify `src/openitup/render/note_renderer.h` — Add `NoteSkin*` member, `TextureCache&` member, update signatures
- Modify `src/openitup/render/note_renderer.cpp` — Sprite rendering path with fallback to rectangles

**What to implement**:

Changes to `NoteRenderer`:
- Constructor gains: `const NoteSkin* skin` (nullable, non-owning), `TextureCache& cache`
- `render()` signature becomes: `void render(SDL_Renderer* renderer, double song_position_ms, double global_time_ms) const`
- `render_receptors()` stub updated to accept `global_time_ms` (full implementation in Step 5)
- `NoteFieldConfig` gains: `float note_sprite_size = 64.0f`

Private members:
- `const NoteSkin* skin_` (nullable)
- `TextureCache& cache_`

Rendering logic in `render()`:
```cpp
// For each visible note:
int track = note.column % NUM_TRACKS;
const Sprite* sprite = nullptr;

switch (note.type) {
    case NoteType::TAP:       sprite = skin_ ? skin_->tap(track) : nullptr; break;
    case NoteType::FAKE:      sprite = skin_ ? skin_->faketap(track) : nullptr; break;
    case NoteType::HOLD_HEAD: sprite = skin_ ? skin_->hold(track, HoldPart::HEAD) : nullptr; break;
    // HOLD_TAIL, MINE deferred to future phases
    default: break;
}

if (sprite) {
    float t = noteskin_loop_t(global_time_ms);
    LayerTransform xform{};
    xform.translate_x = column_x[note.column] - (config_.note_sprite_size / 2.0f);
    xform.translate_y = y - (config_.note_sprite_size / 2.0f);
    sprite->draw(renderer, cache_, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND);
} else {
    // Phase 1 fallback: colored rectangle (existing code)
}
```

**Tests**:
- Modify `test/test_noteskin.cpp` — Add integration test with mock NoteData

Test cases (requires SDL or mock renderer):
- `NullSkinFallsBackToRectangles` — NoteRenderer(data, timing, config, nullptr, cache), verify no crash on render
- `SpriteSelectedByNoteType` — Verify TAP -> tap(), FAKE -> faketap(), HOLD_HEAD -> hold(HEAD)
- `ColumnToTrackMapping` — Column 5 in double mode maps to track 0 (5 % 5 == 0)
- `SpriteCenteredOnColumn` — xform.translate_x == column_x[col] - 32.0

For these tests, use a manually-constructed `NoteSkin` with test Sprites or mock the draw call.

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R NoteSkin` passes
- [ ] Existing Phase 1 tests still pass (pass `nullptr` for skin)

**Expected commit message**:
`feat(render): integrate NoteSkin sprites into NoteRenderer with rectangle fallback`

**Estimated time**: ~35 minutes

---

## Step 5: Receptor 3-Layer Compositing (Press + Judge Overlays)

**Files**:
- Modify `src/openitup/render/note_renderer.h` — Update `render_receptors()` signature
- Modify `src/openitup/render/note_renderer.cpp` — Implement 3-layer receptor rendering

**What to implement**:

New `render_receptors()` signature:
```cpp
void render_receptors(SDL_Renderer* renderer,
                      double global_time_ms,
                      const bool* pressed_columns,
                      const double* judge_trigger_times) const;
```

Where:
- `pressed_columns` — bool array of size `num_columns`, true if panel pressed
- `judge_trigger_times` — double array of size `num_columns`, timestamp of last judge animation trigger (negative or 0 = inactive)

Implementation:
```cpp
for (int col = 0; col < config_.num_columns; ++col) {
    int track = col % NUM_TRACKS;
    float x = config_.column_x[col] - (config_.note_sprite_size / 2.0f);
    float y = config_.receptor_y - (config_.note_sprite_size / 2.0f);

    LayerTransform xform{};
    xform.translate_x = x;
    xform.translate_y = y;

    // Layer 1: Receptor background (deferred — use placeholder rectangle for now)
    // const Sprite* receptor_sprite = skin_ ? skin_->receptor(PlayMode::SINGLE) : nullptr;
    // if (receptor_sprite) { ... }
    // else { draw placeholder rectangle }

    // Layer 2: Press overlay (loops while pressed)
    if (pressed_columns && pressed_columns[col] && skin_) {
        const Sprite* press_sprite = skin_->press(track);
        if (press_sprite) {
            float t = noteskin_loop_t(global_time_ms);
            press_sprite->draw(renderer, cache_, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND);
        }
    }

    // Layer 3: Judge overlay (one-shot, 300ms)
    if (judge_trigger_times && noteskin_oneshot_active(global_time_ms, judge_trigger_times[col]) && skin_) {
        const Sprite* judge_sprite = skin_->judge(track);
        if (judge_sprite) {
            float t = noteskin_oneshot_t(global_time_ms, judge_trigger_times[col]);
            judge_sprite->draw(renderer, cache_, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND);
        }
    }
}
```

**Tests**:
- Modify `test/test_noteskin.cpp` — Add receptor compositing tests

Test cases:
- `PressOverlayRendersWhenPressed` — pressed_columns[0] = true, verify press(0) sprite would be used
- `PressOverlayHiddenWhenNotPressed` — pressed_columns[0] = false, verify no press sprite draw
- `JudgeOverlayActiveFor300ms` — trigger_times[0] = 0, global_time = 100, verify oneshot active
- `JudgeOverlayInactiveAfter300ms` — trigger_times[0] = 0, global_time = 301, verify oneshot inactive
- `JudgeOverlayHiddenBeforeTrigger` — trigger_times[0] = 100, global_time = 50, verify t == 0

These can be pure logic tests checking the oneshot_active/oneshot_t functions, or mock renderer tests.

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R NoteSkin` passes
- [ ] Press and judge overlays render on top of receptor background

**Expected commit message**:
`feat(render): add 3-layer receptor compositing with press and judge overlays`

**Estimated time**: ~35 minutes

---

## Step 6: Integration Verification with Real Noteskins

**Files**:
- Modify `test/test_noteskin.cpp` — Add end-to-end integration tests

**What to implement**:

Full pipeline tests using actual noteskin/default/ files:

Test cases:
- `LoadAndRenderDefaultNoteskin` — Load default skin, render TAP notes, verify sprites used (non-null)
- `AnimationFrameChanges` — Render at t=0 and t=50ms, verify different frames selected (via ani_frame())
- `DoubleModeTapReusesTrack` — Column 5 note uses ARROW00_TAP.sprj (same as column 0)
- `MissingSprjFallsBackToRectangle` — Request a missing sprite (e.g., MINE), verify rectangle fallback
- `CompositingLayerOrder` — Verify press overlay renders, then judge overlay renders on top
- `NoteSkinCompletenessSummary` — Load default, log: "NoteSkin 'default' loaded: 30/48 sprites"

For pixel-level verification (if needed), render to an offscreen texture and check a single pixel is non-black where a sprite should appear.

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R NoteSkin` passes all tests
- [ ] Manual verification: Visual inspection of sprites in gameplay (requires GameplayScene from IP-SCN-001)

**Expected commit message**:
`test(render): add integration tests for noteskin loading and sprite rendering`

**Estimated time**: ~35 minutes

---

## PR Strategy

- [ ] **Single PR** recommended — All 6 steps are tightly coupled (NoteSkin data model → loader → animation → rendering). Splitting would create incomplete intermediate states.
- [ ] **Review checkpoints**: After Step 3 (foundation complete), after Step 5 (rendering complete)

## Build Verification

After all steps complete:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DSDL_X11_XSCRNSAVER=OFF
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure -R NoteSkin
```

Expected output:
- All NoteSkin tests pass
- Existing NoteRenderer Phase 1 tests still pass (backward compatibility via nullable skin pointer)

## Acceptance Verification

| Story ID | How to verify |
|----------|--------------|
| US-REN-021 | `LoadDefaultSkin`, `TapSpritesPerTrack`, `LoadWithFallbackUsesDefault`, `AnimationFrameChanges`, `DoubleModeTapReusesTrack` |
| US-REN-022 | `PressOverlayRendersWhenPressed`, `CompositingLayerOrder`, `LoadAndRenderDefaultNoteskin` |
| US-REN-023 | `JudgeOverlayActiveFor300ms`, `JudgeOverlayInactiveAfter300ms`, `CompositingLayerOrder` |

---

## Summary

| Step | What | Key Files | Stories | Est. |
|------|------|-----------|---------|------|
| 1 | NoteSkin data class | noteskin.h/cpp | US-REN-021 | 30m |
| 2 | NoteSkinLoader factory | noteskin_loader.h/cpp | US-REN-021 | 40m |
| 3 | Animation timer utils | noteskin_anim_timer.h | US-REN-021 | 25m |
| 4 | NoteRenderer sprite integration | note_renderer.h/cpp | US-REN-021 | 35m |
| 5 | Receptor 3-layer compositing | note_renderer.cpp | US-REN-022, US-REN-023 | 35m |
| 6 | Integration verification | test_noteskin.cpp | All three | 35m |

**Total new source files**: 5 (3 headers + 2 .cpp)
**Total new test files**: 1 (`test/test_noteskin.cpp`)
**CMakeLists.txt modifications**: Add 2 .cpp to `openitup_engine`, add 1 test to `openitup_tests`

Each step compiles, passes tests, and is independently committable. Steps 1-3 build the foundation (data model + loading + timing). Steps 4-5 integrate sprites into rendering. Step 6 verifies the full pipeline with real noteskin assets.
