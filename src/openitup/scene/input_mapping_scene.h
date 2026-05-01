#pragma once

#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;
class InputSystem;
class Engine;

// US-INP-071: Input mapping configuration screen
class InputMappingScene : public Scene {
public:
    InputMappingScene(Renderer* renderer,
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
    Renderer* renderer_;
    TextRenderer* text_;
    SceneStack* stack_;
    Engine* engine_;

    int selected_panel_;  // Index into panel list (0-9)
    bool mapping_mode_;   // True when waiting for button press
    PadInput current_mapping_target_;  // Which panel we're mapping

    void render_panel_layout();
    void render_instructions();
    void enter_mapping_mode(PadInput target);
    void capture_button_press(uint32_t pressed_mask);
};

} // namespace openitup
