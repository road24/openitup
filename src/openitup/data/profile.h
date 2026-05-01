#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace openitup::data {

// Speed mod type
enum class SpeedModType : uint8_t {
    MULTIPLIER = 0,  // M2.0, M3.0, etc.
    CONSTANT = 1      // C450, C550, etc.
};

struct SpeedMod {
    SpeedModType type = SpeedModType::MULTIPLIER;
    float value = 3.0f;  // Default M3.0

    static SpeedMod make_default();
};

struct JudgmentCounts {
    int perfect = 0;
    int great = 0;
    int good = 0;
    int bad = 0;
    int miss = 0;
};

// High score entry for a single play
struct HighScoreEntry {
    int score = 0;
    std::string grade;  // "SSS", "SS", "S", "A", "B", "C", "D", "F"
    int max_combo = 0;
    JudgmentCounts judgments;
    std::string date;  // ISO 8601 format: "2026-04-30T15:30:00Z"
    std::string judge_profile;  // "exceed", "nx2", etc.
};

struct PlayStatistics {
    int songs_played = 0;
    float total_time_hours = 0.0f;
    int total_score = 0;
};

struct ProfileData {
    int schema_version = 1;

    std::string display_name;
    std::string created_date;  // ISO 8601
    std::string last_played_date;  // ISO 8601
    int total_plays = 0;

    SpeedMod speed_mod;
    std::string note_skin;  // Directory name, e.g. "default"
    int input_offset_ms = 0;
    int audio_offset_ms = 0;

    PlayStatistics statistics;

    // High scores keyed by chart content hash (64-char hex string).
    // Each chart can have multiple high score entries (up to 10).
    std::map<std::string, std::vector<HighScoreEntry>> high_scores;

    // Preserve unknown fields for forward compatibility (US-DAT-030)
    nlohmann::json unknown_fields_;

    static ProfileData make_default(const std::string& name);
};

// JSON serialization functions
void to_json(nlohmann::json& j, const SpeedMod& s);
void from_json(const nlohmann::json& j, SpeedMod& s);

void to_json(nlohmann::json& j, const JudgmentCounts& jc);
void from_json(const nlohmann::json& j, JudgmentCounts& jc);

void to_json(nlohmann::json& j, const HighScoreEntry& h);
void from_json(const nlohmann::json& j, HighScoreEntry& h);

void to_json(nlohmann::json& j, const PlayStatistics& p);
void from_json(const nlohmann::json& j, PlayStatistics& p);

void to_json(nlohmann::json& j, const ProfileData& p);
void from_json(const nlohmann::json& j, ProfileData& p);

// Create a default profile with given name
ProfileData create_default_profile(const std::string& name);

} // namespace openitup::data
