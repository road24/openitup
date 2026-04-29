# TD-JDG-001: Gameplay Judge — Timing Classification, Event Emission, and GameplayState

**Stories**: US-JDG-001, US-JDG-002, US-JDG-003, US-JDG-004, US-JDG-005, US-JDG-006, US-JDG-011, US-JDG-012, US-JDG-019
**Phase**: 1
**Author**: technical-architect agent
**Status**: Draft

## Overview

This design introduces the gameplay judge subsystem for Phase 1: a pure-logic `Judge` class that classifies tap-note timing into five tiers, a `TimingProfile` value type holding hardcoded Exceed-era timing windows, a `JudgmentEvent` immutable struct for decoupled event emission, and a `GameplayState` class that consumes judgment events to maintain combo and score. The entire subsystem has zero SDL dependency — no audio, no rendering, no input polling. It accepts three inputs: `NoteData` and `TimingData` from TD-CHT-001, input edge events (pressed-mask bitmasks from TD-INP-001's `InputSnapshot`), and the current song position as a `double` millisecond value (sourced from TD-AUD-001's `get_position_ms()`).

The design follows patterns established throughout the codebase: `std::unique_ptr` ownership (as in `Engine`→`Clock`), scoped enums with string conversion helpers (as in `BlendEffect` in `keyframe.h`), `double` for all time values (as in `Clock` and `TimingData`), and header-only value types (as in `types.h`). The judge is designed for testability first — every component is constructable and callable from a GoogleTest suite with no initialization beyond C++ standard library.

## Architecture

### Component Diagram

```
GameplayScene (future, not in this design)
  |
  |  Calls each tick:
  |    double song_ms = audio_->get_position_ms();
  |    auto events = judge_->update(song_ms, input_snapshot);
  |    gameplay_state_->apply(events);
  |
  v

Judge (src/openitup/judge/judge.h)
  |  reads (const ref, non-owning)
  ├── NoteData (from TD-CHT-001)
  ├── TimingData (from TD-CHT-001)
  └── TimingProfile (src/openitup/judge/timing_profile.h)
  |
  |  per-tick inputs:
  ├── song_position_ms (double, from AudioSystem::get_position_ms())
  └── pressed_columns (uint32_t bitmask, from InputSnapshot::pressed_mask())
  |
  |  per-tick output:
  └── std::vector<JudgmentEvent> (sorted by beat)

GameplayState (src/openitup/judge/gameplay_state.h)
  |  consumes
  └── std::vector<JudgmentEvent> (from Judge::update)
  |
  |  maintains
  ├── current_combo_, max_combo_
  ├── score_
  └── judgment_counts_[] (per tier)
```

### New Types

#### `JudgmentTier` (`src/openitup/judge/judgment_tier.h`)

A scoped enum representing the five judgment categories. Follows the `BlendEffect` pattern: scoped enum with string conversion helpers.

```cpp
// src/openitup/judge/judgment_tier.h
#pragma once

#include <cstdint>
#include <string>

namespace openitup {

enum class JudgmentTier : uint8_t {
    PERFECT = 0,
    GREAT = 1,
    GOOD = 2,
    BAD = 3,
    MISS = 4,
};

// Total number of tiers, for array sizing.
inline constexpr int JUDGMENT_TIER_COUNT = 5;

// String conversion for logging and display.
const char* judgment_tier_to_string(JudgmentTier tier);
JudgmentTier judgment_tier_from_string(const std::string& s);

// Returns true if this tier maintains combo (Perfect, Great, Good).
inline constexpr bool tier_maintains_combo(JudgmentTier tier) {
    return tier == JudgmentTier::PERFECT ||
           tier == JudgmentTier::GREAT ||
           tier == JudgmentTier::GOOD;
}

} // namespace openitup
```

**Key decisions**:

- Integer values 0-4 are intentional and stable. They serve as array indices into `judgment_counts_[]` and per-tier scoring tables in future phases.
- `tier_maintains_combo()` encodes the combo rule (US-JDG-006): Perfect, Great, and Good maintain combo. Bad and Miss break it. This is a constexpr free function, not a method on GameplayState, because the rule is intrinsic to the judgment model and reusable across subsystems (e.g., note renderer for hit effects).
- MISS is included in the enum even though it has special generation rules (auto-miss). From the GameplayState's perspective, a MISS is a MISS regardless of how it was produced.

---

#### `TimingProfile` (`src/openitup/judge/timing_profile.h`)

A value type holding the timing window widths for each judgment tier. Phase 1 provides a single hardcoded `default_timing_profile()` factory. Phase 4 (US-JDG-013) will add JSON loading.

```cpp
// src/openitup/judge/timing_profile.h
#pragma once

namespace openitup {

struct TimingProfile {
    // Symmetric timing windows in milliseconds.
    // A hit within ±perfect_window_ms of the note is PERFECT, etc.
    // Windows must be ordered: perfect <= great <= good <= bad.
    double perfect_window_ms;
    double great_window_ms;
    double good_window_ms;
    double bad_window_ms;

    // Validate that windows are ordered and non-negative.
    // Returns true if valid.
    bool is_valid() const;
};

// Exceed-era hardcoded default (US-JDG-019).
// Perfect ±16ms, Great ±33ms, Good ±66ms, Bad ±100ms.
TimingProfile default_timing_profile();

} // namespace openitup
```

**Key decisions**:

- `double` for window widths, matching the engine's convention for all time values (TD-ENG-001 ADR-1). Although window widths are small integers in the Exceed era, future profiles may use fractional values.
- Windows are symmetric: the same width applies early and late. US-JDG-002 Scenario 4 confirms early/late is expressed as the sign of the timing error, not as separate windows. If a future PIU version needs asymmetric windows (unlikely but possible), the struct gains `_early` and `_late` fields without changing the judge API.
- `is_valid()` checks `0 < perfect <= great <= good <= bad`. This is called during construction/loading, not per-tick.
- The struct is a plain data type with no behavior beyond validation. The judge reads it; the struct does not perform classification.
- Phase 1 hardcoded values from US-JDG-019: Perfect ±16ms, Great ±33ms, Good ±66ms, Bad ±100ms. These match documented Exceed-era timing.

---

#### `JudgmentEvent` (`src/openitup/judge/judgment_event.h`)

An immutable value type emitted by the judge for each note judgment. Immutability is enforced by making all fields private with const-returning accessors and no setters — the same pattern as `InputSnapshot` in TD-INP-001.

```cpp
// src/openitup/judge/judgment_event.h
#pragma once

#include <cstdint>

#include <openitup/judge/judgment_tier.h>

namespace openitup {

class JudgmentEvent {
public:
    // Construct a complete judgment event. All fields set at construction.
    JudgmentEvent(std::size_t note_index, uint8_t column, double beat,
                  JudgmentTier tier, double timing_error_ms, bool is_auto_miss);

    // The index of the judged note within NoteData::events().
    std::size_t note_index() const;

    // Column of the judged note (0-4 for single, 0-9 for double).
    uint8_t column() const;

    // Beat position of the note.
    double beat() const;

    // Judgment classification.
    JudgmentTier tier() const;

    // Signed timing error in milliseconds.
    // Negative = early, positive = late, 0 = exact.
    // For auto-misses, this is the distance past the bad window.
    double timing_error_ms() const;

    // True if this judgment was produced by auto-miss (note passed
    // without input), false if produced by input matching.
    bool is_auto_miss() const;

    // Comparison for sorting by beat (used to order events within a tick).
    bool operator<(const JudgmentEvent& other) const;

private:
    std::size_t note_index_;
    uint8_t column_;
    double beat_;
    JudgmentTier tier_;
    double timing_error_ms_;
    bool is_auto_miss_;
};

} // namespace openitup
```

**Key decisions**:

- `note_index` rather than a separate `note_id` field. Notes in `NoteData` are identified by their index in the sorted events vector (TD-CHT-001). This is stable because NoteData is immutable after construction. The index is used for: (a) the judge's internal tracking of which notes have been judged, (b) the note renderer looking up which note to animate a hit effect on. US-JDG-004 Scenario 2 calls this "note_id" — the index serves as the id.
- `timing_error_ms` is signed: negative means early, positive means late (US-JDG-002 Scenario 4). For auto-misses, the value is `+bad_window_ms` (the maximum late error that triggers miss, per US-JDG-003 Scenario 1).
- `is_auto_miss` distinguishes automatic misses from input-driven misses (input arrived but was beyond the bad window). Both produce `JudgmentTier::MISS`, but the distinction is useful for analytics and the timing breakdown on the result screen.
- No `const` member variables — same reasoning as `InputSnapshot` in TD-INP-001. `const` members delete move assignment, making the type unusable in `std::vector`. Immutability is enforced by the class interface (no setters).
- `operator<` sorts by beat for US-JDG-004 Scenario 3: events emitted in beat order.

---

#### `Judge` (`src/openitup/judge/judge.h`)

The core judge class. Stateful — tracks which notes have been judged via an internal cursor and a bitset. Receives per-tick inputs (song position and pressed columns) and produces a vector of judgment events.

```cpp
// src/openitup/judge/judge.h
#pragma once

#include <cstdint>
#include <vector>

#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>
#include <openitup/judge/judgment_event.h>
#include <openitup/judge/timing_profile.h>

namespace openitup {

class Judge {
public:
    // Construct a judge for a chart.
    // note_data and timing_data must outlive the Judge.
    // profile is copied into the Judge.
    Judge(const NoteData& note_data, const TimingData& timing_data,
          const TimingProfile& profile);

    // Process one tick of judgment logic.
    // song_position_ms: current playback position from audio system.
    // pressed_columns: bitmask of columns pressed this tick (edge events,
    //   not held state). Bit N = 1 means column N was pressed this tick.
    //   This corresponds to InputSnapshot::pressed_mask() masked to panel bits.
    // Returns: judgment events produced this tick, sorted by beat.
    std::vector<JudgmentEvent> update(double song_position_ms,
                                       uint32_t pressed_columns);

    // Flush all remaining unjudged notes as misses.
    // Called at end of song to satisfy US-JDG-003 Scenario 3.
    std::vector<JudgmentEvent> flush_remaining();

    // Number of notes that have been judged so far.
    std::size_t judged_count() const;

    // Total number of judgable notes (TAP and HOLD_HEAD in Phase 1).
    std::size_t total_judgable() const;

    // True if all judgable notes have been judged.
    bool is_complete() const;

    // Access the timing profile (for GameplayState or display).
    const TimingProfile& profile() const;

    // Reset judge state (for retry). Notes and timing data unchanged.
    void reset();

private:
    const NoteData& note_data_;
    const TimingData& timing_data_;
    TimingProfile profile_;

    // Per-note judged state. Index matches NoteData::events() index.
    std::vector<bool> judged_;

    // Cursor: index of the first note that might still be unjudged.
    // Advances forward only. Notes before cursor are guaranteed judged.
    std::size_t cursor_;

    // Total count of judgable notes (precomputed at construction).
    std::size_t total_judgable_;

    // Count of notes judged so far.
    std::size_t judged_count_;

    // Classify the absolute timing error into a judgment tier.
    JudgmentTier classify(double abs_error_ms) const;

    // Find the best (closest) unjudged note for a given column and time.
    // Returns the note index, or SIZE_MAX if no match within bad window.
    std::size_t find_closest_unjudged(uint8_t column, double note_time_ms,
                                       double song_position_ms) const;
};

} // namespace openitup
```

**Key decisions**:

- The judge holds non-owning const references to `NoteData` and `TimingData`. These are owned by the `Chart` (TD-CHT-001), which outlives the judge (the chart is loaded before gameplay begins and stays alive until the result screen). This follows the same non-owning reference pattern as `BgaAnimation` referencing sprites.
- `TimingProfile` is copied into the judge (small value type, 32 bytes). Copying avoids lifetime concerns and allows the profile to be discarded after construction.
- `update()` returns `std::vector<JudgmentEvent>` by value. The vector is small (typically 0-3 events per tick, occasionally up to 10 for burst patterns) and benefits from move semantics. Returning by value makes the API clean and avoids the judge accumulating state that callers must poll.
- `pressed_columns` is a raw `uint32_t` bitmask rather than an `InputSnapshot` reference. This decouples the judge from the input system's types. The caller (GameplayScene) extracts the pressed mask from the snapshot and passes it. For testing, passing a bitmask directly is simpler than constructing InputSnapshot objects.
- `cursor_` is the key optimization. Notes are sorted by beat. Once the song position passes a note's beat by more than `bad_window_ms`, that note cannot be judged by input (it's an auto-miss candidate). The cursor advances past all such notes, so each `update()` only scans a narrow window, not the entire note list.
- `judged_` is `std::vector<bool>` (bitset). For a chart with 1000 notes, this uses ~125 bytes. The alternative (`std::vector<uint8_t>`) uses 1000 bytes. Since we index by note position and the vector is dense, `vector<bool>` is appropriate here despite its reputation — we never take pointers or references to individual bits.
- `find_closest_unjudged()` implements the note-to-input matching algorithm. When multiple unjudged notes exist on the same column within the judgable window, the closest one (smallest absolute error) wins. This prevents a late hit on note N from accidentally matching note N+1.
- `flush_remaining()` assigns MISS to all unjudged notes at end-of-song (US-JDG-003 Scenario 3). It is called explicitly rather than triggered by a sentinel time value, giving the caller control over when "end of song" occurs.
- Phase 1 only judges `NoteType::TAP` notes. `HOLD_HEAD` and `HOLD_TAIL` are present in `NoteData` but skipped by the judge (logged as warning if encountered). Phase 3 (US-JDG-007) extends the judge to handle holds. The `total_judgable_` precomputation counts only TAP notes in Phase 1.

---

#### `GameplayState` (`src/openitup/judge/gameplay_state.h`)

Consumes judgment events and maintains running game state. Completely decoupled from the judge — it receives events via `apply()`, not by observing judge internals.

```cpp
// src/openitup/judge/gameplay_state.h
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <openitup/judge/judgment_event.h>
#include <openitup/judge/judgment_tier.h>

namespace openitup {

class GameplayState {
public:
    // Construct with the total number of judgable notes in the chart.
    // Used for score percentage calculation.
    explicit GameplayState(std::size_t total_notes);

    // Apply a batch of judgment events (one tick's worth).
    // Events are processed in order. Combo and score update per event.
    void apply(const std::vector<JudgmentEvent>& events);

    // --- Combo ---

    int current_combo() const;
    int max_combo() const;

    // --- Score ---

    // Running score (integer, increases monotonically).
    int64_t score() const;

    // --- Judgment counts ---

    // Count of judgments by tier.
    int count(JudgmentTier tier) const;

    // Total judgments received so far.
    int total_judged() const;

    // --- Derived stats ---

    // Percentage of max possible score (0.0 to 100.0).
    // Returns 0.0 if no notes have been judged.
    double score_percentage() const;

    // --- Reset ---

    // Reset all state to initial values. Does not affect total_notes.
    void reset();

private:
    std::size_t total_notes_;

    int current_combo_ = 0;
    int max_combo_ = 0;
    int64_t score_ = 0;

    std::array<int, JUDGMENT_TIER_COUNT> judgment_counts_ = {};

    // Phase 1 hardcoded scoring constants.
    // Phase 4 (US-JDG-014) moves these into the judge profile.
    static constexpr int PERFECT_POINTS = 1000;
    static constexpr int GREAT_POINTS = 800;
    static constexpr int GOOD_POINTS = 500;
    static constexpr int BAD_POINTS = 100;
    static constexpr int MISS_POINTS = 0;

    // Points per tier, indexed by JudgmentTier integer value.
    static constexpr int POINTS_PER_TIER[JUDGMENT_TIER_COUNT] = {
        PERFECT_POINTS, GREAT_POINTS, GOOD_POINTS, BAD_POINTS, MISS_POINTS
    };

    // Score one judgment event.
    void apply_single(const JudgmentEvent& event);
};

} // namespace openitup
```

**Key decisions**:

- `GameplayState` takes `total_notes` at construction for percentage calculation. It does NOT take a reference to the judge or the chart. This enforces the separation required by US-JDG-005: the judge emits events, GameplayState consumes them, neither knows about the other.
- `apply()` takes a const reference to the event vector. Events are processed in order (beat-sorted, guaranteed by the judge). This is a pull model: the caller passes events explicitly. An alternative (observer/callback) would couple the judge and GameplayState via a shared interface. The explicit pass is simpler, testable without the judge, and supports US-JDG-005 Scenario 3 (multiple GameplayState instances consuming the same events by passing the vector to each).
- Score is `int64_t`. With 1000 notes at 1000 points each plus combo bonuses, the maximum score for a single chart is well under 2^31. But `int64_t` prevents overflow in pathological cases and is consistent with the project's preference for generous precision.
- Phase 1 scoring is simple: fixed points per tier, no combo multiplier. Phase 4 (US-JDG-014) introduces combo multipliers and profile-driven scoring. The `POINTS_PER_TIER` array is a `static constexpr` that Phase 4 replaces with a profile-driven lookup.
- `judgment_counts_` uses `std::array<int, 5>` indexed by the `JudgmentTier` integer value. This is O(1) lookup and avoids map overhead.
- No life gauge in Phase 1. US-JDG-010 (life gauge) is Phase 3. The GameplayState is designed to accommodate it via additional members, but Phase 1 omits it to keep scope tight.
- No grade calculation in Phase 1. US-JDG-015 (grade) is Phase 4. Same extension strategy.

---

### Modified Types

No existing types are modified. The judge subsystem is entirely new code. It reads `NoteData` and `TimingData` from TD-CHT-001 and `InputSnapshot`'s pressed mask from TD-INP-001, but does not modify those types.

The future `GameplayScene` (Phase 1, screen system) will wire the judge, audio, input, and chart together, but that wiring is outside this design's scope.

## File Plan

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/openitup/judge/judgment_tier.h` | JudgmentTier enum, string conversions, tier_maintains_combo |
| Create | `src/openitup/judge/judgment_tier.cpp` | judgment_tier_to_string/from_string implementations |
| Create | `src/openitup/judge/timing_profile.h` | TimingProfile struct, is_valid(), default_timing_profile() declaration |
| Create | `src/openitup/judge/timing_profile.cpp` | is_valid() and default_timing_profile() implementations |
| Create | `src/openitup/judge/judgment_event.h` | JudgmentEvent class declaration |
| Create | `src/openitup/judge/judgment_event.cpp` | JudgmentEvent constructor, accessors, operator< |
| Create | `src/openitup/judge/judge.h` | Judge class declaration |
| Create | `src/openitup/judge/judge.cpp` | Judge implementation (update, classify, find_closest, flush, reset) |
| Create | `src/openitup/judge/gameplay_state.h` | GameplayState class declaration |
| Create | `src/openitup/judge/gameplay_state.cpp` | GameplayState implementation (apply, combo, score, reset) |
| Modify | `CMakeLists.txt` | Add judge .cpp files to `openitup_engine` library sources |
| Create | `test/test_judge.cpp` | Unit tests for Judge, TimingProfile, JudgmentTier, JudgmentEvent |
| Create | `test/test_gameplay_state.cpp` | Unit tests for GameplayState combo, score, counts |
| Modify | `CMakeLists.txt` | Add test files to `openitup_tests` sources |

## Data Flow

### Normal Tick: Single Note Hit

```
1. GameplayScene::update(dt):
   song_ms = audio_->get_position_ms();           // e.g., 5015.0 ms
   const auto& input = input_system_->snapshot();
   uint32_t pressed = input.pressed_mask() & 0x03FF;  // mask to 10 panel bits

2. Judge::update(5015.0, pressed):
   a. Convert song position to beat:
      song_beat = timing_data_.beat_at_time(song_ms / 1000.0)  // 20.06 beats

   b. Compute judgable window in beats:
      early_beat = timing_data_.beat_at_time((song_ms - bad_window_ms) / 1000.0)
      late_beat  = timing_data_.beat_at_time((song_ms + bad_window_ms) / 1000.0)

   c. Auto-miss scan: iterate from cursor_ forward.
      For each unjudged note with beat < early_beat:
        note_time_ms = timing_data_.time_at_beat(note.beat) * 1000.0
        error = song_ms - note_time_ms  // guaranteed > bad_window_ms
        emit JudgmentEvent(index, column, beat, MISS, +bad_window_ms, true)
        mark judged_[index] = true
        advance cursor_

   d. Input matching: for each column bit set in pressed_columns:
      Find closest unjudged note on this column within [early_beat, late_beat]
      If found:
        note_time_ms = timing_data_.time_at_beat(note.beat) * 1000.0
        error_ms = song_ms - note_time_ms  // negative=early, positive=late
        tier = classify(abs(error_ms))
        emit JudgmentEvent(index, column, beat, tier, error_ms, false)
        mark judged_[index] = true

   e. Sort events by beat (US-JDG-004 SC3)
   f. Return events

3. GameplayScene passes events to GameplayState:
   gameplay_state_->apply(events);

4. GameplayState::apply(events):
   for each event:
     judgment_counts_[tier]++
     score_ += POINTS_PER_TIER[tier]
     if tier_maintains_combo(tier):
       current_combo_++
       max_combo_ = max(max_combo_, current_combo_)
     else:
       current_combo_ = 0
```

### Auto-Miss: Note Passes Without Input

```
Given: note at beat 20.0 (time 5000.0 ms), bad window ±100ms
Song position advances to 5150.0 ms (150ms past note)

Judge::update(5150.0, 0x0000):  // no inputs
  a. song_beat = ~20.6
  b. early_beat = beat_at_time(5050.0 / 1000.0) = ~20.2

  c. Auto-miss scan: note at beat 20.0 is before early_beat (20.2)
     note_time_ms = 5000.0
     error = 5150.0 - 5000.0 = 150.0 > 100.0 (bad window)
     emit JudgmentEvent(index, col, 20.0, MISS, +100.0, true)
     mark judged

  d. No pressed columns, skip input matching
  e. Return [MISS event]
```

### Boundary: Perfect at Exact Window Edge

```
Given: note at beat 4.0 (time 1000.0 ms), perfect window ±16ms

Judge::update(1016.0, pressed=column_2):
  a. Input matching: column 2 pressed, find note at beat 4.0 on column 2
  b. note_time_ms = 1000.0
  c. error_ms = 1016.0 - 1000.0 = +16.0
  d. abs_error = 16.0
  e. classify(16.0): 16.0 <= 16.0 → PERFECT  (boundary inclusive, US-JDG-002 SC5)
  f. emit JudgmentEvent(index, 2, 4.0, PERFECT, +16.0, false)
```

### Multiple Notes Same Tick

```
Given: notes at beats 4.0 (col 0) and 4.0 (col 3), song at 1000.0ms
Input: columns 0 and 3 both pressed

Judge::update(1000.0, pressed=0x0009):  // bits 0 and 3
  d. Input matching:
     - Column 0: find note at beat 4.0, col 0 → match → PERFECT at 0.0ms
     - Column 3: find note at beat 4.0, col 3 → match → PERFECT at 0.0ms
  e. Sort by beat: both at 4.0, stable order by column → col 0, col 3
  f. Return [PERFECT(col0), PERFECT(col3)]
```

### End of Song: Flush Remaining

```
GameplayScene detects song complete (audio state == STOPPED):
  auto remaining = judge_->flush_remaining();
  gameplay_state_->apply(remaining);

Judge::flush_remaining():
  for each unjudged note in events_:
    note_time_ms = timing_data_.time_at_beat(note.beat) * 1000.0
    emit JudgmentEvent(index, col, beat, MISS, +bad_window_ms, true)
    mark judged
  return all events (sorted by beat)
```

## Key Algorithms

### classify(abs_error_ms) — Timing Window Classification

```
Input: absolute timing error in milliseconds (always >= 0)
Output: JudgmentTier

if abs_error_ms <= profile_.perfect_window_ms: return PERFECT
if abs_error_ms <= profile_.great_window_ms:   return GREAT
if abs_error_ms <= profile_.good_window_ms:    return GOOD
if abs_error_ms <= profile_.bad_window_ms:     return BAD
return MISS
```

This is a linear scan of 4 comparisons. The windows are ordered (perfect < great < good < bad), so the first matching window is the best tier. Boundary is inclusive (US-JDG-002 Scenario 5: exactly ±16ms is PERFECT).

### find_closest_unjudged(column, note_time_ms, song_position_ms) — Input-Note Matching

```
Input:
  column: which column the input was on
  song_position_ms: current song time
Output:
  index of best matching note, or SIZE_MAX if no match

best_index = SIZE_MAX
best_abs_error = +infinity

// Scan from cursor_ to the end of the judgable window
for i in [cursor_, events_.size()):
  note = events_[i]
  note_time = timing_data_.time_at_beat(note.beat) * 1000.0

  // If note is too far in the future, stop scanning
  if note_time - song_position_ms > bad_window_ms: break

  // Skip already-judged notes
  if judged_[i]: continue

  // Skip wrong column
  if note.column != column: continue

  // Skip non-tap notes in Phase 1
  if note.type != NoteType::TAP: continue

  abs_error = abs(song_position_ms - note_time)

  // Outside the bad window? Skip
  if abs_error > bad_window_ms: continue

  // Closest match wins
  if abs_error < best_abs_error:
    best_abs_error = abs_error
    best_index = i

return best_index
```

**Why closest-match**: When two notes are on the same column within the judgable window (e.g., notes at beats 4.0 and 4.25 with a wide bad window), the input should match the closest note. This prevents the earlier note from "stealing" an input that was clearly aimed at the later note, and vice versa.

**Why scan from cursor**: Notes before the cursor are guaranteed to be past the late miss boundary. Scanning from cursor limits the search to the active window (typically 5-20 notes).

### update() — Per-Tick Top-Level Logic

```
Judge::update(song_position_ms, pressed_columns):
  events = []

  // Phase 1: auto-miss scan
  while cursor_ < events_.size():
    note = note_data_.events()[cursor_]
    if judged_[cursor_]:
      cursor_++
      continue

    // Skip non-judgable types
    if note.type != NoteType::TAP:
      cursor_++
      continue

    note_time_ms = timing_data_.time_at_beat(note.beat) * 1000.0
    // If this note is still within the judgable window, stop advancing
    if song_position_ms - note_time_ms <= bad_window_ms:
      break

    // Note is past the late miss boundary → auto-miss
    events.push_back(JudgmentEvent(cursor_, note.column, note.beat,
                                    MISS, +profile_.bad_window_ms, true))
    judged_[cursor_] = true
    judged_count_++
    cursor_++

  // Phase 2: match pressed columns to unjudged notes
  for column = 0; column < 10; column++:
    if !(pressed_columns & (1 << column)): continue

    idx = find_closest_unjudged(column, song_position_ms)
    if idx == SIZE_MAX: continue  // no matching note

    note = note_data_.events()[idx]
    note_time_ms = timing_data_.time_at_beat(note.beat) * 1000.0
    error_ms = song_position_ms - note_time_ms
    tier = classify(abs(error_ms))

    if tier == MISS:
      // Input was within scan range but outside bad window.
      // Do not judge the note as miss yet — it may be hit on a later tick.
      continue

    events.push_back(JudgmentEvent(idx, note.column, note.beat,
                                    tier, error_ms, false))
    judged_[idx] = true
    judged_count_++

  // Sort events by beat (US-JDG-004 SC3)
  std::sort(events.begin(), events.end())

  return events
```

**Critical subtlety**: When `find_closest_unjudged` returns a match but `classify` returns MISS (the absolute error exceeds the bad window), we do NOT emit a miss event. The note may be hit on the next tick when the song position is closer. Auto-miss handles the note if the entire window passes without a valid hit. This prevents premature misses on notes that the player hasn't had a full chance to hit yet.

## Dependencies

### Internal
- **NoteData** (`src/openitup/chart/note_data.h`, TD-CHT-001) — Read-only access to sorted note events. The judge iterates notes via `events()` and uses `notes_in_range()` for scanning.
- **TimingData** (`src/openitup/chart/timing_data.h`, TD-CHT-001) — Read-only access to `time_at_beat()` and `beat_at_time()` for converting between beat and millisecond domains.
- **NoteType** (`src/openitup/chart/note_type.h`, TD-CHT-001) — Used to filter judgable notes (TAP in Phase 1, HOLD_HEAD added in Phase 3).
- **InputSnapshot** (TD-INP-001) — The caller extracts `pressed_mask()` and passes the bitmask to `Judge::update()`. The judge itself does not include `input_snapshot.h`.
- **AudioSystem** (TD-AUD-001) — The caller queries `get_position_ms()` and passes the value to `Judge::update()`. The judge itself does not include `audio_system.h`.
- **spdlog** — Used for warning logging (e.g., encountering HOLD notes in Phase 1 judge).

### External (new libraries)
None. The judge subsystem is pure C++ with no external dependencies beyond what already exists in the project.

## Architectural Decisions

### ADR-1: Judge Accepts Raw Bitmask, Not InputSnapshot

- **Context**: The judge needs to know which columns were pressed this tick. It could accept an `InputSnapshot` reference (from TD-INP-001) or a raw `uint32_t` bitmask.
- **Decision**: Accept `uint32_t pressed_columns` — a bitmask where bit N means column N was pressed this tick.
- **Alternatives considered**: (a) Accept `const InputSnapshot&` — couples judge to input system types. Testing requires constructing InputSnapshot objects. (b) Accept a `std::vector<uint8_t>` of pressed column indices — allocation per tick, harder to batch-process.
- **Consequences**: The judge has zero input-system headers in its include chain. Testing is trivially simple (pass an integer). The caller (GameplayScene) does the one-line conversion: `snapshot.pressed_mask() & 0x03FF`. The 0x03FF mask extracts the 10 panel bits from PadInput (bits 0-9).

### ADR-2: Return Events by Value, Not Observer/Callback

- **Context**: The judge produces judgment events that GameplayState and potentially other consumers (note renderer, sound effects) need. Design choices: (a) return by value, (b) observer/callback interface, (c) internal event queue.
- **Decision**: `update()` returns `std::vector<JudgmentEvent>` by value. The caller passes the vector to each consumer.
- **Alternatives considered**: (a) Observer pattern (judge holds a list of `JudgeObserver*` pointers, calls `on_judgment()` during update) — adds coupling, complicates testing, makes event ordering harder to guarantee, and doesn't compose well with the fixed-step loop (observers would be called during judge's update, not at a caller-controlled point). (b) Internal queue (`judge.pop_events()`) — requires the caller to remember to pop, events accumulate if forgotten.
- **Consequences**: Simple, testable, composable. The typical event count per tick is 0-3, so the vector allocation is minimal (and can be reused with a member vector + move in a future optimization if profiling shows allocation pressure). US-JDG-005 Scenario 3 (multiple GameplayState instances) is trivially supported by passing the same vector to each.

### ADR-3: Note Index as Note Identity

- **Context**: JudgmentEvent needs to identify which note was judged. Options: (a) note index in NoteData::events(), (b) a separate note_id assigned by the parser, (c) the (beat, column, type) tuple.
- **Decision**: Use the index within `NoteData::events()`.
- **Alternatives considered**: (a) Separate ID field on NoteEvent — requires ChartBuilder to assign IDs, adds a field that must be unique, complicates serialization. (b) (beat, column, type) tuple — not guaranteed unique (two taps on the same column at the same beat is possible in malformed charts). (c) A hash — over-engineered for a dense, immutable array.
- **Consequences**: Index is stable because NoteData is immutable (TD-CHT-001 ADR-5). Index is O(1) to look up. The note renderer can use the same index to correlate hit effects to notes. The downside: if NoteData ever became mutable (it won't — it's explicitly immutable), indices would break. The immutability guarantee from TD-CHT-001 makes this safe.

### ADR-4: Cursor-Based Auto-Miss Rather Than Timer-Based

- **Context**: Auto-miss must be detected for notes that pass without input (US-JDG-003). Two approaches: (a) each update scans from a cursor, (b) each note gets a timer set at its expected time + bad_window.
- **Decision**: Cursor-based scan. The cursor advances forward through the note list, emitting auto-misses for notes that are past the late boundary.
- **Alternatives considered**: Timer-based — each note would need a scheduled "deadline" tick. This requires a priority queue or sorted timer list. Since notes are already sorted by beat and we process them in order, the cursor accomplishes the same thing with simpler code and no additional data structure.
- **Consequences**: Auto-miss detection is O(k) per tick where k is the number of notes that pass the boundary this tick (typically 0-1). The cursor only moves forward, so total work across all ticks is O(n) for n notes. No additional memory allocation. The cursor also serves as the lower bound for input matching searches.

### ADR-5: GameplayState Phase 1 Scoring Is Hardcoded Constants

- **Context**: US-JDG-014 (Phase 4) introduces profile-driven scoring formulas. Phase 1 needs scoring to function but JSON profiles are not yet loaded.
- **Decision**: GameplayState uses `static constexpr` arrays for Phase 1 scoring (1000/800/500/100/0 points per tier). No combo multiplier. Phase 4 replaces these with profile lookups.
- **Alternatives considered**: (a) Defer all scoring to Phase 4 — leaves GameplayState incomplete for Phase 1 testing and display. (b) Introduce a ScoringProfile struct now — over-engineering for Phase 1 when the formula will change in Phase 4.
- **Consequences**: Phase 1 has functional scoring for testing and minimal gameplay display. The constants are clearly marked as Phase 1 placeholders. Phase 4 changes the internal scoring logic without changing the public API (`score()`, `score_percentage()`).

### ADR-6: Milliseconds Throughout Judge API, Not Seconds

- **Context**: The engine uses `double` for time, but different subsystems use different units. `TimingData::time_at_beat()` returns seconds. `AudioSystem::get_position_ms()` returns milliseconds. Timing windows in PIU documentation are in milliseconds.
- **Decision**: The judge's external API (`update()`, `JudgmentEvent::timing_error_ms()`) and `TimingProfile` all use milliseconds. Internally, the judge converts `TimingData`'s seconds to milliseconds (multiply by 1000.0) at the point of use.
- **Alternatives considered**: (a) Use seconds throughout — timing windows become 0.016, 0.033, etc., which are less intuitive and more prone to transcription errors. (b) Milliseconds in judge, convert at the boundary — this is what we do. (c) A units wrapper type — over-engineered for two units.
- **Consequences**: The judge's API matches the audio system's `get_position_ms()` and the PIU community's conventions (timing windows are always discussed in milliseconds). The conversion `* 1000.0` happens inside `update()` when calling `timing_data_.time_at_beat()`. This is a single multiplication per note per tick, negligible cost.

## Frame Independence (US-JDG-011)

The judge achieves frame independence through its API design, not through internal fixed-step logic:

1. **The judge has no internal clock or timer**. It receives `song_position_ms` as a parameter. This value comes from the audio system's sample-accurate position (TD-AUD-001), not from any frame counter or wall clock.

2. **The judge is called from the fixed-step loop** (60 Hz). The Engine's `update()` method calls `judge.update()` with the audio position at each fixed step. Whether the display renders at 60 Hz or 144 Hz, the judge logic runs at exactly 60 Hz.

3. **The judge is deterministic**. Given the same `song_position_ms` and `pressed_columns` sequence, it produces the same events regardless of how many render frames occurred between logic ticks.

4. **Input timestamps are implicit in the tick**. In Phase 1, input is polled at the start of each 60 Hz tick (TD-INP-001). The pressed mask represents "was this column pressed during this tick." The judge uses `song_position_ms` as the timing reference, not an input timestamp. This means all inputs within one tick are judged against the same song position. At 60 Hz, the maximum timing quantization is ±8.33ms (half a tick period), which is within the Perfect window (±16ms).

5. **No RNG (US-JDG-012)**. The classify function is a pure comparison chain. No randomness anywhere in the judge. Replay determinism is guaranteed by replaying the same (song_position_ms, pressed_columns) sequence.

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Timing quantization at 60 Hz introduces ±8.33ms jitter | Med | High | This is inherent in the 60 Hz tick rate. Perfect window (±16ms) is wider than the jitter. Phase 5's input timestamp refinement (US-INP-011) can interpolate within the tick for sub-tick precision. Documented as known limitation. |
| Note-to-input matching selects wrong note when two notes are close on same column | Med | Med | `find_closest_unjudged` picks the closest note by absolute error. Test with notes 50ms apart on the same column. If ambiguity persists, add a preference for the earlier note (bias toward not leaving notes unjudged). |
| `std::vector<bool>` performance on large charts (10k+ notes) | Low | Low | `vector<bool>` is bitset-optimized. 10k notes = 1.25 KB. Access is O(1). If profiling ever shows bit access overhead, replace with `vector<uint8_t>` (10 KB, no bit manipulation). |
| Auto-miss cursor advancing past a note that the player could still hit (clock jitter) | High | Low | The cursor only advances past notes where `song_position_ms - note_time_ms > bad_window_ms`. The audio position is monotonically increasing (TD-AUD-001 guarantees this). As long as the audio position doesn't jump backwards, the cursor is correct. Seek is not supported during gameplay. |
| Phase 3 hold note extension requires significant judge refactoring | Med | Med | Phase 1 judge explicitly filters to TAP notes. Hold processing in Phase 3 adds a parallel code path (active holds list, per-tick panel state check) that does not modify the tap judgment logic. The `judged_` array and cursor work identically for HOLD_HEAD as for TAP. |
| GameplayState scoring constants don't match Exceed-era values | Low | Med | Phase 1 values (1000/800/500/100/0) are placeholder. Exact Exceed scoring is poorly documented. Phase 4 (US-JDG-014) replaces these with researched values. Log a warning if Phase 1 scoring is used in a release build. |

## Testing Strategy

### Unit Tests (`test/test_judge.cpp`) — Pure Logic, No SDL

All judge tests are pure C++ with no SDL dependency. They construct `NoteData`, `TimingData`, and `TimingProfile` directly in test code.

**JudgmentTier Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `AllTiersExist` | PERFECT, GREAT, GOOD, BAD, MISS all defined | US-JDG-002 SC1 |
| `TierMaintainsCombo` | Perfect/Great/Good → true, Bad/Miss → false | US-JDG-006 SC1/SC2 |
| `StringRoundTrip` | to_string/from_string are inverse for all tiers | — |
| `TierCount` | JUDGMENT_TIER_COUNT == 5 | US-JDG-002 |

**TimingProfile Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `DefaultProfileValues` | Perfect=16, Great=33, Good=66, Bad=100 | US-JDG-019 SC1 |
| `DefaultProfileIsValid` | is_valid() returns true | US-JDG-019 SC1 |
| `InvalidProfileNegative` | Negative window → is_valid() false | US-JDG-019 |
| `InvalidProfileUnordered` | Perfect > Great → is_valid() false | US-JDG-019 |
| `BoundaryProfileAllSame` | All windows equal → is_valid() true | — |

**JudgmentEvent Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `FieldsAccessible` | All constructor args retrievable via accessors | US-JDG-004 SC2 |
| `ImmutableNoSetters` | Compilation test: no public mutator methods | US-JDG-004 SC4 |
| `SortByBeat` | Events with beats 5.0, 3.0, 4.0 sort to 3.0, 4.0, 5.0 | US-JDG-004 SC3 |
| `NegativeErrorIsEarly` | error -5.0 means early | US-JDG-002 SC4 |
| `PositiveErrorIsLate` | error +5.0 means late | US-JDG-002 SC4 |

**Judge Classification Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `PerfectAtExactHit` | 0ms error → PERFECT | US-JDG-002 SC1 |
| `PerfectAtBoundary` | 16ms error → PERFECT (inclusive) | US-JDG-002 SC5 |
| `GreatOutsidePerfect` | 17ms error → GREAT | US-JDG-002 SC2 |
| `GreatAtBoundary` | 33ms error → GREAT | US-JDG-002 |
| `GoodOutsideGreat` | 34ms error → GOOD | US-JDG-002 |
| `GoodAtBoundary` | 66ms error → GOOD | US-JDG-002 |
| `BadOutsideGood` | 67ms error → BAD | US-JDG-002 |
| `BadAtBoundary` | 100ms error → BAD | US-JDG-002 |
| `MissBeyondBad` | 101ms error → MISS | US-JDG-002 SC3 |
| `EarlyPerfect` | -15ms → PERFECT, negative timing error | US-JDG-002 SC4 |

**Judge Update Tests** (construct chart fixtures inline):

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `SingleNotePerfect` | One note, exact hit → PERFECT event | US-JDG-001 SC2 |
| `SameInputsSameOutputs` | Two identical update calls → identical results | US-JDG-001 SC2, US-JDG-012 SC2 |
| `AutoMissWhenNoInput` | Note passes bad window → auto MISS emitted | US-JDG-003 SC1 |
| `AutoMissEmitsEvent` | Auto-miss produces JudgmentEvent with is_auto_miss=true | US-JDG-003 SC2 |
| `AllNotesJudgedAtEnd` | 10 notes, 7 inputs → flush gives 3 misses → 10 total | US-JDG-003 SC3 |
| `MissedNotesDontBlockFuture` | Miss note A, then hit note B normally | US-JDG-003 SC4 |
| `EventsInBeatOrder` | 3 notes judged in one tick → events sorted by beat | US-JDG-004 SC3 |
| `OneEventPerNote` | 3 notes, 3 inputs → exactly 3 events | US-JDG-004 SC1 |
| `EventContainsCompleteData` | Verify note_index, tier, error, column | US-JDG-004 SC2 |
| `IdenticalAt60And144Hz` | Same inputs produce same output regardless of call count | US-JDG-011 SC1 |
| `AudioPositionNotWallClock` | Pass audio position, verify error calculated from it | US-JDG-011 SC3 |
| `NoRNGInJudge` | Run 1000 identical evaluations → all identical | US-JDG-012 SC2 |
| `ResetClearsState` | After reset, judged_count is 0 and cursor is 0 | — |
| `ClosestNoteWins` | Two notes on same column, input hits closest | — |
| `WrongColumnIgnored` | Input on column 0 doesn't match note on column 2 | — |
| `MultipleColumnsOneTick` | Press 3 columns, each matches its column's note | — |
| `FlushRemainingAllMiss` | flush_remaining on fully unjudged chart → all MISS | — |

**No-SDL Verification Test**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `JudgeCompilesWithoutSDL` | test_judge.cpp includes no SDL headers, links without SDL | US-JDG-001 SC3 |

### Unit Tests (`test/test_gameplay_state.cpp`) — Pure Logic, No SDL

Tests construct `JudgmentEvent` objects directly and pass them to `GameplayState::apply()`. No Judge object needed.

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `InitialStateZero` | combo=0, score=0, all counts=0 | US-JDG-005 SC2 |
| `ComboIncrementsOnPerfect` | PERFECT → combo 0→1 | US-JDG-006 SC1 |
| `ComboIncrementsOnGreat` | GREAT → combo 1→2 | US-JDG-006 SC1 |
| `ComboIncrementsOnGood` | GOOD → combo 2→3 | US-JDG-006 SC1 |
| `ComboResetsOnBad` | BAD → combo 20→0 | US-JDG-006 SC2 |
| `ComboResetsOnMiss` | MISS → combo 10→0 | US-JDG-006 SC2 |
| `MaxComboTracked` | 10 → miss → 15 → miss → max=15 | US-JDG-006 SC3 |
| `ComboAccessible` | current_combo() returns expected value | US-JDG-006 SC4 |
| `ScoreIncrements` | PERFECT adds 1000 points | US-JDG-005 |
| `ScorePercentage` | 5 PERFECT out of 10 notes → 50% | US-JDG-005 |
| `JudgmentCountsByTier` | After mixed events, count(PERFECT)==3, count(MISS)==2 | — |
| `ResetClearsAll` | After reset, combo=0, score=0, counts=0 | US-JDG-005 SC2 |
| `JudgeHasNoScoreKnowledge` | Judge class has no score/combo methods | US-JDG-005 SC1 |
| `MultipleStatesIndependent` | Two GameplayState, same events → identical, no interference | US-JDG-005 SC3 |
| `StateDontInfluenceJudge` | GameplayState with combo 50 → no effect on judge classify | US-JDG-005 SC4 |
| `TotalJudgedMatchesApplied` | After applying 10 events, total_judged==10 | — |

---

*Generated from stories in docs/stories/05-gameplay-judge.md (Phase 1 subset)*
*Last updated: 2026-04-28*
