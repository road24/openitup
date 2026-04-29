#pragma once

#include <vector>
#include <cstdint>

namespace openitup {

enum class TimingEventType : uint8_t {
    BPM_CHANGE = 0,
    STOP = 1,
};

struct TimingEvent {
    double beat;            // beat position where this event occurs
    TimingEventType type;

    // For BPM_CHANGE: the new BPM value (beats per minute).
    // For STOP: unused (0.0).
    double bpm;

    // For STOP: duration in seconds.
    // For BPM_CHANGE: unused (0.0).
    double stop_duration;

    // Comparison for sorting by beat position, then type.
    bool operator<(const TimingEvent& other) const {
        if (beat != other.beat) return beat < other.beat;
        // BPM changes sort before stops at the same beat.
        return static_cast<uint8_t>(type) < static_cast<uint8_t>(other.type);
    }
};

class TimingData {
public:
    // Construct from a vector of timing events.
    // The constructor sorts events and pre-computes segment data.
    explicit TimingData(std::vector<TimingEvent> events);

    // Default constructor: 120 BPM at beat 0, no stops.
    TimingData();

    // Convert beat position to elapsed time in seconds.
    // Returns the time including all BPM changes and stops up to that beat.
    // Beats before 0.0 return 0.0 (clamped).
    double time_at_beat(double beat) const;

    // Convert elapsed time in seconds to beat position.
    // Inverse of time_at_beat. Time during a stop returns the stop's beat.
    // Negative time returns 0.0 (clamped).
    double beat_at_time(double time) const;

    // BPM at a given beat position.
    double bpm_at_beat(double beat) const;

    // Access the underlying event list (sorted).
    const std::vector<TimingEvent>& events() const;

    // Total number of timing events.
    std::size_t size() const;

    // True if no events (should never happen -- default has 120 BPM at beat 0).
    bool empty() const;

private:
    std::vector<TimingEvent> events_;

    // Build internal segment structure. Called by constructor.
    void build_segments();

    // Internal segment structure for O(log n) lookup.
    struct TimingSegment {
        double start_beat;
        double start_time;          // cumulative seconds at start_beat
        double seconds_per_beat;    // 60/bpm. 0 for stops.
        double stop_duration;       // >0 only for stop segments
        bool is_stop;
    };
    std::vector<TimingSegment> segments_;
};

} // namespace openitup
