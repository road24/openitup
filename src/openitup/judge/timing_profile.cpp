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
    return TimingProfile{"exceed", 16.0, 33.0, 66.0, 100.0};
}

void to_json(nlohmann::json& j, const TimingProfile& profile) {
    j = nlohmann::json{
        {"name", profile.name},
        {"perfect_window_ms", profile.perfect_window_ms},
        {"great_window_ms", profile.great_window_ms},
        {"good_window_ms", profile.good_window_ms},
        {"bad_window_ms", profile.bad_window_ms}
    };
}

void from_json(const nlohmann::json& j, TimingProfile& profile) {
    j.at("name").get_to(profile.name);
    j.at("perfect_window_ms").get_to(profile.perfect_window_ms);
    j.at("great_window_ms").get_to(profile.great_window_ms);
    j.at("good_window_ms").get_to(profile.good_window_ms);
    j.at("bad_window_ms").get_to(profile.bad_window_ms);
}

} // namespace openitup
