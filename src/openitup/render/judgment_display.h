#pragma once

#include <cstdint>

#include <openitup/judge/judgment_tier.h>

struct SDL_Renderer;

namespace openitup {

class JudgmentDisplay {
public:
    JudgmentDisplay();

    // Called when a new judgment is issued. Updates the displayed tier.
    void on_judgment(JudgmentTier tier);

    // Render the judgment indicator at a fixed screen position.
    // Shows a colored rectangle whose color corresponds to the tier.
    // Fades after DISPLAY_DURATION seconds.
    void render(SDL_Renderer* renderer, double dt);

    // The most recently displayed tier (for testing).
    JudgmentTier current_tier() const;

    // True if the display is currently visible (within fade duration).
    bool is_visible() const;

private:
    JudgmentTier current_tier_ = JudgmentTier::MISS;
    mutable double time_since_judgment_ = 999.0;  // start invisible

    static constexpr double DISPLAY_DURATION = 0.5;  // seconds

    // Screen position (640x480 logical space).
    static constexpr float DISPLAY_X = 260.0f;   // centered-ish
    static constexpr float DISPLAY_Y = 200.0f;   // above note field
    static constexpr float DISPLAY_W = 120.0f;
    static constexpr float DISPLAY_H = 40.0f;

    struct TierColor { uint8_t r, g, b; };
    static const TierColor TIER_COLORS[];
};

} // namespace openitup
