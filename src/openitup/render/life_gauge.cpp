#include <openitup/render/life_gauge.h>

#include <algorithm>

#include <SDL3/SDL.h>

namespace openitup {

LifeGauge::LifeGauge() = default;

LifeGauge::Color LifeGauge::get_color(float hp_ratio) {
    if (hp_ratio > GREEN_THRESHOLD) {
        // Green when HP > 50%
        return {0, 255, 0, 255};
    } else if (hp_ratio > YELLOW_THRESHOLD) {
        // Yellow when HP between 25-50%
        return {255, 255, 0, 255};
    } else {
        // Red when HP < 25%
        return {255, 0, 0, 255};
    }
}

void LifeGauge::render(SDL_Renderer* renderer, float hp_ratio) const {
    // Clamp hp_ratio to [0.0, 1.0]
    hp_ratio = std::clamp(hp_ratio, 0.0f, 1.0f);

    // Draw background (dark gray border)
    SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
    SDL_FRect border_rect;
    border_rect.x = GAUGE_X;
    border_rect.y = GAUGE_Y;
    border_rect.w = GAUGE_WIDTH;
    border_rect.h = GAUGE_HEIGHT;
    SDL_RenderRect(renderer, &border_rect);

    // Draw black background inside
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_FRect bg_rect;
    bg_rect.x = GAUGE_X + 2.0f;
    bg_rect.y = GAUGE_Y + 2.0f;
    bg_rect.w = GAUGE_WIDTH - 4.0f;
    bg_rect.h = GAUGE_HEIGHT - 4.0f;
    SDL_RenderFillRect(renderer, &bg_rect);

    // Draw filled portion (HP bar)
    if (hp_ratio > 0.0f) {
        Color bar_color = get_color(hp_ratio);
        SDL_SetRenderDrawColor(renderer, bar_color.r, bar_color.g, bar_color.b, bar_color.a);

        SDL_FRect fill_rect;
        fill_rect.x = GAUGE_X + 2.0f;
        fill_rect.y = GAUGE_Y + 2.0f;
        fill_rect.w = (GAUGE_WIDTH - 4.0f) * hp_ratio;
        fill_rect.h = GAUGE_HEIGHT - 4.0f;
        SDL_RenderFillRect(renderer, &fill_rect);
    }
}

} // namespace openitup
