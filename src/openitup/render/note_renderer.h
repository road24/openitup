#pragma once

#include <cstdint>
#include <vector>

#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>

struct SDL_Renderer;

namespace openitup {

struct NoteFieldConfig {
    float receptor_y = 400.0f;
    float note_width = 48.0f;
    float note_height = 48.0f;
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
    NoteRenderer(const NoteData& note_data, const TimingData& timing_data,
                 const NoteFieldConfig& config);

    // Convert a beat position to a Y screen coordinate given the current song beat.
    // Pure function — testable without SDL.
    float beat_to_y(double note_beat, double current_beat) const;

    // Render all visible notes for the current song position.
    // song_position_ms: current audio playback position from AudioSystem.
    void render(SDL_Renderer* renderer, double song_position_ms) const;

    // Render receptor indicators at the receptor line.
    void render_receptors(SDL_Renderer* renderer) const;

    // Access config for external queries (e.g., judgment display positioning).
    const NoteFieldConfig& config() const;

private:
    const NoteData& note_data_;
    const TimingData& timing_data_;
    NoteFieldConfig config_;
};

// Build a default single-mode config (5 columns centered in 640px).
NoteFieldConfig default_single_config();

// Per-column colors for placeholder rectangles (10 entries for double mode support).
extern const ColumnColor COLUMN_COLORS[10];

} // namespace openitup
