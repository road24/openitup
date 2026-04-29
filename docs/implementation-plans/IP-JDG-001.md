# IP-JDG-001: Gameplay Judge Phase 1 Implementation Plan

**Design**: TD-JDG-001
**Stories**: US-JDG-001, US-JDG-002, US-JDG-003, US-JDG-004, US-JDG-005, US-JDG-006, US-JDG-011, US-JDG-012, US-JDG-019
**Total Steps**: 7
**Estimated Total**: ~5.5 hours
**Author**: technical-lead agent
**Status**: Draft

## Prerequisites

- **TD-CHT-001 / IP-CHT-001**: `NoteData`, `NoteEvent`, `NoteType`, `TimingData`, and `TimingEvent` must exist in `src/openitup/chart/`. The judge reads these types at runtime. If they are not yet implemented, implement IP-CHT-001 steps 1-4 (data model and TimingData) first.
- **TD-INP-001**: Not a compile-time dependency (the judge accepts a raw `uint32_t` bitmask). However, understanding that `InputSnapshot::pressed_mask()` produces the bitmask informs the integration in GameplayScene (outside this plan).
- **TD-AUD-001**: Not a compile-time dependency (the judge accepts `double song_position_ms`). Same integration-only dependency.

The entire judge subsystem compiles and tests with zero SDL headers. Every step in this plan produces a committable unit with passing tests.

---

## Step 1: Create JudgmentTier Enum and String Conversions

**Files**:
- Create `src/openitup/judge/judgment_tier.h` — JudgmentTier enum, JUDGMENT_TIER_COUNT, tier_maintains_combo(), string conversion declarations
- Create `src/openitup/judge/judgment_tier.cpp` — judgment_tier_to_string/from_string implementations
- Modify `CMakeLists.txt` — Add `src/openitup/judge/judgment_tier.cpp` to `openitup_engine` library sources

**What to implement**:

The JudgmentTier enum with five values (PERFECT=0 through MISS=4), matching the `BlendEffect` pattern in `src/openitup/bga/keyframe.h`. The integer values are stable array indices used later by GameplayState.

Key implementation details from TD-JDG-001:
- `enum class JudgmentTier : uint8_t` with values 0-4
- `JUDGMENT_TIER_COUNT = 5` (inline constexpr)
- `tier_maintains_combo()` — inline constexpr, returns true for PERFECT/GREAT/GOOD
- `judgment_tier_to_string()` — switch statement returning string literals
- `judgment_tier_from_string()` — if-chain matching strings to enum values

**Tests**:
- Create `test/test_judge.cpp` — Initial test file for judge unit tests
- Modify `CMakeLists.txt` — Add `test/test_judge.cpp` to `openitup_tests` sources

Test cases:
- `AllTiersExist` — PERFECT, GREAT, GOOD, BAD, MISS compile and are distinct values
- `TierIntegerValues` — PERFECT==0, GREAT==1, GOOD==2, BAD==3, MISS==4
- `TierCount` — JUDGMENT_TIER_COUNT == 5
- `TierMaintainsCombo_Perfect` — tier_maintains_combo(PERFECT) == true
- `TierMaintainsCombo_Great` — tier_maintains_combo(GREAT) == true
- `TierMaintainsCombo_Good` — tier_maintains_combo(GOOD) == true
- `TierBreaksCombo_Bad` — tier_maintains_combo(BAD) == false
- `TierBreaksCombo_Miss` — tier_maintains_combo(MISS) == false
- `StringRoundTrip` — for each tier: from_string(to_string(tier)) == tier

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Judge` passes all JudgmentTier tests
- [ ] No SDL headers in judgment_tier.h or judgment_tier.cpp

**Expected commit message**:
`feat(judge): add JudgmentTier enum with five-tier classification and string conversions`

**Estimated time**: ~20 minutes

---

## Step 2: Create TimingProfile with Hardcoded Default

**Files**:
- Create `src/openitup/judge/timing_profile.h` — TimingProfile struct, is_valid() declaration, default_timing_profile() declaration
- Create `src/openitup/judge/timing_profile.cpp` — is_valid() and default_timing_profile() implementations
- Modify `CMakeLists.txt` — Add `src/openitup/judge/timing_profile.cpp` to `openitup_engine` library sources

**What to implement**:

TimingProfile is a plain struct with four `double` fields: `perfect_window_ms`, `great_window_ms`, `good_window_ms`, `bad_window_ms`.

Key implementation details from TD-JDG-001:
- `is_valid()`: returns true iff all windows are > 0 and `perfect <= great <= good <= bad`
- `default_timing_profile()`: returns `{16.0, 33.0, 66.0, 100.0}` (Exceed-era values from US-JDG-019)

**Tests**:
- Modify `test/test_judge.cpp` — Add TimingProfile tests

Test cases:
- `DefaultProfileValues` — default_timing_profile() returns {16.0, 33.0, 66.0, 100.0}
- `DefaultProfileIsValid` — is_valid() returns true for default profile
- `InvalidProfileNegativeWindow` — {-16.0, 33.0, 66.0, 100.0} → is_valid() false
- `InvalidProfileUnordered` — {50.0, 33.0, 66.0, 100.0} (perfect > great) → is_valid() false
- `InvalidProfileZeroWindow` — {0.0, 33.0, 66.0, 100.0} → is_valid() false
- `ValidProfileAllEqual` — {16.0, 16.0, 16.0, 16.0} → is_valid() true
- `ValidProfileMinimal` — {1.0, 2.0, 3.0, 4.0} → is_valid() true
- `HardcodedValuesMatchExceedEra` — verify values match documented Exceed timing

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Judge` passes all TimingProfile tests
- [ ] No SDL headers in timing_profile.h or timing_profile.cpp

**Expected commit message**:
`feat(judge): add TimingProfile with Exceed-era hardcoded default (±16/33/66/100ms)`

**Estimated time**: ~20 minutes

---

## Step 3: Create JudgmentEvent Immutable Value Type

**Files**:
- Create `src/openitup/judge/judgment_event.h` — JudgmentEvent class declaration
- Create `src/openitup/judge/judgment_event.cpp` — Constructor, accessors, operator< implementations
- Modify `CMakeLists.txt` — Add `src/openitup/judge/judgment_event.cpp` to `openitup_engine` library sources

**What to implement**:

JudgmentEvent follows the InputSnapshot pattern from TD-INP-001: private fields, const-returning accessors, no setters.

Constructor takes all six fields:
```cpp
JudgmentEvent(std::size_t note_index, uint8_t column, double beat,
              JudgmentTier tier, double timing_error_ms, bool is_auto_miss);
```

Accessors: `note_index()`, `column()`, `beat()`, `tier()`, `timing_error_ms()`, `is_auto_miss()`.

`operator<`: sort by beat, then by column for stable ordering.

Default constructor: not provided (a JudgmentEvent must have all fields).

**Tests**:
- Modify `test/test_judge.cpp` — Add JudgmentEvent tests

Test cases:
- `FieldsAccessible` — Construct event, verify all accessors return correct values
- `NegativeErrorIsEarly` — timing_error_ms() returns negative for early hit
- `PositiveErrorIsLate` — timing_error_ms() returns positive for late hit
- `ZeroErrorIsExact` — timing_error_ms() returns 0.0 for exact hit
- `SortByBeat` — Events at beats 5.0, 3.0, 4.0 sort to 3.0, 4.0, 5.0
- `SortStableByColumn` — Events at same beat sort by column
- `AutoMissFlag` — is_auto_miss() returns true/false as constructed
- `CopyAndMove` — Verify copy and move work (no const members breaking them)

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Judge` passes all JudgmentEvent tests
- [ ] No SDL headers in judgment_event.h or judgment_event.cpp

**Expected commit message**:
`feat(judge): add immutable JudgmentEvent value type with beat-ordered sorting`

**Estimated time**: ~25 minutes

---

## Step 4: Create Judge Core — Classification and Single-Note Evaluation

**Files**:
- Create `src/openitup/judge/judge.h` — Judge class declaration
- Create `src/openitup/judge/judge.cpp` — Judge implementation: constructor, classify(), find_closest_unjudged(), update() with input matching only (auto-miss in next step)
- Modify `CMakeLists.txt` — Add `src/openitup/judge/judge.cpp` to `openitup_engine` library sources

**What to implement**:

The Judge class with constructor (accepts NoteData&, TimingData&, TimingProfile), internal state (judged_ vector, cursor_, counts), and the core classification algorithm.

Phase 4a — implement in this step:
1. Constructor: store references, copy profile, build judged_ vector, precompute total_judgable_ (count TAP notes), init cursor_ = 0
2. `classify(double abs_error_ms)` — the 4-comparison chain returning JudgmentTier
3. `find_closest_unjudged(column, song_position_ms)` — scan from cursor, find closest unjudged TAP on the given column within the bad window
4. `update(song_position_ms, pressed_columns)` — input matching phase only. For each pressed column, find closest unjudged note, classify, emit event. Sort events by beat. Auto-miss scanning deferred to Step 5.
5. `judged_count()`, `total_judgable()`, `is_complete()`, `profile()`, `reset()`

Key implementation notes:
- `pressed_columns` is a `uint32_t` bitmask. Iterate columns 0-9, check bit.
- Convert note beats to ms via `timing_data_.time_at_beat(note.beat) * 1000.0`
- When classify returns MISS for an input match, do NOT emit — the note may be hit closer. See TD-JDG-001 "Critical subtlety" in update() algorithm.
- `total_judgable_` counts only NoteType::TAP. Log warning if HOLD_HEAD encountered.

**Tests**:
- Modify `test/test_judge.cpp` — Add Judge classification and update tests

To build test fixtures inline, create helper functions:
```cpp
// Helper: build a simple NoteData with N tap notes at given beats/columns
NoteData make_notes(std::vector<std::pair<double, uint8_t>> beat_col_pairs);
// Helper: build a simple 120 BPM TimingData
TimingData make_simple_timing(double bpm = 120.0);
```

Test cases — Classification:
- `ClassifyExactHit` — abs_error 0.0 → PERFECT
- `ClassifyPerfectBoundary` — abs_error 16.0 → PERFECT
- `ClassifyGreatJustOutsidePerfect` — abs_error 16.1 → GREAT
- `ClassifyGreatBoundary` — abs_error 33.0 → GREAT
- `ClassifyGoodBoundary` — abs_error 66.0 → GOOD
- `ClassifyBadBoundary` — abs_error 100.0 → BAD
- `ClassifyMissBeyondBad` — abs_error 100.1 → MISS

Test cases — Single note update:
- `SingleNotePerfectHit` — Note at beat 4.0 (1000ms at 240BPM), input at 1001ms, col match → PERFECT, error -1ms
- `SingleNoteGreatHit` — Input 25ms late → GREAT, error +25ms
- `SingleNoteMissBeyondWindow` — Input 150ms late → no event emitted (note not yet auto-missed)
- `WrongColumnNoMatch` — Input on column 0, note on column 2 → no event
- `MultipleColumnsOneTick` — 3 notes on cols 0/1/2, all 3 pressed → 3 events
- `ClosestNoteWinsOnSameColumn` — Two notes on col 0 at 1000ms and 1050ms, input at 1020ms → matches 1000ms note (closer)
- `SameInputsSameOutputs` — Identical update calls produce identical results (determinism)
- `NoInputsNoEvents` — pressed_columns=0 → empty event vector

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Judge` passes all classification and update tests
- [ ] No SDL headers anywhere in judge source files
- [ ] `grep -r "SDL_\|rand\|random\|RNG" src/openitup/judge/` returns zero matches

**Expected commit message**:
`feat(judge): add Judge core with timing classification and input-note matching`

**Estimated time**: ~1.5 hours

---

## Step 5: Add Auto-Miss Detection and Flush

**Files**:
- Modify `src/openitup/judge/judge.cpp` — Add auto-miss scanning to update(), implement flush_remaining()

**What to implement**:

Extend `update()` with the auto-miss phase that runs before input matching:

```
Auto-miss scan (from TD-JDG-001):
  while cursor_ < events_.size():
    if judged_[cursor_]: cursor_++; continue
    if note.type != TAP: cursor_++; continue
    note_time_ms = timing_data_.time_at_beat(note.beat) * 1000.0
    if song_position_ms - note_time_ms <= bad_window_ms: break
    // Note is past late boundary → auto-miss
    emit MISS event with is_auto_miss=true
    mark judged, increment cursor
```

Implement `flush_remaining()`:
```
for each unjudged note:
  emit MISS event with is_auto_miss=true
  mark judged
return all events sorted by beat
```

**Tests**:
- Modify `test/test_judge.cpp` — Add auto-miss and flush tests

Test cases:
- `AutoMissWhenNoInput` — Note at 1000ms, advance to 1200ms with no input → MISS event emitted
- `AutoMissHasCorrectError` — Auto-miss error equals +bad_window_ms (100ms)
- `AutoMissIsAutoMissFlag` — event.is_auto_miss() == true for auto-miss
- `MissedNoteDoesntBlockFuture` — Note A missed, note B hit normally → both produce events
- `FlushRemainingAllMiss` — 5 unjudged notes → flush produces 5 MISS events
- `FlushAfterPartialPlay` — 10 notes, 7 hit, flush → 3 MISS events → total 10
- `AllNotesJudgedAtSongEnd` — Play through some notes, flush rest → is_complete() true, judged_count == total_judgable
- `NoAutoMissBeforeWindow` — Note at 1000ms, song at 1050ms (within 100ms window) → no auto-miss yet
- `AutoMissEventInBeatOrder` — Multiple auto-misses emitted in beat order
- `CursorAdvancesPastJudgedNotes` — Already-judged notes don't re-emit
- `FixedStepCountDoesntAffectResult` — Call update() at 60Hz vs call at 30Hz with same final positions → same total judgments

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Judge` passes all tests including auto-miss
- [ ] Verify: 100-note chart with 90 hits + flush → exactly 100 JudgmentEvents total

**Expected commit message**:
`feat(judge): add auto-miss detection and flush_remaining for end-of-song completion`

**Estimated time**: ~1 hour

---

## Step 6: Create GameplayState — Combo and Score Tracking

**Files**:
- Create `src/openitup/judge/gameplay_state.h` — GameplayState class declaration
- Create `src/openitup/judge/gameplay_state.cpp` — GameplayState implementation (apply, combo, score, counts, reset)
- Modify `CMakeLists.txt` — Add `src/openitup/judge/gameplay_state.cpp` to `openitup_engine` library sources

**What to implement**:

GameplayState is completely independent of the Judge class. It receives JudgmentEvent vectors via `apply()`.

Key implementation:
```cpp
void GameplayState::apply_single(const JudgmentEvent& event) {
    JudgmentTier tier = event.tier();
    judgment_counts_[static_cast<int>(tier)]++;
    score_ += POINTS_PER_TIER[static_cast<int>(tier)];

    if (tier_maintains_combo(tier)) {
        current_combo_++;
        if (current_combo_ > max_combo_) {
            max_combo_ = current_combo_;
        }
    } else {
        current_combo_ = 0;
    }
}

void GameplayState::apply(const std::vector<JudgmentEvent>& events) {
    for (const auto& event : events) {
        apply_single(event);
    }
}
```

`score_percentage()`:
```cpp
double GameplayState::score_percentage() const {
    if (total_notes_ == 0) return 0.0;
    int64_t max_score = static_cast<int64_t>(total_notes_) * PERFECT_POINTS;
    if (max_score == 0) return 0.0;
    return (static_cast<double>(score_) / static_cast<double>(max_score)) * 100.0;
}
```

`reset()`: zero all counters, set combo to 0, keep total_notes_.

**Tests**:
- Create `test/test_gameplay_state.cpp` — Dedicated test file for GameplayState
- Modify `CMakeLists.txt` — Add `test/test_gameplay_state.cpp` to `openitup_tests` sources

Helper: create JudgmentEvent objects directly (no Judge needed):
```cpp
JudgmentEvent make_event(JudgmentTier tier, uint8_t column = 0, double beat = 0.0);
```

Test cases:
- `InitialStateZero` — combo=0, max_combo=0, score=0, all counts=0
- `ComboIncrementsOnPerfect` — apply PERFECT → combo 0→1
- `ComboIncrementsOnGreat` — apply GREAT → combo 1→2
- `ComboIncrementsOnGood` — apply GOOD → combo 2→3
- `ComboResetsOnBad` — combo 20, apply BAD → combo=0, max_combo=20
- `ComboResetsOnMiss` — combo 10, apply MISS → combo=0, max_combo=10
- `MaxComboTracked` — sequence 10 perfects, miss, 15 perfects, miss → max_combo=15
- `ComboAccessible` — current_combo() returns 42 after 42 perfects
- `ScoreIncrementsPerfect` — 1 PERFECT → score=1000
- `ScoreIncrementsGreat` — 1 GREAT → score=800
- `ScoreIncrementsGood` — 1 GOOD → score=500
- `ScoreIncrementsBad` — 1 BAD → score=100
- `ScoreIncrementsMiss` — 1 MISS → score=0
- `ScoreCumulative` — PERFECT + GREAT → score=1800
- `ScorePercentage` — 5 PERFECT out of 10 notes → 50%
- `ScorePercentageZeroNotes` — total_notes=0 → 0%
- `JudgmentCountsByTier` — 3 PERFECT + 2 GREAT + 1 MISS → count(PERFECT)=3, count(MISS)=1
- `TotalJudged` — 10 events applied → total_judged()=10
- `ResetClearsAll` — apply events, reset, verify all zeros
- `ResetPreservesTotalNotes` — after reset, score_percentage denominator unchanged
- `MultipleStatesIndependent` — Two GameplayState instances, apply same events → identical results
- `ApplyBatch` — apply() with 5 events → all processed in order

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R GameplayState` passes all tests
- [ ] No SDL headers in gameplay_state.h or gameplay_state.cpp
- [ ] GameplayState has no reference to Judge, NoteData, TimingData, or AudioSystem

**Expected commit message**:
`feat(judge): add GameplayState with combo tracking and Phase 1 scoring`

**Estimated time**: ~1 hour

---

## Step 7: Integration Tests — Full Judge + GameplayState Pipeline

**Files**:
- Modify `test/test_judge.cpp` — Add end-to-end tests that wire Judge + GameplayState

**What to implement**:

Integration tests that construct a complete chart fixture (NoteData + TimingData), run the judge through a simulated song, and verify the final GameplayState.

These tests prove that the entire pipeline works together without SDL. They construct realistic scenarios:

Test scenarios:

1. **Full Perfect Song** (10 notes, all Perfect):
   - Build 10 tap notes at regular intervals on columns 0-4
   - Build 120 BPM TimingData
   - Create Judge and GameplayState
   - For each note: advance song position to note time, set pressed column, call update, apply events
   - Verify: score = 10000, max_combo = 10, all counts correct, is_complete = true

2. **Mixed Judgments** (10 notes, various tiers):
   - 3 PERFECT (exact), 2 GREAT (+20ms), 2 GOOD (+50ms), 1 BAD (+80ms), 2 auto-MISS
   - Verify: score = 3*1000+2*800+2*500+1*100+2*0 = 5700
   - Verify: max_combo = 7 (3+2+2 before BAD), current_combo = 0 (last 2 are MISS)

3. **60Hz Tick Simulation** (verifying frame independence):
   - Build chart with notes at known times
   - Run at 60 Hz: advance song_ms by 16.67ms each tick, call update each tick
   - Run same chart at 30 Hz: advance by 33.33ms each tick
   - Verify: both produce identical judgment_counts and max_combo (timing within 1 tick)

4. **Determinism / No RNG**:
   - Run full scenario twice with identical inputs
   - Verify: events are bitwise identical (same note_index, tier, error for each)

5. **Multiple GameplayState Observers**:
   - One Judge, two GameplayState instances
   - Pass same events to both
   - Verify: both have identical combo, score, counts

6. **Judge Reuse After Reset**:
   - Play through chart, reset judge + gameplay state, play again
   - Verify: second run produces identical results

7. **BPM Change Chart** (timing accuracy):
   - Build chart with BPM change mid-song (120 → 180 at beat 8)
   - Place notes before and after BPM change
   - Hit all notes at exact times
   - Verify: all PERFECT, timing errors near 0.0

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Judge` passes all tests (unit + integration)
- [ ] `cd build && ctest --output-on-failure -R GameplayState` passes all tests
- [ ] Zero SDL dependencies anywhere in judge source or test files
- [ ] `grep -rn "SDL_\|rand\|random\|RNG\|mt19937" src/openitup/judge/` returns zero matches
- [ ] `grep -rn "SDL_" test/test_judge.cpp test/test_gameplay_state.cpp` returns zero matches

**Expected commit message**:
`test(judge): add end-to-end integration tests for Judge + GameplayState pipeline`

**Estimated time**: ~1.5 hours

---

## Summary

| Step | What | Files Created/Modified | Stories Covered | Est. |
|------|------|----------------------|-----------------|------|
| 1 | JudgmentTier enum | 2 new + 1 modified | US-JDG-002 | 20m |
| 2 | TimingProfile struct | 2 new + 1 modified | US-JDG-019 | 20m |
| 3 | JudgmentEvent class | 2 new + 1 modified | US-JDG-004 | 25m |
| 4 | Judge core (classify + match) | 2 new + 1 modified | US-JDG-001, US-JDG-002, US-JDG-012 | 1.5h |
| 5 | Auto-miss + flush | 1 modified | US-JDG-003 | 1h |
| 6 | GameplayState | 2 new + 1 modified | US-JDG-005, US-JDG-006 | 1h |
| 7 | Integration tests | 1 modified | US-JDG-011, US-JDG-012 (all stories) | 1.5h |

**Total new source files**: 10 (5 headers + 5 .cpp in `src/openitup/judge/`)
**Total new test files**: 2 (`test/test_judge.cpp`, `test/test_gameplay_state.cpp`)
**CMakeLists.txt modifications**: Add 5 .cpp to `openitup_engine`, add 2 test files to `openitup_tests`

Each step is independently committable and leaves the build green. Steps 1-3 build foundational types with no cross-dependencies. Step 4 is the largest (core algorithm). Step 5 completes the judge. Step 6 adds GameplayState independently. Step 7 proves the full pipeline.
