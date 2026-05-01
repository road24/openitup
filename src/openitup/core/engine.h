#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <openitup/audio/audio_system.h>
#include <openitup/core/clock.h>
#include <openitup/core/system_asset_manager.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_system.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>

namespace openitup {
    namespace core {
        class SystemAssetManager;
    }

struct EngineConfig {
    std::string window_title = "openitup";
    int window_width = 1280;
    int window_height = 960;
    double target_fps = 0.0;
    std::string data_dir_path;
    std::string chart_path;
};

class Engine {
public:
    explicit Engine(const EngineConfig& config = {});
    Engine(const EngineConfig& config, std::unique_ptr<Clock> clock);
    Engine(const EngineConfig& config, std::unique_ptr<Clock> clock, std::unique_ptr<AudioSystem> audio);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    int run();
    int run_gameplay(const std::filesystem::path& chart_path,
                     const std::filesystem::path& data_dir);
    void request_quit();

    Renderer* get_renderer() const { return renderer_.get(); }
    Clock* get_clock() const { return clock_.get(); }
    InputSystem* get_input_system() const { return input_system_.get(); }
    AudioSystem* get_audio() const { return audio_.get(); }
    SceneStack* get_scene_stack() const { return scene_stack_.get(); }
    TextRenderer* get_text_renderer() const { return text_renderer_.get(); }
    core::SystemAssetManager* get_system_assets() const { return system_asset_mgr_.get(); }
    std::filesystem::path get_data_dir() const { return config_.data_dir_path; }
    uint64_t tick_count() const { return tick_count_; }
    double render_alpha() const { return render_alpha_; }
    bool is_running() const { return running_; }

    void set_input_system(std::unique_ptr<InputSystem> input);
    void set_audio_system(std::unique_ptr<AudioSystem> audio);

private:
    void init_sdl();
    void init_renderer(const EngineConfig& config);
    void init_audio();
    void process_events();
    void update(double dt);
    void render(double alpha);

    EngineConfig config_;
    std::unique_ptr<Clock> clock_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<InputSystem> input_system_;
    std::unique_ptr<AudioSystem> audio_;
    std::unique_ptr<SceneStack> scene_stack_;
    std::unique_ptr<TextRenderer> text_renderer_;
    std::unique_ptr<core::SystemAssetManager> system_asset_mgr_;

    bool running_ = false;
    uint64_t tick_count_ = 0;
    double accumulator_ = 0.0;
    double render_alpha_ = 0.0;
    double target_frame_time_ = 0.0;
};

} // namespace openitup
