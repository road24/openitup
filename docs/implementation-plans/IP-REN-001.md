# IP-REN-001: Note Renderer Phase 1 Implementation Plan

**Design**: TD-REN-001
**Stories**: US-REN-019, US-REN-020, US-REN-036
**Total Steps**: 4
**Estimated Total**: ~2.5 hours
**Author**: technical-lead agent
**Status**: Draft

## Prerequisites

- **TD-CHT-001 / IP-CHT-001**: `NoteData`, `NoteEvent`, `NoteType`, `TimingData` must exist in `src/openitup/chart/`. The NoteRenderer reads these types.
- **TD-JDG-001 / IP-JDG-001**: `JudgmentTier`, `judgment_tier_to_string()` must exist in `src/openitup/judge/`. The JudgmentDisplay uses JudgmentTier.
- **Renderer** (`src/openitup/gfx/renderer.h`): Must exist (already implemented).

All rendering tests for `beat_to_y()` and config are pure math — no SDL required. Only the actual `render()` calls need SDL, and those are tested via integration tests.

---

## Step 1: Create NoteFieldConfig and beat_to_y() Conversion

**Files**:
- Create `src/openitup/render/note_renderer.h` — NoteFieldConfig struct, NoteRenderer class declaration
- Create `src/openitup/render/note_renderer.cpp` — `beat_to_y()`, `default_single_config()`, column color definitions
- Modify `CMakeLists.txt` — Add `src/openitup/render/note_renderer.cpp` to `openitup_engine` library sources

**What to implement**:

The core beat-to-screen conversion math and note field layout configuration.

`NoteFieldConfig` struct with defaults:
- `receptor_y = 400.0f`, `note_width = 48.0f`, `note_height = 48.0f`
- `pixels_per_beat = 80.0f`, `scroll_speed = 1.0f`, `num_columns = 5`
- `column_x` vector (computed by `default_single_config()`)

`beat_to_y()` algorithm:
```cpp
float NoteRenderer::beat_to_y(double note_beat, double current_beat) const {
    double beat_delta = note_beat - current_beat;
    return config_.receptor_y
         - static_cast<float>(beat_delta) * config_.pixels_per_beat * config_.scroll_speed;
}
```

`default_single_config()`:
- 5 columns centered in 640px: column_x = {208, 264, 320, 376, 432} with 56px spacing

Column colors (10 entries, 5 unique for single + 5 repeated for double):
- Column 0: Red (255, 50, 50)
- Column 1: Blue (50, 100, 255)
- Column 2: Yellow (255, 255, 50)
- Column 3: Green (50, 255, 50)
- Column 4: Magenta (255, 50, 255)

Stub `render()` and `render_receptors()` — empty bodies, implemented in Step 2.

**Tests**:
- Create `test/test_note_renderer.cpp` — Unit tests for beat_to_y and config
- Modify `CMakeLists.txt` — Add `test/test_note_renderer.cpp` to `openitup_tests`

Test cases (pure math, no SDL):
- `BeatToYAtReceptor` — note_beat == current_beat -> y == 400.0
- `BeatToYFourBeatsAbove` — note at current+4.0, pxPerBeat=80 -> y = 400 - 320 = 80
- `BeatToYOneBeatsBelow` — note at current-1.0 -> y = 400 + 80 = 480
- `BeatToYScrollSpeedDoubled` — scroll_speed=2.0 -> distance doubles (y = 400 - 640 = -240)
- `BeatToYScrollSpeedHalf` — scroll_speed=0.5 -> distance halves
- `BeatToYZeroBeatDelta` — beat_delta=0 -> y = receptor_y exactly
- `DefaultSingleConfigFiveColumns` — column_x has 5 entries
- `DefaultSingleConfigCentered` — column_x[2] == 320.0
- `DefaultSingleConfigEqualSpacing` — all adjacent pairs differ by 56px
- `DefaultSingleConfigNoteSize` — note_width==48, note_height==48
- `ColumnColorsDistinct` — All 5 colors are distinct (no two are equal)

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R NoteRenderer` passes all tests
- [ ] No SDL calls in tests

**Expected commit message**:
`feat(render): add NoteRenderer with beat-to-screen conversion and column layout`

**Estimated time**: ~40 minutes

---

## Step 2: Implement Note Rendering (Colored Rectangles)

**Files**:
- Modify `src/openitup/render/note_renderer.cpp` — Implement `render()` and `render_receptors()`

**What to implement**:

`render(SDL_Renderer* renderer, double song_position_ms)`:
1. Convert song_position_ms to current_beat via `timing_data_.beat_at_time(song_position_ms / 1000.0)`
2. Compute visible beat range (top of screen to bottom of screen)
3. Get notes in range via `note_data_.notes_in_range(bottom_beat, top_beat)`
4. For each visible note:
   - Skip if `note.type` is not TAP or HOLD_HEAD
   - Skip if `note.column >= num_columns`
   - Compute y = `beat_to_y(note.beat, current_beat)`
   - Skip if y is offscreen (y < -note_height or y > 480 + note_height)
   - Compute x = `column_x[note.column] - note_width/2`
   - Set draw color from `COLUMN_COLORS[note.column]`
   - Draw filled rectangle at (x, y - note_height/2, note_width, note_height)

`render_receptors(SDL_Renderer* renderer)`:
1. For each column 0..num_columns-1:
   - Set draw color to dim gray (80, 80, 80, 180)
   - Draw outlined rectangle at receptor_y position for column
   - `SDL_RenderRect` (outline only, not filled) to distinguish from notes

**Tests**:

No new unit tests — rendering requires SDL. Integration testing happens in Step 4 of IP-SCN-001 when the full scene is running.

Manual verification:
```bash
# After IP-SCN-001 is complete:
./build/openitup --data-dir /path/to/song/
# Verify: colored rectangles scroll, receptors visible as outlines at y=400
```

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] All existing tests still pass
- [ ] Code review: render loop uses notes_in_range for efficiency

**Expected commit message**:
`feat(render): implement placeholder rectangle rendering for notes and receptors`

**Estimated time**: ~40 minutes

---

## Step 3: Create JudgmentDisplay with Tier Colors

**Files**:
- Create `src/openitup/render/judgment_display.h` — JudgmentDisplay class declaration
- Create `src/openitup/render/judgment_display.cpp` — Implementation (on_judgment, render, tier colors)
- Modify `CMakeLists.txt` — Add `src/openitup/render/judgment_display.cpp` to `openitup_engine`

**What to implement**:

JudgmentDisplay class with:
- `on_judgment(JudgmentTier tier)`: store tier, reset timer to 0.0
- `render(SDL_Renderer* renderer, double dt)`: accumulate dt in timer, if timer < 0.5s draw colored rectangle at (260, 200, 120, 40) with tier color
- `current_tier()`: returns stored tier
- `is_visible()`: returns `time_since_judgment_ < DISPLAY_DURATION`

Tier colors (static array):
- PERFECT: green (0, 255, 0)
- GREAT: cyan (0, 200, 255)
- GOOD: yellow (255, 255, 0)
- BAD: orange (255, 128, 0)
- MISS: red (255, 0, 0)

If `SDL_RenderDebugText` is available in the linked SDL3 version, add the tier name as text inside the rectangle. This is a bonus, not a requirement — guard with `#if SDL_VERSION_ATLEAST(...)` or a function pointer check.

**Tests**:
- Modify `test/test_note_renderer.cpp` — Add JudgmentDisplay tests

Test cases (pure logic, no SDL for the state tests):
- `InitiallyInvisible` — Default-constructed display, is_visible() == false
- `OnJudgmentMakesVisible` — on_judgment(PERFECT), is_visible() == true
- `CurrentTierUpdates` — on_judgment(GREAT), current_tier() == GREAT
- `FadesAfterDuration` — Simulate 31 calls to render() with dt=1/60 (0.516s total) -> is_visible() == false
- `StaysVisibleBeforeDuration` — Simulate 29 calls (0.483s) -> is_visible() == true
- `NewJudgmentResetsTimer` — on_judgment, advance 0.3s, on_judgment again -> is_visible() == true
- `TierColorsDistinct` — All 5 tier colors are different

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R NoteRenderer` passes all tests including JudgmentDisplay
- [ ] JudgmentDisplay has no NoteRenderer dependency

**Expected commit message**:
`feat(render): add JudgmentDisplay with tier-colored feedback rectangles`

**Estimated time**: ~30 minutes

---

## Step 4: Integration Verification and Polish

**Files**:
- Modify `test/test_note_renderer.cpp` — Add integration-style tests combining NoteRenderer with chart data

**What to implement**:

End-to-end tests that construct real NoteData and TimingData and verify the renderer's behavior:

Test cases:
- `VisibleRangeAt120BPM` — At 120 BPM, current_beat=10: notes at beats 5-15 should have y values within [0, 480]
- `NoteBeyondScreenIgnored` — Note at beat 100 while current_beat=0: y is far above screen (< -48)
- `BpmChangeDoesNotJumpNotes` — Two notes at beat 7.9 and 8.1 (BPM change at 8.0): y positions are close together (no visual gap)
- `StopFreezesNotes` — During a stop: same song_ms gives same beat_to_y values
- `FourConsecutiveQuarterNotes` — 4 notes one beat apart: y spacing is exactly pixels_per_beat (80px)
- `JudgmentDisplayIntegration` — on_judgment then check state, simulate 30 frames, check fade

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R NoteRenderer` passes all tests
- [ ] All tests are pure logic (no SDL initialization)

**Expected commit message**:
`test(render): add integration tests for NoteRenderer with chart fixtures`

**Estimated time**: ~30 minutes

---

## Summary

| Step | What | Files Created/Modified | Stories Covered | Est. |
|------|------|----------------------|-----------------|------|
| 1 | NoteFieldConfig + beat_to_y | 2 new + 1 modified | US-REN-019 | 40m |
| 2 | Placeholder rectangle rendering | 1 modified | US-REN-020 | 40m |
| 3 | JudgmentDisplay | 2 new + 1 modified | US-REN-036 | 30m |
| 4 | Integration tests + polish | 1 modified | All three stories | 30m |

**Total new source files**: 4 (2 headers + 2 .cpp in `src/openitup/render/`)
**Total new test files**: 1 (`test/test_note_renderer.cpp`)
**CMakeLists.txt modifications**: Add 2 .cpp to `openitup_engine`, add 1 test file to `openitup_tests`

Each step is independently committable. Steps 1 and 3 are testable without SDL. Step 2 is render code verified visually via the full gameplay scene (IP-SCN-001). Step 4 ties the renderer to chart data for correctness verification.

## Acceptance Verification

| Story ID | How to verify |
|----------|--------------|
| US-REN-019 | `test/test_note_renderer.cpp`: `BeatToYAtReceptor`, `BeatToYFourBeatsAbove`, `BeatToYScrollSpeedDoubled`, `StopFreezesNotes`, `BpmChangeDoesNotJumpNotes` |
| US-REN-020 | Visual: colored rectangles visible per column, 48x48 size, proportional spacing. `ColumnColorsDistinct`, `DefaultSingleConfigFiveColumns` |
| US-REN-036 | `test/test_note_renderer.cpp`: `OnJudgmentMakesVisible`, `FadesAfterDuration`, `TierColorsDistinct` |
