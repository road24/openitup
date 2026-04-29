#include <openitup/core/clock.h>

#include <SDL3/SDL.h>

namespace openitup {

Clock::Clock()
    : counter_fn_([]() -> uint64_t { return SDL_GetPerformanceCounter(); }),
      frequency_fn_([]() -> uint64_t { return SDL_GetPerformanceFrequency(); }),
      frequency_value_(SDL_GetPerformanceFrequency()),
      start_counter_(SDL_GetPerformanceCounter()),
      last_counter_(start_counter_),
      first_tick_(true) {}

Clock::Clock(CounterFn counter_fn, FrequencyFn frequency_fn)
    : counter_fn_(std::move(counter_fn)),
      frequency_fn_(std::move(frequency_fn)),
      frequency_value_(frequency_fn_()),
      start_counter_(counter_fn_()),
      last_counter_(start_counter_),
      first_tick_(true) {}

double Clock::tick() {
    uint64_t now = counter_fn_();

    if (first_tick_) {
        first_tick_ = false;
        last_counter_ = now;
        return 0.0;
    }

    uint64_t delta_counts = now - last_counter_;
    last_counter_ = now;

    return static_cast<double>(delta_counts) / static_cast<double>(frequency_value_);
}

double Clock::elapsed() const {
    uint64_t now = counter_fn_();
    uint64_t delta_counts = now - start_counter_;
    return static_cast<double>(delta_counts) / static_cast<double>(frequency_value_);
}

void Clock::reset() {
    uint64_t now = counter_fn_();
    start_counter_ = now;
    last_counter_ = now;
    first_tick_ = true;
}

uint64_t Clock::raw_counter() const {
    return counter_fn_();
}

uint64_t Clock::frequency() const {
    return frequency_value_;
}

FixedStepResult compute_fixed_steps(double delta, double accumulator) {
    FixedStepResult result{};
    accumulator += delta;

    double max_accumulator = FIXED_STEP * MAX_STEPS_PER_FRAME;
    if (accumulator > max_accumulator) {
        accumulator = max_accumulator;
        result.spiral_guard_triggered = true;
    }

    while (accumulator >= FIXED_STEP) {
        result.num_steps++;
        accumulator -= FIXED_STEP;
    }

    result.new_accumulator = accumulator;
    result.alpha = (FIXED_STEP > 0.0) ? accumulator / FIXED_STEP : 0.0;

    return result;
}

} // namespace openitup
