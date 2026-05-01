#pragma once

#include <string>
#include <vector>

#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;
class Engine;

// NameEntryScene: character-by-character name entry for high scores.
// Displays a character grid (A-Z, 0-9, space). Player navigates with arrows and selects with confirm.
// Minimum 3 characters, maximum 10 characters.
// Saves name to profile on confirmation.
class NameEntryScene : public Scene {
public:
    NameEntryScene(Renderer* renderer,
                   TextRenderer* text_renderer,
                   SceneStack* scene_stack,
                   Engine* engine,
                   int rank,
                   int score);

    void on_enter() override;
    void on_exit() override;
    void on_pause() override;
    void on_resume() override;
    void update(double dt) override;
    void handle_input(const InputSnapshot& input) override;
    void render() override;

private:
    void initialize_character_grid();
    void move_cursor(int dx, int dy);
    void select_character();
    void backspace();
    void confirm_name();

    Renderer* renderer_;
    TextRenderer* text_;
    SceneStack* stack_;
    Engine* engine_;
    int rank_;
    int score_;
    std::string name_;
    int cursor_x_ = 0;
    int cursor_y_ = 0;
    std::vector<std::vector<char>> char_grid_;

    static constexpr int MIN_NAME_LENGTH = 3;
    static constexpr int MAX_NAME_LENGTH = 10;
    static constexpr int GRID_WIDTH = 10;
    static constexpr int GRID_HEIGHT = 4;
};

} // namespace openitup
