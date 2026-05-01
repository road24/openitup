#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <openitup/asset/song_database.h>
#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;
class Engine;
class CachedSongDatabase;

enum class GameMode {
    SINGLE,
    DOUBLE,
    COOP,    // Phase 3+
    BATTLE   // Phase 3+
};

// ModeSelectScene: displays "Single / Double / Co-op / Battle" with cursor navigation.
// Phase 2: Single and Double selectable. Co-op/Battle disabled (grayed).
// Phase 3: Transitions to SongSelectScene instead of directly to gameplay.
class ModeSelectScene : public Scene {
public:
    ModeSelectScene(
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
    int cursor_ = 0;  // 0=Single, 1=Double, 2=Co-op, 3=Battle
    static constexpr int NUM_MODES = 4;
};

} // namespace openitup
