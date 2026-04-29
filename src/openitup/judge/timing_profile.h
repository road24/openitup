#pragma once

namespace openitup {

struct TimingProfile {
    double perfect_window_ms;
    double great_window_ms;
    double good_window_ms;
    double bad_window_ms;

    bool is_valid() const;
};

TimingProfile default_timing_profile();

} // namespace openitup
