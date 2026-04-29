#pragma once

#include <cstdint>
#include <functional>

namespace openitup {

class Clock {
public:
    using CounterFn = std::function<uint64_t()>;
    using FrequencyFn = std::function<uint64_t()>;

    Clock();
    Clock(CounterFn counter_fn, FrequencyFn frequency_fn);

    double tick();
    double elapsed() const;
    void reset();

    uint64_t raw_counter() const;
    uint64_t frequency() const;

private:
    CounterFn counter_fn_;
    FrequencyFn frequency_fn_;

    uint64_t frequency_value_;
    uint64_t start_counter_;
    uint64_t last_counter_;
    bool first_tick_;
};

static constexpr double FIXED_STEP = 1.0 / 60.0;
static constexpr int MAX_STEPS_PER_FRAME = 10;

struct FixedStepResult {
    int num_steps;
    double new_accumulator;
    double alpha;
    bool spiral_guard_triggered;
};

FixedStepResult compute_fixed_steps(double delta, double accumulator);

} // namespace openitup
