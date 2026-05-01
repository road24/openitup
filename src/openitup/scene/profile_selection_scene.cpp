#include <openitup/scene/profile_selection_scene.h>

#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <openitup/core/engine.h>
#include <openitup/data/user_data_dir.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>
#include <openitup/scene/title_scene.h>

namespace openitup {

ProfileSelectionScene::ProfileSelectionScene(Renderer* renderer,
                                             TextRenderer* text_renderer,
                                             SceneStack* scene_stack,
                                             Engine* engine)
    : renderer_(renderer),
      text_(text_renderer),
      stack_(stack_),
      engine_(engine) {}

void ProfileSelectionScene::on_enter() {
    spdlog::info("ProfileSelectionScene entered");
    selected_index_ = 0;
    mode_ = Mode::LIST;
    scan_profiles();
}

void ProfileSelectionScene::on_exit() {
    spdlog::info("ProfileSelectionScene exited");
}

void ProfileSelectionScene::on_pause() {}
void ProfileSelectionScene::on_resume() {}

void ProfileSelectionScene::update(double /*dt*/) {
    // ProfileSelectionScene is input-driven
}

void ProfileSelectionScene::scan_profiles() {
    profiles_.clear();

    // Get profiles directory from user data directory
    const auto& user_data = engine_->get_user_data_dir();
    std::filesystem::path profiles_dir = user_data.path() / "profiles";

    if (!std::filesystem::exists(profiles_dir)) {
        spdlog::info("ProfileSelectionScene: profiles directory does not exist, creating");
        std::filesystem::create_directories(profiles_dir);
        return;
    }

    // Scan for profile JSON files
    for (const auto& entry : std::filesystem::directory_iterator(profiles_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            ProfileInfo info;
            info.path = entry.path();
            info.name = entry.path().stem().string();

            // Try to read stats from profile
            try {
                std::ifstream file(entry.path());
                if (file.is_open()) {
                    nlohmann::json j;
                    file >> j;
                    info.songs_played = j.value("songs_played", 0);
                    info.total_score = j.value("total_score", 0);
                }
            } catch (const std::exception& e) {
                spdlog::warn("ProfileSelectionScene: failed to read profile {}: {}", info.name, e.what());
            }

            profiles_.push_back(info);
            spdlog::debug("ProfileSelectionScene: found profile '{}'", info.name);
        }
    }

    spdlog::info("ProfileSelectionScene: found {} profiles", profiles_.size());
}

void ProfileSelectionScene::handle_input(const InputSnapshot& input) {
    if (mode_ == Mode::DELETE_CONFIRM) {
        // In delete confirmation mode
        if (input.is_pressed(PadInput::P1_CENTER) || input.is_pressed(PadInput::START)) {
            // Confirm deletion
            delete_profile();
            mode_ = Mode::LIST;
        }
        if (input.is_pressed(PadInput::BACK)) {
            // Cancel deletion
            spdlog::info("ProfileSelectionScene: delete cancelled");
            mode_ = Mode::LIST;
        }
        return;
    }

    // Normal list mode
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

    // Back button returns to previous scene (title or boot)
    if (input.is_pressed(PadInput::BACK)) {
        spdlog::info("ProfileSelectionScene: back pressed, returning to title");
        stack_->replace(std::make_unique<TitleScene>(
            renderer_, text_, stack_, engine_, std::filesystem::path(), nullptr));
    }

    // Delete button (using P1_DOWN_RIGHT for now)
    if (input.is_pressed(PadInput::P1_DOWN_RIGHT)) {
        if (selected_index_ < static_cast<int>(profiles_.size())) {
            mode_ = Mode::DELETE_CONFIRM;
        }
    }
}

void ProfileSelectionScene::change_selection(int delta) {
    selected_index_ += delta;

    int max_index = static_cast<int>(profiles_.size());
    if (show_create_new_) {
        max_index++; // Include "Create New Profile" option
    }

    if (selected_index_ < 0) selected_index_ = max_index - 1;
    if (selected_index_ >= max_index) selected_index_ = 0;
}

void ProfileSelectionScene::confirm_selection() {
    // If "Create New Profile" is selected
    if (show_create_new_ && selected_index_ == static_cast<int>(profiles_.size())) {
        create_profile();
        return;
    }

    // Otherwise, select an existing profile
    if (selected_index_ < static_cast<int>(profiles_.size())) {
        const auto& profile = profiles_[selected_index_];
        spdlog::info("ProfileSelectionScene: profile '{}' selected", profile.name);
        // TODO: Store selected profile in engine or global state
        // For now, just transition to title
        stack_->replace(std::make_unique<TitleScene>(
            renderer_, text_, stack_, engine_, std::filesystem::path(), nullptr));
    }
}

void ProfileSelectionScene::create_profile() {
    spdlog::info("ProfileSelectionScene: create new profile");
    // TODO: Push NameEntryScene or similar for profile name input
    // For now, create a default profile
    const auto& user_data = engine_->get_user_data_dir();
    std::filesystem::path profiles_dir = user_data.path() / "profiles";
    std::filesystem::create_directories(profiles_dir);

    // Generate default name
    int profile_num = static_cast<int>(profiles_.size()) + 1;
    std::string default_name = "PLAYER" + std::to_string(profile_num);
    std::filesystem::path profile_path = profiles_dir / (default_name + ".json");

    // Create empty profile
    nlohmann::json profile_data;
    profile_data["name"] = default_name;
    profile_data["songs_played"] = 0;
    profile_data["total_score"] = 0;

    try {
        std::ofstream file(profile_path);
        file << profile_data.dump(2);
        spdlog::info("ProfileSelectionScene: created profile '{}'", default_name);
        scan_profiles(); // Refresh list
    } catch (const std::exception& e) {
        spdlog::error("ProfileSelectionScene: failed to create profile: {}", e.what());
    }
}

void ProfileSelectionScene::delete_profile() {
    if (selected_index_ >= static_cast<int>(profiles_.size())) {
        return;
    }

    const auto& profile = profiles_[selected_index_];
    spdlog::info("ProfileSelectionScene: deleting profile '{}'", profile.name);

    try {
        std::filesystem::remove(profile.path);
        spdlog::info("ProfileSelectionScene: profile '{}' deleted", profile.name);
        scan_profiles(); // Refresh list
        if (selected_index_ >= static_cast<int>(profiles_.size())) {
            selected_index_ = static_cast<int>(profiles_.size()) - 1;
        }
        if (selected_index_ < 0) selected_index_ = 0;
    } catch (const std::exception& e) {
        spdlog::error("ProfileSelectionScene: failed to delete profile: {}", e.what());
    }
}

void ProfileSelectionScene::render() {
    if (!renderer_ || !text_) return;

    if (mode_ == Mode::DELETE_CONFIRM) {
        // Render delete confirmation dialog
        if (selected_index_ < static_cast<int>(profiles_.size())) {
            const auto& profile = profiles_[selected_index_];
            text_->draw_text("Delete Profile?", 220, 180, SDL_Color{255, 100, 100, 255});
            text_->draw_text("Profile: " + profile.name, 200, 220, SDL_Color{255, 255, 255, 255});
            text_->draw_text("This cannot be undone!", 180, 260, SDL_Color{255, 100, 100, 255});
            text_->draw_text("Center/Start: Confirm  Back: Cancel", 140, 320, SDL_Color{200, 200, 200, 255});
        }
        return;
    }

    // Normal list mode
    text_->draw_text("Profile Selection", 220, 40, SDL_Color{255, 255, 255, 255});

    // Render profile list
    int y_offset = 120;
    for (int i = 0; i < static_cast<int>(profiles_.size()); ++i) {
        const auto& profile = profiles_[i];
        SDL_Color color = (i == selected_index_) ?
            SDL_Color{255, 255, 0, 255} : SDL_Color{200, 200, 200, 255};

        text_->draw_text("> " + profile.name, 140, y_offset, color);
        text_->draw_text("Songs: " + std::to_string(profile.songs_played), 320, y_offset, color);
        text_->draw_text("Score: " + std::to_string(profile.total_score), 460, y_offset, color);

        y_offset += 40;
    }

    // Render "Create New Profile" option
    if (show_create_new_) {
        SDL_Color color = (selected_index_ == static_cast<int>(profiles_.size())) ?
            SDL_Color{255, 255, 0, 255} : SDL_Color{150, 150, 255, 255};
        text_->draw_text("> Create New Profile", 140, y_offset, color);
    }

    // Instructions
    text_->draw_text("Up/Down: Navigate  Center/Start: Select  Back: Exit",
                     80, 440, SDL_Color{180, 180, 180, 255});
}

} // namespace openitup
