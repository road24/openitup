#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL_gamepad.h>

#include <openitup/input/input_driver.h>
#include <openitup/input/pad_input.h>

namespace openitup {
namespace data {
    struct InputSettings;
}

enum class PlayerAssignment {
    P1,
    P2,
};

struct GamepadMapping {
    SDL_GamepadButton button;
    PadInput input;
};

struct HidDeviceConfig {
    std::string vid_pid;  // "1234:5678"
    std::vector<GamepadMapping> button_map;
    float axis_threshold = 0.5f;  // US-INP-033: analog sensor threshold
    PlayerAssignment player = PlayerAssignment::P1;  // US-INP-063: player binding
    int latency_offset_ms = 0;  // US-INP-072: calibration offset
    int priority = 20;  // US-INP-082: driver priority (lower = higher priority)
};

class HidPadDriver : public InputDriver {
public:
    HidPadDriver();
    explicit HidPadDriver(std::vector<GamepadMapping> button_map);
    ~HidPadDriver() override;

    HidPadDriver(const HidPadDriver&) = delete;
    HidPadDriver& operator=(const HidPadDriver&) = delete;

    uint32_t poll_held() override;
    std::string device_name() const override;

    bool is_connected() const { return gamepad_ != nullptr; }

    const std::vector<GamepadMapping>& button_map() const { return config_.button_map; }
    void set_button_map(std::vector<GamepadMapping> button_map) { config_.button_map = std::move(button_map); }

    // US-INP-033: Axis threshold configuration
    void set_axis_threshold(float threshold);
    float axis_threshold() const { return config_.axis_threshold; }

    // US-INP-063: Player assignment
    void set_player_assignment(PlayerAssignment player);
    PlayerAssignment player_assignment() const { return config_.player; }

    // US-INP-072: Latency offset
    void set_latency_offset(int offset_ms);
    int latency_offset() const { return config_.latency_offset_ms; }

    // US-INP-082: Priority
    void set_priority(int priority);
    int priority() const { return config_.priority; }

    // Device identification
    std::string vid_pid() const;

    // Configuration loading/saving
    void load_config(const data::InputSettings& settings);
    void save_config(data::InputSettings& settings) const;

    static std::vector<GamepadMapping> default_button_map();

private:
    SDL_Gamepad* gamepad_;
    HidDeviceConfig config_;

    // Helper: check if axis value exceeds threshold
    bool axis_pressed(int axis_index) const;
};

} // namespace openitup
