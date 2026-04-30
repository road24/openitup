#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <openitup/gfx/texture_cache.h>
#include <openitup/render/noteskin.h>

namespace openitup {

class NoteSkinLoader {
public:
    // Load a noteskin from "<skin_dir>/" directory.
    // skin_dir: path to the noteskin directory (e.g., "noteskin/default").
    // cache: TextureCache for loading textures referenced by SPRJs.
    //
    // Returns a populated NoteSkin. Missing individual SPRJ files produce
    // warnings but do not fail — the corresponding sprite pointer will be null.
    // Throws if the skin directory does not exist.
    static std::unique_ptr<NoteSkin> load(
        const std::filesystem::path& skin_dir,
        TextureCache& cache);

    // Load with automatic fallback: tries primary skin_dir first,
    // then fallback_dir if the primary fails.
    static std::unique_ptr<NoteSkin> load_with_fallback(
        const std::filesystem::path& skin_dir,
        const std::filesystem::path& fallback_dir,
        TextureCache& cache);

private:
    // Try to load a single SPRJ from the skin directory.
    // Returns nullptr (with warning log) if file does not exist.
    static std::unique_ptr<Sprite> try_load_sprj(
        const std::filesystem::path& skin_dir,
        const std::string& filename,
        TextureCache& cache);
};

} // namespace openitup
