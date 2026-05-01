#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace openitup {

// A discovered song folder containing at least one chart file.
struct SongEntry {
    std::filesystem::path path;            // Absolute path to the song folder
    std::vector<std::filesystem::path> ksf_files;  // Chart files found in the folder
};

// Recursively scans directories for song folders.
// A song folder is any directory containing at least one .ksf file.
// Hidden directories (starting with .) are skipped.
class SongScanner {
public:
    SongScanner() = default;

    // Recursively scan a directory for song folders.
    // Returns a vector of SongEntry objects, one per discovered song.
    // Handles nested directories (songs inside subdirectories).
    // Skips hidden directories (starting with .).
    // Logs warnings for filesystem errors but continues scanning.
    std::vector<SongEntry> scan_directory(const std::filesystem::path& root_dir) const;

private:
    // Check if a directory is a song folder (contains at least one chart file).
    // Returns vector of chart files found (.ksf for now, future: .ssc, .see, .nx).
    std::vector<std::filesystem::path> find_chart_files(
        const std::filesystem::path& dir) const;

    // Check if a directory should be skipped during scan.
    // Returns true for hidden directories (starting with '.').
    bool should_skip_directory(const std::filesystem::path& dir) const;
};

} // namespace openitup
