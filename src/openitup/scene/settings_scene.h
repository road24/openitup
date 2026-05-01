#pragma once

#include <openitup/data/settings.h>
#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;
class Engine;

namespace data {
    class SettingsManager;
}

// SettingsScene: configuration screen for video, audio, input, and gameplay settings.
// Organized into tabs/categories. Changes are saved immediately.
// Accessible from title screen or song select.
class SettingsScene : public Scene {
public:
    SettingsScene(Renderer* renderer,
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
    enum class Tab {
        VIDEO,
        AUDIO,
        INPUT,
        GAMEPLAY
    };

    enum class VideoOption {
        RESOLUTION,
        // Future: FULLSCREEN, VSYNC
    };

    enum class AudioOption {
        MASTER_VOLUME,
        MUSIC_VOLUME,
        SFX_VOLUME,
        // Future: GLOBAL_OFFSET
    };

    void change_tab(int delta);
    void change_selection(int delta);
    void adjust_value(int delta);
    void apply_current_setting();

    void render_video_tab();
    void render_audio_tab();
    void render_input_tab();
    void render_gameplay_tab();

    Renderer* renderer_;
    TextRenderer* text_;
    SceneStack* stack_;
    Engine* engine_;
    data::SettingsManager* settings_mgr_;
    data::SettingsData current_settings_;

    Tab current_tab_ = Tab::VIDEO;
    int video_selection_ = 0;
    int audio_selection_ = 0;
    int input_selection_ = 0;
    int gameplay_selection_ = 0;
};

} // namespace openitup
