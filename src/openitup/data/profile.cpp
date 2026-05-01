#include <openitup/data/profile.h>

#include <chrono>
#include <iomanip>
#include <sstream>

namespace openitup::data {

namespace {

// Get current time as ISO 8601 string
std::string current_iso8601_time() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc{};

#ifdef _WIN32
    gmtime_s(&tm_utc, &time_t);
#else
    gmtime_r(&time_t, &tm_utc);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // anonymous namespace

SpeedMod SpeedMod::make_default() {
    SpeedMod mod;
    mod.type = SpeedModType::MULTIPLIER;
    mod.value = 3.0f;
    return mod;
}

ProfileData ProfileData::make_default(const std::string& name) {
    ProfileData profile;
    profile.schema_version = 1;
    profile.display_name = name;
    profile.created_date = current_iso8601_time();
    profile.last_played_date = profile.created_date;
    profile.total_plays = 0;
    profile.speed_mod = SpeedMod::make_default();
    profile.note_skin = "default";
    profile.input_offset_ms = 0;
    profile.audio_offset_ms = 0;
    profile.statistics = PlayStatistics{};
    profile.high_scores.clear();
    return profile;
}

ProfileData create_default_profile(const std::string& name) {
    return ProfileData::make_default(name);
}

// JSON serialization

void to_json(nlohmann::json& j, const SpeedMod& s) {
    j = nlohmann::json{
        {"type", s.type == SpeedModType::MULTIPLIER ? "M" : "C"},
        {"value", s.value}
    };
}

void from_json(const nlohmann::json& j, SpeedMod& s) {
    std::string type_str = j.at("type").get<std::string>();
    s.type = (type_str == "M") ? SpeedModType::MULTIPLIER : SpeedModType::CONSTANT;
    s.value = j.at("value").get<float>();
}

void to_json(nlohmann::json& j, const JudgmentCounts& jc) {
    j = nlohmann::json{
        {"perfect", jc.perfect},
        {"great", jc.great},
        {"good", jc.good},
        {"bad", jc.bad},
        {"miss", jc.miss}
    };
}

void from_json(const nlohmann::json& j, JudgmentCounts& jc) {
    jc.perfect = j.at("perfect").get<int>();
    jc.great = j.at("great").get<int>();
    jc.good = j.at("good").get<int>();
    jc.bad = j.at("bad").get<int>();
    jc.miss = j.at("miss").get<int>();
}

void to_json(nlohmann::json& j, const HighScoreEntry& h) {
    j = nlohmann::json{
        {"score", h.score},
        {"grade", h.grade},
        {"max_combo", h.max_combo},
        {"judgments", h.judgments},
        {"date", h.date},
        {"judge_profile", h.judge_profile}
    };
}

void from_json(const nlohmann::json& j, HighScoreEntry& h) {
    h.score = j.at("score").get<int>();
    h.grade = j.at("grade").get<std::string>();
    h.max_combo = j.at("max_combo").get<int>();
    h.judgments = j.at("judgments").get<JudgmentCounts>();
    h.date = j.at("date").get<std::string>();
    h.judge_profile = j.at("judge_profile").get<std::string>();
}

void to_json(nlohmann::json& j, const PlayStatistics& p) {
    j = nlohmann::json{
        {"songs_played", p.songs_played},
        {"total_time_hours", p.total_time_hours},
        {"total_score", p.total_score}
    };
}

void from_json(const nlohmann::json& j, PlayStatistics& p) {
    p.songs_played = j.at("songs_played").get<int>();
    p.total_time_hours = j.at("total_time_hours").get<float>();
    p.total_score = j.at("total_score").get<int>();
}

void to_json(nlohmann::json& j, const ProfileData& p) {
    j = nlohmann::json{
        {"schema_version", p.schema_version},
        {"display_name", p.display_name},
        {"created_date", p.created_date},
        {"last_played_date", p.last_played_date},
        {"total_plays", p.total_plays},
        {"speed_mod", p.speed_mod},
        {"note_skin", p.note_skin},
        {"input_offset_ms", p.input_offset_ms},
        {"audio_offset_ms", p.audio_offset_ms},
        {"statistics", p.statistics},
        {"high_scores", p.high_scores}
    };
}

void from_json(const nlohmann::json& j, ProfileData& p) {
    p.schema_version = j.value("schema_version", 1);
    p.display_name = j.at("display_name").get<std::string>();
    p.created_date = j.value("created_date", "");
    p.last_played_date = j.value("last_played_date", "");
    p.total_plays = j.value("total_plays", 0);
    p.speed_mod = j.value("speed_mod", SpeedMod::make_default());
    p.note_skin = j.value("note_skin", "default");
    p.input_offset_ms = j.value("input_offset_ms", 0);
    p.audio_offset_ms = j.value("audio_offset_ms", 0);
    p.statistics = j.value("statistics", PlayStatistics{});
    p.high_scores = j.value("high_scores", std::map<std::string, std::vector<HighScoreEntry>>{});
}

} // namespace openitup::data
