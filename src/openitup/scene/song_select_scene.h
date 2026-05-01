#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <openitup/asset/song_database.h>
#include <openitup/scene/mode_select_scene.h>
#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;
class Engine;

// SongSelectScene: displays a text-based list of available songs.
// Phase 3: text-based music wheel (sprite-based wheel in Phase 5).
// US-SCN-006: song select scene with navigation, info display, and transitions.
class SongSelectScene : public Scene {
public:
    SongSelectScene(
        Renderer* renderer,
        TextRenderer* text_renderer,
        SceneStack* scene_stack,
        Engine* engine,
        GameMode selected_mode,
        std::vector<SongDatabaseEntry> songs);

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
    GameMode selected_mode_;
    std::vector<SongDatabaseEntry> songs_;
    int cursor_ = 0;          // Currently selected song index
    int difficulty_cursor_ = 0;  // Currently selected difficulty (chart) index

    // Get the currently selected song.
    const SongDatabaseEntry* get_selected_song() const;

    // Get the number of available charts for the selected song.
    int get_chart_count() const;
};

} // namespace openitup
