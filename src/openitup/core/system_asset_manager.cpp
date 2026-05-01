#include <openitup/core/system_asset_manager.h>

#include <openitup/core/system_paths.h>
#include <openitup/sprite/sprite.h>
#include <openitup/sprite/sprite_loader.h>
#include <openitup/bga/animation.h>
#include <openitup/bga/bga_loader.h>
#include <openitup/gfx/texture_cache.h>
#include <openitup/render/text_renderer.h>
#include <spdlog/spdlog.h>

namespace openitup::core {

std::unique_ptr<SystemAssetManager> SystemAssetManager::create(
    const std::filesystem::path& system_dir_override,
    const std::filesystem::path& binary_path) {

    auto system_dir = find_system_dir(system_dir_override, binary_path);
    if (!system_dir.has_value()) {
        spdlog::error("failed to locate system asset directory");
        return nullptr;
    }

    spdlog::info("system asset directory: {}", system_dir->string());

    // SystemAssetManager needs a TextureCache, but we don't have access to it
    // until Engine initialization. For now, pass nullptr and let Engine set it.
    // This is a temporary limitation that will be resolved when Engine is refactored.
    return std::make_unique<SystemAssetManager>(*system_dir, nullptr);
}

SystemAssetManager::SystemAssetManager(std::filesystem::path system_dir,
                                       TextureCache* texture_cache,
                                       SDL_Renderer* sdl_renderer)
    : system_dir_(std::move(system_dir)), texture_cache_(texture_cache) {
    if (sdl_renderer) {
        text_renderer_ = std::make_unique<TextRenderer>(sdl_renderer);
        spdlog::debug("TextRenderer initialized in SystemAssetManager");
    }
}

SystemAssetManager::~SystemAssetManager() = default;

std::shared_ptr<Sprite> SystemAssetManager::get_sprite(const std::string& name) {
    // Check cache first
    auto it = sprite_cache_.find(name);
    if (it != sprite_cache_.end()) {
        spdlog::debug("system sprite cache hit: {}", name);
        return it->second;
    }

    // Lazy load
    auto sprite_path = system_dir_ / "sprites" / (name + ".sprj");
    if (!std::filesystem::exists(sprite_path)) {
        spdlog::error("system sprite not found: {} (expected at {})", name, sprite_path.string());
        return nullptr;
    }

    if (!texture_cache_) {
        spdlog::error("cannot load sprite '{}': texture cache not available", name);
        return nullptr;
    }

    spdlog::info("loading system sprite: {} from {}", name, sprite_path.string());

    try {
        auto sprite = load_sprite(sprite_path, *texture_cache_);
        auto shared_sprite = std::shared_ptr<Sprite>(std::move(sprite));

        // Pin all textures used by this sprite
        // Note: We need to track which textures this sprite uses.
        // For now, we'll rely on the sprite loader to have loaded them.
        // A more robust approach would be for Sprite to expose texture paths.
        // This is acceptable for Phase 2 since US-AST-018 (LRU eviction) is also Phase 2.

        sprite_cache_[name] = shared_sprite;
        return shared_sprite;
    } catch (const std::exception& e) {
        spdlog::error("failed to load system sprite '{}': {}", name, e.what());
        return nullptr;
    }
}

std::shared_ptr<BgaAnimation> SystemAssetManager::get_animation(const std::string& name) {
    // Check cache first
    auto it = animation_cache_.find(name);
    if (it != animation_cache_.end()) {
        spdlog::debug("system animation cache hit: {}", name);
        return it->second;
    }

    // Lazy load
    auto anim_path = system_dir_ / "animations" / (name + ".bgaj");
    if (!std::filesystem::exists(anim_path)) {
        spdlog::warn("system animation not found: {} (expected at {})", name, anim_path.string());
        return nullptr;  // Animations are optional
    }

    if (!texture_cache_) {
        spdlog::error("cannot load animation '{}': texture cache not available", name);
        return nullptr;
    }

    spdlog::info("loading system animation: {} from {}", name, anim_path.string());

    try {
        auto animation = load_bgaj(anim_path, *texture_cache_);
        auto shared_animation = std::shared_ptr<BgaAnimation>(std::move(animation));

        animation_cache_[name] = shared_animation;
        return shared_animation;
    } catch (const std::exception& e) {
        spdlog::warn("failed to load system animation '{}': {}", name, e.what());
        return nullptr;
    }
}

bool SystemAssetManager::has_sprite(const std::string& name) const {
    if (sprite_cache_.count(name) > 0) {
        return true;
    }
    auto sprite_path = system_dir_ / "sprites" / (name + ".sprj");
    return std::filesystem::exists(sprite_path);
}

}  // namespace openitup::core
