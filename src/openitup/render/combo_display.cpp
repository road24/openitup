#include <openitup/render/combo_display.h>

#include <SDL3/SDL.h>

#include <openitup/gfx/texture_cache.h>
#include <openitup/math/types.h>
#include <openitup/render/noteskin.h>
#include <openitup/sprite/sprite.h>

namespace openitup {

ComboDisplay::ComboDisplay(const NoteSkin* skin, TextureCache* cache)
    : skin_(skin)
    , cache_(cache) {
}

std::vector<int> ComboDisplay::extract_digits(int value) {
    if (value == 0) {
        return {0};
    }

    std::vector<int> digits;
    int remaining = value;
    while (remaining > 0) {
        digits.push_back(remaining % 10);
        remaining /= 10;
    }

    // Reverse to get most-significant digit first
    std::reverse(digits.begin(), digits.end());
    return digits;
}

void ComboDisplay::render(SDL_Renderer* renderer, int combo_value, double /*current_time_ms*/) const {
    if (combo_value == 0) {
        return;  // Don't display zero combo
    }

    auto digits = extract_digits(combo_value);
    int num_digits = static_cast<int>(digits.size());

    // Center the entire combo display
    float total_width = (num_digits - 1) * DIGIT_SPACING;
    float start_x = COMBO_X - (total_width / 2.0f);

    for (int i = 0; i < num_digits; ++i) {
        int digit = digits[i];
        float x = start_x + i * DIGIT_SPACING;

        // Try to render sprite if noteskin available
        const Sprite* sprite = nullptr;
        if (skin_) {
            sprite = skin_->combo_digit(digit);
        }

        if (sprite && cache_) {
            // Render digit sprite
            LayerTransform transform{};
            transform.translate_x = x;
            transform.translate_y = COMBO_Y;
            transform.scale_x = 1.0f;
            transform.scale_y = 1.0f;
            transform.rotate = 0.0f;
            transform.pivot_x = 0;
            transform.pivot_y = 0;

            ColorMod color{255, 255, 255, 255};
            sprite->draw(renderer, *cache_, 0.0f, transform, color, SDL_BLENDMODE_BLEND);
        } else {
            // Fallback: render digit as white text rectangle placeholder
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_FRect rect;
            rect.x = x - 15.0f;
            rect.y = COMBO_Y - 20.0f;
            rect.w = 30.0f;
            rect.h = 40.0f;
            SDL_RenderFillRect(renderer, &rect);

            // Draw digit outline (simple visual distinction)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderRect(renderer, &rect);
        }
    }
}

} // namespace openitup
