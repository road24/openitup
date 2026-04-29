#include <openitup/render/note_renderer.h>

#include <SDL3/SDL.h>

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
                           const NoteFieldConfig& config)
    : note_data_(note_data), timing_data_(timing_data), config_(config) {
}

float NoteRenderer::beat_to_y(double note_beat, double current_beat) const {
    double beat_delta = note_beat - current_beat;
    return config_.receptor_y
         - static_cast<float>(beat_delta) * config_.pixels_per_beat * config_.scroll_speed;
}

void NoteRenderer::render(SDL_Renderer* renderer, double song_position_ms) const {
    // Stub implementation — Step 2
}

void NoteRenderer::render_receptors(SDL_Renderer* renderer) const {
    // Stub implementation — Step 2
}

const NoteFieldConfig& NoteRenderer::config() const {
    return config_;
}

} // namespace openitup
