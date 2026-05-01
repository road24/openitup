#include "openitup/asset/data_dir_config.h"

#include <fstream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace openitup {

DataDirConfig::DataDirConfig(std::filesystem::path default_dir) {
    entries_.push_back({std::filesystem::absolute(default_dir), true});
}

bool DataDirConfig::load_from_file(const std::filesystem::path& config_path) {
    // If config file doesn't exist, not an error - just use defaults
    if (!std::filesystem::exists(config_path)) {
        spdlog::debug("Config file not found: {}, using default directory only",
                      config_path.string());
        return false;
    }

    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            spdlog::warn("Failed to open config file: {}", config_path.string());
            return false;
        }

        nlohmann::json config = nlohmann::json::parse(file);

        // Expect array at key "data_dirs"
        if (!config.contains("data_dirs") || !config["data_dirs"].is_array()) {
            spdlog::warn("Config file {} missing 'data_dirs' array",
                         config_path.string());
            return false;
        }

        const auto& dirs = config["data_dirs"];
        int loaded_count = 0;
        int skipped_count = 0;

        for (const auto& entry : dirs) {
            // Each entry must have "path" and "enabled" fields
            if (!entry.contains("path") || !entry["path"].is_string()) {
                spdlog::warn("Skipping config entry: missing or invalid 'path' field");
                skipped_count++;
                continue;
            }

            std::string path_str = entry["path"];
            bool enabled = entry.value("enabled", true);

            std::filesystem::path abs_path = std::filesystem::absolute(path_str);

            // Check if directory exists (warn but don't fail)
            if (!std::filesystem::exists(abs_path)) {
                spdlog::warn("Directory not found: {} (from config)", abs_path.string());
                skipped_count++;
                continue;
            }

            if (!std::filesystem::is_directory(abs_path)) {
                spdlog::warn("Path is not a directory: {} (from config)", abs_path.string());
                skipped_count++;
                continue;
            }

            entries_.push_back({abs_path, enabled});
            loaded_count++;
        }

        spdlog::info("Loaded {} additional data directories from config (skipped {})",
                     loaded_count, skipped_count);
        return true;

    } catch (const nlohmann::json::parse_error& e) {
        spdlog::warn("Failed to parse config file {}: {}",
                     config_path.string(), e.what());
        return false;
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::warn("Filesystem error reading config {}: {}",
                     config_path.string(), e.what());
        return false;
    }
}

std::vector<std::filesystem::path> DataDirConfig::get_enabled_directories() const {
    std::vector<std::filesystem::path> result;
    for (const auto& entry : entries_) {
        if (entry.enabled) {
            result.push_back(entry.path);
        }
    }
    return result;
}

const std::vector<DataDirEntry>& DataDirConfig::get_all_entries() const {
    return entries_;
}

} // namespace openitup
