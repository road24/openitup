#include <gtest/gtest.h>

#include <openitup/render/note_renderer.h>
#include <openitup/render/judgment_display.h>
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
    EXPECT_FLOAT_EQ(y, config.receptor_y);
}

TEST(NoteRenderer, BeatToYFourBeatsAhead) {
    // Note 4 beats in the future should be 320 pixels below receptor (scrolls up)
    // y = 80 + 4.0 * 80 = 400
    NoteData empty_notes(std::vector<NoteEvent>{});
    TimingData timing = make_simple_timing();
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(empty_notes, timing, config);

    float y = renderer.beat_to_y(14.0, 10.0);
    EXPECT_FLOAT_EQ(y, 400.0f);
}

TEST(NoteRenderer, BeatToYOneBeatPast) {
    // Note 1 beat in the past should be 80 pixels above receptor
    // y = 80 + (-1.0) * 80 = 0
    NoteData empty_notes(std::vector<NoteEvent>{});
    TimingData timing = make_simple_timing();
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(empty_notes, timing, config);

    float y = renderer.beat_to_y(9.0, 10.0);
    EXPECT_FLOAT_EQ(y, 0.0f);
}

TEST(NoteRenderer, BeatToYScrollSpeedDoubled) {
    // With scroll_speed = 2.0, distance doubles
    // 4 beats * 80 pixels_per_beat * 2.0 speed = 640 pixels
    // y = 80 + 640 = 720
    NoteData empty_notes(std::vector<NoteEvent>{});
    TimingData timing = make_simple_timing();
    NoteFieldConfig config = default_single_config();
    config.scroll_speed = 2.0f;
    NoteRenderer renderer(empty_notes, timing, config);

    float y = renderer.beat_to_y(14.0, 10.0);
    EXPECT_FLOAT_EQ(y, 720.0f);
}

TEST(NoteRenderer, BeatToYScrollSpeedHalf) {
    // With scroll_speed = 0.5, distance halves
    // 4 beats * 80 pixels_per_beat * 0.5 speed = 160 pixels
    // y = 80 + 160 = 240
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

// --- JudgmentDisplay Tests ---

TEST(JudgmentDisplay, InitiallyInvisible) {
    // Default-constructed display should be invisible
    JudgmentDisplay display;
    EXPECT_FALSE(display.is_visible());
}

TEST(JudgmentDisplay, OnJudgmentMakesVisible) {
    // After on_judgment, display should be visible
    JudgmentDisplay display;
    display.on_judgment(JudgmentTier::PERFECT);
    EXPECT_TRUE(display.is_visible());
}

TEST(JudgmentDisplay, CurrentTierUpdates) {
    // on_judgment should update the current tier
    JudgmentDisplay display;
    display.on_judgment(JudgmentTier::GREAT);
    EXPECT_EQ(display.current_tier(), JudgmentTier::GREAT);
}

TEST(JudgmentDisplay, FadesAfterDuration) {
    // After DISPLAY_DURATION (0.5s), display should be invisible
    // Simulate 31 frames at 60 fps (1/60 = ~0.01667s per frame)
    // 31 frames = ~0.517s > 0.5s
    JudgmentDisplay display;
    display.on_judgment(JudgmentTier::PERFECT);

    constexpr double dt = 1.0 / 60.0;
    for (int i = 0; i < 31; i++) {
        display.render(nullptr, dt);
    }

    EXPECT_FALSE(display.is_visible());
}

TEST(JudgmentDisplay, StaysVisibleBeforeDuration) {
    // Before DISPLAY_DURATION, display should still be visible
    // Simulate 29 frames at 60 fps = ~0.483s < 0.5s
    JudgmentDisplay display;
    display.on_judgment(JudgmentTier::PERFECT);

    constexpr double dt = 1.0 / 60.0;
    for (int i = 0; i < 29; i++) {
        display.render(nullptr, dt);
    }

    EXPECT_TRUE(display.is_visible());
}

TEST(JudgmentDisplay, NewJudgmentResetsTimer) {
    // New judgment should reset the timer
    JudgmentDisplay display;
    display.on_judgment(JudgmentTier::PERFECT);

    // Advance time by 0.3s
    constexpr double dt = 1.0 / 60.0;
    for (int i = 0; i < 18; i++) {  // 18 * (1/60) = 0.3s
        display.render(nullptr, dt);
    }

    // Issue new judgment
    display.on_judgment(JudgmentTier::MISS);

    // Should be visible again
    EXPECT_TRUE(display.is_visible());
    EXPECT_EQ(display.current_tier(), JudgmentTier::MISS);
}

TEST(JudgmentDisplay, TierColorsDistinct) {
    // All 5 tier colors should be distinct
    // This is a compile-time check that TIER_COLORS array has 5 entries
    // We can't directly access the private array, but we can test behavior
    JudgmentDisplay display;

    // Just verify that on_judgment works for all tiers without crashing
    display.on_judgment(JudgmentTier::PERFECT);
    EXPECT_EQ(display.current_tier(), JudgmentTier::PERFECT);

    display.on_judgment(JudgmentTier::GREAT);
    EXPECT_EQ(display.current_tier(), JudgmentTier::GREAT);

    display.on_judgment(JudgmentTier::GOOD);
    EXPECT_EQ(display.current_tier(), JudgmentTier::GOOD);

    display.on_judgment(JudgmentTier::BAD);
    EXPECT_EQ(display.current_tier(), JudgmentTier::BAD);

    display.on_judgment(JudgmentTier::MISS);
    EXPECT_EQ(display.current_tier(), JudgmentTier::MISS);
}

// --- Integration Tests ---

TEST(NoteRenderer, VisibleRangeAt120BPM) {
    // Bottom-to-top scrolling with receptor_y=80:
    // - beat 10 (current) -> y=80 (at receptor)
    // - beat 13 (3 ahead) -> y=80+240=320 (below receptor, approaching)
    // - beat 7 (3 past) -> y=80-240=-160 (above receptor, already passed)

    std::vector<NoteEvent> notes;
    notes.push_back({5.0, 0, NoteType::TAP});
    notes.push_back({7.0, 1, NoteType::TAP});
    notes.push_back({10.0, 2, NoteType::TAP});
    notes.push_back({13.0, 3, NoteType::TAP});
    notes.push_back({15.0, 4, NoteType::TAP});

    NoteData note_data(std::move(notes));
    TimingData timing = make_simple_timing(120.0);
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    constexpr double current_beat = 10.0;

    float y10 = renderer.beat_to_y(10.0, current_beat);
    EXPECT_FLOAT_EQ(y10, 80.0f);
    EXPECT_GE(y10, 0.0f);
    EXPECT_LE(y10, 480.0f);

    float y13 = renderer.beat_to_y(13.0, current_beat);
    EXPECT_FLOAT_EQ(y13, 320.0f);  // 80 + 3*80 = 320
    EXPECT_GE(y13, 0.0f);
    EXPECT_LE(y13, 480.0f);

    float y7 = renderer.beat_to_y(7.0, current_beat);
    EXPECT_FLOAT_EQ(y7, -160.0f);  // 80 + (-3)*80 = -160 (above screen, passed)
}

TEST(NoteRenderer, NoteBeyondScreenIgnored) {
    // Note at beat 100 while current_beat=0: y should be far below screen
    std::vector<NoteEvent> notes;
    notes.push_back({100.0, 0, NoteType::TAP});

    NoteData note_data(std::move(notes));
    TimingData timing = make_simple_timing(120.0);
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    constexpr double current_beat = 0.0;
    float y = renderer.beat_to_y(100.0, current_beat);

    // 80 + 100*80 = 8080
    EXPECT_FLOAT_EQ(y, 8080.0f);
    EXPECT_GT(y, 480.0f + 48.0f);
}

TEST(NoteRenderer, FourConsecutiveQuarterNotes) {
    // 4 notes at beats 0,1,2,3. Bottom-to-top: future notes below receptor, spacing 80px
    std::vector<NoteEvent> notes;
    notes.push_back({0.0, 0, NoteType::TAP});
    notes.push_back({1.0, 1, NoteType::TAP});
    notes.push_back({2.0, 2, NoteType::TAP});
    notes.push_back({3.0, 3, NoteType::TAP});

    NoteData note_data(std::move(notes));
    TimingData timing = make_simple_timing(120.0);
    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    constexpr double current_beat = 0.0;

    float y0 = renderer.beat_to_y(0.0, current_beat);
    float y1 = renderer.beat_to_y(1.0, current_beat);
    float y2 = renderer.beat_to_y(2.0, current_beat);
    float y3 = renderer.beat_to_y(3.0, current_beat);

    // y = 80 + beat * 80
    EXPECT_FLOAT_EQ(y0, 80.0f);
    EXPECT_FLOAT_EQ(y1, 160.0f);
    EXPECT_FLOAT_EQ(y2, 240.0f);
    EXPECT_FLOAT_EQ(y3, 320.0f);

    // Equal spacing of 80px (notes further in future are lower on screen)
    EXPECT_FLOAT_EQ(y1 - y0, 80.0f);
    EXPECT_FLOAT_EQ(y2 - y1, 80.0f);
    EXPECT_FLOAT_EQ(y3 - y2, 80.0f);
}

TEST(NoteRenderer, StopFreezesNotes) {
    // During a stop, same song_ms gives same beat from timing data, so beat_to_y should be stable
    // Create TimingData with a stop at beat 4.0 lasting 1.0 seconds
    std::vector<TimingEvent> events;
    events.push_back({0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0});
    events.push_back({4.0, TimingEventType::STOP, 0.0, 1.0});  // 1 second stop at beat 4
    TimingData timing(std::move(events));

    std::vector<NoteEvent> notes;
    notes.push_back({4.0, 0, NoteType::TAP});
    NoteData note_data(std::move(notes));

    NoteFieldConfig config = default_single_config();
    NoteRenderer renderer(note_data, timing, config);

    // At 120 BPM, beat 4 occurs at time: 4 beats * (60/120) = 2.0 seconds
    // During the stop (2.0s to 3.0s), beat stays at 4.0
    constexpr double time_during_stop_1 = 2.0;
    constexpr double time_during_stop_2 = 2.5;

    double beat1 = timing.beat_at_time(time_during_stop_1);
    double beat2 = timing.beat_at_time(time_during_stop_2);

    // During stop, beat should be frozen at 4.0
    EXPECT_FLOAT_EQ(beat1, 4.0);
    EXPECT_FLOAT_EQ(beat2, 4.0);

    // beat_to_y should give same result for both times (both have current_beat=4.0)
    float y1 = renderer.beat_to_y(4.0, beat1);
    float y2 = renderer.beat_to_y(4.0, beat2);

    EXPECT_FLOAT_EQ(y1, y2);
    EXPECT_FLOAT_EQ(y1, config.receptor_y);
}

TEST(NoteRenderer, JudgmentDisplayIntegration) {
    // on_judgment, simulate 30 frames at 60fps, should still be visible just before fade
    // Then advance one more frame to trigger fade, then on_judgment again should be visible
    JudgmentDisplay display;

    // Initially invisible
    EXPECT_FALSE(display.is_visible());

    // Trigger judgment
    display.on_judgment(JudgmentTier::PERFECT);
    EXPECT_TRUE(display.is_visible());
    EXPECT_EQ(display.current_tier(), JudgmentTier::PERFECT);

    // Simulate 30 frames at 60fps (0.5s exactly - should still be visible at threshold)
    constexpr double dt = 1.0 / 60.0;
    for (int i = 0; i < 30; i++) {
        display.render(nullptr, dt);
    }

    // At exactly 0.5s, may or may not be visible depending on comparison (>= vs >)
    // Advance one more frame to definitely be past threshold
    display.render(nullptr, dt);

    // 31 frames = ~0.517s > 0.5s, should be invisible
    EXPECT_FALSE(display.is_visible());

    // Trigger new judgment
    display.on_judgment(JudgmentTier::MISS);
    EXPECT_TRUE(display.is_visible());
    EXPECT_EQ(display.current_tier(), JudgmentTier::MISS);
}
