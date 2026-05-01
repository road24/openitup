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

    // Estimate memory needed for new texture: assume RGBA format (4 bytes per pixel)
    size_t texture_bytes = static_cast<size_t>(w) * h * 4;

    // Evict textures if needed before creating the SDL_Texture
    evict_lru_until_below_threshold(texture_bytes);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);

    if (!texture) {
        spdlog::error("failed to create SDL texture from '{}': {}", resolved.string(), SDL_GetError());
        throw std::runtime_error(
            std::string("Failed to create texture: ") + SDL_GetError());
    }

    auto idx = static_cast<uint16_t>(entries_.size());

    // Update memory usage tracking
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
    // Count only non-null textures (evicted entries have nullptr texture)
    size_t count = 0;
    for (const auto& entry : entries_) {
        if (entry.texture != nullptr) {
            count++;
        }
    }
    return count;
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

void TextureCache::evict_lru_until_below_threshold(size_t bytes_needed) {
    // Check if eviction is necessary
    if (current_memory_usage_ + bytes_needed <= memory_threshold_bytes_) {
        return; // No eviction needed
    }

    spdlog::debug("eviction triggered: current={}MB + needed={}MB > threshold={}MB",
                  current_memory_usage_ / (1024.0 * 1024.0),
                  bytes_needed / (1024.0 * 1024.0),
                  memory_threshold_bytes_ / (1024.0 * 1024.0));

    // Collect eviction candidates: (index, last_access_tick, size_bytes)
    struct Candidate {
        uint16_t index;
        uint64_t tick;
        size_t bytes;
        std::string path;
    };
    std::vector<Candidate> candidates;

    for (const auto& [path, idx] : path_to_index_) {
        const auto& entry = entries_[idx];
        if (!entry.pinned) {
            size_t bytes = static_cast<size_t>(entry.width) * entry.height * 4;
            candidates.push_back({idx, entry.last_access_tick, bytes, path});
        }
    }

    // Sort by last_access_tick (oldest first)
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.tick < b.tick;
              });

    // Evict oldest textures until we're below threshold
    size_t target_usage = memory_threshold_bytes_ - bytes_needed;
    for (const auto& candidate : candidates) {
        if (current_memory_usage_ <= target_usage) {
            break; // Evicted enough
        }

        spdlog::info("evicting texture: handle={}, tick={}, {}x{}, memory={}KB",
                     candidate.index,
                     candidate.tick,
                     entries_[candidate.index].width,
                     entries_[candidate.index].height,
                     candidate.bytes / 1024);

        // Destroy the texture
        if (entries_[candidate.index].texture) {
            SDL_DestroyTexture(entries_[candidate.index].texture);
            entries_[candidate.index].texture = nullptr;
        }

        // Update memory usage
        current_memory_usage_ -= candidate.bytes;

        // Remove from path_to_index
        path_to_index_.erase(candidate.path);

        // Mark entry as deleted (we'll keep the slot to maintain handle stability)
        entries_[candidate.index].width = 0;
        entries_[candidate.index].height = 0;
    }

    spdlog::debug("eviction complete: memory={}MB",
                  current_memory_usage_ / (1024.0 * 1024.0));
}

} // namespace openitup
