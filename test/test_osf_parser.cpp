#include <gtest/gtest.h>
#include <openitup/chart/chart_builder.h>
#include <openitup/chart/osf_parser.h>
#include <openitup/chart/osf_writer.h>

using namespace openitup;

TEST(OsfParser, ParsesValidOsfFile) {
    std::string osf_content = R"({
  "version": "1.0",
  "metadata": {
    "title": "Test Song",
    "artist": "Test Artist",
    "mode": "SINGLE",
    "difficulty_name": "Normal",
    "difficulty_rating": 7
  },
  "timing_events": [
    { "type": "BPM_CHANGE", "beat": 0.0, "bpm": 120.0 }
  ],
  "notes": [
    { "beat": 0.0, "column": 2, "type": "TAP" },
    { "beat": 1.0, "column": 1, "type": "TAP" }
  ]
})";

    auto reader = [osf_content](const std::filesystem::path&) { return osf_content; };
    OsfParser parser(reader);

    Chart chart = parser.parse("/fake/path.osf");

    EXPECT_EQ(chart.metadata().title, "Test Song");
    EXPECT_EQ(chart.metadata().artist, "Test Artist");
    EXPECT_EQ(chart.metadata().mode, PlayMode::SINGLE);
    EXPECT_EQ(chart.metadata().difficulty_name, "Normal");
    EXPECT_EQ(chart.metadata().difficulty_rating, 7);
    EXPECT_EQ(chart.note_count(), 2);
}

TEST(OsfParser, ParsesTimingEvents) {
    std::string osf_content = R"({
  "version": "1.0",
  "metadata": {
    "title": "Timing Test",
    "mode": "SINGLE"
  },
  "timing_events": [
    { "type": "BPM_CHANGE", "beat": 0.0, "bpm": 120.0 },
    { "type": "BPM_CHANGE", "beat": 8.0, "bpm": 180.0 },
    { "type": "STOP", "beat": 16.0, "stop_duration": 1.0 }
  ],
  "notes": []
})";

    auto reader = [osf_content](const std::filesystem::path&) { return osf_content; };
    OsfParser parser(reader);

    Chart chart = parser.parse("/fake/path.osf");

    const auto& timing = chart.timing_data();
    EXPECT_EQ(timing.size(), 3);
    EXPECT_DOUBLE_EQ(timing.bpm_at_beat(0.0), 120.0);
    EXPECT_DOUBLE_EQ(timing.bpm_at_beat(8.0), 180.0);
}

TEST(OsfParser, ThrowsOnMissingRequiredFields) {
    std::string osf_content = R"({
  "version": "1.0",
  "metadata": {
    "mode": "SINGLE"
  },
  "timing_events": [],
  "notes": []
})";

    auto reader = [osf_content](const std::filesystem::path&) { return osf_content; };
    OsfParser parser(reader);

    EXPECT_THROW(parser.parse("/fake/path.osf"), ChartLoadException);
}

TEST(OsfParser, ThrowsOnInvalidJson) {
    std::string osf_content = "{ invalid json }";

    auto reader = [osf_content](const std::filesystem::path&) { return osf_content; };
    OsfParser parser(reader);

    EXPECT_THROW(parser.parse("/fake/path.osf"), ChartLoadException);
}

TEST(OsfParser, ParsesAllNoteTypes) {
    std::string osf_content = R"({
  "version": "1.0",
  "metadata": {
    "title": "Note Types",
    "mode": "SINGLE"
  },
  "timing_events": [
    { "type": "BPM_CHANGE", "beat": 0.0, "bpm": 120.0 }
  ],
  "notes": [
    { "beat": 0.0, "column": 0, "type": "TAP" },
    { "beat": 1.0, "column": 1, "type": "HOLD_HEAD" },
    { "beat": 2.0, "column": 1, "type": "HOLD_TAIL" },
    { "beat": 3.0, "column": 2, "type": "MINE" },
    { "beat": 4.0, "column": 3, "type": "FAKE" },
    { "beat": 5.0, "column": 4, "type": "LIFT" }
  ]
})";

    auto reader = [osf_content](const std::filesystem::path&) { return osf_content; };
    OsfParser parser(reader);

    Chart chart = parser.parse("/fake/path.osf");

    EXPECT_EQ(chart.note_count(), 6);
}

TEST(OsfWriter, SerializesChartToJson) {
    ChartBuilder builder;
    builder.set_title("Test Chart");
    builder.set_artist("Test Artist");
    builder.set_mode(PlayMode::SINGLE);
    builder.set_difficulty_name("Hard");
    builder.set_difficulty_rating(12);
    builder.add_bpm_change(0.0, 140.0);
    builder.add_note(0.0, 2, NoteType::TAP);
    builder.add_note(1.0, 1, NoteType::TAP);

    Chart chart = builder.build();

    OsfWriter writer;
    std::string json = writer.to_json(chart);

    // Verify JSON contains expected fields
    EXPECT_NE(json.find("\"version\""), std::string::npos);
    EXPECT_NE(json.find("\"title\""), std::string::npos);
    EXPECT_NE(json.find("Test Chart"), std::string::npos);
    EXPECT_NE(json.find("\"mode\""), std::string::npos);
    EXPECT_NE(json.find("SINGLE"), std::string::npos);
}

TEST(OsfRoundTrip, PreservesAllData) {
    ChartBuilder builder;
    builder.set_title("Round Trip");
    builder.set_artist("Tester");
    builder.set_mode(PlayMode::DOUBLE);
    builder.set_difficulty_name("Crazy");
    builder.set_difficulty_rating(18);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_bpm_change(8.0, 180.0);
    builder.add_stop(16.0, 1.5);
    builder.add_note(0.0, 2, NoteType::TAP);
    builder.add_note(1.0, 5, NoteType::HOLD_HEAD);
    builder.add_note(3.0, 5, NoteType::HOLD_TAIL);

    Chart original = builder.build();

    // Write to JSON
    OsfWriter writer;
    std::string json = writer.to_json(original);

    // Parse back
    auto reader = [json](const std::filesystem::path&) { return json; };
    OsfParser parser(reader);
    Chart parsed = parser.parse("/fake/path.osf");

    // Verify metadata
    EXPECT_EQ(parsed.metadata().title, "Round Trip");
    EXPECT_EQ(parsed.metadata().artist, "Tester");
    EXPECT_EQ(parsed.metadata().mode, PlayMode::DOUBLE);
    EXPECT_EQ(parsed.metadata().difficulty_name, "Crazy");
    EXPECT_EQ(parsed.metadata().difficulty_rating, 18);

    // Verify timing
    EXPECT_EQ(parsed.timing_data().size(), 3);

    // Verify notes
    EXPECT_EQ(parsed.note_count(), 3);
}
