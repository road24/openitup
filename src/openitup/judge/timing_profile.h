#pragma once

#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace openitup {

struct TimingProfile {
    std::string name;
    double perfect_window_ms;
    double great_window_ms;
    double good_window_ms;
    double bad_window_ms;

    // US-JDG-014: Scoring formula
    int score_perfect = 1000;
    int score_great = 800;
    int score_good = 500;
    int score_bad = 100;
    int score_miss = 0;

    // US-JDG-015: Grade thresholds (percentage: 0.0-1.0)
    std::map<std::string, double> grade_thresholds = {
        {"SSS", 0.995},
        {"SS", 0.99},
        {"S", 0.95},
        {"A", 0.90},
        {"B", 0.80},
        {"C", 0.70},
        {"D", 0.60}
    };

    bool is_valid() const;
};

TimingProfile default_timing_profile();

// US-JDG-015: Calculate grade from score percentage
std::string calculate_grade(double score_percentage, const TimingProfile& profile);

// JSON serialization support
void to_json(nlohmann::json& j, const TimingProfile& profile);
void from_json(const nlohmann::json& j, TimingProfile& profile);

} // namespace openitup
