#include <openitup/chart/timing_data.h>

#include <algorithm>
#include <cmath>

namespace openitup {

TimingData::TimingData(std::vector<TimingEvent> events)
    : events_(std::move(events)) {
    std::sort(events_.begin(), events_.end());
    build_segments();
}

TimingData::TimingData() {
    // Default: 120 BPM at beat 0
    events_.push_back({0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0});
    build_segments();
}

void TimingData::build_segments() {
    if (events_.empty()) {
        return;
    }

    segments_.clear();
    segments_.reserve(events_.size());

    double current_bpm = 120.0;  // fallback
    double current_time = 0.0;
    double prev_beat = 0.0;

    for (const auto& event : events_) {
        // Time elapsed from prev_beat to this event's beat at the current BPM
        if (current_bpm > 0.0 && event.beat > prev_beat) {
            double beat_delta = event.beat - prev_beat;
            current_time += beat_delta * (60.0 / current_bpm);
        }

        if (event.type == TimingEventType::BPM_CHANGE) {
            current_bpm = event.bpm;
            segments_.push_back({
                event.beat,
                current_time,
                60.0 / event.bpm,
                0.0,
                false
            });
        } else if (event.type == TimingEventType::STOP) {
            segments_.push_back({
                event.beat,
                current_time,
                0.0,
                event.stop_duration,
                true
            });
            current_time += event.stop_duration;
        }

        prev_beat = event.beat;
    }
}

double TimingData::time_at_beat(double beat) const {
    if (segments_.empty() || beat <= 0.0) {
        return 0.0;
    }

    // Find the last segment with start_beat <= beat
    auto it = std::upper_bound(segments_.begin(), segments_.end(), beat,
        [](double b, const TimingSegment& seg) { return b < seg.start_beat; });

    if (it == segments_.begin()) {
        // Before all segments - use default 120 BPM
        return beat * 0.5;
    }

    --it;  // Last segment at or before beat

    double time = 0.0;

    if (it->is_stop) {
        // The active segment is a stop
        // Time includes the stop's start time + full stop duration
        // Plus any additional beats after the stop
        time = it->start_time + it->stop_duration;

        // If query beat is past the stop, need to add time from stop's beat to query beat
        if (beat > it->start_beat) {
            // Find the current BPM (look backwards for last BPM segment)
            double current_spb = 0.5;  // default 120 BPM
            for (auto seg_it = it; seg_it != segments_.begin(); ) {
                --seg_it;
                if (!seg_it->is_stop) {
                    current_spb = seg_it->seconds_per_beat;
                    break;
                }
            }
            double remaining_beats = beat - it->start_beat;
            time += remaining_beats * current_spb;
        }
    } else {
        // BPM segment: add linear offset
        double remaining_beats = beat - it->start_beat;
        time = it->start_time + remaining_beats * it->seconds_per_beat;
    }

    return time;
}

double TimingData::beat_at_time(double time) const {
    if (segments_.empty() || time <= 0.0) {
        return 0.0;
    }

    // Find the last segment with start_time <= time
    auto it = std::upper_bound(segments_.begin(), segments_.end(), time,
        [](double t, const TimingSegment& seg) { return t < seg.start_time; });

    if (it == segments_.begin()) {
        // Before all segments - use default 120 BPM
        return time / 0.5;
    }

    --it;  // Last segment with start_time <= time

    if (it->is_stop) {
        // Check if we're within the stop window
        double stop_end_time = it->start_time + it->stop_duration;
        if (time < stop_end_time) {
            // During the stop - beat doesn't advance
            return it->start_beat;
        }

        // After the stop - use the current BPM to compute beats
        // Find the current BPM (look backwards for last BPM segment)
        double current_spb = 0.5;  // default 120 BPM
        for (auto seg_it = it; seg_it != segments_.begin(); ) {
            --seg_it;
            if (!seg_it->is_stop) {
                current_spb = seg_it->seconds_per_beat;
                break;
            }
        }

        double remaining_time = time - stop_end_time;
        return it->start_beat + remaining_time / current_spb;
    } else {
        // BPM segment: compute beat from time
        double remaining_time = time - it->start_time;
        if (it->seconds_per_beat > 0.0) {
            return it->start_beat + remaining_time / it->seconds_per_beat;
        }
        return it->start_beat;
    }
}

double TimingData::bpm_at_beat(double beat) const {
    if (segments_.empty()) {
        return 120.0;
    }

    // Find the last BPM segment at or before beat
    auto it = std::upper_bound(segments_.begin(), segments_.end(), beat,
        [](double b, const TimingSegment& seg) { return b < seg.start_beat; });

    if (it == segments_.begin()) {
        return 120.0;  // default
    }

    --it;

    // Walk backwards to find the last BPM segment (skip stops)
    while (it != segments_.begin() && it->is_stop) {
        --it;
    }

    if (it->is_stop) {
        // All segments up to this point are stops - use default
        return 120.0;
    }

    return 60.0 / it->seconds_per_beat;
}

const std::vector<TimingEvent>& TimingData::events() const {
    return events_;
}

std::size_t TimingData::size() const {
    return events_.size();
}

bool TimingData::empty() const {
    return events_.empty();
}

} // namespace openitup
