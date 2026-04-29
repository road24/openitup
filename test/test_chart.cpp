#include <gtest/gtest.h>

#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>
#include <openitup/chart/note_data.h>
#include <openitup/chart/chart_metadata.h>
#include <openitup/chart/chart.h>
#include <openitup/chart/chart_builder.h>

#include <algorithm>

using namespace openitup;

// --- NoteType Tests ---

TEST(Chart, AllNoteTypesExist) {
    // Verify all 6 note types are defined and distinct
    EXPECT_EQ(static_cast<uint8_t>(NoteType::TAP), 0);
    EXPECT_EQ(static_cast<uint8_t>(NoteType::HOLD_HEAD), 1);
    EXPECT_EQ(static_cast<uint8_t>(NoteType::HOLD_TAIL), 2);
    EXPECT_EQ(static_cast<uint8_t>(NoteType::MINE), 3);
    EXPECT_EQ(static_cast<uint8_t>(NoteType::FAKE), 4);
    EXPECT_EQ(static_cast<uint8_t>(NoteType::LIFT), 5);
}

TEST(Chart, NoteTypeStringRoundTrip) {
    // Verify string conversions are inverse for all types
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::TAP)), NoteType::TAP);
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::HOLD_HEAD)), NoteType::HOLD_HEAD);
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::HOLD_TAIL)), NoteType::HOLD_TAIL);
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::MINE)), NoteType::MINE);
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::FAKE)), NoteType::FAKE);
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::LIFT)), NoteType::LIFT);
}

// --- PlayMode Tests ---

TEST(Chart, SingleModeHas5Columns) {
    EXPECT_EQ(max_columns(PlayMode::SINGLE), 5);
}

TEST(Chart, DoubleModeHas10Columns) {
    EXPECT_EQ(max_columns(PlayMode::DOUBLE), 10);
}

TEST(Chart, PlayModeStringRoundTrip) {
    // Verify string conversions are inverse for both modes
    EXPECT_EQ(play_mode_from_string(play_mode_to_string(PlayMode::SINGLE)), PlayMode::SINGLE);
    EXPECT_EQ(play_mode_from_string(play_mode_to_string(PlayMode::DOUBLE)), PlayMode::DOUBLE);
}

// --- NoteEvent Tests ---

TEST(Chart, NoteEventFieldsAccessible) {
    // Construct NoteEvent{4.5, 2, TAP}, verify all fields
    NoteEvent event{4.5, 2, NoteType::TAP};
    EXPECT_DOUBLE_EQ(event.beat, 4.5);
    EXPECT_EQ(event.column, 2);
    EXPECT_EQ(event.type, NoteType::TAP);
}

TEST(Chart, NoteEventSortsByBeat) {
    // Sort vector of [8.0, 2.5, 4.0] -> [2.5, 4.0, 8.0]
    std::vector<NoteEvent> events = {
        {8.0, 0, NoteType::TAP},
        {2.5, 0, NoteType::TAP},
        {4.0, 0, NoteType::TAP}
    };
    std::sort(events.begin(), events.end());

    EXPECT_DOUBLE_EQ(events[0].beat, 2.5);
    EXPECT_DOUBLE_EQ(events[1].beat, 4.0);
    EXPECT_DOUBLE_EQ(events[2].beat, 8.0);
}

TEST(Chart, NoteEventSortsByColumnThenType) {
    // Notes at same beat sort by column then type
    std::vector<NoteEvent> events = {
        {4.0, 2, NoteType::TAP},
        {4.0, 0, NoteType::HOLD_HEAD},
        {4.0, 0, NoteType::TAP},
        {4.0, 1, NoteType::TAP}
    };
    std::sort(events.begin(), events.end());

    EXPECT_EQ(events[0].column, 0);
    EXPECT_EQ(events[0].type, NoteType::TAP);
    EXPECT_EQ(events[1].column, 0);
    EXPECT_EQ(events[1].type, NoteType::HOLD_HEAD);
    EXPECT_EQ(events[2].column, 1);
    EXPECT_EQ(events[2].type, NoteType::TAP);
    EXPECT_EQ(events[3].column, 2);
    EXPECT_EQ(events[3].type, NoteType::TAP);
}

// --- NoteData Tests ---

TEST(Chart, NoteDataSize) {
    // NoteData with 3 events has size() == 3
    std::vector<NoteEvent> events = {
        {0.0, 0, NoteType::TAP},
        {2.5, 1, NoteType::TAP},
        {4.0, 2, NoteType::TAP}
    };
    NoteData note_data(std::move(events));
    EXPECT_EQ(note_data.size(), 3);
}

TEST(Chart, NoteDataEmpty) {
    // Default NoteData (empty vector) has empty() == true
    NoteData note_data;
    EXPECT_TRUE(note_data.empty());
    EXPECT_EQ(note_data.size(), 0);
}

TEST(Chart, NotesInRangeReturnsSubset) {
    // Range [2.0, 5.0) on [0.0, 2.5, 4.0, 8.0] returns [2.5, 4.0]
    std::vector<NoteEvent> events = {
        {0.0, 0, NoteType::TAP},
        {2.5, 1, NoteType::TAP},
        {4.0, 2, NoteType::TAP},
        {8.0, 3, NoteType::TAP}
    };
    NoteData note_data(std::move(events));

    auto [begin, end] = note_data.notes_in_range(2.0, 5.0);
    EXPECT_EQ(std::distance(begin, end), 2);
    EXPECT_DOUBLE_EQ(begin->beat, 2.5);
    EXPECT_DOUBLE_EQ((begin + 1)->beat, 4.0);
}

TEST(Chart, NotesInRangeEmptyForNoMatch) {
    // Range [5.0, 7.0) on [0.0, 2.5, 4.0, 8.0] returns empty
    std::vector<NoteEvent> events = {
        {0.0, 0, NoteType::TAP},
        {2.5, 1, NoteType::TAP},
        {4.0, 2, NoteType::TAP},
        {8.0, 3, NoteType::TAP}
    };
    NoteData note_data(std::move(events));

    auto [begin, end] = note_data.notes_in_range(5.0, 7.0);
    EXPECT_EQ(begin, end);
    EXPECT_EQ(std::distance(begin, end), 0);
}

TEST(Chart, CountByType) {
    // 3 TAP + 2 HOLD_HEAD -> count_by_type(TAP) == 3
    std::vector<NoteEvent> events = {
        {0.0, 0, NoteType::TAP},
        {2.0, 1, NoteType::HOLD_HEAD},
        {4.0, 2, NoteType::TAP},
        {6.0, 3, NoteType::HOLD_HEAD},
        {8.0, 4, NoteType::TAP}
    };
    NoteData note_data(std::move(events));

    EXPECT_EQ(note_data.count_by_type(NoteType::TAP), 3);
    EXPECT_EQ(note_data.count_by_type(NoteType::HOLD_HEAD), 2);
    EXPECT_EQ(note_data.count_by_type(NoteType::MINE), 0);
}

// --- ChartMetadata Tests ---

TEST(Chart, DefaultMetadataHasEmptyStrings) {
    // Default ChartMetadata has title == "", artist == ""
    ChartMetadata metadata;
    EXPECT_EQ(metadata.title, "");
    EXPECT_EQ(metadata.artist, "");
    EXPECT_EQ(metadata.genre, "");
    EXPECT_EQ(metadata.charter_name, "");
    EXPECT_EQ(metadata.difficulty_name, "");
}

TEST(Chart, Utf8StringsPreserved) {
    // Japanese string "\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88" survives round-trip
    ChartMetadata metadata;
    std::string japanese_text = "\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88";  // "テスト"
    metadata.title = japanese_text;

    EXPECT_EQ(metadata.title, japanese_text);
    EXPECT_EQ(metadata.title.size(), 9);  // 3 characters, 3 bytes each
}

TEST(Chart, OptionalFieldsHaveSafeDefaults) {
    // difficulty_rating == 0, paths == "", preview values == -1.0
    ChartMetadata metadata;
    EXPECT_EQ(metadata.difficulty_rating, 0);
    EXPECT_EQ(metadata.audio_path, "");
    EXPECT_EQ(metadata.banner_path, "");
    EXPECT_EQ(metadata.background_path, "");
    EXPECT_DOUBLE_EQ(metadata.display_bpm, 0.0);
    EXPECT_DOUBLE_EQ(metadata.preview_start_seconds, -1.0);
    EXPECT_DOUBLE_EQ(metadata.preview_length_seconds, -1.0);
    EXPECT_EQ(metadata.mode, PlayMode::SINGLE);
}

// --- Chart Tests ---

TEST(Chart, ChartExposesAllComponents) {
    // metadata(), timing_data(), note_data() all accessible
    ChartBuilder builder;
    builder.set_title("Test Chart");
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(4.0, 1, NoteType::TAP);

    Chart chart = builder.build();

    EXPECT_EQ(chart.metadata().title, "Test Chart");
    EXPECT_EQ(chart.metadata().mode, PlayMode::SINGLE);
    EXPECT_EQ(chart.timing_data().size(), 1);
    EXPECT_EQ(chart.note_data().size(), 2);
}

TEST(Chart, ChartDurationSeconds) {
    // Chart with notes at beat 4 at 120 BPM has duration ~2.0s
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(4.0, 1, NoteType::TAP);

    Chart chart = builder.build();

    // 120 BPM = 2 beats per second
    // 4 beats = 2.0 seconds
    EXPECT_DOUBLE_EQ(chart.duration_seconds(), 2.0);
}

TEST(Chart, ChartNoteCount) {
    // Chart with 5 notes has note_count() == 5
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(1.0, 1, NoteType::TAP);
    builder.add_note(2.0, 2, NoteType::TAP);
    builder.add_note(3.0, 3, NoteType::TAP);
    builder.add_note(4.0, 4, NoteType::TAP);

    Chart chart = builder.build();

    EXPECT_EQ(chart.note_count(), 5);
}

// --- ChartBuilder Tests ---

TEST(Chart, BuilderProducesValidChart) {
    // Full builder flow produces accessible Chart
    ChartBuilder builder;
    builder.set_title("Test Song");
    builder.set_artist("Test Artist");
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 140.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(2.0, 2, NoteType::HOLD_HEAD);
    builder.add_note(4.0, 2, NoteType::HOLD_TAIL);

    Chart chart = builder.build();

    EXPECT_EQ(chart.metadata().title, "Test Song");
    EXPECT_EQ(chart.metadata().artist, "Test Artist");
    EXPECT_EQ(chart.note_count(), 3);
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 140.0);
}

TEST(Chart, BuilderSortsNotes) {
    // Notes added [8.0, 2.5, 4.0] are sorted in built Chart
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(8.0, 0, NoteType::TAP);
    builder.add_note(2.5, 1, NoteType::TAP);
    builder.add_note(4.0, 2, NoteType::TAP);

    Chart chart = builder.build();

    const auto& events = chart.note_data().events();
    EXPECT_DOUBLE_EQ(events[0].beat, 2.5);
    EXPECT_DOUBLE_EQ(events[1].beat, 4.0);
    EXPECT_DOUBLE_EQ(events[2].beat, 8.0);
}

TEST(Chart, BuilderSortsTimingEvents) {
    // BPM events added out of order are sorted
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(8.0, 180.0);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_bpm_change(4.0, 150.0);
    builder.add_note(0.0, 0, NoteType::TAP);

    Chart chart = builder.build();

    const auto& events = chart.timing_data().events();
    EXPECT_DOUBLE_EQ(events[0].beat, 0.0);
    EXPECT_DOUBLE_EQ(events[0].bpm, 120.0);
    EXPECT_DOUBLE_EQ(events[1].beat, 4.0);
    EXPECT_DOUBLE_EQ(events[1].bpm, 150.0);
    EXPECT_DOUBLE_EQ(events[2].beat, 8.0);
    EXPECT_DOUBLE_EQ(events[2].bpm, 180.0);
}

TEST(Chart, BuilderThrowsOnNegativeBpm) {
    // add_bpm_change(0, -120) -> ChartLoadException on build()
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, -120.0);
    builder.add_note(0.0, 0, NoteType::TAP);

    EXPECT_THROW(builder.build(), ChartLoadException);
}

TEST(Chart, BuilderWarnsOnInvalidColumn) {
    // Column 7 in single mode -> logs warning, chart still builds
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(2.0, 7, NoteType::TAP);  // Column 7 is out of range for single mode

    // Should not throw, just log warning
    Chart chart = builder.build();

    EXPECT_EQ(chart.note_count(), 2);
}

TEST(Chart, BuilderDefaultBpmWhenNoneAdded) {
    // No BPM events -> default 120 BPM, logs warning
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(4.0, 1, NoteType::TAP);

    Chart chart = builder.build();

    // Should have default 120 BPM
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 120.0);
    EXPECT_EQ(chart.timing_data().size(), 1);
}
