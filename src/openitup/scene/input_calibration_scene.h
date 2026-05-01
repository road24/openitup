#pragma once

#include <vector>

#include <openitup/scene/scene.h>

namespace openitup {

class Renderer;
class TextRenderer;
class SceneStack;
class Engine;

// US-INP-073: Input calibration feedback screen
class InputCalibrationScene : public Scene {
public:
    InputCalibrationScene(Renderer* renderer,
                          TextRenderer* text_renderer,
                          SceneStack* scene_stack,
                          Engine* engine);

    void on_enter() override;
    void on_exit() override;
    void on_pause() override;
    void on_resume() override;

    void update(double dt) override;
    void handle_input(const InputSnapshot& input) override;
    void render() override;

private:
    Renderer* renderer_;
    TextRenderer* text_;
    SceneStack* stack_;
    Engine* engine_;

    int current_offset_ms_;  // Current calibration offset
    std::vector<int> timing_samples_;  // Last 20 timing errors in ms
    int metronome_tick_;  // Current metronome beat
    double time_accumulator_;  // Time since last beat

    void render_timing_display();
    void render_offset_controls();
    void render_average_timing();
    void play_metronome_sound();
    void record_hit_timing(int error_ms);
    int calculate_average_timing() const;
};

} // namespace openitup
