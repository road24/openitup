#include <openitup/asset/noteskin_manager.h>

#include <algorithm>
#include <spdlog/spdlog.h>

#include <openitup/render/noteskin_loader.h>

namespace openitup {

NoteSkinManager::NoteSkinManager(std::filesystem::path noteskin_dir)
    : noteskin_dir_(std::move(noteskin_dir)) {}

int NoteSkinManager::scan() {
    available_skins_.clear();

    if (!std::filesystem::exists(noteskin_dir_)) {
        spdlog::warn("noteskin directory does not exist: {}", noteskin_dir_.string());
        return 0;
    }

    if (!std::filesystem::is_directory(noteskin_dir_)) {
        spdlog::warn("noteskin path is not a directory: {}", noteskin_dir_.string());
        return 0;
    }

    for (const auto& entry : std::filesystem::directory_iterator(noteskin_dir_)) {
        if (!entry.is_directory()) {
            continue;
        }

        std::string skin_name = entry.path().stem().string();
        available_skins_.push_back(skin_name);
    }

    std::sort(available_skins_.begin(), available_skins_.end());

    spdlog::info("Discovered {} noteskin(s) in {}", available_skins_.size(), noteskin_dir_.string());

    return static_cast<int>(available_skins_.size());
}

const std::vector<std::string>& NoteSkinManager::available_skins() const {
    return available_skins_;
}

std::unique_ptr<NoteSkin> NoteSkinManager::load_skin(
    const std::string& name,
    TextureCache& cache) const {

    if (!has_skin(name)) {
        spdlog::error("noteskin '{}' not found in {}", name, noteskin_dir_.string());
        return nullptr;
    }

    std::filesystem::path skin_path = noteskin_dir_ / name;

    try {
        return NoteSkinLoader::load(skin_path, cache);
    } catch (const std::exception& e) {
        spdlog::error("failed to load noteskin '{}': {}", name, e.what());
        return nullptr;
    }
}

bool NoteSkinManager::has_skin(const std::string& name) const {
    return std::find(available_skins_.begin(), available_skins_.end(), name)
        != available_skins_.end();
}

} // namespace openitup
