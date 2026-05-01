#include <openitup/chart/ksf_parser.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <array>

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

// Split a string into lines.
std::vector<std::string> split_lines(const std::string& content) {
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        // Remove trailing \r if present (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
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

// Parse a metadata tag line (#TAG:VALUE;).
// Returns {tag, value} if valid, or {"", ""} if not a tag line.
std::pair<std::string, std::string> parse_tag(const std::string& line) {
    if (line.empty() || line[0] != '#') {
        return {"", ""};
    }

    auto colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        return {"", ""};
    }

    std::string tag = line.substr(1, colon_pos - 1);
    std::string value = line.substr(colon_pos + 1);

    // Remove trailing semicolon if present
    if (!value.empty() && value.back() == ';') {
        value.pop_back();
    }

    return {trim(tag), trim(value)};
}

// Check if a line is note data (5 characters of digits).
bool is_note_data_line(const std::string& line) {
    if (line.length() < 5) {
        return false;
    }
    // Note data lines start with 5 digit characters
    for (int i = 0; i < 5; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(line[i]))) {
            return false;
        }
    }
    return true;
}

// Check if a line marks the end of note data (starts with '2').
bool is_end_of_data(const std::string& line) {
    return !line.empty() && line[0] == '2';
}

} // anonymous namespace

KsfParser::KsfParser() : file_reader_(read_file_from_disk) {}

KsfParser::KsfParser(FileReaderFn file_reader) : file_reader_(std::move(file_reader)) {}

Chart KsfParser::parse(const std::filesystem::path& chart_path) const {
    // Read file contents
    std::string content = file_reader_(chart_path);
    if (content.empty()) {
        throw ChartLoadException("KSF file is empty");
    }

    auto lines = split_lines(content);
    if (lines.empty()) {
        throw ChartLoadException("KSF file is empty");
    }

    ChartBuilder builder;

    // KSF format parameters
    int tickcount = 2;  // TICKCOUNT = rows per beat
    bool has_title = false;
    bool has_bpm = false;

    // Parse metadata section
    std::size_t line_idx = 0;
    for (; line_idx < lines.size(); ++line_idx) {
        const auto& line = lines[line_idx];

        // Check if we've reached note data
        if (is_note_data_line(line)) {
            break;
        }

        // Parse metadata tag
        auto [tag, value] = parse_tag(line);
        if (tag.empty()) {
            continue;  // Not a tag line, skip
        }

        if (tag == "TITLE") {
            builder.set_title(value);
            has_title = true;
        } else if (tag == "ARTIST") {
            builder.set_artist(value);
        } else if (tag == "BPM") {
            try {
                double bpm = std::stod(value);
                builder.add_bpm_change(0.0, bpm);
                builder.set_display_bpm(bpm);
                has_bpm = true;
            } catch (const std::exception& e) {
                spdlog::warn("KSF parser: invalid BPM value '{}': {}", value, e.what());
            }
        } else if (tag == "TICKCOUNT") {
            try {
                tickcount = std::stoi(value);
                if (tickcount <= 0 || tickcount > 64) {
                    spdlog::warn("KSF parser: unusual TICKCOUNT value {}, using anyway", tickcount);
                }
            } catch (const std::exception& e) {
                spdlog::warn("KSF parser: invalid TICKCOUNT value '{}': {}", value, e.what());
            }
        } else if (tag == "STARTTIME") {
            try {
                double ms = std::stod(value);
                builder.set_start_time_ms(ms);
            } catch (const std::exception& e) {
                spdlog::warn("KSF parser: invalid STARTTIME value '{}': {}", value, e.what());
            }
        } else if (tag == "AUDIOFILE" || tag == "SONGFILE") {
            auto chart_dir = chart_path.parent_path();
            auto audio_resolved = chart_dir / value;
            builder.set_audio_path(audio_resolved.string());
        } else if (tag == "INTROFILE") {
            auto chart_dir = chart_path.parent_path();
            auto intro_resolved = chart_dir / value;
            builder.set_intro_path(intro_resolved.string());
        } else if (tag == "DIFFICULTY") {
            builder.set_difficulty_name(value);
        } else if (tag == "SAMPLESTART") {
            try {
                double start = std::stod(value);
                builder.set_preview_start_seconds(start);
            } catch (const std::exception& e) {
                spdlog::warn("KSF parser: invalid SAMPLESTART value '{}': {}", value, e.what());
            }
        } else if (tag == "SAMPLELENGTH") {
            try {
                double length = std::stod(value);
                builder.set_preview_length_seconds(length);
            } catch (const std::exception& e) {
                spdlog::warn("KSF parser: invalid SAMPLELENGTH value '{}': {}", value, e.what());
            }
        } else if (tag == "STEP") {
            // STEP tag marks the beginning of note data
            line_idx++;  // Move to next line after STEP
            break;
        } else {
            spdlog::debug("KSF parser: unrecognized tag '{}'", tag);
        }
    }

    // Warning if no title
    if (!has_title) {
        spdlog::warn("KSF parser: no TITLE tag found, using filename as fallback");
        builder.set_title(chart_path.stem().string());
    }

    // Parse note data section
    int tick = 0;
    bool found_note_data = false;
    std::array<bool, 5> hold_active = {false, false, false, false, false};

    for (; line_idx < lines.size(); ++line_idx) {
        const auto& line = lines[line_idx];

        // Check for end of data marker
        if (is_end_of_data(line)) {
            break;
        }

        // Check if this is a valid note data line
        if (!is_note_data_line(line)) {
            // Skip empty lines or non-note-data lines
            if (!line.empty() && line.find_first_not_of(" \t") != std::string::npos) {
                spdlog::debug("KSF parser: skipping non-note-data line: '{}'", line);
            }
            continue;
        }

        found_note_data = true;
        double beat = static_cast<double>(tick) / tickcount;

        // Parse each column (5 columns for single mode)
        for (int col = 0; col < 5; ++col) {
            char note_char = line[col];

            if (note_char == '1') {
                // Tap note (or hold body if hold is active on this column)
                if (!hold_active[col]) {
                    builder.add_note(beat, col, NoteType::TAP);
                }
                // If hold is active, '1' is a hold body - ignore it
            } else if (note_char == '4') {
                // Hold head
                builder.add_note(beat, col, NoteType::HOLD_HEAD);
                hold_active[col] = true;
            } else if (note_char == '0') {
                // Empty, or hold tail if hold is active
                if (hold_active[col]) {
                    builder.add_note(beat, col, NoteType::HOLD_TAIL);
                    hold_active[col] = false;
                }
            }
            // Other characters (like '2', '3', etc.) are ignored
        }

        tick++;
    }

    // Validate that we found note data
    if (!found_note_data) {
        throw ChartLoadException("KSF file has no note data");
    }

    // KSF is always single mode
    builder.set_mode(PlayMode::SINGLE);

    // Build and return the chart
    // Note: ChartBuilder::build() will throw if no BPM was set
    return builder.build();
}

} // namespace openitup
