#include <openitup/data/settings_manager.h>

#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <openitup/data/atomic_write.h>

namespace openitup::data {

namespace {

// Default file reader: reads entire file into string
std::string default_file_reader(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("File not found: " + path.string());
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Default file writer: uses atomic write
bool default_file_writer(const std::filesystem::path& path, const std::string& content) {
    return atomic_write_file(path, content);
}

} // anonymous namespace

SettingsManager::SettingsManager(std::filesystem::path settings_path)
    : SettingsManager(std::move(settings_path), default_file_reader, default_file_writer) {
}

SettingsManager::SettingsManager(std::filesystem::path settings_path,
                                 FileReaderFn reader,
                                 FileWriterFn writer)
    : settings_path_(std::move(settings_path)),
      data_(SettingsData::make_default()),
      reader_(std::move(reader)),
      writer_(std::move(writer)) {
}

bool SettingsManager::load() {
    try {
        // Try to read the file
        std::string content = reader_(settings_path_);

        // Parse JSON
        nlohmann::json j = nlohmann::json::parse(content);

        // Deserialize
        data_ = j.get<SettingsData>();

        // Validate and fix any invalid values
        validate();

        spdlog::info("Loaded settings from {}", settings_path_.string());
        return true;

    } catch (const std::runtime_error& e) {
        // File not found - create with defaults
        spdlog::info("Settings file not found, creating with defaults at {}", settings_path_.string());
        data_ = SettingsData::make_default();
        return save();

    } catch (const nlohmann::json::exception& e) {
        // JSON parse error
        spdlog::error("Failed to parse settings.json: {}", e.what());
        data_ = SettingsData::make_default();
        // Try to write valid defaults
        save();
        return false;

    } catch (const std::exception& e) {
        spdlog::error("Failed to load settings: {}", e.what());
        data_ = SettingsData::make_default();
        return false;
    }
}

bool SettingsManager::save() const {
    try {
        // Serialize to JSON
        nlohmann::json j = data_;
        std::string content = j.dump(4);  // Pretty-print with 4-space indent

        // Write atomically
        if (!writer_(settings_path_, content)) {
            spdlog::error("Failed to save settings to {}", settings_path_.string());
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        spdlog::error("Exception while saving settings: {}", e.what());
        return false;
    }
}

const SettingsData& SettingsManager::settings() const {
    return data_;
}

bool SettingsManager::update(const SettingsData& new_settings) {
    data_ = new_settings;
    validate();
    return save();
}

bool SettingsManager::update_video(int width, int height) {
    data_.video.width = width;
    data_.video.height = height;
    validate();
    return save();
}

bool SettingsManager::update_audio(const AudioSettings& audio) {
    data_.audio = audio;
    validate();
    return save();
}

bool SettingsManager::validate() {
    bool all_valid = true;

    // Validate resolution: minimum 640x480, maximum 7680x4320
    if (data_.video.width < 640 || data_.video.height < 480 ||
        data_.video.width > 7680 || data_.video.height > 4320) {
        spdlog::warn("Video resolution {}x{} out of range, using 1920x1080",
                     data_.video.width, data_.video.height);
        data_.video.width = 1920;
        data_.video.height = 1080;
        all_valid = false;
    }

    // Validate volume levels: [0.0, 1.0]
    if (data_.audio.master_volume < 0.0f || data_.audio.master_volume > 1.0f) {
        spdlog::warn("Master volume {} out of range, clamping to [0.0, 1.0]",
                     data_.audio.master_volume);
        data_.audio.master_volume = std::clamp(data_.audio.master_volume, 0.0f, 1.0f);
        all_valid = false;
    }

    if (data_.audio.music_volume < 0.0f || data_.audio.music_volume > 1.0f) {
        spdlog::warn("Music volume {} out of range, clamping to [0.0, 1.0]",
                     data_.audio.music_volume);
        data_.audio.music_volume = std::clamp(data_.audio.music_volume, 0.0f, 1.0f);
        all_valid = false;
    }

    if (data_.audio.sfx_volume < 0.0f || data_.audio.sfx_volume > 1.0f) {
        spdlog::warn("SFX volume {} out of range, clamping to [0.0, 1.0]",
                     data_.audio.sfx_volume);
        data_.audio.sfx_volume = std::clamp(data_.audio.sfx_volume, 0.0f, 1.0f);
        all_valid = false;
    }

    return all_valid;
}

} // namespace openitup::data
