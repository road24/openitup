#pragma once

#include <filesystem>

namespace openitup::data {

class UserDataDir {
public:
    // Resolve the platform-appropriate user data directory.
    // Linux: $XDG_DATA_HOME/openitup/ (defaults to ~/.local/share/openitup/)
    // Windows: %APPDATA%/openitup/
    // The path is resolved but NOT created at construction.
    UserDataDir();

    // Injectable constructor for testing: use a custom base path.
    explicit UserDataDir(std::filesystem::path base_path);

    // Ensure the directory tree exists. Creates:
    //   <base>/
    //   <base>/profiles/
    //   <base>/cache/
    // Returns true if all directories exist or were created.
    // On failure: logs ERROR, returns false.
    bool ensure_directories() const;

    // Resolved absolute path to the user data root.
    const std::filesystem::path& path() const;

    // Convenience accessors for subdirectories.
    std::filesystem::path settings_file() const;
    std::filesystem::path profiles_dir() const;
    std::filesystem::path cache_dir() const;

private:
    std::filesystem::path path_;
};

} // namespace openitup::data
