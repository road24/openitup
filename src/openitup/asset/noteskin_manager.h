#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <openitup/gfx/texture_cache.h>
#include <openitup/render/noteskin.h>

namespace openitup {

// Manages discovery and loading of installed noteskins.
// Scans a noteskin directory (e.g., "noteskin/") for subdirectories,
// each representing an installed skin.
class NoteSkinManager {
public:
    // Construct with the base noteskin directory path.
    // Does not scan immediately - call scan() to discover skins.
    explicit NoteSkinManager(std::filesystem::path noteskin_dir);

    // Scan the noteskin directory and populate the list of available skins.
    // Returns the number of valid skins found.
    // Invalid skin directories are logged but do not cause failure.
    int scan();

    // Get list of discovered skin names (directory stems).
    const std::vector<std::string>& available_skins() const;

    // Load a skin by name. Returns nullptr if the skin does not exist.
    // cache: TextureCache for loading sprite textures.
    std::unique_ptr<NoteSkin> load_skin(
        const std::string& name,
        TextureCache& cache) const;

    // Check if a skin with the given name exists.
    bool has_skin(const std::string& name) const;

private:
    std::filesystem::path noteskin_dir_;
    std::vector<std::string> available_skins_;
};

} // namespace openitup
