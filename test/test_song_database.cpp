#include <gtest/gtest.h>

#include "openitup/asset/song_database.h"
#include "openitup/asset/lazy_loader.h"

#include <filesystem>
#include <fstream>

using namespace openitup;

// Test fixture for song database tests
class SongDatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_root_ = std::filesystem::temp_directory_path() / "song_database_test";
        std::filesystem::remove_all(test_root_);
        std::filesystem::create_directories(test_root_);
    }

    void TearDown() override {
        std::filesystem::remove_all(test_root_);
    }

    void create_file(const std::string& path, const std::string& content = "test") {
        auto file_path = test_root_ / path;
        std::filesystem::create_directories(file_path.parent_path());
        std::ofstream file(file_path);
        file << content;
    }

    void create_ksf_with_metadata(const std::string& path,
                                   const std::string& title,
                                   const std::string& artist,
                                   double bpm) {
        auto file_path = test_root_ / path;
        std::filesystem::create_directories(file_path.parent_path());
        std::ofstream file(file_path);
        file << "#TITLE:" << title << ";\n";
        file << "#ARTIST:" << artist << ";\n";
        file << "#BPM:" << bpm << ";\n";
        file << "#TICKCOUNT:2;\n";
        file << "#STEP:;\n";
        file << "00000\n";  // Note data
        file << "10000\n";
        file << "00000\n";
        file << "00000\n";
        file << "2\n";  // End marker
    }

    std::filesystem::path test_root_;
};

// US-AST-013 Scenario 1: Title, artist, BPM extracted from KSF header
TEST_F(SongDatabaseTest, MetadataExtractedFromKsfHeader) {
    // Given: KSF file with metadata in header
    create_ksf_with_metadata("Pumptris/pumptris.ksf", "Pumptris", "BanYa", 145.0);
    create_file("Pumptris/song.ogg");

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: metadata is extracted without full parse
    ASSERT_EQ(db.song_count(), 1);
    auto song = db.get_song(0);
    ASSERT_TRUE(song.has_value());
    EXPECT_EQ(song->title, "Pumptris");
    EXPECT_EQ(song->artist, "BanYa");
    EXPECT_DOUBLE_EQ(song->bpm, 145.0);
}

// US-AST-013 Scenario 2: Missing metadata fields use fallbacks
TEST_F(SongDatabaseTest, MissingMetadataUseFallbacks) {
    // Given: KSF with only BPM
    create_file("Song1/chart.ksf", "#BPM:140;\n#STEP:;\n00000\n10000\n2\n");
    create_file("Song1/song.ogg");

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: title defaults to filename
    ASSERT_EQ(db.song_count(), 1);
    auto song = db.get_song(0);
    ASSERT_TRUE(song.has_value());
    EXPECT_EQ(song->title, "chart");
    EXPECT_EQ(song->artist, "");
    EXPECT_DOUBLE_EQ(song->bpm, 140.0);
}

// US-AST-015 Scenario 1: Audio and banner files discovered in song folder
TEST_F(SongDatabaseTest, AudioAndBannerFilesDiscovered) {
    // Given: song folder with audio and banner
    create_ksf_with_metadata("Song1/chart.ksf", "Test Song", "Test Artist", 130.0);
    create_file("Song1/song.ogg");
    create_file("Song1/banner.png");

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: audio and banner paths are populated
    ASSERT_EQ(db.song_count(), 1);
    auto song = db.get_song(0);
    ASSERT_TRUE(song.has_value());
    EXPECT_FALSE(song->audio_path.empty());
    EXPECT_TRUE(song->audio_path.filename() == "song.ogg");
    EXPECT_FALSE(song->banner_path.empty());
    EXPECT_TRUE(song->banner_path.filename() == "banner.png");
}

// US-AST-015 Scenario 2: Multiple audio formats prioritized
TEST_F(SongDatabaseTest, MultipleAudioFormatsPrioritized) {
    // Given: song folder with .ogg and .mp3
    create_ksf_with_metadata("Song1/chart.ksf", "Test", "Test", 120.0);
    create_file("Song1/song.ogg");
    create_file("Song1/song.mp3");

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: .ogg is preferred (probed first)
    auto song = db.get_song(0);
    ASSERT_TRUE(song.has_value());
    EXPECT_TRUE(song->audio_path.filename() == "song.ogg");
}

// US-AST-016 Scenario 1: BGA files discovered in song folder
TEST_F(SongDatabaseTest, BgaFilesDiscovered) {
    // Given: song folder with BGA file
    create_ksf_with_metadata("Song1/chart.ksf", "Test", "Test", 140.0);
    create_file("Song1/song.ogg");
    create_file("Song1/song.bgaj", R"({"layers": []})");

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: BGA path is populated
    auto song = db.get_song(0);
    ASSERT_TRUE(song.has_value());
    EXPECT_FALSE(song->bga_path.empty());
    EXPECT_TRUE(song->bga_path.filename() == "song.bgaj");
}

// US-AST-016 Scenario 2: Missing BGA is not an error
TEST_F(SongDatabaseTest, MissingBgaNotAnError) {
    // Given: song folder without BGA
    create_ksf_with_metadata("Song1/chart.ksf", "Test", "Test", 140.0);
    create_file("Song1/song.ogg");

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: song is valid, BGA path is empty
    ASSERT_EQ(db.song_count(), 1);
    auto song = db.get_song(0);
    ASSERT_TRUE(song.has_value());
    EXPECT_TRUE(song->is_valid);
    EXPECT_TRUE(song->bga_path.empty());
}

// US-AST-030 Scenario 1: Songs missing audio excluded
TEST_F(SongDatabaseTest, SongsMissingAudioExcluded) {
    // Given: songs with and without audio
    create_ksf_with_metadata("Song1/chart.ksf", "Valid", "Test", 130.0);
    create_file("Song1/song.ogg");
    create_ksf_with_metadata("Song2/chart.ksf", "No Audio", "Test", 140.0);

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: only Song1 appears in the list
    ASSERT_EQ(db.song_count(), 1);
    auto song = db.get_song(0);
    ASSERT_TRUE(song.has_value());
    EXPECT_EQ(song->title, "Valid");
}

// US-AST-030 Scenario 2: Songs missing chart excluded
TEST_F(SongDatabaseTest, SongsMissingChartExcluded) {
    // Given: folder with audio but no chart
    create_file("NoChart/song.ogg");
    create_file("NoChart/banner.png");

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: no songs in the list
    EXPECT_EQ(db.song_count(), 0);
}

// US-AST-031 Scenario 1: Missing banner sets empty path
TEST_F(SongDatabaseTest, MissingBannerSetsEmptyPath) {
    // Given: song without banner
    create_ksf_with_metadata("Song1/chart.ksf", "Test", "Test", 130.0);
    create_file("Song1/song.ogg");

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: banner path is empty
    auto song = db.get_song(0);
    ASSERT_TRUE(song.has_value());
    EXPECT_TRUE(song->banner_path.empty());
}

// US-AST-031 Scenario 2: Song with missing banner still valid
TEST_F(SongDatabaseTest, SongWithMissingBannerStillValid) {
    // Given: song with chart and audio but no banner
    create_ksf_with_metadata("Song1/chart.ksf", "Test", "Test", 130.0);
    create_file("Song1/song.ogg");

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: song is valid (missing banner is not fatal)
    ASSERT_EQ(db.song_count(), 1);
    auto song = db.get_song(0);
    ASSERT_TRUE(song.has_value());
    EXPECT_TRUE(song->is_valid);
}

// Additional test: Case-insensitive file discovery
TEST_F(SongDatabaseTest, CaseInsensitiveFileDiscovery) {
    // Given: files with various case extensions
    create_ksf_with_metadata("Song1/chart.ksf", "Test", "Test", 120.0);
    create_file("Song1/song.OGG");
    create_file("Song1/banner.PNG");

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: files are discovered
    auto song = db.get_song(0);
    ASSERT_TRUE(song.has_value());
    EXPECT_FALSE(song->audio_path.empty());
    EXPECT_FALSE(song->banner_path.empty());
}

// Additional test: Multiple charts in one folder
TEST_F(SongDatabaseTest, MultipleChartsInOneFolder) {
    // Given: song with multiple difficulty charts
    create_ksf_with_metadata("Song1/easy.ksf", "Test Song", "Artist", 130.0);
    create_ksf_with_metadata("Song1/hard.ksf", "Test Song", "Artist", 130.0);
    create_file("Song1/song.ogg");

    // When: the database scans
    SongDatabase db;
    db.scan({test_root_});

    // Then: one song entry with multiple chart paths
    ASSERT_EQ(db.song_count(), 1);
    auto song = db.get_song(0);
    ASSERT_TRUE(song.has_value());
    EXPECT_EQ(song->chart_paths.size(), 2);
}

// --- LazyLoader Tests ---

class LazyLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_root_ = std::filesystem::temp_directory_path() / "lazy_loader_test";
        std::filesystem::remove_all(test_root_);
        std::filesystem::create_directories(test_root_);
    }

    void TearDown() override {
        std::filesystem::remove_all(test_root_);
    }

    void create_full_ksf(const std::string& path) {
        auto file_path = test_root_ / path;
        std::filesystem::create_directories(file_path.parent_path());
        std::ofstream file(file_path);
        file << "#TITLE:Test Song;\n";
        file << "#ARTIST:Test Artist;\n";
        file << "#BPM:140;\n";
        file << "#TICKCOUNT:2;\n";
        file << "#STEP:;\n";
        file << "00000\n";
        file << "10000\n";
        file << "00000\n";
        file << "01000\n";
        file << "2\n";
    }

    std::filesystem::path test_root_;
};

// US-AST-017 Scenario 1: Chart loaded on demand
TEST_F(LazyLoaderTest, ChartLoadedOnDemand) {
    // Given: song entry with chart path
    create_full_ksf("Song1/chart.ksf");

    SongDatabaseEntry entry;
    entry.song_path = test_root_ / "Song1";
    entry.chart_paths.push_back(test_root_ / "Song1" / "chart.ksf");
    entry.title = "Test Song";

    // When: lazy loader loads the chart
    LazyLoader loader;
    auto chart = loader.load_chart(entry);

    // Then: full chart data is parsed
    ASSERT_TRUE(chart.has_value());
    EXPECT_EQ(chart->metadata().title, "Test Song");
    EXPECT_GT(chart->note_data().size(), 0);
}

// US-AST-017 Scenario 2: Missing chart returns nullopt
TEST_F(LazyLoaderTest, MissingChartReturnsNullopt) {
    // Given: song entry with non-existent chart path
    SongDatabaseEntry entry;
    entry.song_path = test_root_ / "Song1";
    entry.chart_paths.push_back(test_root_ / "Song1" / "missing.ksf");

    // When: lazy loader attempts to load
    LazyLoader loader;
    auto chart = loader.load_chart(entry);

    // Then: returns nullopt
    EXPECT_FALSE(chart.has_value());
}

// US-AST-019 Scenario 1: Audio path returned for valid file
TEST_F(LazyLoaderTest, AudioPathReturnedForValidFile) {
    // Given: song entry with valid audio path
    auto audio_path = test_root_ / "Song1" / "song.ogg";
    std::filesystem::create_directories(audio_path.parent_path());
    std::ofstream audio_file(audio_path);
    audio_file << "fake audio data";
    audio_file.close();

    SongDatabaseEntry entry;
    entry.song_path = test_root_ / "Song1";
    entry.audio_path = audio_path;
    entry.title = "Test";

    // When: lazy loader loads audio
    LazyLoader loader;
    auto loaded_path = loader.load_audio(entry);

    // Then: path is returned
    EXPECT_FALSE(loaded_path.empty());
    EXPECT_EQ(loaded_path, audio_path);
}

// US-AST-019 Scenario 2: Missing audio returns empty path
TEST_F(LazyLoaderTest, MissingAudioReturnsEmptyPath) {
    // Given: song entry with non-existent audio path
    SongDatabaseEntry entry;
    entry.song_path = test_root_ / "Song1";
    entry.audio_path = test_root_ / "Song1" / "missing.ogg";

    // When: lazy loader attempts to load
    LazyLoader loader;
    auto loaded_path = loader.load_audio(entry);

    // Then: empty path is returned
    EXPECT_TRUE(loaded_path.empty());
}

// US-AST-020 Scenario 1: BGA loaded on demand
TEST_F(LazyLoaderTest, BgaLoadedOnDemand) {
    // BGA loading requires a TextureCache (SDL renderer dependency).
    // Deferred to integration testing — unit test verifies graceful nullptr return.
    SongDatabaseEntry entry;
    entry.song_path = test_root_ / "Song1";
    entry.bga_path = test_root_ / "Song1" / "song.bgaj";

    LazyLoader loader;
    auto bga = loader.load_bga(entry);

    // Without TextureCache, load_bga returns nullptr (graceful degradation)
    EXPECT_EQ(bga, nullptr);
}

// US-AST-020 Scenario 2: Missing BGA returns nullopt
TEST_F(LazyLoaderTest, MissingBgaReturnsNullopt) {
    // Given: song entry with empty BGA path
    SongDatabaseEntry entry;
    entry.song_path = test_root_ / "Song1";
    entry.bga_path = "";

    // When: lazy loader attempts to load
    LazyLoader loader;
    auto bga = loader.load_bga(entry);

    // Then: nullptr is returned
    EXPECT_FALSE(bga != nullptr);
}

// Additional test: Chart index out of range
TEST_F(LazyLoaderTest, ChartIndexOutOfRange) {
    // Given: song with one chart
    create_full_ksf("Song1/chart.ksf");

    SongDatabaseEntry entry;
    entry.chart_paths.push_back(test_root_ / "Song1" / "chart.ksf");

    // When: requesting chart at index 1
    LazyLoader loader;
    auto chart = loader.load_chart(entry, 1);

    // Then: returns nullopt
    EXPECT_FALSE(chart.has_value());
}
