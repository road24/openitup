#pragma once

#include <memory>

#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;
class Scene;

// TransitionScene: wraps scene replacements with a timed fade animation.
// Renders outgoing scene with decreasing alpha, then incoming scene with increasing alpha.
// Automatically completes the transition and replaces itself with the target scene.
class TransitionScene : public Scene {
public:
    enum class Type {
        FADE,
        // Future: WIPE_LEFT, WIPE_RIGHT, etc.
    };

    TransitionScene(Renderer* renderer,
                    TextRenderer* text_renderer,
                    SceneStack* scene_stack,
                    std::unique_ptr<Scene> target_scene,
                    Type transition_type = Type::FADE,
                    double duration = 0.5);

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
    std::unique_ptr<Scene> target_scene_;
    Type type_;
    double duration_;
    double elapsed_ = 0.0;
};

} // namespace openitup
