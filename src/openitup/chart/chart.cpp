#include <openitup/chart/chart.h>

#include <algorithm>

namespace openitup {

Chart::Chart(ChartMetadata metadata, TimingData timing_data, NoteData note_data)
    : metadata_(std::move(metadata))
    , timing_data_(std::move(timing_data))
    , note_data_(std::move(note_data)) {
}

const ChartMetadata& Chart::metadata() const {
    return metadata_;
}

const TimingData& Chart::timing_data() const {
    return timing_data_;
}

const NoteData& Chart::note_data() const {
    return note_data_;
}

double Chart::duration_seconds() const {
    if (note_data_.empty()) {
        return 0.0;
    }
    // Find the last note's beat
    const auto& events = note_data_.events();
    double last_beat = events.back().beat;
    return timing_data_.time_at_beat(last_beat);
}

std::size_t Chart::note_count() const {
    return note_data_.size();
}

} // namespace openitup
