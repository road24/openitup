#pragma once

#include <cmath>

namespace openitup {

struct ScreenRect {
    int x1, y1, x2, y2;

    int width() const { return x2 - x1; }
    int height() const { return y2 - y1; }
};

struct UVRect {
    float u1, v1, u2, v2;
};

struct ColorMod {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

struct LayerTransform {
    float translate_x = 0.0f, translate_y = 0.0f;
    float pivot_x = 0.0f, pivot_y = 0.0f;
    float scale_x = 1.0f, scale_y = 1.0f;
    float rotate = 0.0f;
};

inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

} // namespace openitup
