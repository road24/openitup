#include "openitup/asset/data_directory.h"

#include <algorithm>
#include <cstdlib>

#include <spdlog/spdlog.h>

namespace openitup {

DataDirectory::DataDirectory(std::filesystem::path path)
    : path_(std::filesystem::absolute(path)) {}

bool DataDirectory::validate() const {
    try {
        if (!std::filesystem::exists(path_)) {
            spdlog::error("Data directory does not exist: {}", path_.string());
            return false;
        }
        if (!std::filesystem::is_directory(path_)) {
            spdlog::error("Path is not a directory: {}", path_.string());
            return false;
        }
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Failed to validate directory '{}': {}", path_.string(), e.what());
        return false;
    }
}

const std::filesystem::path& DataDirectory::path() const {
    return path_;
}

std::optional<std::filesystem::path> DataDirectory::find_file_by_extension(
    const std::string& extension) const {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(path_)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            // Lowercase comparison
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            std::string ext_query = extension;
            std::transform(ext_query.begin(), ext_query.end(), ext_query.begin(), ::tolower);
            if (ext == ext_query) {
                return entry.path();
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Failed to scan directory '{}': {}", path_.string(), e.what());
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> DataDirectory::find_file_ci(
    const std::string& filename) const {
    try {
        std::string filename_lower = filename;
        std::transform(filename_lower.begin(), filename_lower.end(),
                       filename_lower.begin(), ::tolower);
        for (const auto& entry : std::filesystem::directory_iterator(path_)) {
            if (!entry.is_regular_file()) continue;
            std::string entry_name = entry.path().filename().string();
            std::transform(entry_name.begin(), entry_name.end(),
                           entry_name.begin(), ::tolower);
            if (entry_name == filename_lower) {
                return entry.path();
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Failed to scan directory '{}': {}", path_.string(), e.what());
    }
    return std::nullopt;
}

std::optional<DataDirectory> resolve_data_directory(const std::string& cli_path) {
    // Priority 1: CLI argument
    if (!cli_path.empty()) {
        return DataDirectory(cli_path);
    }

    // Priority 2: Environment variable
    const char* env = std::getenv("OPENITUP_DATA_DIR");
    if (env && env[0] != '\0') {
        spdlog::info("Using data directory from OPENITUP_DATA_DIR: {}", env);
        return DataDirectory(env);
    }

    // Priority 3: Error
    spdlog::error("No data directory specified. Use --data-dir <path> or set OPENITUP_DATA_DIR");
    return std::nullopt;
}

} // namespace openitup
