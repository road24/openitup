#include <openitup/render/note_renderer.h>

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

    // Render each visible note
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
        const Sprite* sprite = nullptr;
        int track = note.column % NUM_TRACKS;

        if (skin_ && cache_) {
            switch (note.type) {
                case NoteType::TAP:
                    sprite = skin_->tap(track);
                    break;
                case NoteType::HOLD_HEAD:
                    sprite = skin_->hold(track, HoldPart::HEAD);
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

} // namespace openitup
