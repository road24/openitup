#include <openitup/judge/coop_gameplay_state.h>

#include <algorithm>

namespace openitup {

CoopGameplayState::CoopGameplayState(int total_notes, CoopLifeMode mode,
                                     const TimingProfile& profile)
    : life_mode_(mode),
      total_notes_(total_notes),
      current_combo_(0),
      max_combo_(0),
      score_(0),
      hold_score_(0),
      judgment_counts_{},
      hp_p1_(1.0f),
      hp_p2_(1.0f),
      profile_(profile) {
}

void CoopGameplayState::apply_single(const JudgmentEvent& event) {
    JudgmentTier tier = event.tier();
    int tier_index = static_cast<int>(tier);

    judgment_counts_[tier_index]++;

    // Score accumulation (combined for both players)
    int64_t points = 0;
    switch (tier) {
        case JudgmentTier::PERFECT:
            points = profile_.score_perfect;
            break;
        case JudgmentTier::GREAT:
            points = profile_.score_great;
            break;
        case JudgmentTier::GOOD:
            points = profile_.score_good;
            break;
        case JudgmentTier::BAD:
            points = profile_.score_bad;
            break;
        case JudgmentTier::MISS:
            points = profile_.score_miss;
            break;
    }
    score_ += points;

    // Combo (combined)
    if (tier_maintains_combo(tier)) {
        current_combo_++;
        if (current_combo_ > max_combo_) {
            max_combo_ = current_combo_;
        }
    } else {
        current_combo_ = 0;
    }

    // Life gauge handling (US-JDG-018)
    float hp_delta = 0.0f;
    switch (tier) {
        case JudgmentTier::PERFECT:
            hp_delta = 0.02f;
            break;
        case JudgmentTier::GREAT:
            hp_delta = 0.01f;
            break;
        case JudgmentTier::GOOD:
            hp_delta = 0.0f;
            break;
        case JudgmentTier::BAD:
            hp_delta = -0.05f;
            break;
        case JudgmentTier::MISS:
            hp_delta = -0.10f;
            break;
    }

    if (life_mode_ == CoopLifeMode::SHARED) {
        // SHARED mode: both players affect the same gauge (US-JDG-018 SC1)
        hp_p1_ += hp_delta;
        hp_p1_ = std::clamp(hp_p1_, 0.0f, 1.0f);
    } else {
        // SEPARATE mode: determine which player based on column (US-JDG-018 SC2)
        // Columns 0-4 = P1, columns 5-9 = P2
        if (event.column() <= 4) {
            hp_p1_ += hp_delta;
            hp_p1_ = std::clamp(hp_p1_, 0.0f, 1.0f);
        } else {
            hp_p2_ += hp_delta;
            hp_p2_ = std::clamp(hp_p2_, 0.0f, 1.0f);
        }
    }
}

void CoopGameplayState::apply(const std::vector<JudgmentEvent>& events) {
    for (const auto& event : events) {
        apply_single(event);
    }
}

double CoopGameplayState::score_percentage() const {
    if (total_notes_ == 0) {
        return 0.0;
    }
    int64_t max_score = static_cast<int64_t>(total_notes_) * profile_.score_perfect;
    if (max_score == 0) {
        return 0.0;
    }
    return (static_cast<double>(score_) / static_cast<double>(max_score)) * 100.0;
}

std::string CoopGameplayState::current_grade() const {
    double percentage = score_percentage();
    return calculate_grade(percentage, profile_);
}

int CoopGameplayState::judgment_count(JudgmentTier tier) const {
    return judgment_counts_[static_cast<int>(tier)];
}

int CoopGameplayState::total_judged() const {
    int total = 0;
    for (int count : judgment_counts_) {
        total += count;
    }
    return total;
}

bool CoopGameplayState::is_failed() const {
    if (life_mode_ == CoopLifeMode::SHARED) {
        // SHARED: fails when the shared gauge reaches 0 (US-JDG-018 SC4)
        return hp_p1_ <= 0.0f;
    } else {
        // SEPARATE: fails when EITHER player's gauge reaches 0 (US-JDG-018 SC3)
        return hp_p1_ <= 0.0f || hp_p2_ <= 0.0f;
    }
}

void CoopGameplayState::reset() {
    current_combo_ = 0;
    max_combo_ = 0;
    score_ = 0;
    hold_score_ = 0;
    judgment_counts_.fill(0);
    hp_p1_ = 1.0f;
    hp_p2_ = 1.0f;
}

} // namespace openitup
