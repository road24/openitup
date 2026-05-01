#include <openitup/core/engine.h>

#include <stdexcept>

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <openitup/audio/sdl3_audio_system.h>
#include <openitup/chart/chart_builder.h>
#include <openitup/scene/boot_scene.h>
#include <openitup/scene/minimal_gameplay_scene.h>

namespace openitup {

Engine::Engine(const EngineConfig& config)
    : Engine(config, std::make_unique<Clock>()) {}

Engine::Engine(const EngineConfig& config, std::unique_ptr<Clock> clock)
    : Engine(config, std::move(clock), nullptr) {}

Engine::Engine(const EngineConfig& config, std::unique_ptr<Clock> clock, std::unique_ptr<AudioSystem> audio)
    : config_(config), clock_(std::move(clock)), audio_(std::move(audio)) {
    if (config_.target_fps > 0.0) {
        target_frame_time_ = 1.0 / config_.target_fps;
    }
    init_sdl();
    init_renderer(config_);
    init_audio();

    // Initialize system assets
    // TODO: Pass binary path for heuristic search
    system_asset_mgr_ = core::SystemAssetManager::create({}, {});
    if (!system_asset_mgr_) {
        spdlog::warn("system asset manager not initialized (system assets not found)");
        // For now, continue without system assets (they're not strictly required yet)
        // In Phase 2+, this may become a fatal error once UI sprites are required
    } else {
        // Note: SystemAssetManager needs a TextureCache to load sprites/animations.
        // Since Engine doesn't own a TextureCache yet, sprites won't load.
        // This will be resolved when TextureCache is integrated into Engine (future work).
        spdlog::info("system asset manager initialized (asset loading deferred until TextureCache integration)");
    }

    // Initialize text renderer
    text_renderer_ = std::make_unique<TextRenderer>(renderer_->get());
    spdlog::debug("text renderer initialized");

    // Initialize scene stack
    scene_stack_ = std::make_unique<SceneStack>();
    spdlog::debug("scene stack initialized");
}

Engine::~Engine() {
    if (audio_) {
        audio_->shutdown();
    }
    renderer_.reset();
    SDL_Quit();
}

void Engine::init_sdl() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        spdlog::critical("SDL_Init failed: {}", SDL_GetError());
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }
    spdlog::debug("SDL initialized with VIDEO and AUDIO subsystems");
}

void Engine::init_renderer(const EngineConfig& config) {
    renderer_ = std::make_unique<Renderer>();
    if (!renderer_->init(config.window_title, config.window_width, config.window_height)) {
        spdlog::critical("failed to initialize renderer: {}", SDL_GetError());
        throw std::runtime_error(
            std::string("Failed to initialize renderer: ") + SDL_GetError());
    }
    spdlog::info("renderer initialized: {}x{}", config.window_width, config.window_height);
}

void Engine::init_audio() {
    if (audio_) {
        // Audio was injected for testing
        if (!audio_->init()) {
            spdlog::error("failed to initialize injected audio system");
            audio_ = nullptr;
        }
        return;
    }

    // Create default SDL3AudioSystem
    auto sdl_audio = std::make_unique<SDL3AudioSystem>();
    if (!sdl_audio->init()) {
        spdlog::error("failed to initialize SDL3AudioSystem - continuing without audio");
        audio_ = nullptr;
        return;
    }

    audio_ = std::move(sdl_audio);
    spdlog::info("audio system initialized");
}

int Engine::run() {
    running_ = true;
    tick_count_ = 0;
    accumulator_ = 0.0;
    clock_->reset();

    // Push BootScene if stack is empty (default flow)
    if (scene_stack_->empty()) {
        // Find a test chart for gameplay (if data_dir is configured)
        std::filesystem::path test_chart;
        if (!config_.data_dir_path.empty()) {
            // For Phase 2, use the chart_path from config if available
            if (!config_.chart_path.empty()) {
                test_chart = config_.chart_path;
            }
        }
        scene_stack_->push(std::make_unique<BootScene>(
            renderer_.get(),
            text_renderer_.get(),
            scene_stack_.get(),
            this,
            test_chart
        ));
        spdlog::info("Started with BootScene");
    }

    while (running_ && !scene_stack_->empty()) {
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
                if (input_system_) {
                    input_system_->poll(tick_count_);
                    scene_stack_->handle_input(input_system_->snapshot());
                }
                scene_stack_->update(FIXED_STEP);
            } catch (const std::exception& e) {
                spdlog::error("exception in scene update: {}", e.what());
            }
            tick_count_++;
        }

        renderer_->begin_frame();
        try {
            scene_stack_->render();
        } catch (const std::exception& e) {
            spdlog::error("exception in scene render: {}", e.what());
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

    spdlog::info("scene stack empty or quit requested, exiting");
    return 0;
}

int Engine::run_gameplay(const std::filesystem::path& chart_path,
                         const std::filesystem::path& data_dir) {
    try {
        auto scene = std::make_unique<MinimalGameplayScene>(
            chart_path, data_dir,
            audio_.get(),
            input_system_ ? input_system_.get() : nullptr,
            renderer_.get());

        running_ = true;
        clock_->reset();
        accumulator_ = 0.0;
        tick_count_ = 0;

        while (running_ && !scene->is_complete()) {
            process_events();

            double delta = clock_->tick();
            auto result = compute_fixed_steps(delta, accumulator_);
            accumulator_ = result.new_accumulator;
            render_alpha_ = result.alpha;

            if (result.spiral_guard_triggered) {
                spdlog::warn("Spiral-of-death guard triggered");
            }

            for (int i = 0; i < result.num_steps; i++) {
                try {
                    if (input_system_) input_system_->poll(tick_count_);
                    scene->update(FIXED_STEP);
                } catch (const std::exception& e) {
                    spdlog::error("Exception in gameplay update: {}", e.what());
                }
                tick_count_++;
            }

            renderer_->begin_frame();
            try {
                scene->render(render_alpha_);
            } catch (const std::exception& e) {
                spdlog::error("Exception in gameplay render: {}", e.what());
            }
            renderer_->end_frame();
        }

        // Log final results
        const auto& state = scene->gameplay_state();
        spdlog::info("=== Results ===");
        spdlog::info("Score: {}", state.score());
        spdlog::info("Max combo: {}", state.max_combo());
        spdlog::info("Perfect: {}, Great: {}, Good: {}, Bad: {}, Miss: {}",
                     state.judgment_count(JudgmentTier::PERFECT),
                     state.judgment_count(JudgmentTier::GREAT),
                     state.judgment_count(JudgmentTier::GOOD),
                     state.judgment_count(JudgmentTier::BAD),
                     state.judgment_count(JudgmentTier::MISS));

        return 0;

    } catch (const ChartLoadException& e) {
        spdlog::error("Failed to load chart: {}", e.what());
        return 1;
    } catch (const std::exception& e) {
        spdlog::error("Gameplay error: {}", e.what());
        return 1;
    }
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

void Engine::set_audio_system(std::unique_ptr<AudioSystem> audio) {
    audio_ = std::move(audio);
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
