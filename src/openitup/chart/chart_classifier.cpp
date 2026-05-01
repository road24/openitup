#include <openitup/chart/chart_classifier.h>

#include <algorithm>
#include <cmath>

namespace openitup {

PlayMode ChartClassifier::auto_classify_mode(const NoteData& note_data) {
    int max_col = find_max_column(note_data);

    // If any column >= 5 is used, it's DOUBLE mode
    if (max_col >= 5) {
        return PlayMode::DOUBLE;
    }

    // Otherwise it's SINGLE mode
    return PlayMode::SINGLE;
}

int ChartClassifier::estimate_difficulty(const NoteData& note_data, const TimingData& timing_data) {
    if (note_data.empty()) {
        return 1;  // Minimum difficulty for empty chart
    }

    double peak_nps = calculate_peak_nps(note_data, timing_data);

    // Map NPS to difficulty rating (1-28 scale)
    // <2 NPS = 1-5 (Easy)
    if (peak_nps < 2.0) {
        return std::max(1, static_cast<int>(1 + peak_nps * 2.0));
    }
    // 2-4 NPS = 6-10 (Normal)
    else if (peak_nps < 4.0) {
        return std::max(6, static_cast<int>(6 + (peak_nps - 2.0) * 2.0));
    }
    // 4-6 NPS = 11-15 (Hard)
    else if (peak_nps < 6.0) {
        return std::max(11, static_cast<int>(11 + (peak_nps - 4.0) * 2.0));
    }
    // 6-8 NPS = 16-20 (Crazy)
    else if (peak_nps < 8.0) {
        return std::max(16, static_cast<int>(16 + (peak_nps - 6.0) * 2.0));
    }
    // 8+ NPS = 21-28 (Freestyle)
    else {
        int rating = static_cast<int>(21 + (peak_nps - 8.0) * 1.0);
        return std::min(28, std::max(21, rating));
    }
}

int ChartClassifier::find_max_column(const NoteData& note_data) {
    int max_col = -1;

    const auto& events = note_data.events();
    for (const auto& event : events) {
        max_col = std::max(max_col, static_cast<int>(event.column));
    }

    return max_col;
}

double ChartClassifier::calculate_peak_nps(const NoteData& note_data, const TimingData& timing_data) {
    const auto& events = note_data.events();

    if (events.empty()) {
        return 0.0;
    }

    // Use a sliding window of 4 seconds to find peak density
    constexpr double window_seconds = 4.0;
    double peak_nps = 0.0;

    // Convert first and last beat to time
    double start_beat = events.front().beat;
    double end_beat = events.back().beat;
    double start_time = timing_data.time_at_beat(start_beat);
    double end_time = timing_data.time_at_beat(end_beat);

    // If chart is shorter than the window, just compute overall NPS
    if (end_time - start_time <= window_seconds) {
        double duration = std::max(0.1, end_time - start_time);
        return static_cast<double>(events.size()) / duration;
    }

    // Slide the window from start to end
    // Sample every 0.5 seconds for performance
    constexpr double step_seconds = 0.5;

    for (double window_start_time = start_time; window_start_time <= end_time - window_seconds; window_start_time += step_seconds) {
        double window_end_time = window_start_time + window_seconds;

        // Convert time window to beat window
        double window_start_beat = timing_data.beat_at_time(window_start_time);
        double window_end_beat = timing_data.beat_at_time(window_end_time);

        // Count notes in this window
        auto [begin, end] = note_data.notes_in_range(window_start_beat, window_end_beat);
        int note_count = static_cast<int>(std::distance(begin, end));

        double nps = static_cast<double>(note_count) / window_seconds;
        peak_nps = std::max(peak_nps, nps);
    }

    return peak_nps;
}

} // namespace openitup
