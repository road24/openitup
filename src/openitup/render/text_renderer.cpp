#include <openitup/render/text_renderer.h>

#include <spdlog/spdlog.h>

namespace openitup {

TextRenderer::TextRenderer(SDL_Renderer* renderer)
    : renderer_(renderer) {
    spdlog::debug("TextRenderer initialized with SDL_RenderDebugText");
}

void TextRenderer::draw_text(const std::string& text, int x, int y, SDL_Color color) {
    if (text.empty()) {
        return;
    }

    // Save current draw color
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer_, &r, &g, &b, &a);

    // Set text color
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);

    // Draw debug text (SDL3 built-in function)
    SDL_RenderDebugText(renderer_, static_cast<float>(x), static_cast<float>(y), text.c_str());

    // Restore previous draw color
    SDL_SetRenderDrawColor(renderer_, r, g, b, a);
}

} // namespace openitup
