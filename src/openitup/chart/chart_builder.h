#pragma once

#include <string>
#include <vector>
#include <stdexcept>

#include <openitup/chart/chart.h>
#include <openitup/chart/chart_metadata.h>
#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>
#include <openitup/chart/timing_data.h>

namespace openitup {

class ChartLoadException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ChartBuilder {
public:
    ChartBuilder();

    // --- Metadata setters ---
    void set_title(std::string title);
    void set_artist(std::string artist);
    void set_genre(std::string genre);
    void set_charter_name(std::string name);
    void set_difficulty_name(std::string name);
    void set_difficulty_rating(int rating);
    void set_mode(PlayMode mode);
    void set_audio_path(std::string path);
    void set_intro_path(std::string path);
    void set_banner_path(std::string path);
    void set_background_path(std::string path);
    void set_display_bpm(double bpm);

    // --- Timing event additions ---
    void add_bpm_change(double beat, double bpm);
    void add_stop(double beat, double duration_seconds);

    // --- Note additions ---
    void add_note(double beat, uint8_t column, NoteType type);

    // --- Build the immutable Chart ---
    // Sorts notes, validates data, builds cumulative timing offsets.
    // Throws ChartLoadException if data is fundamentally invalid
    // (e.g., negative BPM).
    // Logs warnings for non-fatal issues (e.g., column out of range).
    Chart build();

private:
    ChartMetadata metadata_;
    std::vector<TimingEvent> timing_events_;
    std::vector<NoteEvent> note_events_;
};

} // namespace openitup
