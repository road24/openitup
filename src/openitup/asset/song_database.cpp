#include "openitup/asset/song_database.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

#include <spdlog/spdlog.h>

#include "openitup/asset/song_scanner.h"

namespace openitup {

namespace {

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

// Check if a line is note data (starts with 5 digits).
bool is_note_data_line(const std::string& line) {
    if (line.length() < 5) {
        return false;
    }
    for (int i = 0; i < 5; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(line[i]))) {
            return false;
        }
    }
    return true;
}

// Case-insensitive file exists check.
// Probes for file with given base name and each extension in order.
std::filesystem::path probe_file(const std::filesystem::path& dir,
                                  const std::string& base_name,
                                  const std::vector<std::string>& extensions) {
    for (const auto& ext : extensions) {
        auto path = dir / (base_name + ext);
        if (std::filesystem::exists(path)) {
            return path;
        }
        // Try lowercase
        std::string ext_lower = ext;
        std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);
        path = dir / (base_name + ext_lower);
        if (std::filesystem::exists(path)) {
            return path;
        }
        // Try uppercase
        std::string ext_upper = ext;
        std::transform(ext_upper.begin(), ext_upper.end(), ext_upper.begin(), ::toupper);
        path = dir / (base_name + ext_upper);
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return {};
}

} // anonymous namespace

void SongDatabase::scan(const std::vector<std::filesystem::path>& directories) {
    songs_.clear();

    SongScanner scanner;

    for (const auto& dir : directories) {
        if (!std::filesystem::exists(dir)) {
            spdlog::warn("SongDatabase: scan directory does not exist: {}", dir.string());
            continue;
        }

        spdlog::info("SongDatabase: scanning {}", dir.string());
        auto song_entries = scanner.scan_directory(dir);

        for (const auto& song_entry : song_entries) {
            SongDatabaseEntry db_entry;
            db_entry.song_path = song_entry.path;
            db_entry.chart_paths = song_entry.ksf_files;

            // Extract metadata from the first chart file
            if (!song_entry.ksf_files.empty()) {
                auto metadata = extract_metadata(song_entry.ksf_files[0]);
                db_entry.title = metadata.title;
                db_entry.artist = metadata.artist;
                db_entry.bpm = metadata.bpm;
            }

            // Discover asset files
            db_entry.audio_path = find_audio_file(song_entry.path);
            db_entry.banner_path = find_banner_file(song_entry.path);
            db_entry.bga_path = find_bga_file(song_entry.path);

            // Validate entry (US-AST-030: must have chart and audio)
            db_entry.is_valid = validate_entry(db_entry);

            if (db_entry.is_valid) {
                songs_.push_back(std::move(db_entry));
                spdlog::debug("SongDatabase: added valid song: {}", db_entry.title);
            } else {
                spdlog::warn("SongDatabase: skipping invalid song in {}: missing chart or audio",
                           song_entry.path.string());
            }
        }
    }

    spdlog::info("SongDatabase: scan complete, {} valid songs", songs_.size());
}

std::vector<SongDatabaseEntry> SongDatabase::get_songs() const {
    return songs_;
}

std::optional<SongDatabaseEntry> SongDatabase::get_song(std::size_t index) const {
    if (index >= songs_.size()) {
        return std::nullopt;
    }
    return songs_[index];
}

std::size_t SongDatabase::song_count() const {
    return songs_.size();
}

SongMetadata SongDatabase::extract_metadata(const std::filesystem::path& ksf_path) const {
    SongMetadata metadata;

    std::ifstream file(ksf_path);
    if (!file) {
        spdlog::warn("SongDatabase: failed to open KSF file for metadata: {}", ksf_path.string());
        // Use filename as fallback title
        metadata.title = ksf_path.stem().string();
        return metadata;
    }

    // Read only the first ~50 lines (metadata section, before note data)
    std::string line;
    int line_count = 0;
    const int max_metadata_lines = 50;

    while (std::getline(file, line) && line_count < max_metadata_lines) {
        // Remove trailing \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Stop if we hit note data
        if (is_note_data_line(line)) {
            break;
        }

        auto [tag, value] = parse_tag(line);
        if (tag.empty()) {
            line_count++;
            continue;
        }

        if (tag == "TITLE") {
            metadata.title = value;
        } else if (tag == "ARTIST") {
            metadata.artist = value;
        } else if (tag == "BPM") {
            try {
                metadata.bpm = std::stod(value);
            } catch (const std::exception& e) {
                spdlog::warn("SongDatabase: invalid BPM value '{}' in {}", value, ksf_path.string());
            }
        } else if (tag == "STEP") {
            // STEP marks the start of note data, stop reading
            break;
        }

        line_count++;
    }

    // Fallback to filename if no title found
    if (metadata.title.empty()) {
        metadata.title = ksf_path.stem().string();
    }

    return metadata;
}

std::filesystem::path SongDatabase::find_audio_file(const std::filesystem::path& song_dir) const {
    // Probe for common audio files
    // US-AST-015: discover audio files
    std::vector<std::string> audio_extensions = {".ogg", ".mp3", ".wav"};
    std::vector<std::string> audio_base_names = {"song", "audio", "music"};

    for (const auto& base : audio_base_names) {
        auto path = probe_file(song_dir, base, audio_extensions);
        if (!path.empty()) {
            return path;
        }
    }

    // Try finding any audio file in the directory
    try {
        for (const auto& entry : std::filesystem::directory_iterator(song_dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".ogg" || ext == ".mp3" || ext == ".wav") {
                return entry.path();
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::warn("SongDatabase: error searching for audio in {}: {}", song_dir.string(), e.what());
    }

    return {};
}

std::filesystem::path SongDatabase::find_banner_file(const std::filesystem::path& song_dir) const {
    // Probe for common banner files
    // US-AST-015: discover banner files
    // US-AST-031: return empty path if not found (UI shows placeholder)
    std::vector<std::string> banner_extensions = {".png", ".bmp", ".jpg", ".jpeg"};
    std::vector<std::string> banner_base_names = {"banner", "bn", "jacket"};

    for (const auto& base : banner_base_names) {
        auto path = probe_file(song_dir, base, banner_extensions);
        if (!path.empty()) {
            return path;
        }
    }

    return {};
}

std::filesystem::path SongDatabase::find_bga_file(const std::filesystem::path& song_dir) const {
    // Probe for BGA files
    // US-AST-016: discover .bga/.bgaj files
    std::vector<std::string> bga_extensions = {".bgaj", ".bga"};
    std::vector<std::string> bga_base_names = {"song", "bga", "animation"};

    for (const auto& base : bga_base_names) {
        auto path = probe_file(song_dir, base, bga_extensions);
        if (!path.empty()) {
            return path;
        }
    }

    // Try finding any BGA file in the directory
    try {
        for (const auto& entry : std::filesystem::directory_iterator(song_dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".bga" || ext == ".bgaj") {
                return entry.path();
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::warn("SongDatabase: error searching for BGA in {}: {}", song_dir.string(), e.what());
    }

    return {};
}

bool SongDatabase::validate_entry(const SongDatabaseEntry& entry) const {
    // US-AST-030: Song must have at least one chart and an audio file
    bool has_chart = !entry.chart_paths.empty();
    bool has_audio = !entry.audio_path.empty() && std::filesystem::exists(entry.audio_path);

    return has_chart && has_audio;
}

} // namespace openitup
