#include <openitup/gfx/renderer.h>
#include <openitup/gfx/blend_modes.h>

#include <spdlog/spdlog.h>

namespace openitup {

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init(const std::string& title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::error("SDL_Init failed: {}", SDL_GetError());
        return false;
    }

    window_ = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
    if (!window_) {
        spdlog::error("SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_) {
        spdlog::error("SDL_CreateRenderer failed: {}", SDL_GetError());
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }

    SDL_SetRenderLogicalPresentation(renderer_, 640, 480,
                                      SDL_LOGICAL_PRESENTATION_LETTERBOX);

    init_blend_modes(renderer_);

    return true;
}

void Renderer::shutdown() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void Renderer::begin_frame() {
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
}

void Renderer::end_frame() {
    SDL_RenderPresent(renderer_);
}

} // namespace openitup
