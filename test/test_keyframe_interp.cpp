#include <gtest/gtest.h>
#include <openitup/bga/keyframe.h>
#include <openitup/math/types.h>
#include <cmath>
#include <vector>

using namespace openitup;

// --- lerp ---

TEST(Lerp, AtZero) {
    EXPECT_FLOAT_EQ(lerp(10.0f, 20.0f, 0.0f), 10.0f);
}

TEST(Lerp, AtOne) {
    EXPECT_FLOAT_EQ(lerp(10.0f, 20.0f, 1.0f), 20.0f);
}

TEST(Lerp, AtHalf) {
    EXPECT_FLOAT_EQ(lerp(10.0f, 20.0f, 0.5f), 15.0f);
}

TEST(Lerp, NegativeValues) {
    EXPECT_FLOAT_EQ(lerp(-10.0f, 10.0f, 0.5f), 0.0f);
}

// --- Keyframe JSON round-trip ---

TEST(KeyframeJson, RoundTrip) {
    Keyframe kf;
    kf.tick = 120;
    kf.translate_x = 200.0f;
    kf.translate_y = 100.0f;
    kf.pivot_x = 32.0f;
    kf.pivot_y = 48.0f;
    kf.scale_x = 2.0f;
    kf.scale_y = 0.5f;
    kf.rotate = 45.0f;
    kf.color_r = 1.0f;
    kf.color_g = 0.5f;
    kf.color_b = 0.0f;
    kf.color_a = 0.75f;
    kf.display = true;
    kf.effect = BlendEffect::Screen;

    auto j = kf.to_json();

    Keyframe kf2;
    kf2.from_json(j);

    EXPECT_EQ(kf2.tick, 120);
    EXPECT_FLOAT_EQ(kf2.translate_x, 200.0f);
    EXPECT_FLOAT_EQ(kf2.translate_y, 100.0f);
    EXPECT_FLOAT_EQ(kf2.pivot_x, 32.0f);
    EXPECT_FLOAT_EQ(kf2.pivot_y, 48.0f);
    EXPECT_FLOAT_EQ(kf2.scale_x, 2.0f);
    EXPECT_FLOAT_EQ(kf2.scale_y, 0.5f);
    EXPECT_FLOAT_EQ(kf2.rotate, 45.0f);
    EXPECT_FLOAT_EQ(kf2.color_r, 1.0f);
    EXPECT_FLOAT_EQ(kf2.color_g, 0.5f);
    EXPECT_FLOAT_EQ(kf2.color_b, 0.0f);
    EXPECT_FLOAT_EQ(kf2.color_a, 0.75f);
    EXPECT_TRUE(kf2.display);
    EXPECT_EQ(kf2.effect, BlendEffect::Screen);
}

// --- BlendEffect string conversion ---

TEST(BlendEffect, FromString) {
    EXPECT_EQ(blend_effect_from_string("normal"), BlendEffect::Normal);
    EXPECT_EQ(blend_effect_from_string("screen"), BlendEffect::Screen);
    EXPECT_EQ(blend_effect_from_string("multiply"), BlendEffect::Multiply);
    EXPECT_EQ(blend_effect_from_string("dodge"), BlendEffect::Dodge);
    EXPECT_EQ(blend_effect_from_string("difference"), BlendEffect::Difference);
    EXPECT_EQ(blend_effect_from_string("unknown"), BlendEffect::Normal);
}

TEST(BlendEffect, ToString) {
    EXPECT_STREQ(blend_effect_to_string(BlendEffect::Normal), "normal");
    EXPECT_STREQ(blend_effect_to_string(BlendEffect::Screen), "screen");
    EXPECT_STREQ(blend_effect_to_string(BlendEffect::Multiply), "multiply");
    EXPECT_STREQ(blend_effect_to_string(BlendEffect::Dodge), "dodge");
    EXPECT_STREQ(blend_effect_to_string(BlendEffect::Difference), "difference");
}

// --- Keyframe interpolation ---

class KeyframeInterpolation : public ::testing::Test {
protected:
    Keyframe make_kf(uint16_t tick, float tx, float ty, float sx, float sy,
                     float rot, float cr, float cg, float cb, float ca,
                     float px = 0, float py = 0,
                     bool disp = true, BlendEffect eff = BlendEffect::Normal) {
        Keyframe kf;
        kf.tick = tick;
        kf.translate_x = tx; kf.translate_y = ty;
        kf.pivot_x = px; kf.pivot_y = py;
        kf.scale_x = sx; kf.scale_y = sy;
        kf.rotate = rot;
        kf.color_r = cr; kf.color_g = cg; kf.color_b = cb; kf.color_a = ca;
        kf.display = disp;
        kf.effect = eff;
        return kf;
    }
};

TEST_F(KeyframeInterpolation, AtStartKeyframe) {
    Keyframe start = make_kf(30, 0, 0, 1, 1, 0, 1, 1, 1, 0);
    Keyframe end   = make_kf(90, 100, 200, 2, 2, 90, 1, 1, 1, 1);

    auto props = interpolate_keyframes(start, end, 30.0f);
    EXPECT_FLOAT_EQ(props.dt, 0.0f);
    EXPECT_FLOAT_EQ(props.translate_x, 0.0f);
    EXPECT_FLOAT_EQ(props.translate_y, 0.0f);
    EXPECT_FLOAT_EQ(props.color_a, 0.0f);
}

TEST_F(KeyframeInterpolation, AtEndKeyframe) {
    Keyframe start = make_kf(30, 0, 0, 1, 1, 0, 1, 1, 1, 0);
    Keyframe end   = make_kf(90, 100, 200, 2, 2, 90, 1, 1, 1, 1);

    auto props = interpolate_keyframes(start, end, 90.0f);
    EXPECT_FLOAT_EQ(props.dt, 1.0f);
    EXPECT_FLOAT_EQ(props.translate_x, 100.0f);
    EXPECT_FLOAT_EQ(props.translate_y, 200.0f);
    EXPECT_FLOAT_EQ(props.color_a, 1.0f);
}

TEST_F(KeyframeInterpolation, Midpoint) {
    Keyframe start = make_kf(0, 0, 0, 1, 1, 0, 0, 0, 0, 0);
    Keyframe end   = make_kf(60, 120, 240, 3, 3, 180, 1, 1, 1, 1);

    auto props = interpolate_keyframes(start, end, 30.0f);
    EXPECT_FLOAT_EQ(props.dt, 0.5f);
    EXPECT_FLOAT_EQ(props.translate_x, 60.0f);
    EXPECT_FLOAT_EQ(props.translate_y, 120.0f);
    EXPECT_FLOAT_EQ(props.scale_x, 2.0f);
    EXPECT_FLOAT_EQ(props.rotate, 90.0f);
    EXPECT_FLOAT_EQ(props.color_a, 0.5f);
}

TEST_F(KeyframeInterpolation, SubTickPrecision) {
    Keyframe start = make_kf(30, 0, 0, 1, 1, 0, 1, 1, 1, 0);
    Keyframe end   = make_kf(90, 600, 0, 1, 1, 0, 1, 1, 1, 1);

    // At tick 30.5 — half a tick past start (sub-tick for 120fps rendering)
    auto props = interpolate_keyframes(start, end, 30.5f);
    EXPECT_NEAR(props.dt, 0.5f / 60.0f, 1e-5f);
    EXPECT_NEAR(props.translate_x, 600.0f * 0.5f / 60.0f, 0.01f);
}

TEST_F(KeyframeInterpolation, PivotSnapsFromStart) {
    Keyframe start = make_kf(0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 10.0f, 20.0f);
    Keyframe end   = make_kf(60, 0, 0, 1, 1, 0, 1, 1, 1, 1, 99.0f, 99.0f);

    auto props = interpolate_keyframes(start, end, 30.0f);
    EXPECT_FLOAT_EQ(props.pivot_x, 10.0f);
    EXPECT_FLOAT_EQ(props.pivot_y, 20.0f);
}

TEST_F(KeyframeInterpolation, EffectSnapsFromStart) {
    Keyframe start = make_kf(0, 0, 0, 1, 1, 0, 1, 1, 1, 1,
                             0, 0, true, BlendEffect::Screen);
    Keyframe end   = make_kf(60, 0, 0, 1, 1, 0, 1, 1, 1, 1,
                             0, 0, true, BlendEffect::Multiply);

    auto props = interpolate_keyframes(start, end, 30.0f);
    EXPECT_EQ(props.effect, BlendEffect::Screen);
}

TEST_F(KeyframeInterpolation, DisplaySnapsFromStart) {
    Keyframe start = make_kf(0, 0, 0, 1, 1, 0, 1, 1, 1, 1,
                             0, 0, false, BlendEffect::Normal);
    Keyframe end   = make_kf(60, 0, 0, 1, 1, 0, 1, 1, 1, 1,
                             0, 0, true, BlendEffect::Normal);

    auto props = interpolate_keyframes(start, end, 30.0f);
    EXPECT_FALSE(props.display);
}

// --- evaluate_keyframes (visibility window) ---

class EvaluateKeyframes : public KeyframeInterpolation {
protected:
    std::vector<Keyframe> make_timeline() {
        return {
            make_kf(30, 0, 0, 1, 1, 0, 1, 1, 1, 0),     // tick 30: alpha=0
            make_kf(90, 200, 100, 1, 1, 0, 1, 1, 1, 1),  // tick 90: alpha=1
            make_kf(600, 200, 100, 1, 1, 0, 1, 1, 1, 1),  // tick 600: hold
        };
    }
};

TEST_F(EvaluateKeyframes, InvisibleBeforeFirstKeyframe) {
    auto kfs = make_timeline();
    EXPECT_FALSE(evaluate_keyframes(kfs, 0.0f).has_value());
    EXPECT_FALSE(evaluate_keyframes(kfs, 29.0f).has_value());
    EXPECT_FALSE(evaluate_keyframes(kfs, 29.9f).has_value());
}

TEST_F(EvaluateKeyframes, VisibleAtFirstKeyframe) {
    auto kfs = make_timeline();
    auto props = evaluate_keyframes(kfs, 30.0f);
    ASSERT_TRUE(props.has_value());
    EXPECT_FLOAT_EQ(props->translate_x, 0.0f);
    EXPECT_FLOAT_EQ(props->color_a, 0.0f);
}

TEST_F(EvaluateKeyframes, InterpolatesBetweenKeyframes) {
    auto kfs = make_timeline();
    auto props = evaluate_keyframes(kfs, 60.0f);
    ASSERT_TRUE(props.has_value());
    EXPECT_FLOAT_EQ(props->dt, 0.5f);
    EXPECT_FLOAT_EQ(props->translate_x, 100.0f);
    EXPECT_FLOAT_EQ(props->color_a, 0.5f);
}

TEST_F(EvaluateKeyframes, InvisibleAtLastKeyframeTick) {
    auto kfs = make_timeline();
    EXPECT_FALSE(evaluate_keyframes(kfs, 600.0f).has_value());
}

TEST_F(EvaluateKeyframes, InvisiblePastLastKeyframe) {
    auto kfs = make_timeline();
    EXPECT_FALSE(evaluate_keyframes(kfs, 601.0f).has_value());
    EXPECT_FALSE(evaluate_keyframes(kfs, 9999.0f).has_value());
}

TEST_F(EvaluateKeyframes, SubTickBetweenKeyframes) {
    auto kfs = make_timeline();
    // tick 30.5: just past the first keyframe
    auto props = evaluate_keyframes(kfs, 30.5f);
    ASSERT_TRUE(props.has_value());
    EXPECT_NEAR(props->dt, 0.5f / 60.0f, 1e-5f);
}

TEST_F(EvaluateKeyframes, HiddenByDisplayFlag) {
    std::vector<Keyframe> kfs = {
        make_kf(0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, false),
        make_kf(60, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, true),
        make_kf(120, 0, 0, 1, 1, 0, 1, 1, 1, 1),
    };

    // Between kf[0] and kf[1]: display=false on start -> invisible
    EXPECT_FALSE(evaluate_keyframes(kfs, 30.0f).has_value());

    // Between kf[1] and kf[2]: display=true on start -> visible
    auto props = evaluate_keyframes(kfs, 90.0f);
    ASSERT_TRUE(props.has_value());
}

TEST_F(EvaluateKeyframes, EmptyKeyframes) {
    std::vector<Keyframe> kfs;
    EXPECT_FALSE(evaluate_keyframes(kfs, 0.0f).has_value());
}

TEST_F(EvaluateKeyframes, SingleKeyframe) {
    std::vector<Keyframe> kfs = {
        make_kf(30, 0, 0, 1, 1, 0, 1, 1, 1, 1),
    };
    // tick < first: invisible
    EXPECT_FALSE(evaluate_keyframes(kfs, 0.0f).has_value());
    // tick >= last (and only): invisible
    EXPECT_FALSE(evaluate_keyframes(kfs, 30.0f).has_value());
}

// --- ScreenRect ---

TEST(ScreenRect, Dimensions) {
    ScreenRect r{10, 20, 74, 116};
    EXPECT_EQ(r.width(), 64);
    EXPECT_EQ(r.height(), 96);
}
