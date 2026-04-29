#include <openitup/chart/note_data.h>

#include <algorithm>

namespace openitup {

NoteData::NoteData(std::vector<NoteEvent> events)
    : events_(std::move(events)) {
}

NoteData::NoteData()
    : events_() {
}

std::size_t NoteData::size() const {
    return events_.size();
}

bool NoteData::empty() const {
    return events_.empty();
}

const std::vector<NoteEvent>& NoteData::events() const {
    return events_;
}

std::pair<NoteData::const_iterator, NoteData::const_iterator>
NoteData::notes_in_range(double lo_beat, double hi_beat) const {
    auto lo_it = std::lower_bound(events_.begin(), events_.end(), lo_beat,
        [](const NoteEvent& e, double b) { return e.beat < b; });
    auto hi_it = std::lower_bound(events_.begin(), events_.end(), hi_beat,
        [](const NoteEvent& e, double b) { return e.beat < b; });
    return {lo_it, hi_it};
}

std::size_t NoteData::count_by_type(NoteType type) const {
    return std::count_if(events_.begin(), events_.end(),
        [type](const NoteEvent& e) { return e.type == type; });
}

} // namespace openitup
