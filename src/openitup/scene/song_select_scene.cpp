#include <openitup/scene/song_select_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/core/engine.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/minimal_gameplay_scene.h>
#include <openitup/scene/mode_select_scene.h>
#include <openitup/scene/scene_stack.h>

namespace openitup {

SongSelectScene::SongSelectScene(
    Renderer* renderer,
    TextRenderer* text_renderer,
    SceneStack* scene_stack,
    Engine* engine,
    GameMode selected_mode,
    std::vector<SongDatabaseEntry> songs)
    : renderer_(renderer),
      text_(text_renderer),
      stack_(scene_stack),
      engine_(engine),
      selected_mode_(selected_mode),
      songs_(std::move(songs)),
      cursor_(0),
      difficulty_cursor_(0) {}

void SongSelectScene::on_enter() {
    spdlog::info("SongSelectScene entered with {} songs", songs_.size());
    cursor_ = 0;
    difficulty_cursor_ = 0;
}

void SongSelectScene::on_exit() {
    spdlog::info("SongSelectScene exited");
}

void SongSelectScene::on_pause() {}
void SongSelectScene::on_resume() {}

void SongSelectScene::update(double /*dt*/) {
    // No time-based logic in this scene
}

void SongSelectScene::handle_input(const InputSnapshot& input) {
    if (songs_.empty()) {
        // No songs available, only allow back
        if (input.is_pressed(PadInput::BACK)) {
            spdlog::info("SongSelectScene: BACK pressed, returning to ModeSelectScene");
            // Need to pass test_chart_path but we don't have it here
            // For now, pass empty path
            stack_->replace(std::make_unique<ModeSelectScene>(
                renderer_, text_, stack_, engine_, std::filesystem::path{}));
        }
        return;
    }

    // Up/Down navigation (vertical scrolling through songs)
    if (input.is_pressed(PadInput::P1_DOWN_LEFT) || input.is_pressed(PadInput::P1_DOWN_RIGHT)) {
        cursor_ = (cursor_ + 1) % static_cast<int>(songs_.size());
        difficulty_cursor_ = 0;  // Reset difficulty cursor when changing songs
        spdlog::debug("SongSelectScene: cursor moved to song {}", cursor_);
    }
    if (input.is_pressed(PadInput::P1_UP_LEFT) || input.is_pressed(PadInput::P1_UP_RIGHT)) {
        cursor_ = (cursor_ - 1 + static_cast<int>(songs_.size())) % static_cast<int>(songs_.size());
        difficulty_cursor_ = 0;  // Reset difficulty cursor when changing songs
        spdlog::debug("SongSelectScene: cursor moved to song {}", cursor_);
    }

    // Left/Right navigation (horizontal scrolling through difficulties)
    const auto* selected_song = get_selected_song();
    if (selected_song && !selected_song->chart_paths.empty()) {
        int chart_count = get_chart_count();

        if (input.is_pressed(PadInput::P1_UP_RIGHT) && chart_count > 1) {
            difficulty_cursor_ = (difficulty_cursor_ + 1) % chart_count;
            spdlog::debug("SongSelectScene: difficulty cursor moved to {}", difficulty_cursor_);
        }
        if (input.is_pressed(PadInput::P1_UP_LEFT) && chart_count > 1) {
            difficulty_cursor_ = (difficulty_cursor_ - 1 + chart_count) % chart_count;
            spdlog::debug("SongSelectScene: difficulty cursor moved to {}", difficulty_cursor_);
        }
    }

    // START confirms selection
    if (input.is_pressed(PadInput::START)) {
        if (!selected_song || selected_song->chart_paths.empty()) {
            spdlog::warn("SongSelectScene: no valid chart selected");
            return;
        }

        if (difficulty_cursor_ >= static_cast<int>(selected_song->chart_paths.size())) {
            spdlog::warn("SongSelectScene: difficulty cursor out of range");
            return;
        }

        const auto& chart_path = selected_song->chart_paths[difficulty_cursor_];
        const auto& data_dir = selected_song->song_path;

        spdlog::info("SongSelectScene: song '{}' selected, launching gameplay", selected_song->title);

        try {
            auto gameplay_scene = std::make_unique<MinimalGameplayScene>(
                chart_path,
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
            spdlog::error("SongSelectScene: failed to launch gameplay: {}", e.what());
        }
    }

    // BACK returns to mode select
    if (input.is_pressed(PadInput::BACK)) {
        spdlog::info("SongSelectScene: BACK pressed, returning to ModeSelectScene");
        // Need to pass test_chart_path but we don't have it here
        stack_->replace(std::make_unique<ModeSelectScene>(
            renderer_, text_, stack_, engine_, std::filesystem::path{}));
    }
}

void SongSelectScene::render() {
    if (!renderer_ || !text_) return;

    // Title
    text_->draw_text("Song Select", 20, 20, SDL_Color{255, 255, 255, 255});

    if (songs_.empty()) {
        text_->draw_text("No songs found", 200, 200, SDL_Color{255, 100, 100, 255});
        text_->draw_text("Press BACK to return", 200, 240, SDL_Color{200, 200, 200, 255});
        return;
    }

    // Display song list (show 5 songs at a time)
    const int visible_songs = 5;
    const int y_start = 80;
    const int y_spacing = 40;

    int first_visible = std::max(0, cursor_ - visible_songs / 2);
    int last_visible = std::min(static_cast<int>(songs_.size()), first_visible + visible_songs);

    // Adjust if we're near the end
    if (last_visible - first_visible < visible_songs && last_visible == static_cast<int>(songs_.size())) {
        first_visible = std::max(0, last_visible - visible_songs);
    }

    for (int i = first_visible; i < last_visible; ++i) {
        int y = y_start + (i - first_visible) * y_spacing;

        // Cursor indicator
        if (i == cursor_) {
            text_->draw_text(">", 40, y, SDL_Color{255, 255, 0, 255});
        }

        // Song title
        SDL_Color color = (i == cursor_) ? SDL_Color{255, 255, 255, 255} : SDL_Color{150, 150, 150, 255};
        text_->draw_text(songs_[i].title, 70, y, color);
    }

    // Display info for selected song
    const auto* selected_song = get_selected_song();
    if (selected_song) {
        int info_y = 300;
        text_->draw_text("Title: " + selected_song->title, 40, info_y, SDL_Color{255, 255, 255, 255});
        info_y += 30;

        if (!selected_song->artist.empty()) {
            text_->draw_text("Artist: " + selected_song->artist, 40, info_y, SDL_Color{200, 200, 200, 255});
            info_y += 30;
        }

        if (selected_song->bpm > 0.0) {
            text_->draw_text("BPM: " + std::to_string(static_cast<int>(selected_song->bpm)),
                           40, info_y, SDL_Color{200, 200, 200, 255});
            info_y += 30;
        }

        // Display available difficulties
        if (!selected_song->chart_paths.empty()) {
            std::string diff_str = "Difficulty: ";
            if (selected_song->chart_paths.size() > 1) {
                diff_str += std::to_string(difficulty_cursor_ + 1) + "/" +
                           std::to_string(selected_song->chart_paths.size());
            } else {
                diff_str += "1/1";
            }
            text_->draw_text(diff_str, 40, info_y, SDL_Color{200, 200, 200, 255});
            info_y += 30;
        }

        // Show chart filename
        if (difficulty_cursor_ < static_cast<int>(selected_song->chart_paths.size())) {
            const auto& chart_path = selected_song->chart_paths[difficulty_cursor_];
            text_->draw_text("Chart: " + chart_path.filename().string(),
                           40, info_y, SDL_Color{150, 150, 150, 255});
        }
    }

    // Instructions
    text_->draw_text("UP/DOWN: Select song  LEFT/RIGHT: Select difficulty  START: Play  BACK: Return",
                    20, 440, SDL_Color{128, 128, 128, 255});
}

const SongDatabaseEntry* SongSelectScene::get_selected_song() const {
    if (cursor_ < 0 || cursor_ >= static_cast<int>(songs_.size())) {
        return nullptr;
    }
    return &songs_[cursor_];
}

int SongSelectScene::get_chart_count() const {
    const auto* song = get_selected_song();
    if (!song) {
        return 0;
    }
    return static_cast<int>(song->chart_paths.size());
}

} // namespace openitup
