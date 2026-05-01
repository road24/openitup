#include <openitup/chart/osf_parser.h>

#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <openitup/chart/chart_builder.h>
#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>

using json = nlohmann::json;

namespace openitup {

namespace {

// Default filesystem reader.
std::string read_file_from_disk(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw ChartLoadException("Could not open file: " + path.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Parse mode string to PlayMode enum
PlayMode parse_mode(const std::string& mode_str) {
    if (mode_str == "SINGLE") {
        return PlayMode::SINGLE;
    } else if (mode_str == "DOUBLE") {
        return PlayMode::DOUBLE;
    } else {
        throw ChartLoadException("Invalid mode: " + mode_str + " (expected SINGLE or DOUBLE)");
    }
}

// Parse note type string to NoteType enum
NoteType parse_note_type(const std::string& type_str) {
    if (type_str == "TAP") {
        return NoteType::TAP;
    } else if (type_str == "HOLD_HEAD") {
        return NoteType::HOLD_HEAD;
    } else if (type_str == "HOLD_TAIL") {
        return NoteType::HOLD_TAIL;
    } else if (type_str == "MINE") {
        return NoteType::MINE;
    } else if (type_str == "FAKE") {
        return NoteType::FAKE;
    } else if (type_str == "LIFT") {
        return NoteType::LIFT;
    } else {
        throw ChartLoadException("Invalid note type: " + type_str);
    }
}

} // anonymous namespace

OsfParser::OsfParser() : file_reader_(read_file_from_disk) {}

OsfParser::OsfParser(FileReaderFn file_reader) : file_reader_(std::move(file_reader)) {}

Chart OsfParser::parse(const std::filesystem::path& chart_path) const {
    std::string content = file_reader_(chart_path);
    if (content.empty()) {
        throw ChartLoadException("OSF file is empty");
    }

    // Parse JSON
    json j;
    try {
        j = json::parse(content);
    } catch (const json::exception& e) {
        throw ChartLoadException(std::string("JSON parse error: ") + e.what());
    }

    // Validate top-level structure
    if (!j.is_object()) {
        throw ChartLoadException("OSF root must be a JSON object");
    }
    if (!j.contains("version")) {
        throw ChartLoadException("Missing required field: version");
    }
    if (!j.contains("metadata")) {
        throw ChartLoadException("Missing required field: metadata");
    }
    if (!j.contains("timing_events")) {
        throw ChartLoadException("Missing required field: timing_events");
    }
    if (!j.contains("notes")) {
        throw ChartLoadException("Missing required field: notes");
    }

    // Check version
    std::string version = j["version"].get<std::string>();
    if (version != "1.0") {
        spdlog::warn("OSF parser: unexpected version '{}', expected '1.0'", version);
    }

    ChartBuilder builder;

    // Parse metadata
    const auto& meta = j["metadata"];
    if (!meta.is_object()) {
        throw ChartLoadException("metadata must be an object");
    }

    if (!meta.contains("title") || meta["title"].get<std::string>().empty()) {
        throw ChartLoadException("metadata.title is required and must not be empty");
    }
    builder.set_title(meta["title"].get<std::string>());

    if (!meta.contains("mode")) {
        throw ChartLoadException("metadata.mode is required");
    }
    builder.set_mode(parse_mode(meta["mode"].get<std::string>()));

    // Optional metadata fields
    if (meta.contains("artist")) {
        builder.set_artist(meta["artist"].get<std::string>());
    }
    if (meta.contains("genre")) {
        builder.set_genre(meta["genre"].get<std::string>());
    }
    if (meta.contains("charter_name")) {
        builder.set_charter_name(meta["charter_name"].get<std::string>());
    }
    if (meta.contains("difficulty_name")) {
        builder.set_difficulty_name(meta["difficulty_name"].get<std::string>());
    }
    if (meta.contains("difficulty_rating")) {
        builder.set_difficulty_rating(meta["difficulty_rating"].get<int>());
    }
    if (meta.contains("audio_path")) {
        auto chart_dir = chart_path.parent_path();
        auto audio_resolved = chart_dir / meta["audio_path"].get<std::string>();
        builder.set_audio_path(audio_resolved.string());
    }
    if (meta.contains("intro_path")) {
        auto chart_dir = chart_path.parent_path();
        auto intro_resolved = chart_dir / meta["intro_path"].get<std::string>();
        builder.set_intro_path(intro_resolved.string());
    }
    if (meta.contains("banner_path")) {
        auto chart_dir = chart_path.parent_path();
        auto banner_resolved = chart_dir / meta["banner_path"].get<std::string>();
        builder.set_banner_path(banner_resolved.string());
    }
    if (meta.contains("background_path")) {
        auto chart_dir = chart_path.parent_path();
        auto bg_resolved = chart_dir / meta["background_path"].get<std::string>();
        builder.set_background_path(bg_resolved.string());
    }
    if (meta.contains("display_bpm")) {
        builder.set_display_bpm(meta["display_bpm"].get<double>());
    }
    if (meta.contains("start_time_ms")) {
        builder.set_start_time_ms(meta["start_time_ms"].get<double>());
    }
    if (meta.contains("preview_start_seconds")) {
        builder.set_preview_start_seconds(meta["preview_start_seconds"].get<double>());
    }
    if (meta.contains("preview_length_seconds")) {
        builder.set_preview_length_seconds(meta["preview_length_seconds"].get<double>());
    }

    // Parse timing events
    const auto& timing = j["timing_events"];
    if (!timing.is_array()) {
        throw ChartLoadException("timing_events must be an array");
    }

    for (const auto& event : timing) {
        if (!event.is_object()) {
            spdlog::warn("OSF parser: skipping non-object timing event");
            continue;
        }
        if (!event.contains("type") || !event.contains("beat")) {
            spdlog::warn("OSF parser: timing event missing type or beat");
            continue;
        }

        std::string type = event["type"].get<std::string>();
        double beat = event["beat"].get<double>();

        if (type == "BPM_CHANGE") {
            if (!event.contains("bpm")) {
                spdlog::warn("OSF parser: BPM_CHANGE missing bpm field at beat {}", beat);
                continue;
            }
            double bpm = event["bpm"].get<double>();
            builder.add_bpm_change(beat, bpm);
        } else if (type == "STOP") {
            if (!event.contains("stop_duration")) {
                spdlog::warn("OSF parser: STOP missing stop_duration field at beat {}", beat);
                continue;
            }
            double duration = event["stop_duration"].get<double>();
            builder.add_stop(beat, duration);
        } else {
            spdlog::warn("OSF parser: unknown timing event type '{}'", type);
        }
    }

    // Parse notes
    const auto& notes = j["notes"];
    if (!notes.is_array()) {
        throw ChartLoadException("notes must be an array");
    }

    for (const auto& note : notes) {
        if (!note.is_object()) {
            spdlog::warn("OSF parser: skipping non-object note");
            continue;
        }
        if (!note.contains("beat") || !note.contains("column") || !note.contains("type")) {
            spdlog::warn("OSF parser: note missing required fields");
            continue;
        }

        double beat = note["beat"].get<double>();
        int column = note["column"].get<int>();
        std::string type_str = note["type"].get<std::string>();

        try {
            NoteType type = parse_note_type(type_str);
            builder.add_note(beat, static_cast<uint8_t>(column), type);
        } catch (const ChartLoadException& e) {
            spdlog::warn("OSF parser: {}", e.what());
        }
    }

    // Build and return chart
    return builder.build();
}

} // namespace openitup
