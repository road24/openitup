#include <gtest/gtest.h>

#include <openitup/bga/animation.h>
#include <openitup/bga/bga_loader.h>
#include <openitup/gfx/blend_modes.h>
#include <openitup/gfx/texture_cache.h>
#include <openitup/sprite/sprite_loader.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

using namespace openitup;
namespace fs = std::filesystem;

class IntegrationTest : public ::testing::Test {
protected:
    fs::path tmp_dir_;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "openitup_integration";
        fs::remove_all(tmp_dir_);
        fs::create_directories(tmp_dir_);

        SDL_Init(SDL_INIT_VIDEO);
        window_ = SDL_CreateWindow("test", 640, 480, SDL_WINDOW_HIDDEN);
        if (window_) {
            renderer_ = SDL_CreateRenderer(window_, nullptr);
        }
        if (renderer_) {
            SDL_SetRenderLogicalPresentation(renderer_, 640, 480,
                SDL_LOGICAL_PRESENTATION_LETTERBOX);
            init_blend_modes(renderer_);
        }
    }

    void TearDown() override {
        if (captured_) SDL_DestroySurface(captured_);
        captured_ = nullptr;
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_) SDL_DestroyWindow(window_);
        SDL_Quit();
        fs::remove_all(tmp_dir_);
    }

    bool has_renderer() const { return renderer_ != nullptr; }

    // Create a solid-color BMP texture and return its path
    fs::path make_texture(const std::string& name, int w, int h,
                          uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        SDL_Surface* surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
        uint32_t color = SDL_MapSurfaceRGBA(surface, r, g, b, a);
        SDL_FillSurfaceRect(surface, nullptr, color);
        auto path = tmp_dir_ / name;
        SDL_SaveBMP(surface, path.string().c_str());
        SDL_DestroySurface(surface);
        return path;
    }

    // Write JSON to a file
    void write_json(const std::string& filename, const nlohmann::json& j) {
        std::ofstream f(tmp_dir_ / filename);
        f << j.dump(2);
    }

    struct Pixel { uint8_t r, g, b, a; };

    // Render an animation to an offscreen target and capture the full surface.
    // Must be called before read_pixel().
    SDL_Surface* captured_ = nullptr;

    void render_frame(BgaAnimation& anim, TextureCache& cache, float tick) {
        if (captured_) { SDL_DestroySurface(captured_); captured_ = nullptr; }

        SDL_Texture* target = SDL_CreateTexture(
            renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 640, 480);
        SDL_SetRenderTarget(renderer_, target);
        SDL_SetRenderLogicalPresentation(renderer_, 640, 480,
            SDL_LOGICAL_PRESENTATION_DISABLED);
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
        SDL_RenderClear(renderer_);
        anim.render(renderer_, cache, tick, resolve_blend_mode);

        captured_ = SDL_RenderReadPixels(renderer_, nullptr);

        SDL_SetRenderTarget(renderer_, nullptr);
        SDL_DestroyTexture(target);
    }

    Pixel read_pixel(int x, int y) {
        Pixel px{};
        if (!captured_ || x < 0 || y < 0 ||
            x >= captured_->w || y >= captured_->h) return px;

        uint8_t* row = static_cast<uint8_t*>(captured_->pixels) + y * captured_->pitch;
        uint32_t raw = reinterpret_cast<uint32_t*>(row)[x];
        SDL_GetRGBA(raw,
            SDL_GetPixelFormatDetails(captured_->format),
            nullptr,
            &px.r, &px.g, &px.b, &px.a);
        return px;
    }
};

// Test: a single full-screen red sprite drawn at tick 0
TEST_F(IntegrationTest, SingleRedSprite) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    // texture: solid red 64x64 (will be stretched to 640x480 by the sprite rect)
    make_texture("red.tga", 64, 64, 255, 0, 0);

    write_json("red.sprj", {
        {"source_format", "spr"},
        {"mode", "tile"},
        {"pictures", {{
            {"texture", "red.tga"},
            {"rect", {0, 0, 640, 480}},
            {"uv", {0.0, 0.0, 1.0, 1.0}}
        }}}
    });

    write_json("test.bgaj", {
        {"version", 2},
        {"layers", {{
            {"sprite", "red.sprj"},
            {"keyframes", {
                {{"tick", 0}, {"translate", {0, 0}}, {"pivot", {0, 0}},
                 {"scale", {1, 1}}, {"rotate", 0},
                 {"color", {1, 1, 1, 1}}, {"display", true}, {"effect", "normal"}},
                {{"tick", 60}, {"translate", {0, 0}}, {"pivot", {0, 0}},
                 {"scale", {1, 1}}, {"rotate", 0},
                 {"color", {1, 1, 1, 1}}, {"display", true}, {"effect", "normal"}}
            }}
        }}}
    });

    TextureCache cache(renderer_, [](const fs::path& p) -> SDL_Surface* {
        return SDL_LoadBMP(p.string().c_str());
    });

    auto anim = load_bgaj(tmp_dir_ / "test.bgaj", cache);
    render_frame(*anim, cache, 0.0f);

    // Center pixel should be red
    auto px = read_pixel(320, 240);
    EXPECT_GE(px.r, 250);
    EXPECT_LE(px.g, 5);
    EXPECT_LE(px.b, 5);
}

// Test: layer invisible before its first keyframe tick
TEST_F(IntegrationTest, InvisibleBeforeFirstKeyframe) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    make_texture("green.tga", 64, 64, 0, 255, 0);

    write_json("green.sprj", {
        {"source_format", "spr"},
        {"mode", "tile"},
        {"pictures", {{
            {"texture", "green.tga"},
            {"rect", {0, 0, 640, 480}},
            {"uv", {0.0, 0.0, 1.0, 1.0}}
        }}}
    });

    write_json("test.bgaj", {
        {"version", 2},
        {"layers", {{
            {"sprite", "green.sprj"},
            {"keyframes", {
                {{"tick", 30}, {"translate", {0, 0}}, {"pivot", {0, 0}},
                 {"scale", {1, 1}}, {"rotate", 0},
                 {"color", {1, 1, 1, 1}}, {"display", true}, {"effect", "normal"}},
                {{"tick", 90}, {"translate", {0, 0}}, {"pivot", {0, 0}},
                 {"scale", {1, 1}}, {"rotate", 0},
                 {"color", {1, 1, 1, 1}}, {"display", true}, {"effect", "normal"}}
            }}
        }}}
    });

    TextureCache cache(renderer_, [](const fs::path& p) -> SDL_Surface* {
        return SDL_LoadBMP(p.string().c_str());
    });

    auto anim = load_bgaj(tmp_dir_ / "test.bgaj", cache);

    // At tick 10: before first keyframe → layer invisible → black screen
    render_frame(*anim, cache, 10.0f);
    auto px = read_pixel(320, 240);
    EXPECT_LE(px.r, 5);
    EXPECT_LE(px.g, 5);
    EXPECT_LE(px.b, 5);

    // At tick 60: between keyframes → layer visible → green
    render_frame(*anim, cache, 60.0f);
    px = read_pixel(320, 240);
    EXPECT_LE(px.r, 5);
    EXPECT_GE(px.g, 250);
    EXPECT_LE(px.b, 5);
}

// Test: alpha fade-in via keyframe color interpolation
TEST_F(IntegrationTest, AlphaFadeIn) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    make_texture("white.tga", 64, 64, 255, 255, 255);

    write_json("white.sprj", {
        {"source_format", "spr"},
        {"mode", "tile"},
        {"pictures", {{
            {"texture", "white.tga"},
            {"rect", {0, 0, 640, 480}},
            {"uv", {0.0, 0.0, 1.0, 1.0}}
        }}}
    });

    // White sprite fading in from alpha 0 to alpha 1 over ticks 0-60
    write_json("test.bgaj", {
        {"version", 2},
        {"layers", {{
            {"sprite", "white.sprj"},
            {"keyframes", {
                {{"tick", 0}, {"translate", {0, 0}}, {"pivot", {0, 0}},
                 {"scale", {1, 1}}, {"rotate", 0},
                 {"color", {1, 1, 1, 0}}, {"display", true}, {"effect", "normal"}},
                {{"tick", 60}, {"translate", {0, 0}}, {"pivot", {0, 0}},
                 {"scale", {1, 1}}, {"rotate", 0},
                 {"color", {1, 1, 1, 1}}, {"display", true}, {"effect", "normal"}}
            }}
        }}}
    });

    TextureCache cache(renderer_, [](const fs::path& p) -> SDL_Surface* {
        return SDL_LoadBMP(p.string().c_str());
    });

    auto anim = load_bgaj(tmp_dir_ / "test.bgaj", cache);

    // At tick 0: alpha=0, black background visible → pixel near black
    render_frame(*anim, cache, 0.0f);
    auto px0 = read_pixel(320, 240);
    EXPECT_LE(px0.r, 10);

    // At tick 30: alpha=0.5, white blended on black → pixel near mid-gray
    render_frame(*anim, cache, 30.0f);
    auto px30 = read_pixel(320, 240);
    EXPECT_GE(px30.r, 100);
    EXPECT_LE(px30.r, 200);

    // At tick 59: alpha near 1.0 → pixel near white
    render_frame(*anim, cache, 59.0f);
    auto px59 = read_pixel(320, 240);
    EXPECT_GE(px59.r, 230);
}

// Test: ANI mode selects correct frame based on dt
TEST_F(IntegrationTest, AniModeFrameSelection) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    // Two textures: red and blue, 64x64 each
    make_texture("frame_r.tga", 64, 64, 255, 0, 0);
    make_texture("frame_b.tga", 64, 64, 0, 0, 255);

    // ANI sprite with 2 frames: frame 0 = red, frame 1 = blue
    write_json("ani.sprj", {
        {"source_format", "spr"},
        {"mode", "ani"},
        {"pictures", {
            {{"texture", "frame_r.tga"},
             {"rect", {0, 0, 640, 480}},
             {"uv", {0.0, 0.0, 1.0, 1.0}}},
            {{"texture", "frame_b.tga"},
             {"rect", {0, 0, 640, 480}},
             {"uv", {0.0, 0.0, 1.0, 1.0}}}
        }}
    });

    // Keyframes span ticks 0-60: dt goes 0→1 over that interval
    write_json("test.bgaj", {
        {"version", 2},
        {"layers", {{
            {"sprite", "ani.sprj"},
            {"keyframes", {
                {{"tick", 0}, {"translate", {0, 0}}, {"pivot", {0, 0}},
                 {"scale", {1, 1}}, {"rotate", 0},
                 {"color", {1, 1, 1, 1}}, {"display", true}, {"effect", "normal"}},
                {{"tick", 60}, {"translate", {0, 0}}, {"pivot", {0, 0}},
                 {"scale", {1, 1}}, {"rotate", 0},
                 {"color", {1, 1, 1, 1}}, {"display", true}, {"effect", "normal"}}
            }}
        }}}
    });

    TextureCache cache(renderer_, [](const fs::path& p) -> SDL_Surface* {
        return SDL_LoadBMP(p.string().c_str());
    });

    auto anim = load_bgaj(tmp_dir_ / "test.bgaj", cache);

    // At tick 0: dt=0, frame=floor(2*0)=0 → red
    render_frame(*anim, cache, 0.0f);
    auto px = read_pixel(320, 240);
    EXPECT_GE(px.r, 250);
    EXPECT_LE(px.b, 5);

    // At tick 45: dt=0.75, frame=floor(2*0.75)=1 → blue
    render_frame(*anim, cache, 45.0f);
    px = read_pixel(320, 240);
    EXPECT_LE(px.r, 5);
    EXPECT_GE(px.b, 250);
}

// Test: two layers compositing (back red, front green small)
TEST_F(IntegrationTest, TwoLayerCompositing) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    make_texture("bg_red.tga", 64, 64, 255, 0, 0);
    make_texture("fg_green.tga", 64, 64, 0, 255, 0);

    write_json("bg.sprj", {
        {"source_format", "spr"},
        {"mode", "tile"},
        {"pictures", {{
            {"texture", "bg_red.tga"},
            {"rect", {0, 0, 640, 480}},
            {"uv", {0.0, 0.0, 1.0, 1.0}}
        }}}
    });

    // Green foreground covers only the center 100x100
    write_json("fg.sprj", {
        {"source_format", "spr"},
        {"mode", "tile"},
        {"pictures", {{
            {"texture", "fg_green.tga"},
            {"rect", {270, 190, 370, 290}},
            {"uv", {0.0, 0.0, 1.0, 1.0}}
        }}}
    });

    auto make_kfs = []() {
        return nlohmann::json::array({
            {{"tick", 0}, {"translate", {0, 0}}, {"pivot", {0, 0}},
             {"scale", {1, 1}}, {"rotate", 0},
             {"color", {1, 1, 1, 1}}, {"display", true}, {"effect", "normal"}},
            {{"tick", 60}, {"translate", {0, 0}}, {"pivot", {0, 0}},
             {"scale", {1, 1}}, {"rotate", 0},
             {"color", {1, 1, 1, 1}}, {"display", true}, {"effect", "normal"}}
        });
    };

    write_json("test.bgaj", {
        {"version", 2},
        {"layers", {
            {{"sprite", "bg.sprj"}, {"keyframes", make_kfs()}},
            {{"sprite", "fg.sprj"}, {"keyframes", make_kfs()}}
        }}
    });

    TextureCache cache(renderer_, [](const fs::path& p) -> SDL_Surface* {
        return SDL_LoadBMP(p.string().c_str());
    });

    auto anim = load_bgaj(tmp_dir_ / "test.bgaj", cache);
    render_frame(*anim, cache, 30.0f);

    // Center (320, 240) should be green (foreground on top)
    auto center = read_pixel(320, 240);
    EXPECT_LE(center.r, 5);
    EXPECT_GE(center.g, 250);

    // Corner (10, 10) should be red (only background)
    auto corner = read_pixel(10, 10);
    EXPECT_GE(corner.r, 250);
    EXPECT_LE(corner.g, 5);
}
