#pragma once

#include <string>
#include <SDL3/SDL.h>

namespace openitup {

// Simple text renderer using SDL3's built-in debug text.
// Phase 2 implementation - sufficient for combo/score display and basic UI.
// Full TTF font support deferred to Phase 3.
class TextRenderer {
public:
    explicit TextRenderer(SDL_Renderer* renderer);
    ~TextRenderer() = default;

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    // Draw text at screen coordinates (x, y) with given color.
    // Uses SDL_RenderDebugText which has limited glyph coverage (ASCII).
    void draw_text(const std::string& text, int x, int y, SDL_Color color);

private:
    SDL_Renderer* renderer_;
};

} // namespace openitup
