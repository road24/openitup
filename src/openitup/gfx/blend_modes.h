#pragma once

#include <SDL3/SDL.h>
#include <openitup/bga/keyframe.h>

namespace openitup {

// Initialize custom blend modes (dodge, difference).
// Must be called after renderer creation.
// Returns true if custom modes are supported by the renderer backend.
bool init_blend_modes(SDL_Renderer* renderer);

// Resolve a BlendEffect enum to an SDL_BlendMode.
// Falls back to nearest native mode if custom modes are unavailable.
SDL_BlendMode resolve_blend_mode(BlendEffect effect);

} // namespace openitup
