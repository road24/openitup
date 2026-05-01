#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

namespace openitup {

enum class TextureHandle : uint16_t { Invalid = UINT16_MAX };

class TextureCache {
public:
    struct LoadResult {
        TextureHandle handle;
        int width;
        int height;
    };

    // image_loader is a function that loads an image file and returns an
    // SDL_Surface*. The caller (TextureCache) takes ownership of the surface.
    // This indirection allows testing without SDL3_image.
    using ImageLoaderFn = std::function<SDL_Surface*(const std::filesystem::path&)>;

    explicit TextureCache(SDL_Renderer* renderer, ImageLoaderFn loader,
                         size_t memory_threshold_mb = 200);
    ~TextureCache();

    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    // Load or retrieve a cached texture.
    // `name` is the .tga base-name hint from the sprite file (e.g. "arrow.tga").
    // `base_dir` is the directory to search in.
    // Throws on failure (file not found or texture creation error).
    LoadResult load(const std::string& name, const std::filesystem::path& base_dir);

    // Resolve a handle to SDL_Texture* for rendering. O(1).
    SDL_Texture* get(TextureHandle h) const;

    // Number of unique textures currently cached.
    size_t size() const;

    // Destroy all cached textures.
    void clear();

    // Get current GPU memory usage estimate in bytes.
    size_t get_memory_usage_bytes() const { return current_memory_usage_; }

    // Get configured memory threshold in bytes.
    size_t get_memory_threshold_bytes() const { return memory_threshold_bytes_; }

    // Pin a texture so it is never evicted by LRU (future: US-AST-018).
    // If the texture path is not currently cached, logs a warning and does nothing.
    void pin_texture(const std::string& canonical_path);

    // Probe for the actual image file given a .tga base-name hint.
    // Returns the resolved path or throws if no file is found.
    // Public for testing.
    static std::filesystem::path probe(const std::string& name,
                                       const std::filesystem::path& base_dir);

private:
    // Evict least-recently-used unpinned textures until memory usage + bytes_needed
    // is below the threshold.
    void evict_lru_until_below_threshold(size_t bytes_needed);
    SDL_Renderer* renderer_;
    ImageLoaderFn loader_;

    struct Entry {
        SDL_Texture* texture;
        int width;
        int height;
        bool pinned = false;
        uint64_t last_access_tick = 0;
    };

    std::vector<Entry> entries_;
    std::unordered_map<std::string, uint16_t> path_to_index_;

    uint64_t current_tick_ = 0;
    size_t memory_threshold_bytes_;
    size_t current_memory_usage_ = 0;
};

} // namespace openitup
