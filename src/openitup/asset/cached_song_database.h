#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <openitup/asset/song_database.h>

namespace openitup {

// Cached wrapper around SongDatabase for startup performance.
// US-AST-014: On startup, load cache if valid; otherwise scan and save cache.
// Phase 3 implementation.
class CachedSongDatabase {
public:
    CachedSongDatabase();

    // Initialize the database: load cache or scan directories.
    // cache_path: where to load/save the cache (typically ~/.cache/openitup/songdb.json)
    // data_dirs: directories to scan for songs
    // Returns true if initialization succeeded.
    bool initialize(const std::filesystem::path& cache_path,
                   const std::vector<std::filesystem::path>& data_dirs);

    // Get all valid songs.
    std::vector<SongDatabaseEntry> get_songs() const;

    // Get a specific song by index.
    std::optional<SongDatabaseEntry> get_song(std::size_t index) const;

    // Get total count of valid songs.
    std::size_t song_count() const;

private:
    std::unique_ptr<SongDatabase> database_;
    std::filesystem::path cache_path_;
    std::vector<std::filesystem::path> data_dirs_;

    // Try to load cache. Returns true if cache was valid and loaded.
    bool try_load_cache();

    // Scan directories and build database from scratch.
    void scan_directories();

    // Save current database to cache.
    void save_cache();

    // Check if cache is valid for current data directories.
    bool is_cache_valid() const;
};

} // namespace openitup
