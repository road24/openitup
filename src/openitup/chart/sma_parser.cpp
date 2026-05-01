#include <openitup/chart/sma_parser.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include <openitup/chart/chart_builder.h>
#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>

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

// Trim whitespace from both ends of a string.
std::string trim(const std::string& s) {
    auto start = std::find_if_not(s.begin(), s.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

// Split string by delimiter
std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::istringstream stream(s);
    std::string item;
    while (std::getline(stream, item, delim)) {
        result.push_back(item);
    }
    return result;
}

// Parse BPMs tag: "0.000=120.000,8.000=180.000"
void parse_bpms(ChartBuilder& builder, const std::string& value) {
    auto pairs = split(value, ',');
    for (const auto& pair : pairs) {
        auto kv = split(pair, '=');
        if (kv.size() == 2) {
            try {
                double beat = std::stod(trim(kv[0]));
                double bpm = std::stod(trim(kv[1]));
                builder.add_bpm_change(beat, bpm);
            } catch (const std::exception& e) {
                spdlog::warn("SMA parser: invalid BPM pair '{}': {}", pair, e.what());
            }
        }
    }
}

// Parse STOPS tag: "4.000=1.000,8.000=0.500"
void parse_stops(ChartBuilder& builder, const std::string& value) {
    auto pairs = split(value, ',');
    for (const auto& pair : pairs) {
        auto kv = split(pair, '=');
        if (kv.size() == 2) {
            try {
                double beat = std::stod(trim(kv[0]));
                double duration = std::stod(trim(kv[1]));
                builder.add_stop(beat, duration);
            } catch (const std::exception& e) {
                spdlog::warn("SMA parser: invalid STOP pair '{}': {}", pair, e.what());
            }
        }
    }
}

// Parse note data for one measure. SMA uses measure notation where each line
// is a subdivision of a beat. Returns notes for the measure starting at start_beat.
void parse_measure_notes(ChartBuilder& builder, const std::string& measure_data,
                        double start_beat, int num_columns) {
    // Split measure into lines
    auto lines = split(measure_data, '\n');

    // Filter out empty lines
    std::vector<std::string> note_lines;
    for (const auto& line : lines) {
        std::string trimmed = trim(line);
        if (!trimmed.empty()) {
            note_lines.push_back(trimmed);
        }
    }

    if (note_lines.empty()) {
        return;
    }

    // Each line is a subdivision: beat increment = 4.0 / num_lines (4 beats per measure)
    double beat_increment = 4.0 / note_lines.size();

    for (std::size_t i = 0; i < note_lines.size(); ++i) {
        const auto& line = note_lines[i];
        double beat = start_beat + i * beat_increment;

        // Parse each column
        for (int col = 0; col < num_columns && col < static_cast<int>(line.length()); ++col) {
            char note_char = line[col];

            if (note_char == '1') {
                builder.add_note(beat, col, NoteType::TAP);
            } else if (note_char == '2') {
                builder.add_note(beat, col, NoteType::HOLD_HEAD);
            } else if (note_char == '3') {
                builder.add_note(beat, col, NoteType::HOLD_TAIL);
            } else if (note_char == 'M') {
                builder.add_note(beat, col, NoteType::MINE);
            } else if (note_char == 'F') {
                builder.add_note(beat, col, NoteType::FAKE);
            } else if (note_char == 'L') {
                builder.add_note(beat, col, NoteType::LIFT);
            }
            // '0' or '4' = no note
        }
    }
}

} // anonymous namespace

SmaParser::SmaParser() : file_reader_(read_file_from_disk) {}

SmaParser::SmaParser(FileReaderFn file_reader) : file_reader_(std::move(file_reader)) {}

std::vector<Chart> SmaParser::parse(const std::filesystem::path& chart_path) const {
    std::string content = file_reader_(chart_path);
    if (content.empty()) {
        throw ChartLoadException("SMA file is empty");
    }

    std::vector<Chart> charts;

    // SMA files use tag format: #TAG:value;
    // Global metadata applies to all charts
    std::unordered_map<std::string, std::string> global_tags;

    // Parse file by splitting on #
    std::size_t pos = 0;
    while (pos < content.length()) {
        std::size_t hash_pos = content.find('#', pos);
        if (hash_pos == std::string::npos) {
            break;
        }

        std::size_t colon_pos = content.find(':', hash_pos);
        if (colon_pos == std::string::npos) {
            pos = hash_pos + 1;
            continue;
        }

        std::string tag = trim(content.substr(hash_pos + 1, colon_pos - hash_pos - 1));

        // Find semicolon that ends the value
        std::size_t semi_pos = content.find(';', colon_pos);
        if (semi_pos == std::string::npos) {
            // No semicolon found, take rest of file
            semi_pos = content.length();
        }

        std::string value = content.substr(colon_pos + 1, semi_pos - colon_pos - 1);

        // Store tag
        if (tag == "NOTES") {
            // NOTES tag contains chart-specific data
            // Format: STEPSTYPE:DESCRIPTION:DIFFICULTY:METER:RADAR:NOTES
            auto parts = split(value, ':');
            if (parts.size() >= 6) {
                std::string steps_type = trim(parts[0]);
                std::string description = trim(parts[1]);
                std::string difficulty = trim(parts[2]);
                std::string meter = trim(parts[3]);
                // parts[4] is radar values (unused)
                std::string notes_data = parts[5]; // rest is note data

                // Only parse pump-single5 and pump-double10
                int num_columns = 0;
                PlayMode mode = PlayMode::SINGLE;

                if (steps_type == "pump-single5") {
                    num_columns = 5;
                    mode = PlayMode::SINGLE;
                } else if (steps_type == "pump-double10") {
                    num_columns = 10;
                    mode = PlayMode::DOUBLE;
                } else {
                    spdlog::debug("SMA parser: skipping unsupported chart type '{}'", steps_type);
                    pos = semi_pos + 1;
                    continue;
                }

                // Build chart
                ChartBuilder builder;
                builder.set_mode(mode);

                // Apply global metadata
                if (global_tags.count("TITLE")) {
                    builder.set_title(global_tags["TITLE"]);
                }
                if (global_tags.count("ARTIST")) {
                    builder.set_artist(global_tags["ARTIST"]);
                }
                if (global_tags.count("BPMS")) {
                    parse_bpms(builder, global_tags["BPMS"]);
                }
                if (global_tags.count("STOPS")) {
                    parse_stops(builder, global_tags["STOPS"]);
                }
                if (global_tags.count("DISPLAYBPM")) {
                    try {
                        double display_bpm = std::stod(trim(global_tags["DISPLAYBPM"]));
                        builder.set_display_bpm(display_bpm);
                    } catch (...) {}
                }
                if (global_tags.count("OFFSET")) {
                    try {
                        double offset = std::stod(trim(global_tags["OFFSET"]));
                        builder.set_start_time_ms(offset * 1000.0); // Convert seconds to ms
                    } catch (...) {}
                }
                if (global_tags.count("MUSIC")) {
                    auto chart_dir = chart_path.parent_path();
                    auto audio_resolved = chart_dir / trim(global_tags["MUSIC"]);
                    builder.set_audio_path(audio_resolved.string());
                }
                if (global_tags.count("BANNER")) {
                    auto chart_dir = chart_path.parent_path();
                    auto banner_resolved = chart_dir / trim(global_tags["BANNER"]);
                    builder.set_banner_path(banner_resolved.string());
                }
                if (global_tags.count("BACKGROUND")) {
                    auto chart_dir = chart_path.parent_path();
                    auto bg_resolved = chart_dir / trim(global_tags["BACKGROUND"]);
                    builder.set_background_path(bg_resolved.string());
                }

                // Apply chart-specific metadata
                builder.set_difficulty_name(difficulty);
                try {
                    int rating = std::stoi(meter);
                    builder.set_difficulty_rating(rating);
                } catch (...) {}

                // Parse note data by measures (separated by commas)
                auto measures = split(notes_data, ',');
                double current_beat = 0.0;

                for (const auto& measure : measures) {
                    std::string trimmed_measure = trim(measure);
                    if (!trimmed_measure.empty()) {
                        parse_measure_notes(builder, trimmed_measure, current_beat, num_columns);
                    }
                    current_beat += 4.0; // Each measure is 4 beats
                }

                try {
                    charts.push_back(builder.build());
                } catch (const ChartLoadException& e) {
                    spdlog::warn("SMA parser: failed to build chart '{}': {}", difficulty, e.what());
                }
            }
        } else {
            // Global tag
            global_tags[tag] = trim(value);
        }

        pos = semi_pos + 1;
    }

    if (charts.empty()) {
        spdlog::warn("SMA parser: no pump-single5 or pump-double10 charts found in file");
    }

    return charts;
}

} // namespace openitup
