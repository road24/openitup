#pragma once

#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;

enum class GameMode {
    SINGLE,
    DOUBLE,
    COOP,    // Phase 3+
    BATTLE   // Phase 3+
};

// ModeSelectScene: displays "Single / Double / Co-op / Battle" with cursor navigation.
// Phase 2: Single and Double selectable. Co-op/Battle disabled (grayed).
class ModeSelectScene : public Scene {
public:
    ModeSelectScene(Renderer* renderer, TextRenderer* text_renderer, SceneStack* scene_stack);

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
    int cursor_ = 0;  // 0=Single, 1=Double, 2=Co-op, 3=Battle
    static constexpr int NUM_MODES = 4;
};

} // namespace openitup
