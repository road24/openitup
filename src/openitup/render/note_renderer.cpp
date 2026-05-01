#include <openitup/render/note_renderer.h>

#include <algorithm>

#include <SDL3/SDL.h>
#include <openitup/chart/note_type.h>
#include <openitup/render/noteskin.h>
#include <openitup/render/noteskin_anim.h>
#include <openitup/gfx/texture_cache.h>
#include <openitup/sprite/sprite.h>
#include <openitup/math/types.h>

namespace openitup {

// Per-column colors: Red, Blue, Yellow, Green, Magenta repeated for single (0-4) and double (5-9)
const ColumnColor COLUMN_COLORS[10] = {
    {255, 50, 50},    // Column 0: Red
    {50, 100, 255},   // Column 1: Blue
    {255, 255, 50},   // Column 2: Yellow
    {50, 255, 50},    // Column 3: Green
    {255, 50, 255},   // Column 4: Magenta
    {255, 50, 50},    // Column 5: Red (double mode)
    {50, 100, 255},   // Column 6: Blue (double mode)
    {255, 255, 50},   // Column 7: Yellow (double mode)
    {50, 255, 50},    // Column 8: Green (double mode)
    {255, 50, 255}    // Column 9: Magenta (double mode)
};

NoteFieldConfig default_single_config() {
    NoteFieldConfig config;
    config.num_columns = 5;
    config.column_x.resize(5);

    float center_x = 320.0f;
    float spacing = 56.0f;  // pixels between column centers
    float start_x = center_x - 2.0f * spacing;

    for (int i = 0; i < 5; i++) {
        config.column_x[i] = start_x + static_cast<float>(i) * spacing;
    }
    // column_x = {208, 264, 320, 376, 432}

    return config;
}

NoteRenderer::NoteRenderer(const NoteData& note_data, const TimingData& timing_data,
                           const NoteFieldConfig& config,
                           const NoteSkin* skin,
                           TextureCache* cache)
    : note_data_(note_data), timing_data_(timing_data), config_(config),
      skin_(skin), cache_(cache) {
}

float NoteRenderer::beat_to_y(double note_beat, double current_beat) const {
    double beat_delta = note_beat - current_beat;
    return config_.receptor_y
         + static_cast<float>(beat_delta) * config_.pixels_per_beat * config_.scroll_speed;
}

void NoteRenderer::render(SDL_Renderer* renderer, double song_position_ms, double global_time_ms, double render_alpha, const std::vector<bool>* judged_notes) const {
    // Convert song position to current beat
    double current_beat = timing_data_.beat_at_time(song_position_ms / 1000.0);

    // Cache sprite lookups per frame (optimization: avoid repeated virtual calls)
    std::array<const Sprite*, 10> tap_sprite_cache = {nullptr};
    std::array<const Sprite*, 10> hold_head_sprite_cache = {nullptr};
    std::array<const Sprite*, 10> hold_body_sprite_cache = {nullptr};
    if (skin_ && cache_) {
        for (int col = 0; col < config_.num_columns; ++col) {
            int track = col % NUM_TRACKS;
            tap_sprite_cache[col] = skin_->tap(track);
            hold_head_sprite_cache[col] = skin_->hold(track, HoldPart::HEAD);
            hold_body_sprite_cache[col] = skin_->hold(track, HoldPart::BODY);
        }
    }

    // Compute visible beat range (notes scroll bottom-to-top)
    // Tightened range: exact viewport bounds without generous margins
    double beats_below_receptor = (config_.receptor_y + config_.note_height) / (config_.pixels_per_beat * config_.scroll_speed);
    double beats_above_receptor = (480.0f - config_.receptor_y) / (config_.pixels_per_beat * config_.scroll_speed);
    double top_beat = current_beat + beats_above_receptor;
    double bottom_beat = current_beat - beats_below_receptor;

    // Get notes in visible range
    auto [begin_it, end_it] = note_data_.notes_in_range(bottom_beat, top_beat);

    // Compute next beat for interpolation (if render_alpha > 0)
    constexpr double FIXED_STEP = 1.0 / 60.0;  // Fixed logic tick rate
    double next_beat = current_beat;
    if (render_alpha > 0.0) {
        next_beat = timing_data_.beat_at_time((song_position_ms + FIXED_STEP * 1000.0) / 1000.0);
    }

    // First pass: render hold bodies (behind everything)
    for (auto it = begin_it; it != end_it; ++it) {
        const auto& note = *it;

        // Only process HOLD_HEAD notes in this pass
        if (note.type != NoteType::HOLD_HEAD) {
            continue;
        }

        // Skip if column is out of range
        if (note.column >= config_.num_columns) {
            continue;
        }

        // Find the matching HOLD_TAIL
        auto tail_it = std::next(it);
        const NoteEvent* tail = nullptr;
        while (tail_it != end_it) {
            if (tail_it->column == note.column && tail_it->type == NoteType::HOLD_TAIL) {
                tail = &(*tail_it);
                break;
            }
            ++tail_it;
        }

        if (!tail) {
            // No tail found, skip this hold body
            continue;
        }

        // Compute screen positions for head and tail
        float head_y;
        float tail_y;
        if (render_alpha > 0.0) {
            float head_y_current = beat_to_y(note.beat, current_beat);
            float head_y_next = beat_to_y(note.beat, next_beat);
            head_y = head_y_current + (head_y_next - head_y_current) * static_cast<float>(render_alpha);

            float tail_y_current = beat_to_y(tail->beat, current_beat);
            float tail_y_next = beat_to_y(tail->beat, next_beat);
            tail_y = tail_y_current + (tail_y_next - tail_y_current) * static_cast<float>(render_alpha);
        } else {
            head_y = beat_to_y(note.beat, current_beat);
            tail_y = beat_to_y(tail->beat, current_beat);
        }

        // Hold body spans from tail to head (tail beat is earlier, so tail_y < head_y)
        float body_top = tail_y;
        float body_bottom = head_y;
        float body_height = body_bottom - body_top;

        // Skip if hold body is completely offscreen
        if (body_bottom < 0.0f || body_top > 480.0f) {
            continue;
        }

        // Render hold body
        const Sprite* body_sprite = hold_body_sprite_cache[note.column];
        if (body_sprite && skin_ && cache_) {
            // Sprite rendering: tile the body sprite vertically
            float t = noteskin_loop_t(global_time_ms);

            // Tile the body sprite to fill the gap between head and tail
            // Use note_height as the tile size
            float tile_height = config_.note_height;
            int num_tiles = static_cast<int>(std::ceil(body_height / tile_height));

            for (int tile = 0; tile < num_tiles; ++tile) {
                float tile_y = body_top + tile * tile_height;

                LayerTransform xform{};
                xform.translate_x = config_.column_x[note.column] - (config_.note_sprite_size / 2.0f);
                xform.translate_y = tile_y - (config_.note_sprite_size / 2.0f);

                body_sprite->draw(renderer, *cache_, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND);
            }
        } else {
            // Fallback: colored rectangle for hold body
            float x = config_.column_x[note.column] - config_.note_width / 2.0f;
            const auto& color = COLUMN_COLORS[note.column];

            // Dimmer color for hold body (50% brightness)
            SDL_SetRenderDrawColor(renderer, color.r / 2, color.g / 2, color.b / 2, 180);

            SDL_FRect rect;
            rect.x = x;
            rect.y = body_top;
            rect.w = config_.note_width;
            rect.h = body_height;
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    // Second pass: render note heads (TAP and HOLD_HEAD)
    size_t note_idx = 0;
    for (auto it = begin_it; it != end_it; ++it) {
        const auto& note = *it;

        // Skip if already judged (optimization: don't render judged notes)
        if (judged_notes && note_idx < judged_notes->size() && (*judged_notes)[note_idx]) {
            note_idx++;
            continue;
        }
        note_idx++;

        // Skip non-TAP and non-HOLD_HEAD notes
        if (note.type != NoteType::TAP && note.type != NoteType::HOLD_HEAD) {
            continue;
        }

        // Skip if column is out of range
        if (note.column >= config_.num_columns) {
            continue;
        }

        // Compute screen position with optional interpolation
        float y;
        if (render_alpha > 0.0) {
            float y_current = beat_to_y(note.beat, current_beat);
            float y_next = beat_to_y(note.beat, next_beat);
            y = y_current + (y_next - y_current) * static_cast<float>(render_alpha);
        } else {
            y = beat_to_y(note.beat, current_beat);
        }

        // Skip if offscreen (with margin for note height)
        if (y < -config_.note_height || y > 480.0f + config_.note_height) {
            continue;
        }

        // Try sprite rendering if noteskin is available
        // Use cached sprites to avoid repeated virtual calls
        const Sprite* sprite = nullptr;

        if (skin_ && cache_) {
            switch (note.type) {
                case NoteType::TAP:
                    sprite = tap_sprite_cache[note.column];
                    break;
                case NoteType::HOLD_HEAD:
                    sprite = hold_head_sprite_cache[note.column];
                    break;
                default:
                    break;
            }
        }

        if (sprite) {
            // Sprite rendering path
            float t = noteskin_loop_t(global_time_ms);
            LayerTransform xform{};
            xform.translate_x = config_.column_x[note.column] - (config_.note_sprite_size / 2.0f);
            xform.translate_y = y - (config_.note_sprite_size / 2.0f);
            sprite->draw(renderer, *cache_, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND);
        } else {
            // Phase 1 fallback: colored rectangle
            float x = config_.column_x[note.column] - config_.note_width / 2.0f;

            const auto& color = COLUMN_COLORS[note.column];
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

            SDL_FRect rect;
            rect.x = x;
            rect.y = y - config_.note_height / 2.0f;
            rect.w = config_.note_width;
            rect.h = config_.note_height;
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

void NoteRenderer::render_receptors(SDL_Renderer* renderer,
                                    double global_time_ms,
                                    const bool* pressed_columns,
                                    const double* judge_trigger_times) const {
    for (int col = 0; col < config_.num_columns; ++col) {
        int track = col % NUM_TRACKS;
        float x = config_.column_x[col] - (config_.note_sprite_size / 2.0f);
        float y = config_.receptor_y - (config_.note_sprite_size / 2.0f);

        LayerTransform xform{};
        xform.translate_x = x;
        xform.translate_y = y;

        // Layer 1: Receptor background
        bool receptor_drawn = false;
        if (skin_ && cache_) {
            const Sprite* receptor_sprite = skin_->receptor(PlayMode::SINGLE);
            if (receptor_sprite) {
                float t = noteskin_loop_t(global_time_ms);
                receptor_sprite->draw(renderer, *cache_, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND);
                receptor_drawn = true;
            }
        }

        // Fallback: dim gray outlined rectangle
        if (!receptor_drawn) {
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 180);
            SDL_FRect rect;
            rect.x = config_.column_x[col] - config_.note_width / 2.0f;
            rect.y = config_.receptor_y - config_.note_height / 2.0f;
            rect.w = config_.note_width;
            rect.h = config_.note_height;
            SDL_RenderRect(renderer, &rect);
        }

        // Layer 2: Press overlay (loops while pressed)
        if (pressed_columns && pressed_columns[col] && skin_ && cache_) {
            const Sprite* press_sprite = skin_->press(track);
            if (press_sprite) {
                float t = noteskin_loop_t(global_time_ms);
                press_sprite->draw(renderer, *cache_, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND);
            }
        }

        // Layer 3: Judge overlay (one-shot, 300ms)
        if (judge_trigger_times && noteskin_oneshot_active(global_time_ms, judge_trigger_times[col]) && skin_ && cache_) {
            const Sprite* judge_sprite = skin_->judge(track);
            if (judge_sprite) {
                float t = noteskin_oneshot_t(global_time_ms, judge_trigger_times[col]);
                judge_sprite->draw(renderer, *cache_, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND);
            }
        }
    }
}

const NoteFieldConfig& NoteRenderer::config() const {
    return config_;
}

void NoteRenderer::trigger_hit_effect(uint8_t column, JudgmentTier tier, double global_time_ms) {
    // Only trigger effects for successful hits (not Miss or Bad)
    if (tier == JudgmentTier::MISS || tier == JudgmentTier::BAD) {
        return;
    }

    // Add new effect to the active list
    active_hit_effects_.push_back({column, global_time_ms, tier});

    // Clean up old effects (older than 300ms)
    constexpr double EFFECT_DURATION_MS = 300.0;
    active_hit_effects_.erase(
        std::remove_if(active_hit_effects_.begin(), active_hit_effects_.end(),
            [global_time_ms](const HitEffect& effect) {
                return (global_time_ms - effect.trigger_time_ms) >= EFFECT_DURATION_MS;
            }),
        active_hit_effects_.end()
    );
}

void NoteRenderer::render_hit_effects(SDL_Renderer* renderer, double global_time_ms) const {
    constexpr double EFFECT_DURATION_MS = 300.0;

    // Define colors per judgment tier (RGB + alpha based on timing)
    // Perfect = cyan/white bright flash
    // Great = yellow/gold
    // Good = green
    struct EffectColor {
        uint8_t r, g, b;
    };

    constexpr EffectColor tier_colors[] = {
        {200, 255, 255},  // PERFECT: bright cyan
        {255, 230, 100},  // GREAT: gold/yellow
        {100, 255, 150},  // GOOD: green
        {255, 150, 50},   // BAD: orange (not used, but defined)
        {150, 50, 50}     // MISS: dim red (not used, but defined)
    };

    for (const auto& effect : active_hit_effects_) {
        double elapsed = global_time_ms - effect.trigger_time_ms;
        if (elapsed >= EFFECT_DURATION_MS || elapsed < 0.0) {
            continue;
        }

        // Skip if column out of range
        if (effect.column >= config_.num_columns) {
            continue;
        }

        // Calculate fade-out alpha (1.0 at start, 0.0 at end)
        double t = elapsed / EFFECT_DURATION_MS;
        double alpha_factor = 1.0 - t;

        // Perfect tier gets extra brightness initially
        if (effect.tier == JudgmentTier::PERFECT && t < 0.3) {
            alpha_factor = 1.0 - (t / 0.3) * 0.5;  // Stay brighter longer
        }

        uint8_t alpha = static_cast<uint8_t>(alpha_factor * 200.0);  // Max alpha = 200 for translucency

        // Get color for this tier
        const auto& color = tier_colors[static_cast<int>(effect.tier)];

        // Draw a colored rectangle burst at receptor position
        // Make it slightly larger than the note for visual impact
        float burst_size = config_.note_width * 1.5f;
        float x = config_.column_x[effect.column] - burst_size / 2.0f;
        float y = config_.receptor_y - burst_size / 2.0f;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);

        SDL_FRect rect;
        rect.x = x;
        rect.y = y;
        rect.w = burst_size;
        rect.h = burst_size;
        SDL_RenderFillRect(renderer, &rect);
    }
}

} // namespace openitup
