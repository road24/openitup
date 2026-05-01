#include <openitup/scene/input_calibration_scene.h>

#include <numeric>

#include <spdlog/spdlog.h>

#include <openitup/core/engine.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>

namespace openitup {

InputCalibrationScene::InputCalibrationScene(Renderer* renderer,
                                             TextRenderer* text_renderer,
                                             SceneStack* scene_stack,
                                             Engine* engine)
    : renderer_(renderer),
      text_(text_renderer),
      stack_(scene_stack),
      engine_(engine),
      current_offset_ms_(0),
      timing_samples_(),
      metronome_tick_(0),
      time_accumulator_(0.0) {}

void InputCalibrationScene::on_enter() {
    spdlog::info("InputCalibrationScene entered");
    current_offset_ms_ = 0;
    timing_samples_.clear();
    metronome_tick_ = 0;
    time_accumulator_ = 0.0;
}

void InputCalibrationScene::on_exit() {
    spdlog::info("InputCalibrationScene exited");
}

void InputCalibrationScene::on_pause() {}
void InputCalibrationScene::on_resume() {}

void InputCalibrationScene::update(double dt) {
    // US-INP-073: Simple metronome at 120 BPM (0.5 seconds per beat)
    time_accumulator_ += dt;
    const double beat_duration = 0.5;

    if (time_accumulator_ >= beat_duration) {
        time_accumulator_ -= beat_duration;
        metronome_tick_++;
        play_metronome_sound();
    }
}

void InputCalibrationScene::handle_input(const InputSnapshot& input) {
    // US-INP-073 Scenario 2: Adjust offset in real-time
    if (input.is_pressed(PadInput::P1_UP_LEFT)) {
        current_offset_ms_ -= 5;
        spdlog::info("InputCalibrationScene: offset adjusted to {}ms", current_offset_ms_);
    }
    if (input.is_pressed(PadInput::P1_UP_RIGHT)) {
        current_offset_ms_ += 5;
        spdlog::info("InputCalibrationScene: offset adjusted to {}ms", current_offset_ms_);
    }

    // Capture hits for timing feedback
    if (input.is_pressed(PadInput::P1_CENTER)) {
        // US-INP-073 Scenario 1: Calculate and display raw vs adjusted timing
        // Simplified: assume hit is on current beat, calculate error based on time_accumulator_
        int raw_error_ms = static_cast<int>((time_accumulator_ - 0.25) * 1000.0);  // Target is 250ms into beat
        int adjusted_error_ms = raw_error_ms + current_offset_ms_;

        record_hit_timing(adjusted_error_ms);
        spdlog::info("InputCalibrationScene: raw={}ms, adjusted={}ms", raw_error_ms, adjusted_error_ms);
    }

    // Back to previous scene
    if (input.is_pressed(PadInput::BACK)) {
        spdlog::info("InputCalibrationScene: back pressed, returning to previous scene");
        stack_->pop();
    }

    // Save offset and exit
    if (input.is_pressed(PadInput::START)) {
        spdlog::info("InputCalibrationScene: saving offset {}ms and exiting", current_offset_ms_);
        // TODO: Save to settings
        stack_->pop();
    }
}

void InputCalibrationScene::render() {
    if (!renderer_ || !text_) return;

    // Title
    text_->draw_text("Input Calibration", 230, 40, SDL_Color{255, 255, 255, 255});

    render_timing_display();
    render_offset_controls();
    render_average_timing();
}

void InputCalibrationScene::render_timing_display() {
    // US-INP-073 Scenario 1: Display raw and adjusted timing
    std::string offset_str = "Current Offset: " + std::to_string(current_offset_ms_) + "ms";
    text_->draw_text(offset_str, 200, 100, SDL_Color{255, 255, 0, 255});

    // Show metronome beat indicator
    std::string beat_str = "Beat: " + std::to_string(metronome_tick_);
    text_->draw_text(beat_str, 260, 140, SDL_Color{200, 200, 200, 255});

    // Visual metronome indicator
    if (time_accumulator_ < 0.1) {
        text_->draw_text("BEAT!", 280, 180, SDL_Color{255, 0, 0, 255});
    }
}

void InputCalibrationScene::render_offset_controls() {
    text_->draw_text("Left/Right: Adjust Offset (-5/+5 ms)", 140, 250, SDL_Color{200, 200, 200, 255});
    text_->draw_text("Center: Hit on beat", 200, 290, SDL_Color{200, 200, 200, 255});
}

void InputCalibrationScene::render_average_timing() {
    // US-INP-073 Scenario 3: Display average timing
    if (!timing_samples_.empty()) {
        int avg = calculate_average_timing();
        std::string avg_str = "Average Timing (last " + std::to_string(timing_samples_.size()) + " hits): " + std::to_string(avg) + "ms";
        text_->draw_text(avg_str, 140, 350, SDL_Color{0, 255, 0, 255});
    }

    // Instructions
    text_->draw_text("Start: Save and Exit  Back: Cancel", 140, 420, SDL_Color{180, 180, 180, 255});
}

void InputCalibrationScene::play_metronome_sound() {
    // TODO: Play metronome click sound via audio system
}

void InputCalibrationScene::record_hit_timing(int error_ms) {
    timing_samples_.push_back(error_ms);
    if (timing_samples_.size() > 20) {
        timing_samples_.erase(timing_samples_.begin());
    }
}

int InputCalibrationScene::calculate_average_timing() const {
    if (timing_samples_.empty()) {
        return 0;
    }
    int sum = std::accumulate(timing_samples_.begin(), timing_samples_.end(), 0);
    return sum / static_cast<int>(timing_samples_.size());
}

} // namespace openitup
