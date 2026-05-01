#include <openitup/scene/input_mapping_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/core/engine.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>

namespace openitup {

InputMappingScene::InputMappingScene(Renderer* renderer,
                                     TextRenderer* text_renderer,
                                     SceneStack* scene_stack,
                                     Engine* engine)
    : renderer_(renderer),
      text_(text_renderer),
      stack_(scene_stack),
      engine_(engine),
      selected_panel_(0),
      mapping_mode_(false),
      current_mapping_target_(PadInput::P1_DOWN_LEFT) {}

void InputMappingScene::on_enter() {
    spdlog::info("InputMappingScene entered");
    selected_panel_ = 0;
    mapping_mode_ = false;
}

void InputMappingScene::on_exit() {
    spdlog::info("InputMappingScene exited");
}

void InputMappingScene::on_pause() {}
void InputMappingScene::on_resume() {}

void InputMappingScene::update(double /*dt*/) {
    // Input-driven scene
}

void InputMappingScene::handle_input(const InputSnapshot& input) {
    if (mapping_mode_) {
        // US-INP-071 Scenario 3: Capture button press in mapping mode
        uint32_t pressed = input.pressed_mask();
        if (pressed != 0) {
            capture_button_press(pressed);
            mapping_mode_ = false;
        }
        return;
    }

    // Navigate between panels
    if (input.is_pressed(PadInput::P1_UP_LEFT) || input.is_pressed(PadInput::P1_UP_RIGHT)) {
        selected_panel_--;
        if (selected_panel_ < 0) selected_panel_ = 9;
    }
    if (input.is_pressed(PadInput::P1_DOWN_LEFT) || input.is_pressed(PadInput::P1_DOWN_RIGHT)) {
        selected_panel_++;
        if (selected_panel_ > 9) selected_panel_ = 0;
    }

    // US-INP-071 Scenario 2: Enter mapping mode
    if (input.is_pressed(PadInput::P1_CENTER)) {
        enter_mapping_mode(ALL_PAD_INPUTS[selected_panel_]);
    }

    // Back to previous scene
    if (input.is_pressed(PadInput::BACK)) {
        spdlog::info("InputMappingScene: back pressed, returning to previous scene");
        stack_->pop();
    }
}

void InputMappingScene::render() {
    if (!renderer_ || !text_) return;

    // Title
    text_->draw_text("Input Mapping Configuration", 200, 40, SDL_Color{255, 255, 255, 255});

    render_panel_layout();
    render_instructions();
}

void InputMappingScene::render_panel_layout() {
    // US-INP-071 Scenario 1: Visual layout of all 10 panels in PIU X-pattern
    // Simplified text-based layout for now
    const char* panel_names[] = {
        "P1 Down-Left", "P1 Up-Left", "P1 Center", "P1 Up-Right", "P1 Down-Right",
        "P2 Down-Left", "P2 Up-Left", "P2 Center", "P2 Up-Right", "P2 Down-Right"
    };

    int y = 100;
    for (int i = 0; i < 10; ++i) {
        SDL_Color color = (i == selected_panel_) ?
            SDL_Color{255, 255, 0, 255} : SDL_Color{200, 200, 200, 255};

        std::string label = std::string(panel_names[i]);
        if (i == selected_panel_ && mapping_mode_) {
            label += " <-- Press button now!";
            color = SDL_Color{255, 0, 0, 255};
        }

        text_->draw_text(label, 160, y, color);
        y += 30;
    }
}

void InputMappingScene::render_instructions() {
    if (mapping_mode_) {
        text_->draw_text("Press the button you want to map to the selected panel",
                        80, 420, SDL_Color{255, 255, 0, 255});
    } else {
        text_->draw_text("Up/Down: Select Panel  Center: Map  Back: Exit",
                        120, 440, SDL_Color{180, 180, 180, 255});
    }
}

void InputMappingScene::enter_mapping_mode(PadInput target) {
    spdlog::info("InputMappingScene: entering mapping mode for panel {}", static_cast<int>(target));
    mapping_mode_ = true;
    current_mapping_target_ = target;
}

void InputMappingScene::capture_button_press(uint32_t pressed_mask) {
    spdlog::info("InputMappingScene: captured button press: 0x{:x}", pressed_mask);
    // US-INP-071 Scenario 3: Assign button to panel and save to settings
    // TODO: Update InputSettings and save via SettingsManager
}

} // namespace openitup
