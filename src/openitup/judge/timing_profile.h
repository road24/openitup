#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace openitup {

struct TimingProfile {
    std::string name;
    double perfect_window_ms;
    double great_window_ms;
    double good_window_ms;
    double bad_window_ms;

    bool is_valid() const;
};

TimingProfile default_timing_profile();

// JSON serialization support
void to_json(nlohmann::json& j, const TimingProfile& profile);
void from_json(const nlohmann::json& j, TimingProfile& profile);

} // namespace openitup
