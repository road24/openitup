#include <openitup/scene/pause_overlay_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/audio/audio_system.h>
#include <openitup/core/engine.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>

namespace openitup {

PauseOverlayScene::PauseOverlayScene(Renderer* renderer,
                                     TextRenderer* text_renderer,
                                     SceneStack* scene_stack,
                                     Engine* engine)
    : renderer_(renderer),
      text_(text_renderer),
      stack_(stack_),
      engine_(engine) {}

void PauseOverlayScene::on_enter() {
    spdlog::info("PauseOverlayScene entered");
    selected_option_ = Option::RESUME;

    // Pause audio
    if (engine_->get_audio()) {
        engine_->get_audio()->pause();
    }
}

void PauseOverlayScene::on_exit() {
    spdlog::info("PauseOverlayScene exited");
}

void PauseOverlayScene::on_pause() {}
void PauseOverlayScene::on_resume() {}

void PauseOverlayScene::update(double /*dt*/) {
    // PauseOverlayScene is input-driven, no time-based updates
}

void PauseOverlayScene::handle_input(const InputSnapshot& input) {
    if (input.is_pressed(PadInput::P1_UP_LEFT) || input.is_pressed(PadInput::P1_UP_LEFT)) {
        change_selection(-1);
    }
    if (input.is_pressed(PadInput::P1_DOWN_LEFT) || input.is_pressed(PadInput::P1_DOWN_LEFT)) {
        change_selection(1);
    }

    // Center or Start confirms selection
    if (input.is_pressed(PadInput::P1_CENTER) || input.is_pressed(PadInput::START)) {
        confirm_selection();
    }

    // Back button resumes (same as selecting Resume)
    if (input.is_pressed(PadInput::BACK)) {
        selected_option_ = Option::RESUME;
        confirm_selection();
    }
}

void PauseOverlayScene::change_selection(int delta) {
    int current = static_cast<int>(selected_option_);
    current += delta;

    // Wrap around
    if (current < 0) current = 2;
    if (current > 2) current = 0;

    selected_option_ = static_cast<Option>(current);
}

void PauseOverlayScene::confirm_selection() {
    switch (selected_option_) {
        case Option::RESUME:
            spdlog::info("PauseOverlayScene: Resume selected");
            // Resume audio
            if (engine_->get_audio()) {
                engine_->get_audio()->resume();
            }
            // Pop self to return to gameplay
            stack_->pop();
            break;

        case Option::RESTART:
            spdlog::info("PauseOverlayScene: Restart selected");
            // TODO: Restart current song (pop pause, replace gameplay with new gameplay)
            // For now, just pop to return to gameplay
            stack_->pop();
            break;

        case Option::QUIT:
            spdlog::info("PauseOverlayScene: Quit selected");
            // TODO: Return to SongSelectScene (pop pause, pop gameplay, transition to song select)
            // For now, just pop to return to gameplay
            stack_->pop();
            break;
    }
}

void PauseOverlayScene::render() {
    if (!renderer_ || !text_) return;

    // Draw semi-transparent overlay (using text as indicator)
    // In a real implementation, this would render a semi-transparent quad
    text_->draw_text("==================", 180, 180, SDL_Color{50, 50, 50, 255});

    // Draw PAUSED title
    text_->draw_text("PAUSED", 280, 180, SDL_Color{255, 255, 255, 255});

    // Draw options
    SDL_Color resume_color = (selected_option_ == Option::RESUME) ?
        SDL_Color{255, 255, 0, 255} : SDL_Color{200, 200, 200, 255};
    SDL_Color restart_color = (selected_option_ == Option::RESTART) ?
        SDL_Color{255, 255, 0, 255} : SDL_Color{200, 200, 200, 255};
    SDL_Color quit_color = (selected_option_ == Option::QUIT) ?
        SDL_Color{255, 255, 0, 255} : SDL_Color{200, 200, 200, 255};

    text_->draw_text("> Resume", 260, 240, resume_color);
    text_->draw_text("> Restart", 260, 280, restart_color);
    text_->draw_text("> Quit to Select", 260, 320, quit_color);

    text_->draw_text("==================", 180, 360, SDL_Color{50, 50, 50, 255});

    // Draw instructions
    text_->draw_text("Up/Down: Navigate  Center/Start: Confirm  Back: Resume",
                     80, 440, SDL_Color{180, 180, 180, 255});
}

} // namespace openitup
