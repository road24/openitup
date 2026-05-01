#include <gtest/gtest.h>

#include <openitup/chart/note_data.h>
#include <openitup/chart/note_type.h>
#include <openitup/chart/timing_data.h>
#include <openitup/judge/coop_judge.h>
#include <openitup/judge/coop_gameplay_state.h>
#include <openitup/judge/timing_profile.h>

using namespace openitup;

// US-JDG-017: Co-op mode dual judge instances
class CoopJudgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create timing data: 120 BPM (0.5 seconds per beat)
        timing_data_.add_bpm_change(0.0, 120.0);

        // Create notes for both players
        // P1 (columns 0-4): notes at beats 1.0, 2.0
        std::vector<NoteEvent> events;
        events.push_back({1.0, 0, NoteType::TAP});  // P1 down-left
        events.push_back({2.0, 2, NoteType::TAP});  // P1 center

        // P2 (columns 5-9): notes at beats 1.5, 2.5
        events.push_back({1.5, 5, NoteType::TAP});  // P2 down-left (remapped column)
        events.push_back({2.5, 7, NoteType::TAP});  // P2 center (remapped column)

        note_data_ = NoteData(events);

        profile_ = default_timing_profile();
    }

    NoteData note_data_;
    TimingData timing_data_;
    TimingProfile profile_;
};

// US-JDG-017 Scenario 1: Two independent judge instances
TEST_F(CoopJudgeTest, TwoIndependentJudges) {
    CoopJudge coop_judge(note_data_, timing_data_, profile_);

    // P1 should have 2 notes (columns 0-4)
    EXPECT_EQ(coop_judge.p1_judge().total_judgable(), 2);

    // P2 should have 2 notes (columns 5-9)
    EXPECT_EQ(coop_judge.p2_judge().total_judgable(), 2);

    // Combined total should be 4
    EXPECT_EQ(coop_judge.total_judgable(), 4);
}

// US-JDG-017 Scenario 2: Each judge receives own InputSnapshot
TEST_F(CoopJudgeTest, IndependentInputs) {
    CoopJudge coop_judge(note_data_, timing_data_, profile_);

    // Beat 1.0 = 500ms (P1 note in column 0)
    // P1 presses column 0 (bit 0), P2 does nothing
    uint32_t p1_pressed = 0x01;  // Column 0
    uint32_t p1_held = 0x01;
    uint32_t p2_pressed = 0x00;
    uint32_t p2_held = 0x00;

    auto events = coop_judge.update(500.0, p1_pressed, p1_held, p2_pressed, p2_held);

    // Should have 1 judgment event (P1's note)
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].column(), 0);
    EXPECT_EQ(events[0].tier(), JudgmentTier::PERFECT);

    // P1 has judged 1 note, P2 has judged 0
    EXPECT_EQ(coop_judge.p1_judge().judged_count(), 1);
    EXPECT_EQ(coop_judge.p2_judge().judged_count(), 0);
}

// US-JDG-017 Scenario 3: GameplayState aggregates judgments from both
TEST_F(CoopJudgeTest, AggregatedGameplayState) {
    CoopJudge coop_judge(note_data_, timing_data_, profile_);
    CoopGameplayState state(4, CoopLifeMode::SHARED, profile_);

    // Beat 1.0: P1 hits perfectly
    uint32_t p1_pressed = 0x01;  // Column 0
    auto events1 = coop_judge.update(500.0, p1_pressed, 0x01, 0, 0);
    state.apply(events1);

    // Combo should be 1
    EXPECT_EQ(state.current_combo(), 1);
    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 1);

    // Beat 1.5: P2 hits perfectly (column 5)
    uint32_t p2_pressed = 0x01 << 5;  // Column 5
    auto events2 = coop_judge.update(750.0, 0, 0, p2_pressed, p2_pressed);
    state.apply(events2);

    // Combo should be 2 (combined)
    EXPECT_EQ(state.current_combo(), 2);
    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 2);
}

// US-JDG-018 Scenario 1: Shared life gauge mode
TEST_F(CoopJudgeTest, SharedLifeGaugeMode) {
    CoopJudge coop_judge(note_data_, timing_data_, profile_);
    CoopGameplayState state(4, CoopLifeMode::SHARED, profile_);

    EXPECT_EQ(state.life_mode(), CoopLifeMode::SHARED);

    // P1 gets a Miss (drain 0.10 HP)
    JudgmentEvent miss_event(0, 0, 1.0, JudgmentTier::MISS, 100.0, true);
    state.apply_single(miss_event);

    // Shared gauge should decrease
    EXPECT_LT(state.hp(), 1.0f);
    float shared_hp = state.hp();

    // P2 gets a Perfect (recover 0.02 HP)
    JudgmentEvent perfect_event(1, 5, 1.5, JudgmentTier::PERFECT, 0.0, false);
    state.apply_single(perfect_event);

    // Shared gauge should recover slightly
    EXPECT_GT(state.hp(), shared_hp);
}

// US-JDG-018 Scenario 2: Separate life gauge mode
TEST_F(CoopJudgeTest, SeparateLifeGaugeMode) {
    CoopJudge coop_judge(note_data_, timing_data_, profile_);
    CoopGameplayState state(4, CoopLifeMode::SEPARATE, profile_);

    EXPECT_EQ(state.life_mode(), CoopLifeMode::SEPARATE);

    // P1 gets a Miss (column 0)
    JudgmentEvent p1_miss(0, 0, 1.0, JudgmentTier::MISS, 100.0, true);
    state.apply_single(p1_miss);

    // Only P1's gauge should decrease
    float p1_hp = state.hp();
    float p2_hp = state.hp_p2();
    EXPECT_LT(p1_hp, 1.0f);
    EXPECT_FLOAT_EQ(p2_hp, 1.0f) << "P2's HP should remain at 1.0";

    // P2 gets a Perfect (column 5)
    JudgmentEvent p2_perfect(1, 5, 1.5, JudgmentTier::PERFECT, 0.0, false);
    state.apply_single(p2_perfect);

    // P1's HP unchanged, P2's HP should recover (already at 1.0, so stays at 1.0)
    EXPECT_FLOAT_EQ(state.hp(), p1_hp) << "P1's HP should remain unchanged";
    EXPECT_FLOAT_EQ(state.hp_p2(), 1.0f) << "P2's HP should remain at 1.0 (clamped)";
}

// US-JDG-018 Scenario 3: Both players must survive in separate mode
TEST_F(CoopJudgeTest, BothMustSurviveSeparateMode) {
    CoopGameplayState state(4, CoopLifeMode::SEPARATE, default_timing_profile());

    // Drain P1's life to 0
    for (int i = 0; i < 11; ++i) {
        JudgmentEvent miss(i, 0, i * 1.0, JudgmentTier::MISS, 100.0, true);
        state.apply_single(miss);
    }

    // P1's HP should be 0, P2's should still be 1.0
    EXPECT_FLOAT_EQ(state.hp(), 0.0f);
    EXPECT_FLOAT_EQ(state.hp_p2(), 1.0f);

    // Should be failed (US-JDG-018 SC3)
    EXPECT_TRUE(state.is_failed()) << "Should fail when either player's HP reaches 0 in SEPARATE mode";
}

// US-JDG-018 Scenario 4: Failure requires both in shared mode
TEST_F(CoopJudgeTest, FailureBothInSharedMode) {
    CoopGameplayState state(4, CoopLifeMode::SHARED, default_timing_profile());

    // Drain shared life to 0
    for (int i = 0; i < 11; ++i) {
        JudgmentEvent miss(i, 0, i * 1.0, JudgmentTier::MISS, 100.0, true);
        state.apply_single(miss);
    }

    EXPECT_FLOAT_EQ(state.hp(), 0.0f);

    // Should be failed (US-JDG-018 SC4)
    EXPECT_TRUE(state.is_failed()) << "Should fail when shared gauge reaches 0";
}
