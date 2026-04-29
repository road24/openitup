#include <openitup/render/judgment_display.h>

#include <SDL3/SDL.h>

namespace openitup {

// Tier colors: PERFECT=green, GREAT=cyan, GOOD=yellow, BAD=orange, MISS=red
const JudgmentDisplay::TierColor JudgmentDisplay::TIER_COLORS[] = {
    {0, 255, 0},       // PERFECT: green
    {0, 200, 255},     // GREAT: cyan
    {255, 255, 0},     // GOOD: yellow
    {255, 128, 0},     // BAD: orange
    {255, 0, 0}        // MISS: red
};

JudgmentDisplay::JudgmentDisplay() {
    // Start invisible
}

void JudgmentDisplay::on_judgment(JudgmentTier tier) {
    current_tier_ = tier;
    time_since_judgment_ = 0.0;
}

void JudgmentDisplay::render(SDL_Renderer* renderer, double dt) {
    // Accumulate time
    time_since_judgment_ += dt;

    // Only render if visible
    if (time_since_judgment_ < DISPLAY_DURATION) {
        // Get color for current tier
        const auto& color = TIER_COLORS[static_cast<int>(current_tier_)];
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

        // Draw filled rectangle
        SDL_FRect rect;
        rect.x = DISPLAY_X;
        rect.y = DISPLAY_Y;
        rect.w = DISPLAY_W;
        rect.h = DISPLAY_H;
        SDL_RenderFillRect(renderer, &rect);
    }
}

JudgmentTier JudgmentDisplay::current_tier() const {
    return current_tier_;
}

bool JudgmentDisplay::is_visible() const {
    return time_since_judgment_ < DISPLAY_DURATION;
}

} // namespace openitup
