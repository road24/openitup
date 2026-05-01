#pragma once

#include <filesystem>
#include <openitup/judge/gameplay_state.h>
#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;
class Engine;

// ResultScene: displays final score, grade, combo, and judgment breakdown after gameplay.
// Automatically transitions to TitleScene after 5 seconds or on any input.
class ResultScene : public Scene {
public:
    ResultScene(Renderer* renderer,
                TextRenderer* text_renderer,
                SceneStack* scene_stack,
                Engine* engine,
                const GameplayState& gameplay_state);

    void on_enter() override;
    void on_exit() override;
    void on_pause() override;
    void on_resume() override;
    void update(double dt) override;
    void handle_input(const InputSnapshot& input) override;
    void render() override;

private:
    std::string calculate_grade() const;

    Renderer* renderer_;
    TextRenderer* text_;
    SceneStack* stack_;
    Engine* engine_;
    GameplayState gameplay_state_;
    double elapsed_ = 0.0;
    static constexpr double AUTO_TRANSITION_TIME = 5.0;
};

} // namespace openitup
