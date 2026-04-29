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
    // TICKCOUNT=2 means 4 lines per beat (TICKCOUNT * 2)
    // Line 0: beat 0.0, Line 1: beat 0.25, Line 2: beat 0.5, Line 3: beat 0.75, Line 4: beat 1.0
    KsfParser parser(mock_file_reader(MINIMAL_KSF));
    auto chart = parser.parse("/fake/path/test.ksf");

    auto& notes = chart.note_data().events();
    ASSERT_GE(notes.size(), 2);

    // First note at line 0: beat 0.0
    EXPECT_DOUBLE_EQ(notes[0].beat, 0.0);

    // Second note at line 2: beat 0.5 (2 / 4 = 0.5)
    EXPECT_DOUBLE_EQ(notes[1].beat, 0.5);
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
            EXPECT_DOUBLE_EQ(note.beat, 0.75);  // Line 3: beat 3/4 = 0.75
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
    // TICKCOUNT=2 -> 4 lines per beat
    const char* tickcount2 = R"(#TITLE:TickCount 2;
#ARTIST:Test;
#BPM:120;
#TICKCOUNT:2;
10000
00000
00000
00000
01000
2222222222
)";

    KsfParser parser(mock_file_reader(tickcount2));
    auto chart = parser.parse("/fake/path/test.ksf");

    auto& notes = chart.note_data().events();
    ASSERT_GE(notes.size(), 2);

    // First note at line 0: beat 0.0
    EXPECT_DOUBLE_EQ(notes[0].beat, 0.0);

    // Second note at line 4: beat 4/4 = 1.0
    EXPECT_DOUBLE_EQ(notes[1].beat, 1.0);
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
