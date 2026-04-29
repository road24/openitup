# TD-CHT-001: Chart System — Data Model, Timing Conversion, and KSF Parser

**Stories**: US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004, US-CHT-005
**Phase**: 1
**Author**: technical-architect agent
**Status**: Draft

## Overview

This design introduces the chart subsystem for Phase 1: the internal `Chart` data structure, its three components (`ChartMetadata`, `TimingData`, `NoteData`), the beat-to-time and time-to-beat conversion algorithms, and a KSF file parser that produces `Chart` instances from Kick It Up chart files. The chart system is pure data -- it has no SDL dependency, no rendering, and no audio coupling. This makes every component trivially unit-testable.

The `TimingData` conversion functions follow the sorted-vector + binary-search pattern established by `evaluate_keyframes()` in `src/openitup/bga/keyframe.h`. The key difference: keyframe lookup returns interpolated properties, while timing lookup returns cumulative time offsets with piecewise-linear BPM integration. Both use `std::upper_bound` on a sorted vector to find the active segment in O(log n).

## Architecture

### Component Diagram

```
Chart (src/openitup/chart/chart.h)
  |  contains (value members)
  |
  ├── ChartMetadata (src/openitup/chart/chart_metadata.h)
  |     title, artist, genre, audio_path, difficulty, etc.
  |
  ├── TimingData (src/openitup/chart/timing_data.h)
  |     sorted vector of TimingEvent (BPM changes, stops)
  |     time_at_beat(double beat) -> double seconds
  |     beat_at_time(double seconds) -> double beat
  |
  └── NoteData (src/openitup/chart/note_data.h)
        sorted vector of NoteEvent (beat, column, type)
        notes_in_range(double lo, double hi) -> span

KsfParser (src/openitup/chart/ksf_parser.h)
  |  reads .ksf file
  |  produces Chart via ChartBuilder
  v
ChartBuilder (src/openitup/chart/chart_builder.h)
  |  mutable staging area for parsers
  |  validates + sorts + computes cumulative offsets
  |  produces immutable Chart
  v
Chart (immutable after construction)
```

### New Types

#### `NoteType` (`src/openitup/chart/note_type.h`)

An enum for all note types the internal representation supports. Follows the `BlendEffect` pattern in `keyframe.h`: scoped enum with string conversion helpers.

```cpp
// src/openitup/chart/note_type.h
#pragma once

#include <cstdint>
#include <string>

namespace openitup {

enum class NoteType : uint8_t {
    TAP = 0,
    HOLD_HEAD = 1,
    HOLD_TAIL = 2,
    MINE = 3,
    FAKE = 4,
    LIFT = 5,
};

const char* note_type_to_string(NoteType type);
NoteType note_type_from_string(const std::string& s);

} // namespace openitup
```

---

#### `PlayMode` (`src/openitup/chart/play_mode.h`)

An enum distinguishing single (5-panel) and double (10-panel) charts.

```cpp
// src/openitup/chart/play_mode.h
#pragma once

#include <cstdint>
#include <string>

namespace openitup {

enum class PlayMode : uint8_t {
    SINGLE = 0,   // 5 panels, columns 0-4
    DOUBLE = 1,   // 10 panels, columns 0-9
};

// Maximum valid column index for the mode.
inline constexpr int max_columns(PlayMode mode) {
    return mode == PlayMode::DOUBLE ? 10 : 5;
}

const char* play_mode_to_string(PlayMode mode);
PlayMode play_mode_from_string(const std::string& s);

} // namespace openitup
```

---

#### `NoteEvent` (`src/openitup/chart/note_data.h`)

A plain struct representing a single note. Sorted by beat position in the `NoteData` vector.

```cpp
// src/openitup/chart/note_data.h (partial — NoteEvent portion)
#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>

#include <openitup/chart/note_type.h>

namespace openitup {

struct NoteEvent {
    double beat;        // beat position (double for sub-beat precision)
    uint8_t column;     // 0-4 for single, 0-9 for double
    NoteType type;      // tap, hold_head, hold_tail, mine, fake, lift

    // Comparison for sorting: by beat, then column, then type.
    bool operator<(const NoteEvent& other) const {
        if (beat != other.beat) return beat < other.beat;
        if (column != other.column) return column < other.column;
        return static_cast<uint8_t>(type) < static_cast<uint8_t>(other.type);
    }

    bool operator==(const NoteEvent& other) const {
        return beat == other.beat && column == other.column && type == other.type;
    }
};
```

---

#### `NoteData` (`src/openitup/chart/note_data.h`)

A wrapper around a sorted `std::vector<NoteEvent>` providing range queries. The vector is sorted once after parsing (not maintained as a sorted insertion structure).

```cpp
// src/openitup/chart/note_data.h (continued)

class NoteData {
public:
    // Construct from a pre-sorted vector of events.
    explicit NoteData(std::vector<NoteEvent> events);

    // Total number of note events.
    std::size_t size() const;

    // True if no events.
    bool empty() const;

    // Access underlying sorted vector.
    const std::vector<NoteEvent>& events() const;

    // All notes with beat positions in [lo_beat, hi_beat).
    // Uses std::lower_bound/upper_bound for O(log n) endpoints.
    // Returns iterators into the events vector.
    using const_iterator = std::vector<NoteEvent>::const_iterator;
    std::pair<const_iterator, const_iterator>
        notes_in_range(double lo_beat, double hi_beat) const;

    // Count of notes by type (for metadata/stats).
    std::size_t count_by_type(NoteType type) const;

private:
    std::vector<NoteEvent> events_;
};

} // namespace openitup
```

**Key decisions**:

- `NoteData` takes ownership of a pre-sorted vector via move semantics. The builder sorts the vector before constructing `NoteData`, so `NoteData` itself does no sorting.
- `notes_in_range()` returns an iterator pair (not a copy) for zero-allocation scanning. The judge and note renderer will call this every tick with a narrow beat window.
- `double` for beat positions, not `float`. Sub-beat precision matters for 192nd-note subdivisions in KSF (1/192 = 0.00520833...). A `float` has 7 decimal digits of precision, which is adequate for most cases, but `double` matches the timing system's precision and avoids accumulation errors when converting between beat and time domains.

---

#### `TimingEvent` (`src/openitup/chart/timing_data.h`)

A timing event represents either a BPM change or a stop. The `TimingData` class holds these in sorted order and pre-computes cumulative time offsets for O(log n) lookup.

```cpp
// src/openitup/chart/timing_data.h
#pragma once

#include <vector>
#include <cstdint>

namespace openitup {

enum class TimingEventType : uint8_t {
    BPM_CHANGE = 0,
    STOP = 1,
};

struct TimingEvent {
    double beat;            // beat position where this event occurs
    TimingEventType type;

    // For BPM_CHANGE: the new BPM value (beats per minute).
    // For STOP: unused (0.0).
    double bpm;

    // For STOP: duration in seconds.
    // For BPM_CHANGE: unused (0.0).
    double stop_duration;

    // --- Pre-computed by TimingData::build() ---
    // Cumulative time in seconds from beat 0.0 to this event's beat,
    // including all prior stops. Used for O(1) time_at_beat within a segment.
    double cumulative_time;

    // Seconds per beat at this event's BPM (60.0 / bpm).
    // Pre-computed to avoid division in the hot path.
    double seconds_per_beat;

    // Comparison for sorting by beat position.
    bool operator<(const TimingEvent& other) const {
        if (beat != other.beat) return beat < other.beat;
        // BPM changes sort before stops at the same beat.
        return static_cast<uint8_t>(type) < static_cast<uint8_t>(other.type);
    }
};
```

---

#### `TimingData` (`src/openitup/chart/timing_data.h`)

The core timing conversion class. Stores a sorted vector of `TimingEvent` with pre-computed cumulative time offsets. Both `time_at_beat()` and `beat_at_time()` use `std::upper_bound` to find the active segment, then compute a linear offset within that segment.

```cpp
// src/openitup/chart/timing_data.h (continued)

class TimingData {
public:
    // Construct from a vector of timing events.
    // The constructor sorts events and pre-computes cumulative_time
    // and seconds_per_beat fields.
    explicit TimingData(std::vector<TimingEvent> events);

    // Default constructor: 120 BPM at beat 0, no stops.
    TimingData();

    // Convert beat position to elapsed time in seconds.
    // Returns the time including all BPM changes and stops up to that beat.
    // Beats before 0.0 return 0.0 (clamped).
    double time_at_beat(double beat) const;

    // Convert elapsed time in seconds to beat position.
    // Inverse of time_at_beat. Time during a stop returns the stop's beat.
    // Negative time returns 0.0 (clamped).
    double beat_at_time(double time) const;

    // Access the underlying event list (sorted).
    const std::vector<TimingEvent>& events() const;

    // BPM at a given beat position.
    double bpm_at_beat(double beat) const;

    // Total number of timing events.
    std::size_t size() const;

    // True if no events (should never happen -- default has 120 BPM at beat 0).
    bool empty() const;

private:
    std::vector<TimingEvent> events_;

    // Build cumulative time offsets. Called by constructor.
    void build_cumulative_offsets();
};

} // namespace openitup
```

**Key decisions**:

- The constructor accepts a mutable vector, sorts it, and pre-computes cumulative offsets in a single pass. This is an O(n log n) one-time cost at load time.
- `cumulative_time` on each event stores the exact time in seconds from beat 0 to that event's beat. `time_at_beat(b)` finds the last event before beat `b` via `upper_bound`, reads its `cumulative_time`, then adds `(b - event.beat) * event.seconds_per_beat`. This is O(log n) per query.
- `seconds_per_beat` is pre-computed as `60.0 / bpm` to avoid a division in the hot path. The judge calls `time_at_beat` every tick.
- Stops contribute their `stop_duration` to the cumulative time but do not advance the beat position. A stop at beat 4.0 lasting 1.0 second means: time_at_beat(4.0) includes that 1.0 second, but beats after 4.0 also include it in their cumulative base.
- Default constructor provides 120 BPM at beat 0 as a safe fallback.

---

#### `ChartMetadata` (`src/openitup/chart/chart_metadata.h`)

All display and identification data for a chart. All strings are `std::string` (UTF-8). Optional paths use `std::string` with empty-string default (not `std::optional`) to keep the struct POD-like and simple to serialize.

```cpp
// src/openitup/chart/chart_metadata.h
#pragma once

#include <string>

#include <openitup/chart/play_mode.h>

namespace openitup {

struct ChartMetadata {
    // Display info
    std::string title;
    std::string artist;
    std::string genre;
    std::string charter_name;

    // Difficulty
    std::string difficulty_name;   // "Easy", "Normal", "Hard", "Crazy", etc.
    int difficulty_rating = 0;     // numeric rating (1-28 classic, higher modern)

    // Play mode
    PlayMode mode = PlayMode::SINGLE;

    // File paths (relative to chart directory, resolved by parser)
    std::string audio_path;        // primary audio file
    std::string banner_path;       // song banner image
    std::string background_path;   // background image

    // Display BPM (what the player sees; may differ from actual timing)
    double display_bpm = 0.0;

    // Preview audio (Phase 3, but included here to avoid struct changes later)
    double preview_start_seconds = -1.0;   // -1.0 = not set
    double preview_length_seconds = -1.0;  // -1.0 = not set, default 15.0
};

} // namespace openitup
```

**Key decisions**:

- Preview fields are included now with sentinel defaults (-1.0 = unset) even though US-CHT-018 is Phase 3. This avoids a struct layout change later. The sentinel approach is simpler than `std::optional<double>` for a POD-like metadata struct.
- `display_bpm` is the BPM shown to the player on the song select screen. For single-BPM charts this equals the actual BPM. For variable-BPM charts, parsers may set this to the most common BPM. This is a display hint, not a timing value.
- `mode` is stored in metadata rather than derived at query time. The parser determines the mode from the chart format (KSF is always single, SSC specifies pump-single5/pump-double10). Storing it avoids scanning all notes to find the max column.
- All paths are relative to the chart directory. The parser resolves them to absolute paths at load time.

---

#### `Chart` (`src/openitup/chart/chart.h`)

The top-level immutable chart structure. Contains metadata, timing, and notes as value members. Once constructed, a Chart does not change.

```cpp
// src/openitup/chart/chart.h
#pragma once

#include <openitup/chart/chart_metadata.h>
#include <openitup/chart/timing_data.h>
#include <openitup/chart/note_data.h>

namespace openitup {

class Chart {
public:
    // Construct a complete chart. All components are moved in.
    Chart(ChartMetadata metadata, TimingData timing_data, NoteData note_data);

    // Access components (const references -- chart is immutable after construction).
    const ChartMetadata& metadata() const;
    const TimingData& timing_data() const;
    const NoteData& note_data() const;

    // Convenience: total duration in seconds (time at the last note's beat).
    double duration_seconds() const;

    // Convenience: total number of notes.
    std::size_t note_count() const;

private:
    ChartMetadata metadata_;
    TimingData timing_data_;
    NoteData note_data_;
};

} // namespace openitup
```

**Key decisions**:

- Chart is a class (not struct) because its members are private and access is const-only. This enforces immutability after construction without using `const` member variables (which would delete move assignment and make the type unusable in vectors).
- All three components are value members, not pointers. A Chart is self-contained and movable. `std::vector<Chart>` works naturally for Phase 4's multi-chart file support.
- No default constructor. A Chart must have all three components. This prevents partially-initialized charts from reaching gameplay code.

---

#### `ChartBuilder` (`src/openitup/chart/chart_builder.h`)

A mutable staging area that parsers populate incrementally. When parsing is complete, `build()` sorts the notes, validates column indices, builds cumulative timing offsets, and returns an immutable `Chart`.

```cpp
// src/openitup/chart/chart_builder.h
#pragma once

#include <string>
#include <vector>
#include <stdexcept>

#include <openitup/chart/chart.h>
#include <openitup/chart/chart_metadata.h>
#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>

namespace openitup {

class ChartLoadException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ChartBuilder {
public:
    ChartBuilder();

    // --- Metadata setters ---
    void set_title(std::string title);
    void set_artist(std::string artist);
    void set_genre(std::string genre);
    void set_charter_name(std::string name);
    void set_difficulty_name(std::string name);
    void set_difficulty_rating(int rating);
    void set_mode(PlayMode mode);
    void set_audio_path(std::string path);
    void set_banner_path(std::string path);
    void set_background_path(std::string path);
    void set_display_bpm(double bpm);

    // --- Timing event additions ---
    void add_bpm_change(double beat, double bpm);
    void add_stop(double beat, double duration_seconds);

    // --- Note additions ---
    void add_note(double beat, uint8_t column, NoteType type);

    // --- Build the immutable Chart ---
    // Sorts notes, validates data, builds cumulative timing offsets.
    // Throws ChartLoadException if data is fundamentally invalid
    // (e.g., no BPM events, negative BPM).
    // Logs warnings for non-fatal issues (e.g., column out of range).
    Chart build();

private:
    ChartMetadata metadata_;
    std::vector<TimingEvent> timing_events_;
    std::vector<NoteEvent> note_events_;
};

} // namespace openitup
```

**Key decisions**:

- `ChartBuilder` is the only way to construct a `Chart`. Parsers call setters and `add_*` methods in any order, then call `build()`. This separates the mutable parsing phase from the immutable runtime phase.
- `build()` validates and logs warnings via spdlog for non-fatal issues (column index out of range for the declared mode, zero-BPM events). It throws `ChartLoadException` only for fundamentally broken data (no BPM events at all).
- `ChartLoadException` extends `std::runtime_error`, following the throw-from-constructor pattern established in TD-ENG-001 for fatal errors.
- The builder does not check for duplicate notes, orphaned holds, or overlapping notes. That is US-CHT-016 (Phase 4 validation). Phase 1 builds the structure; Phase 4 validates it.

---

#### `KsfParser` (`src/openitup/chart/ksf_parser.h`)

Parses Kick It Up `.ksf` text files. KSF is a line-oriented text format with `#TAG:VALUE;` metadata headers followed by tick-based note data. The parser reads the file, populates a `ChartBuilder`, and returns a `Chart`.

```cpp
// src/openitup/chart/ksf_parser.h
#pragma once

#include <string>
#include <filesystem>
#include <functional>

#include <openitup/chart/chart.h>

namespace openitup {

// Type alias for file reading. Injectable for testing.
// Default: reads entire file to string from filesystem.
using FileReaderFn = std::function<std::string(const std::filesystem::path&)>;

class KsfParser {
public:
    // Default constructor: reads files from the filesystem.
    KsfParser();

    // Injectable constructor for testing: provide custom file reader.
    explicit KsfParser(FileReaderFn file_reader);

    // Parse a .ksf file and return a Chart.
    // chart_path: path to the .ksf file.
    // Throws ChartLoadException on fatal parse errors.
    // Logs warnings for non-fatal issues (missing optional fields).
    Chart parse(const std::filesystem::path& chart_path) const;

private:
    FileReaderFn file_reader_;
};

} // namespace openitup
```

**Key decisions**:

- The parser is injectable via `FileReaderFn`, following the established `TextureCache::ImageLoaderFn` and `Clock::CounterFn` patterns. Tests provide a lambda that returns a string (the file contents) without touching the filesystem.
- `parse()` takes a `std::filesystem::path` so it can resolve the audio filename relative to the chart directory (US-CHT-005 Scenario 6). The path's parent directory is the chart directory.
- `parse()` returns a single `Chart`. KSF files contain one chart per file (unlike SSC which contains multiple). Phase 4 parsers that produce multiple charts will return `std::vector<Chart>`.
- The parser is const-correct: `parse()` is const because the parser holds no mutable state between calls. Multiple files can be parsed with the same parser instance.

## File Plan

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/openitup/chart/note_type.h` | NoteType enum with string conversion declarations |
| Create | `src/openitup/chart/note_type.cpp` | NoteType string conversion implementations |
| Create | `src/openitup/chart/play_mode.h` | PlayMode enum with max_columns() and string conversions |
| Create | `src/openitup/chart/play_mode.cpp` | PlayMode string conversion implementations |
| Create | `src/openitup/chart/chart_metadata.h` | ChartMetadata struct (header-only, no .cpp) |
| Create | `src/openitup/chart/note_data.h` | NoteEvent struct and NoteData class declaration |
| Create | `src/openitup/chart/note_data.cpp` | NoteData implementation (range queries, count_by_type) |
| Create | `src/openitup/chart/timing_data.h` | TimingEvent struct and TimingData class declaration |
| Create | `src/openitup/chart/timing_data.cpp` | TimingData implementation (build offsets, time_at_beat, beat_at_time) |
| Create | `src/openitup/chart/chart.h` | Chart class declaration |
| Create | `src/openitup/chart/chart.cpp` | Chart implementation (accessors, duration) |
| Create | `src/openitup/chart/chart_builder.h` | ChartBuilder class + ChartLoadException declaration |
| Create | `src/openitup/chart/chart_builder.cpp` | ChartBuilder build logic (sort, validate, construct) |
| Create | `src/openitup/chart/ksf_parser.h` | KsfParser class declaration |
| Create | `src/openitup/chart/ksf_parser.cpp` | KSF file parsing implementation |
| Modify | `CMakeLists.txt` | Add all .cpp files to `openitup_engine` sources |
| Create | `test/test_chart.cpp` | Unit tests for NoteType, PlayMode, NoteEvent, NoteData, Chart |
| Create | `test/test_timing_data.cpp` | Unit tests for TimingData (beat/time conversions, BPM changes, stops) |
| Create | `test/test_ksf_parser.cpp` | Unit tests for KsfParser with injectable file reader |
| Modify | `CMakeLists.txt` | Add test files to `openitup_tests` sources |
| Create | `test/fixtures/test_basic.ksf` | Minimal KSF fixture for regression testing |

## Data Flow

### Chart Loading (KSF)

```
1. KsfParser::parse("/data/Pumptris/pumptris.ksf")
   a. file_reader_("/data/Pumptris/pumptris.ksf") -> raw text
   b. Create ChartBuilder

2. Parse metadata lines:
   "#TITLE:Pumptris;"     -> builder.set_title("Pumptris")
   "#ARTIST:BanYa;"       -> builder.set_artist("BanYa")
   "#BPM:145;"            -> builder.add_bpm_change(0.0, 145.0)
                              builder.set_display_bpm(145.0)
   "#AUDIOFILE:pumptris.ogg;" -> resolve to "/data/Pumptris/pumptris.ogg"
                                  builder.set_audio_path("/data/Pumptris/pumptris.ogg")
   "#TICKCOUNT:2;"        -> ticks_per_beat = 2 (note: KSF TICKCOUNT varies)

3. Parse note data section:
   For each line of 5 characters (one per column):
     '1' -> builder.add_note(current_beat, column, NoteType::TAP)
     '4' -> builder.add_note(current_beat, column, NoteType::HOLD_HEAD)
     '0' -> no note
     current_beat += 1.0 / lines_per_beat

4. builder.build()
   a. Sort note_events_ by beat
   b. Sort timing_events_ by beat
   c. Validate: at least one BPM event exists
   d. Build cumulative time offsets on timing events
   e. Log warnings for any issues
   f. Return Chart(metadata_, TimingData(timing_events_), NoteData(note_events_))
```

### time_at_beat(beat) Query

```
Given: events_ = [
  { beat=0.0,  BPM_CHANGE, bpm=120, cumulative_time=0.0, spb=0.5 },
  { beat=8.0,  BPM_CHANGE, bpm=180, cumulative_time=4.0, spb=0.333 },
  { beat=8.0,  STOP,       stop=1.0, cumulative_time=4.0 + 1.0 = 5.0 },
]

Query: time_at_beat(12.0)

1. upper_bound(events_, 12.0, by beat) -> points to end
2. prev(it) -> event at beat 8.0, STOP (cumulative_time=5.0, spb=0.333)
   Wait -- we need the BPM, not the stop. The algorithm must find
   the last BPM_CHANGE at or before the query beat.

REVISED: The cumulative approach needs careful handling of stops.
See the "Key Algorithms" section below for the precise formulation.
```

### beat_at_time(time) Query (Inverse)

```
Given the same events, query: beat_at_time(5.5)

1. Binary search for the segment where cumulative_time <= 5.5
2. Find last event with cumulative_time <= 5.5:
   Event at beat 8.0 (STOP), cumulative_time = 5.0
3. Remaining time: 5.5 - 5.0 = 0.5 seconds
4. BPM at this point: 180 -> seconds_per_beat = 0.333
5. Additional beats: 0.5 / 0.333 = 1.5
6. Result: 8.0 + 1.5 = 9.5 beats
```

### notes_in_range(lo, hi) Query

```
Given: events_ = [
  { beat=0.0, col=0, TAP },
  { beat=2.5, col=1, TAP },
  { beat=4.0, col=2, TAP },
  { beat=8.0, col=3, TAP },
]

Query: notes_in_range(2.0, 5.0)

1. lower_bound(events_, 2.0, by beat) -> iterator to beat=2.5
2. lower_bound(events_, 5.0, by beat) -> iterator to beat=8.0
3. Return pair: [beat=2.5, beat=4.0]  (two notes in range)
```

## Key Algorithms

### Cumulative Time Offset Computation (build_cumulative_offsets)

Called once during `TimingData` construction. Pre-computes `cumulative_time` and `seconds_per_beat` for each event so that `time_at_beat` can run in O(log n).

```
Precondition: events_ sorted by (beat, type) where BPM_CHANGE < STOP

current_bpm = 120.0  // fallback if first event isn't at beat 0
current_spb = 60.0 / current_bpm
cumulative = 0.0
prev_beat = 0.0

for each event in events_:
    // Time elapsed from prev_beat to this event's beat at the current BPM
    beat_delta = event.beat - prev_beat
    cumulative += beat_delta * current_spb

    if event.type == BPM_CHANGE:
        event.cumulative_time = cumulative
        current_bpm = event.bpm
        current_spb = 60.0 / current_bpm
        event.seconds_per_beat = current_spb

    else if event.type == STOP:
        event.cumulative_time = cumulative
        cumulative += event.stop_duration
        event.seconds_per_beat = current_spb  // inherit current BPM

    prev_beat = event.beat
```

After this pass, each event's `cumulative_time` is the exact elapsed seconds from beat 0 to that event, including all prior stops but NOT including the current event's stop duration (if it is a stop). The stop's duration is added to `cumulative` after setting the event's `cumulative_time`, so subsequent events include it.

Wait -- this is subtle. Let me clarify the stop semantics:

A stop at beat 4.0 lasting 1.0 second means: when the song reaches beat 4.0, time freezes for 1.0 second before the song continues. So:
- `time_at_beat(3.99)` = time up to beat 3.99 (no stop yet)
- `time_at_beat(4.0)` = time up to beat 4.0 + 1.0 second stop
- `time_at_beat(4.01)` = time up to beat 4.0 + 1.0 + time from 4.0 to 4.01

For the cumulative model, we store the stop event with `cumulative_time` = time at beat 4.0 AFTER the stop (i.e., including the stop duration). This way, any beat query past the stop automatically includes the stop's time.

**Revised algorithm**:

```
current_bpm = 120.0
current_spb = 60.0 / current_bpm
cumulative = 0.0
prev_beat = 0.0

for each event in events_:
    beat_delta = event.beat - prev_beat
    cumulative += beat_delta * current_spb

    if event.type == BPM_CHANGE:
        event.cumulative_time = cumulative
        current_bpm = event.bpm
        current_spb = 60.0 / current_bpm
        event.seconds_per_beat = current_spb

    else if event.type == STOP:
        cumulative += event.stop_duration
        event.cumulative_time = cumulative
        event.seconds_per_beat = current_spb  // BPM unchanged by stops

    prev_beat = event.beat
```

Now `event.cumulative_time` for a stop event is the time at that beat INCLUDING the stop. For a BPM change, it is the time at that beat BEFORE any BPM change takes effect (the new BPM applies after).

### time_at_beat(beat)

```
if events_ is empty: return 0.0
if beat <= 0.0: return 0.0

// Find the last event at or before 'beat'
it = upper_bound(events_, beat, compare by beat)
  // upper_bound gives first event with beat > query
  // prev(it) gives last event with beat <= query

if it == events_.begin():
    // No events at or before this beat -- use default 120 BPM
    return beat * (60.0 / 120.0)

--it  // now points to last event at or before 'beat'

// Linear offset from that event to the query beat
remaining_beats = beat - it->beat
time = it->cumulative_time + remaining_beats * it->seconds_per_beat

return time
```

This is O(log n) for the binary search plus O(1) for the arithmetic.

### beat_at_time(time)

The inverse is slightly more complex because stops create flat regions where time advances but beat does not.

```
if events_ is empty: return 0.0
if time <= 0.0: return 0.0

// Find the last event with cumulative_time <= time.
// We search by cumulative_time, not by beat.
// Use a linear scan of events backwards from upper_bound.

// First, find the segment by cumulative_time.
// We need to search events by cumulative_time field.
// Since events are sorted by beat AND cumulative_time is monotonically
// increasing with beat, we can use upper_bound with a custom comparator.

it = upper_bound(events_, time,
    [](double t, const TimingEvent& e) { return t < e.cumulative_time; })
  // first event with cumulative_time > time

if it == events_.begin():
    // Before all events -- use default 120 BPM
    return time / (60.0 / 120.0)

--it  // last event with cumulative_time <= time

// Time remaining after this event
remaining_time = time - it->cumulative_time

// Convert remaining time to beats
if it->seconds_per_beat > 0.0:
    remaining_beats = remaining_time / it->seconds_per_beat
else:
    remaining_beats = 0.0  // zero BPM = time doesn't advance

return it->beat + remaining_beats
```

**Correctness of cumulative_time monotonicity**: Since events are sorted by beat, and cumulative_time is the integral of 1/BPM over beats (plus stops), cumulative_time is strictly non-decreasing. It increases monotonically as long as BPM > 0 (which we validate in `build()`). Stops add positive duration. So `upper_bound` on `cumulative_time` is valid.

**Stop handling in beat_at_time**: During a stop, the time advances but the beat does not. If the query time falls within a stop window (between the pre-stop cumulative time and the post-stop cumulative time), `remaining_time` will be positive but `remaining_beats` will correctly be 0 because we're at the stop's beat position. Actually -- let me think about this more carefully.

With the revised cumulative model, a stop event at beat 4.0 with duration 1.0s has `cumulative_time` = time at beat 4.0 + 1.0s. If we query `beat_at_time(T)` where T is in the middle of the stop (after the BPM-change's cumulative_time but before the stop's cumulative_time), `upper_bound` will find the stop event as the first with cumulative_time > T, then we step back to the BPM-change event. The remaining_time is positive, and we compute remaining_beats normally, which would give us a beat past 4.0. But we should return beat 4.0 during the stop.

This means the cumulative model needs one more refinement: we need to distinguish "time consumed by BPM traversal" from "time consumed by stops." One approach: store a `cumulative_time_before_stop` on stop events that equals the time at the stop's beat WITHOUT the stop duration, plus a `cumulative_time_after_stop` that includes it. The query checks if the time falls within the stop window.

**Simpler approach**: Flatten BPM changes and stops into a unified segment list where each segment has:
- `start_beat`, `start_time`, `seconds_per_beat`, and `is_stop`

For stops, `seconds_per_beat` is effectively infinite (beat doesn't advance). For BPM changes, `seconds_per_beat` is `60.0 / bpm`.

Let me adopt this cleaner segment model:

### Revised: TimingSegment (replaces TimingEvent for internal storage)

The `TimingData` constructor converts the raw `TimingEvent` input into a flat list of `TimingSegment` entries. Each segment represents a region where time advances at a constant rate.

```cpp
struct TimingSegment {
    double start_beat;          // beat where this segment begins
    double start_time;          // cumulative time at start_beat
    double seconds_per_beat;    // 60.0 / bpm for BPM segments, 0.0 for stops
    double duration_seconds;    // for stops: stop duration. for BPM: 0.0 (unbounded)
    bool is_stop;               // true = beat is frozen during this segment
};
```

**Segment construction**:

```
Input: sorted TimingEvents (BPM changes and stops)
Output: segments_

current_bpm = 0.0  (will be set by first BPM event)
current_time = 0.0
prev_beat = 0.0

for each event in sorted_events:
    if current_bpm > 0:
        beat_delta = event.beat - prev_beat
        current_time += beat_delta * (60.0 / current_bpm)

    if event.type == BPM_CHANGE:
        current_bpm = event.bpm
        segments_.push_back({
            .start_beat = event.beat,
            .start_time = current_time,
            .seconds_per_beat = 60.0 / event.bpm,
            .duration_seconds = 0.0,
            .is_stop = false,
        })

    else if event.type == STOP:
        segments_.push_back({
            .start_beat = event.beat,
            .start_time = current_time,
            .seconds_per_beat = 0.0,
            .duration_seconds = event.stop_duration,
            .is_stop = true,
        })
        current_time += event.stop_duration

    prev_beat = event.beat
```

Now `time_at_beat(beat)` finds the last non-stop segment at or before `beat` and adds the linear offset. Stops at the exact query beat add their durations.

`beat_at_time(time)` finds the segment where `start_time <= time < start_time + segment_span` and inverts.

This is more robust than the single-event cumulative approach because stop windows are explicit. The implementation will store `std::vector<TimingSegment> segments_` instead of the raw events.

**Revised public interface**: `TimingData` still accepts `std::vector<TimingEvent>` in its constructor (the parser-facing API). Internally, it converts to `std::vector<TimingSegment>`. The raw events are also stored for serialization/inspection. The segment list is the O(log n) lookup structure.

I will revise the `TimingData` class to reflect this:

```cpp
class TimingData {
public:
    explicit TimingData(std::vector<TimingEvent> events);
    TimingData();  // 120 BPM at beat 0

    double time_at_beat(double beat) const;
    double beat_at_time(double time) const;
    double bpm_at_beat(double beat) const;

    const std::vector<TimingEvent>& events() const;
    std::size_t size() const;
    bool empty() const;

private:
    std::vector<TimingEvent> events_;      // original events, for serialization
    std::vector<TimingSegment> segments_;   // compiled lookup structure

    void build_segments();
};
```

### time_at_beat(beat) -- Final Algorithm

```
if segments_ is empty: return 0.0
if beat <= 0.0: return 0.0

time = 0.0

// Find the last BPM segment at or before 'beat'
// Binary search on segments_ by start_beat
it = upper_bound(segments_, beat, compare by start_beat)
// step back to last segment at or before beat
if it == segments_.begin(): return 0.0
--it

// Walk backwards if we landed on a stop at the same beat as a BPM segment
// Actually, since segments are ordered and we want the BPM segment:

// Simpler: iterate forward through all segments, accumulating time.
// But that's O(n). Use binary search instead.

// With the segment model, binary search by start_beat works because
// segments are ordered by (start_beat, is_stop) where stops come after
// BPM changes at the same beat.

// upper_bound gives first segment with start_beat > beat.
// All segments before it have start_beat <= beat.

// The last segment before 'it' is our active segment.
// If it's a stop, the beat is at the stop's beat and we include the stop duration.
// If it's a BPM segment, we compute linear offset.

// Handle: there may be multiple segments at the same beat (BPM + stop).
// We need to include all stops at or before the query beat.

// The start_time of each segment already includes all prior stops.
// So we just need to find the last segment and compute offset.

time = it->start_time

if it->is_stop:
    // Query is at or past the stop's beat. The stop's full duration is
    // already included in the next segment's start_time. But if this is
    // the last segment, add the stop duration.
    time += it->duration_seconds
    // Beat offset is 0 (beat doesn't advance during stop)
else:
    // BPM segment: add linear offset
    remaining_beats = beat - it->start_beat
    time += remaining_beats * it->seconds_per_beat

return time
```

Actually, I am overcomplicating this. Let me simplify to a clean, proven model.

### Final Approach: Two Parallel Vectors

Store BPM changes and stops as separate concepts, but combine them into a single timeline for lookup. The proven approach used by StepMania's `TimingData` (which this engine is inspired by) is:

1. Store a vector of BPM segments, each with `(beat, bpm, cumulative_time_at_beat)`.
2. Store a vector of stops, each with `(beat, duration)`.
3. `time_at_beat(b)`: find the BPM segment, compute base time, then add all stops at or before `b`.
4. `beat_at_time(t)`: reverse the process.

But this requires scanning stops, which is O(n) in the worst case.

The cleanest O(log n) approach is the **segment list** where every BPM change and stop boundary creates a new segment, and each segment stores `(start_beat, end_beat, start_time, seconds_per_beat)`. For stops, `seconds_per_beat = 0` and `end_beat = start_beat` (beat doesn't advance). The segment's time span equals `stop_duration`.

Let me finalize with this concrete model. The implementation in `timing_data.cpp` will use `TimingSegment` as an internal detail:

```cpp
// Internal to timing_data.cpp, not exposed in header
struct TimingSegment {
    double start_beat;
    double start_time;          // cumulative seconds at start_beat
    double seconds_per_beat;    // 60/bpm. 0 for stops.
    double stop_duration;       // >0 only for stop segments
    bool is_stop;
};
```

**time_at_beat(beat)**:
```
1. upper_bound(segments_, beat, by start_beat) -> it
2. --it (last segment with start_beat <= beat)
3. Walk backwards collecting any stop segments at this exact beat
4. Base = it->start_time + (beat - it->start_beat) * it->seconds_per_beat
5. Add stop durations at exactly this beat
```

Wait -- this still has the "multiple segments at same beat" problem. Let me just use a single merged list where each segment knows both its time contribution and beat contribution:

**Final final approach** (keeping it simple for Phase 1):

Phase 1 only needs BPM changes and stops. No warps, no delays. The data set is small (typically < 10 events). The algorithm:

1. Store `std::vector<TimingEvent>` sorted by beat.
2. Pre-compute `cumulative_time` on each event (time from beat 0 to this event's beat, including all prior stops but using a two-field approach).
3. For `time_at_beat(b)`: binary search for the active BPM, compute base time, add stop contributions.

Since the typical chart has 1-5 timing events, even an O(n) scan per query is sub-microsecond. But we design for O(log n) because the story requires it (US-CHT-003 Scenario 5: 100 events, 10k queries in < 10ms).

**I will use the segment approach in the implementation** but keep the public API clean. The header exposes `TimingData` with `time_at_beat/beat_at_time`. The `.cpp` file internally builds a segment list. The technical details of the segment model are implementation, not interface.

## Dependencies

### Internal
- **spdlog** -- Used for warning/error logging in parser and builder. Already linked via `openitup_engine`.
- **nlohmann/json** -- Not used in Phase 1 chart system. Will be used in Phase 4 for OSF parser.
- **std::filesystem** -- Used by KsfParser for path resolution. Part of C++17 standard library, no additional linking needed.

### External (new libraries)
None. The chart system is pure C++ with no external dependencies beyond what is already in the project.

## Architectural Decisions

### ADR-1: ChartBuilder Mediates Between Parsers and Chart

- **Context**: Parsers need to populate chart data incrementally as they read file contents. But Chart should be immutable after construction. Something must bridge the mutable parsing phase and the immutable runtime phase.
- **Decision**: A `ChartBuilder` class provides a mutable API (`set_*`, `add_*`) for parsers, and a `build()` method that sorts, validates, and returns an immutable `Chart`.
- **Alternatives considered**: (a) Chart constructors that accept raw vectors directly -- parsers must sort and validate themselves, duplicating logic across parsers. (b) A mutable Chart with a `freeze()` method -- runtime cost to check frozen state on every access, and a frozen-but-mutable type is confusing.
- **Consequences**: All parsers (KSF now, SSC/SMA/etc in Phase 4) share validation and sorting logic. Adding a new parser requires only implementing the parsing logic, not the data finalization.

### ADR-2: Double Precision for Beat Positions

- **Context**: KSF uses tick-based timing (commonly 192 ticks per quarter note). A beat value of 1/192 = 0.005208... requires sub-beat precision. The timing system also uses `double` (established in TD-ENG-001).
- **Decision**: All beat positions are `double`, not `float`.
- **Alternatives considered**: (a) `float` -- 7 decimal digits of precision. A beat at 10000.0 + 1/192 would lose the fractional part. Unlikely in practice (10000 beats = ~83 minutes at 120 BPM), but `double` eliminates the concern entirely. (b) Rational numbers (`int numerator / int denominator`) -- exact representation but complex arithmetic and harder to binary-search.
- **Consequences**: Consistent with the engine's `double`-for-time convention (TD-ENG-001 ADR-1). No precision concerns for any realistic chart length.

### ADR-3: Injectable FileReaderFn for KSF Parser

- **Context**: The KSF parser must read files. Testing the parser requires providing test data without writing to the filesystem.
- **Decision**: `KsfParser` accepts an injectable `FileReaderFn` (returns file contents as string), defaulting to `std::filesystem` file reading. Tests inject lambdas that return hardcoded strings.
- **Alternatives considered**: (a) Accept `std::istream&` -- works but awkward for tests (must construct `std::istringstream`). (b) Accept `std::string_view` of file contents -- simpler but loses the file path, which is needed to resolve relative audio paths. (c) No injection, test with real fixture files -- slower, filesystem-dependent, harder to test edge cases.
- **Consequences**: Consistent with the project's injection pattern (`Clock::CounterFn`, `TextureCache::ImageLoaderFn`, `KeyboardDriver::KeyboardStateFn`). The parser is fully testable without filesystem access. Real fixture files are used for regression tests only.

### ADR-4: Segment-Based Timing Lookup (Internal Implementation)

- **Context**: `time_at_beat` and `beat_at_time` need O(log n) performance. BPM changes and stops interact in complex ways (stops at BPM change boundaries, multiple stops at the same beat).
- **Decision**: Internally, `TimingData` compiles the raw events into a flat list of "segments" where each segment has a start beat, start time, and rate. Binary search on this list gives O(log n) lookup with O(1) computation per query.
- **Alternatives considered**: (a) Store raw events and scan linearly -- O(n) per query, fails the 100-event performance scenario. (b) Two separate vectors (BPM changes + stops) with parallel binary searches -- correct but requires merging results from two searches, more complex. (c) Cumulative offsets on raw events -- works for `time_at_beat` but `beat_at_time` during stops requires special handling that cumulative offsets don't naturally provide.
- **Consequences**: The public API is clean (`time_at_beat`, `beat_at_time`). The internal segment list is an implementation detail in `timing_data.cpp`. Future timing event types (warps, delays in Phase 4) can be added by extending the segment builder without changing the lookup algorithm.

### ADR-5: NoteData Stores Pre-Sorted Vector (Not Self-Sorting Container)

- **Context**: Notes need to be sorted by beat for efficient scanning. Should the container maintain sort order on insertion (like `std::multiset`) or sort once after all insertions?
- **Decision**: `NoteData` accepts a pre-sorted `std::vector<NoteEvent>`. The `ChartBuilder` sorts the vector in `build()`.
- **Alternatives considered**: (a) `std::multiset<NoteEvent>` -- maintains order on insert but has per-element heap allocation, poor cache locality, and no contiguous memory for iterator-pair range returns. (b) Insertion-sorted vector -- O(n^2) for n inserts due to shifting elements.
- **Consequences**: One O(n log n) sort at load time. After that, all access is O(log n) range queries on a contiguous, cache-friendly vector. This matches the existing `std::vector<Keyframe>` pattern in the BGA system.

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| KSF format variations across Kick It Up versions (TICKCOUNT values, encoding quirks) | Med | Med | Start with the most common format. Log warnings for unrecognized tags. Add format quirk handling as test fixtures from real game data are collected. |
| Floating-point comparison issues in beat_at_time during stop boundaries | Med | Med | Use epsilon-based comparisons (1e-9) at segment boundaries. Unit test every boundary condition explicitly. |
| ChartBuilder validation too strict -- rejects charts that real games accept | Low | Med | Validation only throws for truly fatal issues (no BPM). Everything else is a logged warning. Mirrors the engine's "graceful degradation" philosophy from CLAUDE.md. |
| UTF-8 encoding issues in KSF metadata (files may use EUC-KR for Korean titles) | Med | High | Phase 1: store raw bytes as std::string. Phase 4: add encoding detection/conversion. Log a warning if non-UTF-8 bytes are detected. |
| TimingSegment model doesn't handle warps/delays needed in Phase 4 | Low | Low | Warps and delays are additional segment types. The segment model is extensible -- add new segment types without changing the binary search. |

## Testing Strategy

### Unit Tests (`test/test_chart.cpp`) -- Pure Logic, No SDL

All chart data model tests are pure C++ with no SDL dependency.

**NoteType Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `AllNoteTypesExist` | TAP, HOLD_HEAD, HOLD_TAIL, MINE, FAKE, LIFT defined | US-CHT-001 SC3 |
| `NoteTypeStringRoundTrip` | to_string/from_string are inverse for all types | -- |

**PlayMode Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `SingleModeHas5Columns` | max_columns(SINGLE) == 5 | US-CHT-004 SC3 |
| `DoubleModeHas10Columns` | max_columns(DOUBLE) == 10 | US-CHT-004 SC4 |
| `PlayModeStringRoundTrip` | to_string/from_string are inverse | -- |

**NoteEvent Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `NoteEventFieldsAccessible` | beat, column, type fields correct after construction | US-CHT-004 SC1 |
| `NoteEventSortsByBeat` | sort([8.0, 2.5, 4.0]) == [2.5, 4.0, 8.0] | US-CHT-004 SC2 |
| `NoteEventSortsByColumnThenType` | notes at same beat sort by column, then type | -- |

**NoteData Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `NoteDataSize` | size() returns correct count | US-CHT-004 |
| `NoteDataEmpty` | empty() on empty NoteData | -- |
| `NotesInRangeReturnsSubset` | notes_in_range(2.0, 5.0) returns correct notes | US-CHT-004 SC2 |
| `NotesInRangeEmptyForNoMatch` | range with no notes returns empty pair | -- |
| `CountByType` | count_by_type(TAP) returns correct count | -- |

**ChartMetadata Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `DefaultMetadataHasEmptyStrings` | title, artist, genre default to "" | US-CHT-002 SC3 |
| `Utf8StringsPreserved` | Japanese characters survive round-trip | US-CHT-002 SC2 |
| `OptionalFieldsHaveSafeDefaults` | difficulty_rating defaults to 0, paths default to "" | US-CHT-002 SC3 |

**Chart Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `ChartExposesAllComponents` | metadata(), timing_data(), note_data() all accessible | US-CHT-001 SC1 |
| `ChartComponentsAreConst` | returned references are const | US-CHT-001 SC2 |
| `ChartDurationSeconds` | duration_seconds() matches time at last note | -- |
| `ChartNoteCount` | note_count() matches note_data().size() | -- |

**ChartBuilder Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `BuilderProducesValidChart` | set metadata + add notes + build() -> valid Chart | US-CHT-001 |
| `BuilderSortsNotes` | notes added out of order are sorted in built Chart | US-CHT-004 SC2 |
| `BuilderSortsTimingEvents` | BPM events added out of order are sorted | US-CHT-003 |
| `BuilderThrowsOnNoBpm` | build() with no BPM event -> ChartLoadException | US-CHT-005 SC4 |
| `BuilderWarnsOnInvalidColumn` | column > max_columns logs warning | US-CHT-004 SC5 |
| `BuilderDefaultBpm` | if no explicit BPM, default 120 is used | -- |

### Unit Tests (`test/test_timing_data.cpp`) -- Timing Conversion Tests

These tests verify the mathematical correctness of `time_at_beat` and `beat_at_time`.

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `SingleBpmTimeAtBeat` | 120 BPM: time_at_beat(4.0) == 2.0 | US-CHT-003 SC1 |
| `SingleBpmBeatAtTime` | 120 BPM: beat_at_time(2.0) == 4.0 | US-CHT-003 SC2 |
| `BpmChangeTimeAtBeat` | 120->180 at beat 8: time_at_beat(12.0) ~= 5.333 | US-CHT-003 SC3 |
| `BpmChangeBeatAtTime` | 120->180: beat_at_time(5.333) ~= 12.0 | US-CHT-003 SC3 |
| `StopExtendsTime` | 120 BPM + 1s stop at beat 4: time_at_beat(5.0) == 3.0 | US-CHT-003 SC4 |
| `StopBeatAtTime` | beat_at_time during stop returns stop's beat | US-CHT-003 SC4 |
| `BeatAtTimeBeforeStop` | beat_at_time just before stop returns correct beat | -- |
| `BeatAtTimeAfterStop` | beat_at_time after stop returns correct beat | -- |
| `MultipleBpmChanges` | 3+ BPM changes, verify time at various beats | -- |
| `MultipleStops` | 2+ stops, verify cumulative time includes all | -- |
| `StopAtBpmChange` | stop at same beat as BPM change | -- |
| `ZeroBeat` | time_at_beat(0.0) == 0.0 | -- |
| `NegativeBeat` | time_at_beat(-1.0) == 0.0 (clamped) | -- |
| `NegativeTime` | beat_at_time(-1.0) == 0.0 (clamped) | -- |
| `BpmAtBeat` | bpm_at_beat returns correct BPM for given position | -- |
| `RoundTrip` | time_at_beat(beat_at_time(t)) ~= t for various t | -- |
| `PerformanceWith100Events` | 100 BPM events, 10k queries in < 10ms | US-CHT-003 SC5 |
| `DefaultTimingDataIs120Bpm` | Default TimingData has 120 BPM at beat 0 | -- |
| `PrecisionAfterLongChart` | time_at_beat at beat 10000 within 0.1ms of exact | -- |

### Unit Tests (`test/test_ksf_parser.cpp`) -- KSF Parser Tests

All tests use injectable file reader (no filesystem access).

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `ValidKsfParsed` | Basic .ksf with metadata and notes -> valid Chart | US-CHT-005 SC1 |
| `MetadataExtracted` | TITLE, ARTIST lines -> correct metadata fields | US-CHT-005 SC2 |
| `NotePositionsConvertedToBeats` | tick 0, 48, 96 -> beat 0.0, 0.25, 0.5 | US-CHT-005 SC3 |
| `BpmExtracted` | #BPM:140 -> timing_data with 140 BPM | US-CHT-005 SC1 |
| `AudioFilenameResolved` | #AUDIOFILE:song.ogg in /data/Song/ -> /data/Song/song.ogg | US-CHT-005 SC6 |
| `MalformedKsfThrows` | Truncated note section -> ChartLoadException | US-CHT-005 SC4 |
| `MissingTitleLogsWarning` | No TITLE line -> warning logged, chart still loads | US-CHT-005 SC5 |
| `EmptyFileThrows` | Empty file -> ChartLoadException | -- |
| `OnlyMetadataNoNotes` | Metadata but no note data -> ChartLoadException | -- |
| `HoldNotesConverted` | '4' in KSF -> HOLD_HEAD note | -- |
| `MissingBpmThrows` | No #BPM line -> ChartLoadException | -- |
| `MultipleBpmNotSupported` | KSF with single BPM only (format limitation) | -- |

### Fixture File (`test/fixtures/test_basic.ksf`)

A minimal committed KSF fixture for integration/regression testing:

```
#TITLE:Test Song;
#ARTIST:Test Artist;
#BPM:120;
#TICKCOUNT:2;
#AUDIOFILE:test.ogg;
10000
00000
01000
00000
00010
00000
00100
00000
2222222222
```

---

*Generated from stories in docs/stories/04-chart-system.md (Phase 1 subset)*
*Last updated: 2026-04-28*
