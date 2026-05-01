#include "timing_profile.h"

#include <algorithm>

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
    TimingProfile profile;
    profile.name = "exceed";
    profile.perfect_window_ms = 16.0;
    profile.great_window_ms = 33.0;
    profile.good_window_ms = 66.0;
    profile.bad_window_ms = 100.0;
    profile.score_perfect = 1000;
    profile.score_great = 800;
    profile.score_good = 500;
    profile.score_bad = 100;
    profile.score_miss = 0;
    profile.grade_thresholds = {
        {"SSS", 0.995},
        {"SS", 0.99},
        {"S", 0.95},
        {"A", 0.90},
        {"B", 0.80},
        {"C", 0.70},
        {"D", 0.60}
    };
    return profile;
}

std::string calculate_grade(double score_percentage, const TimingProfile& profile) {
    // Convert to 0.0-1.0 range if input is 0-100
    double ratio = score_percentage;
    if (ratio > 1.0) {
        ratio = ratio / 100.0;
    }

    // Sort grade thresholds by value (descending) and find first match
    std::vector<std::pair<std::string, double>> sorted_grades;
    for (const auto& [grade, threshold] : profile.grade_thresholds) {
        sorted_grades.push_back({grade, threshold});
    }
    std::sort(sorted_grades.begin(), sorted_grades.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    for (const auto& [grade, threshold] : sorted_grades) {
        if (ratio >= threshold) {
            return grade;
        }
    }

    return "F";
}

void to_json(nlohmann::json& j, const TimingProfile& profile) {
    j = nlohmann::json{
        {"name", profile.name},
        {"perfect_window_ms", profile.perfect_window_ms},
        {"great_window_ms", profile.great_window_ms},
        {"good_window_ms", profile.good_window_ms},
        {"bad_window_ms", profile.bad_window_ms},
        {"score_perfect", profile.score_perfect},
        {"score_great", profile.score_great},
        {"score_good", profile.score_good},
        {"score_bad", profile.score_bad},
        {"score_miss", profile.score_miss},
        {"grade_thresholds", profile.grade_thresholds}
    };
}

void from_json(const nlohmann::json& j, TimingProfile& profile) {
    j.at("name").get_to(profile.name);
    j.at("perfect_window_ms").get_to(profile.perfect_window_ms);
    j.at("great_window_ms").get_to(profile.great_window_ms);
    j.at("good_window_ms").get_to(profile.good_window_ms);
    j.at("bad_window_ms").get_to(profile.bad_window_ms);

    // Optional fields with defaults (US-JDG-014, US-JDG-015)
    if (j.contains("score_perfect")) {
        j.at("score_perfect").get_to(profile.score_perfect);
    }
    if (j.contains("score_great")) {
        j.at("score_great").get_to(profile.score_great);
    }
    if (j.contains("score_good")) {
        j.at("score_good").get_to(profile.score_good);
    }
    if (j.contains("score_bad")) {
        j.at("score_bad").get_to(profile.score_bad);
    }
    if (j.contains("score_miss")) {
        j.at("score_miss").get_to(profile.score_miss);
    }
    if (j.contains("grade_thresholds")) {
        j.at("grade_thresholds").get_to(profile.grade_thresholds);
    }
}

} // namespace openitup
