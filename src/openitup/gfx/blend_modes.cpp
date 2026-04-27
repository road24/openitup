#include <openitup/gfx/blend_modes.h>

#include <spdlog/spdlog.h>

namespace openitup {

namespace {
    SDL_BlendMode g_dodge_mode  = SDL_BLENDMODE_ADD;
    SDL_BlendMode g_diff_mode   = SDL_BLENDMODE_BLEND;
    bool g_custom_available = false;
}

bool init_blend_modes(SDL_Renderer* renderer) {
    // Color Dodge approximation: dst * (src + 1)
    // srcFactor=DST_COLOR, dstFactor=ONE, op=ADD gives src*dst + dst
    g_dodge_mode = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_DST_COLOR,
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_DST_ALPHA,
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDOPERATION_ADD
    );

    // Difference approximation: dst - src
    // REV_SUBTRACT with both factors ONE gives dst*1 - src*1
    g_diff_mode = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDOPERATION_REV_SUBTRACT,
        SDL_BLENDFACTOR_ZERO,
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDOPERATION_ADD
    );

    SDL_Texture* test = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);

    if (test) {
        bool dodge_ok = SDL_SetTextureBlendMode(test, g_dodge_mode);
        bool diff_ok  = SDL_SetTextureBlendMode(test, g_diff_mode);
        g_custom_available = dodge_ok && diff_ok;
        SDL_DestroyTexture(test);
    }

    if (g_custom_available) {
        spdlog::info("custom blend modes supported (dodge, difference)");
    } else {
        spdlog::warn("custom blend modes not supported by renderer — "
                     "dodge falls back to additive, difference falls back to normal");
        g_dodge_mode = SDL_BLENDMODE_ADD;
        g_diff_mode  = SDL_BLENDMODE_BLEND;
    }

    return g_custom_available;
}

SDL_BlendMode resolve_blend_mode(BlendEffect effect) {
    switch (effect) {
        case BlendEffect::Normal:     return SDL_BLENDMODE_BLEND;
        case BlendEffect::Screen:     return SDL_BLENDMODE_ADD;
        case BlendEffect::Multiply:   return SDL_BLENDMODE_MUL;
        case BlendEffect::Dodge:      return g_dodge_mode;
        case BlendEffect::Difference: return g_diff_mode;
    }
    return SDL_BLENDMODE_BLEND;
}

} // namespace openitup
