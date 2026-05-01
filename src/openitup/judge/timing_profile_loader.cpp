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

    // US-JDG-016: Complete profiles for each PIU version

    // Exceed (2003) - Original timing windows, standard scoring
    TimingProfile exceed;
    exceed.name = "exceed";
    exceed.perfect_window_ms = 16.0;
    exceed.great_window_ms = 33.0;
    exceed.good_window_ms = 66.0;
    exceed.bad_window_ms = 100.0;
    exceed.score_perfect = 1000;
    exceed.score_great = 800;
    exceed.score_good = 500;
    exceed.score_bad = 100;
    exceed.score_miss = 0;
    exceed.grade_thresholds = {
        {"SSS", 0.995},
        {"SS", 0.99},
        {"S", 0.95},
        {"A", 0.90},
        {"B", 0.80},
        {"C", 0.70},
        {"D", 0.60}
    };
    profiles["exceed"] = exceed;

    // Zero (2004) - Similar to Exceed, slightly different grade thresholds
    TimingProfile zero;
    zero.name = "zero";
    zero.perfect_window_ms = 16.0;
    zero.great_window_ms = 33.0;
    zero.good_window_ms = 66.0;
    zero.bad_window_ms = 100.0;
    zero.score_perfect = 1000;
    zero.score_great = 800;
    zero.score_good = 500;
    zero.score_bad = 100;
    zero.score_miss = 0;
    zero.grade_thresholds = {
        {"SSS", 0.995},
        {"SS", 0.99},
        {"S", 0.95},
        {"A", 0.90},
        {"B", 0.80},
        {"C", 0.70},
        {"D", 0.60}
    };
    profiles["zero"] = zero;

    // NX (2006) - Tighter timing windows, adjusted scoring
    TimingProfile nx;
    nx.name = "nx";
    nx.perfect_window_ms = 15.0;
    nx.great_window_ms = 30.0;
    nx.good_window_ms = 60.0;
    nx.bad_window_ms = 90.0;
    nx.score_perfect = 1000;
    nx.score_great = 750;
    nx.score_good = 450;
    nx.score_bad = 80;
    nx.score_miss = 0;
    nx.grade_thresholds = {
        {"SSS", 0.995},
        {"SS", 0.99},
        {"S", 0.96},
        {"A", 0.92},
        {"B", 0.85},
        {"C", 0.75},
        {"D", 0.65}
    };
    profiles["nx"] = nx;

    // NX2 (2007) - Further refined timing
    TimingProfile nx2;
    nx2.name = "nx2";
    nx2.perfect_window_ms = 15.0;
    nx2.great_window_ms = 30.0;
    nx2.good_window_ms = 60.0;
    nx2.bad_window_ms = 90.0;
    nx2.score_perfect = 1000;
    nx2.score_great = 750;
    nx2.score_good = 450;
    nx2.score_bad = 80;
    nx2.score_miss = 0;
    nx2.grade_thresholds = {
        {"SSS", 0.995},
        {"SS", 0.99},
        {"S", 0.96},
        {"A", 0.92},
        {"B", 0.85},
        {"C", 0.75},
        {"D", 0.65}
    };
    profiles["nx2"] = nx2;

    // Fiesta (2008) - Tightest timing windows
    TimingProfile fiesta;
    fiesta.name = "fiesta";
    fiesta.perfect_window_ms = 14.0;
    fiesta.great_window_ms = 28.0;
    fiesta.good_window_ms = 56.0;
    fiesta.bad_window_ms = 84.0;
    fiesta.score_perfect = 1000;
    fiesta.score_great = 700;
    fiesta.score_good = 400;
    fiesta.score_bad = 50;
    fiesta.score_miss = 0;
    fiesta.grade_thresholds = {
        {"SSS", 0.995},
        {"SS", 0.99},
        {"S", 0.97},
        {"A", 0.93},
        {"B", 0.87},
        {"C", 0.78},
        {"D", 0.68}
    };
    profiles["fiesta"] = fiesta;

    // Fiesta 2 (2010) - Similar to Fiesta with minor adjustments
    TimingProfile fiesta2;
    fiesta2.name = "fiesta2";
    fiesta2.perfect_window_ms = 14.0;
    fiesta2.great_window_ms = 28.0;
    fiesta2.good_window_ms = 56.0;
    fiesta2.bad_window_ms = 84.0;
    fiesta2.score_perfect = 1000;
    fiesta2.score_great = 700;
    fiesta2.score_good = 400;
    fiesta2.score_bad = 50;
    fiesta2.score_miss = 0;
    fiesta2.grade_thresholds = {
        {"SSS", 0.995},
        {"SS", 0.99},
        {"S", 0.97},
        {"A", 0.93},
        {"B", 0.87},
        {"C", 0.78},
        {"D", 0.68}
    };
    profiles["fiesta2"] = fiesta2;

    // Prime (2014) - Modern timing, refined scoring
    TimingProfile prime;
    prime.name = "prime";
    prime.perfect_window_ms = 14.0;
    prime.great_window_ms = 28.0;
    prime.good_window_ms = 56.0;
    prime.bad_window_ms = 84.0;
    prime.score_perfect = 1000;
    prime.score_great = 700;
    prime.score_good = 400;
    prime.score_bad = 50;
    prime.score_miss = 0;
    prime.grade_thresholds = {
        {"SSS", 0.998},
        {"SS", 0.995},
        {"S", 0.98},
        {"A", 0.95},
        {"B", 0.90},
        {"C", 0.80},
        {"D", 0.70}
    };
    profiles["prime"] = prime;

    return profiles;
}

} // namespace openitup
