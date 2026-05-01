#include <openitup/scene/settings_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/core/engine.h>
#include <openitup/data/settings_manager.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>

namespace openitup {

SettingsScene::SettingsScene(Renderer* renderer,
                             TextRenderer* text_renderer,
                             SceneStack* scene_stack,
                             Engine* engine)
    : renderer_(renderer),
      text_(text_renderer),
      stack_(stack_),
      engine_(engine),
      settings_mgr_(engine->get_settings()) {}

void SettingsScene::on_enter() {
    spdlog::info("SettingsScene entered");
    current_tab_ = Tab::VIDEO;
    video_selection_ = 0;
    audio_selection_ = 0;
    input_selection_ = 0;
    gameplay_selection_ = 0;

    // Load current settings
    if (settings_mgr_) {
        current_settings_ = settings_mgr_->settings();
    }
}

void SettingsScene::on_exit() {
    spdlog::info("SettingsScene exited");
}

void SettingsScene::on_pause() {}
void SettingsScene::on_resume() {}

void SettingsScene::update(double /*dt*/) {
    // SettingsScene is input-driven
}

void SettingsScene::handle_input(const InputSnapshot& input) {
    // Tab navigation with shoulder buttons (if available) or left/right at top level
    if (input.is_pressed(PadInput::P1_UP_LEFT)) {
        change_tab(-1);
    }
    if (input.is_pressed(PadInput::P1_UP_RIGHT)) {
        change_tab(1);
    }

    // Option navigation
    if (input.is_pressed(PadInput::P1_UP_LEFT) || input.is_pressed(PadInput::P1_UP_LEFT)) {
        change_selection(-1);
    }
    if (input.is_pressed(PadInput::P1_DOWN_LEFT) || input.is_pressed(PadInput::P1_DOWN_LEFT)) {
        change_selection(1);
    }

    // Value adjustment (for current selection)
    if (input.is_pressed(PadInput::P1_UP_LEFT)) {
        adjust_value(-1);
    }
    if (input.is_pressed(PadInput::P1_UP_RIGHT)) {
        adjust_value(1);
    }

    // Apply/confirm
    if (input.is_pressed(PadInput::P1_CENTER)) {
        apply_current_setting();
    }

    // Back button returns to previous scene
    if (input.is_pressed(PadInput::BACK)) {
        spdlog::info("SettingsScene: back pressed, returning to previous scene");
        stack_->pop();
    }
}

void SettingsScene::change_tab(int delta) {
    int tab = static_cast<int>(current_tab_);
    tab += delta;
    if (tab < 0) tab = 3;
    if (tab > 3) tab = 0;
    current_tab_ = static_cast<Tab>(tab);
}

void SettingsScene::change_selection(int delta) {
    switch (current_tab_) {
        case Tab::VIDEO:
            video_selection_ += delta;
            if (video_selection_ < 0) video_selection_ = 0;
            if (video_selection_ > 0) video_selection_ = 0; // Only 1 option for now
            break;
        case Tab::AUDIO:
            audio_selection_ += delta;
            if (audio_selection_ < 0) audio_selection_ = 0;
            if (audio_selection_ > 2) audio_selection_ = 2; // 3 volume options
            break;
        case Tab::INPUT:
            input_selection_ += delta;
            if (input_selection_ < 0) input_selection_ = 0;
            if (input_selection_ > 0) input_selection_ = 0;
            break;
        case Tab::GAMEPLAY:
            gameplay_selection_ += delta;
            if (gameplay_selection_ < 0) gameplay_selection_ = 0;
            if (gameplay_selection_ > 0) gameplay_selection_ = 0;
            break;
    }
}

void SettingsScene::adjust_value(int delta) {
    bool changed = false;

    switch (current_tab_) {
        case Tab::VIDEO:
            // Adjust resolution (cycle through common resolutions)
            if (video_selection_ == static_cast<int>(VideoOption::RESOLUTION)) {
                if (delta > 0) {
                    if (current_settings_.video.width == 1280) {
                        current_settings_.video.width = 1920;
                        current_settings_.video.height = 1080;
                    } else if (current_settings_.video.width == 1920) {
                        current_settings_.video.width = 2560;
                        current_settings_.video.height = 1440;
                    }
                } else {
                    if (current_settings_.video.width == 2560) {
                        current_settings_.video.width = 1920;
                        current_settings_.video.height = 1080;
                    } else if (current_settings_.video.width == 1920) {
                        current_settings_.video.width = 1280;
                        current_settings_.video.height = 960;
                    }
                }
                changed = true;
            }
            break;

        case Tab::AUDIO:
            if (audio_selection_ == static_cast<int>(AudioOption::MASTER_VOLUME)) {
                current_settings_.audio.master_volume += delta * 0.1f;
                if (current_settings_.audio.master_volume < 0.0f) current_settings_.audio.master_volume = 0.0f;
                if (current_settings_.audio.master_volume > 1.0f) current_settings_.audio.master_volume = 1.0f;
                changed = true;
            } else if (audio_selection_ == static_cast<int>(AudioOption::MUSIC_VOLUME)) {
                current_settings_.audio.music_volume += delta * 0.1f;
                if (current_settings_.audio.music_volume < 0.0f) current_settings_.audio.music_volume = 0.0f;
                if (current_settings_.audio.music_volume > 1.0f) current_settings_.audio.music_volume = 1.0f;
                changed = true;
            } else if (audio_selection_ == static_cast<int>(AudioOption::SFX_VOLUME)) {
                current_settings_.audio.sfx_volume += delta * 0.1f;
                if (current_settings_.audio.sfx_volume < 0.0f) current_settings_.audio.sfx_volume = 0.0f;
                if (current_settings_.audio.sfx_volume > 1.0f) current_settings_.audio.sfx_volume = 1.0f;
                changed = true;
            }
            break;

        case Tab::INPUT:
        case Tab::GAMEPLAY:
            // TODO: Implement when we have more options
            break;
    }

    // Save immediately after change
    if (changed && settings_mgr_) {
        settings_mgr_->update(current_settings_);
        spdlog::info("SettingsScene: settings updated and saved");
    }
}

void SettingsScene::apply_current_setting() {
    // For most settings, they're applied immediately in adjust_value
    // This could be used for "reset to defaults" or similar actions
}

void SettingsScene::render() {
    if (!renderer_ || !text_) return;

    // Title
    text_->draw_text("Settings", 280, 40, SDL_Color{255, 255, 255, 255});

    // Tab headers
    SDL_Color video_color = (current_tab_ == Tab::VIDEO) ?
        SDL_Color{255, 255, 0, 255} : SDL_Color{150, 150, 150, 255};
    SDL_Color audio_color = (current_tab_ == Tab::AUDIO) ?
        SDL_Color{255, 255, 0, 255} : SDL_Color{150, 150, 150, 255};
    SDL_Color input_color = (current_tab_ == Tab::INPUT) ?
        SDL_Color{255, 255, 0, 255} : SDL_Color{150, 150, 150, 255};
    SDL_Color gameplay_color = (current_tab_ == Tab::GAMEPLAY) ?
        SDL_Color{255, 255, 0, 255} : SDL_Color{150, 150, 150, 255};

    text_->draw_text("Video", 120, 80, video_color);
    text_->draw_text("Audio", 240, 80, audio_color);
    text_->draw_text("Input", 360, 80, input_color);
    text_->draw_text("Gameplay", 480, 80, gameplay_color);

    // Render current tab content
    switch (current_tab_) {
        case Tab::VIDEO:
            render_video_tab();
            break;
        case Tab::AUDIO:
            render_audio_tab();
            break;
        case Tab::INPUT:
            render_input_tab();
            break;
        case Tab::GAMEPLAY:
            render_gameplay_tab();
            break;
    }

    // Instructions
    text_->draw_text("Left/Right: Change Tab  Up/Down: Navigate  Center: Apply  Back: Exit",
                     60, 440, SDL_Color{180, 180, 180, 255});
}

void SettingsScene::render_video_tab() {
    SDL_Color selected_color = {255, 255, 0, 255};
    SDL_Color normal_color = {200, 200, 200, 255};

    SDL_Color res_color = (video_selection_ == static_cast<int>(VideoOption::RESOLUTION)) ?
        selected_color : normal_color;

    text_->draw_text("Resolution:", 140, 140, res_color);
    std::string res_str = std::to_string(current_settings_.video.width) + "x" +
                          std::to_string(current_settings_.video.height);
    text_->draw_text(res_str, 340, 140, res_color);
}

void SettingsScene::render_audio_tab() {
    SDL_Color selected_color = {255, 255, 0, 255};
    SDL_Color normal_color = {200, 200, 200, 255};

    SDL_Color master_color = (audio_selection_ == static_cast<int>(AudioOption::MASTER_VOLUME)) ?
        selected_color : normal_color;
    SDL_Color music_color = (audio_selection_ == static_cast<int>(AudioOption::MUSIC_VOLUME)) ?
        selected_color : normal_color;
    SDL_Color sfx_color = (audio_selection_ == static_cast<int>(AudioOption::SFX_VOLUME)) ?
        selected_color : normal_color;

    text_->draw_text("Master Volume:", 140, 140, master_color);
    text_->draw_text(std::to_string(static_cast<int>(current_settings_.audio.master_volume * 100)) + "%",
                     380, 140, master_color);

    text_->draw_text("Music Volume:", 140, 180, music_color);
    text_->draw_text(std::to_string(static_cast<int>(current_settings_.audio.music_volume * 100)) + "%",
                     380, 180, music_color);

    text_->draw_text("SFX Volume:", 140, 220, sfx_color);
    text_->draw_text(std::to_string(static_cast<int>(current_settings_.audio.sfx_volume * 100)) + "%",
                     380, 220, sfx_color);
}

void SettingsScene::render_input_tab() {
    text_->draw_text("Input configuration", 180, 180, SDL_Color{200, 200, 200, 255});
    text_->draw_text("(Not yet implemented)", 180, 220, SDL_Color{150, 150, 150, 255});
}

void SettingsScene::render_gameplay_tab() {
    text_->draw_text("Gameplay settings", 180, 180, SDL_Color{200, 200, 200, 255});
    text_->draw_text("(Not yet implemented)", 180, 220, SDL_Color{150, 150, 150, 255});
}

} // namespace openitup
