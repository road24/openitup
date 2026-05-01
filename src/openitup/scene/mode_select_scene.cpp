#include <openitup/scene/mode_select_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>

namespace openitup {

ModeSelectScene::ModeSelectScene(Renderer* renderer, TextRenderer* text_renderer, SceneStack* scene_stack)
    : renderer_(renderer), text_(text_renderer), stack_(scene_stack) {}

void ModeSelectScene::on_enter() {
    spdlog::info("ModeSelectScene entered");
    cursor_ = 0;
}

void ModeSelectScene::on_exit() {
    spdlog::info("ModeSelectScene exited");
}

void ModeSelectScene::on_pause() {}
void ModeSelectScene::on_resume() {}

void ModeSelectScene::update(double /*dt*/) {
    // No time-based logic in this scene
}

void ModeSelectScene::handle_input(const InputSnapshot& input) {
    // Left/Right navigation
    if (input.is_pressed(PadInput::P1_UP_RIGHT) || input.is_pressed(PadInput::P1_DOWN_RIGHT)) {
        cursor_ = (cursor_ + 1) % NUM_MODES;
        spdlog::debug("ModeSelectScene: cursor moved to {}", cursor_);
    }
    if (input.is_pressed(PadInput::P1_UP_LEFT) || input.is_pressed(PadInput::P1_DOWN_LEFT)) {
        cursor_ = (cursor_ - 1 + NUM_MODES) % NUM_MODES;
        spdlog::debug("ModeSelectScene: cursor moved to {}", cursor_);
    }

    // START confirms selection
    if (input.is_pressed(PadInput::START)) {
        if (cursor_ == 0 || cursor_ == 1) {
            const char* mode_name = (cursor_ == 0) ? "Single" : "Double";
            spdlog::info("ModeSelectScene: {} mode selected, transitioning to SongSelectScene (stub)", mode_name);
            // Transition will be wired in Phase 3 when SongSelectScene exists
        } else {
            spdlog::warn("ModeSelectScene: Co-op/Battle not yet implemented");
        }
    }

    // BACK returns to title
    if (input.is_pressed(PadInput::BACK)) {
        spdlog::info("ModeSelectScene: BACK pressed, returning to TitleScene (stub)");
        // Transition will be wired when TitleScene can push ModeSelectScene
    }
}

void ModeSelectScene::render() {
    if (!renderer_ || !text_) return;

    const char* mode_names[] = {"Single", "Double", "Co-op", "Battle"};
    int y_start = 150;
    int y_spacing = 40;

    for (int i = 0; i < NUM_MODES; ++i) {
        int y = y_start + i * y_spacing;

        // Cursor indicator
        if (i == cursor_) {
            text_->draw_text(">", 250, y, SDL_Color{255, 255, 0, 255});
        }

        // Mode name (dimmed if disabled)
        SDL_Color color = (i < 2) ? SDL_Color{255, 255, 255, 255} : SDL_Color{100, 100, 100, 255};
        text_->draw_text(mode_names[i], 280, y, color);
    }
}

} // namespace openitup
