#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;
class Engine;

// ProfileSelectionScene: lists available player profiles, allows selection, creation, and deletion.
// Selected profile persists for the session.
// Displays profile stats (songs played, total score).
class ProfileSelectionScene : public Scene {
public:
    ProfileSelectionScene(Renderer* renderer,
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
    struct ProfileInfo {
        std::string name;
        int songs_played = 0;
        int total_score = 0;
        std::filesystem::path path;
    };

    enum class Mode {
        LIST,           // Normal profile selection
        DELETE_CONFIRM  // Confirmation dialog for deletion
    };

    void scan_profiles();
    void change_selection(int delta);
    void confirm_selection();
    void delete_profile();
    void create_profile();

    Renderer* renderer_;
    TextRenderer* text_;
    SceneStack* stack_;
    Engine* engine_;
    std::vector<ProfileInfo> profiles_;
    int selected_index_ = 0;
    Mode mode_ = Mode::LIST;
    bool show_create_new_ = true;
};

} // namespace openitup
