#include "openitup/asset/song_scanner.h"

#include <algorithm>

#include <spdlog/spdlog.h>

namespace openitup {

std::vector<SongEntry> SongScanner::scan_directory(
    const std::filesystem::path& root_dir) const {

    std::vector<SongEntry> songs;

    if (!std::filesystem::exists(root_dir)) {
        spdlog::error("Scan directory does not exist: {}", root_dir.string());
        return songs;
    }

    if (!std::filesystem::is_directory(root_dir)) {
        spdlog::error("Scan path is not a directory: {}", root_dir.string());
        return songs;
    }

    spdlog::info("Scanning directory for songs: {}", root_dir.string());

    try {
        // Recursive iteration through the directory tree
        std::filesystem::recursive_directory_iterator it(
            root_dir, std::filesystem::directory_options::skip_permission_denied);
        std::filesystem::recursive_directory_iterator end;

        for (; it != end; ++it) {
            const auto& entry = *it;

            try {
                // Skip non-directories
                if (!entry.is_directory()) {
                    continue;
                }

                const auto& dir_path = entry.path();

                // Skip hidden directories
                if (should_skip_directory(dir_path)) {
                    // Don't descend into this directory
                    it.disable_recursion_pending();
                    continue;
                }

                // Check if this directory contains chart files
                auto chart_files = find_chart_files(dir_path);
                if (!chart_files.empty()) {
                    SongEntry song;
                    song.path = dir_path;
                    song.ksf_files = std::move(chart_files);
                    songs.push_back(std::move(song));

                    spdlog::debug("Found song folder: {} ({} charts)",
                                 dir_path.filename().string(),
                                 songs.back().ksf_files.size());
                }

            } catch (const std::filesystem::filesystem_error& e) {
                // Log warning but continue scanning
                spdlog::warn("Error scanning {}: {}", entry.path().string(), e.what());
                continue;
            }
        }

    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Failed to scan directory {}: {}", root_dir.string(), e.what());
        return songs;
    }

    spdlog::info("Scan complete: found {} song folders", songs.size());
    return songs;
}

std::vector<std::filesystem::path> SongScanner::find_chart_files(
    const std::filesystem::path& dir) const {

    std::vector<std::filesystem::path> chart_files;

    try {
        // Only scan immediate children (non-recursive)
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string ext = entry.path().extension().string();
            // Lowercase comparison for case-insensitive match
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            // Phase 1: only .ksf files
            // Future phases: add .ssc, .sma, .stx, .see, .nx
            if (ext == ".ksf") {
                chart_files.push_back(entry.path());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::warn("Error reading directory {}: {}", dir.string(), e.what());
    }

    return chart_files;
}

bool SongScanner::should_skip_directory(const std::filesystem::path& dir) const {
    std::string filename = dir.filename().string();

    // Skip hidden directories (starting with '.')
    if (!filename.empty() && filename[0] == '.') {
        return true;
    }

    return false;
}

} // namespace openitup
