#include <openitup/chart/ssc_parser.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <map>
#include <vector>

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

// Convert string to uppercase.
std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
    return s;
}

// Parse a tag line (#TAG:VALUE;).
// Returns {tag, value} if valid, or {"", ""} if not a tag line.
std::pair<std::string, std::string> parse_tag(const std::string& line) {
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] != '#') {
        return {"", ""};
    }

    auto colon_pos = trimmed.find(':');
    if (colon_pos == std::string::npos) {
        return {"", ""};
    }

    std::string tag = trimmed.substr(1, colon_pos - 1);
    std::string value = trimmed.substr(colon_pos + 1);

    // Remove trailing semicolon if present
    if (!value.empty() && value.back() == ';') {
        value.pop_back();
    }

    return {to_upper(trim(tag)), trim(value)};
}

// Parse a STEPSTYPE value to PlayMode.
PlayMode parse_stepstype(const std::string& stepstype) {
    std::string lower = stepstype;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    if (lower == "pump-single") {
        return PlayMode::SINGLE;
    } else if (lower == "pump-double") {
        return PlayMode::DOUBLE;
    } else if (lower == "pump-halfdouble") {
        return PlayMode::HALF;
    } else {
        // Default to single for unknown types
        spdlog::warn("SSC parser: unknown STEPSTYPE '{}', defaulting to SINGLE", stepstype);
        return PlayMode::SINGLE;
    }
}

// Parse a BPM changes string (beat=bpm,beat=bpm,...).
// Returns a vector of (beat, bpm) pairs.
std::vector<std::pair<double, double>> parse_bpms(const std::string& bpms_str) {
    std::vector<std::pair<double, double>> result;
    if (bpms_str.empty()) {
        return result;
    }

    std::istringstream stream(bpms_str);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = trim(token);
        if (token.empty()) continue;

        auto eq_pos = token.find('=');
        if (eq_pos == std::string::npos) {
            spdlog::warn("SSC parser: malformed BPM entry '{}'", token);
            continue;
        }

        try {
            double beat = std::stod(trim(token.substr(0, eq_pos)));
            double bpm = std::stod(trim(token.substr(eq_pos + 1)));
            result.push_back({beat, bpm});
        } catch (const std::exception& e) {
            spdlog::warn("SSC parser: invalid BPM entry '{}': {}", token, e.what());
        }
    }

    return result;
}

// Parse a STOPS string (beat=seconds,beat=seconds,...).
// Returns a vector of (beat, seconds) pairs.
std::vector<std::pair<double, double>> parse_stops(const std::string& stops_str) {
    std::vector<std::pair<double, double>> result;
    if (stops_str.empty()) {
        return result;
    }

    std::istringstream stream(stops_str);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = trim(token);
        if (token.empty()) continue;

        auto eq_pos = token.find('=');
        if (eq_pos == std::string::npos) {
            spdlog::warn("SSC parser: malformed STOP entry '{}'", token);
            continue;
        }

        try {
            double beat = std::stod(trim(token.substr(0, eq_pos)));
            double seconds = std::stod(trim(token.substr(eq_pos + 1)));
            result.push_back({beat, seconds});
        } catch (const std::exception& e) {
            spdlog::warn("SSC parser: invalid STOP entry '{}': {}", token, e.what());
        }
    }

    return result;
}

// Parse note data string (measures separated by commas).
// Each measure has rows, each row has num_columns characters.
// Returns vector of (beat, column, note_type) tuples.
std::vector<std::tuple<double, uint8_t, NoteType>> parse_notes(
    const std::string& notes_str,
    int num_columns
) {
    std::vector<std::tuple<double, uint8_t, NoteType>> result;

    // Split by commas to get measures
    std::vector<std::string> measures;
    std::istringstream stream(notes_str);
    std::string measure_str;
    while (std::getline(stream, measure_str, ',')) {
        measures.push_back(measure_str);
    }

    double current_beat = 0.0;
    std::vector<bool> hold_active(num_columns, false);

    for (const auto& measure : measures) {
        // Extract rows from this measure (non-empty lines)
        std::vector<std::string> rows;
        std::istringstream measure_stream(measure);
        std::string row;
        while (std::getline(measure_stream, row)) {
            row = trim(row);
            if (!row.empty() && row.length() >= static_cast<std::size_t>(num_columns)) {
                rows.push_back(row);
            }
        }

        if (rows.empty()) {
            // Empty measure, advance by 4 beats
            current_beat += 4.0;
            continue;
        }

        // Calculate subdivision: each measure is 4 beats
        double beats_per_row = 4.0 / rows.size();

        // Parse each row
        for (std::size_t row_idx = 0; row_idx < rows.size(); ++row_idx) {
            const auto& row_data = rows[row_idx];
            double beat = current_beat + row_idx * beats_per_row;

            // Parse each column
            for (int col = 0; col < num_columns && col < static_cast<int>(row_data.length()); ++col) {
                char note_char = row_data[col];

                if (note_char == '0') {
                    // Empty or hold tail
                    if (hold_active[col]) {
                        result.push_back({beat, static_cast<uint8_t>(col), NoteType::HOLD_TAIL});
                        hold_active[col] = false;
                    }
                } else if (note_char == '1') {
                    // Tap note
                    if (!hold_active[col]) {
                        result.push_back({beat, static_cast<uint8_t>(col), NoteType::TAP});
                    }
                    // If hold is active, '1' is hold body - ignore
                } else if (note_char == '2') {
                    // Hold head
                    result.push_back({beat, static_cast<uint8_t>(col), NoteType::HOLD_HEAD});
                    hold_active[col] = true;
                } else if (note_char == '3') {
                    // Hold/roll tail (explicit)
                    result.push_back({beat, static_cast<uint8_t>(col), NoteType::HOLD_TAIL});
                    hold_active[col] = false;
                } else if (note_char == '4') {
                    // Roll head (treat as hold head for now)
                    result.push_back({beat, static_cast<uint8_t>(col), NoteType::HOLD_HEAD});
                    hold_active[col] = true;
                } else if (note_char == 'M') {
                    // Mine
                    result.push_back({beat, static_cast<uint8_t>(col), NoteType::MINE});
                } else if (note_char == 'F') {
                    // Fake note
                    result.push_back({beat, static_cast<uint8_t>(col), NoteType::FAKE});
                }
                // Other characters are ignored
            }
        }

        // Advance to next measure
        current_beat += 4.0;
    }

    return result;
}

} // anonymous namespace

SscParser::SscParser() : file_reader_(read_file_from_disk) {}

SscParser::SscParser(FileReaderFn file_reader) : file_reader_(std::move(file_reader)) {}

std::vector<Chart> SscParser::parse(const std::filesystem::path& chart_path) const {
    // Read file contents
    std::string content = file_reader_(chart_path);
    if (content.empty()) {
        throw ChartLoadException("SSC file is empty");
    }

    std::vector<Chart> charts;

    // Song-level metadata (applies to all charts unless overridden)
    std::string song_title;
    std::string song_artist;
    std::string song_music;
    double song_offset = 0.0;
    double song_sample_start = 0.0;
    double song_sample_length = 0.0;
    double song_display_bpm = 0.0;
    std::vector<std::pair<double, double>> song_bpms;
    std::vector<std::pair<double, double>> song_stops;

    // Parse content line by line
    std::istringstream stream(content);
    std::string line;
    bool in_notedata = false;
    std::ostringstream notes_buffer;

    // Per-chart data
    PlayMode chart_mode = PlayMode::SINGLE;
    std::string chart_difficulty;
    int chart_meter = 0;
    std::vector<std::pair<double, double>> chart_bpms;
    std::vector<std::pair<double, double>> chart_stops;
    double chart_offset = song_offset;
    bool has_chart_timing = false;

    while (std::getline(stream, line)) {
        auto [tag, value] = parse_tag(line);

        if (tag.empty()) {
            // Not a tag line
            if (in_notedata && !line.empty()) {
                // Accumulate note data
                notes_buffer << line << "\n";
            }
            continue;
        }

        // Song-level tags
        if (tag == "TITLE") {
            song_title = value;
        } else if (tag == "ARTIST") {
            song_artist = value;
        } else if (tag == "MUSIC") {
            song_music = value;
        } else if (tag == "OFFSET") {
            try {
                song_offset = std::stod(value);
            } catch (const std::exception& e) {
                spdlog::warn("SSC parser: invalid OFFSET value '{}': {}", value, e.what());
            }
        } else if (tag == "SAMPLESTART") {
            try {
                song_sample_start = std::stod(value);
            } catch (const std::exception& e) {
                spdlog::warn("SSC parser: invalid SAMPLESTART value '{}': {}", value, e.what());
            }
        } else if (tag == "SAMPLELENGTH") {
            try {
                song_sample_length = std::stod(value);
            } catch (const std::exception& e) {
                spdlog::warn("SSC parser: invalid SAMPLELENGTH value '{}': {}", value, e.what());
            }
        } else if (tag == "DISPLAYBPM") {
            try {
                song_display_bpm = std::stod(value);
            } catch (const std::exception& e) {
                spdlog::warn("SSC parser: invalid DISPLAYBPM value '{}': {}", value, e.what());
            }
        } else if (tag == "BPMS") {
            song_bpms = parse_bpms(value);
        } else if (tag == "STOPS") {
            song_stops = parse_stops(value);
        } else if (tag == "NOTEDATA") {
            // Start of a new chart
            in_notedata = true;
            notes_buffer.str("");
            notes_buffer.clear();
            chart_mode = PlayMode::SINGLE;
            chart_difficulty = "";
            chart_meter = 0;
            chart_bpms.clear();
            chart_stops.clear();
            chart_offset = song_offset;
            has_chart_timing = false;
        } else if (in_notedata) {
            // Per-chart tags
            if (tag == "STEPSTYPE") {
                chart_mode = parse_stepstype(value);
            } else if (tag == "DIFFICULTY") {
                chart_difficulty = value;
            } else if (tag == "METER") {
                try {
                    chart_meter = std::stoi(value);
                } catch (const std::exception& e) {
                    spdlog::warn("SSC parser: invalid METER value '{}': {}", value, e.what());
                }
            } else if (tag == "BPMS") {
                chart_bpms = parse_bpms(value);
                has_chart_timing = true;
            } else if (tag == "STOPS") {
                chart_stops = parse_stops(value);
                has_chart_timing = true;
            } else if (tag == "OFFSET") {
                try {
                    chart_offset = std::stod(value);
                    has_chart_timing = true;
                } catch (const std::exception& e) {
                    spdlog::warn("SSC parser: invalid chart OFFSET value '{}': {}", value, e.what());
                }
            } else if (tag == "NOTES") {
                // End of chart, build it
                in_notedata = false;

                try {
                    ChartBuilder builder;

                    // Set metadata
                    builder.set_title(song_title.empty() ? chart_path.stem().string() : song_title);
                    builder.set_artist(song_artist);
                    builder.set_mode(chart_mode);
                    builder.set_difficulty_name(chart_difficulty);
                    builder.set_difficulty_rating(chart_meter);

                    // Set audio path
                    if (!song_music.empty()) {
                        auto chart_dir = chart_path.parent_path();
                        auto audio_resolved = chart_dir / song_music;
                        builder.set_audio_path(audio_resolved.string());
                    }

                    // Set preview
                    if (song_sample_start > 0.0) {
                        builder.set_preview_start_seconds(song_sample_start);
                    }
                    if (song_sample_length > 0.0) {
                        builder.set_preview_length_seconds(song_sample_length);
                    }

                    // Use chart-specific timing if present, otherwise song timing
                    const auto& bpms = has_chart_timing && !chart_bpms.empty() ? chart_bpms : song_bpms;
                    const auto& stops = has_chart_timing && !chart_stops.empty() ? chart_stops : song_stops;
                    double offset = has_chart_timing ? chart_offset : song_offset;

                    // Add BPM changes (adjust for offset)
                    for (const auto& [beat, bpm] : bpms) {
                        builder.add_bpm_change(beat, bpm);
                    }

                    // Add stops
                    for (const auto& [beat, seconds] : stops) {
                        builder.add_stop(beat, seconds);
                    }

                    // Set display BPM
                    if (song_display_bpm > 0.0) {
                        builder.set_display_bpm(song_display_bpm);
                    } else if (!bpms.empty()) {
                        builder.set_display_bpm(bpms[0].second);
                    }

                    // Convert offset from seconds to milliseconds and negate
                    // (SSC offset is audio offset, we want chart offset)
                    builder.set_start_time_ms(-offset * 1000.0);

                    // Parse notes
                    int num_columns = max_columns(chart_mode);
                    auto notes = parse_notes(value, num_columns);

                    for (const auto& [beat, col, note_type] : notes) {
                        builder.add_note(beat, col, note_type);
                    }

                    // Build and add chart
                    charts.push_back(builder.build());

                } catch (const ChartLoadException& e) {
                    spdlog::error("SSC parser: failed to build chart: {}", e.what());
                    // Continue to next chart
                }
            }
        }
    }

    if (charts.empty()) {
        throw ChartLoadException("SSC file contains no valid charts");
    }

    return charts;
}

} // namespace openitup
