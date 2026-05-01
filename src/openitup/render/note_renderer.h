#pragma once

#include <cstdint>
#include <vector>

#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>
#include <openitup/judge/judgment_tier.h>

struct SDL_Renderer;

namespace openitup {

class NoteSkin;
class TextureCache;

struct HitEffect {
    uint8_t column;
    double trigger_time_ms;
    JudgmentTier tier;
};

struct NoteFieldConfig {
    float receptor_y = 80.0f;
    float note_width = 48.0f;
    float note_height = 48.0f;
    float note_sprite_size = 64.0f;
    float pixels_per_beat = 80.0f;
    float scroll_speed = 1.0f;
    int num_columns = 5;
    std::vector<float> column_x;
};

struct ColumnColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class NoteRenderer {
public:
    // Construct for a chart. note_data and timing_data must outlive the renderer.
    // skin: optional noteskin pointer (nullable, non-owning) for sprite rendering.
    // cache: required when skin is non-null, for texture resolution during sprite draw.
    NoteRenderer(const NoteData& note_data, const TimingData& timing_data,
                 const NoteFieldConfig& config,
                 const NoteSkin* skin = nullptr,
                 TextureCache* cache = nullptr);

    // Convert a beat position to a Y screen coordinate given the current song beat.
    // Pure function — testable without SDL.
    float beat_to_y(double note_beat, double current_beat) const;

    // Render all visible notes for the current song position.
    // song_position_ms: current audio playback position from AudioSystem.
    // global_time_ms: wall-clock time for noteskin animation (from SDL_GetTicks()).
    // render_alpha: interpolation factor [0.0, 1.0) for sub-tick note position smoothing (default 0.0).
    // judged_notes: optional bool vector matching note_data size, true if note already judged (skip rendering).
    void render(SDL_Renderer* renderer, double song_position_ms, double global_time_ms, double render_alpha = 0.0, const std::vector<bool>* judged_notes = nullptr) const;

    // Render receptor indicators at the receptor line.
    // global_time_ms: wall-clock time for noteskin animation.
    // pressed_columns: bool array of size num_columns (nullable), true if panel pressed.
    // judge_trigger_times: double array of size num_columns (nullable), timestamp of last judge trigger.
    void render_receptors(SDL_Renderer* renderer,
                          double global_time_ms = 0.0,
                          const bool* pressed_columns = nullptr,
                          const double* judge_trigger_times = nullptr) const;

    // Trigger a hit effect for a column.
    // column: the column index (0-9)
    // tier: the judgment tier (Perfect, Great, Good, Bad, Miss)
    // global_time_ms: current wall-clock time for tracking effect duration
    void trigger_hit_effect(uint8_t column, JudgmentTier tier, double global_time_ms);

    // Render active hit effects (burst/flash overlays at receptor positions).
    // global_time_ms: current wall-clock time for fade-out calculation
    void render_hit_effects(SDL_Renderer* renderer, double global_time_ms) const;

    // Access config for external queries (e.g., judgment display positioning).
    const NoteFieldConfig& config() const;

private:
    const NoteData& note_data_;
    const TimingData& timing_data_;
    NoteFieldConfig config_;
    const NoteSkin* skin_;
    TextureCache* cache_;
    mutable std::vector<HitEffect> active_hit_effects_;
};

// Build a default single-mode config (5 columns centered in 640px).
NoteFieldConfig default_single_config();

// Per-column colors for placeholder rectangles (10 entries for double mode support).
extern const ColumnColor COLUMN_COLORS[10];

} // namespace openitup
