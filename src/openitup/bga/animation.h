#pragma once

#include <memory>
#include <vector>

#include <SDL3/SDL.h>

#include <openitup/bga/keyframe.h>
#include <openitup/bga/layer.h>
#include <openitup/core/json_serializable.h>
#include <openitup/gfx/texture_cache.h>
#include <openitup/sprite/sprite.h>

namespace openitup {

class BgaAnimation {
public:
    std::vector<Layer> layers;
    std::vector<std::unique_ptr<Sprite>> owned_sprites;

    void render(SDL_Renderer* renderer,
                const TextureCache& cache,
                float tick,
                SDL_BlendMode (*resolve_blend)(BlendEffect) = nullptr) const;

    // Max tick across all layers (last keyframe tick of any layer)
    float max_tick() const;
};

} // namespace openitup
