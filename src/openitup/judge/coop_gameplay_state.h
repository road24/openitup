#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <openitup/judge/coop_life_mode.h>
#include <openitup/judge/gameplay_state.h>
#include <openitup/judge/judgment_event.h>
#include <openitup/judge/judgment_tier.h>
#include <openitup/judge/timing_profile.h>

namespace openitup {

// US-JDG-017, US-JDG-018: Co-op gameplay state with configurable life gauge mode.
class CoopGameplayState {
public:
    // Construct for co-op mode.
    // total_notes: total judgable notes (P1 + P2 combined).
    // mode: SHARED = one life gauge, SEPARATE = two independent gauges.
    explicit CoopGameplayState(int total_notes,
                               CoopLifeMode mode = CoopLifeMode::SHARED,
                               const TimingProfile& profile = default_timing_profile());

    // Apply judgment events from both players.
    void apply(const std::vector<JudgmentEvent>& events);
    void apply_single(const JudgmentEvent& event);

    // Combined stats (both players).
    int current_combo() const { return current_combo_; }
    int max_combo() const { return max_combo_; }
    int64_t score() const { return score_; }
    int64_t hold_score() const { return hold_score_; }
    double score_percentage() const;
    std::string current_grade() const;

    int judgment_count(JudgmentTier tier) const;
    int total_judged() const;
    int total_notes() const { return total_notes_; }

    // Life gauge access
    CoopLifeMode life_mode() const { return life_mode_; }

    // SHARED mode: returns the shared gauge value
    // SEPARATE mode: returns P1's gauge value
    float hp() const { return hp_p1_; }

    // SEPARATE mode only: P2's gauge value
    float hp_p2() const { return hp_p2_; }

    // Failure check
    // SHARED: fails when shared gauge reaches 0
    // SEPARATE: fails when EITHER player's gauge reaches 0
    bool is_failed() const;

    void reset();

private:
    CoopLifeMode life_mode_;
    int total_notes_;
    int current_combo_;
    int max_combo_;
    int64_t score_;
    int64_t hold_score_;
    std::array<int, JUDGMENT_TIER_COUNT> judgment_counts_;

    // Life gauges
    // SHARED mode: only hp_p1_ is used
    // SEPARATE mode: both are used
    float hp_p1_;
    float hp_p2_;

    TimingProfile profile_;
};

} // namespace openitup
