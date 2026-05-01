#include <openitup/data/atomic_write.h>

#include <fstream>
#include <filesystem>

#include <spdlog/spdlog.h>

namespace openitup::data {

bool atomic_write_file(const std::filesystem::path& path,
                       const std::string& content) {
    // Write to a temporary file in the same directory
    auto temp_path = path;
    temp_path += ".tmp";

    try {
        // Write to temp file
        std::ofstream out(temp_path, std::ios::binary);
        if (!out) {
            spdlog::error("Failed to open temp file {} for writing", temp_path.string());
            return false;
        }

        out.write(content.data(), content.size());
        out.close();

        if (!out.good()) {
            spdlog::error("Failed to write to temp file {}: I/O error", temp_path.string());
            std::filesystem::remove(temp_path);
            return false;
        }

        // Atomic rename
        std::error_code ec;
        std::filesystem::rename(temp_path, path, ec);
        if (ec) {
            spdlog::error("Failed to rename {} to {}: {}",
                         temp_path.string(), path.string(), ec.message());
            std::filesystem::remove(temp_path);
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        spdlog::error("Exception during atomic write to {}: {}", path.string(), e.what());
        // Clean up temp file if it exists
        std::error_code ec;
        std::filesystem::remove(temp_path, ec);
        return false;
    }
}

} // namespace openitup::data
