#include <gtest/gtest.h>

#include "openitup/asset/data_dir_config.h"
#include "openitup/asset/song_scanner.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

using namespace openitup;

// Test fixture for song scanner tests
class SongScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a unique temp directory for this test
        test_root_ = std::filesystem::temp_directory_path() / "song_scanner_test";
        std::filesystem::remove_all(test_root_);
        std::filesystem::create_directories(test_root_);
    }

    void TearDown() override {
        // Clean up temp directory
        std::filesystem::remove_all(test_root_);
    }

    // Helper to create a directory structure
    void create_directory(const std::string& path) {
        std::filesystem::create_directories(test_root_ / path);
    }

    // Helper to create a file
    void create_file(const std::string& path, const std::string& content = "test") {
        auto file_path = test_root_ / path;
        std::filesystem::create_directories(file_path.parent_path());
        std::ofstream file(file_path);
        file << content;
    }

    std::filesystem::path test_root_;
};

// US-AST-012 Scenario 1: Nested song folders discovered
TEST_F(SongScannerTest, NestedSongFoldersDiscovered) {
    // Given: nested directory structure with songs
    create_file("exceed/Pumptris/pumptris.ksf");
    create_file("exceed/Pumptris/pumptris.ogg");
    create_file("nx/Sorceress/sorceress.ksf");
    create_file("nx/Sorceress/sorceress.ogg");

    // When: the engine scans the root
    SongScanner scanner;
    auto songs = scanner.scan_directory(test_root_);

    // Then: the song database contains 2 entries
    ASSERT_EQ(songs.size(), 2);

    // Verify song names (sorted for stable comparison)
    std::vector<std::string> song_names;
    for (const auto& song : songs) {
        song_names.push_back(song.path.filename().string());
    }
    std::sort(song_names.begin(), song_names.end());

    EXPECT_EQ(song_names[0], "Pumptris");
    EXPECT_EQ(song_names[1], "Sorceress");
}

// US-AST-012 Scenario 2: Multiple chart files in one folder
TEST_F(SongScannerTest, MultipleChartFilesInOneFolder) {
    // Given: song folder with multiple chart files
    create_file("Pumptris/pumptris.ksf");
    create_file("Pumptris/pumptris_hard.ksf");
    create_file("Pumptris/pumptris.ogg");

    // When: the engine scans
    SongScanner scanner;
    auto songs = scanner.scan_directory(test_root_);

    // Then: one song entry with 2 chart files
    ASSERT_EQ(songs.size(), 1);
    EXPECT_EQ(songs[0].path.filename().string(), "Pumptris");
    EXPECT_EQ(songs[0].ksf_files.size(), 2);
}

// US-AST-012 Scenario 3: Directories without charts skipped
TEST_F(SongScannerTest, DirectoriesWithoutChartsSkipped) {
    // Given: mixed directory structure
    create_file("songs/Pumptris/pumptris.ksf");
    create_file("videos/intro.mp4");
    create_directory("system");

    // When: the engine scans
    SongScanner scanner;
    auto songs = scanner.scan_directory(test_root_);

    // Then: only the songs directory contributes entries
    ASSERT_EQ(songs.size(), 1);
    EXPECT_EQ(songs[0].path.filename().string(), "Pumptris");
}

// Additional test: Hidden directories are skipped
TEST_F(SongScannerTest, HiddenDirectoriesSkipped) {
    // Given: hidden directory with a chart
    create_file("normal_song/song.ksf");
    create_file(".hidden_song/hidden.ksf");

    // When: the engine scans
    SongScanner scanner;
    auto songs = scanner.scan_directory(test_root_);

    // Then: only normal_song is found
    ASSERT_EQ(songs.size(), 1);
    EXPECT_EQ(songs[0].path.filename().string(), "normal_song");
}

// Additional test: Empty directory returns no songs
TEST_F(SongScannerTest, EmptyDirectoryReturnsNoSongs) {
    // Given: empty directory
    create_directory("empty");

    // When: the engine scans
    SongScanner scanner;
    auto songs = scanner.scan_directory(test_root_);

    // Then: no songs found
    EXPECT_EQ(songs.size(), 0);
}

// Additional test: Non-existent directory handled gracefully
TEST_F(SongScannerTest, NonExistentDirectoryHandledGracefully) {
    // Given: non-existent path
    auto non_existent = test_root_ / "does_not_exist";

    // When: the engine scans
    SongScanner scanner;
    auto songs = scanner.scan_directory(non_existent);

    // Then: returns empty vector, does not crash
    EXPECT_EQ(songs.size(), 0);
}

// Additional test: Case-insensitive extension matching
TEST_F(SongScannerTest, CaseInsensitiveExtensionMatching) {
    // Given: chart files with various case extensions
    create_file("Song1/chart.ksf");
    create_file("Song2/chart.KSF");
    create_file("Song3/chart.Ksf");

    // When: the engine scans
    SongScanner scanner;
    auto songs = scanner.scan_directory(test_root_);

    // Then: all three songs are found
    EXPECT_EQ(songs.size(), 3);
}

// --- DataDirConfig Tests ---

class DataDirConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_root_ = std::filesystem::temp_directory_path() / "data_dir_config_test";
        std::filesystem::remove_all(test_root_);
        std::filesystem::create_directories(test_root_);
    }

    void TearDown() override {
        std::filesystem::remove_all(test_root_);
    }

    std::filesystem::path test_root_;
};

// US-AST-011 Scenario 1: Multiple directories scanned
TEST_F(DataDirConfigTest, MultipleDirectoriesScanned) {
    // Given: config file with multiple directories
    auto dir1 = test_root_ / "exceed";
    auto dir2 = test_root_ / "nx";
    std::filesystem::create_directories(dir1);
    std::filesystem::create_directories(dir2);

    auto config_path = test_root_ / "settings.json";
    std::ofstream config_file(config_path);
    config_file << R"({
        "data_dirs": [
            { "path": ")" << dir1.string() << R"(", "enabled": true },
            { "path": ")" << dir2.string() << R"(", "enabled": true }
        ]
    })";
    config_file.close();

    // When: the engine starts
    DataDirConfig config(test_root_ / "default");
    config.load_from_file(config_path);

    // Then: both directories are in the enabled list
    auto enabled = config.get_enabled_directories();
    EXPECT_EQ(enabled.size(), 3);  // default + 2 from config
}

// US-AST-011 Scenario 2: Invalid directory logged but not fatal
TEST_F(DataDirConfigTest, InvalidDirectoryLoggedButNotFatal) {
    // Given: config with one valid and one missing directory
    auto valid_dir = test_root_ / "valid";
    auto missing_dir = test_root_ / "missing";
    auto another_dir = test_root_ / "another";
    std::filesystem::create_directories(valid_dir);
    std::filesystem::create_directories(another_dir);

    auto config_path = test_root_ / "settings.json";
    std::ofstream config_file(config_path);
    config_file << R"({
        "data_dirs": [
            { "path": ")" << valid_dir.string() << R"(", "enabled": true },
            { "path": ")" << missing_dir.string() << R"(", "enabled": true },
            { "path": ")" << another_dir.string() << R"(", "enabled": true }
        ]
    })";
    config_file.close();

    // When: the engine starts
    DataDirConfig config(test_root_ / "default");
    bool loaded = config.load_from_file(config_path);

    // Then: valid and another are included, missing is skipped
    EXPECT_TRUE(loaded);
    auto enabled = config.get_enabled_directories();
    EXPECT_EQ(enabled.size(), 3);  // default + valid + another (missing skipped)
}

// US-AST-011 Scenario 3: Empty config array uses fallback
TEST_F(DataDirConfigTest, EmptyConfigArrayUsesFallback) {
    // Given: config with empty data_dirs array
    auto default_dir = test_root_ / "default";
    std::filesystem::create_directories(default_dir);

    auto config_path = test_root_ / "settings.json";
    std::ofstream config_file(config_path);
    config_file << R"({ "data_dirs": [] })";
    config_file.close();

    // When: the engine starts
    DataDirConfig config(default_dir);
    config.load_from_file(config_path);

    // Then: only default directory is used
    auto enabled = config.get_enabled_directories();
    EXPECT_EQ(enabled.size(), 1);
    EXPECT_EQ(enabled[0], std::filesystem::absolute(default_dir));
}

// Additional test: Disabled directories are not included
TEST_F(DataDirConfigTest, DisabledDirectoriesNotIncluded) {
    // Given: config with enabled and disabled directories
    auto enabled_dir = test_root_ / "enabled";
    auto disabled_dir = test_root_ / "disabled";
    std::filesystem::create_directories(enabled_dir);
    std::filesystem::create_directories(disabled_dir);

    auto config_path = test_root_ / "settings.json";
    std::ofstream config_file(config_path);
    config_file << R"({
        "data_dirs": [
            { "path": ")" << enabled_dir.string() << R"(", "enabled": true },
            { "path": ")" << disabled_dir.string() << R"(", "enabled": false }
        ]
    })";
    config_file.close();

    // When: loaded
    DataDirConfig config(test_root_ / "default");
    config.load_from_file(config_path);

    // Then: only enabled directory is in the list
    auto enabled = config.get_enabled_directories();
    EXPECT_EQ(enabled.size(), 2);  // default + enabled (disabled excluded)
}

// Additional test: Missing config file returns false but doesn't crash
TEST_F(DataDirConfigTest, MissingConfigFileHandledGracefully) {
    // Given: non-existent config file
    auto non_existent = test_root_ / "does_not_exist.json";

    // When: loading is attempted
    DataDirConfig config(test_root_ / "default");
    bool loaded = config.load_from_file(non_existent);

    // Then: returns false, default still works
    EXPECT_FALSE(loaded);
    auto enabled = config.get_enabled_directories();
    EXPECT_EQ(enabled.size(), 1);  // Only default
}

// Additional test: Invalid JSON handled gracefully
TEST_F(DataDirConfigTest, InvalidJsonHandledGracefully) {
    // Given: malformed JSON config
    auto config_path = test_root_ / "settings.json";
    std::ofstream config_file(config_path);
    config_file << "{ invalid json }";
    config_file.close();

    // When: loading is attempted
    DataDirConfig config(test_root_ / "default");
    bool loaded = config.load_from_file(config_path);

    // Then: returns false, logs warning, default still works
    EXPECT_FALSE(loaded);
    auto enabled = config.get_enabled_directories();
    EXPECT_EQ(enabled.size(), 1);
}
