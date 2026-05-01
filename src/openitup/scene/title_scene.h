#pragma once

#include <filesystem>
#include <memory>

#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;
class Engine;
class CachedSongDatabase;

// TitleScene: displays "TITLE SCREEN - Press START" with 30s inactivity timeout.
// Phase 2: text-only. Phase 3+: add BGA attract loop, audio.
class TitleScene : public Scene {
public:
    TitleScene(
        Renderer* renderer,
        TextRenderer* text_renderer,
        SceneStack* scene_stack,
        Engine* engine,
        const std::filesystem::path& test_chart_path,
        std::shared_ptr<CachedSongDatabase> song_database = nullptr);

    void on_enter() override;
    void on_exit() override;
    void on_pause() override;
    void on_resume() override;
    void update(double dt) override;
    void handle_input(const InputSnapshot& input) override;
    void render() override;

private:
    Renderer* renderer_;
    TextRenderer* text_;
    SceneStack* stack_;
    Engine* engine_;
    std::filesystem::path test_chart_path_;
    std::shared_ptr<CachedSongDatabase> song_database_;
    double elapsed_ = 0.0;
    static constexpr double TIMEOUT = 30.0;
};

} // namespace openitup
