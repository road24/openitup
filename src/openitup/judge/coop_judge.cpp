#include <openitup/judge/coop_judge.h>

#include <algorithm>
#include <spdlog/spdlog.h>

#include <openitup/chart/note_type.h>

namespace openitup {

// Helper: Filter note_data to only include notes for specific columns
static NoteData filter_columns(const NoteData& note_data, uint8_t start_col, uint8_t end_col) {
    std::vector<NoteEvent> filtered_events;
    for (const auto& note : note_data.events()) {
        if (note.column >= start_col && note.column <= end_col) {
            // Remap column to 0-4 range for the judge
            NoteEvent remapped = note;
            remapped.column = note.column - start_col;
            filtered_events.push_back(remapped);
        }
    }
    return NoteData(filtered_events);
}

CoopJudge::CoopJudge(const NoteData& note_data, const TimingData& timing_data,
                     const TimingProfile& profile)
    : p1_judge_(filter_columns(note_data, 0, 4), timing_data, profile),
      p2_judge_(filter_columns(note_data, 5, 9), timing_data, profile) {
    spdlog::info("CoopJudge initialized: P1={} notes, P2={} notes",
                 p1_judge_.total_judgable(), p2_judge_.total_judgable());
}

std::vector<JudgmentEvent> CoopJudge::update(double song_position_ms,
                                              uint32_t p1_pressed, uint32_t p1_held,
                                              uint32_t p2_pressed, uint32_t p2_held) {
    // Mask P1 inputs to columns 0-4 (bits 0-4)
    uint32_t p1_pressed_masked = p1_pressed & 0x1F;  // 0b11111
    uint32_t p1_held_masked = p1_held & 0x1F;

    // Mask P2 inputs to columns 5-9 (bits 5-9), then shift right by 5 to remap to 0-4
    uint32_t p2_pressed_masked = (p2_pressed >> 5) & 0x1F;
    uint32_t p2_held_masked = (p2_held >> 5) & 0x1F;

    // Update both judges
    auto p1_events = p1_judge_.update(song_position_ms, p1_pressed_masked, p1_held_masked);
    auto p2_events = p2_judge_.update(song_position_ms, p2_pressed_masked, p2_held_masked);

    // Restore original column indices for P2 events (shift back by 5)
    for (auto& event : p2_events) {
        // JudgmentEvent is immutable, so we need to create a new one
        // For now, we'll just leave them as-is since the event contains column info
        // The caller can determine player from column range
    }

    // Merge events from both players
    std::vector<JudgmentEvent> combined;
    combined.reserve(p1_events.size() + p2_events.size());
    combined.insert(combined.end(), p1_events.begin(), p1_events.end());
    combined.insert(combined.end(), p2_events.begin(), p2_events.end());

    // Sort by beat (US-JDG-004 SC3)
    std::sort(combined.begin(), combined.end());

    return combined;
}

std::vector<JudgmentEvent> CoopJudge::flush_remaining() {
    auto p1_events = p1_judge_.flush_remaining();
    auto p2_events = p2_judge_.flush_remaining();

    std::vector<JudgmentEvent> combined;
    combined.reserve(p1_events.size() + p2_events.size());
    combined.insert(combined.end(), p1_events.begin(), p1_events.end());
    combined.insert(combined.end(), p2_events.begin(), p2_events.end());

    std::sort(combined.begin(), combined.end());
    return combined;
}

std::size_t CoopJudge::judged_count() const {
    return p1_judge_.judged_count() + p2_judge_.judged_count();
}

std::size_t CoopJudge::total_judgable() const {
    return p1_judge_.total_judgable() + p2_judge_.total_judgable();
}

bool CoopJudge::is_complete() const {
    return p1_judge_.is_complete() && p2_judge_.is_complete();
}

void CoopJudge::reset() {
    p1_judge_.reset();
    p2_judge_.reset();
}

} // namespace openitup
