#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <openitup/asset/song_database.h>

namespace openitup {

// JSON-serializable cache for song database.
// Stores the full SongDatabase plus directory metadata for invalidation.
// Phase 3 implementation: supports warm start by loading cache if valid.
class SongCache {
public:
    SongCache() = default;

    // Load cache from disk. Returns nullopt if file doesn't exist or is invalid.
    static std::optional<SongCache> load(const std::filesystem::path& cache_path);

    // Save cache to disk. Returns false on failure.
    bool save(const std::filesystem::path& cache_path) const;

    // Check if cache is valid for the given data directories.
    // Cache is valid if all directories exist and none have been modified
    // since the cache was created.
    bool is_valid_for(const std::vector<std::filesystem::path>& data_dirs) const;

    // Get the cached song database.
    const SongDatabase& get_database() const { return database_; }

    // Set the song database (for creating new cache).
    void set_database(SongDatabase database);

    // Set the data directories this cache was built from.
    void set_data_directories(std::vector<std::filesystem::path> dirs);

private:
    SongDatabase database_;
    std::vector<std::filesystem::path> data_directories_;
    // TODO Phase 3: add directory mtimes for invalidation
};

} // namespace openitup
