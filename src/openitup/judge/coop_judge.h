#pragma once

#include <cstdint>
#include <vector>

#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>
#include <openitup/judge/judge.h>
#include <openitup/judge/judgment_event.h>
#include <openitup/judge/timing_profile.h>

namespace openitup {

// Co-op mode judge managing two independent judge instances for 2-player charts.
// US-JDG-017: P1 judges columns 0-4, P2 judges columns 5-9.
class CoopJudge {
public:
    // Construct a co-op judge.
    // note_data and timing_data must outlive CoopJudge.
    // profile is copied into both Judge instances.
    CoopJudge(const NoteData& note_data, const TimingData& timing_data,
              const TimingProfile& profile);

    // Process one tick for both players.
    // Returns judgment events from both judges, sorted by beat.
    // p1_pressed/p1_held: bitmask for columns 0-4 (P1 inputs)
    // p2_pressed/p2_held: bitmask for columns 5-9 (P2 inputs)
    std::vector<JudgmentEvent> update(double song_position_ms,
                                       uint32_t p1_pressed, uint32_t p1_held,
                                       uint32_t p2_pressed, uint32_t p2_held);

    // Flush all remaining notes as misses from both judges.
    std::vector<JudgmentEvent> flush_remaining();

    // Access individual judges (for testing).
    const Judge& p1_judge() const { return p1_judge_; }
    const Judge& p2_judge() const { return p2_judge_; }

    // Combined judged count from both players.
    std::size_t judged_count() const;

    // Total judgable notes (sum of both judges).
    std::size_t total_judgable() const;

    // True if all notes have been judged by both players.
    bool is_complete() const;

    // Access the timing profile.
    const TimingProfile& profile() const { return p1_judge_.profile(); }

    // Reset both judges.
    void reset();

private:
    Judge p1_judge_;  // Columns 0-4
    Judge p2_judge_;  // Columns 5-9
};

} // namespace openitup
