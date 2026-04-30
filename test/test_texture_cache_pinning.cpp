#include <gtest/gtest.h>
#include <openitup/gfx/texture_cache.h>
#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>

using namespace openitup;
namespace fs = std::filesystem;

// Minimal mock image loader for testing
static SDL_Surface* mock_loader(const fs::path& path) {
    // Create a minimal 16x16 RGBA surface
    return SDL_CreateSurface(16, 16, SDL_PIXELFORMAT_RGBA8888);
}

class TextureCachePinning : public ::testing::Test {
protected:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    fs::path tmp_dir_;

    void SetUp() override {
        // Initialize SDL for texture creation
        ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));

        // Create window (required for renderer)
        window_ = SDL_CreateWindow("Test", 640, 480, SDL_WINDOW_HIDDEN);
        ASSERT_NE(window_, nullptr);

        renderer_ = SDL_CreateRenderer(window_, nullptr);
        ASSERT_NE(renderer_, nullptr);

        // Create temp directory for test textures
        tmp_dir_ = fs::temp_directory_path() / "openitup_test_pinning";
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        if (renderer_) {
            SDL_DestroyRenderer(renderer_);
        }
        if (window_) {
            SDL_DestroyWindow(window_);
        }
        SDL_Quit();
        fs::remove_all(tmp_dir_);
    }

    void touch(const std::string& filename) {
        std::ofstream(tmp_dir_ / filename).put('\0');
    }
};

TEST_F(TextureCachePinning, PinExistingTexture) {
    touch("test.tga");
    TextureCache cache(renderer_, mock_loader);

    auto result = cache.load("test.tga", tmp_dir_);
    EXPECT_NE(result.handle, TextureHandle::Invalid);

    // Pin the texture using the canonical path
    auto canonical_path = fs::canonical(tmp_dir_ / "test.tga").string();
    cache.pin_texture(canonical_path);

    // Verify texture is still accessible (pinning doesn't break access)
    EXPECT_NE(cache.get(result.handle), nullptr);
}

TEST_F(TextureCachePinning, PinNonExistentTextureLogsWarning) {
    TextureCache cache(renderer_, mock_loader);

    // This should log a warning but not crash
    cache.pin_texture("/nonexistent/path.tga");

    // Cache should be empty
    EXPECT_EQ(cache.size(), 0);
}

TEST_F(TextureCachePinning, PinBeforeLoadHasNoEffect) {
    touch("test.tga");
    TextureCache cache(renderer_, mock_loader);

    auto canonical_path = fs::canonical(tmp_dir_ / "test.tga").string();
    cache.pin_texture(canonical_path);  // Pin before loading

    // Load the texture
    auto result = cache.load("test.tga", tmp_dir_);
    EXPECT_NE(result.handle, TextureHandle::Invalid);

    // Texture is loaded but not pinned (pinning before load has no effect)
    EXPECT_EQ(cache.size(), 1);
}

TEST_F(TextureCachePinning, MultiplePins) {
    touch("texture1.tga");
    touch("texture2.tga");
    TextureCache cache(renderer_, mock_loader);

    auto result1 = cache.load("texture1.tga", tmp_dir_);
    auto result2 = cache.load("texture2.tga", tmp_dir_);

    auto path1 = fs::canonical(tmp_dir_ / "texture1.tga").string();
    auto path2 = fs::canonical(tmp_dir_ / "texture2.tga").string();

    cache.pin_texture(path1);
    cache.pin_texture(path2);

    // Both textures should still be accessible
    EXPECT_NE(cache.get(result1.handle), nullptr);
    EXPECT_NE(cache.get(result2.handle), nullptr);
    EXPECT_EQ(cache.size(), 2);
}

TEST_F(TextureCachePinning, ClearRemovesPinnedTextures) {
    touch("test.tga");
    TextureCache cache(renderer_, mock_loader);

    auto result = cache.load("test.tga", tmp_dir_);
    auto canonical_path = fs::canonical(tmp_dir_ / "test.tga").string();
    cache.pin_texture(canonical_path);

    // Clear should remove even pinned textures
    cache.clear();
    EXPECT_EQ(cache.size(), 0);
}
