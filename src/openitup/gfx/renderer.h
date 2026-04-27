#pragma once

#include <string>

#include <SDL3/SDL.h>

namespace openitup {

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool init(const std::string& title, int width, int height);
    void shutdown();

    SDL_Renderer* get() const { return renderer_; }
    SDL_Window* window() const { return window_; }

    void begin_frame();
    void end_frame();

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

} // namespace openitup
