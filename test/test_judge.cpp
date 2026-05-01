#include <gtest/gtest.h>

#include <openitup/judge/judgment_tier.h>
#include <openitup/judge/timing_profile.h>
#include <openitup/judge/judgment_event.h>
#include <openitup/judge/judge.h>
#include <openitup/judge/gameplay_state.h>

#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>
#include <openitup/chart/note_type.h>

#include <algorithm>
#include <vector>
#include <utility>

using namespace openitup;

// --- Helper functions for building test fixtures ---

// Helper: build a simple NoteData with N tap notes at given beats/columns
NoteData make_notes(std::vector<std::pair<double, uint8_t>> beat_col_pairs) {
    std::vector<NoteEvent> events;
    for (auto& [beat, col] : beat_col_pairs) {
        events.push_back({beat, col, NoteType::TAP});
    }
    std::sort(events.begin(), events.end());
    return NoteData(std::move(events));
}

// Helper: build a simple 120 BPM TimingData
TimingData make_simple_timing(double bpm = 120.0) {
    std::vector<TimingEvent> events;
    events.push_back({0.0, TimingEventType::BPM_CHANGE, bpm, 0.0});
    return TimingData(std::move(events));
}

// --- JudgmentTier Tests ---

TEST(Judge, AllTiersExist) {
    // Verify all 5 judgment tiers are defined and distinct
    JudgmentTier perfect = JudgmentTier::PERFECT;
    JudgmentTier great = JudgmentTier::GREAT;
    JudgmentTier good = JudgmentTier::GOOD;
    JudgmentTier bad = JudgmentTier::BAD;
    JudgmentTier miss = JudgmentTier::MISS;

    EXPECT_NE(perfect, great);
    EXPECT_NE(perfect, good);
    EXPECT_NE(great, good);
    EXPECT_NE(good, bad);
    EXPECT_NE(bad, miss);
}

TEST(Judge, TierIntegerValues) {
    // Verify the integer values match the spec (used as array indices)
    EXPECT_EQ(static_cast<uint8_t>(JudgmentTier::PERFECT), 0);
    EXPECT_EQ(static_cast<uint8_t>(JudgmentTier::GREAT), 1);
    EXPECT_EQ(static_cast<uint8_t>(JudgmentTier::GOOD), 2);
    EXPECT_EQ(static_cast<uint8_t>(JudgmentTier::BAD), 3);
    EXPECT_EQ(static_cast<uint8_t>(JudgmentTier::MISS), 4);
}

TEST(Judge, TierCount) {
    // Verify the count constant is correct
    EXPECT_EQ(JUDGMENT_TIER_COUNT, 5);
}

TEST(Judge, TierMaintainsCombo_Perfect) {
    // PERFECT judgments maintain combo
    EXPECT_TRUE(tier_maintains_combo(JudgmentTier::PERFECT));
}

TEST(Judge, TierMaintainsCombo_Great) {
    // GREAT judgments maintain combo
    EXPECT_TRUE(tier_maintains_combo(JudgmentTier::GREAT));
}

TEST(Judge, TierMaintainsCombo_Good) {
    // GOOD judgments maintain combo
    EXPECT_TRUE(tier_maintains_combo(JudgmentTier::GOOD));
}

TEST(Judge, TierBreaksCombo_Bad) {
    // BAD judgments break combo
    EXPECT_FALSE(tier_maintains_combo(JudgmentTier::BAD));
}

TEST(Judge, TierBreaksCombo_Miss) {
    // MISS judgments break combo
    EXPECT_FALSE(tier_maintains_combo(JudgmentTier::MISS));
}

TEST(Judge, StringRoundTrip) {
    // Verify string conversions are inverse for all tiers
    EXPECT_EQ(judgment_tier_from_string(judgment_tier_to_string(JudgmentTier::PERFECT)), JudgmentTier::PERFECT);
    EXPECT_EQ(judgment_tier_from_string(judgment_tier_to_string(JudgmentTier::GREAT)), JudgmentTier::GREAT);
    EXPECT_EQ(judgment_tier_from_string(judgment_tier_to_string(JudgmentTier::GOOD)), JudgmentTier::GOOD);
    EXPECT_EQ(judgment_tier_from_string(judgment_tier_to_string(JudgmentTier::BAD)), JudgmentTier::BAD);
    EXPECT_EQ(judgment_tier_from_string(judgment_tier_to_string(JudgmentTier::MISS)), JudgmentTier::MISS);
}

// --- TimingProfile Tests ---

TEST(Judge, DefaultProfileValues) {
    // Verify default_timing_profile() returns Exceed-era values
    TimingProfile profile = default_timing_profile();
    EXPECT_DOUBLE_EQ(profile.perfect_window_ms, 16.0);
    EXPECT_DOUBLE_EQ(profile.great_window_ms, 33.0);
    EXPECT_DOUBLE_EQ(profile.good_window_ms, 66.0);
    EXPECT_DOUBLE_EQ(profile.bad_window_ms, 100.0);
}

TEST(Judge, DefaultProfileIsValid) {
    // Verify the default profile is valid
    TimingProfile profile = default_timing_profile();
    EXPECT_TRUE(profile.is_valid());
}

TEST(Judge, InvalidProfileNegativeWindow) {
    // Negative window values should be invalid
    TimingProfile profile{-16.0, 33.0, 66.0, 100.0};
    EXPECT_FALSE(profile.is_valid());
}

TEST(Judge, InvalidProfileUnordered) {
    // Windows must be ordered: perfect <= great <= good <= bad
    TimingProfile profile{50.0, 33.0, 66.0, 100.0};
    EXPECT_FALSE(profile.is_valid());
}

TEST(Judge, InvalidProfileZeroWindow) {
    // Zero window values should be invalid
    TimingProfile profile{0.0, 33.0, 66.0, 100.0};
    EXPECT_FALSE(profile.is_valid());
}

TEST(Judge, ValidProfileAllEqual) {
    // All windows equal is valid (degenerate case)
    TimingProfile profile{16.0, 16.0, 16.0, 16.0};
    EXPECT_TRUE(profile.is_valid());
}

TEST(Judge, ValidProfileMinimal) {
    // Small but strictly ordered windows are valid
    TimingProfile profile{1.0, 2.0, 3.0, 4.0};
    EXPECT_TRUE(profile.is_valid());
}

// --- JudgmentEvent Tests ---

TEST(Judge, JudgmentEventFieldsAccessible) {
    JudgmentEvent event(42, 3, 8.5, JudgmentTier::GREAT, 25.0, false);

    EXPECT_EQ(event.note_index(), 42);
    EXPECT_EQ(event.column(), 3);
    EXPECT_EQ(event.beat(), 8.5);
    EXPECT_EQ(event.tier(), JudgmentTier::GREAT);
    EXPECT_EQ(event.timing_error_ms(), 25.0);
    EXPECT_FALSE(event.is_auto_miss());
}

TEST(Judge, JudgmentEventNegativeErrorIsEarly) {
    JudgmentEvent event(0, 0, 4.0, JudgmentTier::PERFECT, -5.5, false);
    EXPECT_LT(event.timing_error_ms(), 0.0);
}

TEST(Judge, JudgmentEventPositiveErrorIsLate) {
    JudgmentEvent event(0, 0, 4.0, JudgmentTier::GREAT, 20.3, false);
    EXPECT_GT(event.timing_error_ms(), 0.0);
}

TEST(Judge, JudgmentEventZeroErrorIsExact) {
    JudgmentEvent event(0, 0, 4.0, JudgmentTier::PERFECT, 0.0, false);
    EXPECT_EQ(event.timing_error_ms(), 0.0);
}

TEST(Judge, JudgmentEventSortByBeat) {
    JudgmentEvent event1(0, 0, 5.0, JudgmentTier::PERFECT, 0.0, false);
    JudgmentEvent event2(1, 0, 3.0, JudgmentTier::PERFECT, 0.0, false);
    JudgmentEvent event3(2, 0, 4.0, JudgmentTier::PERFECT, 0.0, false);

    std::vector<JudgmentEvent> events = {event1, event2, event3};
    std::sort(events.begin(), events.end());

    EXPECT_EQ(events[0].beat(), 3.0);
    EXPECT_EQ(events[1].beat(), 4.0);
    EXPECT_EQ(events[2].beat(), 5.0);
}

TEST(Judge, JudgmentEventSortStableByColumn) {
    JudgmentEvent event1(0, 5, 4.0, JudgmentTier::PERFECT, 0.0, false);
    JudgmentEvent event2(1, 2, 4.0, JudgmentTier::PERFECT, 0.0, false);
    JudgmentEvent event3(2, 7, 4.0, JudgmentTier::PERFECT, 0.0, false);

    std::vector<JudgmentEvent> events = {event1, event2, event3};
    std::sort(events.begin(), events.end());

    EXPECT_EQ(events[0].column(), 2);
    EXPECT_EQ(events[1].column(), 5);
    EXPECT_EQ(events[2].column(), 7);
}

TEST(Judge, JudgmentEventAutoMissFlag) {
    JudgmentEvent auto_miss(0, 0, 4.0, JudgmentTier::MISS, 100.0, true);
    JudgmentEvent input_miss(1, 0, 5.0, JudgmentTier::MISS, 150.0, false);

    EXPECT_TRUE(auto_miss.is_auto_miss());
    EXPECT_FALSE(input_miss.is_auto_miss());
}

TEST(Judge, JudgmentEventCopyAndMove) {
    JudgmentEvent original(42, 3, 8.5, JudgmentTier::GREAT, 25.0, false);

    JudgmentEvent copy(original);
    EXPECT_EQ(copy.note_index(), original.note_index());
    EXPECT_EQ(copy.tier(), original.tier());

    JudgmentEvent moved(std::move(copy));
    EXPECT_EQ(moved.note_index(), original.note_index());
    EXPECT_EQ(moved.tier(), original.tier());
}

// --- Judge Classification Tests ---

TEST(Judge, ClassifyExactHit) {
    // Note at beat 4.0 = 2000 ms at 120 BPM
    NoteData notes = make_notes({{4.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Input at exactly 2000 ms (0.0 ms error) should be PERFECT
    uint32_t pressed = 1u << 0;
    auto events = judge.update(2000.0, pressed);

    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].tier(), JudgmentTier::PERFECT);
    EXPECT_NEAR(events[0].timing_error_ms(), 0.0, 0.1);
}

TEST(Judge, ClassifyPerfectBoundary) {
    // Note at beat 4.0 = 2000 ms at 120 BPM
    NoteData notes = make_notes({{4.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Input at 2016 ms (16.0 ms error) should be PERFECT (boundary inclusive)
    uint32_t pressed = 1u << 0;
    auto events = judge.update(2016.0, pressed);

    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].tier(), JudgmentTier::PERFECT);
    EXPECT_NEAR(events[0].timing_error_ms(), 16.0, 0.1);
}

TEST(Judge, ClassifyGreatJustOutsidePerfect) {
    // Note at beat 4.0 = 2000 ms at 120 BPM
    NoteData notes = make_notes({{4.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Input at 2016.1 ms (16.1 ms error) should be GREAT
    uint32_t pressed = 1u << 0;
    auto events = judge.update(2016.1, pressed);

    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].tier(), JudgmentTier::GREAT);
}

TEST(Judge, ClassifyBadBoundary) {
    // Note at beat 4.0 = 2000 ms at 120 BPM
    NoteData notes = make_notes({{4.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Input at 2100 ms (100.0 ms error) should be BAD (boundary inclusive)
    uint32_t pressed = 1u << 0;
    auto events = judge.update(2100.0, pressed);

    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].tier(), JudgmentTier::BAD);
    EXPECT_NEAR(events[0].timing_error_ms(), 100.0, 0.1);
}

TEST(Judge, ClassifyMissBeyondBad) {
    // Note at beat 4.0 = 2000 ms at 120 BPM
    NoteData notes = make_notes({{4.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Input at 2100.1 ms (100.1 ms error) is beyond the bad window.
    // The auto-miss logic will trigger and emit a MISS event.
    uint32_t pressed = 1u << 0;
    auto events = judge.update(2100.1, pressed);

    // Should get 1 auto-miss event
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].tier(), JudgmentTier::MISS);
    EXPECT_TRUE(events[0].is_auto_miss());
}

// --- Judge Single Note Update Tests ---

TEST(Judge, SingleNotePerfectHit) {
    // Note at beat 4.0
    // At 120 BPM: 60/120 = 0.5 seconds per beat
    // Beat 4.0 = 4 * 0.5 = 2.0 seconds = 2000 ms
    NoteData notes = make_notes({{4.0, 2}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Input at 2001 ms (1ms late) on column 2
    uint32_t pressed = 1u << 2; // column 2
    auto events = judge.update(2001.0, pressed);

    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].tier(), JudgmentTier::PERFECT);
    EXPECT_EQ(events[0].column(), 2);
    EXPECT_NEAR(events[0].timing_error_ms(), 1.0, 0.1);
}

TEST(Judge, SingleNoteGreatHit) {
    // Note at beat 4.0 = 2000 ms at 120 BPM
    NoteData notes = make_notes({{4.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Input 25ms late
    uint32_t pressed = 1u << 0; // column 0
    auto events = judge.update(2025.0, pressed);

    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].tier(), JudgmentTier::GREAT);
    EXPECT_NEAR(events[0].timing_error_ms(), 25.0, 0.1);
}

TEST(Judge, WrongColumnNoMatch) {
    // Note at beat 4.0, column 2
    NoteData notes = make_notes({{4.0, 2}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Input on column 0 (wrong column)
    uint32_t pressed = 1u << 0; // column 0
    auto events = judge.update(2000.0, pressed);

    // No event should be emitted
    EXPECT_EQ(events.size(), 0);
}

TEST(Judge, MultipleColumnsOneTick) {
    // 3 notes on 3 different columns at the same beat
    NoteData notes = make_notes({{4.0, 0}, {4.0, 1}, {4.0, 2}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Press all 3 columns
    uint32_t pressed = (1u << 0) | (1u << 1) | (1u << 2);
    auto events = judge.update(2000.0, pressed);

    // Should get 3 events
    ASSERT_EQ(events.size(), 3);
    EXPECT_EQ(events[0].column(), 0);
    EXPECT_EQ(events[1].column(), 1);
    EXPECT_EQ(events[2].column(), 2);
}

TEST(Judge, NoInputsNoEvents) {
    NoteData notes = make_notes({{4.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // No inputs (pressed_columns = 0)
    auto events = judge.update(2000.0, 0);

    EXPECT_EQ(events.size(), 0);
}

// --- Auto-Miss and Flush Tests ---

TEST(Judge, AutoMissWhenNoInput) {
    // Note at beat 4.0 = 2000 ms at 120 BPM
    NoteData notes = make_notes({{4.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Advance to 2200 ms (200ms past note, beyond 100ms bad window) with no input
    auto events = judge.update(2200.0, 0);

    // Should get an auto-miss event
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].tier(), JudgmentTier::MISS);
    EXPECT_TRUE(events[0].is_auto_miss());
    EXPECT_NEAR(events[0].timing_error_ms(), 100.0, 0.1);
}

TEST(Judge, AutoMissIsAutoMissFlag) {
    // Note at beat 4.0 = 2000 ms at 120 BPM
    NoteData notes = make_notes({{4.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Advance past the note without input
    auto events = judge.update(2150.0, 0);

    ASSERT_EQ(events.size(), 1);
    EXPECT_TRUE(events[0].is_auto_miss());
}

TEST(Judge, MissedNoteDoesntBlockFuture) {
    // Two notes: one at beat 4.0, another at beat 8.0
    // At 120 BPM: beat 4.0 = 2000ms, beat 8.0 = 4000ms
    NoteData notes = make_notes({{4.0, 0}, {8.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Advance past first note without input (auto-miss)
    auto events1 = judge.update(2200.0, 0);
    ASSERT_EQ(events1.size(), 1);
    EXPECT_EQ(events1[0].beat(), 4.0);
    EXPECT_EQ(events1[0].tier(), JudgmentTier::MISS);

    // Now hit the second note normally
    uint32_t pressed = 1u << 0;
    auto events2 = judge.update(4000.0, pressed);
    ASSERT_EQ(events2.size(), 1);
    EXPECT_EQ(events2[0].beat(), 8.0);
    EXPECT_EQ(events2[0].tier(), JudgmentTier::PERFECT);
}

TEST(Judge, FlushRemainingAllMiss) {
    // 5 unjudged notes
    NoteData notes = make_notes({{4.0, 0}, {8.0, 1}, {12.0, 2}, {16.0, 3}, {20.0, 4}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Flush without playing
    auto events = judge.flush_remaining();

    // Should get 5 MISS events
    ASSERT_EQ(events.size(), 5);
    for (const auto& event : events) {
        EXPECT_EQ(event.tier(), JudgmentTier::MISS);
        EXPECT_TRUE(event.is_auto_miss());
    }
}

TEST(Judge, FlushAfterPartialPlay) {
    // 5 notes
    NoteData notes = make_notes({{4.0, 0}, {8.0, 1}, {12.0, 2}, {16.0, 3}, {20.0, 4}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Hit 3 notes
    auto events1 = judge.update(2000.0, 1u << 0);
    auto events2 = judge.update(4000.0, 1u << 1);
    auto events3 = judge.update(6000.0, 1u << 2);

    EXPECT_EQ(events1.size(), 1);
    EXPECT_EQ(events2.size(), 1);
    EXPECT_EQ(events3.size(), 1);

    // Flush remaining (should get 2 misses)
    auto flush_events = judge.flush_remaining();
    ASSERT_EQ(flush_events.size(), 2);
    EXPECT_EQ(flush_events[0].beat(), 16.0);
    EXPECT_EQ(flush_events[1].beat(), 20.0);
}

TEST(Judge, AllNotesJudgedAtSongEnd) {
    // 5 notes
    NoteData notes = make_notes({{4.0, 0}, {8.0, 1}, {12.0, 2}, {16.0, 3}, {20.0, 4}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Hit some notes
    judge.update(2000.0, 1u << 0);
    judge.update(4000.0, 1u << 1);
    judge.update(6000.0, 1u << 2);

    // Flush rest
    judge.flush_remaining();

    // All notes should be judged
    EXPECT_TRUE(judge.is_complete());
    EXPECT_EQ(judge.judged_count(), judge.total_judgable());
}

TEST(Judge, NoAutoMissBeforeWindow) {
    // Note at beat 4.0 = 2000 ms at 120 BPM
    NoteData notes = make_notes({{4.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Advance to 2050 ms (only 50ms past note, within 100ms bad window)
    auto events = judge.update(2050.0, 0);

    // Should NOT get an auto-miss yet (still within judgable window)
    EXPECT_EQ(events.size(), 0);
}

// --- Integration Tests: Judge + GameplayState Pipeline ---

TEST(Judge, FullPerfectSong) {
    // Build 10 tap notes at regular beat intervals (beat 0, 2, 4, 6, 8, 10, 12, 14, 16, 18)
    // cycling through columns 0-4
    std::vector<std::pair<double, uint8_t>> notes_spec;
    for (int i = 0; i < 10; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        notes_spec.push_back({beat, column});
    }
    NoteData notes = make_notes(notes_spec);

    // Build 120 BPM TimingData (each beat = 500ms)
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();

    // Create Judge and GameplayState(10)
    Judge judge(notes, timing, profile);
    GameplayState state(10);

    // For each note: compute exact note_time_ms, call judge.update(note_time_ms, column_bitmask), apply events to state
    for (int i = 0; i < 10; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        double note_time_ms = timing.time_at_beat(beat) * 1000.0;
        uint32_t column_bitmask = 1u << column;

        auto events = judge.update(note_time_ms, column_bitmask);
        state.apply(events);
    }

    // Verify: score = 10000, max_combo = 10, judgment_count(PERFECT) = 10, is_complete = true
    EXPECT_EQ(state.score(), 10000);
    EXPECT_EQ(state.max_combo(), 10);
    EXPECT_EQ(state.current_combo(), 10);
    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 10);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GREAT), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GOOD), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::BAD), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::MISS), 0);
    EXPECT_TRUE(judge.is_complete());
}

TEST(Judge, MixedJudgments) {
    // Build 10 notes at regular intervals
    std::vector<std::pair<double, uint8_t>> notes_spec;
    for (int i = 0; i < 10; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        notes_spec.push_back({beat, column});
    }
    NoteData notes = make_notes(notes_spec);

    // Build 120 BPM TimingData
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();

    Judge judge(notes, timing, profile);
    GameplayState state(10);

    // Hit with varying timing errors:
    // Notes 0-2: exact (PERFECT)
    for (int i = 0; i < 3; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        double note_time_ms = timing.time_at_beat(beat) * 1000.0;
        uint32_t column_bitmask = 1u << column;

        auto events = judge.update(note_time_ms, column_bitmask);
        state.apply(events);
    }

    // Notes 3-4: +25ms (GREAT)
    for (int i = 3; i < 5; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        double note_time_ms = timing.time_at_beat(beat) * 1000.0 + 25.0;
        uint32_t column_bitmask = 1u << column;

        auto events = judge.update(note_time_ms, column_bitmask);
        state.apply(events);
    }

    // Notes 5-6: +50ms (GOOD)
    for (int i = 5; i < 7; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        double note_time_ms = timing.time_at_beat(beat) * 1000.0 + 50.0;
        uint32_t column_bitmask = 1u << column;

        auto events = judge.update(note_time_ms, column_bitmask);
        state.apply(events);
    }

    // Note 7: +80ms (BAD)
    {
        int i = 7;
        double beat = i * 2.0;
        uint8_t column = i % 5;
        double note_time_ms = timing.time_at_beat(beat) * 1000.0 + 80.0;
        uint32_t column_bitmask = 1u << column;

        auto events = judge.update(note_time_ms, column_bitmask);
        state.apply(events);
    }

    // Notes 8-9: skip (auto-miss by advancing past window)
    // Advance past beat 18 + bad window
    double last_note_beat = 18.0;
    double last_note_time_ms = timing.time_at_beat(last_note_beat) * 1000.0;
    double advance_time_ms = last_note_time_ms + 150.0; // beyond 100ms window
    auto events = judge.update(advance_time_ms, 0);
    state.apply(events);

    // Flush to get remaining misses
    auto flush_events = judge.flush_remaining();
    state.apply(flush_events);

    // Verify: score = 3*1000 + 2*800 + 2*500 + 1*100 + 2*0 = 5700
    EXPECT_EQ(state.score(), 5700);

    // Verify: max_combo = 7 (3+2+2 before BAD), current_combo = 0
    EXPECT_EQ(state.max_combo(), 7);
    EXPECT_EQ(state.current_combo(), 0);

    // Verify judgment counts
    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 3);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GREAT), 2);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GOOD), 2);
    EXPECT_EQ(state.judgment_count(JudgmentTier::BAD), 1);
    EXPECT_EQ(state.judgment_count(JudgmentTier::MISS), 2);

    EXPECT_TRUE(judge.is_complete());
}

TEST(Judge, DeterminismTest) {
    // Build a chart with 5 notes
    std::vector<std::pair<double, uint8_t>> notes_spec;
    for (int i = 0; i < 5; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        notes_spec.push_back({beat, column});
    }
    NoteData notes = make_notes(notes_spec);
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();

    // Run scenario 1
    Judge judge1(notes, timing, profile);
    GameplayState state1(5);

    for (int i = 0; i < 5; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        double note_time_ms = timing.time_at_beat(beat) * 1000.0 + 10.0; // +10ms
        uint32_t column_bitmask = 1u << column;

        auto events = judge1.update(note_time_ms, column_bitmask);
        state1.apply(events);
    }

    // Run scenario 2 (identical inputs)
    Judge judge2(notes, timing, profile);
    GameplayState state2(5);

    for (int i = 0; i < 5; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        double note_time_ms = timing.time_at_beat(beat) * 1000.0 + 10.0; // +10ms
        uint32_t column_bitmask = 1u << column;

        auto events = judge2.update(note_time_ms, column_bitmask);
        state2.apply(events);
    }

    // Verify: both runs produce identical results
    EXPECT_EQ(state1.score(), state2.score());
    EXPECT_EQ(state1.max_combo(), state2.max_combo());
    EXPECT_EQ(state1.current_combo(), state2.current_combo());
    EXPECT_EQ(state1.judgment_count(JudgmentTier::PERFECT), state2.judgment_count(JudgmentTier::PERFECT));
    EXPECT_EQ(state1.judgment_count(JudgmentTier::GREAT), state2.judgment_count(JudgmentTier::GREAT));
    EXPECT_EQ(state1.judgment_count(JudgmentTier::GOOD), state2.judgment_count(JudgmentTier::GOOD));
    EXPECT_EQ(state1.judgment_count(JudgmentTier::BAD), state2.judgment_count(JudgmentTier::BAD));
    EXPECT_EQ(state1.judgment_count(JudgmentTier::MISS), state2.judgment_count(JudgmentTier::MISS));
}

TEST(Judge, JudgeResetAndReplay) {
    // Build a chart with 5 notes
    std::vector<std::pair<double, uint8_t>> notes_spec;
    for (int i = 0; i < 5; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        notes_spec.push_back({beat, column});
    }
    NoteData notes = make_notes(notes_spec);
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();

    Judge judge(notes, timing, profile);
    GameplayState state(5);

    // First playthrough
    for (int i = 0; i < 5; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        double note_time_ms = timing.time_at_beat(beat) * 1000.0 + 5.0;
        uint32_t column_bitmask = 1u << column;

        auto events = judge.update(note_time_ms, column_bitmask);
        state.apply(events);
    }

    int64_t first_score = state.score();
    int first_max_combo = state.max_combo();

    // Reset both judge and state
    judge.reset();
    state.reset();

    // Second playthrough (identical inputs)
    for (int i = 0; i < 5; i++) {
        double beat = i * 2.0;
        uint8_t column = i % 5;
        double note_time_ms = timing.time_at_beat(beat) * 1000.0 + 5.0;
        uint32_t column_bitmask = 1u << column;

        auto events = judge.update(note_time_ms, column_bitmask);
        state.apply(events);
    }

    // Verify: second run produces identical results
    EXPECT_EQ(state.score(), first_score);
    EXPECT_EQ(state.max_combo(), first_max_combo);
}

TEST(Judge, BpmChangeChart) {
    // Build chart with notes before and after BPM change
    // BPM change at beat 8: 120 → 180
    std::vector<std::pair<double, uint8_t>> notes_spec;
    // 3 notes before BPM change
    notes_spec.push_back({2.0, 0});
    notes_spec.push_back({4.0, 1});
    notes_spec.push_back({6.0, 2});
    // 3 notes after BPM change
    notes_spec.push_back({10.0, 3});
    notes_spec.push_back({12.0, 4});
    notes_spec.push_back({14.0, 0});

    NoteData notes = make_notes(notes_spec);

    // Build TimingData with BPM change at beat 8
    std::vector<TimingEvent> timing_events;
    timing_events.push_back({0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0});
    timing_events.push_back({8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0});
    TimingData timing(std::move(timing_events));

    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);
    GameplayState state(6);

    // Hit all notes at exact times
    std::vector<std::pair<double, uint8_t>> test_notes = {
        {2.0, 0}, {4.0, 1}, {6.0, 2}, {10.0, 3}, {12.0, 4}, {14.0, 0}
    };

    for (const auto& [beat, column] : test_notes) {
        double note_time_ms = timing.time_at_beat(beat) * 1000.0;
        uint32_t column_bitmask = 1u << column;

        auto events = judge.update(note_time_ms, column_bitmask);
        state.apply(events);
    }

    // Verify: all PERFECT, timing errors near 0.0
    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 6);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GREAT), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GOOD), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::BAD), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::MISS), 0);
    EXPECT_EQ(state.score(), 6000);
    EXPECT_EQ(state.max_combo(), 6);
    EXPECT_TRUE(judge.is_complete());
}

// --- Hold Note Head Judgment Tests (US-JDG-007) ---

// Helper: build NoteData with hold notes (head + tail pairs)
NoteData make_hold_notes(std::vector<std::tuple<double, double, uint8_t>> head_tail_col_tuples) {
    std::vector<NoteEvent> events;
    for (auto& [head_beat, tail_beat, col] : head_tail_col_tuples) {
        events.push_back({head_beat, col, NoteType::HOLD_HEAD});
        events.push_back({tail_beat, col, NoteType::HOLD_TAIL});
    }
    std::sort(events.begin(), events.end());
    return NoteData(std::move(events));
}

TEST(Judge, HoldHeadActivatesAfterJudgment) {
    // US-JDG-007 Scenario 1: Hold activates after head judgment
    // Given a hold note with head at beat 4.0
    // When input arrives within Great window
    // Then judgment event "Great" is emitted and hold state transitions to "Active"

    NoteData notes = make_hold_notes({{4.0, 6.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Head at beat 4.0 = 2000 ms at 120 BPM
    // Hit at 2020 ms (20 ms late, within Great window)
    uint32_t pressed = 1u << 0;
    auto events = judge.update(2020.0, pressed);

    // Verify judgment event
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].tier(), JudgmentTier::GREAT);
    EXPECT_NEAR(events[0].timing_error_ms(), 20.0, 0.1);
    EXPECT_EQ(events[0].beat(), 4.0);
    EXPECT_EQ(events[0].column(), 0);

    // Verify hold state is active
    const auto& holds = judge.active_holds();
    ASSERT_EQ(holds.size(), 1);
    EXPECT_EQ(holds[0].column, 0);
    EXPECT_EQ(holds[0].tail_beat, 6.0);
    EXPECT_TRUE(holds[0].active);
    EXPECT_EQ(holds[0].head_tier, JudgmentTier::GREAT);
}

TEST(Judge, HoldDoesNotActivateOnMissedHead) {
    // US-JDG-007 Scenario 2: Hold does not activate on missed head
    // Given a hold note with head at beat 4.0
    // When no input arrives and head auto-misses at beat 4.2
    // Then judgment event "Miss" is emitted and hold state remains "Inactive"

    NoteData notes = make_hold_notes({{4.0, 6.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Head at beat 4.0 = 2000 ms at 120 BPM
    // Auto-miss threshold: 2000 ms + bad_window (100 ms) = 2100 ms
    // Advance past the miss window without pressing
    uint32_t no_press = 0u;
    auto events = judge.update(2101.0, no_press);

    // Verify auto-miss event
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].tier(), JudgmentTier::MISS);
    EXPECT_TRUE(events[0].is_auto_miss());

    // Verify no hold state was created
    const auto& holds = judge.active_holds();
    EXPECT_EQ(holds.size(), 0);
}

TEST(Judge, HoldHeadTimingErrorIncluded) {
    // US-JDG-007 Scenario 3: Head timing error included in event
    // Given a hold note head at 1000ms
    // When input arrives at 1012ms
    // Then judgment event includes timing_error +12ms

    NoteData notes = make_hold_notes({{2.0, 4.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Head at beat 2.0 = 1000 ms at 120 BPM
    // Hit at 1012 ms (12 ms late)
    uint32_t pressed = 1u << 0;
    auto events = judge.update(1012.0, pressed);

    // Verify timing error
    ASSERT_EQ(events.size(), 1);
    EXPECT_NEAR(events[0].timing_error_ms(), 12.0, 0.1);
    EXPECT_EQ(events[0].tier(), JudgmentTier::PERFECT);
}

TEST(Judge, HoldBodyNotScoredUntilHeadJudged) {
    // US-JDG-007 Scenario 4: Hold body not scored until head judged
    // Given a hold note with head at beat 4.0 and tail at beat 6.0
    // When player holds panel continuously from beat 3.5 through 6.0
    // Then hold body score begins accumulating only after head is judged at beat 4.0
    //
    // Note: This test verifies that no hold state exists before head judgment,
    // and that hold state is created only after the head is judged.
    // Body scoring logic (US-JDG-008) will use this state.

    NoteData notes = make_hold_notes({{4.0, 6.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Head at beat 4.0 = 2000 ms at 120 BPM
    // Before head is judged (at beat 3.5 = 1750 ms)
    uint32_t pressed = 1u << 0;
    auto events_early = judge.update(1750.0, pressed);

    // No judgment event yet (too early, outside bad window)
    EXPECT_EQ(events_early.size(), 0);

    // No hold state should exist yet
    EXPECT_EQ(judge.active_holds().size(), 0);

    // Now judge the head at exact timing (2000 ms)
    auto events_head = judge.update(2000.0, pressed);

    // Head is judged
    ASSERT_EQ(events_head.size(), 1);
    EXPECT_EQ(events_head[0].tier(), JudgmentTier::PERFECT);

    // Now hold state exists
    EXPECT_EQ(judge.active_holds().size(), 1);
}

TEST(Judge, MultipleActiveHolds) {
    // Test multiple hold notes active simultaneously on different columns
    NoteData notes = make_hold_notes({
        {4.0, 8.0, 0},
        {5.0, 9.0, 1},
        {6.0, 10.0, 2}
    });
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Judge first hold head at beat 4.0 (2000 ms)
    uint32_t press_col0 = 1u << 0;
    auto events1 = judge.update(2000.0, press_col0);
    ASSERT_EQ(events1.size(), 1);
    EXPECT_EQ(judge.active_holds().size(), 1);

    // Judge second hold head at beat 5.0 (2500 ms)
    uint32_t press_col1 = 1u << 1;
    auto events2 = judge.update(2500.0, press_col1);
    ASSERT_EQ(events2.size(), 1);
    EXPECT_EQ(judge.active_holds().size(), 2);

    // Judge third hold head at beat 6.0 (3000 ms)
    uint32_t press_col2 = 1u << 2;
    auto events3 = judge.update(3000.0, press_col2);
    ASSERT_EQ(events3.size(), 1);
    EXPECT_EQ(judge.active_holds().size(), 3);

    // Verify all holds are tracked correctly
    const auto& holds = judge.active_holds();
    EXPECT_EQ(holds[0].column, 0);
    EXPECT_EQ(holds[0].tail_beat, 8.0);
    EXPECT_EQ(holds[1].column, 1);
    EXPECT_EQ(holds[1].tail_beat, 9.0);
    EXPECT_EQ(holds[2].column, 2);
    EXPECT_EQ(holds[2].tail_beat, 10.0);
}

TEST(Judge, HoldHeadCountsTowardTotal) {
    // Verify that HOLD_HEAD notes count toward total_judgable
    NoteData notes = make_hold_notes({{4.0, 6.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // HOLD_HEAD should count as 1 judgable note
    // (HOLD_TAIL does not count)
    EXPECT_EQ(judge.total_judgable(), 1);
    EXPECT_EQ(judge.judged_count(), 0);
    EXPECT_FALSE(judge.is_complete());

    // Judge the head
    uint32_t pressed = 1u << 0;
    auto events = judge.update(2000.0, pressed);

    EXPECT_EQ(judge.judged_count(), 1);
    EXPECT_TRUE(judge.is_complete());
}

TEST(Judge, HoldResetClearsActiveHolds) {
    // Verify that reset() clears active hold states
    NoteData notes = make_hold_notes({{4.0, 6.0, 0}});
    TimingData timing = make_simple_timing(120.0);
    TimingProfile profile = default_timing_profile();
    Judge judge(notes, timing, profile);

    // Judge the head to create an active hold
    uint32_t pressed = 1u << 0;
    judge.update(2000.0, pressed);
    EXPECT_EQ(judge.active_holds().size(), 1);

    // Reset the judge
    judge.reset();

    // Active holds should be cleared
    EXPECT_EQ(judge.active_holds().size(), 0);
    EXPECT_EQ(judge.judged_count(), 0);
}
