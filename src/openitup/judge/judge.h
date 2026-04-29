#pragma once

#include <cstdint>
#include <vector>

#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>
#include <openitup/judge/judgment_event.h>
#include <openitup/judge/timing_profile.h>

namespace openitup {

class Judge {
public:
    // Construct a judge for a chart.
    // note_data and timing_data must outlive the Judge.
    // profile is copied into the Judge.
    Judge(const NoteData& note_data, const TimingData& timing_data,
          const TimingProfile& profile);

    // Process one tick of judgment logic.
    // song_position_ms: current playback position from audio system.
    // pressed_columns: bitmask of columns pressed this tick (edge events,
    //   not held state). Bit N = 1 means column N was pressed this tick.
    //   This corresponds to InputSnapshot::pressed_mask() masked to panel bits.
    // Returns: judgment events produced this tick, sorted by beat.
    std::vector<JudgmentEvent> update(double song_position_ms,
                                       uint32_t pressed_columns);

    // Flush all remaining unjudged notes as misses.
    // Called at end of song to satisfy US-JDG-003 Scenario 3.
    std::vector<JudgmentEvent> flush_remaining();

    // Number of notes that have been judged so far.
    std::size_t judged_count() const;

    // Total number of judgable notes (TAP in Phase 1).
    std::size_t total_judgable() const;

    // True if all judgable notes have been judged.
    bool is_complete() const;

    // Access the timing profile (for GameplayState or display).
    const TimingProfile& profile() const;

    // Reset judge state (for retry). Notes and timing data unchanged.
    void reset();

private:
    const NoteData& note_data_;
    const TimingData& timing_data_;
    TimingProfile profile_;

    // Per-note judged state. Index matches NoteData::events() index.
    std::vector<bool> judged_;

    // Cursor: index of the first note that might still be unjudged.
    // Advances forward only. Notes before cursor are guaranteed judged.
    std::size_t cursor_;

    // Total count of judgable notes (precomputed at construction).
    std::size_t total_judgable_;

    // Count of notes judged so far.
    std::size_t judged_count_;

    // Classify the absolute timing error into a judgment tier.
    JudgmentTier classify(double abs_error_ms) const;

    // Find the best (closest) unjudged note for a given column and time.
    // Returns the note index, or SIZE_MAX if no match within bad window.
    std::size_t find_closest_unjudged(uint8_t column, double song_position_ms) const;
};

} // namespace openitup
