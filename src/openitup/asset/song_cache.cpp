#include "openitup/asset/song_cache.h"

#include <fstream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

namespace openitup {

std::optional<SongCache> SongCache::load(const std::filesystem::path& cache_path) {
    if (!std::filesystem::exists(cache_path)) {
        spdlog::debug("Song cache does not exist: {}", cache_path.string());
        return std::nullopt;
    }

    try {
        std::ifstream file(cache_path);
        if (!file.is_open()) {
            spdlog::warn("Failed to open song cache: {}", cache_path.string());
            return std::nullopt;
        }

        json j;
        file >> j;

        SongCache cache;

        // Load data directories
        if (j.contains("data_directories")) {
            for (const auto& dir_str : j["data_directories"]) {
                cache.data_directories_.push_back(dir_str.get<std::string>());
            }
        }

        // Load songs
        if (j.contains("songs")) {
            for (const auto& song_json : j["songs"]) {
                SongMetadata song;
                song.folder_path = song_json["folder_path"].get<std::string>();
                song.title = song_json["title"].get<std::string>();

                if (song_json.contains("artist")) {
                    song.artist = song_json["artist"].get<std::string>();
                }
                if (song_json.contains("bpm")) {
                    song.bpm = song_json["bpm"].get<double>();
                }

                if (song_json.contains("charts")) {
                    for (const auto& chart_json : song_json["charts"]) {
                        ChartMetadata chart;
                        chart.chart_path = chart_json["chart_path"].get<std::string>();
                        if (chart_json.contains("difficulty")) {
                            chart.difficulty = chart_json["difficulty"].get<std::string>();
                        }
                        if (chart_json.contains("level")) {
                            chart.level = chart_json["level"].get<int>();
                        }
                        song.charts.push_back(std::move(chart));
                    }
                }

                cache.database_.add_song(std::move(song));
            }
        }

        spdlog::info("Loaded song cache with {} songs from {}",
                     cache.database_.song_count(), cache_path.string());
        return cache;

    } catch (const std::exception& e) {
        spdlog::warn("Failed to parse song cache {}: {}", cache_path.string(), e.what());
        return std::nullopt;
    }
}

bool SongCache::save(const std::filesystem::path& cache_path) const {
    try {
        // Ensure parent directory exists
        if (cache_path.has_parent_path()) {
            std::filesystem::create_directories(cache_path.parent_path());
        }

        json j;

        // Save data directories
        j["data_directories"] = json::array();
        for (const auto& dir : data_directories_) {
            j["data_directories"].push_back(dir.string());
        }

        // Save songs
        j["songs"] = json::array();
        for (const auto& song : database_.get_all_songs()) {
            json song_json;
            song_json["folder_path"] = song.folder_path.string();
            song_json["title"] = song.title;

            if (!song.artist.empty()) {
                song_json["artist"] = song.artist;
            }
            if (song.bpm > 0.0) {
                song_json["bpm"] = song.bpm;
            }

            if (!song.charts.empty()) {
                song_json["charts"] = json::array();
                for (const auto& chart : song.charts) {
                    json chart_json;
                    chart_json["chart_path"] = chart.chart_path.string();
                    if (!chart.difficulty.empty()) {
                        chart_json["difficulty"] = chart.difficulty;
                    }
                    if (chart.level > 0) {
                        chart_json["level"] = chart.level;
                    }
                    song_json["charts"].push_back(chart_json);
                }
            }

            j["songs"].push_back(song_json);
        }

        std::ofstream file(cache_path);
        if (!file.is_open()) {
            spdlog::error("Failed to open cache file for writing: {}", cache_path.string());
            return false;
        }

        file << j.dump(2);  // Pretty-print with 2-space indent
        spdlog::info("Saved song cache with {} songs to {}",
                     database_.song_count(), cache_path.string());
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Failed to save song cache {}: {}", cache_path.string(), e.what());
        return false;
    }
}

bool SongCache::is_valid_for(const std::vector<std::filesystem::path>& data_dirs) const {
    // Phase 2: simple validity check — directories must match
    // Phase 3: add mtime checking for invalidation

    if (data_dirs.size() != data_directories_.size()) {
        spdlog::debug("Cache invalid: directory count mismatch ({} != {})",
                     data_directories_.size(), data_dirs.size());
        return false;
    }

    for (size_t i = 0; i < data_dirs.size(); ++i) {
        if (data_dirs[i] != data_directories_[i]) {
            spdlog::debug("Cache invalid: directory mismatch at index {}", i);
            return false;
        }

        // Check directory still exists
        if (!std::filesystem::exists(data_dirs[i])) {
            spdlog::debug("Cache invalid: directory no longer exists: {}",
                         data_dirs[i].string());
            return false;
        }
    }

    spdlog::debug("Cache is valid for current data directories");
    return true;
}

void SongCache::set_database(SongDatabase database) {
    database_ = std::move(database);
}

void SongCache::set_data_directories(std::vector<std::filesystem::path> dirs) {
    data_directories_ = std::move(dirs);
}

} // namespace openitup
