#include <openitup/data/user_data_dir.h>

#include <cstdlib>
#include <filesystem>

#include <spdlog/spdlog.h>

namespace openitup::data {

namespace {

std::filesystem::path resolve_platform_path() {
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        return std::filesystem::path(appdata) / "openitup";
    }
    // Fallback if APPDATA is not set
    spdlog::warn("APPDATA environment variable not set, using current directory fallback");
    return std::filesystem::current_path() / "openitup";
#else
    // Linux/Unix path resolution
    const char* xdg_data_home = std::getenv("XDG_DATA_HOME");
    if (xdg_data_home && xdg_data_home[0] != '\0') {
        return std::filesystem::path(xdg_data_home) / "openitup";
    }

    // Fallback to ~/.local/share/openitup
    const char* home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) / ".local" / "share" / "openitup";
    }

    // Last resort fallback
    spdlog::warn("HOME environment variable not set, using current directory fallback");
    return std::filesystem::current_path() / "openitup";
#endif
}

} // anonymous namespace

UserDataDir::UserDataDir()
    : path_(resolve_platform_path()) {
}

UserDataDir::UserDataDir(std::filesystem::path base_path)
    : path_(std::move(base_path)) {
}

bool UserDataDir::ensure_directories() const {
    try {
        // Create base directory
        if (!std::filesystem::exists(path_)) {
            std::filesystem::create_directories(path_);
            spdlog::info("Created user data directory: {}", path_.string());
        }

        // Create profiles subdirectory
        auto profiles = profiles_dir();
        if (!std::filesystem::exists(profiles)) {
            std::filesystem::create_directories(profiles);
            spdlog::debug("Created profiles directory: {}", profiles.string());
        }

        // Create cache subdirectory
        auto cache = cache_dir();
        if (!std::filesystem::exists(cache)) {
            std::filesystem::create_directories(cache);
            spdlog::debug("Created cache directory: {}", cache.string());
        }

        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Failed to create user data directory {}: {}", path_.string(), e.what());
        return false;
    }
}

const std::filesystem::path& UserDataDir::path() const {
    return path_;
}

std::filesystem::path UserDataDir::settings_file() const {
    return path_ / "settings.json";
}

std::filesystem::path UserDataDir::profiles_dir() const {
    return path_ / "profiles";
}

std::filesystem::path UserDataDir::cache_dir() const {
    return path_ / "cache";
}

} // namespace openitup::data
