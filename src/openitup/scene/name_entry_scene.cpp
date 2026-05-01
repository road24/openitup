#include <openitup/scene/name_entry_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/core/engine.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>

namespace openitup {

NameEntryScene::NameEntryScene(Renderer* renderer,
                               TextRenderer* text_renderer,
                               SceneStack* scene_stack,
                               Engine* engine,
                               int rank,
                               int score)
    : renderer_(renderer),
      text_(text_renderer),
      stack_(stack_),
      engine_(engine),
      rank_(rank),
      score_(score) {}

void NameEntryScene::on_enter() {
    spdlog::info("NameEntryScene entered - Rank: {}, Score: {}", rank_, score_);
    name_.clear();
    cursor_x_ = 0;
    cursor_y_ = 0;
    initialize_character_grid();
}

void NameEntryScene::on_exit() {
    spdlog::info("NameEntryScene exited - Final name: {}", name_);
}

void NameEntryScene::on_pause() {}
void NameEntryScene::on_resume() {}

void NameEntryScene::update(double /*dt*/) {
    // NameEntryScene is input-driven, no time-based updates
}

void NameEntryScene::initialize_character_grid() {
    // Create character grid: A-Z, 0-9, space, backspace indicator
    char_grid_.clear();
    char_grid_.resize(GRID_HEIGHT);

    // Row 0: A-J
    for (int i = 0; i < GRID_WIDTH; ++i) {
        char_grid_[0].push_back('A' + i);
    }

    // Row 1: K-T
    for (int i = 0; i < GRID_WIDTH; ++i) {
        char_grid_[1].push_back('K' + i);
    }

    // Row 2: U-Z, 0-3
    for (int i = 0; i < 6; ++i) {
        char_grid_[2].push_back('U' + i);
    }
    for (int i = 0; i < 4; ++i) {
        char_grid_[2].push_back('0' + i);
    }

    // Row 3: 4-9, space, special
    for (int i = 4; i < 10; ++i) {
        char_grid_[3].push_back('0' + i);
    }
}

void NameEntryScene::handle_input(const InputSnapshot& input) {
    if (input.is_pressed(PadInput::P1_UP_LEFT) || input.is_pressed(PadInput::P1_UP_LEFT)) {
        move_cursor(0, -1);
    }
    if (input.is_pressed(PadInput::P1_DOWN_LEFT) || input.is_pressed(PadInput::P1_DOWN_LEFT)) {
        move_cursor(0, 1);
    }
    if (input.is_pressed(PadInput::P1_UP_LEFT) || input.is_pressed(PadInput::P1_UP_LEFT)) {
        move_cursor(-1, 0);
    }
    if (input.is_pressed(PadInput::P1_UP_RIGHT) || input.is_pressed(PadInput::P1_UP_RIGHT)) {
        move_cursor(1, 0);
    }

    // Center button confirms character selection
    if (input.is_pressed(PadInput::P1_CENTER)) {
        select_character();
    }

    // Back button removes last character
    if (input.is_pressed(PadInput::BACK)) {
        backspace();
    }

    // Start button confirms name
    if (input.is_pressed(PadInput::START)) {
        confirm_name();
    }
}

void NameEntryScene::move_cursor(int dx, int dy) {
    cursor_x_ += dx;
    cursor_y_ += dy;

    // Wrap around
    if (cursor_x_ < 0) cursor_x_ = GRID_WIDTH - 1;
    if (cursor_x_ >= GRID_WIDTH) cursor_x_ = 0;
    if (cursor_y_ < 0) cursor_y_ = GRID_HEIGHT - 1;
    if (cursor_y_ >= GRID_HEIGHT) cursor_y_ = 0;

    // Ensure we have a valid character at this position
    if (cursor_y_ >= static_cast<int>(char_grid_.size()) ||
        cursor_x_ >= static_cast<int>(char_grid_[cursor_y_].size())) {
        cursor_x_ = 0;
    }
}

void NameEntryScene::select_character() {
    if (cursor_y_ >= static_cast<int>(char_grid_.size()) ||
        cursor_x_ >= static_cast<int>(char_grid_[cursor_y_].size())) {
        return;
    }

    if (static_cast<int>(name_.length()) >= MAX_NAME_LENGTH) {
        spdlog::warn("NameEntryScene: name at maximum length");
        return;
    }

    char selected = char_grid_[cursor_y_][cursor_x_];
    name_ += selected;
    spdlog::debug("NameEntryScene: selected '{}', name now: '{}'", selected, name_);
}

void NameEntryScene::backspace() {
    if (!name_.empty()) {
        name_.pop_back();
        spdlog::debug("NameEntryScene: backspace, name now: '{}'", name_);
    }
}

void NameEntryScene::confirm_name() {
    if (static_cast<int>(name_.length()) < MIN_NAME_LENGTH) {
        spdlog::warn("NameEntryScene: name too short (min {} chars)", MIN_NAME_LENGTH);
        return;
    }

    spdlog::info("NameEntryScene: name confirmed: '{}'", name_);
    // TODO: Save name to profile with rank and score
    // For now, just pop the scene
    stack_->pop();
}

void NameEntryScene::render() {
    if (!renderer_ || !text_) return;

    // Display rank and score at top
    text_->draw_text("New High Score!", 220, 40, SDL_Color{255, 255, 0, 255});
    text_->draw_text("Rank: " + std::to_string(rank_), 240, 80, SDL_Color{255, 255, 255, 255});
    text_->draw_text("Score: " + std::to_string(score_), 240, 110, SDL_Color{255, 255, 255, 255});

    // Display current name
    text_->draw_text("Name: " + name_, 240, 150, SDL_Color{0, 255, 255, 255});

    // Display length constraint feedback
    if (static_cast<int>(name_.length()) >= MAX_NAME_LENGTH) {
        text_->draw_text("Maximum length reached", 200, 180, SDL_Color{255, 100, 100, 255});
    } else if (static_cast<int>(name_.length()) < MIN_NAME_LENGTH) {
        text_->draw_text("Minimum 3 characters", 200, 180, SDL_Color{200, 200, 200, 255});
    }

    // Draw character grid
    int grid_start_x = 160;
    int grid_start_y = 220;
    int cell_width = 32;
    int cell_height = 32;

    for (int y = 0; y < static_cast<int>(char_grid_.size()); ++y) {
        for (int x = 0; x < static_cast<int>(char_grid_[y].size()); ++x) {
            int draw_x = grid_start_x + x * cell_width;
            int draw_y = grid_start_y + y * cell_height;

            SDL_Color color = {200, 200, 200, 255};
            if (x == cursor_x_ && y == cursor_y_) {
                color = {255, 255, 0, 255}; // Highlight cursor position
            }

            std::string ch(1, char_grid_[y][x]);
            text_->draw_text(ch, draw_x, draw_y, color);
        }
    }

    // Draw instructions
    text_->draw_text("Arrows: Navigate  Center: Select  Back: Delete  Start: Confirm",
                     80, 440, SDL_Color{180, 180, 180, 255});
}

} // namespace openitup
