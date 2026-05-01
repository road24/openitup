#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <openitup/judge/judgment_event.h>
#include <openitup/judge/judgment_tier.h>
#include <openitup/judge/timing_profile.h>

namespace openitup {

class GameplayState {
public:
    explicit GameplayState(int total_notes, const TimingProfile& profile = default_timing_profile());

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

    // US-JDG-015: Grade calculation from profile
    std::string current_grade() const;

    void reset();

private:
    int total_notes_;
    int current_combo_;
    int max_combo_;
    int64_t score_;
    int64_t hold_score_;
    std::array<int, JUDGMENT_TIER_COUNT> judgment_counts_;
    float hp_;  // Life gauge: 0.0 to 1.0, starts at 1.0 (US-JDG-010)
    TimingProfile profile_;  // US-JDG-014: Profile with scoring formula
};

} // namespace openitup
