# IP-CHT-001: Chart System Phase 1 Implementation Plan

**Design**: TD-CHT-001
**Stories**: US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004, US-CHT-005
**Total Steps**: 7
**Estimated Total**: ~5.5 hours
**Prerequisite**: IP-ENG-001 (Engine class exists, but chart system does not depend on it at runtime -- it is pure data)
**Author**: technical-lead agent
**Status**: Draft

## Step 1: Create NoteType and PlayMode Enums

**Files**:
- Create `src/openitup/chart/note_type.h` -- NoteType enum with string conversion declarations
- Create `src/openitup/chart/note_type.cpp` -- String conversion implementations
- Create `src/openitup/chart/play_mode.h` -- PlayMode enum with max_columns() and string conversions
- Create `src/openitup/chart/play_mode.cpp` -- String conversion implementations
- Modify `CMakeLists.txt` -- Add both .cpp files to `openitup_engine` sources

**What to implement**:

Two small enum types that all other chart components depend on. Follow the `BlendEffect` pattern from `src/openitup/bga/keyframe.h`: scoped `enum class` with free-function `to_string` / `from_string` helpers.

`NoteType` values (US-CHT-001 Scenario 3):
```cpp
enum class NoteType : uint8_t {
    TAP = 0, HOLD_HEAD = 1, HOLD_TAIL = 2,
    MINE = 3, FAKE = 4, LIFT = 5,
};
```

`PlayMode` values:
```cpp
enum class PlayMode : uint8_t {
    SINGLE = 0,  // 5 panels, columns 0-4
    DOUBLE = 1,  // 10 panels, columns 0-9
};

inline constexpr int max_columns(PlayMode mode) {
    return mode == PlayMode::DOUBLE ? 10 : 5;
}
```

Both enums get `to_string` and `from_string` conversions implemented in the .cpp files.

**Tests**:
- Create `test/test_chart.cpp` -- Initial test file for chart data model
- Modify `CMakeLists.txt` -- Add `test/test_chart.cpp` to `openitup_tests`

Test cases:
- `AllNoteTypesExist` -- All 6 NoteType values (TAP, HOLD_HEAD, HOLD_TAIL, MINE, FAKE, LIFT) are defined and distinct
- `NoteTypeStringRoundTrip` -- `note_type_from_string(note_type_to_string(t)) == t` for all types
- `SingleModeHas5Columns` -- `max_columns(PlayMode::SINGLE) == 5`
- `DoubleModeHas10Columns` -- `max_columns(PlayMode::DOUBLE) == 10`
- `PlayModeStringRoundTrip` -- `play_mode_from_string(play_mode_to_string(m)) == m` for both modes

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Chart` passes all enum tests
- [ ] No SDL dependency in any of the new files

**Expected commit message**:
`feat(chart): add NoteType and PlayMode enums with string conversions`

**Estimated time**: ~30 minutes

---

## Step 2: Create ChartMetadata and NoteEvent/NoteData

**Files**:
- Create `src/openitup/chart/chart_metadata.h` -- ChartMetadata struct (header-only)
- Create `src/openitup/chart/note_data.h` -- NoteEvent struct and NoteData class declaration
- Create `src/openitup/chart/note_data.cpp` -- NoteData implementation (constructor, range queries, count_by_type)
- Modify `CMakeLists.txt` -- Add `note_data.cpp` to `openitup_engine`

**What to implement**:

`ChartMetadata` is a plain struct with string fields (title, artist, genre, charter_name, difficulty_name, audio_path, banner_path, background_path), numeric fields (difficulty_rating, display_bpm), play mode, and preview fields (preview_start_seconds, preview_length_seconds with -1.0 sentinel defaults). Header-only, no .cpp.

`NoteEvent` is a plain struct with `double beat`, `uint8_t column`, `NoteType type`, and comparison operators (`operator<` sorts by beat, then column, then type; `operator==` for equality).

`NoteData` wraps `std::vector<NoteEvent>`:
- Constructor takes a pre-sorted vector via move
- `size()`, `empty()`, `events()` accessors
- `notes_in_range(double lo_beat, double hi_beat)` returns an iterator pair using `std::lower_bound`
- `count_by_type(NoteType)` scans the vector with `std::count_if`

The range query uses `std::lower_bound` with a comparator that compares by beat position:
```cpp
auto lo_it = std::lower_bound(events_.begin(), events_.end(), lo_beat,
    [](const NoteEvent& e, double b) { return e.beat < b; });
auto hi_it = std::lower_bound(events_.begin(), events_.end(), hi_beat,
    [](const NoteEvent& e, double b) { return e.beat < b; });
return {lo_it, hi_it};
```

**Tests**:
- Add to `test/test_chart.cpp`:

Test cases:
- `NoteEventFieldsAccessible` -- Construct NoteEvent{4.5, 2, TAP}, verify all fields
- `NoteEventSortsByBeat` -- Sort vector of [8.0, 2.5, 4.0] -> [2.5, 4.0, 8.0]
- `NoteEventSortsByColumnThenType` -- Notes at same beat sort by column then type
- `NoteDataSize` -- NoteData with 3 events has size() == 3
- `NoteDataEmpty` -- Default NoteData (empty vector) has empty() == true
- `NotesInRangeReturnsSubset` -- Range [2.0, 5.0) on [0.0, 2.5, 4.0, 8.0] returns [2.5, 4.0]
- `NotesInRangeEmptyForNoMatch` -- Range [5.0, 7.0) on same data returns empty
- `CountByType` -- 3 TAP + 2 HOLD_HEAD -> count_by_type(TAP) == 3
- `DefaultMetadataHasEmptyStrings` -- Default ChartMetadata has title == "", artist == ""
- `Utf8StringsPreserved` -- Japanese string "\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88" survives round-trip
- `OptionalFieldsHaveSafeDefaults` -- difficulty_rating == 0, paths == "", preview values == -1.0

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Chart` passes all tests
- [ ] No SDL dependency

**Expected commit message**:
`feat(chart): add ChartMetadata, NoteEvent, and NoteData with range queries`

**Estimated time**: ~45 minutes

---

## Step 3: Create TimingData with Beat/Time Conversion

**Files**:
- Create `src/openitup/chart/timing_data.h` -- TimingEvent struct, TimingData class declaration
- Create `src/openitup/chart/timing_data.cpp` -- Segment builder, time_at_beat, beat_at_time
- Modify `CMakeLists.txt` -- Add `timing_data.cpp` to `openitup_engine`

**What to implement**:

This is the most complex step (US-CHT-003, 5 story points). Follow the sorted-vector + `std::upper_bound` pattern from `evaluate_keyframes()` in `src/openitup/bga/keyframe.h`.

`TimingEvent` struct with: `double beat`, `TimingEventType type` (BPM_CHANGE or STOP), `double bpm`, `double stop_duration`. Plus pre-computed fields used internally: `double cumulative_time`, `double seconds_per_beat`. Comparison operator sorts by (beat, type).

`TimingData` class:
- Constructor accepts `std::vector<TimingEvent>`, sorts them, calls `build_segments()` to pre-compute cumulative time offsets
- Default constructor creates a single 120 BPM event at beat 0
- `time_at_beat(double beat) -> double` -- O(log n) binary search
- `beat_at_time(double time) -> double` -- O(log n) binary search on cumulative_time
- `bpm_at_beat(double beat) -> double` -- finds active BPM at given beat
- Accessors: `events()`, `size()`, `empty()`

Internal segment model (defined in .cpp, not header):
```cpp
struct TimingSegment {
    double start_beat;
    double start_time;
    double seconds_per_beat;  // 60.0/bpm, 0 for stops
    double stop_duration;     // >0 only for stops
    bool is_stop;
};
```

Key algorithm: `build_segments()` walks the sorted events and builds the segment list:
1. For each BPM_CHANGE: compute time delta from previous beat at previous BPM, push a new BPM segment
2. For each STOP: push a stop segment with duration, advance cumulative time by stop_duration

`time_at_beat(beat)`:
1. `std::upper_bound` on segments by `start_beat` to find first segment after query beat
2. Step back one to get the active segment
3. If active segment is a BPM segment: `return start_time + (beat - start_beat) * seconds_per_beat`
4. Handle stops at the exact query beat: include stop durations
5. Clamp negative beats to 0.0

`beat_at_time(time)`:
1. `std::upper_bound` on segments by `start_time` to find first segment with start_time > query time
2. Step back one to get the active segment
3. If it's a stop and time falls within the stop window: return the stop's beat
4. Otherwise: `return start_beat + (time - start_time) / seconds_per_beat`
5. Clamp negative time to 0.0

**Tests**:
- Create `test/test_timing_data.cpp`
- Modify `CMakeLists.txt` -- Add `test/test_timing_data.cpp` to `openitup_tests`

Test cases (from TD-CHT-001 testing strategy):
- `SingleBpmTimeAtBeat` -- 120 BPM: time_at_beat(4.0) == 2.0
- `SingleBpmBeatAtTime` -- 120 BPM: beat_at_time(2.0) == 4.0
- `BpmChangeTimeAtBeat` -- 120->180 at beat 8: time_at_beat(12.0) within 0.0001 of 5.333
- `BpmChangeBeatAtTime` -- 120->180: beat_at_time(5.333) within 0.0001 of 12.0
- `StopExtendsTime` -- 120 BPM + 1s stop at beat 4: time_at_beat(5.0) == 3.0
- `StopBeatAtTime` -- beat_at_time during stop returns stop's beat (4.0)
- `BeatAtTimeBeforeStop` -- time just before stop boundary
- `BeatAtTimeAfterStop` -- time after stop returns correct beat
- `MultipleBpmChanges` -- 3 BPM changes, verify at several points
- `MultipleStops` -- 2 stops, verify cumulative time includes both
- `StopAtBpmChange` -- stop at same beat as BPM change
- `ZeroBeat` -- time_at_beat(0.0) == 0.0
- `NegativeBeat` -- time_at_beat(-1.0) == 0.0
- `NegativeTime` -- beat_at_time(-1.0) == 0.0
- `BpmAtBeat` -- returns correct BPM at various positions
- `RoundTrip` -- time_at_beat(beat_at_time(t)) ~= t for various t values
- `PerformanceWith100Events` -- 100 BPM changes, 10k queries < 10ms
- `DefaultTimingDataIs120Bpm` -- Default constructor has 120 BPM
- `PrecisionAfterLongChart` -- beat 10000 within 0.1ms of exact value

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R TimingData` passes all timing tests
- [ ] Performance test completes within 10ms for 10k queries on 100 events
- [ ] No SDL dependency

**Expected commit message**:
`feat(chart): add TimingData with O(log n) beat/time conversion and stop support`

**Estimated time**: ~75 minutes

---

## Step 4: Create Chart and ChartBuilder

**Files**:
- Create `src/openitup/chart/chart.h` -- Chart class declaration
- Create `src/openitup/chart/chart.cpp` -- Chart implementation (accessors, duration_seconds)
- Create `src/openitup/chart/chart_builder.h` -- ChartBuilder class + ChartLoadException
- Create `src/openitup/chart/chart_builder.cpp` -- Builder implementation (setters, add methods, build with sort/validate)
- Modify `CMakeLists.txt` -- Add `chart.cpp` and `chart_builder.cpp` to `openitup_engine`

**What to implement**:

`Chart` class:
- Constructor accepts `ChartMetadata`, `TimingData`, `NoteData` via move
- Const accessors: `metadata()`, `timing_data()`, `note_data()`
- `duration_seconds()`: calls `timing_data_.time_at_beat(last_note_beat)`
- `note_count()`: returns `note_data_.size()`
- No default constructor (Chart must be fully constructed)

`ChartLoadException`:
- Extends `std::runtime_error`
- Used by builder and parsers for fatal errors

`ChartBuilder` class:
- Default constructor initializes empty metadata, empty event vectors
- Metadata setters: `set_title`, `set_artist`, `set_genre`, `set_charter_name`, `set_difficulty_name`, `set_difficulty_rating`, `set_mode`, `set_audio_path`, `set_banner_path`, `set_background_path`, `set_display_bpm`
- Timing: `add_bpm_change(beat, bpm)`, `add_stop(beat, duration)`
- Notes: `add_note(beat, column, type)`
- `build()`:
  1. If no BPM events exist, add default 120 BPM at beat 0, log warning
  2. Sort timing events
  3. Validate: negative BPM -> throw ChartLoadException
  4. Sort note events
  5. Validate columns against declared mode: log warning for out-of-range (do not throw)
  6. Construct and return `Chart(metadata_, TimingData(timing_events_), NoteData(note_events_))`

**Tests**:
- Add to `test/test_chart.cpp`:

Test cases:
- `ChartExposesAllComponents` -- metadata(), timing_data(), note_data() all accessible
- `ChartComponentsAreConst` -- Verify const references (compile-time, implicit)
- `ChartDurationSeconds` -- Chart with notes at beat 4 at 120 BPM has duration ~2.0s
- `ChartNoteCount` -- Chart with 5 notes has note_count() == 5
- `BuilderProducesValidChart` -- Full builder flow produces accessible Chart
- `BuilderSortsNotes` -- Notes added [8.0, 2.5, 4.0] are sorted in built Chart
- `BuilderSortsTimingEvents` -- BPM events added out of order are sorted
- `BuilderThrowsOnNegativeBpm` -- add_bpm_change(0, -120) -> ChartLoadException on build()
- `BuilderWarnsOnInvalidColumn` -- Column 7 in single mode -> logs warning, chart still builds
- `BuilderDefaultBpmWhenNoneAdded` -- No BPM events -> default 120 BPM, logs warning
- `BuilderMoveSemantics` -- Build with many notes, verify no copy overhead (implicit)

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Chart` passes all tests
- [ ] ChartBuilder produces immutable Chart with sorted data
- [ ] ChartLoadException thrown for fatal issues, warnings logged for non-fatal

**Expected commit message**:
`feat(chart): add Chart and ChartBuilder with validation and sorting`

**Estimated time**: ~45 minutes

---

## Step 5: Implement KSF Parser

**Files**:
- Create `src/openitup/chart/ksf_parser.h` -- KsfParser class declaration with injectable FileReaderFn
- Create `src/openitup/chart/ksf_parser.cpp` -- KSF parsing implementation
- Modify `CMakeLists.txt` -- Add `ksf_parser.cpp` to `openitup_engine`

**What to implement**:

`FileReaderFn` type alias: `std::function<std::string(const std::filesystem::path&)>`. Default implementation reads file to string using `std::ifstream`.

`KsfParser` class:
- Default constructor: uses filesystem reader
- Injectable constructor: accepts custom `FileReaderFn`
- `parse(const std::filesystem::path& chart_path) const -> Chart`

KSF format parsing logic:

1. Read file contents via `file_reader_`
2. Split into lines
3. Parse metadata tags (lines matching `#TAG:VALUE;`):
   - `#TITLE:` -> `builder.set_title(value)`
   - `#ARTIST:` -> `builder.set_artist(value)`
   - `#BPM:` -> `builder.add_bpm_change(0.0, std::stod(value))` and `builder.set_display_bpm(bpm)`
   - `#TICKCOUNT:` -> store ticks_per_beat (used for note timing)
   - `#AUDIOFILE:` -> resolve relative to chart_path parent directory, `builder.set_audio_path(resolved)`
   - `#DIFFICULTY:` -> `builder.set_difficulty_name(value)`
   - `#STEP:` -> marks beginning of note data section
   - Other tags: log debug message, skip

4. Parse note data section (after #STEP or first line of 5+ digit characters):
   - Each line is 5 characters for single mode (columns 0-4)
   - Character meanings: `0` = empty, `1` = tap, `4` = hold head, `2` = end of data marker
   - Track current tick position, increment per line
   - Convert ticks to beats: `beat = tick / ticks_per_beat`
   - Default ticks_per_beat depends on TICKCOUNT (commonly: TICKCOUNT=2 means 4 lines per beat)
   - The exact tick resolution: KSF typically uses 1 line = 1/4 beat when TICKCOUNT is 2, meaning 4 lines per beat. But this varies. Implement the most common convention and log a warning if TICKCOUNT is unusual.

5. Handle hold notes:
   - `4` starts a hold at a column. Track the hold's start beat.
   - Subsequent `1` at the same column while a hold is open -> hold body (ignore)
   - `0` after a hold was opened -> `builder.add_note(beat, column, HOLD_TAIL)` to end the hold

6. End of data: line starting with `2` marks end of step data

7. Set mode: `builder.set_mode(PlayMode::SINGLE)` (KSF is always single/5-panel)

8. Call `builder.build()` and return the Chart

**Audio path resolution** (US-CHT-005 Scenario 6):
```cpp
auto chart_dir = chart_path.parent_path();
auto audio_resolved = chart_dir / audio_filename;
builder.set_audio_path(audio_resolved.string());
```

**Error handling**:
- Empty file -> throw ChartLoadException("KSF file is empty")
- No note data -> throw ChartLoadException("KSF file has no note data")
- No BPM -> ChartLoadException (caught by builder.build())
- Missing TITLE -> log warning, use filename as fallback title
- Malformed tag line -> log warning, skip line
- Truncated note data -> throw ChartLoadException("Unexpected end of note data")

**Tests**:
- Create `test/test_ksf_parser.cpp`
- Modify `CMakeLists.txt` -- Add `test/test_ksf_parser.cpp` to `openitup_tests`

All tests inject a `FileReaderFn` lambda that returns hardcoded KSF content:

Test cases:
- `ValidKsfParsed` -- Basic KSF with TITLE, ARTIST, BPM 140, 50 tap notes -> Chart with 140 BPM and 50 notes
- `MetadataExtracted` -- TITLE and ARTIST lines match exact strings in Chart metadata
- `NotePositionsConvertedToBeats` -- Notes at specific ticks map to expected beat values (tick 0 -> beat 0.0, etc.)
- `BpmExtracted` -- #BPM:140 -> timing_data with 140 BPM, time_at_beat(4.0) == 60/140 * 4
- `AudioFilenameResolved` -- #AUDIOFILE:song.ogg with chart at /data/Song/chart.ksf -> audio_path == "/data/Song/song.ogg"
- `MalformedKsfThrows` -- Truncated note section -> ChartLoadException
- `MissingTitleLogsWarning` -- No TITLE line -> chart loads with fallback title
- `EmptyFileThrows` -- Empty string -> ChartLoadException
- `OnlyMetadataNoNotes` -- Metadata but no step data -> ChartLoadException
- `HoldNotesConverted` -- '4' in step data -> HOLD_HEAD, subsequent '0' -> HOLD_TAIL
- `MissingBpmThrows` -- No #BPM tag -> ChartLoadException
- `AlwaysSingleMode` -- Every KSF chart has mode == PlayMode::SINGLE
- `TickCountAffectsBeatPositions` -- TICKCOUNT=2 -> 4 lines per beat

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R KsfParser` passes all parser tests
- [ ] No filesystem access in tests (injectable file reader)
- [ ] Audio path correctly resolved relative to chart directory

**Expected commit message**:
`feat(chart): add KSF parser with metadata extraction and audio path resolution`

**Estimated time**: ~75 minutes

---

## Step 6: Add KSF Fixture File and Regression Test

**Files**:
- Create `test/fixtures/test_basic.ksf` -- Minimal committed KSF fixture
- Modify `test/test_ksf_parser.cpp` -- Add fixture-based regression test

**What to implement**:

Create a minimal but complete KSF fixture file at `test/fixtures/test_basic.ksf`:

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

This fixture has:
- Known metadata (title, artist, BPM)
- 4 tap notes at predictable beat positions
- A known end-of-data marker
- Audio filename to verify path resolution

Add a regression test that loads the actual fixture file (using the default filesystem reader, not the injectable one):

```cpp
TEST(KsfParserRegression, FixtureFileLoadedCorrectly) {
    KsfParser parser;
    auto chart = parser.parse("test/fixtures/test_basic.ksf");
    // or use a path relative to the test binary location

    EXPECT_EQ(chart.metadata().title, "Test Song");
    EXPECT_EQ(chart.metadata().artist, "Test Artist");
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 120.0);
    EXPECT_EQ(chart.note_count(), 4);
    EXPECT_EQ(chart.metadata().mode, PlayMode::SINGLE);
}
```

This test uses the real filesystem and proves the full pipeline works end-to-end. It complements the unit tests in Step 5 which test individual parser behaviors with injected data.

**Tests**:
- `FixtureFileLoadedCorrectly` -- Load test_basic.ksf, verify metadata and note count
- `FixtureNoteBeats` -- Verify exact beat positions of the 4 notes
- `FixtureTiming` -- Verify time_at_beat matches expected values at 120 BPM

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R KsfParser` passes all tests including fixture test
- [ ] Fixture file committed to test/fixtures/

**Expected commit message**:
`test(chart): add KSF fixture file and regression test for end-to-end parsing`

**Estimated time**: ~30 minutes

---

## Step 7: Wire Chart Sources into Build and Final Verification

**Files**:
- Verify `CMakeLists.txt` -- All source files and test files are present (should already be done incrementally)

**What to implement**:

This step is a verification pass. All source files should already have been added to CMakeLists.txt in the steps above. Verify the final state:

Sources added to `openitup_engine`:
```cmake
src/openitup/chart/note_type.cpp
src/openitup/chart/play_mode.cpp
src/openitup/chart/note_data.cpp
src/openitup/chart/timing_data.cpp
src/openitup/chart/chart.cpp
src/openitup/chart/chart_builder.cpp
src/openitup/chart/ksf_parser.cpp
```

Tests added to `openitup_tests`:
```cmake
test/test_chart.cpp
test/test_timing_data.cpp
test/test_ksf_parser.cpp
```

Run the full test suite to confirm no regressions:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DSDL_X11_XSCRNSAVER=OFF
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

Verify:
1. All new chart tests pass
2. All existing tests still pass (BGA, sprite, texture cache, keyframe, integration, regression)
3. No new compiler warnings

**Definition of done**:
- [ ] Full test suite passes with zero failures
- [ ] No new compiler warnings
- [ ] All 7 new chart source files compiled into openitup_engine
- [ ] All 3 new test files compiled into openitup_tests
- [ ] KSF fixture file present in test/fixtures/

**Expected commit message**:
`chore(chart): verify full chart system build and all tests pass`

**Estimated time**: ~15 minutes

---

## PR Strategy

- [ ] **Single PR recommended** -- All 7 steps build the chart data model from primitives to parser. Splitting would create intermediate states where types exist without the parser that populates them.
- [ ] **Review checkpoint**: After Step 4 (Chart + ChartBuilder), before Step 5 (KSF parser). This validates the data model before the parser is written.
- [ ] **Low risk**: The chart system is pure data with no SDL dependency and no modification to existing code. It cannot break existing tests.

Alternative: Split into two PRs:
1. Steps 1-4: Data model (NoteType, PlayMode, NoteData, TimingData, Chart, ChartBuilder)
2. Steps 5-7: KSF parser and fixture

This allows reviewing the data model in isolation before the parsing logic.

## Build Verification

After all steps complete:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DSDL_X11_XSCRNSAVER=OFF
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

All tests should pass including:
- `test_chart.cpp`: NoteType, PlayMode, NoteEvent, NoteData, ChartMetadata, Chart, ChartBuilder tests
- `test_timing_data.cpp`: TimingData conversion tests
- `test_ksf_parser.cpp`: KSF parser tests including fixture regression

Existing tests should be unaffected:
- `test_keyframe_interp.cpp`
- `test_texture_cache.cpp`
- `test_sprite_modes.cpp`
- `test_integration.cpp`
- `test_regression.cpp`

## Acceptance Verification

| Story ID | How to verify |
|----------|--------------|
| US-CHT-001 | `test/test_chart.cpp`: `ChartExposesAllComponents`, `ChartComponentsAreConst`, `AllNoteTypesExist` |
| US-CHT-002 | `test/test_chart.cpp`: `DefaultMetadataHasEmptyStrings`, `Utf8StringsPreserved`, `OptionalFieldsHaveSafeDefaults` |
| US-CHT-003 | `test/test_timing_data.cpp`: `SingleBpmTimeAtBeat`, `SingleBpmBeatAtTime`, `BpmChangeTimeAtBeat`, `StopExtendsTime`, `PerformanceWith100Events` |
| US-CHT-004 | `test/test_chart.cpp`: `NoteEventFieldsAccessible`, `NoteEventSortsByBeat`, `NotesInRangeReturnsSubset`, `SingleModeHas5Columns`, `DoubleModeHas10Columns` |
| US-CHT-005 | `test/test_ksf_parser.cpp`: `ValidKsfParsed`, `MetadataExtracted`, `NotePositionsConvertedToBeats`, `AudioFilenameResolved`, `MalformedKsfThrows`, `FixtureFileLoadedCorrectly` |

## Notes

**Why the chart system has no SDL dependency**: Every component is pure C++ math and data structures. This means all tests run headless without Xvfb or display servers, all tests are fast (no SDL initialization overhead), and the chart system can be used by standalone tools (e.g., a future chart converter) without pulling in SDL.

**Why ChartBuilder exists instead of direct Chart construction**: Phase 4 adds 5+ more parsers (SSC, SMA, STX, SEE, NX). All parsers share the same finalization logic: sort notes, validate columns, build cumulative timing offsets. Centralizing this in ChartBuilder avoids duplicating it in every parser.

**Why the KSF fixture is a separate step**: The fixture file is committed to the repository and used for regression testing. It should be reviewed carefully to ensure the KSF format is correctly represented. Separating it from the parser implementation makes the review clearer.

**KSF format quirks to watch for**: The TICKCOUNT field has been interpreted differently across Kick It Up versions. The most common convention is TICKCOUNT=2 meaning 4 lines per beat (quarter note). Some files use TICKCOUNT=1 (2 lines per beat). The parser should handle common variations and log warnings for unusual values. Exact tick resolution can be refined as real game data is tested against the parser.

**Future extension**: Phase 4 (US-CHT-006 through US-CHT-019) adds more parsers and features. The data model designed here supports all of them. Warps and delays (SSC format) will be added as new `TimingEventType` values and segment types in `TimingData`. The `Chart`, `ChartBuilder`, and `NoteData` classes will not change.
