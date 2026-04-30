#pragma once

#include <vector>

struct SDL_Renderer;

namespace openitup {

class NoteSkin;
class TextureCache;

class ComboDisplay {
public:
    ComboDisplay(const NoteSkin* skin = nullptr, TextureCache* cache = nullptr);

    void render(SDL_Renderer* renderer, int combo_value, double current_time_ms) const;

    // Extract decimal digits from a number (for testing).
    static std::vector<int> extract_digits(int value);

private:
    const NoteSkin* skin_;
    TextureCache* cache_;

    // Screen position (640x480 logical space, upper center).
    static constexpr float COMBO_X = 320.0f;
    static constexpr float COMBO_Y = 100.0f;
    static constexpr float DIGIT_SPACING = 40.0f;
};

} // namespace openitup
