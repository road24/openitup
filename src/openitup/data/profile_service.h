#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <openitup/data/profile.h>

namespace openitup::data {

class SettingsManager;

// Service for managing user profiles.
// Handles loading, saving, creating, deleting profiles and recording high scores.
// Uses atomic writes for all file operations.
// Persists the active profile name in settings.json.
class ProfileService {
public:
    // Constructor requires SettingsManager to persist active profile.
    // profiles_dir is the directory where profile JSON files are stored.
    ProfileService(SettingsManager* settings_manager,
                   const std::filesystem::path& profiles_dir);

    // US-DAT-010: Load a profile by name from disk.
    // Returns ProfileData on success, nullopt on failure (logged as ERROR).
    std::optional<ProfileData> load_profile(const std::string& name);

    // US-DAT-011, US-DAT-012: Save a profile to disk using atomic write.
    // Returns true on success, false on failure (logged as ERROR).
    bool save_profile(const ProfileData& profile);

    // US-DAT-024: Create a new profile with the given name.
    // Returns false if a profile with that name already exists.
    // On success, creates the profile file and sets it as active.
    bool create_profile(const std::string& name);

    // US-DAT-025: Delete a profile by name.
    // Returns false if the profile doesn't exist or is the active profile.
    bool delete_profile(const std::string& name);

    // US-DAT-023: List all profile names (filenames without .json extension).
    std::vector<std::string> list_profiles() const;

    // US-DAT-026, US-DAT-027: Set the active profile.
    // Loads the profile from disk and persists the name in settings.
    // Returns false if the profile doesn't exist (logged as ERROR).
    bool set_active_profile(const std::string& name);

    // Get the currently active profile.
    // Returns nullptr if no profile is active.
    ProfileData* active_profile();
    const ProfileData* active_profile() const;

    // US-DAT-015: Record a high score for a chart.
    // Inserts the entry into the active profile's high_scores map.
    // Maintains up to 10 entries per chart, sorted by score descending.
    // Does NOT save the profile to disk — caller must call save_profile().
    // Returns false if no profile is active.
    bool record_high_score(const std::string& chart_hash,
                          const HighScoreEntry& entry);

    // US-DAT-017: Query top N high scores for a chart.
    // Returns up to `limit` entries sorted by score descending.
    // Returns empty vector if no profile is active or no scores for this chart.
    std::vector<HighScoreEntry> get_high_scores(const std::string& chart_hash,
                                                int limit = 10) const;

private:
    std::filesystem::path profile_path(const std::string& name) const;

    SettingsManager* settings_manager_;
    std::filesystem::path profiles_dir_;
    std::optional<ProfileData> active_profile_;
};

} // namespace openitup::data
