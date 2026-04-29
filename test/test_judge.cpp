#include <gtest/gtest.h>

#include <openitup/judge/judgment_tier.h>

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
