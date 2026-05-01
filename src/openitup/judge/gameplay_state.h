#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <openitup/judge/judgment_event.h>
#include <openitup/judge/judgment_tier.h>

namespace openitup {

class GameplayState {
public:
    explicit GameplayState(int total_notes);

    void apply(const std::vector<JudgmentEvent>& events);
    void apply_single(const JudgmentEvent& event);

    int current_combo() const { return current_combo_; }
    int max_combo() const { return max_combo_; }
    int64_t score() const { return score_; }
    int64_t hold_score() const { return hold_score_; }
    double score_percentage() const;

    int judgment_count(JudgmentTier tier) const;
    int total_judged() const;
    int total_notes() const { return total_notes_; }

    // Life gauge (US-JDG-010)
    float hp() const { return hp_; }
    bool is_failed() const { return hp_ <= 0.0f; }

    void reset();

private:
    static constexpr int64_t PERFECT_POINTS = 1000;
    static constexpr int64_t GREAT_POINTS = 800;
    static constexpr int64_t GOOD_POINTS = 500;
    static constexpr int64_t BAD_POINTS = 100;
    static constexpr int64_t MISS_POINTS = 0;

    static constexpr std::array<int64_t, JUDGMENT_TIER_COUNT> POINTS_PER_TIER = {
        PERFECT_POINTS, GREAT_POINTS, GOOD_POINTS, BAD_POINTS, MISS_POINTS
    };

    int total_notes_;
    int current_combo_;
    int max_combo_;
    int64_t score_;
    int64_t hold_score_;
    std::array<int, JUDGMENT_TIER_COUNT> judgment_counts_;
    float hp_;  // Life gauge: 0.0 to 1.0, starts at 1.0 (US-JDG-010)
};

} // namespace openitup
