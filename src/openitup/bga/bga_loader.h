#pragma once

#include <filesystem>
#include <memory>

#include <openitup/bga/animation.h>
#include <openitup/gfx/texture_cache.h>

namespace openitup {

// Load a BGA animation from a .bgaj file.
// Parses JSON, loads sprites for each layer (resolving .spr/.sp2 -> .sprj),
// and creates synthetic TILE sprites for raw .tga layer references.
std::unique_ptr<BgaAnimation> load_bgaj(const std::filesystem::path& path,
                                         TextureCache& cache);

// Load directly from BGA binary format.
std::unique_ptr<BgaAnimation> load_bga(const std::filesystem::path& path,
                                        TextureCache& cache);

// Auto-detect by extension (.bgaj, .bga)
std::unique_ptr<BgaAnimation> load_bga_auto(const std::filesystem::path& path,
                                             TextureCache& cache);

} // namespace openitup
