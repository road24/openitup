#include "song_cache.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "atomic_write.h"

using json = nlohmann::json;

namespace openitup::data {

namespace {

// Default file reader using std::ifstream
std::string default_file_reader(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Default file writer using atomic_write_file
bool default_file_writer(const std::filesystem::path& path,
                        const std::string& content) {
    return atomic_write_file(path, content);
}

// Convert system time to ISO 8601 string
std::string time_to_iso8601(const std::filesystem::file_time_type& time) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        time - std::filesystem::file_time_type::clock::now() +
        std::chrono::system_clock::now());
    auto time_t = std::chrono::system_clock::to_time_t(sctp);
    std::tm tm;
    gmtime_r(&time_t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// Get current time as ISO 8601 string
std::string current_time_iso8601() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    gmtime_r(&time_t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// Parse ISO 8601 string to system time
std::chrono::system_clock::time_point iso8601_to_time(const std::string& iso) {
    std::tm tm = {};
    std::istringstream ss(iso);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (ss.fail()) {
        return std::chrono::system_clock::time_point::min();
    }
    return std::chrono::system_clock::from_time_t(timegm(&tm));
}

} // anonymous namespace

void to_json(json& j, const DifficultyEntry& d) {
    j = json{{"type", d.type}, {"level", d.level}};
}

void from_json(const json& j, DifficultyEntry& d) {
    j.at("type").get_to(d.type);
    j.at("level").get_to(d.level);
}

void to_json(json& j, const ChartCacheEntry& c) {
    j = json{
        {"path", c.path.string()},
        {"hash", c.hash},
        {"title", c.title},
        {"artist", c.artist},
        {"difficulties", c.difficulties}
    };
}

void from_json(const json& j, ChartCacheEntry& c) {
    c.path = j.at("path").get<std::string>();
    c.hash = j.at("hash").get<std::string>();
    c.title = j.at("title").get<std::string>();
    c.artist = j.at("artist").get<std::string>();
    c.difficulties = j.at("difficulties").get<std::vector<DifficultyEntry>>();
}

SongCache::SongCache(std::filesystem::path cache_path)
    : cache_path_(std::move(cache_path))
    , reader_(default_file_reader)
    , writer_(default_file_writer) {
}

SongCache::SongCache(std::filesystem::path cache_path,
                     FileReaderFn reader,
                     FileWriterFn writer)
    : cache_path_(std::move(cache_path))
    , reader_(std::move(reader))
    , writer_(std::move(writer)) {
}

bool SongCache::load() {
    auto start_time = std::chrono::steady_clock::now();

    try {
        std::string content = reader_(cache_path_);
        json j = json::parse(content);

        schema_version_ = j.at("schema_version").get<int>();
        last_updated_ = j.at("last_updated").get<std::string>();
        entries_ = j.at("charts").get<std::vector<ChartCacheEntry>>();

        auto end_time = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();

        spdlog::debug("Loaded chart cache with {} entries in {}ms",
                     entries_.size(), elapsed_ms);

        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to load chart cache from {}: {}",
                     cache_path_.string(), e.what());
        entries_.clear();
        last_updated_.clear();
        return false;
    }
}

bool SongCache::save() const {
    json j;
    j["schema_version"] = schema_version_;
    j["last_updated"] = current_time_iso8601();
    j["charts"] = entries_;

    std::string content = j.dump(2);  // Pretty-print with 2-space indent

    if (writer_(cache_path_, content)) {
        spdlog::debug("Saved chart cache with {} entries to {}",
                     entries_.size(), cache_path_.string());
        return true;
    } else {
        spdlog::error("Failed to save chart cache to {}", cache_path_.string());
        return false;
    }
}

bool SongCache::is_fresh(const std::vector<std::filesystem::path>& song_dirs) const {
    if (last_updated_.empty()) {
        return false;
    }

    auto cache_time = iso8601_to_time(last_updated_);
    if (cache_time == std::chrono::system_clock::time_point::min()) {
        spdlog::warn("Invalid cache timestamp: {}", last_updated_);
        return false;
    }

    for (const auto& dir : song_dirs) {
        if (!std::filesystem::exists(dir)) {
            spdlog::info("Song directory {} no longer exists, cache invalidated",
                        dir.string());
            return false;
        }

        try {
            auto dir_mtime = std::filesystem::last_write_time(dir);
            auto dir_time = time_to_iso8601(dir_mtime);
            auto dir_sys_time = iso8601_to_time(dir_time);

            if (dir_sys_time > cache_time) {
                spdlog::info("Chart cache invalidated due to directory modification, rebuilding");
                return false;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            spdlog::warn("Failed to check mtime for {}: {}", dir.string(), e.what());
            return false;
        }
    }

    return true;
}

void SongCache::set_entries(std::vector<ChartCacheEntry> entries) {
    entries_ = std::move(entries);
    last_updated_ = current_time_iso8601();
}

const std::vector<ChartCacheEntry>& SongCache::entries() const {
    return entries_;
}

std::optional<ChartCacheEntry> SongCache::get(const std::filesystem::path& path) const {
    for (const auto& entry : entries_) {
        if (entry.path == path) {
            return entry;
        }
    }
    return std::nullopt;
}

void SongCache::put(const ChartCacheEntry& entry) {
    // Update existing entry or add new one
    for (auto& existing : entries_) {
        if (existing.path == entry.path) {
            existing = entry;
            return;
        }
    }
    entries_.push_back(entry);
}

size_t SongCache::size() const {
    return entries_.size();
}

const std::string& SongCache::last_updated() const {
    return last_updated_;
}

} // namespace openitup::data
