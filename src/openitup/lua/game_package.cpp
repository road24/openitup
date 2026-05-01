#include "game_package.h"

#include <spdlog/spdlog.h>

namespace openitup {

std::unique_ptr<GamePackage> GamePackage::load(const std::filesystem::path& games_dir, const std::string& name) {
    auto package = std::unique_ptr<GamePackage>(new GamePackage());
    package->directory_ = games_dir / name;

    // Check if directory exists
    if (!std::filesystem::is_directory(package->directory_)) {
        spdlog::error("Game directory does not exist: {}", package->directory_.string());
        return nullptr;
    }

    // Load manifest.lua
    std::filesystem::path manifest_path = package->directory_ / "manifest.lua";
    if (!std::filesystem::exists(manifest_path)) {
        spdlog::error("Game directory '{}' missing manifest.lua", name);
        return nullptr;
    }

    if (!package->load_manifest(manifest_path)) {
        return nullptr;
    }

    // Validate required files
    if (!package->validate()) {
        return nullptr;
    }

    spdlog::info("Loaded game package: {} v{}", package->manifest_.name, package->manifest_.version);
    return package;
}

bool GamePackage::load_manifest(const std::filesystem::path& manifest_path) {
    lua_ = std::make_unique<sol::state>();
    lua_->open_libraries(sol::lib::base, sol::lib::string, sol::lib::table);

    // Load and execute manifest.lua
    auto result = lua_->script_file(manifest_path.string(), sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("Failed to load manifest.lua: {}", err.what());
        return false;
    }

    // Extract game table
    sol::optional<sol::table> game_table = (*lua_)["game"];
    if (!game_table) {
        spdlog::error("manifest.lua does not define 'game' table");
        return false;
    }

    sol::table game = *game_table;

    // Required fields
    sol::optional<std::string> name = game["name"];
    sol::optional<std::string> version = game["version"];
    sol::optional<std::string> judge_profile = game["judge_profile"];

    if (!name || !version || !judge_profile) {
        spdlog::error("manifest.lua missing required fields (name, version, judge_profile)");
        return false;
    }

    manifest_.name = *name;
    manifest_.version = *version;
    manifest_.judge_profile = *judge_profile;

    // Optional fields
    manifest_.asset_dir = game.get_or<std::string>("asset_dir", "assets/");
    manifest_.initial_scene = game.get_or<std::string>("initial_scene", "boot");
    manifest_.author = game.get_or<std::string>("author", "");
    manifest_.description = game.get_or<std::string>("description", "");
    sol::optional<int> base_year = game["base_year"];
    manifest_.base_year = base_year.value_or(0);

    return true;
}

bool GamePackage::validate() {
    // Check judge profile exists
    std::filesystem::path judge_path = judge_profile_path();
    if (!std::filesystem::exists(judge_path)) {
        spdlog::error("Judge profile not found: {}", judge_path.string());
        return false;
    }

    // Check initial scene exists
    if (!manifest_.initial_scene.empty() && !has_screen(manifest_.initial_scene)) {
        spdlog::warn("Initial scene '{}' not found in {}/screens/",
                     manifest_.initial_scene, manifest_.name);
        // This is a warning, not an error - engine can fall back
    }

    // Check screens directory exists
    std::filesystem::path screens_dir = directory_ / "screens";
    if (!std::filesystem::is_directory(screens_dir)) {
        spdlog::warn("Game package '{}' has no screens/ directory", manifest_.name);
    }

    return true;
}

std::filesystem::path GamePackage::screen_path(const std::string& screen_name) const {
    return directory_ / "screens" / (screen_name + ".lua");
}

std::filesystem::path GamePackage::asset_path(const std::string& relative_path) const {
    return directory_ / relative_path;
}

std::filesystem::path GamePackage::judge_profile_path() const {
    return directory_ / manifest_.judge_profile;
}

bool GamePackage::has_screen(const std::string& screen_name) const {
    return std::filesystem::exists(screen_path(screen_name));
}

} // namespace openitup
