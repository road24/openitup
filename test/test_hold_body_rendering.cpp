#include <gtest/gtest.h>

#include <openitup/render/note_renderer.h>
#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>
#include <openitup/chart/note_type.h>

#include <vector>

using namespace openitup;

// Helper: build a simple 120 BPM TimingData
static TimingData make_simple_timing(double bpm = 120.0) {
    std::vector<TimingEvent> events;
    events.push_back({0.0, TimingEventType::BPM_CHANGE, bpm, 0.0});
    return TimingData(std::move(events));
}

// --- Hold Body Rendering Tests ---

TEST(HoldBodyRendering, HoldBodySpansBetweenHeadAndTail) {
    // Hold from beat 4.0 to beat 8.0 (4-beat duration)
    // At current_beat = 0.0:
    //   Head at beat 8.0: y = 80 + 8*80 = 720
    //   Tail at beat 4.0: y = 80 + 4*80 = 400
    // Body should span from 400 to 720 (height = 320)
    std::vector<NoteEvent> notes;
    notes.push_back({4.0, 0, NoteType::HOLD_TAIL});
    notes.push_back({8.0, 0, NoteType::HOLD_HEAD});

    NoteData note_data(std::move(notes));
    TimingData timing = make_simple_timing(120.0);
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    constexpr double current_beat = 0.0;

    float head_y = renderer.beat_to_y(8.0, current_beat);
    float tail_y = renderer.beat_to_y(4.0, current_beat);

    EXPECT_FLOAT_EQ(head_y, 720.0f);  // 80 + 8*80
    EXPECT_FLOAT_EQ(tail_y, 400.0f);  // 80 + 4*80

    // Body height = head_y - tail_y
    float body_height = head_y - tail_y;
    EXPECT_FLOAT_EQ(body_height, 320.0f);  // 4 beats * 80 pixels/beat
}

TEST(HoldBodyRendering, HoldBodyAdjustsWithScrollSpeed) {
    // With scroll_speed = 2.0, hold body height doubles
    std::vector<NoteEvent> notes;
    notes.push_back({4.0, 0, NoteType::HOLD_TAIL});
    notes.push_back({8.0, 0, NoteType::HOLD_HEAD});

    NoteData note_data(std::move(notes));
    TimingData timing = make_simple_timing(120.0);
    NoteFieldConfig config = default_single_config();
    config.scroll_speed = 2.0f;
    NoteRenderer renderer(note_data, timing, config);

    constexpr double current_beat = 0.0;

    float head_y = renderer.beat_to_y(8.0, current_beat);
    float tail_y = renderer.beat_to_y(4.0, current_beat);

    // With scroll 2.0x: distance doubles
    // Head: 80 + 8*80*2 = 1360
    // Tail: 80 + 4*80*2 = 720
    EXPECT_FLOAT_EQ(head_y, 1360.0f);
    EXPECT_FLOAT_EQ(tail_y, 720.0f);

    float body_height = head_y - tail_y;
    EXPECT_FLOAT_EQ(body_height, 640.0f);  // 4 beats * 80 * 2.0
}

TEST(HoldBodyRendering, ShortHoldOnebeat) {
    // 1-beat hold from beat 4.0 to beat 5.0
    std::vector<NoteEvent> notes;
    notes.push_back({4.0, 1, NoteType::HOLD_TAIL});
    notes.push_back({5.0, 1, NoteType::HOLD_HEAD});

    NoteData note_data(std::move(notes));
    TimingData timing = make_simple_timing(120.0);
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    constexpr double current_beat = 0.0;

    float head_y = renderer.beat_to_y(5.0, current_beat);
    float tail_y = renderer.beat_to_y(4.0, current_beat);

    EXPECT_FLOAT_EQ(head_y, 480.0f);  // 80 + 5*80
    EXPECT_FLOAT_EQ(tail_y, 400.0f);  // 80 + 4*80

    float body_height = head_y - tail_y;
    EXPECT_FLOAT_EQ(body_height, 80.0f);  // 1 beat * 80 pixels/beat
}

TEST(HoldBodyRendering, LongHoldEightBeats) {
    // 8-beat hold from beat 0.0 to beat 8.0
    std::vector<NoteEvent> notes;
    notes.push_back({0.0, 2, NoteType::HOLD_TAIL});
    notes.push_back({8.0, 2, NoteType::HOLD_HEAD});

    NoteData note_data(std::move(notes));
    TimingData timing = make_simple_timing(120.0);
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    constexpr double current_beat = 0.0;

    float head_y = renderer.beat_to_y(8.0, current_beat);
    float tail_y = renderer.beat_to_y(0.0, current_beat);

    EXPECT_FLOAT_EQ(head_y, 720.0f);  // 80 + 8*80
    EXPECT_FLOAT_EQ(tail_y, 80.0f);   // 80 + 0*80

    float body_height = head_y - tail_y;
    EXPECT_FLOAT_EQ(body_height, 640.0f);  // 8 beats * 80 pixels/beat
}

TEST(HoldBodyRendering, MultipleHoldsSameColumn) {
    // Two holds in same column at different times
    std::vector<NoteEvent> notes;
    notes.push_back({4.0, 0, NoteType::HOLD_TAIL});
    notes.push_back({8.0, 0, NoteType::HOLD_HEAD});
    notes.push_back({12.0, 0, NoteType::HOLD_TAIL});
    notes.push_back({16.0, 0, NoteType::HOLD_HEAD});

    NoteData note_data(std::move(notes));
    TimingData timing = make_simple_timing(120.0);
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    constexpr double current_beat = 0.0;

    // First hold: 4.0 to 8.0
    float head1_y = renderer.beat_to_y(8.0, current_beat);
    float tail1_y = renderer.beat_to_y(4.0, current_beat);
    float body1_height = head1_y - tail1_y;
    EXPECT_FLOAT_EQ(body1_height, 320.0f);

    // Second hold: 12.0 to 16.0
    float head2_y = renderer.beat_to_y(16.0, current_beat);
    float tail2_y = renderer.beat_to_y(12.0, current_beat);
    float body2_height = head2_y - tail2_y;
    EXPECT_FLOAT_EQ(body2_height, 320.0f);
}

TEST(HoldBodyRendering, HoldAcrossDifferentColumns) {
    // Holds in different columns should not interfere
    std::vector<NoteEvent> notes;
    notes.push_back({4.0, 0, NoteType::HOLD_TAIL});
    notes.push_back({8.0, 0, NoteType::HOLD_HEAD});
    notes.push_back({4.0, 1, NoteType::HOLD_TAIL});
    notes.push_back({6.0, 1, NoteType::HOLD_HEAD});

    NoteData note_data(std::move(notes));
    TimingData timing = make_simple_timing(120.0);
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    constexpr double current_beat = 0.0;

    // Column 0 hold: 4.0 to 8.0 (4 beats)
    float head0_y = renderer.beat_to_y(8.0, current_beat);
    float tail0_y = renderer.beat_to_y(4.0, current_beat);
    EXPECT_FLOAT_EQ(head0_y - tail0_y, 320.0f);

    // Column 1 hold: 4.0 to 6.0 (2 beats)
    float head1_y = renderer.beat_to_y(6.0, current_beat);
    float tail1_y = renderer.beat_to_y(4.0, current_beat);
    EXPECT_FLOAT_EQ(head1_y - tail1_y, 160.0f);
}

TEST(HoldBodyRendering, HoldThroughBPMChange) {
    // Hold spanning a BPM change should maintain visual continuity
    // BPM changes from 120 to 180 at beat 8.0
    std::vector<TimingEvent> events;
    events.push_back({0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0});
    events.push_back({8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0});
    TimingData timing(std::move(events));

    std::vector<NoteEvent> notes;
    notes.push_back({4.0, 0, NoteType::HOLD_TAIL});
    notes.push_back({12.0, 0, NoteType::HOLD_HEAD});  // Spans BPM change at beat 8.0

    NoteData note_data(std::move(notes));
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    constexpr double current_beat = 0.0;

    // Beat-to-Y conversion should work correctly regardless of BPM
    // (BPM affects timing, not beat-space positions)
    float head_y = renderer.beat_to_y(12.0, current_beat);
    float tail_y = renderer.beat_to_y(4.0, current_beat);

    // In beat space: 12 - 4 = 8 beats * 80 pixels/beat = 640 pixels
    EXPECT_FLOAT_EQ(head_y, 1040.0f);  // 80 + 12*80
    EXPECT_FLOAT_EQ(tail_y, 400.0f);   // 80 + 4*80

    float body_height = head_y - tail_y;
    EXPECT_FLOAT_EQ(body_height, 640.0f);
}

TEST(HoldBodyRendering, HoldHeadWithoutTailIgnored) {
    // HOLD_HEAD without matching HOLD_TAIL should not crash
    // (malformed chart data - should be skipped gracefully)
    std::vector<NoteEvent> notes;
    notes.push_back({8.0, 0, NoteType::HOLD_HEAD});  // No tail

    NoteData note_data(std::move(notes));
    TimingData timing = make_simple_timing(120.0);
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    // Rendering should not crash (we can't directly test render() without SDL)
    // But we can verify the head position is calculated correctly
    float head_y = renderer.beat_to_y(8.0, 0.0);
    EXPECT_FLOAT_EQ(head_y, 720.0f);
}

TEST(HoldBodyRendering, ColumnColorsForHoldBodies) {
    // Hold bodies in different columns should use different colors (fallback mode)
    // We can verify that COLUMN_COLORS array has distinct values per column
    std::vector<NoteEvent> notes;
    for (int col = 0; col < 5; ++col) {
        notes.push_back({static_cast<double>(col * 4), static_cast<uint8_t>(col), NoteType::HOLD_TAIL});
        notes.push_back({static_cast<double>(col * 4 + 2), static_cast<uint8_t>(col), NoteType::HOLD_HEAD});
    }

    NoteData note_data(std::move(notes));
    TimingData timing = make_simple_timing(120.0);
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    // Verify distinct colors exist for all columns
    for (int i = 0; i < 4; ++i) {
        const auto& color1 = COLUMN_COLORS[i];
        const auto& color2 = COLUMN_COLORS[i + 1];

        bool different = (color1.r != color2.r) || (color1.g != color2.g) || (color1.b != color2.b);
        EXPECT_TRUE(different) << "Colors for columns " << i << " and " << (i + 1) << " are identical";
    }
}
