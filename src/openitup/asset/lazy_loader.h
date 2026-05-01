#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include "openitup/asset/song_database.h"
#include "openitup/chart/chart.h"
#include "openitup/bga/animation.h"

namespace openitup {

// Forward declarations
class KsfParser;

// Lazy loader for song resources.
// Loads charts, audio, and BGA on demand rather than during scan.
// Covers US-AST-017, US-AST-019, US-AST-020.
class LazyLoader {
public:
    LazyLoader();
    ~LazyLoader();

    // Load full chart data from a song entry.
    // US-AST-017: Lazy-load chart on song selection.
    // Returns nullopt if chart file doesn't exist or parse fails.
    std::optional<Chart> load_chart(const SongDatabaseEntry& entry, std::size_t chart_index = 0) const;

    // Load audio file path from a song entry.
    // US-AST-019: Lazy-load audio at gameplay start.
    // Returns empty path if audio file doesn't exist.
    // Note: Actual audio loading is handled by AudioSystem.
    std::filesystem::path load_audio(const SongDatabaseEntry& entry) const;

    // Load BGA animation from a song entry.
    // US-AST-020: Lazy-load BGA on song selection.
    // Returns nullptr if BGA file doesn't exist or load fails.
    std::unique_ptr<BgaAnimation> load_bga(const SongDatabaseEntry& entry) const;

private:
    std::unique_ptr<KsfParser> ksf_parser_;
};

} // namespace openitup
