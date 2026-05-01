#include <openitup/gfx/texture_cache.h>

#include <algorithm>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace openitup {

static constexpr const char* kProbeExtensions[] = {".tga", ".png", ".dds"};

static std::string to_lower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return r;
}

TextureCache::TextureCache(SDL_Renderer* renderer, ImageLoaderFn loader,
                           size_t memory_threshold_mb)
    : renderer_(renderer), loader_(std::move(loader)),
      memory_threshold_bytes_(memory_threshold_mb * 1024 * 1024) {}

TextureCache::~TextureCache() {
    clear();
}

std::filesystem::path TextureCache::probe(const std::string& name,
                                           const std::filesystem::path& base_dir) {
    auto stem = std::filesystem::path(name).stem().string();

    // Fast path: exact case match
    for (const char* ext : kProbeExtensions) {
        auto candidate = base_dir / (stem + ext);
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    // Case-insensitive: scan directory, compare lowercased stem + extension
    auto stem_lower = to_lower(stem);
    for (const auto& entry : std::filesystem::directory_iterator(base_dir)) {
        if (!entry.is_regular_file()) continue;
        auto entry_stem = to_lower(entry.path().stem().string());
        if (entry_stem != stem_lower) continue;
        auto entry_ext = to_lower(entry.path().extension().string());
        for (const char* ext : kProbeExtensions) {
            if (entry_ext == ext) return entry.path();
        }
    }

    spdlog::error("texture not found: '{}' in {} (probed .tga/.png/.dds, case-insensitive)",
                  name, base_dir.string());
    throw std::runtime_error(
        "Texture not found: '" + name + "' in " + base_dir.string());
}

TextureCache::LoadResult TextureCache::load(const std::string& name,
                                             const std::filesystem::path& base_dir) {
    auto resolved = probe(name, base_dir);
    auto canonical_key = std::filesystem::canonical(resolved).string();

    auto it = path_to_index_.find(canonical_key);
    if (it != path_to_index_.end()) {
        auto idx = it->second;
        entries_[idx].last_access_tick = current_tick_++;
        spdlog::debug("texture cache hit: '{}' -> handle {}", name, idx);
        return {static_cast<TextureHandle>(idx),
                entries_[idx].width, entries_[idx].height};
    }

    spdlog::info("loading texture: '{}' -> {}", name, resolved.string());

    SDL_Surface* surface = loader_(resolved);
    if (!surface) {
        spdlog::error("failed to load image: {}", resolved.string());
        throw std::runtime_error(
            "Failed to load image: " + resolved.string());
    }

    int w = surface->w;
    int h = surface->h;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);

    if (!texture) {
        spdlog::error("failed to create SDL texture from '{}': {}", resolved.string(), SDL_GetError());
        throw std::runtime_error(
            std::string("Failed to create texture: ") + SDL_GetError());
    }

    auto idx = static_cast<uint16_t>(entries_.size());

    // Estimate memory usage: assume RGBA format (4 bytes per pixel)
    size_t texture_bytes = static_cast<size_t>(w) * h * 4;
    current_memory_usage_ += texture_bytes;

    Entry entry;
    entry.texture = texture;
    entry.width = w;
    entry.height = h;
    entry.pinned = false;
    entry.last_access_tick = current_tick_++;

    entries_.push_back(entry);
    path_to_index_[canonical_key] = idx;

    spdlog::info("texture loaded: handle={}, {}x{}, memory={}MB",
                 idx, w, h, current_memory_usage_ / (1024.0 * 1024.0));
    return {static_cast<TextureHandle>(idx), w, h};
}

SDL_Texture* TextureCache::get(TextureHandle h) const {
    auto idx = static_cast<uint16_t>(h);
    if (idx >= entries_.size()) {
        spdlog::error("invalid texture handle: {}", idx);
        return nullptr;
    }
    return entries_[idx].texture;
}

size_t TextureCache::size() const {
    return entries_.size();
}

void TextureCache::clear() {
    for (auto& entry : entries_) {
        if (entry.texture) {
            SDL_DestroyTexture(entry.texture);
        }
    }
    entries_.clear();
    path_to_index_.clear();
    current_memory_usage_ = 0;
    current_tick_ = 0;
}

void TextureCache::pin_texture(const std::string& canonical_path) {
    auto it = path_to_index_.find(canonical_path);
    if (it == path_to_index_.end()) {
        spdlog::warn("cannot pin texture '{}': not in cache", canonical_path);
        return;
    }
    auto idx = it->second;
    entries_[idx].pinned = true;
    spdlog::debug("pinned texture '{}' (handle {})", canonical_path, idx);
}

} // namespace openitup
