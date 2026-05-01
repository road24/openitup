#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <string>

#include <openitup/data/settings_manager.h>

namespace {

class SettingsManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "test_settings_manager";
        std::filesystem::create_directories(test_dir_);
        test_path_ = test_dir_ / "settings.json";
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    std::filesystem::path test_dir_;
    std::filesystem::path test_path_;
};

TEST_F(SettingsManagerTest, LoadExistingValidSettings) {
    // Create a valid settings file
    std::string valid_json = R"({
        "schema_version": 1,
        "video": {
            "width": 1920,
            "height": 1080
        },
        "audio": {
            "master_volume": 0.8,
            "music_volume": 0.9,
            "sfx_volume": 0.7
        },
        "input": {}
    })";

    auto reader = [valid_json](const std::filesystem::path&) -> std::string {
        return valid_json;
    };

    std::string saved_content;
    auto writer = [&saved_content](const std::filesystem::path&, const std::string& content) -> bool {
        saved_content = content;
        return true;
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    bool loaded = mgr.load();

    EXPECT_TRUE(loaded);
    EXPECT_EQ(mgr.settings().video.width, 1920);
    EXPECT_EQ(mgr.settings().video.height, 1080);
    EXPECT_FLOAT_EQ(mgr.settings().audio.master_volume, 0.8f);
    EXPECT_FLOAT_EQ(mgr.settings().audio.music_volume, 0.9f);
    EXPECT_FLOAT_EQ(mgr.settings().audio.sfx_volume, 0.7f);
}

TEST_F(SettingsManagerTest, MissingFileCreatesDefaults) {
    auto reader = [](const std::filesystem::path&) -> std::string {
        throw std::runtime_error("File not found");
    };

    std::string saved_content;
    auto writer = [&saved_content](const std::filesystem::path&, const std::string& content) -> bool {
        saved_content = content;
        return true;
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    bool loaded = mgr.load();

    // Should create defaults and save
    EXPECT_TRUE(loaded);
    EXPECT_FALSE(saved_content.empty());

    // Verify defaults
    EXPECT_EQ(mgr.settings().video.width, 1920);
    EXPECT_EQ(mgr.settings().video.height, 1080);
    EXPECT_FLOAT_EQ(mgr.settings().audio.master_volume, 1.0f);
}

TEST_F(SettingsManagerTest, InvalidJsonUsesDefaults) {
    std::string invalid_json = "{ this is not valid json }";

    auto reader = [invalid_json](const std::filesystem::path&) -> std::string {
        return invalid_json;
    };

    std::string saved_content;
    auto writer = [&saved_content](const std::filesystem::path&, const std::string& content) -> bool {
        saved_content = content;
        return true;
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    bool loaded = mgr.load();

    // Should use defaults
    EXPECT_FALSE(loaded);
    EXPECT_EQ(mgr.settings().video.width, 1920);
    EXPECT_EQ(mgr.settings().video.height, 1080);
}

TEST_F(SettingsManagerTest, UpdateVideoSaves) {
    auto reader = [](const std::filesystem::path&) -> std::string {
        throw std::runtime_error("File not found");
    };

    std::string saved_content;
    int save_count = 0;
    auto writer = [&saved_content, &save_count](const std::filesystem::path&, const std::string& content) -> bool {
        saved_content = content;
        save_count++;
        return true;
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    mgr.load();  // Creates defaults, saves once

    save_count = 0;  // Reset counter
    bool success = mgr.update_video(2560, 1440);

    EXPECT_TRUE(success);
    EXPECT_EQ(save_count, 1);
    EXPECT_EQ(mgr.settings().video.width, 2560);
    EXPECT_EQ(mgr.settings().video.height, 1440);
    EXPECT_FALSE(saved_content.empty());
}

TEST_F(SettingsManagerTest, UpdateAudioSaves) {
    auto reader = [](const std::filesystem::path&) -> std::string {
        throw std::runtime_error("File not found");
    };

    std::string saved_content;
    int save_count = 0;
    auto writer = [&saved_content, &save_count](const std::filesystem::path&, const std::string& content) -> bool {
        saved_content = content;
        save_count++;
        return true;
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    mgr.load();

    openitup::data::AudioSettings audio;
    audio.master_volume = 0.5f;
    audio.music_volume = 0.6f;
    audio.sfx_volume = 0.7f;

    save_count = 0;
    bool success = mgr.update_audio(audio);

    EXPECT_TRUE(success);
    EXPECT_EQ(save_count, 1);
    EXPECT_FLOAT_EQ(mgr.settings().audio.master_volume, 0.5f);
    EXPECT_FLOAT_EQ(mgr.settings().audio.music_volume, 0.6f);
    EXPECT_FLOAT_EQ(mgr.settings().audio.sfx_volume, 0.7f);
}

TEST_F(SettingsManagerTest, WriteFailureLogged) {
    auto reader = [](const std::filesystem::path&) -> std::string {
        throw std::runtime_error("File not found");
    };

    auto writer = [](const std::filesystem::path&, const std::string&) -> bool {
        return false;  // Simulate write failure
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    bool loaded = mgr.load();

    // Load should fail because write fails when creating defaults
    EXPECT_FALSE(loaded);

    // Update should also fail
    bool update_success = mgr.update_video(1920, 1080);
    EXPECT_FALSE(update_success);
}

TEST_F(SettingsManagerTest, ResolutionTooSmall) {
    std::string json_with_tiny_resolution = R"({
        "schema_version": 1,
        "video": {
            "width": 1,
            "height": 1
        },
        "audio": {
            "master_volume": 1.0,
            "music_volume": 1.0,
            "sfx_volume": 1.0
        },
        "input": {}
    })";

    auto reader = [json_with_tiny_resolution](const std::filesystem::path&) -> std::string {
        return json_with_tiny_resolution;
    };

    auto writer = [](const std::filesystem::path&, const std::string&) -> bool {
        return true;
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    mgr.load();

    // Should be corrected to 1920x1080
    EXPECT_EQ(mgr.settings().video.width, 1920);
    EXPECT_EQ(mgr.settings().video.height, 1080);
}

TEST_F(SettingsManagerTest, ResolutionTooLarge) {
    std::string json_with_huge_resolution = R"({
        "schema_version": 1,
        "video": {
            "width": 99999,
            "height": 99999
        },
        "audio": {
            "master_volume": 1.0,
            "music_volume": 1.0,
            "sfx_volume": 1.0
        },
        "input": {}
    })";

    auto reader = [json_with_huge_resolution](const std::filesystem::path&) -> std::string {
        return json_with_huge_resolution;
    };

    auto writer = [](const std::filesystem::path&, const std::string&) -> bool {
        return true;
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    mgr.load();

    // Should be corrected to 1920x1080
    EXPECT_EQ(mgr.settings().video.width, 1920);
    EXPECT_EQ(mgr.settings().video.height, 1080);
}

TEST_F(SettingsManagerTest, ResolutionMinimumBoundary) {
    std::string json_with_min_resolution = R"({
        "schema_version": 1,
        "video": {
            "width": 640,
            "height": 480
        },
        "audio": {
            "master_volume": 1.0,
            "music_volume": 1.0,
            "sfx_volume": 1.0
        },
        "input": {}
    })";

    auto reader = [json_with_min_resolution](const std::filesystem::path&) -> std::string {
        return json_with_min_resolution;
    };

    auto writer = [](const std::filesystem::path&, const std::string&) -> bool {
        return true;
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    mgr.load();

    // Should be accepted
    EXPECT_EQ(mgr.settings().video.width, 640);
    EXPECT_EQ(mgr.settings().video.height, 480);
}

TEST_F(SettingsManagerTest, VolumeClampedToRange) {
    std::string json_with_invalid_volumes = R"({
        "schema_version": 1,
        "video": {
            "width": 1920,
            "height": 1080
        },
        "audio": {
            "master_volume": 1.5,
            "music_volume": -0.5,
            "sfx_volume": 2.0
        },
        "input": {}
    })";

    auto reader = [json_with_invalid_volumes](const std::filesystem::path&) -> std::string {
        return json_with_invalid_volumes;
    };

    auto writer = [](const std::filesystem::path&, const std::string&) -> bool {
        return true;
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    mgr.load();

    // Should be clamped
    EXPECT_FLOAT_EQ(mgr.settings().audio.master_volume, 1.0f);
    EXPECT_FLOAT_EQ(mgr.settings().audio.music_volume, 0.0f);
    EXPECT_FLOAT_EQ(mgr.settings().audio.sfx_volume, 1.0f);
}

TEST_F(SettingsManagerTest, MissingSectionUsesDefaults) {
    std::string json_missing_audio = R"({
        "schema_version": 1,
        "video": {
            "width": 1920,
            "height": 1080
        },
        "input": {}
    })";

    auto reader = [json_missing_audio](const std::filesystem::path&) -> std::string {
        return json_missing_audio;
    };

    auto writer = [](const std::filesystem::path&, const std::string&) -> bool {
        return true;
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    mgr.load();

    // Audio should use defaults
    EXPECT_FLOAT_EQ(mgr.settings().audio.master_volume, 1.0f);
    EXPECT_FLOAT_EQ(mgr.settings().audio.music_volume, 1.0f);
    EXPECT_FLOAT_EQ(mgr.settings().audio.sfx_volume, 1.0f);
}

TEST_F(SettingsManagerTest, SchemaVersionPreserved) {
    std::string json_with_schema = R"({
        "schema_version": 1,
        "video": {"width": 1920, "height": 1080},
        "audio": {"master_volume": 1.0, "music_volume": 1.0, "sfx_volume": 1.0},
        "input": {}
    })";

    auto reader = [json_with_schema](const std::filesystem::path&) -> std::string {
        return json_with_schema;
    };

    std::string saved_content;
    auto writer = [&saved_content](const std::filesystem::path&, const std::string& content) -> bool {
        saved_content = content;
        return true;
    };

    openitup::data::SettingsManager mgr(test_path_, reader, writer);
    mgr.load();

    EXPECT_EQ(mgr.settings().schema_version, 1);

    // Save and verify schema_version is preserved
    mgr.save();
    EXPECT_TRUE(saved_content.find("\"schema_version\": 1") != std::string::npos);
}

} // namespace
