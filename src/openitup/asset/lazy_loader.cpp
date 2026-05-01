#include "openitup/asset/lazy_loader.h"

#include <spdlog/spdlog.h>

#include "openitup/chart/chart_builder.h"
#include "openitup/chart/ksf_parser.h"
#include "openitup/bga/bga_loader.h"

namespace openitup {

LazyLoader::LazyLoader() : ksf_parser_(std::make_unique<KsfParser>()) {}

LazyLoader::~LazyLoader() = default;

std::optional<Chart> LazyLoader::load_chart(const SongDatabaseEntry& entry,
                                             std::size_t chart_index) const {
    // US-AST-017: Lazy-load full chart data on song selection
    if (chart_index >= entry.chart_paths.size()) {
        spdlog::error("LazyLoader: chart index {} out of range (song has {} charts)",
                     chart_index, entry.chart_paths.size());
        return std::nullopt;
    }

    const auto& chart_path = entry.chart_paths[chart_index];

    if (!std::filesystem::exists(chart_path)) {
        spdlog::error("LazyLoader: chart file does not exist: {}", chart_path.string());
        return std::nullopt;
    }

    try {
        spdlog::info("LazyLoader: loading chart from {}", chart_path.string());
        Chart chart = ksf_parser_->parse(chart_path);
        return chart;
    } catch (const ChartLoadException& e) {
        spdlog::error("LazyLoader: failed to parse chart {}: {}", chart_path.string(), e.what());
        return std::nullopt;
    } catch (const std::exception& e) {
        spdlog::error("LazyLoader: unexpected error loading chart {}: {}", chart_path.string(), e.what());
        return std::nullopt;
    }
}

std::filesystem::path LazyLoader::load_audio(const SongDatabaseEntry& entry) const {
    // US-AST-019: Lazy-load audio at gameplay start
    // This method returns the path; actual audio loading is done by AudioSystem
    if (entry.audio_path.empty()) {
        spdlog::warn("LazyLoader: song '{}' has no audio file", entry.title);
        return {};
    }

    if (!std::filesystem::exists(entry.audio_path)) {
        spdlog::error("LazyLoader: audio file does not exist: {}", entry.audio_path.string());
        return {};
    }

    spdlog::debug("LazyLoader: audio path for '{}': {}", entry.title, entry.audio_path.string());
    return entry.audio_path;
}

std::unique_ptr<BgaAnimation> LazyLoader::load_bga(const SongDatabaseEntry& entry) const {
    // US-AST-020: Lazy-load BGA on song selection
    // NOTE: BGA loading requires a TextureCache parameter.
    // This method cannot be implemented without either:
    // 1. LazyLoader owning/storing a TextureCache reference, or
    // 2. Changing this method to accept a TextureCache parameter.
    // For now, returning nullptr to allow compilation.
    // TODO: Fix architecture to pass TextureCache through.

    if (entry.bga_path.empty()) {
        spdlog::debug("LazyLoader: song '{}' has no BGA file", entry.title);
        return nullptr;
    }

    spdlog::error("LazyLoader::load_bga: not implemented (TextureCache dependency needed)");
    return nullptr;
}

} // namespace openitup
