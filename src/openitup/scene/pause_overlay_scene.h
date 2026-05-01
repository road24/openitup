#pragma once

#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;
class Engine;

// PauseOverlayScene: modal pause menu pushed over GameplayScene.
// Displays semi-transparent overlay with options: Resume, Restart, Quit.
// Pauses audio on entry, resumes on resume.
class PauseOverlayScene : public Scene {
public:
    PauseOverlayScene(Renderer* renderer,
                      TextRenderer* text_renderer,
                      SceneStack* scene_stack,
                      Engine* engine);

    void on_enter() override;
    void on_exit() override;
    void on_pause() override;
    void on_resume() override;
    void update(double dt) override;
    void handle_input(const InputSnapshot& input) override;
    void render() override;

private:
    enum class Option {
        RESUME,
        RESTART,
        QUIT
    };

    void change_selection(int delta);
    void confirm_selection();

    Renderer* renderer_;
    TextRenderer* text_;
    SceneStack* stack_;
    Engine* engine_;
    Option selected_option_ = Option::RESUME;
};

} // namespace openitup
