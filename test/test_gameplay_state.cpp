#include <gtest/gtest.h>

#include <openitup/judge/gameplay_state.h>
#include <openitup/judge/judgment_event.h>
#include <openitup/judge/judgment_tier.h>

using namespace openitup;

namespace {

JudgmentEvent make_event(JudgmentTier tier, uint8_t column = 0, double beat = 0.0) {
    return JudgmentEvent(0, column, beat, tier, 0.0, false);
}

} // namespace

TEST(GameplayState, InitialStateZero) {
    GameplayState state(10);
    EXPECT_EQ(state.current_combo(), 0);
    EXPECT_EQ(state.max_combo(), 0);
    EXPECT_EQ(state.score(), 0);
    EXPECT_EQ(state.total_judged(), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GREAT), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GOOD), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::BAD), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::MISS), 0);
}

TEST(GameplayState, ComboIncrementsOnPerfect) {
    GameplayState state(10);
    state.apply_single(make_event(JudgmentTier::PERFECT));
    EXPECT_EQ(state.current_combo(), 1);
}

TEST(GameplayState, ComboIncrementsOnGreat) {
    GameplayState state(10);
    state.apply_single(make_event(JudgmentTier::PERFECT));
    state.apply_single(make_event(JudgmentTier::GREAT));
    EXPECT_EQ(state.current_combo(), 2);
}

TEST(GameplayState, ComboIncrementsOnGood) {
    GameplayState state(10);
    state.apply_single(make_event(JudgmentTier::PERFECT));
    state.apply_single(make_event(JudgmentTier::GREAT));
    state.apply_single(make_event(JudgmentTier::GOOD));
    EXPECT_EQ(state.current_combo(), 3);
}

TEST(GameplayState, ComboResetsOnBad) {
    GameplayState state(10);
    for (int i = 0; i < 20; i++) {
        state.apply_single(make_event(JudgmentTier::PERFECT));
    }
    EXPECT_EQ(state.current_combo(), 20);
    state.apply_single(make_event(JudgmentTier::BAD));
    EXPECT_EQ(state.current_combo(), 0);
    EXPECT_EQ(state.max_combo(), 20);
}

TEST(GameplayState, ComboResetsOnMiss) {
    GameplayState state(10);
    for (int i = 0; i < 10; i++) {
        state.apply_single(make_event(JudgmentTier::PERFECT));
    }
    EXPECT_EQ(state.current_combo(), 10);
    state.apply_single(make_event(JudgmentTier::MISS));
    EXPECT_EQ(state.current_combo(), 0);
    EXPECT_EQ(state.max_combo(), 10);
}

TEST(GameplayState, MaxComboTracked) {
    GameplayState state(30);
    // 10 perfects
    for (int i = 0; i < 10; i++) {
        state.apply_single(make_event(JudgmentTier::PERFECT));
    }
    EXPECT_EQ(state.current_combo(), 10);
    EXPECT_EQ(state.max_combo(), 10);

    // Miss
    state.apply_single(make_event(JudgmentTier::MISS));
    EXPECT_EQ(state.current_combo(), 0);
    EXPECT_EQ(state.max_combo(), 10);

    // 15 perfects
    for (int i = 0; i < 15; i++) {
        state.apply_single(make_event(JudgmentTier::PERFECT));
    }
    EXPECT_EQ(state.current_combo(), 15);
    EXPECT_EQ(state.max_combo(), 15);
}

TEST(GameplayState, ScoreIncrementsPerfect) {
    GameplayState state(10);
    state.apply_single(make_event(JudgmentTier::PERFECT));
    EXPECT_EQ(state.score(), 1000);
}

TEST(GameplayState, ScoreIncrementsGreat) {
    GameplayState state(10);
    state.apply_single(make_event(JudgmentTier::GREAT));
    EXPECT_EQ(state.score(), 800);
}

TEST(GameplayState, ScoreIncrementsMiss) {
    GameplayState state(10);
    state.apply_single(make_event(JudgmentTier::MISS));
    EXPECT_EQ(state.score(), 0);
}

TEST(GameplayState, ScoreCumulative) {
    GameplayState state(10);
    state.apply_single(make_event(JudgmentTier::PERFECT));
    state.apply_single(make_event(JudgmentTier::GREAT));
    EXPECT_EQ(state.score(), 1800);
}

TEST(GameplayState, ScorePercentage) {
    GameplayState state(10);
    for (int i = 0; i < 5; i++) {
        state.apply_single(make_event(JudgmentTier::PERFECT));
    }
    EXPECT_DOUBLE_EQ(state.score_percentage(), 50.0);
}

TEST(GameplayState, ScorePercentageZeroNotes) {
    GameplayState state(0);
    EXPECT_DOUBLE_EQ(state.score_percentage(), 0.0);
}

TEST(GameplayState, JudgmentCountsByTier) {
    GameplayState state(10);
    state.apply_single(make_event(JudgmentTier::PERFECT));
    state.apply_single(make_event(JudgmentTier::PERFECT));
    state.apply_single(make_event(JudgmentTier::PERFECT));
    state.apply_single(make_event(JudgmentTier::GREAT));
    state.apply_single(make_event(JudgmentTier::GREAT));
    state.apply_single(make_event(JudgmentTier::MISS));

    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 3);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GREAT), 2);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GOOD), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::BAD), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::MISS), 1);
}

TEST(GameplayState, TotalJudged) {
    GameplayState state(10);
    for (int i = 0; i < 10; i++) {
        state.apply_single(make_event(JudgmentTier::PERFECT));
    }
    EXPECT_EQ(state.total_judged(), 10);
}

TEST(GameplayState, ResetClearsAll) {
    GameplayState state(10);
    for (int i = 0; i < 5; i++) {
        state.apply_single(make_event(JudgmentTier::PERFECT));
    }
    state.apply_single(make_event(JudgmentTier::GREAT));

    EXPECT_GT(state.score(), 0);
    EXPECT_GT(state.current_combo(), 0);
    EXPECT_GT(state.total_judged(), 0);

    state.reset();

    EXPECT_EQ(state.score(), 0);
    EXPECT_EQ(state.current_combo(), 0);
    EXPECT_EQ(state.max_combo(), 0);
    EXPECT_EQ(state.total_judged(), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 0);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GREAT), 0);
}

TEST(GameplayState, ApplyBatch) {
    GameplayState state(10);
    std::vector<JudgmentEvent> events;
    events.push_back(make_event(JudgmentTier::PERFECT, 0, 1.0));
    events.push_back(make_event(JudgmentTier::PERFECT, 1, 2.0));
    events.push_back(make_event(JudgmentTier::GREAT, 2, 3.0));
    events.push_back(make_event(JudgmentTier::GOOD, 3, 4.0));
    events.push_back(make_event(JudgmentTier::BAD, 4, 5.0));

    state.apply(events);

    EXPECT_EQ(state.total_judged(), 5);
    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 2);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GREAT), 1);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GOOD), 1);
    EXPECT_EQ(state.judgment_count(JudgmentTier::BAD), 1);
    EXPECT_EQ(state.current_combo(), 0);
    EXPECT_EQ(state.max_combo(), 4);
    EXPECT_EQ(state.score(), 2000 + 800 + 500 + 100);
}
