#include <openitup/chart/chart_builder.h>

#include <algorithm>

#include <spdlog/spdlog.h>

namespace openitup {

ChartBuilder::ChartBuilder() = default;

void ChartBuilder::set_title(std::string title) {
    metadata_.title = std::move(title);
}

void ChartBuilder::set_artist(std::string artist) {
    metadata_.artist = std::move(artist);
}

void ChartBuilder::set_genre(std::string genre) {
    metadata_.genre = std::move(genre);
}

void ChartBuilder::set_charter_name(std::string name) {
    metadata_.charter_name = std::move(name);
}

void ChartBuilder::set_difficulty_name(std::string name) {
    metadata_.difficulty_name = std::move(name);
}

void ChartBuilder::set_difficulty_rating(int rating) {
    metadata_.difficulty_rating = rating;
}

void ChartBuilder::set_mode(PlayMode mode) {
    metadata_.mode = mode;
}

void ChartBuilder::set_audio_path(std::string path) {
    metadata_.audio_path = std::move(path);
}

void ChartBuilder::set_intro_path(std::string path) {
    metadata_.intro_path = std::move(path);
}

void ChartBuilder::set_banner_path(std::string path) {
    metadata_.banner_path = std::move(path);
}

void ChartBuilder::set_background_path(std::string path) {
    metadata_.background_path = std::move(path);
}

void ChartBuilder::set_display_bpm(double bpm) {
    metadata_.display_bpm = bpm;
}

void ChartBuilder::set_start_time_ms(double ms) {
    metadata_.start_time_ms = ms;
}

void ChartBuilder::set_preview_start_seconds(double seconds) {
    metadata_.preview_start_seconds = seconds;
}

void ChartBuilder::set_preview_length_seconds(double seconds) {
    metadata_.preview_length_seconds = seconds;
}

void ChartBuilder::add_bpm_change(double beat, double bpm) {
    TimingEvent event;
    event.beat = beat;
    event.type = TimingEventType::BPM_CHANGE;
    event.bpm = bpm;
    event.stop_duration = 0.0;
    timing_events_.push_back(event);
}

void ChartBuilder::add_stop(double beat, double duration_seconds) {
    TimingEvent event;
    event.beat = beat;
    event.type = TimingEventType::STOP;
    event.bpm = 0.0;
    event.stop_duration = duration_seconds;
    timing_events_.push_back(event);
}

void ChartBuilder::add_note(double beat, uint8_t column, NoteType type) {
    NoteEvent event;
    event.beat = beat;
    event.column = column;
    event.type = type;
    note_events_.push_back(event);
}

Chart ChartBuilder::build() {
    // 1. If no BPM events, add default 120 BPM at beat 0, log warning
    if (timing_events_.empty()) {
        spdlog::warn("Chart has no BPM events, using default 120 BPM at beat 0");
        add_bpm_change(0.0, 120.0);
    }

    // 2. Sort timing events
    std::sort(timing_events_.begin(), timing_events_.end());

    // 3. Validate: zero or negative BPM -> throw ChartLoadException
    for (const auto& event : timing_events_) {
        if (event.type == TimingEventType::BPM_CHANGE && event.bpm <= 0.0) {
            throw ChartLoadException("Chart has zero or negative BPM value");
        }
    }

    // 4. Sort note events
    std::sort(note_events_.begin(), note_events_.end());

    // 5. Validate columns against mode: out-of-range -> log warning (don't throw)
    int max_col = max_columns(metadata_.mode);
    for (const auto& event : note_events_) {
        if (event.column >= max_col) {
            spdlog::warn("Note at beat {} has column {} which is out of range for mode {} (max {})",
                event.beat, event.column, play_mode_to_string(metadata_.mode), max_col - 1);
        }
    }

    // 6. Construct Chart(metadata, TimingData(timing_events), NoteData(note_events))
    return Chart(
        std::move(metadata_),
        TimingData(std::move(timing_events_)),
        NoteData(std::move(note_events_))
    );
}

} // namespace openitup
