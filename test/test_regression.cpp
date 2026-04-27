#include <gtest/gtest.h>

#include <openitup/bga/animation.h>
#include <openitup/bga/bga_loader.h>
#include <openitup/gfx/blend_modes.h>
#include <openitup/gfx/image_loader.h>
#include <openitup/gfx/texture_cache.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>

using namespace openitup;
namespace fs = std::filesystem;

static fs::path fixtures_dir() {
    const char* env = std::getenv("OPENITUP_FIXTURES_DIR");
    if (env) return fs::path(env);
    return fs::path(__FILE__).parent_path() / "fixtures";
}

class RegressionTest : public ::testing::Test {
protected:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Surface* captured_ = nullptr;
    fs::path fixtures_;

    void SetUp() override {
        fixtures_ = fixtures_dir();
        SDL_Init(SDL_INIT_VIDEO);
        window_ = SDL_CreateWindow("regression", 640, 480, SDL_WINDOW_HIDDEN);
        if (window_) renderer_ = SDL_CreateRenderer(window_, nullptr);
        if (renderer_) init_blend_modes(renderer_);
    }

    void TearDown() override {
        if (captured_) SDL_DestroySurface(captured_);
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_) SDL_DestroyWindow(window_);
        SDL_Quit();
    }

    bool has_renderer() const { return renderer_ != nullptr; }

    void render_frame(const std::string& bgaj_name, float tick) {
        if (captured_) { SDL_DestroySurface(captured_); captured_ = nullptr; }

        TextureCache cache(renderer_, load_image);
        auto anim = load_bgaj(fixtures_ / bgaj_name, cache);

        SDL_Texture* target = SDL_CreateTexture(
            renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 640, 480);
        SDL_SetRenderTarget(renderer_, target);
        SDL_SetRenderLogicalPresentation(renderer_, 640, 480,
            SDL_LOGICAL_PRESENTATION_DISABLED);
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
        SDL_RenderClear(renderer_);
        anim->render(renderer_, cache, tick, resolve_blend_mode);

        captured_ = SDL_RenderReadPixels(renderer_, nullptr);
        SDL_SetRenderTarget(renderer_, nullptr);
        SDL_DestroyTexture(target);
    }

    struct Pixel { uint8_t r, g, b, a; };

    Pixel read_pixel(int x, int y) {
        Pixel px{};
        if (!captured_ || x < 0 || y < 0 ||
            x >= captured_->w || y >= captured_->h) return px;
        uint8_t* row = static_cast<uint8_t*>(captured_->pixels) + y * captured_->pitch;
        uint32_t raw = reinterpret_cast<uint32_t*>(row)[x];
        SDL_GetRGBA(raw, SDL_GetPixelFormatDetails(captured_->format),
                    nullptr, &px.r, &px.g, &px.b, &px.a);
        return px;
    }

    SDL_Surface* load_reference(const std::string& name) {
        auto path = fixtures_ / "reference" / name;
        return IMG_Load(path.string().c_str());
    }

    Pixel read_ref_pixel(SDL_Surface* ref, int x, int y) {
        Pixel px{};
        if (!ref || x < 0 || y < 0 || x >= ref->w || y >= ref->h) return px;
        SDL_Surface* converted = SDL_ConvertSurface(ref, SDL_PIXELFORMAT_RGBA32);
        if (!converted) return px;
        uint8_t* row = static_cast<uint8_t*>(converted->pixels) + y * converted->pitch;
        uint32_t raw = reinterpret_cast<uint32_t*>(row)[x];
        SDL_GetRGBA(raw, SDL_GetPixelFormatDetails(converted->format),
                    nullptr, &px.r, &px.g, &px.b, &px.a);
        SDL_DestroySurface(converted);
        return px;
    }

    // Compare rendered frame against reference PNG. Returns max per-channel diff.
    // Samples a grid of pixels rather than every pixel for speed.
    int compare_against_reference(const std::string& ref_name, int step = 4) {
        SDL_Surface* ref = load_reference(ref_name);
        if (!ref) {
            ADD_FAILURE() << "Could not load reference: " << ref_name;
            return 999;
        }
        SDL_Surface* converted = SDL_ConvertSurface(ref, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(ref);
        if (!converted) {
            ADD_FAILURE() << "Could not convert reference surface";
            return 999;
        }

        int max_diff = 0;
        for (int y = 0; y < 480; y += step) {
            for (int x = 0; x < 640; x += step) {
                auto got = read_pixel(x, y);

                uint8_t* row = static_cast<uint8_t*>(converted->pixels) + y * converted->pitch;
                uint32_t raw = reinterpret_cast<uint32_t*>(row)[x];
                Pixel exp{};
                SDL_GetRGBA(raw, SDL_GetPixelFormatDetails(converted->format),
                            nullptr, &exp.r, &exp.g, &exp.b, &exp.a);

                int dr = std::abs(got.r - exp.r);
                int dg = std::abs(got.g - exp.g);
                int db = std::abs(got.b - exp.b);
                int da = std::abs(got.a - exp.a);
                int d = std::max({dr, dg, db, da});
                if (d > max_diff) max_diff = d;
            }
        }
        SDL_DestroySurface(converted);
        return max_diff;
    }

    void assert_matches_reference(const std::string& ref_name, int tolerance = 2) {
        int diff = compare_against_reference(ref_name);
        EXPECT_LE(diff, tolerance)
            << "Rendered frame differs from reference '" << ref_name
            << "' by " << diff << " (tolerance: " << tolerance << ")";
    }
};

// ---- Test 1: translate interpolation ----

TEST_F(RegressionTest, TranslateInterpolation) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_translate.bgaj", 0.0f);
    assert_matches_reference("translate_t0.png");

    render_frame("reg_translate.bgaj", 50.0f);
    assert_matches_reference("translate_t50.png");

    render_frame("reg_translate.bgaj", 99.0f);
    assert_matches_reference("translate_t99.png");
}

// ---- Test 2: scale interpolation ----

TEST_F(RegressionTest, ScaleInterpolation) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_scale.bgaj", 0.0f);
    assert_matches_reference("scale_t0.png");

    render_frame("reg_scale.bgaj", 50.0f);
    assert_matches_reference("scale_t50.png");
}

// ---- Test 3: rotate interpolation ----

TEST_F(RegressionTest, RotateInterpolation) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_rotate.bgaj", 0.0f);
    assert_matches_reference("rotate_t0.png");

    render_frame("reg_rotate.bgaj", 50.0f);
    assert_matches_reference("rotate_t50.png");
}

// ---- Test 4: color RGB interpolation ----

TEST_F(RegressionTest, ColorRGBInterpolation) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_color_rgb.bgaj", 0.0f);
    assert_matches_reference("color_rgb_t0.png");

    render_frame("reg_color_rgb.bgaj", 50.0f);
    assert_matches_reference("color_rgb_t50.png");

    render_frame("reg_color_rgb.bgaj", 99.0f);
    assert_matches_reference("color_rgb_t99.png");
}

// ---- Test 5: color alpha interpolation ----

TEST_F(RegressionTest, ColorAlphaInterpolation) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_color_alpha.bgaj", 0.0f);
    assert_matches_reference("color_alpha_t0.png");

    render_frame("reg_color_alpha.bgaj", 25.0f);
    assert_matches_reference("color_alpha_t25.png");

    render_frame("reg_color_alpha.bgaj", 50.0f);
    assert_matches_reference("color_alpha_t50.png");

    render_frame("reg_color_alpha.bgaj", 75.0f);
    assert_matches_reference("color_alpha_t75.png");
}

// ---- Test 6: pivot snap (NOT interpolated) ----

TEST_F(RegressionTest, PivotSnap) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_pivot_snap.bgaj", 49.0f);
    assert_matches_reference("pivot_snap_t49.png");

    render_frame("reg_pivot_snap.bgaj", 50.0f);
    assert_matches_reference("pivot_snap_t50.png");

    // The two frames must be visually different (pivot snapped)
    render_frame("reg_pivot_snap.bgaj", 49.0f);
    auto before = read_pixel(320, 240);
    render_frame("reg_pivot_snap.bgaj", 50.0f);
    auto after = read_pixel(320, 240);
    bool changed = (before.r != after.r) || (before.g != after.g) || (before.b != after.b);
    EXPECT_TRUE(changed) << "Pivot snap should cause visible change at (320,240)";
}

// ---- Test 7: display snap (NOT interpolated) ----

TEST_F(RegressionTest, DisplaySnap) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_display_snap.bgaj", 49.0f);
    assert_matches_reference("display_snap_t49.png");
    auto hidden = read_pixel(320, 240);
    EXPECT_LE(hidden.r, 5);
    EXPECT_LE(hidden.g, 5);
    EXPECT_LE(hidden.b, 5);

    render_frame("reg_display_snap.bgaj", 50.0f);
    assert_matches_reference("display_snap_t50.png");
    auto visible = read_pixel(320, 240);
    EXPECT_GE(visible.g, 250);
}

// ---- Test 8: effect/blend mode snap (NOT interpolated) ----

TEST_F(RegressionTest, EffectSnap) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_effect_snap.bgaj", 49.0f);
    assert_matches_reference("effect_snap_t49.png");
    auto normal = read_pixel(320, 240);
    // Normal blend: gray on gray = gray (~128)
    EXPECT_GE(normal.r, 120);
    EXPECT_LE(normal.r, 136);

    render_frame("reg_effect_snap.bgaj", 50.0f);
    assert_matches_reference("effect_snap_t50.png");
    auto screen = read_pixel(320, 240);
    // Screen/additive: gray + gray = white (~255)
    EXPECT_GE(screen.r, 245);
}

// ---- Test 9: visibility window ----

TEST_F(RegressionTest, VisibilityWindow) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_visibility.bgaj", 9.0f);
    assert_matches_reference("visibility_t9.png");
    auto before = read_pixel(320, 240);
    EXPECT_LE(before.b, 5);   // invisible (black)

    render_frame("reg_visibility.bgaj", 10.0f);
    assert_matches_reference("visibility_t10.png");
    auto at_first = read_pixel(320, 240);
    EXPECT_GE(at_first.b, 250);  // visible (blue)

    render_frame("reg_visibility.bgaj", 49.0f);
    assert_matches_reference("visibility_t49.png");
    auto before_last = read_pixel(320, 240);
    EXPECT_GE(before_last.b, 250);  // still visible

    render_frame("reg_visibility.bgaj", 50.0f);
    assert_matches_reference("visibility_t50.png");
    auto at_last = read_pixel(320, 240);
    EXPECT_LE(at_last.b, 5);  // invisible (at last keyframe tick)
}

// ---- Test 10: ANI mode frame selection ----

TEST_F(RegressionTest, AniModeFrameSelection) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_ani_frames.bgaj", 0.0f);
    assert_matches_reference("ani_t0.png");
    auto t0 = read_pixel(320, 240);
    EXPECT_GE(t0.r, 250);  // red (frame 0)

    render_frame("reg_ani_frames.bgaj", 25.0f);
    assert_matches_reference("ani_t25.png");
    auto t25 = read_pixel(320, 240);
    EXPECT_GE(t25.g, 250);  // green (frame 1)

    render_frame("reg_ani_frames.bgaj", 50.0f);
    assert_matches_reference("ani_t50.png");
    auto t50 = read_pixel(320, 240);
    EXPECT_GE(t50.b, 250);  // blue (frame 2)

    render_frame("reg_ani_frames.bgaj", 75.0f);
    assert_matches_reference("ani_t75.png");
    auto t75 = read_pixel(320, 240);
    EXPECT_GE(t75.r, 250);
    EXPECT_GE(t75.g, 250);  // yellow (frame 3)
}

// ---- Test 11: layer compositing order ----

TEST_F(RegressionTest, LayerCompositingOrder) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_layer_order.bgaj", 50.0f);
    assert_matches_reference("layer_order_t50.png");

    // Only red bg
    auto corner = read_pixel(50, 50);
    EXPECT_GE(corner.r, 250);
    EXPECT_LE(corner.g, 5);
    EXPECT_LE(corner.b, 5);

    // Green on top of red
    auto green_area = read_pixel(150, 150);
    EXPECT_LE(green_area.r, 5);
    EXPECT_GE(green_area.g, 250);

    // Blue on top of green and red
    auto blue_area = read_pixel(250, 250);
    EXPECT_LE(blue_area.r, 5);
    EXPECT_LE(blue_area.g, 5);
    EXPECT_GE(blue_area.b, 250);

    // Blue on top of red (no green here)
    auto blue_red = read_pixel(350, 350);
    EXPECT_LE(blue_red.r, 5);
    EXPECT_GE(blue_red.b, 250);

    // Only red bg (bottom right)
    auto br = read_pixel(450, 450);
    EXPECT_GE(br.r, 250);
    EXPECT_LE(br.g, 5);
}

// ---- Test 12: combined transform and color ----

TEST_F(RegressionTest, CombinedTransformAndColor) {
    if (!has_renderer()) GTEST_SKIP();

    render_frame("reg_combined.bgaj", 50.0f);
    assert_matches_reference("combined_t50.png");

    // Far corner should be black (clear color)
    auto corner = read_pixel(10, 10);
    EXPECT_LE(corner.r, 5);
    EXPECT_LE(corner.g, 5);
    EXPECT_LE(corner.b, 5);
}
