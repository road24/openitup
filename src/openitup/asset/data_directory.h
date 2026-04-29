#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace openitup {

// A value type that validates and resolves a game data directory path.
// Provides helper methods to locate specific asset types within the directory.
class DataDirectory {
public:
    // Construct from a path. Resolves to absolute. Does NOT validate existence
    // at construction (validation is a separate step for clearer error reporting).
    explicit DataDirectory(std::filesystem::path path);

    // Validate that the directory exists and is accessible.
    // Returns true if valid. Logs ERROR with path if not.
    bool validate() const;

    // The resolved absolute path to the data directory.
    const std::filesystem::path& path() const;

    // Find the first file matching an extension in the directory.
    // Case-insensitive extension match.
    // Returns nullopt if no match found.
    std::optional<std::filesystem::path> find_file_by_extension(
        const std::string& extension) const;

    // Find a specific file by name (case-insensitive).
    // Returns nullopt if not found.
    std::optional<std::filesystem::path> find_file_ci(
        const std::string& filename) const;

private:
    std::filesystem::path path_;
};

// Resolve the data directory from CLI argument and environment variable.
// Priority: cli_path > OPENITUP_DATA_DIR > empty (error).
// Returns nullopt if no path available (logs ERROR with instructions).
std::optional<DataDirectory> resolve_data_directory(
    const std::string& cli_path);

} // namespace openitup
