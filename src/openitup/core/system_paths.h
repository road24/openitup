#pragma once

#include <filesystem>
#include <optional>

namespace openitup::core {

// Find the system asset directory using a search heuristic.
// Search order:
// 1. cli_override (if non-empty and exists)
// 2. OPENITUP_SYSTEM_DIR environment variable (if set and exists)
// 3. ./data/system/ relative to CWD
// 4. <binary_dir>/../data/system/ relative to binary path
// 5. /usr/share/openitup/data/system/ (Linux install path)
//
// Returns nullopt if no valid system directory is found.
// All paths are canonicalized to resolve symlinks.
std::optional<std::filesystem::path> find_system_dir(
    const std::filesystem::path& cli_override,
    const std::filesystem::path& binary_path);

}  // namespace openitup::core
