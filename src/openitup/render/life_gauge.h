#pragma once

#include <cstdint>

struct SDL_Renderer;

namespace openitup {

class LifeGauge {
public:
    LifeGauge();

    // Render the life gauge at a fixed screen position (top-center).
    // hp_ratio: current HP as a ratio [0.0, 1.0]
    //   - 0.0 = empty (fail state)
    //   - 1.0 = full (100% HP)
    void render(SDL_Renderer* renderer, float hp_ratio) const;

private:
    // Screen position (640x480 logical space, top-center).
    static constexpr float GAUGE_X = 220.0f;
    static constexpr float GAUGE_Y = 10.0f;
    static constexpr float GAUGE_WIDTH = 200.0f;
    static constexpr float GAUGE_HEIGHT = 16.0f;

    // Color thresholds
    static constexpr float GREEN_THRESHOLD = 0.5f;   // > 50% HP
    static constexpr float YELLOW_THRESHOLD = 0.25f; // 25-50% HP
    // < 25% HP is red

    struct Color { uint8_t r, g, b, a; };
    static Color get_color(float hp_ratio);
};

} // namespace openitup
