#include <openitup/chart/osf_writer.h>

#include <fstream>
#include <nlohmann/json.hpp>

#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>
#include <openitup/chart/timing_data.h>
#include <openitup/chart/note_data.h>

using json = nlohmann::json;

namespace openitup {

namespace {

// Convert PlayMode to string
std::string mode_to_string(PlayMode mode) {
    switch (mode) {
        case PlayMode::SINGLE: return "SINGLE";
        case PlayMode::DOUBLE: return "DOUBLE";
        default: return "SINGLE";
    }
}

// Convert NoteType to string
std::string note_type_to_string_osf(NoteType type) {
    switch (type) {
        case NoteType::TAP: return "TAP";
        case NoteType::HOLD_HEAD: return "HOLD_HEAD";
        case NoteType::HOLD_TAIL: return "HOLD_TAIL";
        case NoteType::MINE: return "MINE";
        case NoteType::FAKE: return "FAKE";
        case NoteType::LIFT: return "LIFT";
        default: return "TAP";
    }
}

} // anonymous namespace

std::string OsfWriter::to_json(const Chart& chart) const {
    json j;

    // Version
    j["version"] = "1.0";

    // Metadata
    const auto& meta = chart.metadata();
    json j_meta;
    j_meta["title"] = meta.title;
    j_meta["mode"] = mode_to_string(meta.mode);

    if (!meta.artist.empty()) {
        j_meta["artist"] = meta.artist;
    }
    if (!meta.genre.empty()) {
        j_meta["genre"] = meta.genre;
    }
    if (!meta.charter_name.empty()) {
        j_meta["charter_name"] = meta.charter_name;
    }
    if (!meta.difficulty_name.empty()) {
        j_meta["difficulty_name"] = meta.difficulty_name;
    }
    if (meta.difficulty_rating > 0) {
        j_meta["difficulty_rating"] = meta.difficulty_rating;
    }
    if (!meta.audio_path.empty()) {
        // Store relative path (just the filename, not full path)
        std::filesystem::path audio_path(meta.audio_path);
        j_meta["audio_path"] = audio_path.filename().string();
    }
    if (!meta.intro_path.empty()) {
        std::filesystem::path intro_path(meta.intro_path);
        j_meta["intro_path"] = intro_path.filename().string();
    }
    if (!meta.banner_path.empty()) {
        std::filesystem::path banner_path(meta.banner_path);
        j_meta["banner_path"] = banner_path.filename().string();
    }
    if (!meta.background_path.empty()) {
        std::filesystem::path bg_path(meta.background_path);
        j_meta["background_path"] = bg_path.filename().string();
    }
    if (meta.display_bpm > 0.0) {
        j_meta["display_bpm"] = meta.display_bpm;
    }
    if (meta.start_time_ms != 0.0) {
        j_meta["start_time_ms"] = meta.start_time_ms;
    }
    if (meta.preview_start_seconds >= 0.0) {
        j_meta["preview_start_seconds"] = meta.preview_start_seconds;
    }
    if (meta.preview_length_seconds >= 0.0) {
        j_meta["preview_length_seconds"] = meta.preview_length_seconds;
    }

    j["metadata"] = j_meta;

    // Timing events
    json j_timing = json::array();
    for (const auto& event : chart.timing_data().events()) {
        json j_event;
        j_event["beat"] = event.beat;

        if (event.type == TimingEventType::BPM_CHANGE) {
            j_event["type"] = "BPM_CHANGE";
            j_event["bpm"] = event.bpm;
        } else if (event.type == TimingEventType::STOP) {
            j_event["type"] = "STOP";
            j_event["stop_duration"] = event.stop_duration;
        }

        j_timing.push_back(j_event);
    }
    j["timing_events"] = j_timing;

    // Notes
    json j_notes = json::array();
    for (const auto& note : chart.note_data().events()) {
        json j_note;
        j_note["beat"] = note.beat;
        j_note["column"] = note.column;
        j_note["type"] = note_type_to_string_osf(note.type);
        j_notes.push_back(j_note);
    }
    j["notes"] = j_notes;

    // Pretty-print with 2-space indentation
    return j.dump(2);
}

void OsfWriter::write(const Chart& chart, const std::filesystem::path& output_path) const {
    std::string json_str = to_json(chart);

    std::ofstream file(output_path);
    if (!file) {
        throw std::runtime_error("Could not open file for writing: " + output_path.string());
    }

    file << json_str;
    if (!file) {
        throw std::runtime_error("Failed to write to file: " + output_path.string());
    }
}

} // namespace openitup
