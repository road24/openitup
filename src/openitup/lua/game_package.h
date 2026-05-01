#pragma once

#include <filesystem>
#include <string>
#include <memory>
#include <sol/sol.hpp>

namespace openitup {

// Metadata for a Lua game package loaded from data/games/<name>/manifest.lua
struct GamePackageManifest {
    std::string name;              // Display name (e.g., "Exceed")
    std::string version;           // Version string (e.g., "1.0")
    std::string judge_profile;     // Relative path to judge profile JSON
    std::string asset_dir;         // Relative path to assets directory (default: "assets/")
    std::string initial_scene;     // Initial screen to load (default: "boot")

    std::string author;            // Optional: package author
    std::string description;       // Optional: brief description
    int base_year = 0;             // Optional: original release year
};

// A Lua game package loaded from data/games/<name>/
class GamePackage {
public:
    // Load a game package from data/games/<name>/
    // Returns nullptr if the package is invalid or missing required files.
    // Logs errors and warnings via spdlog.
    static std::unique_ptr<GamePackage> load(const std::filesystem::path& games_dir, const std::string& name);

    ~GamePackage() = default;

    // Get package metadata
    const GamePackageManifest& manifest() const { return manifest_; }

    // Get absolute path to the game package directory
    const std::filesystem::path& directory() const { return directory_; }

    // Get absolute path to a screen script (e.g., "title" -> "<gamedir>/screens/title.lua")
    std::filesystem::path screen_path(const std::string& screen_name) const;

    // Get absolute path to an asset (e.g., "ui/button.sprj" -> "<gamedir>/assets/ui/button.sprj")
    std::filesystem::path asset_path(const std::string& relative_path) const;

    // Get absolute path to the judge profile JSON
    std::filesystem::path judge_profile_path() const;

    // Check if a screen script exists
    bool has_screen(const std::string& screen_name) const;

private:
    GamePackage() = default;

    // Load and parse manifest.lua
    bool load_manifest(const std::filesystem::path& manifest_path);

    // Validate that required files exist
    bool validate();

    GamePackageManifest manifest_;
    std::filesystem::path directory_;
    std::unique_ptr<sol::state> lua_;  // Lua state for loading manifest
};

} // namespace openitup
