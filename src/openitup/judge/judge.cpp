#include <openitup/judge/judge.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include <openitup/chart/note_type.h>
#include <spdlog/spdlog.h>

namespace openitup {

Judge::Judge(const NoteData& note_data, const TimingData& timing_data,
             const TimingProfile& profile)
    : note_data_(note_data),
      timing_data_(timing_data),
      profile_(profile),
      cursor_(0),
      total_judgable_(0),
      judged_count_(0) {

    // Initialize judged state for all notes
    judged_.resize(note_data_.size(), false);

    // Precompute total judgable notes (Phase 1: only TAP notes)
    for (const auto& note : note_data_.events()) {
        if (note.type == NoteType::TAP) {
            total_judgable_++;
        } else if (note.type == NoteType::HOLD_HEAD) {
            spdlog::warn("Judge Phase 1: HOLD_HEAD note encountered at beat {}, column {} - holds not yet supported",
                        note.beat, note.column);
        }
    }

    if (!profile_.is_valid()) {
        spdlog::error("Judge constructed with invalid TimingProfile");
    }
}

JudgmentTier Judge::classify(double abs_error_ms) const {
    if (abs_error_ms <= profile_.perfect_window_ms) return JudgmentTier::PERFECT;
    if (abs_error_ms <= profile_.great_window_ms) return JudgmentTier::GREAT;
    if (abs_error_ms <= profile_.good_window_ms) return JudgmentTier::GOOD;
    if (abs_error_ms <= profile_.bad_window_ms) return JudgmentTier::BAD;
    return JudgmentTier::MISS;
}

std::size_t Judge::find_closest_unjudged(uint8_t column, double song_position_ms) const {
    std::size_t best_index = SIZE_MAX;
    double best_abs_error = std::numeric_limits<double>::infinity();

    // Scan from cursor to the end of the judgable window
    for (std::size_t i = cursor_; i < note_data_.size(); ++i) {
        const auto& note = note_data_.events()[i];

        // Convert note beat to milliseconds
        // IMPORTANT: time_at_beat returns SECONDS, multiply by 1000.0!
        double note_time_ms = timing_data_.time_at_beat(note.beat) * 1000.0;

        // If note is too far in the future, stop scanning
        if (note_time_ms - song_position_ms > profile_.bad_window_ms) {
            break;
        }

        // Skip already-judged notes
        if (judged_[i]) {
            continue;
        }

        // Skip wrong column
        if (note.column != column) {
            continue;
        }

        // Skip non-tap notes in Phase 1
        if (note.type != NoteType::TAP) {
            continue;
        }

        double abs_error = std::abs(song_position_ms - note_time_ms);

        // Outside the bad window? Skip
        if (abs_error > profile_.bad_window_ms) {
            continue;
        }

        // Closest match wins
        if (abs_error < best_abs_error) {
            best_abs_error = abs_error;
            best_index = i;
        }
    }

    return best_index;
}

std::vector<JudgmentEvent> Judge::update(double song_position_ms,
                                          uint32_t pressed_columns) {
    std::vector<JudgmentEvent> events;

    // Phase 1: Auto-miss scan
    // Advance cursor past notes that are beyond the late boundary
    while (cursor_ < note_data_.size()) {
        const auto& note = note_data_.events()[cursor_];

        // Skip already-judged notes
        if (judged_[cursor_]) {
            cursor_++;
            continue;
        }

        // Skip non-judgable types
        if (note.type != NoteType::TAP) {
            cursor_++;
            continue;
        }

        // Convert note beat to milliseconds
        double note_time_ms = timing_data_.time_at_beat(note.beat) * 1000.0;

        // If this note is still within the judgable window, stop advancing
        if (song_position_ms - note_time_ms <= profile_.bad_window_ms) {
            break;
        }

        // Note is past the late miss boundary → auto-miss
        events.push_back(JudgmentEvent(cursor_, note.column, note.beat,
                                        JudgmentTier::MISS,
                                        profile_.bad_window_ms,
                                        true));
        judged_[cursor_] = true;
        judged_count_++;
        cursor_++;
    }

    // Phase 2: match pressed columns to unjudged notes
    for (uint8_t column = 0; column < 10; ++column) {
        // Check if this column bit is set
        if (!(pressed_columns & (1u << column))) {
            continue;
        }

        std::size_t idx = find_closest_unjudged(column, song_position_ms);
        if (idx == SIZE_MAX) {
            continue; // no matching note
        }

        const auto& note = note_data_.events()[idx];
        double note_time_ms = timing_data_.time_at_beat(note.beat) * 1000.0;
        double error_ms = song_position_ms - note_time_ms;
        JudgmentTier tier = classify(std::abs(error_ms));

        // If tier is MISS, the input was within scan range but outside bad window.
        // Do not judge the note as miss yet — it may be hit on a later tick.
        if (tier == JudgmentTier::MISS) {
            continue;
        }

        events.push_back(JudgmentEvent(idx, note.column, note.beat,
                                        tier, error_ms, false));
        judged_[idx] = true;
        judged_count_++;
    }

    // Sort events by beat (US-JDG-004 SC3)
    std::sort(events.begin(), events.end());

    return events;
}

std::vector<JudgmentEvent> Judge::flush_remaining() {
    std::vector<JudgmentEvent> events;

    for (std::size_t i = 0; i < note_data_.size(); ++i) {
        if (judged_[i]) {
            continue;
        }

        const auto& note = note_data_.events()[i];

        // Skip non-judgable types
        if (note.type != NoteType::TAP) {
            continue;
        }

        events.push_back(JudgmentEvent(i, note.column, note.beat,
                                        JudgmentTier::MISS,
                                        profile_.bad_window_ms,
                                        true));
        judged_[i] = true;
        judged_count_++;
    }

    // Sort by beat
    std::sort(events.begin(), events.end());

    return events;
}

std::size_t Judge::judged_count() const {
    return judged_count_;
}

std::size_t Judge::total_judgable() const {
    return total_judgable_;
}

bool Judge::is_complete() const {
    return judged_count_ >= total_judgable_;
}

const TimingProfile& Judge::profile() const {
    return profile_;
}

void Judge::reset() {
    std::fill(judged_.begin(), judged_.end(), false);
    cursor_ = 0;
    judged_count_ = 0;
}

} // namespace openitup
