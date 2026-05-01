#pragma once

#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;

// BootScene: displays splash text for 3 seconds, then transitions to TitleScene.
// Phase 2: text-only splash. Phase 3+: add BGA logo animation, asset scanning.
class BootScene : public Scene {
public:
    BootScene(Renderer* renderer, TextRenderer* text_renderer, SceneStack* scene_stack);

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
    double elapsed_ = 0.0;
    static constexpr double DURATION = 3.0;
};

} // namespace openitup
