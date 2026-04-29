#include <gtest/gtest.h>

#include <openitup/judge/judgment_tier.h>
#include <openitup/judge/timing_profile.h>
#include <openitup/judge/judgment_event.h>
#include <openitup/judge/judge.h>

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
