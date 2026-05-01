#include <gtest/gtest.h>
#include <fstream>
#include <openitup/chart/chart_builder.h>
#include <openitup/chart/chart_loader.h>

using namespace openitup;

TEST(ChartLoader, IsSupportedExtension) {
    EXPECT_TRUE(ChartLoader::is_supported_extension(".ksf"));
    EXPECT_TRUE(ChartLoader::is_supported_extension(".KSF")); // Case insensitive
    EXPECT_TRUE(ChartLoader::is_supported_extension(".sma"));
    EXPECT_TRUE(ChartLoader::is_supported_extension(".see"));
    EXPECT_TRUE(ChartLoader::is_supported_extension(".osf"));
    EXPECT_FALSE(ChartLoader::is_supported_extension(".txt"));
    EXPECT_FALSE(ChartLoader::is_supported_extension(".mp3"));
}

TEST(ChartLoader, LoadsKsfFile) {
    // Create a temporary KSF file
    std::string ksf_content = R"(
#TITLE:Loader Test;
#BPM:140;
#TICKCOUNT:2;
#STEP:0;
10001
00100
)";

    // Write to temp file
    std::filesystem::path temp_path = std::filesystem::temp_directory_path() / "test_loader.ksf";
    std::ofstream file(temp_path);
    file << ksf_content;
    file.close();

    ChartLoader loader;
    auto charts = loader.load(temp_path);

    ASSERT_EQ(charts.size(), 1);
    EXPECT_EQ(charts[0].metadata().title, "Loader Test");

    std::filesystem::remove(temp_path);
}

TEST(ChartLoader, LoadsOsfFile) {
    std::string osf_content = R"({
  "version": "1.0",
  "metadata": {
    "title": "OSF Loader Test",
    "mode": "SINGLE"
  },
  "timing_events": [
    { "type": "BPM_CHANGE", "beat": 0.0, "bpm": 120.0 }
  ],
  "notes": [
    { "beat": 0.0, "column": 2, "type": "TAP" }
  ]
})";

    std::filesystem::path temp_path = std::filesystem::temp_directory_path() / "test_loader.osf";
    std::ofstream file(temp_path);
    file << osf_content;
    file.close();

    ChartLoader loader;
    auto charts = loader.load(temp_path);

    ASSERT_EQ(charts.size(), 1);
    EXPECT_EQ(charts[0].metadata().title, "OSF Loader Test");

    std::filesystem::remove(temp_path);
}

TEST(ChartLoader, ReturnsEmptyForSeeFile) {
    std::filesystem::path temp_path = std::filesystem::temp_directory_path() / "test_loader.see";
    std::ofstream file(temp_path);
    file << "encrypted data";
    file.close();

    ChartLoader loader;
    auto charts = loader.load(temp_path);

    // SEE parser returns empty vector (not yet supported)
    EXPECT_EQ(charts.size(), 0);

    std::filesystem::remove(temp_path);
}

TEST(ChartLoader, ThrowsOnNonexistentFile) {
    ChartLoader loader;
    EXPECT_THROW(loader.load("/nonexistent/file.ksf"), ChartLoadException);
}

TEST(ChartLoader, ReturnsEmptyForUnsupportedExtension) {
    std::filesystem::path temp_path = std::filesystem::temp_directory_path() / "test_loader.txt";
    std::ofstream file(temp_path);
    file << "some text";
    file.close();

    ChartLoader loader;
    auto charts = loader.load(temp_path);

    EXPECT_EQ(charts.size(), 0);

    std::filesystem::remove(temp_path);
}
