#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>

// Forward declarations
namespace openitup {
class Sprite;
class TextureCache;
class BgaAnimation;
}  // namespace openitup

namespace openitup::core {

// SystemAssetManager loads and caches system assets (UI sprites, fonts, BGAs)
// separate from user content (songs). All textures loaded by this manager are
// pinned in the TextureCache so they are never evicted by LRU.
class SystemAssetManager {
public:
    // Create a SystemAssetManager.
    // Returns nullptr if system_dir_override is invalid or if system directory
    // cannot be located.
    // binary_path is the path to the current executable (for heuristic search).
    static std::unique_ptr<SystemAssetManager> create(
        const std::filesystem::path& system_dir_override = {},
        const std::filesystem::path& binary_path = {});

    // Asset retrieval - lazy loading with caching
    std::shared_ptr<Sprite> get_sprite(const std::string& name);
    std::shared_ptr<BgaAnimation> get_animation(const std::string& name);

    // Query
    std::filesystem::path system_dir() const { return system_dir_; }
    bool has_sprite(const std::string& name) const;

    // Constructor for internal use (use create() instead)
    SystemAssetManager(std::filesystem::path system_dir, TextureCache* texture_cache);

private:
    std::filesystem::path system_dir_;
    TextureCache* texture_cache_;  // borrowed reference

    std::map<std::string, std::shared_ptr<Sprite>> sprite_cache_;
    std::map<std::string, std::shared_ptr<BgaAnimation>> animation_cache_;
};

}  // namespace openitup::core
