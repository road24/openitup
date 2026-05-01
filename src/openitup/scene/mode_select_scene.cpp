#include <openitup/scene/mode_select_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/core/engine.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/minimal_gameplay_scene.h>
#include <openitup/scene/scene_stack.h>
#include <openitup/scene/title_scene.h>

namespace openitup {

ModeSelectScene::ModeSelectScene(Renderer* renderer, TextRenderer* text_renderer, SceneStack* scene_stack, Engine* engine, const std::filesystem::path& test_chart_path)
    : renderer_(renderer), text_(text_renderer), stack_(scene_stack), engine_(engine), test_chart_path_(test_chart_path) {}

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
            spdlog::info("ModeSelectScene: {} mode selected, launching gameplay", mode_name);

            // Phase 2: Use hardcoded test chart path (full song select in Phase 3)
            if (!test_chart_path_.empty() && std::filesystem::exists(test_chart_path_)) {
                std::filesystem::path data_dir = test_chart_path_.parent_path();
                try {
                    auto gameplay_scene = std::make_unique<MinimalGameplayScene>(
                        test_chart_path_,
                        data_dir,
                        engine_->get_audio(),
                        engine_->get_input_system(),
                        engine_->get_renderer(),
                        stack_,
                        engine_,
                        text_
                    );
                    stack_->replace(std::move(gameplay_scene));
                } catch (const std::exception& e) {
                    spdlog::error("Failed to launch gameplay: {}", e.what());
                }
            } else {
                spdlog::warn("No test chart configured, cannot launch gameplay");
            }
        } else {
            spdlog::warn("ModeSelectScene: Co-op/Battle not yet implemented");
        }
    }

    // BACK returns to title
    if (input.is_pressed(PadInput::BACK)) {
        spdlog::info("ModeSelectScene: BACK pressed, returning to TitleScene");
        stack_->replace(std::make_unique<TitleScene>(renderer_, text_, stack_, engine_, test_chart_path_));
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
