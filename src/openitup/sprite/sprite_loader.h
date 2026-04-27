#pragma once

#include <filesystem>
#include <memory>

#include <openitup/gfx/texture_cache.h>
#include <openitup/sprite/sprite.h>

namespace openitup {

std::unique_ptr<Sprite> load_sprj(const std::filesystem::path& path,
                                   TextureCache& cache);

// Load directly from SPR text format. Requires textures for UV normalization.
std::unique_ptr<Sprite> load_spr(const std::filesystem::path& path,
                                  TextureCache& cache);

// Load directly from SP2 text format. Fixed 256 divisor, no texture dims needed.
std::unique_ptr<Sprite> load_sp2(const std::filesystem::path& path,
                                  TextureCache& cache);

// Auto-detect by extension (.sprj, .spr, .sp2)
std::unique_ptr<Sprite> load_sprite(const std::filesystem::path& path,
                                     TextureCache& cache);

} // namespace openitup
