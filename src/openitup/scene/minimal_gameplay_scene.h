#pragma once

#include <filesystem>
#include <memory>

#include <openitup/audio/audio_system.h>
#include <openitup/chart/chart.h>
#include <openitup/chart/ksf_parser.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_system.h>
#include <openitup/judge/gameplay_state.h>
#include <openitup/judge/judge.h>
#include <openitup/render/judgment_display.h>
#include <openitup/render/note_renderer.h>

namespace openitup {

class MinimalGameplayScene {
public:
    // Construct the scene from a chart file. Loads chart, resolves audio, initializes subsystems.
    // Throws ChartLoadException if chart cannot be loaded.
    // audio_system may be nullptr (gameplay proceeds without audio).
    MinimalGameplayScene(
        const std::filesystem::path& chart_path,
        const std::filesystem::path& data_dir,
        AudioSystem* audio_system,
        InputSystem* input_system,
        Renderer* renderer);

    // Test-friendly constructor: accepts a pre-built Chart (no file parsing).
    MinimalGameplayScene(
        Chart chart,
        AudioSystem* audio_system,
        InputSystem* input_system,
        Renderer* renderer);

    ~MinimalGameplayScene();

    MinimalGameplayScene(const MinimalGameplayScene&) = delete;
    MinimalGameplayScene& operator=(const MinimalGameplayScene&) = delete;

    // Called once per fixed-step tick (60 Hz).
    // dt: fixed step duration (1/60th second).
    void update(double dt);

    // Called once per render frame (display refresh rate).
    // alpha: interpolation factor [0.0, 1.0) for sub-tick smoothing.
    void render(double alpha);

    // True if the song has completed (audio stopped or all notes judged).
    bool is_complete() const;

    // Access gameplay state (for future result screen or testing).
    const GameplayState& gameplay_state() const;

    // Access judge (for testing).
    const Judge& judge() const;

private:
    // Owned subsystems — constructed at scene init.
    Chart chart_;
    Judge judge_;
    GameplayState gameplay_state_;
    NoteRenderer note_renderer_;
    JudgmentDisplay judgment_display_;

    // Non-owning references to Engine subsystems.
    AudioSystem* audio_;
    InputSystem* input_;
    Renderer* renderer_;

    // Scene state.
    bool audio_started_ = false;
    bool complete_ = false;
    double last_song_ms_ = 0.0;

    // Press and judge overlay state for receptors.
    bool pressed_columns_[10] = {};
    double judge_trigger_times_[10] = {-1000.0, -1000.0, -1000.0, -1000.0, -1000.0,
                                        -1000.0, -1000.0, -1000.0, -1000.0, -1000.0};
    double global_time_ms_ = 0.0;
};

} // namespace openitup
