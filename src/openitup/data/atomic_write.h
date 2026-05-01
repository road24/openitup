#pragma once

#include <filesystem>
#include <string>

namespace openitup::data {

// Atomically write content to a file.
// 1. Writes to <path>.tmp in the same directory.
// 2. On success, renames <path>.tmp to <path> (atomic on POSIX).
// 3. On failure, removes <path>.tmp (if it exists) and returns false.
//
// Logs ERROR on failure with path and error details.
// Returns true on success.
bool atomic_write_file(const std::filesystem::path& path,
                       const std::string& content);

} // namespace openitup::data
