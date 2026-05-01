#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace openitup::data {

struct DifficultyEntry {
    std::string type;       // "single" or "double"
    int level = 0;          // numeric difficulty rating
};

struct ChartCacheEntry {
    std::filesystem::path path;     // absolute path to chart file
    std::string hash;               // hex-encoded SHA-256 (Phase 5, empty in Phase 3)
    std::string title;
    std::string artist;
    std::vector<DifficultyEntry> difficulties;
};

class SongCache {
public:
    // Injectable file I/O for testing.
    using FileReaderFn = std::function<std::string(const std::filesystem::path&)>;
    using FileWriterFn = std::function<bool(const std::filesystem::path&,
                                            const std::string&)>;

    // Construct with the path to the cache file.
    explicit SongCache(std::filesystem::path cache_path);

    // Injectable constructor for testing.
    SongCache(std::filesystem::path cache_path,
              FileReaderFn reader,
              FileWriterFn writer);

    // Load the cache from disk.
    // Returns true if the cache was loaded successfully.
    // Returns false if the cache is missing or corrupt.
    bool load();

    // Save the cache to disk using atomic write.
    bool save() const;

    // Check if a cache entry is stale by comparing its mtime against the file.
    // Returns true if the entry is fresh (file mtime matches or is older).
    bool is_fresh(const std::vector<std::filesystem::path>& song_dirs) const;

    // Replace the cache contents (called after a full directory scan).
    void set_entries(std::vector<ChartCacheEntry> entries);

    // Read-only access to cached entries.
    const std::vector<ChartCacheEntry>& entries() const;

    // Get a specific entry by path.
    std::optional<ChartCacheEntry> get(const std::filesystem::path& path) const;

    // Add or update an entry.
    void put(const ChartCacheEntry& entry);

    // Number of cached entries.
    size_t size() const;

    // The timestamp when the cache was last updated.
    const std::string& last_updated() const;

private:
    std::filesystem::path cache_path_;
    std::string last_updated_;          // ISO 8601 timestamp
    int schema_version_ = 1;
    std::vector<ChartCacheEntry> entries_;
    FileReaderFn reader_;
    FileWriterFn writer_;
};

} // namespace openitup::data
