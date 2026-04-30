#include <openitup/core/system_paths.h>

#include <cstdlib>
#include <spdlog/spdlog.h>

namespace openitup::core {

static bool is_valid_system_dir(const std::filesystem::path& path) {
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

std::optional<std::filesystem::path> find_system_dir(
    const std::filesystem::path& cli_override,
    const std::filesystem::path& binary_path) {

    // 1. CLI override
    if (!cli_override.empty() && is_valid_system_dir(cli_override)) {
        spdlog::debug("system directory from CLI: {}", cli_override.string());
        return std::filesystem::canonical(cli_override);
    }

    // 2. Environment variable
    if (const char* env_dir = std::getenv("OPENITUP_SYSTEM_DIR")) {
        std::filesystem::path env_path(env_dir);
        if (is_valid_system_dir(env_path)) {
            spdlog::debug("system directory from OPENITUP_SYSTEM_DIR: {}", env_path.string());
            return std::filesystem::canonical(env_path);
        }
    }

    // 3. Relative to CWD
    std::filesystem::path cwd_relative = "./data/system";
    if (is_valid_system_dir(cwd_relative)) {
        spdlog::debug("system directory from CWD: {}", cwd_relative.string());
        return std::filesystem::canonical(cwd_relative);
    }

    // 4. Relative to binary path
    if (!binary_path.empty()) {
        auto bin_dir = binary_path.parent_path();
        auto binary_relative = bin_dir / "../data/system";
        if (is_valid_system_dir(binary_relative)) {
            spdlog::debug("system directory from binary path: {}", binary_relative.string());
            return std::filesystem::canonical(binary_relative);
        }
    }

    // 5. Linux system install path
#ifdef __linux__
    std::filesystem::path system_install = "/usr/share/openitup/data/system";
    if (is_valid_system_dir(system_install)) {
        spdlog::debug("system directory from system install: {}", system_install.string());
        return std::filesystem::canonical(system_install);
    }
#endif

    spdlog::error("system directory not found (tried CLI, env, CWD, binary-relative, system)");
    return std::nullopt;
}

}  // namespace openitup::core
