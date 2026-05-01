#include <gtest/gtest.h>
#include <openitup/core/system_asset_manager.h>
#include <openitup/sprite/sprite.h>
#include <openitup/bga/animation.h>
#include <openitup/gfx/texture_cache.h>
#include <openitup/judge/judgment_tier.h>
#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>

using namespace openitup;
using namespace openitup::core;
namespace fs = std::filesystem;

// Minimal mock image loader for testing
static SDL_Surface* mock_loader(const fs::path& path) {
    return SDL_CreateSurface(16, 16, SDL_PIXELFORMAT_RGBA8888);
}

class SystemAssetManagerTest : public ::testing::Test {
protected:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    std::unique_ptr<TextureCache> texture_cache_;
    fs::path tmp_dir_;
    fs::path system_dir_;

    void SetUp() override {
        // Initialize SDL
        ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
        window_ = SDL_CreateWindow("Test", 640, 480, SDL_WINDOW_HIDDEN);
        ASSERT_NE(window_, nullptr);
        renderer_ = SDL_CreateRenderer(window_, nullptr);
        ASSERT_NE(renderer_, nullptr);

        texture_cache_ = std::make_unique<TextureCache>(renderer_, mock_loader);

        // Create temp system directory structure
        tmp_dir_ = fs::temp_directory_path() / "openitup_test_sysasset";
        system_dir_ = tmp_dir_ / "data" / "system";
        fs::create_directories(system_dir_ / "sprites" / "textures");
        fs::create_directories(system_dir_ / "animations");
        fs::create_directories(system_dir_ / "sfx");

        // Create test sprite JSON
        create_test_sprite("test_sprite");
        create_test_texture("test_texture.tga");
    }

    void TearDown() override {
        texture_cache_.reset();
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_) SDL_DestroyWindow(window_);
        SDL_Quit();
        fs::remove_all(tmp_dir_);
    }

    void create_test_sprite(const std::string& name) {
        std::string json = R"({
            "source_format": "spr",
            "mode": "tile",
            "pictures": [
                {
                    "texture": "test_texture.tga",
                    "rect": [0, 0, 64, 64],
                    "uv": [0.0, 0.0, 1.0, 1.0]
                }
            ]
        })";
        std::ofstream(system_dir_ / "sprites" / (name + ".sprj")) << json;
    }

    void create_test_texture(const std::string& name) {
        std::ofstream(system_dir_ / "sprites" / name).put('\0');
    }

    void create_test_animation(const std::string& name) {
        // Create a minimal valid BGAJ with one layer and one keyframe
        std::string json = R"({
            "version": 2,
            "layers": [{
                "sprite": "../sprites/test_sprite.sprj",
                "keyframes": [
                    {"tick":0, "translate":[0,0], "pivot":[0,0], "scale":[1,1], "rotate":0, "color":[1,1,1,1], "display":true, "effect":"normal"},
                    {"tick":100, "translate":[0,0], "pivot":[0,0], "scale":[1,1], "rotate":0, "color":[1,1,1,1], "display":true, "effect":"normal"}
                ]
            }]
        })";
        std::ofstream(system_dir_ / "animations" / (name + ".bgaj")) << json;
    }

    void create_test_sfx(const std::string& filename) {
        // Create empty placeholder file
        std::ofstream(system_dir_ / "sfx" / filename).put('\0');
    }
};

TEST_F(SystemAssetManagerTest, CreateSucceedsWithValidSystemDir) {
    auto mgr = SystemAssetManager::create(system_dir_, {});
    ASSERT_NE(mgr, nullptr);
    EXPECT_EQ(mgr->system_dir(), fs::canonical(system_dir_));
}

TEST_F(SystemAssetManagerTest, CreateFailsWithInvalidSystemDir) {
    auto nonexistent = tmp_dir_ / "nonexistent";
    auto mgr = SystemAssetManager::create(nonexistent, {});
    EXPECT_EQ(mgr, nullptr);
}

TEST_F(SystemAssetManagerTest, LoadSpriteSuccess) {
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());
    auto sprite = mgr.get_sprite("test_sprite");
    ASSERT_NE(sprite, nullptr);
    EXPECT_EQ(sprite->pictures.size(), 1);
}

TEST_F(SystemAssetManagerTest, LoadSpriteCaches) {
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());

    auto sprite1 = mgr.get_sprite("test_sprite");
    auto sprite2 = mgr.get_sprite("test_sprite");

    ASSERT_NE(sprite1, nullptr);
    ASSERT_NE(sprite2, nullptr);
    // Should return the same shared_ptr
    EXPECT_EQ(sprite1.get(), sprite2.get());
}

TEST_F(SystemAssetManagerTest, LoadMissingSpriteReturnsNull) {
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());
    auto sprite = mgr.get_sprite("nonexistent");
    EXPECT_EQ(sprite, nullptr);
}

TEST_F(SystemAssetManagerTest, HasSpriteDetectsExistence) {
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());

    EXPECT_TRUE(mgr.has_sprite("test_sprite"));
    EXPECT_FALSE(mgr.has_sprite("nonexistent"));
}

TEST_F(SystemAssetManagerTest, HasSpriteReturnsTrueAfterCaching) {
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());

    // Load sprite to cache it
    auto sprite = mgr.get_sprite("test_sprite");
    ASSERT_NE(sprite, nullptr);

    // has_sprite should still return true
    EXPECT_TRUE(mgr.has_sprite("test_sprite"));
}

TEST_F(SystemAssetManagerTest, LoadAnimationSuccess) {
    create_test_animation("test_anim");
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());

    auto anim = mgr.get_animation("test_anim");
    ASSERT_NE(anim, nullptr);
    EXPECT_GT(anim->max_tick(), 0.0f);
}

TEST_F(SystemAssetManagerTest, LoadAnimationCaches) {
    create_test_animation("test_anim");
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());

    auto anim1 = mgr.get_animation("test_anim");
    auto anim2 = mgr.get_animation("test_anim");

    ASSERT_NE(anim1, nullptr);
    ASSERT_NE(anim2, nullptr);
    // Should return the same shared_ptr
    EXPECT_EQ(anim1.get(), anim2.get());
}

TEST_F(SystemAssetManagerTest, LoadMissingAnimationReturnsNull) {
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());
    auto anim = mgr.get_animation("nonexistent");
    EXPECT_EQ(anim, nullptr);  // Animations are optional
}

TEST_F(SystemAssetManagerTest, LoadWithoutTextureCacheReturnsNull) {
    auto mgr = SystemAssetManager(system_dir_, nullptr);  // No texture cache

    auto sprite = mgr.get_sprite("test_sprite");
    EXPECT_EQ(sprite, nullptr);
}

TEST_F(SystemAssetManagerTest, TextRendererAvailableWhenRendererProvided) {
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get(), renderer_);

    auto text_renderer = mgr.get_text_renderer();
    EXPECT_NE(text_renderer, nullptr);
}

TEST_F(SystemAssetManagerTest, TextRendererNullWhenRendererNotProvided) {
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get(), nullptr);

    auto text_renderer = mgr.get_text_renderer();
    EXPECT_EQ(text_renderer, nullptr);
}

// US-AST-023: Judgment and menu sound effects asset discovery

TEST_F(SystemAssetManagerTest, GetJudgmentSfxPathReturnsValidPath) {
    create_test_sfx("Perfect.wav");
    create_test_sfx("Great.wav");
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());

    auto perfect_path = mgr.get_judgment_sfx_path(JudgmentTier::PERFECT);
    auto great_path = mgr.get_judgment_sfx_path(JudgmentTier::GREAT);

    EXPECT_FALSE(perfect_path.empty());
    EXPECT_TRUE(fs::exists(perfect_path));
    EXPECT_EQ(perfect_path.filename(), "Perfect.wav");

    EXPECT_FALSE(great_path.empty());
    EXPECT_TRUE(fs::exists(great_path));
    EXPECT_EQ(great_path.filename(), "Great.wav");
}

TEST_F(SystemAssetManagerTest, GetJudgmentSfxPathAllTiers) {
    create_test_sfx("Perfect.wav");
    create_test_sfx("Great.wav");
    create_test_sfx("Good.wav");
    create_test_sfx("Bad.wav");
    create_test_sfx("Miss.wav");
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());

    EXPECT_FALSE(mgr.get_judgment_sfx_path(JudgmentTier::PERFECT).empty());
    EXPECT_FALSE(mgr.get_judgment_sfx_path(JudgmentTier::GREAT).empty());
    EXPECT_FALSE(mgr.get_judgment_sfx_path(JudgmentTier::GOOD).empty());
    EXPECT_FALSE(mgr.get_judgment_sfx_path(JudgmentTier::BAD).empty());
    EXPECT_FALSE(mgr.get_judgment_sfx_path(JudgmentTier::MISS).empty());
}

TEST_F(SystemAssetManagerTest, GetJudgmentSfxPathReturnsEmptyWhenNotFound) {
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());

    // No SFX files created, should return empty path (graceful degradation)
    auto perfect_path = mgr.get_judgment_sfx_path(JudgmentTier::PERFECT);
    EXPECT_TRUE(perfect_path.empty());
}

TEST_F(SystemAssetManagerTest, GetMenuSfxPathReturnsValidPath) {
    create_test_sfx("cursor.wav");
    create_test_sfx("select.wav");
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());

    auto cursor_path = mgr.get_menu_sfx_path("cursor");
    auto select_path = mgr.get_menu_sfx_path("select.wav");

    EXPECT_FALSE(cursor_path.empty());
    EXPECT_TRUE(fs::exists(cursor_path));
    EXPECT_EQ(cursor_path.filename(), "cursor.wav");

    EXPECT_FALSE(select_path.empty());
    EXPECT_TRUE(fs::exists(select_path));
    EXPECT_EQ(select_path.filename(), "select.wav");
}

TEST_F(SystemAssetManagerTest, GetMenuSfxPathReturnsEmptyWhenNotFound) {
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());

    // No SFX files created, should return empty path (graceful degradation)
    auto missing_path = mgr.get_menu_sfx_path("missing");
    EXPECT_TRUE(missing_path.empty());
}

TEST_F(SystemAssetManagerTest, GetMenuSfxPathAddsExtension) {
    create_test_sfx("back.wav");
    auto mgr = SystemAssetManager(system_dir_, texture_cache_.get());

    // Should add .wav extension if not present
    auto path_without_ext = mgr.get_menu_sfx_path("back");
    EXPECT_FALSE(path_without_ext.empty());
    EXPECT_EQ(path_without_ext.filename(), "back.wav");

    // Should not double-add extension
    auto path_with_ext = mgr.get_menu_sfx_path("back.wav");
    EXPECT_FALSE(path_with_ext.empty());
    EXPECT_EQ(path_with_ext.filename(), "back.wav");
}
