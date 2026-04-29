#include "timing_profile.h"

namespace openitup {

bool TimingProfile::is_valid() const {
    // All windows must be greater than 0
    if (perfect_window_ms <= 0.0 || great_window_ms <= 0.0 ||
        good_window_ms <= 0.0 || bad_window_ms <= 0.0) {
        return false;
    }

    // Windows must be ordered: perfect <= great <= good <= bad
    if (perfect_window_ms > great_window_ms) {
        return false;
    }
    if (great_window_ms > good_window_ms) {
        return false;
    }
    if (good_window_ms > bad_window_ms) {
        return false;
    }

    return true;
}

TimingProfile default_timing_profile() {
    // Exceed-era hardcoded defaults
    return TimingProfile{16.0, 33.0, 66.0, 100.0};
}

} // namespace openitup
