#include <openitup/judge/judgment_event.h>

namespace openitup {

JudgmentEvent::JudgmentEvent(std::size_t note_index, uint8_t column, double beat,
                             JudgmentTier tier, double timing_error_ms, bool is_auto_miss)
    : note_index_(note_index)
    , column_(column)
    , beat_(beat)
    , tier_(tier)
    , timing_error_ms_(timing_error_ms)
    , is_auto_miss_(is_auto_miss) {
}

bool JudgmentEvent::operator<(const JudgmentEvent& other) const {
    if (beat_ != other.beat_) {
        return beat_ < other.beat_;
    }
    return column_ < other.column_;
}

} // namespace openitup
