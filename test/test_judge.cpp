#include <gtest/gtest.h>

#include <openitup/judge/judgment_tier.h>
#include <openitup/judge/timing_profile.h>
#include <openitup/judge/judgment_event.h>

#include <algorithm>
#include <vector>

using namespace openitup;

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
