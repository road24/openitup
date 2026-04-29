#include <gtest/gtest.h>

#include <openitup/judge/judgment_tier.h>
#include <openitup/judge/timing_profile.h>

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
