#include <openitup/bga/animation.h>
#include <openitup/bga/bga_loader.h>
#include <openitup/gfx/blend_modes.h>
#include <openitup/gfx/image_loader.h>
#include <openitup/gfx/renderer.h>
#include <openitup/gfx/texture_cache.h>

#include <cstdio>
#include <filesystem>

using namespace openitup;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: openitup <file.bgaj>\n");
        return 1;
    }

    std::filesystem::path bgaj_path(argv[1]);

    Renderer renderer;
    if (!renderer.init("openitup", 1280, 960)) {
        std::fprintf(stderr, "Failed to init renderer: %s\n", SDL_GetError());
        return 1;
    }

    TextureCache cache(renderer.get(), load_image);

    auto animation = load_bga_auto(bgaj_path, cache);
    float max_tick = animation->max_tick();

    float animation_time = 0.0f;
    Uint64 last_ticks = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN &&
                event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float delta = static_cast<float>(now - last_ticks) / static_cast<float>(freq);
        last_ticks = now;

        animation_time += delta * 60.0f;
        if (max_tick > 0.0f && animation_time >= max_tick) {
            animation_time = 0.0f;
        }

        renderer.begin_frame();
        animation->render(renderer.get(), cache, animation_time, resolve_blend_mode);
        renderer.end_frame();
    }

    return 0;
}
