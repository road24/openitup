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

TEST_F(SettingsTest, InputSettingsHasDefaultKeymap) {
    auto input_settings = InputSettings::make_default();

    // Should have the default QWEASDZXC keymap
    EXPECT_EQ(input_settings.keymap.size(), 8);
    EXPECT_EQ(input_settings.keymap["SDL_SCANCODE_Q"], "P1_UP_LEFT");
    EXPECT_EQ(input_settings.keymap["SDL_SCANCODE_E"], "P1_UP_RIGHT");
    EXPECT_EQ(input_settings.keymap["SDL_SCANCODE_S"], "P1_CENTER");
    EXPECT_EQ(input_settings.keymap["SDL_SCANCODE_Z"], "P1_DOWN_LEFT");
    EXPECT_EQ(input_settings.keymap["SDL_SCANCODE_C"], "P1_DOWN_RIGHT");
    EXPECT_EQ(input_settings.keymap["SDL_SCANCODE_RETURN"], "START");
    EXPECT_EQ(input_settings.keymap["SDL_SCANCODE_ESCAPE"], "BACK");
    EXPECT_EQ(input_settings.keymap["SDL_SCANCODE_SPACE"], "SELECT");
}

TEST_F(SettingsTest, InputSettingsRoundTrip) {
    auto original = InputSettings::make_default();

    // Serialize to JSON
    nlohmann::json j = original;

    // Deserialize back
    auto restored = j.get<InputSettings>();

    EXPECT_EQ(restored.keymap.size(), original.keymap.size());
    for (const auto& [key, value] : original.keymap) {
        EXPECT_EQ(restored.keymap[key], value);
    }
}

TEST_F(SettingsTest, InputSettingsWithCustomKeymap) {
    nlohmann::json j = {
        {"keymap", {
            {"SDL_SCANCODE_A", "P1_UP_LEFT"},
            {"SDL_SCANCODE_D", "P1_UP_RIGHT"},
        }}
    };

    auto settings = j.get<InputSettings>();

    EXPECT_EQ(settings.keymap.size(), 2);
    EXPECT_EQ(settings.keymap["SDL_SCANCODE_A"], "P1_UP_LEFT");
    EXPECT_EQ(settings.keymap["SDL_SCANCODE_D"], "P1_UP_RIGHT");
}

TEST_F(SettingsTest, InputSettingsMissingKeymapUsesDefaults) {
    nlohmann::json j = nlohmann::json::object();  // Empty object

    auto settings = j.get<InputSettings>();

    // Should have default keymap
    EXPECT_EQ(settings.keymap.size(), 8);
    EXPECT_EQ(settings.keymap["SDL_SCANCODE_Q"], "P1_UP_LEFT");
}

TEST_F(SettingsTest, InputSettingsInvalidKeymapUsesDefaults) {
    nlohmann::json j = {
        {"keymap", "not an object"}  // Invalid type
    };

    auto settings = j.get<InputSettings>();

    // Should fall back to default keymap
    EXPECT_EQ(settings.keymap.size(), 8);
    EXPECT_EQ(settings.keymap["SDL_SCANCODE_Q"], "P1_UP_LEFT");
}

TEST_F(SettingsTest, FullSettingsWithKeymapRoundTrip) {
    auto original = SettingsData::make_default();

    // Serialize to JSON
    nlohmann::json j = original;

    // Deserialize back
    auto restored = j.get<SettingsData>();

    // Check input keymap was preserved
    EXPECT_EQ(restored.input.keymap.size(), 8);
    EXPECT_EQ(restored.input.keymap["SDL_SCANCODE_Q"], "P1_UP_LEFT");
    EXPECT_EQ(restored.input.keymap["SDL_SCANCODE_E"], "P1_UP_RIGHT");
}
