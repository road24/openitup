#include <gtest/gtest.h>
#include <openitup/core/clock.h>
#include <cmath>

using namespace openitup;

class ClockTest : public ::testing::Test {
protected:
    static constexpr uint64_t FREQ = 1'000'000'000;  // 1 GHz (realistic high-res counter)

    uint64_t counter_ = 0;

    Clock make_clock() {
        return Clock(
            [this]() -> uint64_t { return counter_; },
            [this]() -> uint64_t { return FREQ; }
        );
    }

    void advance_ms(double ms) {
        counter_ += static_cast<uint64_t>(ms * 1'000'000.0);
    }

    void advance_seconds(double s) {
        counter_ += static_cast<uint64_t>(s * static_cast<double>(FREQ));
    }
};

TEST_F(ClockTest, FirstTickReturnsZero) {
    auto clock = make_clock();
    EXPECT_DOUBLE_EQ(clock.tick(), 0.0);
}

TEST_F(ClockTest, DeltaTimeAccuracy) {
    auto clock = make_clock();
    clock.tick();

    advance_ms(16.6667);
    double delta = clock.tick();
    EXPECT_NEAR(delta, 1.0 / 60.0, 1e-6);
}

TEST_F(ClockTest, ElapsedTimeTracking) {
    auto clock = make_clock();

    advance_ms(100.0);
    EXPECT_NEAR(clock.elapsed(), 0.1, 1e-9);

    advance_ms(400.0);
    EXPECT_NEAR(clock.elapsed(), 0.5, 1e-9);
}

TEST_F(ClockTest, ResetClearsElapsed) {
    auto clock = make_clock();

    advance_ms(500.0);
    EXPECT_NEAR(clock.elapsed(), 0.5, 1e-9);

    clock.reset();
    EXPECT_NEAR(clock.elapsed(), 0.0, 1e-9);

    // After reset, next tick returns 0.0 (first_tick_ flag reset)
    EXPECT_DOUBLE_EQ(clock.tick(), 0.0);

    advance_ms(10.0);
    double delta = clock.tick();
    EXPECT_NEAR(delta, 0.01, 1e-9);
}

TEST_F(ClockTest, PrecisionAfter2Hours) {
    auto clock = make_clock();
    clock.tick();

    double total_time = 0.0;
    constexpr int TICKS = 432'000;  // 2 hours at 60 Hz
    constexpr double STEP = 1.0 / 60.0;

    for (int i = 0; i < TICKS; ++i) {
        advance_seconds(STEP);
        total_time += clock.tick();
    }

    double expected = TICKS * STEP;
    double error = std::fabs(total_time - expected);
    // Each tick() division introduces ~1e-16 relative error. Over 432k ticks,
    // cumulative error is ~432000 * 0.01667 * 1e-16 * random_walk ≈ sub-ms.
    // 1ms tolerance is well within gameplay requirements (judge operates at ±16ms).
    EXPECT_LT(error, 1e-3) << "Cumulative error after 2 hours: " << error << " seconds";
}

TEST_F(ClockTest, HighFrequencyCounter) {
    auto clock = make_clock();
    EXPECT_EQ(clock.frequency(), FREQ);
}

TEST_F(ClockTest, ZeroDeltaWhenCounterUnchanged) {
    auto clock = make_clock();
    clock.tick();

    // Counter doesn't advance
    double delta = clock.tick();
    EXPECT_DOUBLE_EQ(delta, 0.0);
}

TEST_F(ClockTest, RawCounterReturnsCurrentValue) {
    auto clock = make_clock();
    EXPECT_EQ(clock.raw_counter(), 0u);

    advance_ms(100.0);
    EXPECT_EQ(clock.raw_counter(), 100'000'000u);
}

// --- Accumulator logic ---

TEST(FixedStep, SingleStepAt60Hz) {
    auto r = openitup::compute_fixed_steps(1.0 / 60.0, 0.0);
    EXPECT_EQ(r.num_steps, 1);
    EXPECT_NEAR(r.new_accumulator, 0.0, 1e-9);
    EXPECT_FALSE(r.spiral_guard_triggered);
}

TEST(FixedStep, NoStepUnder16ms) {
    auto r = openitup::compute_fixed_steps(0.008, 0.0);
    EXPECT_EQ(r.num_steps, 0);
    EXPECT_NEAR(r.new_accumulator, 0.008, 1e-9);
}

TEST(FixedStep, MultipleStepsOnSlowFrame) {
    auto r = openitup::compute_fixed_steps(0.050, 0.0);
    EXPECT_EQ(r.num_steps, 3);
    EXPECT_NEAR(r.new_accumulator, 0.050 - 3.0 / 60.0, 1e-6);
}

TEST(FixedStep, SpiralGuardCapsAt10Steps) {
    auto r = openitup::compute_fixed_steps(0.500, 0.0);
    EXPECT_EQ(r.num_steps, 10);
    EXPECT_TRUE(r.spiral_guard_triggered);
}

TEST(FixedStep, AccumulatorCarriesRemainder) {
    auto r = openitup::compute_fixed_steps(0.020, 0.0);
    EXPECT_EQ(r.num_steps, 1);
    double expected_remainder = 0.020 - 1.0 / 60.0;
    EXPECT_NEAR(r.new_accumulator, expected_remainder, 1e-9);
}

TEST(FixedStep, AlphaComputedCorrectly) {
    auto r = openitup::compute_fixed_steps(0.008, 0.0);
    double expected_alpha = 0.008 / (1.0 / 60.0);
    EXPECT_NEAR(r.alpha, expected_alpha, 1e-6);
}

TEST(FixedStep, AlphaZeroAfterExactStep) {
    auto r = openitup::compute_fixed_steps(1.0 / 60.0, 0.0);
    EXPECT_NEAR(r.alpha, 0.0, 1e-9);
}

TEST(FixedStep, CarryAccumulatorAcrossFrames) {
    auto r1 = openitup::compute_fixed_steps(0.010, 0.0);
    EXPECT_EQ(r1.num_steps, 0);

    auto r2 = openitup::compute_fixed_steps(0.010, r1.new_accumulator);
    EXPECT_EQ(r2.num_steps, 1);
    EXPECT_NEAR(r2.new_accumulator, 0.020 - 1.0 / 60.0, 1e-6);
}
