#include <openitup/scene/title_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/core/engine.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/boot_scene.h>
#include <openitup/scene/mode_select_scene.h>
#include <openitup/scene/scene_stack.h>

namespace openitup {

TitleScene::TitleScene(Renderer* renderer, TextRenderer* text_renderer, SceneStack* scene_stack, Engine* engine, const std::filesystem::path& test_chart_path)
    : renderer_(renderer), text_(text_renderer), stack_(scene_stack), engine_(engine), test_chart_path_(test_chart_path) {}

void TitleScene::on_enter() {
    spdlog::info("TitleScene entered");
    elapsed_ = 0.0;
}

void TitleScene::on_exit() {
    spdlog::info("TitleScene exited");
}

void TitleScene::on_pause() {}
void TitleScene::on_resume() {}

void TitleScene::update(double dt) {
    elapsed_ += dt;
    if (elapsed_ >= TIMEOUT) {
        spdlog::info("TitleScene: inactivity timeout, returning to BootScene");
        stack_->replace(std::make_unique<BootScene>(renderer_, text_, stack_, engine_, test_chart_path_));
        elapsed_ = 0.0;  // Reset to prevent log spam
    }
}

void TitleScene::handle_input(const InputSnapshot& input) {
    // Any input resets inactivity timer
    if (input.pressed_mask() != 0) {
        elapsed_ = 0.0;
    }

    // START transitions to ModeSelectScene
    if (input.is_pressed(PadInput::START) || input.is_pressed(PadInput::COIN)) {
        spdlog::info("TitleScene: START pressed, transitioning to ModeSelectScene");
        stack_->replace(std::make_unique<ModeSelectScene>(renderer_, text_, stack_, engine_, test_chart_path_));
    }
}

void TitleScene::render() {
    if (!renderer_ || !text_) return;

    // Draw title text centered
    text_->draw_text("TITLE SCREEN", 320, 200, SDL_Color{255, 255, 0, 255});
    text_->draw_text("Press START", 320, 260, SDL_Color{255, 255, 255, 255});
}

} // namespace openitup
