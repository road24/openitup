#include <gtest/gtest.h>
#include <openitup/chart/ssc_parser.h>
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

// Minimal valid SSC content with one chart
const char* MINIMAL_SSC = R"(#VERSION:0.83;
#TITLE:Test Song;
#ARTIST:Test Artist;
#MUSIC:test.ogg;
#OFFSET:0.0;
#SAMPLESTART:10.0;
#SAMPLELENGTH:15.0;
#DISPLAYBPM:140;
#BPMS:0=140;
#STOPS:;

#NOTEDATA:;
#STEPSTYPE:pump-single;
#DIFFICULTY:Easy;
#METER:3;
#NOTES:
10000
00000
01000
00000
,
00100
00000
00010
00000
;
)";

// SSC with multiple charts
const char* MULTI_CHART_SSC = R"(#VERSION:0.83;
#TITLE:Multi Chart;
#ARTIST:Test;
#MUSIC:audio.mp3;
#BPMS:0=120;
#STOPS:;

#NOTEDATA:;
#STEPSTYPE:pump-single;
#DIFFICULTY:Easy;
#METER:2;
#NOTES:
10000
00000
;

#NOTEDATA:;
#STEPSTYPE:pump-double;
#DIFFICULTY:Hard;
#METER:8;
#NOTES:
1000000000
0000000000
;
)";

// SSC with holds, mines, and fakes
const char* COMPLEX_NOTES_SSC = R"(#VERSION:0.83;
#TITLE:Complex;
#ARTIST:Test;
#BPMS:0=140;
#STOPS:;

#NOTEDATA:;
#STEPSTYPE:pump-single;
#DIFFICULTY:Normal;
#METER:5;
#NOTES:
20000
10000
30000
00000
,
M0000
00000
0F000
00000
;
)";

// SSC with per-chart timing overrides
const char* CHART_TIMING_SSC = R"(#VERSION:0.83;
#TITLE:Timing Test;
#ARTIST:Test;
#BPMS:0=100;
#STOPS:4=1.0;

#NOTEDATA:;
#STEPSTYPE:pump-single;
#DIFFICULTY:Easy;
#METER:3;
#BPMS:0=150;
#STOPS:2=0.5;
#NOTES:
10000
00000
01000
00000
;
)";

// SSC with BPM changes
const char* BPM_CHANGE_SSC = R"(#VERSION:0.83;
#TITLE:BPM Changes;
#ARTIST:Test;
#BPMS:0=120,8=180,16=140;
#STOPS:;

#NOTEDATA:;
#STEPSTYPE:pump-single;
#DIFFICULTY:Normal;
#METER:6;
#NOTES:
10000
00000
01000
00000
;
)";

} // anonymous namespace

TEST(SscParser, MinimalSscParsed) {
    SscParser parser(mock_file_reader(MINIMAL_SSC));
    auto charts = parser.parse("/fake/path/test.ssc");

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];

    EXPECT_EQ(chart.metadata().title, "Test Song");
    EXPECT_EQ(chart.metadata().artist, "Test Artist");
    EXPECT_EQ(chart.metadata().mode, PlayMode::SINGLE);
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 140.0);
    EXPECT_GT(chart.note_count(), 0);
}

TEST(SscParser, MultipleChartsParsed) {
    SscParser parser(mock_file_reader(MULTI_CHART_SSC));
    auto charts = parser.parse("/fake/path/multi.ssc");

    ASSERT_EQ(charts.size(), 2);

    // First chart: single mode, easy
    EXPECT_EQ(charts[0].metadata().mode, PlayMode::SINGLE);
    EXPECT_EQ(charts[0].metadata().difficulty_name, "Easy");
    EXPECT_EQ(charts[0].metadata().difficulty_rating, 2);

    // Second chart: double mode, hard
    EXPECT_EQ(charts[1].metadata().mode, PlayMode::DOUBLE);
    EXPECT_EQ(charts[1].metadata().difficulty_name, "Hard");
    EXPECT_EQ(charts[1].metadata().difficulty_rating, 8);
}

TEST(SscParser, MetadataExtracted) {
    SscParser parser(mock_file_reader(MINIMAL_SSC));
    auto charts = parser.parse("/fake/path/test.ssc");

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];

    EXPECT_EQ(chart.metadata().title, "Test Song");
    EXPECT_EQ(chart.metadata().artist, "Test Artist");
    EXPECT_EQ(chart.metadata().difficulty_name, "Easy");
    EXPECT_EQ(chart.metadata().difficulty_rating, 3);
}

TEST(SscParser, AudioPathResolved) {
    SscParser parser(mock_file_reader(MINIMAL_SSC));
    auto charts = parser.parse("/data/Song/chart.ssc");

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];

    // Audio file should be resolved relative to chart directory
    EXPECT_EQ(chart.metadata().audio_path, "/data/Song/test.ogg");
}

TEST(SscParser, PreviewTimesExtracted) {
    SscParser parser(mock_file_reader(MINIMAL_SSC));
    auto charts = parser.parse("/fake/path/test.ssc");

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];

    EXPECT_DOUBLE_EQ(chart.metadata().preview_start_seconds, 10.0);
    EXPECT_DOUBLE_EQ(chart.metadata().preview_length_seconds, 15.0);
}

TEST(SscParser, BpmExtracted) {
    SscParser parser(mock_file_reader(MINIMAL_SSC));
    auto charts = parser.parse("/fake/path/test.ssc");

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];

    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 140.0);
    EXPECT_DOUBLE_EQ(chart.metadata().display_bpm, 140.0);
}

TEST(SscParser, NotesConvertedToBeats) {
    // Measures are 4 beats, rows are evenly subdivided
    SscParser parser(mock_file_reader(MINIMAL_SSC));
    auto charts = parser.parse("/fake/path/test.ssc");

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];
    auto& notes = chart.note_data().events();

    ASSERT_GE(notes.size(), 4);

    // First measure has 4 rows: beat 0, 1, 2, 3
    // Second measure has 4 rows: beat 4, 5, 6, 7
    EXPECT_DOUBLE_EQ(notes[0].beat, 0.0);   // First note: row 0
    EXPECT_DOUBLE_EQ(notes[1].beat, 2.0);   // Second note: row 2 (beat 2)
    EXPECT_DOUBLE_EQ(notes[2].beat, 4.0);   // Third note: measure 2, row 0
    EXPECT_DOUBLE_EQ(notes[3].beat, 6.0);   // Fourth note: measure 2, row 2
}

TEST(SscParser, HoldNotesConverted) {
    SscParser parser(mock_file_reader(COMPLEX_NOTES_SSC));
    auto charts = parser.parse("/fake/path/complex.ssc");

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];
    auto& notes = chart.note_data().events();

    // First measure: 2=hold head, 1=hold body (ignored), 3=hold tail
    ASSERT_GE(notes.size(), 2);

    // Find hold head and tail
    bool found_hold_head = false;
    bool found_hold_tail = false;
    for (const auto& note : notes) {
        if (note.column == 0) {
            if (note.type == NoteType::HOLD_HEAD) {
                found_hold_head = true;
                EXPECT_DOUBLE_EQ(note.beat, 0.0);
            } else if (note.type == NoteType::HOLD_TAIL) {
                found_hold_tail = true;
                EXPECT_DOUBLE_EQ(note.beat, 2.0);
            }
        }
    }

    EXPECT_TRUE(found_hold_head);
    EXPECT_TRUE(found_hold_tail);
}

TEST(SscParser, MinesAndFakesConverted) {
    SscParser parser(mock_file_reader(COMPLEX_NOTES_SSC));
    auto charts = parser.parse("/fake/path/complex.ssc");

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];
    auto& notes = chart.note_data().events();

    // Second measure: M=mine, F=fake
    bool found_mine = false;
    bool found_fake = false;
    for (const auto& note : notes) {
        if (note.type == NoteType::MINE) {
            found_mine = true;
            EXPECT_EQ(note.column, 0);
        } else if (note.type == NoteType::FAKE) {
            found_fake = true;
            EXPECT_EQ(note.column, 1);
        }
    }

    EXPECT_TRUE(found_mine);
    EXPECT_TRUE(found_fake);
}

TEST(SscParser, PerChartTimingOverride) {
    SscParser parser(mock_file_reader(CHART_TIMING_SSC));
    auto charts = parser.parse("/fake/path/timing.ssc");

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];

    // Chart overrides song-level BPM (150 instead of 100)
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 150.0);

    // Chart has its own stop
    auto& events = chart.timing_data().events();
    bool found_stop = false;
    for (const auto& event : events) {
        if (event.type == TimingEventType::STOP && event.beat == 2.0) {
            found_stop = true;
            EXPECT_DOUBLE_EQ(event.stop_duration, 0.5);
        }
    }
    EXPECT_TRUE(found_stop);
}

TEST(SscParser, BpmChanges) {
    SscParser parser(mock_file_reader(BPM_CHANGE_SSC));
    auto charts = parser.parse("/fake/path/bpmchange.ssc");

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];

    // Check BPM at different beats
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 120.0);
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(8.0), 180.0);
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(16.0), 140.0);
}

TEST(SscParser, StepstypeParsing) {
    const char* mode_test = R"(#VERSION:0.83;
#TITLE:Mode Test;
#BPMS:0=120;

#NOTEDATA:;
#STEPSTYPE:pump-single;
#DIFFICULTY:Easy;
#METER:1;
#NOTES:
10000
;

#NOTEDATA:;
#STEPSTYPE:pump-double;
#DIFFICULTY:Easy;
#METER:1;
#NOTES:
1000000000
;

#NOTEDATA:;
#STEPSTYPE:pump-halfdouble;
#DIFFICULTY:Easy;
#METER:1;
#NOTES:
10000
;
)";

    SscParser parser(mock_file_reader(mode_test));
    auto charts = parser.parse("/fake/path/modes.ssc");

    ASSERT_EQ(charts.size(), 3);
    EXPECT_EQ(charts[0].metadata().mode, PlayMode::SINGLE);
    EXPECT_EQ(charts[1].metadata().mode, PlayMode::DOUBLE);
    EXPECT_EQ(charts[2].metadata().mode, PlayMode::HALF);
}

TEST(SscParser, EmptyFileFails) {
    SscParser parser(mock_file_reader(""));
    EXPECT_THROW(parser.parse("/fake/path/empty.ssc"), ChartLoadException);
}

TEST(SscParser, NoChartsFails) {
    const char* no_charts = R"(#VERSION:0.83;
#TITLE:No Charts;
#BPMS:0=120;
)";

    SscParser parser(mock_file_reader(no_charts));
    EXPECT_THROW(parser.parse("/fake/path/nocharts.ssc"), ChartLoadException);
}

TEST(SscParser, DefaultsToFilenameIfNoTitle) {
    const char* no_title = R"(#VERSION:0.83;
#BPMS:0=120;

#NOTEDATA:;
#STEPSTYPE:pump-single;
#DIFFICULTY:Easy;
#METER:1;
#NOTES:
10000
;
)";

    SscParser parser(mock_file_reader(no_title));
    auto charts = parser.parse("/fake/path/song_name.ssc");

    ASSERT_EQ(charts.size(), 1);
    EXPECT_EQ(charts[0].metadata().title, "song_name");
}
