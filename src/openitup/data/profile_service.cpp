#include <openitup/data/profile_service.h>

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <openitup/data/atomic_write.h>
#include <openitup/data/settings_manager.h>

namespace openitup::data {

ProfileService::ProfileService(SettingsManager* settings_manager,
                               const std::filesystem::path& profiles_dir)
    : settings_manager_(settings_manager),
      profiles_dir_(profiles_dir) {
}

std::optional<ProfileData> ProfileService::load_profile(const std::string& name) {
    auto path = profile_path(name);

    if (!std::filesystem::exists(path)) {
        spdlog::error("Profile file does not exist: {}", path.string());
        return std::nullopt;
    }

    try {
        std::ifstream in(path);
        if (!in) {
            spdlog::error("Failed to open profile file: {}", path.string());
            return std::nullopt;
        }

        nlohmann::json j;
        in >> j;

        ProfileData profile = j.get<ProfileData>();
        return profile;

    } catch (const std::exception& e) {
        spdlog::error("Failed to parse profile {}: {}", path.string(), e.what());
        return std::nullopt;
    }
}

bool ProfileService::save_profile(const ProfileData& profile) {
    auto path = profile_path(profile.display_name);

    try {
        nlohmann::json j = profile;
        std::string content = j.dump(2);  // Pretty print with 2-space indent

        if (!atomic_write_file(path, content)) {
            spdlog::error("Failed to save profile: {}", path.string());
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        spdlog::error("Failed to serialize profile {}: {}", profile.display_name, e.what());
        return false;
    }
}

bool ProfileService::create_profile(const std::string& name) {
    auto path = profile_path(name);

    // Check if profile already exists
    if (std::filesystem::exists(path)) {
        spdlog::warn("Profile already exists: {}", name);
        return false;
    }

    // Create default profile
    ProfileData profile = create_default_profile(name);

    // Save to disk
    if (!save_profile(profile)) {
        spdlog::error("Failed to create profile: {}", name);
        return false;
    }

    // Set as active profile
    active_profile_ = profile;

    // Persist active profile name in settings
    auto settings = settings_manager_->load_settings();
    if (settings.has_value()) {
        settings_manager_->set_last_active_profile(name);
        settings_manager_->save_settings(settings.value());
    }

    spdlog::info("Created profile: {}", name);
    return true;
}

bool ProfileService::delete_profile(const std::string& name) {
    auto path = profile_path(name);

    // Check if profile exists
    if (!std::filesystem::exists(path)) {
        spdlog::warn("Profile does not exist: {}", name);
        return false;
    }

    // Don't delete the active profile
    if (active_profile_.has_value() && active_profile_->display_name == name) {
        spdlog::error("Cannot delete active profile: {}", name);
        return false;
    }

    // Delete the file
    try {
        std::filesystem::remove(path);
        spdlog::info("Deleted profile: {}", name);
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Failed to delete profile {}: {}", name, e.what());
        return false;
    }
}

std::vector<std::string> ProfileService::list_profiles() const {
    std::vector<std::string> profiles;

    try {
        if (!std::filesystem::exists(profiles_dir_)) {
            return profiles;
        }

        for (const auto& entry : std::filesystem::directory_iterator(profiles_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                profiles.push_back(entry.path().stem().string());
            }
        }

    } catch (const std::exception& e) {
        spdlog::error("Failed to list profiles in {}: {}", profiles_dir_.string(), e.what());
    }

    return profiles;
}

bool ProfileService::set_active_profile(const std::string& name) {
    auto profile = load_profile(name);
    if (!profile.has_value()) {
        spdlog::error("Failed to load profile: {}", name);
        return false;
    }

    active_profile_ = profile;

    // Persist active profile name in settings
    auto settings = settings_manager_->load_settings();
    if (settings.has_value()) {
        settings_manager_->set_last_active_profile(name);
        settings_manager_->save_settings(settings.value());
    }

    spdlog::info("Set active profile: {}", name);
    return true;
}

ProfileData* ProfileService::active_profile() {
    if (active_profile_.has_value()) {
        return &active_profile_.value();
    }
    return nullptr;
}

const ProfileData* ProfileService::active_profile() const {
    if (active_profile_.has_value()) {
        return &active_profile_.value();
    }
    return nullptr;
}

bool ProfileService::record_high_score(const std::string& chart_hash,
                                      const HighScoreEntry& entry) {
    if (!active_profile_.has_value()) {
        spdlog::error("Cannot record high score: no active profile");
        return false;
    }

    // Get existing scores for this chart
    auto& scores = active_profile_->high_scores[chart_hash];

    // Add new entry
    scores.push_back(entry);

    // Sort by score descending
    std::sort(scores.begin(), scores.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
        return a.score > b.score;
    });

    // Keep only top 10
    if (scores.size() > 10) {
        scores.resize(10);
    }

    return true;
}

std::vector<HighScoreEntry> ProfileService::get_high_scores(const std::string& chart_hash,
                                                           int limit) const {
    if (!active_profile_.has_value()) {
        return {};
    }

    auto it = active_profile_->high_scores.find(chart_hash);
    if (it == active_profile_->high_scores.end()) {
        return {};
    }

    const auto& scores = it->second;
    int count = std::min(limit, static_cast<int>(scores.size()));

    return std::vector<HighScoreEntry>(scores.begin(), scores.begin() + count);
}

std::filesystem::path ProfileService::profile_path(const std::string& name) const {
    return profiles_dir_ / (name + ".json");
}

} // namespace openitup::data
