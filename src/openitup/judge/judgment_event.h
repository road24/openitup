#pragma once

#include <cstddef>
#include <cstdint>

#include <openitup/judge/judgment_tier.h>

namespace openitup {

class JudgmentEvent {
public:
    JudgmentEvent(std::size_t note_index, uint8_t column, double beat,
                  JudgmentTier tier, double timing_error_ms, bool is_auto_miss);

    std::size_t note_index() const { return note_index_; }
    uint8_t column() const { return column_; }
    double beat() const { return beat_; }
    JudgmentTier tier() const { return tier_; }
    double timing_error_ms() const { return timing_error_ms_; }
    bool is_auto_miss() const { return is_auto_miss_; }

    bool operator<(const JudgmentEvent& other) const;

private:
    std::size_t note_index_;
    uint8_t column_;
    double beat_;
    JudgmentTier tier_;
    double timing_error_ms_;
    bool is_auto_miss_;
};

} // namespace openitup
