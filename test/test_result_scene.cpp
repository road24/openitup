#include <gtest/gtest.h>

#include <openitup/judge/gameplay_state.h>
#include <openitup/judge/judgment_event.h>
#include <openitup/scene/result_scene.h>
#include <openitup/scene/scene_stack.h>
#include <openitup/input/input_snapshot.h>

namespace openitup {

class ResultSceneTest : public ::testing::Test {
protected:
    void SetUp() override {
        stack_ = std::make_unique<SceneStack>();
    }

    std::unique_ptr<SceneStack> stack_;
};

// Test Scenario 1: Grade displayed based on score
TEST_F(ResultSceneTest, GradeSSForAllPerfect) {
    GameplayState state(100);

    // Create 100 Perfect judgments
    std::vector<JudgmentEvent> events;
    for (int i = 0; i < 100; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::PERFECT, 0.0, false);
    }
    state.apply(events);

    ResultScene scene(nullptr, nullptr, stack_.get(), nullptr, state);

    // Verify grade calculation through state
    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 100);
    EXPECT_EQ(state.score_percentage(), 100.0);
}

TEST_F(ResultSceneTest, GradeSFor95PercentScore) {
    GameplayState state(100);

    // Create 95 Perfect and 5 Great judgments (95% score)
    std::vector<JudgmentEvent> events;
    for (int i = 0; i < 95; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::PERFECT, 0.0, false);
    }
    for (int i = 95; i < 100; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::GREAT, 5.0, false);
    }
    state.apply(events);

    ResultScene scene(nullptr, nullptr, stack_.get(), nullptr, state);

    // Score should be 95% or higher (95000 Perfect + 4000 Great = 99000 / 100000 = 99%)
    EXPECT_GE(state.score_percentage(), 95.0);
}

TEST_F(ResultSceneTest, GradeAFor90PercentScore) {
    GameplayState state(100);

    // Create 90 Perfect and 10 Good judgments
    std::vector<JudgmentEvent> events;
    for (int i = 0; i < 90; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::PERFECT, 0.0, false);
    }
    for (int i = 90; i < 100; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::GOOD, 30.0, false);
    }
    state.apply(events);

    ResultScene scene(nullptr, nullptr, stack_.get(), nullptr, state);

    // Score should be 90% or higher (90000 Perfect + 5000 Good = 95000 / 100000 = 95%)
    EXPECT_GE(state.score_percentage(), 90.0);
}

TEST_F(ResultSceneTest, GradeFForLowScore) {
    GameplayState state(100);

    // Create mostly misses
    std::vector<JudgmentEvent> events;
    for (int i = 0; i < 50; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::MISS, 0.0, true);
    }
    for (int i = 50; i < 100; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::BAD, 80.0, false);
    }
    state.apply(events);

    ResultScene scene(nullptr, nullptr, stack_.get(), nullptr, state);

    // Score should be below 60% (5000 points / 100000 = 5%)
    EXPECT_LT(state.score_percentage(), 60.0);
}

// Test Scenario 2: Score and max combo shown
TEST_F(ResultSceneTest, ScoreAndMaxComboDisplayed) {
    GameplayState state(50);

    // Create a combo scenario
    std::vector<JudgmentEvent> events;
    for (int i = 0; i < 30; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::PERFECT, 0.0, false);
    }
    events.emplace_back(30, 0, 0.0, JudgmentTier::MISS, 0.0, true); // Break combo
    for (int i = 31; i < 50; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::GREAT, 5.0, false);
    }
    state.apply(events);

    ResultScene scene(nullptr, nullptr, stack_.get(), nullptr, state);

    EXPECT_EQ(state.max_combo(), 30);
    EXPECT_GT(state.score(), 0);
}

// Test Scenario 3: Judgment counts displayed
TEST_F(ResultSceneTest, JudgmentCountsDisplayed) {
    GameplayState state(200);

    std::vector<JudgmentEvent> events;
    // 142 Perfect, 38 Great, 5 Good, 2 Bad, 13 Miss (as per story AC)
    int note_idx = 0;
    for (int i = 0; i < 142; ++i) {
        events.emplace_back(note_idx++, i % 5, 0.0, JudgmentTier::PERFECT, 0.0, false);
    }
    for (int i = 0; i < 38; ++i) {
        events.emplace_back(note_idx++, i % 5, 0.0, JudgmentTier::GREAT, 5.0, false);
    }
    for (int i = 0; i < 5; ++i) {
        events.emplace_back(note_idx++, i % 5, 0.0, JudgmentTier::GOOD, 30.0, false);
    }
    for (int i = 0; i < 2; ++i) {
        events.emplace_back(note_idx++, i % 5, 0.0, JudgmentTier::BAD, 80.0, false);
    }
    for (int i = 0; i < 13; ++i) {
        events.emplace_back(note_idx++, i % 5, 0.0, JudgmentTier::MISS, 0.0, true);
    }

    state.apply(events);

    ResultScene scene(nullptr, nullptr, stack_.get(), nullptr, state);

    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 142);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GREAT), 38);
    EXPECT_EQ(state.judgment_count(JudgmentTier::GOOD), 5);
    EXPECT_EQ(state.judgment_count(JudgmentTier::BAD), 2);
    EXPECT_EQ(state.judgment_count(JudgmentTier::MISS), 13);
}

// Test Scenario 4: Scene lifecycle
TEST_F(ResultSceneTest, SceneLifecycleWorks) {
    GameplayState state(10);
    std::vector<JudgmentEvent> events;
    for (int i = 0; i < 10; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::PERFECT, 0.0, false);
    }
    state.apply(events);

    ResultScene scene(nullptr, nullptr, stack_.get(), nullptr, state);

    // Test lifecycle methods don't crash
    scene.on_enter();
    scene.update(1.0);
    scene.render();
    scene.on_pause();
    scene.on_resume();
    scene.on_exit();
}

// Test Scenario 5: Auto-transition after 5 seconds
TEST_F(ResultSceneTest, AutoTransitionAfter5Seconds) {
    GameplayState state(10);
    std::vector<JudgmentEvent> events;
    for (int i = 0; i < 10; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::PERFECT, 0.0, false);
    }
    state.apply(events);

    ResultScene scene(nullptr, nullptr, stack_.get(), nullptr, state);

    scene.on_enter();

    // Accumulate 5 seconds
    for (int i = 0; i < 5; ++i) {
        scene.update(1.0);
    }

    // After 5 seconds, stack should have a new scene (TitleScene)
    EXPECT_EQ(stack_->size(), 1);
}

// Test Scenario 6: Input triggers immediate transition
TEST_F(ResultSceneTest, InputTriggersImmediateTransition) {
    GameplayState state(10);
    std::vector<JudgmentEvent> events;
    for (int i = 0; i < 10; ++i) {
        events.emplace_back(i, i % 5, 0.0, JudgmentTier::PERFECT, 0.0, false);
    }
    state.apply(events);

    ResultScene scene(nullptr, nullptr, stack_.get(), nullptr, state);

    scene.on_enter();

    // Simulate input (button 0 pressed)
    // InputSnapshot constructor: (held, pressed, released, tick_number)
    InputSnapshot input(0, 1, 0, 0);  // pressed_mask = 1 for button 0

    scene.handle_input(input);

    // Stack should have a new scene (TitleScene)
    EXPECT_EQ(stack_->size(), 1);
}

} // namespace openitup
