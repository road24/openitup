#include <openitup/render/judgment_display.h>

#include <SDL3/SDL.h>

#include <openitup/gfx/texture_cache.h>
#include <openitup/math/types.h>
#include <openitup/render/noteskin.h>
#include <openitup/sprite/sprite.h>

namespace openitup {

// Tier colors: PERFECT=green, GREAT=cyan, GOOD=yellow, BAD=orange, MISS=red
const JudgmentDisplay::TierColor JudgmentDisplay::TIER_COLORS[] = {
    {0, 255, 0},       // PERFECT: green
    {0, 200, 255},     // GREAT: cyan
    {255, 255, 0},     // GOOD: yellow
    {255, 128, 0},     // BAD: orange
    {255, 0, 0}        // MISS: red
};

JudgmentDisplay::JudgmentDisplay(const NoteSkin* skin, TextureCache* cache)
    : skin_(skin)
    , cache_(cache) {
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
        // Try to render sprite if noteskin is available
        const Sprite* sprite = nullptr;
        if (skin_) {
            switch (current_tier_) {
                case JudgmentTier::PERFECT:
                    sprite = skin_->judgment_perfect();
                    break;
                case JudgmentTier::GREAT:
                    sprite = skin_->judgment_great();
                    break;
                case JudgmentTier::GOOD:
                    sprite = skin_->judgment_good();
                    break;
                case JudgmentTier::BAD:
                    sprite = skin_->judgment_bad();
                    break;
                case JudgmentTier::MISS:
                    sprite = skin_->judgment_miss();
                    break;
            }
        }

        if (sprite && cache_) {
            // Render sprite centered at screen center
            LayerTransform transform{};
            transform.translate_x = SPRITE_CENTER_X;
            transform.translate_y = SPRITE_CENTER_Y;
            transform.scale_x = 1.0f;
            transform.scale_y = 1.0f;
            transform.rotate = 0.0f;
            transform.pivot_x = 0;
            transform.pivot_y = 0;

            ColorMod color{255, 255, 255, 255};
            sprite->draw(renderer, *cache_, 0.0f, transform, color, SDL_BLENDMODE_BLEND);
        } else {
            // Fallback: render colored rectangle
            const auto& color = TIER_COLORS[static_cast<int>(current_tier_)];
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

            SDL_FRect rect;
            rect.x = FALLBACK_X;
            rect.y = FALLBACK_Y;
            rect.w = FALLBACK_W;
            rect.h = FALLBACK_H;
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

JudgmentTier JudgmentDisplay::current_tier() const {
    return current_tier_;
}

bool JudgmentDisplay::is_visible() const {
    return time_since_judgment_ < DISPLAY_DURATION;
}

} // namespace openitup
