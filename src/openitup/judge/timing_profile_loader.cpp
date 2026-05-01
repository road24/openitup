#include "timing_profile_loader.h"

#include <fstream>
#include <spdlog/spdlog.h>

namespace openitup {

std::optional<TimingProfile> load_timing_profile(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        spdlog::error("Failed to open timing profile file: {}", file_path);
        return std::nullopt;
    }

    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error("Failed to parse timing profile JSON from {}: {}",
                      file_path, e.what());
        return std::nullopt;
    }

    return load_timing_profile(json);
}

std::optional<TimingProfile> load_timing_profile(const nlohmann::json& json) {
    TimingProfile profile;

    try {
        profile = json.get<TimingProfile>();
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to deserialize timing profile: {}", e.what());
        return std::nullopt;
    }

    // Validate the profile
    if (!profile.is_valid()) {
        spdlog::error("Invalid timing profile: windows must be positive and ordered (perfect <= great <= good <= bad)");
        return std::nullopt;
    }

    return profile;
}

std::map<std::string, TimingProfile> built_in_profiles() {
    std::map<std::string, TimingProfile> profiles;

    // Exceed (2003) - Original timing windows
    profiles["exceed"] = TimingProfile{
        "exceed",
        16.0,  // perfect_window_ms
        33.0,  // great_window_ms
        66.0,  // good_window_ms
        100.0  // bad_window_ms
    };

    // NX (2006) - Tighter timing windows
    profiles["nx"] = TimingProfile{
        "nx",
        15.0,  // perfect_window_ms
        30.0,  // great_window_ms
        60.0,  // good_window_ms
        90.0   // bad_window_ms
    };

    // Fiesta (2008) - Tightest timing windows
    profiles["fiesta"] = TimingProfile{
        "fiesta",
        14.0,  // perfect_window_ms
        28.0,  // great_window_ms
        56.0,  // good_window_ms
        84.0   // bad_window_ms
    };

    return profiles;
}

} // namespace openitup
