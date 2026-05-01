#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace openitup {

// Configuration entry for a single data directory.
struct DataDirEntry {
    std::filesystem::path path;
    bool enabled;
};

// Manages a list of data directories to scan for songs and assets.
// Default: single directory from --data-dir or environment variable.
// Optional: read additional directories from a config file (data_dirs.json).
class DataDirConfig {
public:
    // Construct with a default directory (from CLI or environment variable).
    // Config file loading is separate (see load_from_file()).
    explicit DataDirConfig(std::filesystem::path default_dir);

    // Load additional directories from a JSON config file.
    // Expected format: { "data_dirs": [ { "path": "/path", "enabled": true }, ... ] }
    // Returns true if loaded successfully. Returns false if file does not exist
    // or is invalid (logs WARNING). Invalid entries are skipped, not fatal.
    bool load_from_file(const std::filesystem::path& config_path);

    // Get all enabled directories (default + loaded from config).
    // Returns absolute paths, validated for existence.
    std::vector<std::filesystem::path> get_enabled_directories() const;

    // Get all directories including disabled ones.
    const std::vector<DataDirEntry>& get_all_entries() const;

private:
    std::vector<DataDirEntry> entries_;
};

} // namespace openitup
