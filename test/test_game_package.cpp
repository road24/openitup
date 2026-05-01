#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "openitup/lua/game_package.h"

namespace fs = std::filesystem;

class GamePackageTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "openitup_test_game_packages";
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }

    // Helper: create a minimal valid game package
    void create_minimal_package(const std::string& name) {
        fs::path game_dir = test_dir_ / name;
        fs::create_directories(game_dir);
        fs::create_directories(game_dir / "screens");

        // Write manifest.lua
        std::ofstream manifest(game_dir / "manifest.lua");
        manifest << "game = {\n";
        manifest << "    name = \"" << name << "\",\n";
        manifest << "    version = \"1.0\",\n";
        manifest << "    judge_profile = \"judge.json\"\n";
        manifest << "}\n";
        manifest.close();

        // Write judge profile
        std::ofstream judge(game_dir / "judge.json");
        judge << "{\"name\": \"test\", \"windows_ms\": {\"perfect\": 21}}\n";
        judge.close();

        // Write a screen
        std::ofstream screen(game_dir / "screens" / "title.lua");
        screen << "function on_enter(params) end\n";
        screen << "function update(dt) end\n";
        screen << "function render() end\n";
        screen << "function on_exit() end\n";
        screen.close();
    }

    fs::path test_dir_;
};

TEST_F(GamePackageTest, LoadValidPackage) {
    create_minimal_package("test_game");

    auto package = openitup::GamePackage::load(test_dir_, "test_game");
    ASSERT_NE(package, nullptr);
    EXPECT_EQ(package->manifest().name, "test_game");
    EXPECT_EQ(package->manifest().version, "1.0");
    EXPECT_EQ(package->manifest().judge_profile, "judge.json");
}

TEST_F(GamePackageTest, LoadNonexistentDirectory) {
    auto package = openitup::GamePackage::load(test_dir_, "nonexistent");
    EXPECT_EQ(package, nullptr);
}

TEST_F(GamePackageTest, MissingManifest) {
    fs::path game_dir = test_dir_ / "no_manifest";
    fs::create_directories(game_dir);

    auto package = openitup::GamePackage::load(test_dir_, "no_manifest");
    EXPECT_EQ(package, nullptr);
}

TEST_F(GamePackageTest, ManifestMissingRequiredFields) {
    fs::path game_dir = test_dir_ / "incomplete";
    fs::create_directories(game_dir);

    // Manifest missing 'judge_profile' field
    std::ofstream manifest(game_dir / "manifest.lua");
    manifest << "game = {\n";
    manifest << "    name = \"incomplete\",\n";
    manifest << "    version = \"1.0\"\n";
    manifest << "}\n";
    manifest.close();

    auto package = openitup::GamePackage::load(test_dir_, "incomplete");
    EXPECT_EQ(package, nullptr);
}

TEST_F(GamePackageTest, MissingJudgeProfile) {
    fs::path game_dir = test_dir_ / "no_judge";
    fs::create_directories(game_dir);

    std::ofstream manifest(game_dir / "manifest.lua");
    manifest << "game = {\n";
    manifest << "    name = \"no_judge\",\n";
    manifest << "    version = \"1.0\",\n";
    manifest << "    judge_profile = \"nonexistent.json\"\n";
    manifest << "}\n";
    manifest.close();

    auto package = openitup::GamePackage::load(test_dir_, "no_judge");
    EXPECT_EQ(package, nullptr);  // Validation fails without judge profile
}

TEST_F(GamePackageTest, OptionalFieldsDefaultValues) {
    create_minimal_package("defaults");

    auto package = openitup::GamePackage::load(test_dir_, "defaults");
    ASSERT_NE(package, nullptr);
    EXPECT_EQ(package->manifest().asset_dir, "assets/");
    EXPECT_EQ(package->manifest().initial_scene, "boot");
    EXPECT_EQ(package->manifest().author, "");
    EXPECT_EQ(package->manifest().base_year, 0);
}

TEST_F(GamePackageTest, OptionalFieldsCustomValues) {
    fs::path game_dir = test_dir_ / "custom";
    fs::create_directories(game_dir);
    fs::create_directories(game_dir / "screens");

    std::ofstream manifest(game_dir / "manifest.lua");
    manifest << "game = {\n";
    manifest << "    name = \"custom\",\n";
    manifest << "    version = \"2.0\",\n";
    manifest << "    judge_profile = \"judge.json\",\n";
    manifest << "    asset_dir = \"data/\",\n";
    manifest << "    initial_scene = \"title\",\n";
    manifest << "    author = \"TestAuthor\",\n";
    manifest << "    description = \"Test game\",\n";
    manifest << "    base_year = 2003\n";
    manifest << "}\n";
    manifest.close();

    std::ofstream judge(game_dir / "judge.json");
    judge << "{\"name\": \"test\"}\n";
    judge.close();

    auto package = openitup::GamePackage::load(test_dir_, "custom");
    ASSERT_NE(package, nullptr);
    EXPECT_EQ(package->manifest().asset_dir, "data/");
    EXPECT_EQ(package->manifest().initial_scene, "title");
    EXPECT_EQ(package->manifest().author, "TestAuthor");
    EXPECT_EQ(package->manifest().description, "Test game");
    EXPECT_EQ(package->manifest().base_year, 2003);
}

TEST_F(GamePackageTest, ScreenPathResolution) {
    create_minimal_package("paths");

    auto package = openitup::GamePackage::load(test_dir_, "paths");
    ASSERT_NE(package, nullptr);

    fs::path title_path = package->screen_path("title");
    EXPECT_EQ(title_path, test_dir_ / "paths" / "screens" / "title.lua");
    EXPECT_TRUE(fs::exists(title_path));
}

TEST_F(GamePackageTest, AssetPathResolution) {
    create_minimal_package("assets");

    auto package = openitup::GamePackage::load(test_dir_, "assets");
    ASSERT_NE(package, nullptr);

    fs::path sprite_path = package->asset_path("ui/button.sprj");
    EXPECT_EQ(sprite_path, test_dir_ / "assets" / "assets" / "ui" / "button.sprj");
}

TEST_F(GamePackageTest, JudgeProfilePathResolution) {
    create_minimal_package("judge");

    auto package = openitup::GamePackage::load(test_dir_, "judge");
    ASSERT_NE(package, nullptr);

    fs::path judge_path = package->judge_profile_path();
    EXPECT_EQ(judge_path, test_dir_ / "judge" / "judge.json");
    EXPECT_TRUE(fs::exists(judge_path));
}

TEST_F(GamePackageTest, HasScreen) {
    create_minimal_package("screens");

    auto package = openitup::GamePackage::load(test_dir_, "screens");
    ASSERT_NE(package, nullptr);

    EXPECT_TRUE(package->has_screen("title"));
    EXPECT_FALSE(package->has_screen("nonexistent"));
}

TEST_F(GamePackageTest, MalformedManifestLua) {
    fs::path game_dir = test_dir_ / "malformed";
    fs::create_directories(game_dir);

    std::ofstream manifest(game_dir / "manifest.lua");
    manifest << "game = { invalid syntax\n";
    manifest.close();

    auto package = openitup::GamePackage::load(test_dir_, "malformed");
    EXPECT_EQ(package, nullptr);
}
