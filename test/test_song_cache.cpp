#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include "openitup/data/song_cache.h"

using namespace openitup::data;

namespace {

// Helper: create a temp directory for testing
std::filesystem::path create_temp_dir() {
    auto temp = std::filesystem::temp_directory_path() /
                ("test_song_cache_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(temp);
    return temp;
}

// Helper: remove temp directory
void remove_temp_dir(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
}

// Helper: create a valid cache JSON string
std::string create_cache_json(size_t entry_count = 1) {
    std::ostringstream oss;
    oss << R"({
  "schema_version": 1,
  "last_updated": "2026-04-26T10:00:00Z",
  "charts": [)";

    for (size_t i = 0; i < entry_count; ++i) {
        if (i > 0) oss << ",";
        oss << R"(
    {
      "path": "/songs/song)" << i << R"(/chart.ksf",
      "hash": "",
      "title": "Song )" << i << R"(",
      "artist": "Artist )" << i << R"(",
      "difficulties": [
        {
          "type": "single",
          "level": )" << (5 + i % 10) << R"(
        }
      ]
    })";
    }

    oss << R"(
  ]
})";
    return oss.str();
}

} // anonymous namespace

// US-DAT-031 SC1: Valid cache file with entries
TEST(SongCache, LoadValidCache) {
    auto temp_dir = create_temp_dir();
    auto cache_file = temp_dir / "cache.json";

    // Write a valid cache file
    std::ofstream out(cache_file);
    out << create_cache_json(1);
    out.close();

    SongCache cache(cache_file);
    bool loaded = cache.load();

    EXPECT_TRUE(loaded);
    EXPECT_EQ(cache.size(), 1);
    EXPECT_EQ(cache.entries()[0].path, "/songs/song0/chart.ksf");
    EXPECT_EQ(cache.entries()[0].title, "Song 0");
    EXPECT_EQ(cache.entries()[0].artist, "Artist 0");
    EXPECT_EQ(cache.entries()[0].difficulties.size(), 1);
    EXPECT_EQ(cache.entries()[0].difficulties[0].type, "single");
    EXPECT_EQ(cache.entries()[0].difficulties[0].level, 5);
    EXPECT_EQ(cache.last_updated(), "2026-04-26T10:00:00Z");

    remove_temp_dir(temp_dir);
}

// US-DAT-031 SC2: Corrupt cache is handled gracefully
TEST(SongCache, CorruptCacheReturnsFalse) {
    auto temp_dir = create_temp_dir();
    auto cache_file = temp_dir / "cache.json";

    // Write invalid JSON
    std::ofstream out(cache_file);
    out << "{ invalid json }";
    out.close();

    SongCache cache(cache_file);
    bool loaded = cache.load();

    EXPECT_FALSE(loaded);
    EXPECT_EQ(cache.size(), 0);

    remove_temp_dir(temp_dir);
}

// Missing cache file
TEST(SongCache, MissingCacheReturnsFalse) {
    auto temp_dir = create_temp_dir();
    auto cache_file = temp_dir / "nonexistent.json";

    SongCache cache(cache_file);
    bool loaded = cache.load();

    EXPECT_FALSE(loaded);
    EXPECT_EQ(cache.size(), 0);

    remove_temp_dir(temp_dir);
}

// Save creates valid JSON
TEST(SongCache, SaveWritesValidJson) {
    auto temp_dir = create_temp_dir();
    auto cache_file = temp_dir / "cache.json";

    SongCache cache(cache_file);

    ChartCacheEntry entry;
    entry.path = "/songs/test/chart.ksf";
    entry.hash = "";
    entry.title = "Test Song";
    entry.artist = "Test Artist";
    entry.difficulties.push_back({"single", 7});

    std::vector<ChartCacheEntry> entries = {entry};
    cache.set_entries(std::move(entries));

    bool saved = cache.save();
    EXPECT_TRUE(saved);
    EXPECT_TRUE(std::filesystem::exists(cache_file));

    // Verify we can load it back
    SongCache cache2(cache_file);
    bool loaded = cache2.load();
    EXPECT_TRUE(loaded);
    EXPECT_EQ(cache2.size(), 1);
    EXPECT_EQ(cache2.entries()[0].path, entry.path);
    EXPECT_EQ(cache2.entries()[0].title, entry.title);

    remove_temp_dir(temp_dir);
}

// US-DAT-032 SC1: Cache is fresh
TEST(SongCache, FreshCacheValid) {
    auto temp_dir = create_temp_dir();
    auto cache_file = temp_dir / "cache.json";
    auto song_dir = temp_dir / "songs";
    std::filesystem::create_directories(song_dir);

    // Create a song directory with old mtime
    std::ofstream dummy(song_dir / "dummy.txt");
    dummy << "test";
    dummy.close();

    // Sleep briefly to ensure the cache timestamp is newer
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create and save cache
    SongCache cache(cache_file);
    cache.set_entries({});
    cache.save();

    // Load and check freshness
    SongCache cache2(cache_file);
    cache2.load();

    std::vector<std::filesystem::path> song_dirs = {song_dir};
    bool fresh = cache2.is_fresh(song_dirs);

    EXPECT_TRUE(fresh);

    remove_temp_dir(temp_dir);
}

// US-DAT-032 SC2: Cache is stale
TEST(SongCache, StaleCacheInvalid) {
    auto temp_dir = create_temp_dir();
    auto cache_file = temp_dir / "cache.json";
    auto song_dir = temp_dir / "songs";
    std::filesystem::create_directories(song_dir);

    // Create and save cache first
    SongCache cache(cache_file);
    cache.set_entries({});
    cache.save();

    // Sleep to ensure mtime resolution difference (filesystem typically 1s granularity)
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    std::ofstream dummy(song_dir / "new_song.txt");
    dummy << "new content";
    dummy.close();

    // Explicitly update directory mtime to ensure staleness detection
    std::filesystem::last_write_time(song_dir, std::filesystem::file_time_type::clock::now());

    // Load and check freshness
    SongCache cache2(cache_file);
    cache2.load();

    std::vector<std::filesystem::path> song_dirs = {song_dir};
    bool fresh = cache2.is_fresh(song_dirs);

    EXPECT_FALSE(fresh);

    remove_temp_dir(temp_dir);
}

// Empty cache is valid
TEST(SongCache, EmptyCacheValid) {
    auto temp_dir = create_temp_dir();
    auto cache_file = temp_dir / "cache.json";

    std::ofstream out(cache_file);
    out << create_cache_json(0);
    out.close();

    SongCache cache(cache_file);
    bool loaded = cache.load();

    EXPECT_TRUE(loaded);
    EXPECT_EQ(cache.size(), 0);

    remove_temp_dir(temp_dir);
}

// Entry fields preserved through round-trip
TEST(SongCache, EntryFieldsPreserved) {
    auto temp_dir = create_temp_dir();
    auto cache_file = temp_dir / "cache.json";

    SongCache cache(cache_file);

    ChartCacheEntry entry;
    entry.path = "/songs/special/chart.ksf";
    entry.hash = "abc123def456";
    entry.title = "Special Song";
    entry.artist = "Special Artist";
    entry.difficulties.push_back({"single", 10});
    entry.difficulties.push_back({"double", 12});

    std::vector<ChartCacheEntry> entries = {entry};
    cache.set_entries(std::move(entries));
    cache.save();

    SongCache cache2(cache_file);
    cache2.load();

    ASSERT_EQ(cache2.size(), 1);
    const auto& loaded = cache2.entries()[0];
    EXPECT_EQ(loaded.path, entry.path);
    EXPECT_EQ(loaded.hash, entry.hash);
    EXPECT_EQ(loaded.title, entry.title);
    EXPECT_EQ(loaded.artist, entry.artist);
    ASSERT_EQ(loaded.difficulties.size(), 2);
    EXPECT_EQ(loaded.difficulties[0].type, "single");
    EXPECT_EQ(loaded.difficulties[0].level, 10);
    EXPECT_EQ(loaded.difficulties[1].type, "double");
    EXPECT_EQ(loaded.difficulties[1].level, 12);

    remove_temp_dir(temp_dir);
}

// get() method works
TEST(SongCache, GetReturnsEntry) {
    auto temp_dir = create_temp_dir();
    auto cache_file = temp_dir / "cache.json";

    SongCache cache(cache_file);

    ChartCacheEntry entry;
    entry.path = "/songs/test/chart.ksf";
    entry.title = "Test Song";

    cache.put(entry);

    auto result = cache.get("/songs/test/chart.ksf");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->title, "Test Song");

    auto missing = cache.get("/songs/nonexistent/chart.ksf");
    EXPECT_FALSE(missing.has_value());

    remove_temp_dir(temp_dir);
}

// put() updates existing entry
TEST(SongCache, PutUpdatesExistingEntry) {
    auto temp_dir = create_temp_dir();
    auto cache_file = temp_dir / "cache.json";

    SongCache cache(cache_file);

    ChartCacheEntry entry;
    entry.path = "/songs/test/chart.ksf";
    entry.title = "Original Title";

    cache.put(entry);
    EXPECT_EQ(cache.size(), 1);

    entry.title = "Updated Title";
    cache.put(entry);
    EXPECT_EQ(cache.size(), 1);  // Should not add duplicate

    auto result = cache.get("/songs/test/chart.ksf");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->title, "Updated Title");

    remove_temp_dir(temp_dir);
}

// US-DAT-033 SC1: Load 1000-entry cache under 1 second
TEST(SongCache, Load1000EntriesPerformance) {
    std::string large_cache = create_cache_json(1000);

    // Use injectable reader to isolate parse time from disk I/O
    auto reader = [&large_cache](const std::filesystem::path&) {
        return large_cache;
    };

    auto writer = [](const std::filesystem::path&, const std::string&) {
        return true;
    };

    SongCache cache("/fake/path.json", reader, writer);

    auto start = std::chrono::steady_clock::now();
    bool loaded = cache.load();
    auto end = std::chrono::steady_clock::now();

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();

    EXPECT_TRUE(loaded);
    EXPECT_EQ(cache.size(), 1000);
    EXPECT_LT(elapsed_ms, 1000) << "Load took " << elapsed_ms << "ms (expected < 1000ms)";

    // Conservative threshold: 200ms (TD suggests ~10-20ms on modern hardware)
    EXPECT_LT(elapsed_ms, 200) << "Load took " << elapsed_ms << "ms (expected < 200ms)";
}

// Injectable constructor for testing
TEST(SongCache, InjectableConstructor) {
    bool reader_called = false;
    bool writer_called = false;

    auto reader = [&reader_called](const std::filesystem::path&) {
        reader_called = true;
        return create_cache_json(1);
    };

    auto writer = [&writer_called](const std::filesystem::path&, const std::string&) {
        writer_called = true;
        return true;
    };

    SongCache cache("/fake/path.json", reader, writer);

    cache.load();
    EXPECT_TRUE(reader_called);

    cache.save();
    EXPECT_TRUE(writer_called);
}

// Nonexistent directory in freshness check invalidates cache
TEST(SongCache, NonexistentDirectoryInvalidatesCache) {
    auto temp_dir = create_temp_dir();
    auto cache_file = temp_dir / "cache.json";

    SongCache cache(cache_file);
    cache.set_entries({});
    cache.save();

    SongCache cache2(cache_file);
    cache2.load();

    std::vector<std::filesystem::path> song_dirs = {temp_dir / "nonexistent"};
    bool fresh = cache2.is_fresh(song_dirs);

    EXPECT_FALSE(fresh);

    remove_temp_dir(temp_dir);
}
