#include <gtest/gtest.h>

#include <openitup/data/settings.h>

using namespace openitup::data;

class SettingsTest : public ::testing::Test {
protected:
};

TEST_F(SettingsTest, MakeDefaultReturnsValidSettings) {
    auto defaults = SettingsData::make_default();

    EXPECT_EQ(defaults.schema_version, 1);
    EXPECT_EQ(defaults.video.width, 1920);
    EXPECT_EQ(defaults.video.height, 1080);
    EXPECT_FLOAT_EQ(defaults.audio.master_volume, 1.0f);
    EXPECT_FLOAT_EQ(defaults.audio.music_volume, 1.0f);
    EXPECT_FLOAT_EQ(defaults.audio.sfx_volume, 1.0f);
}

TEST_F(SettingsTest, DefaultSettingsRoundTrip) {
    auto original = SettingsData::make_default();

    // Serialize to JSON
    nlohmann::json j = original;

    // Deserialize back
    auto restored = j.get<SettingsData>();

    EXPECT_EQ(restored.schema_version, original.schema_version);
    EXPECT_EQ(restored.video.width, original.video.width);
    EXPECT_EQ(restored.video.height, original.video.height);
    EXPECT_FLOAT_EQ(restored.audio.master_volume, original.audio.master_volume);
    EXPECT_FLOAT_EQ(restored.audio.music_volume, original.audio.music_volume);
    EXPECT_FLOAT_EQ(restored.audio.sfx_volume, original.audio.sfx_volume);
}

TEST_F(SettingsTest, ParseAllFields) {
    nlohmann::json j = {
        {"schema_version", 1},
        {"video", {
            {"width", 2560},
            {"height", 1440}
        }},
        {"audio", {
            {"master_volume", 0.8f},
            {"music_volume", 0.7f},
            {"sfx_volume", 0.9f}
        }},
        {"input", {}}
    };

    auto settings = j.get<SettingsData>();

    EXPECT_EQ(settings.schema_version, 1);
    EXPECT_EQ(settings.video.width, 2560);
    EXPECT_EQ(settings.video.height, 1440);
    EXPECT_FLOAT_EQ(settings.audio.master_volume, 0.8f);
    EXPECT_FLOAT_EQ(settings.audio.music_volume, 0.7f);
    EXPECT_FLOAT_EQ(settings.audio.sfx_volume, 0.9f);
}

TEST_F(SettingsTest, MissingSectionUsesDefaults) {
    // JSON without audio section
    nlohmann::json j = {
        {"schema_version", 1},
        {"video", {
            {"width", 1280},
            {"height", 720}
        }},
        {"input", {}}
    };

    auto settings = j.get<SettingsData>();

    EXPECT_EQ(settings.video.width, 1280);
    EXPECT_EQ(settings.video.height, 720);
    // Audio should have defaults
    EXPECT_FLOAT_EQ(settings.audio.master_volume, 1.0f);
    EXPECT_FLOAT_EQ(settings.audio.music_volume, 1.0f);
    EXPECT_FLOAT_EQ(settings.audio.sfx_volume, 1.0f);
}

TEST_F(SettingsTest, MissingFieldsUseDefaults) {
    // JSON with partial audio section
    nlohmann::json j = {
        {"schema_version", 1},
        {"video", {
            {"width", 1280},
            {"height", 720}
        }},
        {"audio", {
            {"master_volume", 0.5f}
            // music_volume and sfx_volume missing
        }},
        {"input", {}}
    };

    auto settings = j.get<SettingsData>();

    EXPECT_FLOAT_EQ(settings.audio.master_volume, 0.5f);
    EXPECT_FLOAT_EQ(settings.audio.music_volume, 1.0f);  // default
    EXPECT_FLOAT_EQ(settings.audio.sfx_volume, 1.0f);    // default
}

TEST_F(SettingsTest, InvalidJsonUsesDefaults) {
    // Invalid JSON (not an object)
    nlohmann::json j = "not an object";

    // This should throw, so we catch it and use defaults
    SettingsData settings;
    try {
        settings = j.get<SettingsData>();
    } catch (const nlohmann::json::exception&) {
        settings = SettingsData::make_default();
    }

    EXPECT_EQ(settings.schema_version, 1);
    EXPECT_EQ(settings.video.width, 1920);
    EXPECT_EQ(settings.video.height, 1080);
}

TEST_F(SettingsTest, SchemaVersionPreserved) {
    nlohmann::json j = {
        {"schema_version", 2},
        {"video", {
            {"width", 1920},
            {"height", 1080}
        }},
        {"audio", {
            {"master_volume", 1.0f},
            {"music_volume", 1.0f},
            {"sfx_volume", 1.0f}
        }},
        {"input", {}}
    };

    auto settings = j.get<SettingsData>();
    EXPECT_EQ(settings.schema_version, 2);

    // Round-trip
    nlohmann::json j2 = settings;
    EXPECT_EQ(j2["schema_version"], 2);
}

TEST_F(SettingsTest, SerializesToPrettyJson) {
    auto settings = SettingsData::make_default();
    nlohmann::json j = settings;

    std::string output = j.dump(4);  // 4-space indent

    // Should be valid JSON
    auto parsed = nlohmann::json::parse(output);
    EXPECT_TRUE(parsed.is_object());
    EXPECT_TRUE(parsed.contains("video"));
    EXPECT_TRUE(parsed.contains("audio"));
    EXPECT_TRUE(parsed.contains("input"));
}
