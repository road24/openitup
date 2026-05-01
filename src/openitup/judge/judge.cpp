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

    // Precompute total judgable notes (Phase 3: TAP and HOLD_HEAD notes)
    for (const auto& note : note_data_.events()) {
        if (note.type == NoteType::TAP || note.type == NoteType::HOLD_HEAD) {
            total_judgable_++;
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

        // Skip non-judgable note types (Phase 3: TAP and HOLD_HEAD are judgable)
        if (note.type != NoteType::TAP && note.type != NoteType::HOLD_HEAD) {
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
                                          uint32_t pressed_columns,
                                          uint32_t held_columns) {
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

        // Skip non-judgable types (Phase 3: TAP and HOLD_HEAD are judgable)
        if (note.type != NoteType::TAP && note.type != NoteType::HOLD_HEAD) {
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

        // If this was a HOLD_HEAD that was successfully judged (not MISS),
        // create an active hold state for body scoring (US-JDG-007).
        if (note.type == NoteType::HOLD_HEAD) {
            double tail_beat = find_hold_tail_beat(idx);
            if (tail_beat > 0.0) {
                // Compute ticks required: duration in seconds * 60 ticks/sec
                double head_time_s = timing_data_.time_at_beat(note.beat);
                double tail_time_s = timing_data_.time_at_beat(tail_beat);
                double duration_s = tail_time_s - head_time_s;
                int ticks_required = static_cast<int>(duration_s * 60.0);

                active_holds_.push_back({
                    note.column,
                    tail_beat,
                    true,
                    tier,
                    0,
                    ticks_required,
                    0  // grace_ticks_remaining starts at 0
                });
            }
        }
    }

    // Phase 3: Hold body scoring (US-JDG-008) with grace window (US-JDG-009)
    // Process active holds and track tick-by-tick scoring
    double current_beat = timing_data_.beat_at_time(song_position_ms / 1000.0);

    for (auto& hold : active_holds_) {
        if (!hold.active) {
            continue;
        }

        // Check if we've reached or passed the tail beat
        if (current_beat >= hold.tail_beat) {
            // Hold completed - mark as inactive
            hold.active = false;
            // Tail judgment will be emitted when tail is processed
            continue;
        }

        // Check if the column is currently held
        bool column_held = (held_columns & (1u << hold.column)) != 0;
        bool column_pressed = (pressed_columns & (1u << hold.column)) != 0;

        if (column_held) {
            // Player is holding - increment tick count
            hold.ticks_held++;

            // US-JDG-009: If re-pressed during grace window, reset grace
            if (hold.grace_ticks_remaining > 0) {
                hold.grace_ticks_remaining = 0;
            }
        } else {
            // Player released
            // US-JDG-009: Grace window recovery
            // Hardcoded 100ms grace = 6 ticks at 60Hz for Phase 3
            // Phase 4 will load from judge profile
            if (hold.grace_ticks_remaining == 0) {
                // Just released - start grace window
                hold.grace_ticks_remaining = 6;
            } else {
                // Already in grace period - decrement countdown
                hold.grace_ticks_remaining--;
                if (hold.grace_ticks_remaining == 0) {
                    // Grace window expired - hold is permanently dropped
                    hold.active = false;
                }
            }
        }
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

        // Skip non-judgable types (Phase 3: TAP and HOLD_HEAD are judgable)
        if (note.type != NoteType::TAP && note.type != NoteType::HOLD_HEAD) {
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
    active_holds_.clear();
}

const std::vector<HoldState>& Judge::active_holds() const {
    return active_holds_;
}

double Judge::find_hold_tail_beat(std::size_t head_index) const {
    if (head_index >= note_data_.size()) {
        return -1.0;
    }

    const auto& head_note = note_data_.events()[head_index];

    // Search forward from the head for a HOLD_TAIL in the same column
    for (std::size_t i = head_index + 1; i < note_data_.size(); ++i) {
        const auto& note = note_data_.events()[i];

        // Stop searching if we've gone too far past the head
        if (note.beat > head_note.beat + 100.0) {
            break;
        }

        // Found a matching tail
        if (note.type == NoteType::HOLD_TAIL && note.column == head_note.column) {
            return note.beat;
        }
    }

    return -1.0;
}

} // namespace openitup
