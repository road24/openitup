#include <openitup/bga/animation.h>

#include <spdlog/spdlog.h>

namespace openitup {

static SDL_BlendMode default_resolve_blend(BlendEffect effect) {
    switch (effect) {
        case BlendEffect::Screen:     return SDL_BLENDMODE_ADD;
        case BlendEffect::Multiply:   return SDL_BLENDMODE_MUL;
        default:                      return SDL_BLENDMODE_BLEND;
    }
}

void BgaAnimation::render(SDL_Renderer* renderer,
                          const TextureCache& cache,
                          float tick,
                          SDL_BlendMode (*resolve_blend)(BlendEffect)) const {
    if (!resolve_blend) resolve_blend = default_resolve_blend;

    int layer_idx = 0;
    for (const auto& layer : layers) {
        if (!layer.sprite) {
            ++layer_idx;
            continue;
        }

        auto props = layer.evaluate(tick);
        if (!props.has_value()) {
            spdlog::trace("layer {} '{}': invisible at tick {:.1f}",
                          layer_idx, layer.sprite_name, tick);
            ++layer_idx;
            continue;
        }

        spdlog::trace("layer {} '{}': tick={:.1f} dt={:.3f} translate=({:.1f},{:.1f}) "
                      "scale=({:.2f},{:.2f}) rotate={:.1f} color=({:.2f},{:.2f},{:.2f},{:.2f}) "
                      "effect={} display={}",
                      layer_idx, layer.sprite_name, tick, props->dt,
                      props->translate_x, props->translate_y,
                      props->scale_x, props->scale_y, props->rotate,
                      props->color_r, props->color_g, props->color_b, props->color_a,
                      blend_effect_to_string(props->effect), props->display);

        LayerTransform xform{};
        xform.translate_x = props->translate_x;
        xform.translate_y = props->translate_y;
        xform.pivot_x     = props->pivot_x;
        xform.pivot_y     = props->pivot_y;
        xform.scale_x     = props->scale_x;
        xform.scale_y     = props->scale_y;
        xform.rotate      = props->rotate;

        ColorMod color{props->color_r, props->color_g, props->color_b, props->color_a};
        SDL_BlendMode blend = resolve_blend(props->effect);

        layer.sprite->draw(renderer, cache, props->dt, xform, color, blend);
        ++layer_idx;
    }
}

float BgaAnimation::max_tick() const {
    float max_t = 0.0f;
    for (const auto& layer : layers) {
        if (!layer.keyframes.empty()) {
            float last = static_cast<float>(layer.keyframes.back().tick);
            if (last > max_t) max_t = last;
        }
    }
    return max_t;
}

} // namespace openitup
