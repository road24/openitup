#include <openitup/input/hid_pad_driver.h>

#include <iomanip>
#include <sstream>

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <openitup/data/settings.h>

namespace openitup {

HidPadDriver::HidPadDriver()
    : HidPadDriver(default_button_map()) {}

HidPadDriver::HidPadDriver(std::vector<GamepadMapping> button_map)
    : gamepad_(nullptr), config_() {
    config_.button_map = std::move(button_map);

    int num_joysticks = 0;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&num_joysticks);

    if (!joysticks || num_joysticks == 0) {
        spdlog::info("HidPadDriver: no gamepads available");
        return;
    }

    // Try to open the first available gamepad
    for (int i = 0; i < num_joysticks; ++i) {
        if (SDL_IsGamepad(joysticks[i])) {
            gamepad_ = SDL_OpenGamepad(joysticks[i]);
            if (gamepad_) {
                const char* name = SDL_GetGamepadName(gamepad_);
                spdlog::info("HidPadDriver: opened gamepad \"{}\"", name ? name : "unknown");
                config_.vid_pid = vid_pid();
                break;
            }
        }
    }

    SDL_free(joysticks);

    if (!gamepad_) {
        spdlog::info("HidPadDriver: failed to open any gamepad");
    }
}

HidPadDriver::~HidPadDriver() {
    if (gamepad_) {
        SDL_CloseGamepad(gamepad_);
        gamepad_ = nullptr;
    }
}

uint32_t HidPadDriver::poll_held() {
    if (!gamepad_) {
        return 0;
    }

    uint32_t held = 0;

    // US-INP-032: Button mapping
    for (const auto& mapping : config_.button_map) {
        if (SDL_GetGamepadButton(gamepad_, mapping.button)) {
            held |= static_cast<uint32_t>(mapping.input);
        }
    }

    // US-INP-033: Analog axis support with threshold
    // Check all 6 axes (left stick X/Y, right stick X/Y, left/right triggers)
    for (int axis = 0; axis < 6; ++axis) {
        if (axis_pressed(axis)) {
            // Map axes to inputs based on common pad configurations
            // Left stick: axis 0 (X), axis 1 (Y)
            if (axis == SDL_GAMEPAD_AXIS_LEFTX) {
                int16_t value = SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTX);
                if (value < -config_.axis_threshold * 32767) {
                    held |= static_cast<uint32_t>(PadInput::P1_UP_LEFT);
                } else if (value > config_.axis_threshold * 32767) {
                    held |= static_cast<uint32_t>(PadInput::P1_UP_RIGHT);
                }
            } else if (axis == SDL_GAMEPAD_AXIS_LEFTY) {
                int16_t value = SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTY);
                if (value < -config_.axis_threshold * 32767) {
                    held |= static_cast<uint32_t>(PadInput::P1_DOWN_LEFT);
                } else if (value > config_.axis_threshold * 32767) {
                    held |= static_cast<uint32_t>(PadInput::P1_DOWN_RIGHT);
                }
            }
        }
    }

    return held;
}

std::string HidPadDriver::device_name() const {
    if (!gamepad_) {
        return "HID Pad (disconnected)";
    }
    const char* name = SDL_GetGamepadName(gamepad_);
    return name ? std::string(name) : "HID Pad";
}

void HidPadDriver::set_axis_threshold(float threshold) {
    config_.axis_threshold = threshold;
}

void HidPadDriver::set_player_assignment(PlayerAssignment player) {
    config_.player = player;
}

void HidPadDriver::set_latency_offset(int offset_ms) {
    config_.latency_offset_ms = offset_ms;
}

void HidPadDriver::set_priority(int priority) {
    config_.priority = priority;
}

std::string HidPadDriver::vid_pid() const {
    if (!gamepad_) {
        return "0000:0000";
    }

    uint16_t vendor = SDL_GetGamepadVendor(gamepad_);
    uint16_t product = SDL_GetGamepadProduct(gamepad_);

    std::ostringstream oss;
    oss << std::hex << std::setw(4) << std::setfill('0') << vendor
        << ":" << std::setw(4) << std::setfill('0') << product;
    return oss.str();
}

void HidPadDriver::load_config(const data::InputSettings& settings) {
    // US-INP-032: Load button mapping from settings
    // US-INP-033: Load axis threshold
    // US-INP-063: Load player assignment
    // US-INP-072: Load latency offset
    // US-INP-082: Load priority
    // TODO: Implement JSON deserialization once InputSettings schema is extended
    spdlog::info("HidPadDriver: config loading from settings (not yet implemented)");
}

void HidPadDriver::save_config(data::InputSettings& settings) const {
    // US-INP-032: Save button mapping to settings
    // US-INP-033: Save axis threshold
    // US-INP-063: Save player assignment
    // US-INP-072: Save latency offset
    // US-INP-082: Save priority
    // TODO: Implement JSON serialization once InputSettings schema is extended
    spdlog::info("HidPadDriver: config saving to settings (not yet implemented)");
}

std::vector<GamepadMapping> HidPadDriver::default_button_map() {
    return {
        // DPAD mapped to P1 panel corners
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PadInput::P1_UP_LEFT},
        {SDL_GAMEPAD_BUTTON_DPAD_UP, PadInput::P1_UP_RIGHT},
        {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PadInput::P1_DOWN_LEFT},
        {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PadInput::P1_DOWN_RIGHT},

        // Face buttons
        {SDL_GAMEPAD_BUTTON_SOUTH, PadInput::P1_CENTER},  // A/Cross

        // Menu buttons
        {SDL_GAMEPAD_BUTTON_START, PadInput::START},
        {SDL_GAMEPAD_BUTTON_BACK, PadInput::BACK},  // Select/Back
    };
}

bool HidPadDriver::axis_pressed(int axis_index) const {
    if (!gamepad_) {
        return false;
    }

    int16_t raw_value = SDL_GetGamepadAxis(gamepad_, static_cast<SDL_GamepadAxis>(axis_index));
    float normalized = raw_value / 32767.0f;
    return std::abs(normalized) > config_.axis_threshold;
}

} // namespace openitup
