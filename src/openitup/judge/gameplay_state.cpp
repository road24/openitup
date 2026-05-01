#include <openitup/judge/gameplay_state.h>

#include <algorithm>

namespace openitup {

GameplayState::GameplayState(int total_notes)
    : total_notes_(total_notes),
      current_combo_(0),
      max_combo_(0),
      score_(0),
      hold_score_(0),
      judgment_counts_{} {
}

void GameplayState::apply_single(const JudgmentEvent& event) {
    JudgmentTier tier = event.tier();
    int tier_index = static_cast<int>(tier);

    judgment_counts_[tier_index]++;
    score_ += POINTS_PER_TIER[tier_index];

    if (tier_maintains_combo(tier)) {
        current_combo_++;
        if (current_combo_ > max_combo_) {
            max_combo_ = current_combo_;
        }
    } else {
        current_combo_ = 0;
    }
}

void GameplayState::apply(const std::vector<JudgmentEvent>& events) {
    for (const auto& event : events) {
        apply_single(event);
    }
}

double GameplayState::score_percentage() const {
    if (total_notes_ == 0) {
        return 0.0;
    }
    int64_t max_score = static_cast<int64_t>(total_notes_) * PERFECT_POINTS;
    if (max_score == 0) {
        return 0.0;
    }
    return (static_cast<double>(score_) / static_cast<double>(max_score)) * 100.0;
}

int GameplayState::judgment_count(JudgmentTier tier) const {
    return judgment_counts_[static_cast<int>(tier)];
}

int GameplayState::total_judged() const {
    int total = 0;
    for (int count : judgment_counts_) {
        total += count;
    }
    return total;
}

void GameplayState::reset() {
    current_combo_ = 0;
    max_combo_ = 0;
    score_ = 0;
    hold_score_ = 0;
    judgment_counts_.fill(0);
}

} // namespace openitup
