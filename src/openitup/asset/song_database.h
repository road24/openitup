#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace openitup {

// Metadata extracted from a song without full chart parse.
// Corresponds to US-AST-013: metadata extraction during scan.
struct SongMetadata {
    std::string title;
    std::string artist;
    double bpm = 0.0;
    std::string difficulty_name;
};

// A song entry in the database with lazy-loadable resources.
// Covers US-AST-013 through US-AST-031.
struct SongDatabaseEntry {
    std::filesystem::path song_path;           // Absolute path to song folder

    // Metadata (US-AST-013: extracted during scan, no full parse)
    std::string title;
    std::string artist;
    double bpm = 0.0;

    // Chart files (US-AST-017: lazy-loaded on selection)
    std::vector<std::filesystem::path> chart_paths;

    // Asset files discovered during scan
    std::filesystem::path audio_path;          // US-AST-015, US-AST-019
    std::filesystem::path banner_path;         // US-AST-015, US-AST-031
    std::filesystem::path bga_path;            // US-AST-016, US-AST-020

    // US-AST-030: Validation flag (true if song has both chart and audio)
    bool is_valid = false;
};

// Database of discovered songs with metadata and asset paths.
// Handles scanning, metadata extraction, and asset discovery.
class SongDatabase {
public:
    SongDatabase() = default;

    // Scan directories for songs and populate the database.
    // US-AST-012: Recursive scan
    // US-AST-013: Metadata extraction from KSF headers
    // US-AST-015: Banner and audio file discovery
    // US-AST-016: BGA file discovery
    // US-AST-030: Exclude songs missing chart or audio
    // US-AST-031: Missing banner handled gracefully (empty path)
    void scan(const std::vector<std::filesystem::path>& directories);

    // Get all valid songs in the database.
    // Returns only entries with is_valid == true (US-AST-030).
    std::vector<SongDatabaseEntry> get_songs() const;

    // Get a specific song by index.
    // Returns nullopt if index out of range.
    std::optional<SongDatabaseEntry> get_song(std::size_t index) const;

    // Get total count of valid songs.
    std::size_t song_count() const;

private:
    std::vector<SongDatabaseEntry> songs_;

    // Extract metadata from a KSF file (first few lines only, no full parse).
    // US-AST-013: Read TITLE, ARTIST, BPM from header.
    SongMetadata extract_metadata(const std::filesystem::path& ksf_path) const;

    // Discover audio files in a song folder.
    // US-AST-015: Probe for .ogg, .mp3, .wav files.
    // Returns absolute path or empty path if not found.
    std::filesystem::path find_audio_file(const std::filesystem::path& song_dir) const;

    // Discover banner image in a song folder.
    // US-AST-015: Probe for .png, .bmp, .jpg files.
    // US-AST-031: Returns empty path if not found (placeholder shown in UI).
    std::filesystem::path find_banner_file(const std::filesystem::path& song_dir) const;

    // Discover BGA animation in a song folder.
    // US-AST-016: Probe for .bga, .bgaj files.
    // Returns absolute path or empty path if not found.
    std::filesystem::path find_bga_file(const std::filesystem::path& song_dir) const;

    // Validate that a song entry has required files.
    // US-AST-030: Returns true if both chart and audio exist.
    bool validate_entry(const SongDatabaseEntry& entry) const;
};

} // namespace openitup
