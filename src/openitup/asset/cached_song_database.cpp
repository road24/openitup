#include "openitup/asset/cached_song_database.h"

#include <fstream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

namespace openitup {

CachedSongDatabase::CachedSongDatabase()
    : database_(std::make_unique<SongDatabase>()) {}

bool CachedSongDatabase::initialize(const std::filesystem::path& cache_path,
                                   const std::vector<std::filesystem::path>& data_dirs) {
    cache_path_ = cache_path;
    data_dirs_ = data_dirs;

    // Try to load from cache first
    if (try_load_cache()) {
        spdlog::info("CachedSongDatabase: loaded {} songs from cache", song_count());
        return true;
    }

    // Cache miss or invalid — scan directories
    spdlog::info("CachedSongDatabase: cache miss, scanning directories");
    scan_directories();
    save_cache();

    return song_count() > 0;
}

std::vector<SongDatabaseEntry> CachedSongDatabase::get_songs() const {
    return database_->get_songs();
}

std::optional<SongDatabaseEntry> CachedSongDatabase::get_song(std::size_t index) const {
    return database_->get_song(index);
}

std::size_t CachedSongDatabase::song_count() const {
    return database_->song_count();
}

bool CachedSongDatabase::try_load_cache() {
    if (!std::filesystem::exists(cache_path_)) {
        spdlog::debug("CachedSongDatabase: cache file does not exist: {}", cache_path_.string());
        return false;
    }

    if (!is_cache_valid()) {
        spdlog::debug("CachedSongDatabase: cache is invalid");
        return false;
    }

    try {
        std::ifstream file(cache_path_);
        if (!file.is_open()) {
            spdlog::warn("CachedSongDatabase: failed to open cache file: {}", cache_path_.string());
            return false;
        }

        json j;
        file >> j;

        // Clear existing database
        database_ = std::make_unique<SongDatabase>();

        // Load songs from cache
        if (j.contains("songs")) {
            for (const auto& song_json : j["songs"]) {
                SongDatabaseEntry entry;
                entry.song_path = song_json["song_path"].get<std::string>();
                entry.title = song_json["title"].get<std::string>();

                if (song_json.contains("artist")) {
                    entry.artist = song_json["artist"].get<std::string>();
                }
                if (song_json.contains("bpm")) {
                    entry.bpm = song_json["bpm"].get<double>();
                }

                if (song_json.contains("chart_paths")) {
                    for (const auto& chart_path_str : song_json["chart_paths"]) {
                        entry.chart_paths.push_back(chart_path_str.get<std::string>());
                    }
                }

                if (song_json.contains("audio_path")) {
                    entry.audio_path = song_json["audio_path"].get<std::string>();
                }
                if (song_json.contains("banner_path")) {
                    entry.banner_path = song_json["banner_path"].get<std::string>();
                }
                if (song_json.contains("bga_path")) {
                    entry.bga_path = song_json["bga_path"].get<std::string>();
                }

                entry.is_valid = song_json.value("is_valid", true);

                // Directly add to internal vector (hack since SongDatabase doesn't expose add method)
                // For now, we'll need to use the scan result and manually populate
                // This is a limitation of the current SongDatabase API
            }
        }

        // Since we can't directly populate the database, we'll need to rescan for now
        // TODO Phase 3: add SongDatabase::add_entry() method or make database_ friends
        spdlog::debug("CachedSongDatabase: cache loaded but needs refactoring for proper loading");
        return false;  // Force rescan for now

    } catch (const std::exception& e) {
        spdlog::warn("CachedSongDatabase: failed to parse cache: {}", e.what());
        return false;
    }
}

void CachedSongDatabase::scan_directories() {
    database_->scan(data_dirs_);
}

void CachedSongDatabase::save_cache() {
    try {
        // Ensure parent directory exists
        if (cache_path_.has_parent_path()) {
            std::filesystem::create_directories(cache_path_.parent_path());
        }

        json j;

        // Save data directories for cache validation
        j["data_directories"] = json::array();
        for (const auto& dir : data_dirs_) {
            j["data_directories"].push_back(dir.string());
        }

        // Save songs
        j["songs"] = json::array();
        for (const auto& song : database_->get_songs()) {
            json song_json;
            song_json["song_path"] = song.song_path.string();
            song_json["title"] = song.title;

            if (!song.artist.empty()) {
                song_json["artist"] = song.artist;
            }
            if (song.bpm > 0.0) {
                song_json["bpm"] = song.bpm;
            }

            song_json["chart_paths"] = json::array();
            for (const auto& chart_path : song.chart_paths) {
                song_json["chart_paths"].push_back(chart_path.string());
            }

            if (!song.audio_path.empty()) {
                song_json["audio_path"] = song.audio_path.string();
            }
            if (!song.banner_path.empty()) {
                song_json["banner_path"] = song.banner_path.string();
            }
            if (!song.bga_path.empty()) {
                song_json["bga_path"] = song.bga_path.string();
            }

            song_json["is_valid"] = song.is_valid;

            j["songs"].push_back(song_json);
        }

        std::ofstream file(cache_path_);
        if (!file.is_open()) {
            spdlog::error("CachedSongDatabase: failed to open cache file for writing: {}",
                         cache_path_.string());
            return;
        }

        file << j.dump(2);
        spdlog::info("CachedSongDatabase: saved cache with {} songs to {}",
                    song_count(), cache_path_.string());

    } catch (const std::exception& e) {
        spdlog::error("CachedSongDatabase: failed to save cache: {}", e.what());
    }
}

bool CachedSongDatabase::is_cache_valid() const {
    // Phase 2: simple validity — just check if cache file exists
    // Phase 3: check directory mtimes for invalidation

    try {
        std::ifstream file(cache_path_);
        if (!file.is_open()) {
            return false;
        }

        json j;
        file >> j;

        // Check if data directories match
        if (!j.contains("data_directories")) {
            return false;
        }

        auto cached_dirs = j["data_directories"];
        if (cached_dirs.size() != data_dirs_.size()) {
            spdlog::debug("CachedSongDatabase: cache invalid - directory count mismatch");
            return false;
        }

        for (size_t i = 0; i < data_dirs_.size(); ++i) {
            std::string cached_dir = cached_dirs[i].get<std::string>();
            if (cached_dir != data_dirs_[i].string()) {
                spdlog::debug("CachedSongDatabase: cache invalid - directory mismatch at index {}", i);
                return false;
            }

            // Check directory still exists
            if (!std::filesystem::exists(data_dirs_[i])) {
                spdlog::debug("CachedSongDatabase: cache invalid - directory no longer exists: {}",
                             data_dirs_[i].string());
                return false;
            }
        }

        return true;

    } catch (const std::exception& e) {
        spdlog::warn("CachedSongDatabase: error validating cache: {}", e.what());
        return false;
    }
}

} // namespace openitup
