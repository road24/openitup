#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include <openitup/asset/noteskin_manager.h>
#include <openitup/gfx/texture_cache.h>

using namespace openitup;

class NoteSkinManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary noteskin directory structure
        test_dir_ = std::filesystem::temp_directory_path() / "test_noteskin_manager";
        std::filesystem::create_directories(test_dir_);

        // Create some fake noteskin directories
        skin1_dir_ = test_dir_ / "default";
        skin2_dir_ = test_dir_ / "retro";
        skin3_dir_ = test_dir_ / "neon";

        std::filesystem::create_directories(skin1_dir_);
        std::filesystem::create_directories(skin2_dir_);
        std::filesystem::create_directories(skin3_dir_);

        // Create a non-directory file to test filtering
        std::ofstream(test_dir_ / "README.txt") << "Not a skin directory";
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    std::filesystem::path test_dir_;
    std::filesystem::path skin1_dir_;
    std::filesystem::path skin2_dir_;
    std::filesystem::path skin3_dir_;
};

// US-AST-024 Scenario 1: Discover all valid skins
TEST_F(NoteSkinManagerTest, DiscoverAllValidSkins) {
    NoteSkinManager manager(test_dir_);

    int count = manager.scan();

    EXPECT_EQ(count, 3);
    const auto& skins = manager.available_skins();
    EXPECT_EQ(skins.size(), 3);

    // Skins should be sorted alphabetically
    EXPECT_EQ(skins[0], "default");
    EXPECT_EQ(skins[1], "neon");
    EXPECT_EQ(skins[2], "retro");
}

// US-AST-025 Scenario 1: All valid skins discovered
TEST_F(NoteSkinManagerTest, AllValidSkinsDiscovered) {
    NoteSkinManager manager(test_dir_);
    manager.scan();

    EXPECT_TRUE(manager.has_skin("default"));
    EXPECT_TRUE(manager.has_skin("retro"));
    EXPECT_TRUE(manager.has_skin("neon"));
    EXPECT_FALSE(manager.has_skin("missing"));
}

// US-AST-024: Empty directory returns zero skins
TEST_F(NoteSkinManagerTest, EmptyDirectory) {
    std::filesystem::path empty_dir = test_dir_ / "empty";
    std::filesystem::create_directories(empty_dir);

    NoteSkinManager manager(empty_dir);
    int count = manager.scan();

    EXPECT_EQ(count, 0);
    EXPECT_TRUE(manager.available_skins().empty());
}

// US-AST-024: Non-existent directory returns zero skins
TEST_F(NoteSkinManagerTest, NonExistentDirectory) {
    std::filesystem::path missing_dir = test_dir_ / "does_not_exist";

    NoteSkinManager manager(missing_dir);
    int count = manager.scan();

    EXPECT_EQ(count, 0);
    EXPECT_TRUE(manager.available_skins().empty());
}

// US-AST-024: Check specific skin existence
TEST_F(NoteSkinManagerTest, HasSkin) {
    NoteSkinManager manager(test_dir_);
    manager.scan();

    EXPECT_TRUE(manager.has_skin("default"));
    EXPECT_TRUE(manager.has_skin("retro"));
    EXPECT_FALSE(manager.has_skin("nonexistent"));
}

// US-AST-024: Scan filters out non-directory entries
TEST_F(NoteSkinManagerTest, FiltersNonDirectories) {
    NoteSkinManager manager(test_dir_);
    manager.scan();

    const auto& skins = manager.available_skins();

    // Should not include README.txt
    EXPECT_FALSE(manager.has_skin("README.txt"));
    EXPECT_EQ(skins.size(), 3);  // Only the 3 directories
}

// US-AST-024: Load non-existent skin returns nullptr
TEST_F(NoteSkinManagerTest, LoadNonExistentSkin) {
    NoteSkinManager manager(test_dir_);
    manager.scan();

    // Create a minimal SDL context for TextureCache
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 640, 480, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    TextureCache cache(renderer, test_dir_);

    auto skin = manager.load_skin("nonexistent", cache);

    EXPECT_EQ(skin, nullptr);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// US-AST-024: Multiple scans work correctly
TEST_F(NoteSkinManagerTest, MultipleScan) {
    NoteSkinManager manager(test_dir_);

    int count1 = manager.scan();
    EXPECT_EQ(count1, 3);

    // Add another skin
    std::filesystem::create_directories(test_dir_ / "extra");

    int count2 = manager.scan();
    EXPECT_EQ(count2, 4);
    EXPECT_TRUE(manager.has_skin("extra"));
}
