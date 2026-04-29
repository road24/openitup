#pragma once

#include <openitup/chart/chart_metadata.h>
#include <openitup/chart/timing_data.h>
#include <openitup/chart/note_data.h>

namespace openitup {

class Chart {
public:
    // Construct a complete chart. All components are moved in.
    Chart(ChartMetadata metadata, TimingData timing_data, NoteData note_data);

    // Access components (const references -- chart is immutable after construction).
    const ChartMetadata& metadata() const;
    const TimingData& timing_data() const;
    const NoteData& note_data() const;

    // Convenience: total duration in seconds (time at the last note's beat).
    double duration_seconds() const;

    // Convenience: total number of notes.
    std::size_t note_count() const;

private:
    ChartMetadata metadata_;
    TimingData timing_data_;
    NoteData note_data_;
};

} // namespace openitup
