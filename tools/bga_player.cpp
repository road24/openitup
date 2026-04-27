#include <openitup/bga/animation.h>
#include <openitup/bga/bga_loader.h>
#include <openitup/gfx/blend_modes.h>
#include <openitup/gfx/image_loader.h>
#include <openitup/gfx/renderer.h>
#include <openitup/gfx/texture_cache.h>

#include <CLI/CLI.hpp>
#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

using namespace openitup;

struct Options {
    std::filesystem::path bgaj_path;
    std::filesystem::path asset_dir;
    float start_tick = 0.0f;
    float speed = 1.0f;
    bool snapshot_mode = false;
    float snapshot_tick = 0.0f;
    std::string snapshot_output;
};

static void update_title(SDL_Window* window, float tick, bool paused, float speed) {
    char title[128];
    std::snprintf(title, sizeof(title), "bga_player | tick: %.1f | speed: %.1fx%s",
                  tick, speed, paused ? " [PAUSED]" : "");
    SDL_SetWindowTitle(window, title);
}

static int run_snapshot(const Options& opts) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("snapshot", 640, 480, SDL_WINDOW_HIDDEN);
    if (!window) {
        std::fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return 1;
    }

    SDL_SetRenderLogicalPresentation(renderer, 640, 480,
                                      SDL_LOGICAL_PRESENTATION_LETTERBOX);
    init_blend_modes(renderer);

    TextureCache cache(renderer, load_image);

    std::unique_ptr<BgaAnimation> animation;
    try {
        animation = load_bga_auto(opts.bgaj_path, cache);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Failed to load: %s\n", e.what());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return 1;
    }

    // Render to a target texture
    SDL_Texture* target = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 640, 480);
    if (!target) {
        std::fprintf(stderr, "Failed to create render target: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return 1;
    }

    SDL_SetRenderTarget(renderer, target);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    animation->render(renderer, cache, opts.snapshot_tick, resolve_blend_mode);
    SDL_RenderPresent(renderer);

    // Read pixels from target
    SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
    SDL_SetRenderTarget(renderer, nullptr);

    if (!surface) {
        std::fprintf(stderr, "Failed to read pixels: %s\n", SDL_GetError());
        SDL_DestroyTexture(target);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return 1;
    }

    bool saved = IMG_SavePNG(surface, opts.snapshot_output.c_str());
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(target);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (!saved) {
        std::fprintf(stderr, "Failed to save PNG: %s\n", SDL_GetError());
        return 1;
    }

    std::fprintf(stdout, "Saved tick %.1f to %s\n",
                 opts.snapshot_tick, opts.snapshot_output.c_str());
    return 0;
}

static int run_interactive(const Options& opts) {
    Renderer renderer;
    if (!renderer.init("bga_player", 1280, 960)) {
        std::fprintf(stderr, "Failed to init renderer: %s\n", SDL_GetError());
        return 1;
    }

    TextureCache cache(renderer.get(), load_image);

    std::unique_ptr<BgaAnimation> animation;
    try {
        animation = load_bga_auto(opts.bgaj_path, cache);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Failed to load: %s\n", e.what());
        return 1;
    }

    float max_tick = animation->max_tick();
    float animation_time = opts.start_tick;
    float speed = opts.speed;
    bool paused = false;

    Uint64 last_perf = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();

    update_title(renderer.window(), animation_time, paused, speed);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_SPACE:
                        paused = !paused;
                        update_title(renderer.window(), animation_time, paused, speed);
                        break;
                    case SDLK_LEFT:
                        animation_time -= 1.0f;
                        if (animation_time < 0.0f) animation_time = 0.0f;
                        update_title(renderer.window(), animation_time, paused, speed);
                        break;
                    case SDLK_RIGHT:
                        animation_time += 1.0f;
                        if (max_tick > 0.0f && animation_time >= max_tick) {
                            animation_time = max_tick - 1.0f;
                        }
                        update_title(renderer.window(), animation_time, paused, speed);
                        break;
                    case SDLK_HOME:
                        animation_time = 0.0f;
                        update_title(renderer.window(), animation_time, paused, speed);
                        break;
                    default:
                        break;
                }
            }
        }

        if (!paused) {
            Uint64 now = SDL_GetPerformanceCounter();
            float delta = static_cast<float>(now - last_perf) / static_cast<float>(freq);
            last_perf = now;

            animation_time += delta * 60.0f * speed;
            if (max_tick > 0.0f && animation_time >= max_tick) {
                animation_time = 0.0f;
            }

            update_title(renderer.window(), animation_time, paused, speed);
        } else {
            last_perf = SDL_GetPerformanceCounter();
        }

        renderer.begin_frame();
        animation->render(renderer.get(), cache, animation_time, resolve_blend_mode);
        renderer.end_frame();
    }

    return 0;
}

int main(int argc, char* argv[]) {
    CLI::App app{"BGA animation player and snapshot tool"};
    app.footer("Interactive controls:\n"
               "  Space       Pause / resume\n"
               "  Left/Right  Step -1 / +1 tick\n"
               "  Home        Restart\n"
               "  Escape      Quit");

    Options opts;
    std::string input_str;
    std::string asset_dir_str;
    std::string log_level_str = "info";
    std::vector<std::string> snapshot_args;

    app.add_option("input", input_str, "Input .bgaj file")->required();
    app.add_option("--tick", opts.start_tick, "Start at tick N (default: 0)");
    app.add_option("--speed", opts.speed, "Playback speed multiplier (default: 1.0)");
    app.add_option("--asset-dir", asset_dir_str,
        "Base directory for asset lookup (default: .bgaj file directory)");
    app.add_option("--snapshot", snapshot_args,
        "Render tick N to PNG and exit: --snapshot <tick> <output.png>")
        ->expected(2);
    app.add_option("--log-level", log_level_str,
        "Log level: trace, debug, info, warn, error, off (default: info)");

    CLI11_PARSE(app, argc, argv);

    spdlog::set_level(spdlog::level::from_str(log_level_str));

    opts.bgaj_path = input_str;
    opts.asset_dir = asset_dir_str.empty()
        ? opts.bgaj_path.parent_path()
        : std::filesystem::path(asset_dir_str);

    if (!snapshot_args.empty()) {
        opts.snapshot_mode = true;
        opts.snapshot_tick = std::stof(snapshot_args[0]);
        opts.snapshot_output = snapshot_args[1];
    }

    if (opts.snapshot_mode) {
        return run_snapshot(opts);
    }

    return run_interactive(opts);
}
