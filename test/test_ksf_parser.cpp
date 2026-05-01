#include <gtest/gtest.h>
#include <openitup/chart/ksf_parser.h>
#include <openitup/chart/chart_builder.h>
#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>

using namespace openitup;

namespace {

// Helper: Create a mock file reader that returns hardcoded content.
FileReaderFn mock_file_reader(std::string content) {
    return [content = std::move(content)](const std::filesystem::path&) {
        return content;
    };
}

// Minimal valid KSF content
const char* MINIMAL_KSF = R"(#TITLE:Test Song;
#ARTIST:Test Artist;
#BPM:140;
#TICKCOUNT:2;
#AUDIOFILE:test.ogg;
10000
00000
01000
00000
2222222222
)";

// KSF with hold notes
const char* HOLD_KSF = R"(#TITLE:Hold Test;
#ARTIST:Test;
#BPM:120;
#TICKCOUNT:2;
40000
10000
10000
00000
2222222222
)";

} // anonymous namespace

TEST(KsfParser, ValidKsfParsed) {
    KsfParser parser(mock_file_reader(MINIMAL_KSF));
    auto chart = parser.parse("/fake/path/test.ksf");

    EXPECT_EQ(chart.metadata().title, "Test Song");
    EXPECT_EQ(chart.metadata().artist, "Test Artist");
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 140.0);
    EXPECT_GT(chart.note_count(), 0);
    EXPECT_EQ(chart.metadata().mode, PlayMode::SINGLE);
}

TEST(KsfParser, MetadataExtracted) {
    KsfParser parser(mock_file_reader(MINIMAL_KSF));
    auto chart = parser.parse("/fake/path/test.ksf");

    EXPECT_EQ(chart.metadata().title, "Test Song");
    EXPECT_EQ(chart.metadata().artist, "Test Artist");
}

TEST(KsfParser, BpmExtracted) {
    KsfParser parser(mock_file_reader(MINIMAL_KSF));
    auto chart = parser.parse("/fake/path/test.ksf");

    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 140.0);
    EXPECT_DOUBLE_EQ(chart.metadata().display_bpm, 140.0);

    // Verify timing: at 140 BPM, 4 beats should take 60/140 * 4 = 1.714... seconds
    double expected_time = 60.0 / 140.0 * 4.0;
    EXPECT_NEAR(chart.timing_data().time_at_beat(4.0), expected_time, 0.001);
}

TEST(KsfParser, AudioFilenameResolved) {
    KsfParser parser(mock_file_reader(MINIMAL_KSF));
    auto chart = parser.parse("/data/Song/chart.ksf");

    // Audio file should be resolved relative to chart directory
    EXPECT_EQ(chart.metadata().audio_path, "/data/Song/test.ogg");
}

TEST(KsfParser, NotePositionsConvertedToBeats) {
    // TICKCOUNT=2 means 2 rows per beat.
    // Row 0: beat 0.0, Row 2: beat 1.0
    KsfParser parser(mock_file_reader(MINIMAL_KSF));
    auto chart = parser.parse("/fake/path/test.ksf");

    auto& notes = chart.note_data().events();
    ASSERT_GE(notes.size(), 2);

    // First note at row 0: beat 0.0
    EXPECT_DOUBLE_EQ(notes[0].beat, 0.0);

    // Second note at row 2: beat 2 / 2 = 1.0
    EXPECT_DOUBLE_EQ(notes[1].beat, 1.0);
}

TEST(KsfParser, HoldNotesConverted) {
    KsfParser parser(mock_file_reader(HOLD_KSF));
    auto chart = parser.parse("/fake/path/hold.ksf");

    auto& notes = chart.note_data().events();

    // Find hold head and tail
    bool found_hold_head = false;
    bool found_hold_tail = false;

    for (const auto& note : notes) {
        if (note.type == NoteType::HOLD_HEAD) {
            found_hold_head = true;
            EXPECT_EQ(note.column, 0);  // First column
            EXPECT_DOUBLE_EQ(note.beat, 0.0);  // First line
        }
        if (note.type == NoteType::HOLD_TAIL) {
            found_hold_tail = true;
            EXPECT_EQ(note.column, 0);  // First column
            EXPECT_DOUBLE_EQ(note.beat, 1.5);  // Row 3: beat 3 / 2 = 1.5
        }
    }

    EXPECT_TRUE(found_hold_head);
    EXPECT_TRUE(found_hold_tail);
}

TEST(KsfParser, EmptyFileThrows) {
    KsfParser parser(mock_file_reader(""));

    EXPECT_THROW({
        parser.parse("/fake/path/empty.ksf");
    }, ChartLoadException);
}

TEST(KsfParser, OnlyMetadataNoNotes) {
    const char* metadata_only = R"(#TITLE:No Notes;
#ARTIST:Test;
#BPM:120;
)";

    KsfParser parser(mock_file_reader(metadata_only));

    EXPECT_THROW({
        parser.parse("/fake/path/no_notes.ksf");
    }, ChartLoadException);
}

TEST(KsfParser, MissingBpmThrows) {
    const char* no_bpm = R"(#TITLE:No BPM;
#ARTIST:Test;
10000
00000
2222222222
)";

    KsfParser parser(mock_file_reader(no_bpm));

    // ChartBuilder adds default 120 BPM when none is provided, so this should not throw
    auto chart = parser.parse("/fake/path/no_bpm.ksf");

    // Verify default BPM was added
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 120.0);
}

TEST(KsfParser, AlwaysSingleMode) {
    KsfParser parser(mock_file_reader(MINIMAL_KSF));
    auto chart = parser.parse("/fake/path/test.ksf");

    EXPECT_EQ(chart.metadata().mode, PlayMode::SINGLE);
}

TEST(KsfParser, TickCountAffectsBeatPositions) {
    // TICKCOUNT=4 -> 4 rows per beat -> each row = 0.25 beats (16th notes)
    const char* tickcount4 = R"(#TITLE:TickCount 4;
#ARTIST:Test;
#BPM:120;
#TICKCOUNT:4;
10000
00000
00000
00000
01000
2222222222
)";

    KsfParser parser(mock_file_reader(tickcount4));
    auto chart = parser.parse("/fake/path/test.ksf");

    auto& notes = chart.note_data().events();
    ASSERT_GE(notes.size(), 2);

    // First note at row 0: beat 0.0
    EXPECT_DOUBLE_EQ(notes[0].beat, 0.0);

    // Second note at row 4: beat 4 / 4 = 1.0
    EXPECT_DOUBLE_EQ(notes[1].beat, 1.0);
}

TEST(KsfParser, SongfileTagResolved) {
    const char* songfile_ksf = R"(#TITLE:Songfile Test;
#ARTIST:Test;
#BPM:120;
#SONGFILE:music.ogg;
10000
2222222222
)";

    KsfParser parser(mock_file_reader(songfile_ksf));
    auto chart = parser.parse("/data/Song/chart.ksf");

    EXPECT_EQ(chart.metadata().audio_path, "/data/Song/music.ogg");
}

TEST(KsfParser, IntrofileTagResolved) {
    const char* introfile_ksf = R"(#TITLE:Introfile Test;
#ARTIST:Test;
#BPM:120;
#SONGFILE:music.ogg;
#INTROFILE:preview.ogg;
10000
2222222222
)";

    KsfParser parser(mock_file_reader(introfile_ksf));
    auto chart = parser.parse("/data/Song/chart.ksf");

    EXPECT_EQ(chart.metadata().audio_path, "/data/Song/music.ogg");
    EXPECT_EQ(chart.metadata().intro_path, "/data/Song/preview.ogg");
}

TEST(KsfParser, MissingTitleLogsWarning) {
    // This test verifies that a chart without TITLE still loads
    const char* no_title = R"(#ARTIST:Test Artist;
#BPM:120;
#TICKCOUNT:2;
10000
2222222222
)";

    KsfParser parser(mock_file_reader(no_title));

    // Should not throw - warning is logged instead
    auto chart = parser.parse("/fake/path/notitle.ksf");

    // Fallback title should be the filename stem
    EXPECT_EQ(chart.metadata().title, "notitle");
}

// ============================================================================
// Regression Tests - Load real fixture files from disk
// ============================================================================

namespace {

std::filesystem::path fixtures_dir() {
    const char* env = std::getenv("OPENITUP_FIXTURES_DIR");
    if (env) return std::filesystem::path(env);
    return std::filesystem::path(__FILE__).parent_path() / "fixtures";
}

} // anonymous namespace

TEST(KsfParserRegression, FixtureFileLoadedCorrectly) {
    // Load the actual test_basic.ksf fixture file using real filesystem
    KsfParser parser;  // Default constructor uses filesystem reader
    auto fixture_path = fixtures_dir() / "test_basic.ksf";
    auto chart = parser.parse(fixture_path.string());

    // Verify metadata
    EXPECT_EQ(chart.metadata().title, "Test Song");
    EXPECT_EQ(chart.metadata().artist, "Test Artist");
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 120.0);
    EXPECT_EQ(chart.note_count(), 4);
    EXPECT_EQ(chart.metadata().mode, PlayMode::SINGLE);
}

TEST(KsfParserRegression, FixtureNoteBeats) {
    // Verify exact beat positions of the 4 notes in test_basic.ksf
    // TICKCOUNT=2 means 2 rows per beat
    // Row 0 (tick 0): beat 0.0, "10000" → column 0
    // Row 2 (tick 2): beat 1.0, "01000" → column 1
    // Row 4 (tick 4): beat 2.0, "00010" → column 3
    // Row 6 (tick 6): beat 3.0, "00100" → column 2

    KsfParser parser;
    auto fixture_path = fixtures_dir() / "test_basic.ksf";
    auto chart = parser.parse(fixture_path.string());

    auto& notes = chart.note_data().events();
    ASSERT_EQ(notes.size(), 4) << "Expected exactly 4 notes";

    EXPECT_DOUBLE_EQ(notes[0].beat, 0.0);
    EXPECT_EQ(notes[0].column, 0);
    EXPECT_EQ(notes[0].type, NoteType::TAP);

    EXPECT_DOUBLE_EQ(notes[1].beat, 1.0);
    EXPECT_EQ(notes[1].column, 1);
    EXPECT_EQ(notes[1].type, NoteType::TAP);

    EXPECT_DOUBLE_EQ(notes[2].beat, 2.0);
    EXPECT_EQ(notes[2].column, 3);
    EXPECT_EQ(notes[2].type, NoteType::TAP);

    EXPECT_DOUBLE_EQ(notes[3].beat, 3.0);
    EXPECT_EQ(notes[3].column, 2);
    EXPECT_EQ(notes[3].type, NoteType::TAP);
}

TEST(KsfParserRegression, FixtureTiming) {
    // Verify time_at_beat matches expected values at 120 BPM
    // At 120 BPM: seconds_per_beat = 60/120 = 0.5
    // Beat 0.0 -> 0.0 seconds
    // Beat 0.5 -> 0.25 seconds
    // Beat 1.0 -> 0.5 seconds
    // Beat 1.5 -> 0.75 seconds
    // Beat 2.0 -> 1.0 seconds

    KsfParser parser;
    auto fixture_path = fixtures_dir() / "test_basic.ksf";
    auto chart = parser.parse(fixture_path.string());

    const auto& timing = chart.timing_data();

    EXPECT_DOUBLE_EQ(timing.time_at_beat(0.0), 0.0);
    EXPECT_DOUBLE_EQ(timing.time_at_beat(0.5), 0.25);
    EXPECT_DOUBLE_EQ(timing.time_at_beat(1.0), 0.5);
    EXPECT_DOUBLE_EQ(timing.time_at_beat(1.5), 0.75);
    EXPECT_DOUBLE_EQ(timing.time_at_beat(2.0), 1.0);
}

// ============================================================================
// Preview Audio Tests (US-CHT-018)
// ============================================================================

TEST(KsfParser, PreviewStartParsed) {
    const char* preview_ksf = R"(#TITLE:Preview Test;
#ARTIST:Test;
#BPM:120;
#SAMPLESTART:30.5;
10000
2222222222
)";

    KsfParser parser(mock_file_reader(preview_ksf));
    auto chart = parser.parse("/fake/path/preview.ksf");

    EXPECT_DOUBLE_EQ(chart.metadata().preview_start_seconds, 30.5);
}

TEST(KsfParser, PreviewLengthParsed) {
    const char* preview_ksf = R"(#TITLE:Preview Test;
#ARTIST:Test;
#BPM:120;
#SAMPLELENGTH:12.0;
10000
2222222222
)";

    KsfParser parser(mock_file_reader(preview_ksf));
    auto chart = parser.parse("/fake/path/preview.ksf");

    EXPECT_DOUBLE_EQ(chart.metadata().preview_length_seconds, 12.0);
}

TEST(KsfParser, PreviewStartAndLengthParsed) {
    const char* preview_ksf = R"(#TITLE:Preview Test;
#ARTIST:Test;
#BPM:120;
#SAMPLESTART:45.0;
#SAMPLELENGTH:20.0;
10000
2222222222
)";

    KsfParser parser(mock_file_reader(preview_ksf));
    auto chart = parser.parse("/fake/path/preview.ksf");

    EXPECT_DOUBLE_EQ(chart.metadata().preview_start_seconds, 45.0);
    EXPECT_DOUBLE_EQ(chart.metadata().preview_length_seconds, 20.0);
}

TEST(KsfParser, MissingPreviewDefaultsToNotSet) {
    // When no preview tags are present, defaults should be -1.0
    KsfParser parser(mock_file_reader(MINIMAL_KSF));
    auto chart = parser.parse("/fake/path/test.ksf");

    EXPECT_DOUBLE_EQ(chart.metadata().preview_start_seconds, -1.0);
    EXPECT_DOUBLE_EQ(chart.metadata().preview_length_seconds, -1.0);
}

TEST(KsfParser, InvalidPreviewStartLogsWarning) {
    const char* invalid_ksf = R"(#TITLE:Invalid Preview;
#ARTIST:Test;
#BPM:120;
#SAMPLESTART:not_a_number;
10000
2222222222
)";

    KsfParser parser(mock_file_reader(invalid_ksf));
    auto chart = parser.parse("/fake/path/invalid.ksf");

    // Should fall back to default value
    EXPECT_DOUBLE_EQ(chart.metadata().preview_start_seconds, -1.0);
}

TEST(KsfParser, InvalidPreviewLengthLogsWarning) {
    const char* invalid_ksf = R"(#TITLE:Invalid Preview;
#ARTIST:Test;
#BPM:120;
#SAMPLELENGTH:invalid;
10000
2222222222
)";

    KsfParser parser(mock_file_reader(invalid_ksf));
    auto chart = parser.parse("/fake/path/invalid.ksf");

    // Should fall back to default value
    EXPECT_DOUBLE_EQ(chart.metadata().preview_length_seconds, -1.0);
}
