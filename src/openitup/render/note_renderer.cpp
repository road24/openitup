#include <openitup/render/note_renderer.h>

#include <SDL3/SDL.h>
#include <openitup/chart/note_type.h>

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
         + static_cast<float>(beat_delta) * config_.pixels_per_beat * config_.scroll_speed;
}

void NoteRenderer::render(SDL_Renderer* renderer, double song_position_ms) const {
    // Convert song position to current beat
    double current_beat = timing_data_.beat_at_time(song_position_ms / 1000.0);

    // Compute visible beat range (notes scroll bottom-to-top)
    double beats_below_receptor = config_.receptor_y / (config_.pixels_per_beat * config_.scroll_speed);
    double beats_above_receptor = (480.0f - config_.receptor_y) / (config_.pixels_per_beat * config_.scroll_speed);
    double top_beat = current_beat + beats_above_receptor;
    double bottom_beat = current_beat - beats_below_receptor;

    // Get notes in visible range
    auto [begin_it, end_it] = note_data_.notes_in_range(bottom_beat, top_beat);

    // Render each visible note
    for (auto it = begin_it; it != end_it; ++it) {
        const auto& note = *it;
        // Skip non-TAP and non-HOLD_HEAD notes
        if (note.type != NoteType::TAP && note.type != NoteType::HOLD_HEAD) {
            continue;
        }

        // Skip if column is out of range
        if (note.column >= config_.num_columns) {
            continue;
        }

        // Compute screen position
        float y = beat_to_y(note.beat, current_beat);

        // Skip if offscreen (with margin for note height)
        if (y < -config_.note_height || y > 480.0f + config_.note_height) {
            continue;
        }

        float x = config_.column_x[note.column] - config_.note_width / 2.0f;

        // Set color from column
        const auto& color = COLUMN_COLORS[note.column];
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

        // Draw filled rectangle
        SDL_FRect rect;
        rect.x = x;
        rect.y = y - config_.note_height / 2.0f;
        rect.w = config_.note_width;
        rect.h = config_.note_height;
        SDL_RenderFillRect(renderer, &rect);
    }
}

void NoteRenderer::render_receptors(SDL_Renderer* renderer) const {
    // Set draw color to dim gray
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 180);

    // Draw outlined rectangle at receptor position for each column
    for (int col = 0; col < config_.num_columns; col++) {
        SDL_FRect rect;
        rect.x = config_.column_x[col] - config_.note_width / 2.0f;
        rect.y = config_.receptor_y - config_.note_height / 2.0f;
        rect.w = config_.note_width;
        rect.h = config_.note_height;
        SDL_RenderRect(renderer, &rect);
    }
}

const NoteFieldConfig& NoteRenderer::config() const {
    return config_;
}

} // namespace openitup
