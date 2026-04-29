#include <gtest/gtest.h>

#include <openitup/render/note_renderer.h>
#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>
#include <openitup/chart/note_type.h>

#include <vector>
#include <cmath>

using namespace openitup;

// --- Helper functions for building test fixtures ---

// Helper: build a simple 120 BPM TimingData (static to avoid duplicate definition)
static TimingData make_simple_timing(double bpm = 120.0) {
    std::vector<TimingEvent> events;
    events.push_back({0.0, TimingEventType::BPM_CHANGE, bpm, 0.0});
    return TimingData(std::move(events));
}

// --- beat_to_y Conversion Tests ---

TEST(NoteRenderer, BeatToYAtReceptor) {
    // When note_beat equals current_beat, y should equal receptor_y
    NoteData empty_notes(std::vector<NoteEvent>{});
    TimingData timing = make_simple_timing();
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(empty_notes, timing, config);

    float y = renderer.beat_to_y(10.0, 10.0);
    EXPECT_FLOAT_EQ(y, 400.0f);
}

TEST(NoteRenderer, BeatToYFourBeatsAbove) {
    // Note 4 beats in the future should be 320 pixels above receptor
    // y = 400 - 4.0 * 80 = 80
    NoteData empty_notes(std::vector<NoteEvent>{});
    TimingData timing = make_simple_timing();
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(empty_notes, timing, config);

    float y = renderer.beat_to_y(14.0, 10.0);
    EXPECT_FLOAT_EQ(y, 80.0f);
}

TEST(NoteRenderer, BeatToYOneBeatBelow) {
    // Note 1 beat in the past should be 80 pixels below receptor
    // y = 400 - (-1.0) * 80 = 480
    NoteData empty_notes(std::vector<NoteEvent>{});
    TimingData timing = make_simple_timing();
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(empty_notes, timing, config);

    float y = renderer.beat_to_y(9.0, 10.0);
    EXPECT_FLOAT_EQ(y, 480.0f);
}

TEST(NoteRenderer, BeatToYScrollSpeedDoubled) {
    // With scroll_speed = 2.0, distance doubles
    // 4 beats * 80 pixels_per_beat * 2.0 speed = 640 pixels
    // y = 400 - 640 = -240
    NoteData empty_notes(std::vector<NoteEvent>{});
    TimingData timing = make_simple_timing();
    NoteFieldConfig config = default_single_config();
    config.scroll_speed = 2.0f;
    NoteRenderer renderer(empty_notes, timing, config);

    float y = renderer.beat_to_y(14.0, 10.0);
    EXPECT_FLOAT_EQ(y, -240.0f);
}

TEST(NoteRenderer, BeatToYScrollSpeedHalf) {
    // With scroll_speed = 0.5, distance halves
    // 4 beats * 80 pixels_per_beat * 0.5 speed = 160 pixels
    // y = 400 - 160 = 240
    NoteData empty_notes(std::vector<NoteEvent>{});
    TimingData timing = make_simple_timing();
    NoteFieldConfig config = default_single_config();
    config.scroll_speed = 0.5f;
    NoteRenderer renderer(empty_notes, timing, config);

    float y = renderer.beat_to_y(14.0, 10.0);
    EXPECT_FLOAT_EQ(y, 240.0f);
}

TEST(NoteRenderer, BeatToYZeroBeatDelta) {
    // When beat_delta is exactly 0, y should be exactly receptor_y
    NoteData empty_notes(std::vector<NoteEvent>{});
    TimingData timing = make_simple_timing();
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(empty_notes, timing, config);

    float y = renderer.beat_to_y(5.0, 5.0);
    EXPECT_FLOAT_EQ(y, config.receptor_y);
}

// --- Configuration Tests ---

TEST(NoteRenderer, DefaultSingleConfigFiveColumns) {
    // Default single config should have 5 columns
    NoteFieldConfig config = default_single_config();
    EXPECT_EQ(config.column_x.size(), 5);
    EXPECT_EQ(config.num_columns, 5);
}

TEST(NoteRenderer, DefaultSingleConfigCentered) {
    // Middle column (index 2) should be at screen center (320 in 640px width)
    NoteFieldConfig config = default_single_config();
    EXPECT_FLOAT_EQ(config.column_x[2], 320.0f);
}

TEST(NoteRenderer, DefaultSingleConfigEqualSpacing) {
    // All adjacent columns should have equal spacing (56px)
    NoteFieldConfig config = default_single_config();

    for (size_t i = 1; i < config.column_x.size(); i++) {
        float spacing = config.column_x[i] - config.column_x[i-1];
        EXPECT_FLOAT_EQ(spacing, 56.0f);
    }
}

TEST(NoteRenderer, DefaultSingleConfigNoteSize) {
    // Note dimensions should be 48x48
    NoteFieldConfig config = default_single_config();
    EXPECT_FLOAT_EQ(config.note_width, 48.0f);
    EXPECT_FLOAT_EQ(config.note_height, 48.0f);
}

// --- Column Color Tests ---

TEST(NoteRenderer, ColumnColorsDistinct) {
    // All 5 single-mode column colors should be distinct
    std::vector<ColumnColor> colors;
    for (int i = 0; i < 5; i++) {
        colors.push_back(COLUMN_COLORS[i]);
    }

    // Check each pair is different
    for (size_t i = 0; i < colors.size(); i++) {
        for (size_t j = i + 1; j < colors.size(); j++) {
            bool same = (colors[i].r == colors[j].r &&
                        colors[i].g == colors[j].g &&
                        colors[i].b == colors[j].b);
            EXPECT_FALSE(same) << "Colors at indices " << i << " and " << j << " are the same";
        }
    }
}
