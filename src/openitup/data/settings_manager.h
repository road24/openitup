#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include <openitup/data/settings.h>

namespace openitup::data {

class SettingsManager {
public:
    // Injectable file I/O for testing.
    using FileReaderFn = std::function<std::string(const std::filesystem::path&)>;
    using FileWriterFn = std::function<bool(const std::filesystem::path&,
                                            const std::string&)>;

    // Construct with a path to settings.json.
    // Uses default file I/O (std::ifstream/atomic_write_file).
    explicit SettingsManager(std::filesystem::path settings_path);

    // Injectable constructor for testing.
    SettingsManager(std::filesystem::path settings_path,
                    FileReaderFn reader,
                    FileWriterFn writer);

    // Load settings from disk. If file missing, creates with defaults.
    // If file corrupt, logs ERROR and uses defaults.
    // Validates all loaded values (US-DAT-007).
    // Returns true if a file was successfully loaded (even with corrections).
    bool load();

    // Save current settings to disk using atomic write.
    // Returns true on success. On failure, logs ERROR and returns false.
    bool save() const;

    // Read-only access to current settings.
    const SettingsData& settings() const;

    // Modify a setting. Automatically saves to disk (US-DAT-005).
    // Returns true if save succeeded.
    bool update(const SettingsData& new_settings);

    // Modify video settings specifically (convenience).
    bool update_video(int width, int height);
    bool update_audio(const AudioSettings& audio);

private:
    // Validate settings and replace invalid values with defaults.
    // Returns true if all values were valid.
    bool validate();

    std::filesystem::path settings_path_;
    SettingsData data_;
    FileReaderFn reader_;
    FileWriterFn writer_;
};

} // namespace openitup::data
