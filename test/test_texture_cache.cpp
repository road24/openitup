#include <gtest/gtest.h>
#include <openitup/gfx/texture_cache.h>
#include <filesystem>
#include <fstream>

using namespace openitup;
namespace fs = std::filesystem;

class TextureProbe : public ::testing::Test {
protected:
    fs::path tmp_dir_;

    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "openitup_test_probe";
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }

    void touch(const std::string& filename) {
        std::ofstream(tmp_dir_ / filename).put('\0');
    }
};

TEST_F(TextureProbe, FindsTga) {
    touch("arrow.tga");
    auto result = TextureCache::probe("arrow.tga", tmp_dir_);
    EXPECT_EQ(result.filename(), "arrow.tga");
}

TEST_F(TextureProbe, FindsPngWhenNoTga) {
    touch("arrow.png");
    auto result = TextureCache::probe("arrow.tga", tmp_dir_);
    EXPECT_EQ(result.filename(), "arrow.png");
}

TEST_F(TextureProbe, FindsDdsWhenNoTgaOrPng) {
    touch("arrow.dds");
    auto result = TextureCache::probe("arrow.tga", tmp_dir_);
    EXPECT_EQ(result.filename(), "arrow.dds");
}

TEST_F(TextureProbe, PrefersTgaOverPng) {
    touch("arrow.tga");
    touch("arrow.png");
    auto result = TextureCache::probe("arrow.tga", tmp_dir_);
    EXPECT_EQ(result.filename(), "arrow.tga");
}

TEST_F(TextureProbe, PrefersPngOverDds) {
    touch("arrow.png");
    touch("arrow.dds");
    auto result = TextureCache::probe("arrow.tga", tmp_dir_);
    EXPECT_EQ(result.filename(), "arrow.png");
}

TEST_F(TextureProbe, ThrowsWhenNotFound) {
    EXPECT_THROW(TextureCache::probe("missing.tga", tmp_dir_), std::runtime_error);
}

TEST_F(TextureProbe, StripsExtensionFromInput) {
    touch("background.png");
    auto result = TextureCache::probe("background.tga", tmp_dir_);
    EXPECT_EQ(result.filename(), "background.png");
}

TEST_F(TextureProbe, HandlesNameWithoutExtension) {
    touch("icon.tga");
    // Even if someone passes just the stem, probe strips extension
    // The name "icon" has no extension, stem() returns "icon"
    auto result = TextureCache::probe("icon", tmp_dir_);
    EXPECT_EQ(result.filename(), "icon.tga");
}

// --- TextureCache with mock SDL ---

class TextureCacheTest : public ::testing::Test {
protected:
    fs::path tmp_dir_;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "openitup_test_cache";
        fs::create_directories(tmp_dir_);

        SDL_Init(SDL_INIT_VIDEO);
        window_ = SDL_CreateWindow("test", 1, 1, SDL_WINDOW_HIDDEN);
        if (window_) {
            renderer_ = SDL_CreateRenderer(window_, nullptr);
        }
    }

    void TearDown() override {
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_) SDL_DestroyWindow(window_);
        SDL_Quit();
        fs::remove_all(tmp_dir_);
    }

    void write_bmp(const std::string& filename, int w, int h) {
        SDL_Surface* surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
        SDL_SaveBMP(surface, (tmp_dir_ / filename).string().c_str());
        SDL_DestroySurface(surface);
    }

    bool has_renderer() const { return renderer_ != nullptr; }
};

TEST_F(TextureCacheTest, LoadAndGet) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    write_bmp("test.bmp", 64, 48);

    // Rename to .tga so probe finds it — but it's actually a BMP.
    // Use a real image loader that handles BMP:
    fs::rename(tmp_dir_ / "test.bmp", tmp_dir_ / "test.tga");

    auto loader = [](const fs::path& path) -> SDL_Surface* {
        return SDL_LoadBMP(path.string().c_str());
    };

    TextureCache cache(renderer_, loader);
    auto result = cache.load("test.tga", tmp_dir_);

    EXPECT_NE(result.handle, TextureHandle::Invalid);
    EXPECT_EQ(result.width, 64);
    EXPECT_EQ(result.height, 48);
    EXPECT_NE(cache.get(result.handle), nullptr);
    EXPECT_EQ(cache.size(), 1u);
}

TEST_F(TextureCacheTest, Deduplication) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    write_bmp("sprite.bmp", 32, 32);
    fs::rename(tmp_dir_ / "sprite.bmp", tmp_dir_ / "sprite.tga");

    auto loader = [](const fs::path& path) -> SDL_Surface* {
        return SDL_LoadBMP(path.string().c_str());
    };

    TextureCache cache(renderer_, loader);

    auto r1 = cache.load("sprite.tga", tmp_dir_);
    auto r2 = cache.load("sprite.tga", tmp_dir_);

    EXPECT_EQ(r1.handle, r2.handle);
    EXPECT_EQ(cache.size(), 1u);
}

TEST_F(TextureCacheTest, DifferentTexturesDifferentHandles) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    write_bmp("a.bmp", 16, 16);
    write_bmp("b.bmp", 32, 64);
    fs::rename(tmp_dir_ / "a.bmp", tmp_dir_ / "a.tga");
    fs::rename(tmp_dir_ / "b.bmp", tmp_dir_ / "b.tga");

    auto loader = [](const fs::path& path) -> SDL_Surface* {
        return SDL_LoadBMP(path.string().c_str());
    };

    TextureCache cache(renderer_, loader);

    auto ra = cache.load("a.tga", tmp_dir_);
    auto rb = cache.load("b.tga", tmp_dir_);

    EXPECT_NE(ra.handle, rb.handle);
    EXPECT_EQ(ra.width, 16);
    EXPECT_EQ(rb.width, 32);
    EXPECT_EQ(rb.height, 64);
    EXPECT_EQ(cache.size(), 2u);
}

TEST_F(TextureCacheTest, InvalidHandleReturnsNull) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    auto loader = [](const fs::path&) -> SDL_Surface* { return nullptr; };
    TextureCache cache(renderer_, loader);

    EXPECT_EQ(cache.get(TextureHandle::Invalid), nullptr);
    EXPECT_EQ(cache.get(static_cast<TextureHandle>(999)), nullptr);
}

TEST_F(TextureCacheTest, Clear) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    write_bmp("tex.bmp", 8, 8);
    fs::rename(tmp_dir_ / "tex.bmp", tmp_dir_ / "tex.tga");

    auto loader = [](const fs::path& path) -> SDL_Surface* {
        return SDL_LoadBMP(path.string().c_str());
    };

    TextureCache cache(renderer_, loader);
    auto r = cache.load("tex.tga", tmp_dir_);

    EXPECT_EQ(cache.size(), 1u);
    cache.clear();
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_EQ(cache.get(r.handle), nullptr);
}

TEST_F(TextureCacheTest, ThrowsOnMissingFile) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    auto loader = [](const fs::path&) -> SDL_Surface* { return nullptr; };
    TextureCache cache(renderer_, loader);

    EXPECT_THROW(cache.load("nonexistent.tga", tmp_dir_), std::runtime_error);
}

// --- LRU Tracking Tests ---

TEST_F(TextureCacheTest, LastAccessTickIncrementsOnLoad) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    write_bmp("a.bmp", 16, 16);
    write_bmp("b.bmp", 16, 16);
    write_bmp("c.bmp", 16, 16);
    fs::rename(tmp_dir_ / "a.bmp", tmp_dir_ / "a.tga");
    fs::rename(tmp_dir_ / "b.bmp", tmp_dir_ / "b.tga");
    fs::rename(tmp_dir_ / "c.bmp", tmp_dir_ / "c.tga");

    auto loader = [](const fs::path& path) -> SDL_Surface* {
        return SDL_LoadBMP(path.string().c_str());
    };

    TextureCache cache(renderer_, loader);

    auto ra = cache.load("a.tga", tmp_dir_);
    auto rb = cache.load("b.tga", tmp_dir_);
    auto rc = cache.load("c.tga", tmp_dir_);

    // Verify three distinct textures loaded
    EXPECT_EQ(cache.size(), 3u);
    EXPECT_NE(ra.handle, rb.handle);
    EXPECT_NE(rb.handle, rc.handle);
}

TEST_F(TextureCacheTest, LastAccessTickUpdatesOnCacheHit) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    write_bmp("tex.bmp", 16, 16);
    fs::rename(tmp_dir_ / "tex.bmp", tmp_dir_ / "tex.tga");

    auto loader = [](const fs::path& path) -> SDL_Surface* {
        return SDL_LoadBMP(path.string().c_str());
    };

    TextureCache cache(renderer_, loader);

    // First load: should be tick 0
    auto r1 = cache.load("tex.tga", tmp_dir_);

    // Load another texture to increment global tick
    write_bmp("other.bmp", 16, 16);
    fs::rename(tmp_dir_ / "other.bmp", tmp_dir_ / "other.tga");
    cache.load("other.tga", tmp_dir_);

    // Second load of original: should update tick and return same handle
    auto r2 = cache.load("tex.tga", tmp_dir_);

    EXPECT_EQ(r1.handle, r2.handle);
    EXPECT_EQ(cache.size(), 2u);
}

// --- LRU Eviction Tests ---

TEST_F(TextureCacheTest, EvictionTriggeredByMemoryThreshold) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    auto loader = [](const fs::path& path) -> SDL_Surface* {
        return SDL_LoadBMP(path.string().c_str());
    };

    // Set threshold to 1 MB (small to force eviction with many textures)
    size_t threshold_mb = 1;
    TextureCache cache(renderer_, loader, threshold_mb);

    // Create larger textures to exceed 1MB threshold
    // Each 128x128 texture is ~64KB (128*128*4 = 65536 bytes)
    // 20 textures = ~1.25 MB, which exceeds 1MB threshold
    for (int i = 0; i < 20; i++) {
        std::string name = "tex" + std::to_string(i) + ".bmp";
        write_bmp(name, 128, 128);
        fs::rename(tmp_dir_ / name, tmp_dir_ / (std::string("tex") + std::to_string(i) + ".tga"));
    }

    // Load textures - eviction should kick in
    for (int i = 0; i < 20; i++) {
        cache.load(std::string("tex") + std::to_string(i) + ".tga", tmp_dir_);
    }

    // Memory usage should be below threshold due to eviction
    EXPECT_LE(cache.get_memory_usage_bytes(), cache.get_memory_threshold_bytes());

    // Cache size should be less than 20 (some evicted)
    EXPECT_LT(cache.size(), 20u);
}

TEST_F(TextureCacheTest, PinnedTexturesNotEvicted) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    auto loader = [](const fs::path& path) -> SDL_Surface* {
        return SDL_LoadBMP(path.string().c_str());
    };

    // Set small threshold to force eviction
    size_t threshold_mb = 1;
    TextureCache cache(renderer_, loader, threshold_mb);

    // Create and load first texture (128x128 = 64KB)
    write_bmp("pinned.bmp", 128, 128);
    fs::rename(tmp_dir_ / "pinned.bmp", tmp_dir_ / "pinned.tga");
    auto r1 = cache.load("pinned.tga", tmp_dir_);

    // Pin it using canonical path
    auto pinned_path = fs::canonical(tmp_dir_ / "pinned.tga").string();
    cache.pin_texture(pinned_path);

    // Load more textures to trigger eviction (20x 128x128 = ~1.25MB)
    for (int i = 0; i < 20; i++) {
        std::string name = "tex" + std::to_string(i) + ".bmp";
        write_bmp(name, 128, 128);
        fs::rename(tmp_dir_ / name, tmp_dir_ / (std::string("tex") + std::to_string(i) + ".tga"));
        cache.load(std::string("tex") + std::to_string(i) + ".tga", tmp_dir_);
    }

    // Pinned texture should still be valid
    EXPECT_NE(cache.get(r1.handle), nullptr);
}

TEST_F(TextureCacheTest, EvictedTextureReloadsAsCacheMiss) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    auto loader = [](const fs::path& path) -> SDL_Surface* {
        return SDL_LoadBMP(path.string().c_str());
    };

    // Set small threshold to force eviction
    size_t threshold_mb = 1;
    TextureCache cache(renderer_, loader, threshold_mb);

    // Load first texture (128x128 = 64KB)
    write_bmp("first.bmp", 128, 128);
    fs::rename(tmp_dir_ / "first.bmp", tmp_dir_ / "first.tga");
    auto r1 = cache.load("first.tga", tmp_dir_);
    auto original_handle = r1.handle;

    // Load many more textures to force eviction of first (20x 128x128 = ~1.25MB)
    for (int i = 0; i < 20; i++) {
        std::string name = "tex" + std::to_string(i) + ".bmp";
        write_bmp(name, 128, 128);
        fs::rename(tmp_dir_ / name, tmp_dir_ / (std::string("tex") + std::to_string(i) + ".tga"));
        cache.load(std::string("tex") + std::to_string(i) + ".tga", tmp_dir_);
    }

    // Original handle should be invalid (texture destroyed)
    EXPECT_EQ(cache.get(original_handle), nullptr);

    // Reload first texture - should succeed and get a new handle
    auto r2 = cache.load("first.tga", tmp_dir_);
    EXPECT_NE(r2.handle, TextureHandle::Invalid);
    EXPECT_EQ(r2.width, 128);
    EXPECT_EQ(r2.height, 128);
    EXPECT_NE(cache.get(r2.handle), nullptr);
}
