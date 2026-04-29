#pragma once

#include <cstdint>
#include <vector>
#include <utility>

#include <openitup/chart/note_type.h>

namespace openitup {

struct NoteEvent {
    double beat;        // beat position (double for sub-beat precision)
    uint8_t column;     // 0-4 for single, 0-9 for double
    NoteType type;      // tap, hold_head, hold_tail, mine, fake, lift

    // Comparison for sorting: by beat, then column, then type.
    bool operator<(const NoteEvent& other) const {
        if (beat != other.beat) return beat < other.beat;
        if (column != other.column) return column < other.column;
        return static_cast<uint8_t>(type) < static_cast<uint8_t>(other.type);
    }

    bool operator==(const NoteEvent& other) const {
        return beat == other.beat && column == other.column && type == other.type;
    }
};

class NoteData {
public:
    // Construct from a pre-sorted vector of events.
    explicit NoteData(std::vector<NoteEvent> events);

    // Default constructor: empty note data
    NoteData();

    // Total number of note events.
    std::size_t size() const;

    // True if no events.
    bool empty() const;

    // Access underlying sorted vector.
    const std::vector<NoteEvent>& events() const;

    // All notes with beat positions in [lo_beat, hi_beat).
    // Uses std::lower_bound for O(log n) endpoints.
    // Returns iterators into the events vector.
    using const_iterator = std::vector<NoteEvent>::const_iterator;
    std::pair<const_iterator, const_iterator>
        notes_in_range(double lo_beat, double hi_beat) const;

    // Count of notes by type (for metadata/stats).
    std::size_t count_by_type(NoteType type) const;

private:
    std::vector<NoteEvent> events_;
};

} // namespace openitup
