#include <gtest/gtest.h>

#include <openitup/data/profile.h>
#include <nlohmann/json.hpp>

using namespace openitup::data;

// US-DAT-008 Scenario 1: Valid profile with all required fields
TEST(ProfileTest, ValidProfileAllFields) {
    ProfileData profile = ProfileData::make_default("Player1");
    profile.speed_mod.type = SpeedModType::CONSTANT;
    profile.speed_mod.value = 450.0f;
    profile.note_skin = "default";
    profile.input_offset_ms = 0;
    profile.audio_offset_ms = 0;
    profile.statistics.songs_played = 0;
    profile.statistics.total_time_hours = 0.0f;
    profile.statistics.total_score = 0;

    nlohmann::json j = profile;

    EXPECT_EQ(j["schema_version"], 1);
    EXPECT_EQ(j["display_name"], "Player1");
    EXPECT_EQ(j["speed_mod"]["type"], "C");
    EXPECT_FLOAT_EQ(j["speed_mod"]["value"], 450.0f);
    EXPECT_EQ(j["note_skin"], "default");
    EXPECT_EQ(j["input_offset_ms"], 0);
    EXPECT_EQ(j["audio_offset_ms"], 0);
    EXPECT_EQ(j["statistics"]["songs_played"], 0);
    EXPECT_FLOAT_EQ(j["statistics"]["total_time_hours"], 0.0f);
    EXPECT_EQ(j["statistics"]["total_score"], 0);
    EXPECT_TRUE(j["high_scores"].is_object());
    EXPECT_EQ(j["high_scores"].size(), 0);
}

// US-DAT-008 Scenario 2: High score entry structure
TEST(ProfileTest, HighScoreEntryStructure) {
    HighScoreEntry entry;
    entry.score = 950000;
    entry.grade = "S";
    entry.max_combo = 500;
    entry.judgments.perfect = 480;
    entry.judgments.great = 20;
    entry.judgments.good = 0;
    entry.judgments.bad = 0;
    entry.judgments.miss = 0;
    entry.date = "2026-04-15T10:30:00Z";
    entry.judge_profile = "exceed";

    nlohmann::json j = entry;

    EXPECT_EQ(j["score"], 950000);
    EXPECT_EQ(j["grade"], "S");
    EXPECT_EQ(j["max_combo"], 500);
    EXPECT_EQ(j["judgments"]["perfect"], 480);
    EXPECT_EQ(j["judgments"]["great"], 20);
    EXPECT_EQ(j["judgments"]["good"], 0);
    EXPECT_EQ(j["judgments"]["bad"], 0);
    EXPECT_EQ(j["judgments"]["miss"], 0);
    EXPECT_EQ(j["date"], "2026-04-15T10:30:00Z");
    EXPECT_EQ(j["judge_profile"], "exceed");
}

// US-DAT-008: Profile with high scores keyed by chart hash
TEST(ProfileTest, ProfileWithHighScores) {
    ProfileData profile = ProfileData::make_default("TestPlayer");

    HighScoreEntry entry1;
    entry1.score = 950000;
    entry1.grade = "S";
    entry1.max_combo = 500;
    entry1.date = "2026-04-15T10:30:00Z";
    entry1.judge_profile = "exceed";

    HighScoreEntry entry2;
    entry2.score = 920000;
    entry2.grade = "A";
    entry2.max_combo = 480;
    entry2.date = "2026-04-14T10:30:00Z";
    entry2.judge_profile = "exceed";

    std::string chart_hash = "abc123def456";
    profile.high_scores[chart_hash] = {entry1, entry2};

    nlohmann::json j = profile;

    EXPECT_TRUE(j["high_scores"].contains(chart_hash));
    EXPECT_EQ(j["high_scores"][chart_hash].size(), 2);
    EXPECT_EQ(j["high_scores"][chart_hash][0]["score"], 950000);
    EXPECT_EQ(j["high_scores"][chart_hash][1]["score"], 920000);
}

// US-DAT-009 Scenario 1: No profiles exist
TEST(ProfileTest, CreateDefaultProfile) {
    ProfileData profile = create_default_profile("Player");

    EXPECT_EQ(profile.schema_version, 1);
    EXPECT_EQ(profile.display_name, "Player");
    EXPECT_EQ(profile.speed_mod.type, SpeedModType::MULTIPLIER);
    EXPECT_FLOAT_EQ(profile.speed_mod.value, 3.0f);
    EXPECT_EQ(profile.note_skin, "default");
    EXPECT_EQ(profile.input_offset_ms, 0);
    EXPECT_EQ(profile.audio_offset_ms, 0);
    EXPECT_EQ(profile.statistics.songs_played, 0);
    EXPECT_FLOAT_EQ(profile.statistics.total_time_hours, 0.0f);
    EXPECT_EQ(profile.statistics.total_score, 0);
    EXPECT_TRUE(profile.high_scores.empty());
    EXPECT_FALSE(profile.created_date.empty());
    EXPECT_FALSE(profile.last_played_date.empty());
}

// US-DAT-009: Default profile has M3.0 speed mod
TEST(ProfileTest, DefaultProfileSpeedMod) {
    ProfileData profile = create_default_profile("TestUser");

    EXPECT_EQ(profile.speed_mod.type, SpeedModType::MULTIPLIER);
    EXPECT_FLOAT_EQ(profile.speed_mod.value, 3.0f);
}

// US-DAT-008: JSON round-trip
TEST(ProfileTest, JSONRoundTrip) {
    ProfileData original = create_default_profile("RoundTrip");
    original.total_plays = 42;
    original.speed_mod.type = SpeedModType::CONSTANT;
    original.speed_mod.value = 550.0f;

    nlohmann::json j = original;
    ProfileData restored = j.get<ProfileData>();

    EXPECT_EQ(restored.schema_version, original.schema_version);
    EXPECT_EQ(restored.display_name, original.display_name);
    EXPECT_EQ(restored.total_plays, original.total_plays);
    EXPECT_EQ(restored.speed_mod.type, original.speed_mod.type);
    EXPECT_FLOAT_EQ(restored.speed_mod.value, original.speed_mod.value);
    EXPECT_EQ(restored.note_skin, original.note_skin);
    EXPECT_EQ(restored.input_offset_ms, original.input_offset_ms);
    EXPECT_EQ(restored.audio_offset_ms, original.audio_offset_ms);
}

// US-DAT-008: SpeedMod JSON serialization
TEST(ProfileTest, SpeedModSerialization) {
    SpeedMod multiplier_mod;
    multiplier_mod.type = SpeedModType::MULTIPLIER;
    multiplier_mod.value = 2.5f;

    nlohmann::json j_mult = multiplier_mod;
    EXPECT_EQ(j_mult["type"], "M");
    EXPECT_FLOAT_EQ(j_mult["value"], 2.5f);

    SpeedMod constant_mod;
    constant_mod.type = SpeedModType::CONSTANT;
    constant_mod.value = 450.0f;

    nlohmann::json j_const = constant_mod;
    EXPECT_EQ(j_const["type"], "C");
    EXPECT_FLOAT_EQ(j_const["value"], 450.0f);
}

// US-DAT-008: JudgmentCounts JSON serialization
TEST(ProfileTest, JudgmentCountsSerialization) {
    JudgmentCounts jc;
    jc.perfect = 100;
    jc.great = 20;
    jc.good = 5;
    jc.bad = 2;
    jc.miss = 1;

    nlohmann::json j = jc;
    EXPECT_EQ(j["perfect"], 100);
    EXPECT_EQ(j["great"], 20);
    EXPECT_EQ(j["good"], 5);
    EXPECT_EQ(j["bad"], 2);
    EXPECT_EQ(j["miss"], 1);

    JudgmentCounts restored = j.get<JudgmentCounts>();
    EXPECT_EQ(restored.perfect, 100);
    EXPECT_EQ(restored.great, 20);
    EXPECT_EQ(restored.good, 5);
    EXPECT_EQ(restored.bad, 2);
    EXPECT_EQ(restored.miss, 1);
}

// US-DAT-030 Scenario 1: Unknown field preserved
TEST(ProfileTest, UnknownFieldPreserved) {
    nlohmann::json j = {
        {"schema_version", 1},
        {"display_name", "TestPlayer"},
        {"created_date", "2026-04-30T00:00:00Z"},
        {"last_played_date", "2026-04-30T00:00:00Z"},
        {"total_plays", 0},
        {"speed_mod", {{"type", "M"}, {"value", 3.0f}}},
        {"note_skin", "default"},
        {"input_offset_ms", 0},
        {"audio_offset_ms", 0},
        {"statistics", {{"songs_played", 0}, {"total_time_hours", 0.0f}, {"total_score", 0}}},
        {"high_scores", nlohmann::json::object()},
        {"future_feature", {{"data", 123}, {"enabled", true}}}
    };

    ProfileData profile = j.get<ProfileData>();
    EXPECT_EQ(profile.display_name, "TestPlayer");
    EXPECT_TRUE(profile.unknown_fields_.contains("future_feature"));
    EXPECT_EQ(profile.unknown_fields_["future_feature"]["data"], 123);
    EXPECT_EQ(profile.unknown_fields_["future_feature"]["enabled"], true);

    // Serialize back and verify unknown field is preserved
    nlohmann::json j_saved = profile;
    EXPECT_TRUE(j_saved.contains("future_feature"));
    EXPECT_EQ(j_saved["future_feature"]["data"], 123);
    EXPECT_EQ(j_saved["future_feature"]["enabled"], true);
}

// US-DAT-030: Multiple unknown fields preserved
TEST(ProfileTest, MultipleUnknownFieldsPreserved) {
    nlohmann::json j = {
        {"schema_version", 1},
        {"display_name", "TestPlayer"},
        {"created_date", "2026-04-30T00:00:00Z"},
        {"last_played_date", "2026-04-30T00:00:00Z"},
        {"total_plays", 0},
        {"speed_mod", {{"type", "M"}, {"value", 3.0f}}},
        {"note_skin", "default"},
        {"input_offset_ms", 0},
        {"audio_offset_ms", 0},
        {"statistics", {{"songs_played", 0}, {"total_time_hours", 0.0f}, {"total_score", 0}}},
        {"high_scores", nlohmann::json::object()},
        {"experimental_setting", 42},
        {"future_array", {1, 2, 3}}
    };

    ProfileData profile = j.get<ProfileData>();

    EXPECT_TRUE(profile.unknown_fields_.contains("experimental_setting"));
    EXPECT_EQ(profile.unknown_fields_["experimental_setting"], 42);
    EXPECT_TRUE(profile.unknown_fields_.contains("future_array"));
    EXPECT_EQ(profile.unknown_fields_["future_array"].size(), 3);

    // Round-trip
    nlohmann::json j_saved = profile;
    EXPECT_TRUE(j_saved.contains("experimental_setting"));
    EXPECT_EQ(j_saved["experimental_setting"], 42);
    EXPECT_TRUE(j_saved.contains("future_array"));
    EXPECT_EQ(j_saved["future_array"].size(), 3);
}

// US-DAT-030: Known fields not duplicated in unknown_fields
TEST(ProfileTest, KnownFieldsNotInUnknown) {
    nlohmann::json j = {
        {"schema_version", 1},
        {"display_name", "TestPlayer"},
        {"created_date", "2026-04-30T00:00:00Z"},
        {"last_played_date", "2026-04-30T00:00:00Z"},
        {"total_plays", 5},
        {"speed_mod", {{"type", "M"}, {"value", 3.0f}}},
        {"note_skin", "default"},
        {"input_offset_ms", 0},
        {"audio_offset_ms", 0},
        {"statistics", {{"songs_played", 0}, {"total_time_hours", 0.0f}, {"total_score", 0}}},
        {"high_scores", nlohmann::json::object()}
    };

    ProfileData profile = j.get<ProfileData>();

    EXPECT_FALSE(profile.unknown_fields_.contains("schema_version"));
    EXPECT_FALSE(profile.unknown_fields_.contains("display_name"));
    EXPECT_FALSE(profile.unknown_fields_.contains("total_plays"));
    EXPECT_FALSE(profile.unknown_fields_.contains("speed_mod"));
    EXPECT_FALSE(profile.unknown_fields_.contains("note_skin"));
}
