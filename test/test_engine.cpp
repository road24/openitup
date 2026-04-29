#include <gtest/gtest.h>
#include <openitup/core/engine.h>
#include <openitup/core/clock.h>

using namespace openitup;

class EngineTimingTest : public ::testing::Test {
protected:
    static constexpr uint64_t FREQ = 1'000'000'000;
    uint64_t counter_ = 0;

    std::unique_ptr<Clock> make_clock() {
        return std::make_unique<Clock>(
            [this]() -> uint64_t { return counter_; },
            [this]() -> uint64_t { return FREQ; }
        );
    }

    void advance_ms(double ms) {
        counter_ += static_cast<uint64_t>(ms * 1'000'000.0);
    }
};

TEST_F(EngineTimingTest, AccumulatorProducesCorrectTickCount) {
    // Simulate 5 frames at 60fps = 5 logic ticks
    double accumulator = 0.0;
    uint64_t tick_count = 0;

    for (int frame = 0; frame < 5; ++frame) {
        auto result = compute_fixed_steps(1.0 / 60.0, accumulator);
        accumulator = result.new_accumulator;
        tick_count += result.num_steps;
    }

    EXPECT_EQ(tick_count, 5u);
}

TEST_F(EngineTimingTest, SlowFrameProducesMultipleSteps) {
    auto result = compute_fixed_steps(0.050, 0.0);
    EXPECT_EQ(result.num_steps, 3);
}

TEST_F(EngineTimingTest, AlphaInValidRange) {
    // Half a frame accumulated
    auto result = compute_fixed_steps(0.008, 0.0);
    EXPECT_GE(result.alpha, 0.0);
    EXPECT_LT(result.alpha, 1.0);
}

TEST_F(EngineTimingTest, TwoHourSimulation) {
    double accumulator = 0.0;
    uint64_t tick_count = 0;

    for (int i = 0; i < 432'000; ++i) {
        auto result = compute_fixed_steps(1.0 / 60.0, accumulator);
        accumulator = result.new_accumulator;
        tick_count += result.num_steps;
    }

    EXPECT_EQ(tick_count, 432'000u);
}

TEST_F(EngineTimingTest, SpiralGuardPreventsRunaway) {
    auto result = compute_fixed_steps(1.0, 0.0);
    EXPECT_EQ(result.num_steps, MAX_STEPS_PER_FRAME);
    EXPECT_TRUE(result.spiral_guard_triggered);
}

// Engine construction tests (require SDL — will skip in headless)

class EngineConstructionTest : public ::testing::Test {
protected:
    bool has_display() const {
        // Check if we can create a dummy window to test display availability
        SDL_Init(SDL_INIT_VIDEO);
        SDL_Window* test_window = SDL_CreateWindow("test", 1, 1, SDL_WINDOW_HIDDEN);
        bool available = (test_window != nullptr);
        if (test_window) SDL_DestroyWindow(test_window);
        SDL_Quit();
        return available;
    }
};

TEST_F(EngineConstructionTest, EngineStartsAndStops) {
    if (!has_display()) GTEST_SKIP() << "No display available";

    // Use injectable clock that advances time, and quit after first frame
    uint64_t counter = 0;
    auto clock = std::make_unique<Clock>(
        [&counter]() -> uint64_t { return counter; },
        []() -> uint64_t { return 1'000'000'000; }
    );

    EngineConfig config;
    config.window_title = "test";
    config.window_width = 1;
    config.window_height = 1;

    Engine engine(config, std::move(clock));

    // Push a SDL_QUIT event to stop the loop after one iteration
    SDL_Event quit_event{};
    quit_event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit_event);

    int result = engine.run();
    EXPECT_EQ(result, 0);
}

TEST_F(EngineConstructionTest, AudioSystemAccessor) {
    if (!has_display()) GTEST_SKIP() << "No display available";

    EngineConfig config;
    config.window_title = "audio_test";
    config.window_width = 640;
    config.window_height = 480;

    Engine engine(config);

    // Audio may be null if no device available
    // Just verify the accessor doesn't crash
    AudioSystem* audio = engine.get_audio();

    // If audio is available, verify it was initialized
    if (audio) {
        EXPECT_NE(audio, nullptr);
        // Audio system should be in STOPPED state initially
        EXPECT_EQ(audio->get_state(), AudioState::STOPPED);
    }
}
