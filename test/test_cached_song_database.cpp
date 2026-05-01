#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include <openitup/asset/cached_song_database.h>

using namespace openitup;

// Test fixture for CachedSongDatabase
class CachedSongDatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directory
        test_root_ = std::filesystem::temp_directory_path() / "cached_song_db_test";
        std::filesystem::create_directories(test_root_);

        cache_path_ = test_root_ / "test_cache.json";
        data_dir_ = test_root_ / "data";
        std::filesystem::create_directories(data_dir_);
    }

    void TearDown() override {
        // Clean up test directory
        if (std::filesystem::exists(test_root_)) {
            std::filesystem::remove_all(test_root_);
        }
    }

    // Helper: create a minimal KSF file
    void create_minimal_ksf(const std::filesystem::path& path) {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path);
        file << "#TITLE:Test Song;\n";
        file << "#ARTIST:Test Artist;\n";
        file << "#BPM:140;\n";
        file << "0000000000\n";  // One empty measure
        file.close();
    }

    // Helper: create audio file
    void create_audio_file(const std::filesystem::path& path) {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path);
        file << "fake audio data";
        file.close();
    }

    std::filesystem::path test_root_;
    std::filesystem::path cache_path_;
    std::filesystem::path data_dir_;
};

// US-AST-014 Scenario 1: Cold start scans and caches
TEST_F(CachedSongDatabaseTest, ColdStartScansDirectories) {
    // Given: no cache exists, data directory has one song
    auto song_dir = data_dir_ / "TestSong";
    create_minimal_ksf(song_dir / "chart.ksf");
    create_audio_file(song_dir / "song.ogg");

    // When: database is initialized
    CachedSongDatabase db;
    bool result = db.initialize(cache_path_, {data_dir_});

    // Then: database contains the song
    EXPECT_TRUE(result);
    EXPECT_EQ(db.song_count(), 1);

    // And: cache file was created
    EXPECT_TRUE(std::filesystem::exists(cache_path_));
}

// US-AST-014 Scenario 2: Warm start loads cache
TEST_F(CachedSongDatabaseTest, WarmStartLoadsCache) {
    // Given: cache exists from previous scan
    auto song_dir = data_dir_ / "TestSong";
    create_minimal_ksf(song_dir / "chart.ksf");
    create_audio_file(song_dir / "song.ogg");

    // First initialization creates cache
    CachedSongDatabase db1;
    db1.initialize(cache_path_, {data_dir_});
    ASSERT_TRUE(std::filesystem::exists(cache_path_));

    // When: second initialization with same data dirs
    CachedSongDatabase db2;
    bool result = db2.initialize(cache_path_, {data_dir_});

    // Then: database is loaded successfully
    EXPECT_TRUE(result);
    EXPECT_EQ(db2.song_count(), 1);
}

// US-AST-014: Empty directory returns empty database
TEST_F(CachedSongDatabaseTest, EmptyDirectoryReturnsEmpty) {
    // Given: data directory exists but has no songs
    // (data_dir_ was created in SetUp)

    // When: database is initialized
    CachedSongDatabase db;
    bool result = db.initialize(cache_path_, {data_dir_});

    // Then: database is empty (result can be false since no songs)
    EXPECT_EQ(db.song_count(), 0);
}

// US-AST-014: Multiple songs are discovered
TEST_F(CachedSongDatabaseTest, MultipleSongsDiscovered) {
    // Given: data directory has multiple songs
    auto song1_dir = data_dir_ / "Song1";
    create_minimal_ksf(song1_dir / "chart.ksf");
    create_audio_file(song1_dir / "song.ogg");

    auto song2_dir = data_dir_ / "Song2";
    create_minimal_ksf(song2_dir / "chart.ksf");
    create_audio_file(song2_dir / "song.ogg");

    // When: database is initialized
    CachedSongDatabase db;
    db.initialize(cache_path_, {data_dir_});

    // Then: both songs are in database
    EXPECT_EQ(db.song_count(), 2);
}

// US-AST-014: get_song returns correct song
TEST_F(CachedSongDatabaseTest, GetSongReturnsValidEntry) {
    // Given: database with one song
    auto song_dir = data_dir_ / "TestSong";
    create_minimal_ksf(song_dir / "chart.ksf");
    create_audio_file(song_dir / "song.ogg");

    CachedSongDatabase db;
    db.initialize(cache_path_, {data_dir_});

    // When: retrieving song at index 0
    auto song = db.get_song(0);

    // Then: song is returned
    ASSERT_TRUE(song.has_value());
    EXPECT_EQ(song->title, "Test Song");
}

// US-AST-014: get_song out of bounds returns nullopt
TEST_F(CachedSongDatabaseTest, GetSongOutOfBoundsReturnsNullopt) {
    // Given: empty database
    CachedSongDatabase db;
    db.initialize(cache_path_, {data_dir_});

    // When: retrieving song at invalid index
    auto song = db.get_song(0);

    // Then: nullopt is returned
    EXPECT_FALSE(song.has_value());
}
