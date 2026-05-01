#include <gtest/gtest.h>
#include <openitup/render/note_renderer.h>
#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>

using namespace openitup;

// US-REN-030: Single mode note field layout tests
TEST(NoteFieldConfig, SingleModeProperSpacing) {
    auto config = default_single_config();

    // Verify 5 columns
    EXPECT_EQ(config.num_columns, 5);
    EXPECT_EQ(config.column_x.size(), 5);

    // Verify centered around x=320
    float center_x = 320.0f;
    float spacing = 64.0f;

    // Check column positions with proper spacing
    EXPECT_FLOAT_EQ(config.column_x[0], 192.0f);  // center - 2*spacing
    EXPECT_FLOAT_EQ(config.column_x[1], 256.0f);  // center - spacing
    EXPECT_FLOAT_EQ(config.column_x[2], 320.0f);  // center
    EXPECT_FLOAT_EQ(config.column_x[3], 384.0f);  // center + spacing
    EXPECT_FLOAT_EQ(config.column_x[4], 448.0f);  // center + 2*spacing

    // Verify spacing consistency
    for (int i = 1; i < 5; i++) {
        float actual_spacing = config.column_x[i] - config.column_x[i-1];
        EXPECT_FLOAT_EQ(actual_spacing, spacing);
    }
}

TEST(NoteFieldConfig, SingleModeCenteredInViewport) {
    auto config = default_single_config();

    // Verify all columns are within the 640px viewport
    for (int i = 0; i < 5; i++) {
        EXPECT_GT(config.column_x[i], 0.0f);
        EXPECT_LT(config.column_x[i], 640.0f);
    }

    // Verify center column is at screen center
    EXPECT_FLOAT_EQ(config.column_x[2], 320.0f);
}

// US-REN-030: Double mode note field layout tests
TEST(NoteFieldConfig, DoubleModeProperSpacing) {
    auto config = default_double_config();

    // Verify 10 columns
    EXPECT_EQ(config.num_columns, 10);
    EXPECT_EQ(config.column_x.size(), 10);

    // Verify P1 side (columns 0-4) centered around x=160
    float left_center = 160.0f;
    float spacing = 58.0f;

    EXPECT_FLOAT_EQ(config.column_x[0], left_center - 2.0f * spacing);
    EXPECT_FLOAT_EQ(config.column_x[2], left_center);
    EXPECT_FLOAT_EQ(config.column_x[4], left_center + 2.0f * spacing);

    // Verify P2 side (columns 5-9) centered around x=480
    float right_center = 480.0f;

    EXPECT_FLOAT_EQ(config.column_x[5], right_center - 2.0f * spacing);
    EXPECT_FLOAT_EQ(config.column_x[7], right_center);
    EXPECT_FLOAT_EQ(config.column_x[9], right_center + 2.0f * spacing);

    // Verify spacing consistency within each side
    for (int i = 1; i < 5; i++) {
        float p1_spacing = config.column_x[i] - config.column_x[i-1];
        EXPECT_FLOAT_EQ(p1_spacing, spacing);

        float p2_spacing = config.column_x[i+5] - config.column_x[i+4];
        EXPECT_FLOAT_EQ(p2_spacing, spacing);
    }
}

TEST(NoteFieldConfig, DoubleModeTwoSeparateSides) {
    auto config = default_double_config();

    // Verify visual separation between P1 and P2 sides
    float rightmost_p1 = config.column_x[4];
    float leftmost_p2 = config.column_x[5];
    float gap = leftmost_p2 - rightmost_p1;

    // Gap should be at least 50 pixels for visual distinction
    EXPECT_GT(gap, 50.0f);

    // Verify all columns within viewport
    for (int i = 0; i < 10; i++) {
        EXPECT_GT(config.column_x[i], 0.0f);
        EXPECT_LT(config.column_x[i], 640.0f);
    }
}

// US-REN-032: C-Mod speed modifier tests
TEST(NoteFieldConfig, CModFixedScrollRate) {
    // Create timing data with BPM = 120
    std::vector<TimingEvent> timing_events;
    timing_events.push_back({0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0});
    TimingData timing(std::move(timing_events));

    NoteData notes;  // Empty notes
    NoteFieldConfig config = default_single_config();
    config.speed_mod_type = SpeedModType::C_MOD;
    config.speed_mod_value = 400.0f;  // 400 pixels/second

    NoteRenderer renderer(notes, timing, config);

    // At BPM 120, with C-mod 400px/s:
    // Beats per second = 120/60 = 2.0
    // At beat 0, a note at beat 4 is 4 beats away
    // Should appear at: receptor_y + (400 / 2.0) * 4 = 80 + 200 * 4 = 880
    float y = renderer.beat_to_y(4.0, 0.0);
    EXPECT_NEAR(y, 880.0f, 1.0f);
}

TEST(NoteFieldConfig, CModBPMChangeDoesNotAffectScrollSpeed) {
    // Create timing data with BPM changes
    std::vector<TimingEvent> timing_events;
    timing_events.push_back({0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0});
    timing_events.push_back({8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0});
    TimingData timing(std::move(timing_events));

    NoteData notes;  // Empty notes
    NoteFieldConfig config = default_single_config();
    config.speed_mod_type = SpeedModType::C_MOD;
    config.speed_mod_value = 400.0f;

    NoteRenderer renderer(notes, timing, config);

    // At beat 0 (BPM 120):
    // Note at beat 4 should be at same visual distance as...
    // At beat 8 (BPM 180):
    // Note at beat 12 (also 4 beats away)

    // The scroll *speed* is constant (400px/s), but the spacing between
    // notes adjusts because beats/second changes with BPM

    // At BPM 120: beats_per_sec = 2.0, so 4 beats * (400/2.0) = 800px
    float y_120 = renderer.beat_to_y(4.0, 0.0);

    // At BPM 180: beats_per_sec = 3.0, so 4 beats * (400/3.0) ≈ 533px
    float y_180 = renderer.beat_to_y(12.0, 8.0);

    // The visual spacing should be different because BPM changed
    EXPECT_NE(y_120, y_180);

    // But both should use the same C-mod value (400px/s)
    EXPECT_NEAR(y_120, 80.0f + 800.0f, 1.0f);
    EXPECT_NEAR(y_180, 80.0f + 533.3f, 1.0f);
}

// US-REN-033: M-Mod speed modifier tests
TEST(NoteFieldConfig, MModMultipliesScrollSpeed) {
    // Create timing data with BPM = 120
    std::vector<TimingEvent> timing_events;
    timing_events.push_back({0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0});
    TimingData timing(std::move(timing_events));

    NoteData notes;  // Empty notes
    NoteFieldConfig config = default_single_config();
    config.speed_mod_type = SpeedModType::M_MOD;
    config.speed_mod_value = 2.0f;  // 2x multiplier
    config.scroll_speed = 1.0f;
    config.pixels_per_beat = 80.0f;

    NoteRenderer renderer(notes, timing, config);

    // With M-mod 2.0x:
    // Distance for 4 beats = 80 * 1.0 * 2.0 * 4 = 640 pixels
    float y = renderer.beat_to_y(4.0, 0.0);
    EXPECT_NEAR(y, 80.0f + 640.0f, 1.0f);
}

TEST(NoteFieldConfig, MModScalesWithBPM) {
    // Create timing data with BPM changes
    std::vector<TimingEvent> timing_events;
    timing_events.push_back({0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0});
    timing_events.push_back({8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0});
    TimingData timing(std::move(timing_events));

    NoteData notes;  // Empty notes
    NoteFieldConfig config = default_single_config();
    config.speed_mod_type = SpeedModType::M_MOD;
    config.speed_mod_value = 2.0f;
    config.pixels_per_beat = 80.0f;

    NoteRenderer renderer(notes, timing, config);

    // M-mod scales linearly with multiplier
    // At beat 0, note at beat 4: 4 beats * 80px * 2.0 = 640px
    float y_before = renderer.beat_to_y(4.0, 0.0);
    EXPECT_NEAR(y_before, 80.0f + 640.0f, 1.0f);

    // At beat 8, note at beat 12: same 4 beats * 80px * 2.0 = 640px
    // BPM doesn't affect M-mod spacing (only affects time-to-beat conversion)
    float y_after = renderer.beat_to_y(12.0, 8.0);
    EXPECT_NEAR(y_after, 80.0f + 640.0f, 1.0f);
}

TEST(NoteFieldConfig, MModDefaultValue) {
    auto config = default_single_config();

    // Default should be M-mod with 1.0x multiplier
    EXPECT_EQ(config.speed_mod_type, SpeedModType::M_MOD);
    EXPECT_FLOAT_EQ(config.speed_mod_value, 1.0f);
}
