#include <openitup/core/engine.h>

#include <stdexcept>

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

namespace openitup {

Engine::Engine(const EngineConfig& config)
    : Engine(config, std::make_unique<Clock>()) {}

Engine::Engine(const EngineConfig& config, std::unique_ptr<Clock> clock)
    : config_(config), clock_(std::move(clock)) {
    if (config_.target_fps > 0.0) {
        target_frame_time_ = 1.0 / config_.target_fps;
    }
    init_renderer(config_);
}

Engine::~Engine() = default;

void Engine::init_renderer(const EngineConfig& config) {
    renderer_ = std::make_unique<Renderer>();
    if (!renderer_->init(config.window_title, config.window_width, config.window_height)) {
        spdlog::critical("failed to initialize renderer: {}", SDL_GetError());
        throw std::runtime_error(
            std::string("Failed to initialize renderer: ") + SDL_GetError());
    }
    spdlog::info("renderer initialized: {}x{}", config.window_width, config.window_height);
}

int Engine::run() {
    running_ = true;
    tick_count_ = 0;
    accumulator_ = 0.0;
    clock_->reset();

    while (running_) {
        process_events();

        double delta = clock_->tick();

        auto result = compute_fixed_steps(delta, accumulator_);
        accumulator_ = result.new_accumulator;
        render_alpha_ = result.alpha;

        if (result.spiral_guard_triggered) {
            spdlog::warn("spiral-of-death guard: discarded excess time");
        }

        for (int i = 0; i < result.num_steps; i++) {
            try {
                update(FIXED_STEP);
            } catch (const std::exception& e) {
                spdlog::error("exception in update: {}", e.what());
            }
            tick_count_++;
        }

        renderer_->begin_frame();
        try {
            render(render_alpha_);
        } catch (const std::exception& e) {
            spdlog::error("exception in render: {}", e.what());
        }
        renderer_->end_frame();

        if (target_frame_time_ > 0.0) {
            double frame_elapsed = clock_->tick();
            double remaining = target_frame_time_ - frame_elapsed;
            if (remaining > 0.001) {
                SDL_Delay(static_cast<uint32_t>(remaining * 1000.0));
            }
        }
    }

    return 0;
}

void Engine::process_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            request_quit();
        }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
            request_quit();
        }
    }
}

void Engine::set_input_system(std::unique_ptr<InputSystem> input) {
    input_system_ = std::move(input);
}

void Engine::update(double /*dt*/) {
    if (input_system_) {
        input_system_->poll(tick_count_);
    }
    // Future: scene_stack_->top().update(dt);
}

void Engine::render(double /*alpha*/) {
    // Future: scene_stack_->render(alpha);
}

void Engine::request_quit() {
    running_ = false;
}

} // namespace openitup
