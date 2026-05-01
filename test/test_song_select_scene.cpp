#include <gtest/gtest.h>

#include <filesystem>
#include <memory>

#include <openitup/asset/song_database.h>
#include <openitup/gfx/renderer.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>
#include <openitup/scene/song_select_scene.h>

using namespace openitup;

// Mock/test fixture for SongSelectScene
class SongSelectSceneTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create minimal test songs
        SongDatabaseEntry song1;
        song1.title = "Test Song 1";
        song1.artist = "Artist 1";
        song1.bpm = 140.0;
        song1.song_path = "/fake/path/song1";
        song1.chart_paths.push_back("/fake/path/song1/chart.ksf");
        song1.audio_path = "/fake/path/song1/audio.ogg";
        song1.is_valid = true;

        SongDatabaseEntry song2;
        song2.title = "Test Song 2";
        song2.artist = "Artist 2";
        song2.bpm = 160.0;
        song2.song_path = "/fake/path/song2";
        song2.chart_paths.push_back("/fake/path/song2/chart1.ksf");
        song2.chart_paths.push_back("/fake/path/song2/chart2.ksf");
        song2.audio_path = "/fake/path/song2/audio.ogg";
        song2.is_valid = true;

        test_songs_.push_back(song1);
        test_songs_.push_back(song2);
    }

    std::vector<SongDatabaseEntry> test_songs_;
};

// US-SCN-006 Scenario 1: Music wheel displays all songs
TEST_F(SongSelectSceneTest, ConstructorAcceptsSongList) {
    // Given: song list with 2 songs
    // When: scene is constructed
    auto scene = std::make_unique<SongSelectScene>(
        nullptr,  // renderer
        nullptr,  // text_renderer
        nullptr,  // scene_stack
        nullptr,  // engine
        GameMode::SINGLE,
        test_songs_
    );

    // Then: scene is created without error
    EXPECT_NE(scene, nullptr);
}

// US-SCN-006: Scene lifecycle
TEST_F(SongSelectSceneTest, LifecycleCallsDoNotCrash) {
    auto scene = std::make_unique<SongSelectScene>(
        nullptr, nullptr, nullptr, nullptr,
        GameMode::SINGLE,
        test_songs_
    );

    // Exercise lifecycle
    scene->on_enter();
    scene->update(0.016);
    scene->on_exit();

    SUCCEED();
}

// US-SCN-006: Scene accepts empty song list
TEST_F(SongSelectSceneTest, HandlesEmptySongList) {
    std::vector<SongDatabaseEntry> empty_songs;

    auto scene = std::make_unique<SongSelectScene>(
        nullptr, nullptr, nullptr, nullptr,
        GameMode::SINGLE,
        empty_songs
    );

    // Exercise lifecycle with empty list
    scene->on_enter();
    scene->update(0.016);
    scene->on_exit();

    SUCCEED();
}
