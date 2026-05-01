#include <openitup/input/keyboard_driver.h>

#include <cctype>
#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <openitup/data/settings.h>

namespace openitup {

KeyboardDriver::KeyboardDriver()
    : keymap_(default_keymap()),
      state_fn_([](int* numkeys) -> const bool* {
          return SDL_GetKeyboardState(numkeys);
      }) {}

KeyboardDriver::KeyboardDriver(std::vector<KeyMapping> keymap, KeyboardStateFn state_fn)
    : keymap_(std::move(keymap)), state_fn_(std::move(state_fn)) {}

uint32_t KeyboardDriver::poll_held() {
    int numkeys = 0;
    const bool* state = state_fn_(&numkeys);
    if (!state) return 0;

    uint32_t held = 0;
    for (const auto& mapping : keymap_) {
        if (static_cast<int>(mapping.scancode) < numkeys && state[mapping.scancode]) {
            held |= static_cast<uint32_t>(mapping.input);
        }
    }
    return held;
}

std::string KeyboardDriver::device_name() const {
    return "Keyboard";
}

std::vector<KeyMapping> KeyboardDriver::default_keymap() {
    return {
        {SDL_SCANCODE_Q, PadInput::P1_UP_LEFT},
        {SDL_SCANCODE_E, PadInput::P1_UP_RIGHT},
        {SDL_SCANCODE_S, PadInput::P1_CENTER},
        {SDL_SCANCODE_Z, PadInput::P1_DOWN_LEFT},
        {SDL_SCANCODE_C, PadInput::P1_DOWN_RIGHT},
        {SDL_SCANCODE_RETURN, PadInput::START},
        {SDL_SCANCODE_ESCAPE, PadInput::BACK},
        {SDL_SCANCODE_SPACE, PadInput::SELECT},
    };
}

void KeyboardDriver::load_keymap(const data::InputSettings& settings) {
    std::vector<KeyMapping> new_keymap;

    for (const auto& [scancode_str, pad_input_str] : settings.keymap) {
        SDL_Scancode scancode = string_to_scancode(scancode_str);
        const PadInput* pad_input = string_to_pad_input(pad_input_str);

        if (scancode == SDL_SCANCODE_UNKNOWN) {
            spdlog::warn("Invalid scancode in keymap: {}", scancode_str);
            continue;
        }

        if (pad_input == nullptr) {
            spdlog::warn("Invalid pad input in keymap: {}", pad_input_str);
            continue;
        }

        new_keymap.push_back({scancode, *pad_input});
    }

    if (new_keymap.empty()) {
        spdlog::warn("Keymap is empty after parsing, using defaults");
        keymap_ = default_keymap();
    } else {
        keymap_ = std::move(new_keymap);
    }
}

data::InputSettings KeyboardDriver::save_keymap() const {
    data::InputSettings settings;

    for (const auto& mapping : keymap_) {
        std::string scancode_str = scancode_to_string(mapping.scancode);
        std::string pad_input_str = pad_input_to_string(mapping.input);
        settings.keymap[scancode_str] = pad_input_str;
    }

    return settings;
}

std::string KeyboardDriver::scancode_to_string(SDL_Scancode scancode) {
    const char* name = SDL_GetScancodeName(scancode);
    if (name && name[0] != '\0') {
        // SDL_GetScancodeName returns human-readable names like "Q" or "Return"
        // We need to convert to scancode constant names like "SDL_SCANCODE_Q"
        std::string sdl_name = name;

        // Convert to uppercase to match our constant naming convention
        for (char& c : sdl_name) {
            c = std::toupper(static_cast<unsigned char>(c));
        }

        return std::string("SDL_SCANCODE_") + sdl_name;
    }
    return "SDL_SCANCODE_UNKNOWN";
}

SDL_Scancode KeyboardDriver::string_to_scancode(const std::string& name) {
    // Map common scancode names to their values
    static const std::map<std::string, SDL_Scancode> scancode_map = {
        {"SDL_SCANCODE_Q", SDL_SCANCODE_Q},
        {"SDL_SCANCODE_W", SDL_SCANCODE_W},
        {"SDL_SCANCODE_E", SDL_SCANCODE_E},
        {"SDL_SCANCODE_A", SDL_SCANCODE_A},
        {"SDL_SCANCODE_S", SDL_SCANCODE_S},
        {"SDL_SCANCODE_D", SDL_SCANCODE_D},
        {"SDL_SCANCODE_Z", SDL_SCANCODE_Z},
        {"SDL_SCANCODE_X", SDL_SCANCODE_X},
        {"SDL_SCANCODE_C", SDL_SCANCODE_C},
        {"SDL_SCANCODE_RETURN", SDL_SCANCODE_RETURN},
        {"SDL_SCANCODE_ESCAPE", SDL_SCANCODE_ESCAPE},
        {"SDL_SCANCODE_SPACE", SDL_SCANCODE_SPACE},
        {"SDL_SCANCODE_UP", SDL_SCANCODE_UP},
        {"SDL_SCANCODE_DOWN", SDL_SCANCODE_DOWN},
        {"SDL_SCANCODE_LEFT", SDL_SCANCODE_LEFT},
        {"SDL_SCANCODE_RIGHT", SDL_SCANCODE_RIGHT},
        // Support SDL_GetScancodeName casing variants
        {"SDL_SCANCODE_Return", SDL_SCANCODE_RETURN},
        {"SDL_SCANCODE_Escape", SDL_SCANCODE_ESCAPE},
        {"SDL_SCANCODE_Space", SDL_SCANCODE_SPACE},
    };

    auto it = scancode_map.find(name);
    if (it != scancode_map.end()) {
        return it->second;
    }

    return SDL_SCANCODE_UNKNOWN;
}

const PadInput* KeyboardDriver::string_to_pad_input(const std::string& name) {
    // Map string names to PadInput enum values
    static const std::map<std::string, PadInput> pad_input_map = {
        {"P1_DOWN_LEFT", PadInput::P1_DOWN_LEFT},
        {"P1_UP_LEFT", PadInput::P1_UP_LEFT},
        {"P1_CENTER", PadInput::P1_CENTER},
        {"P1_UP_RIGHT", PadInput::P1_UP_RIGHT},
        {"P1_DOWN_RIGHT", PadInput::P1_DOWN_RIGHT},
        {"P2_DOWN_LEFT", PadInput::P2_DOWN_LEFT},
        {"P2_UP_LEFT", PadInput::P2_UP_LEFT},
        {"P2_CENTER", PadInput::P2_CENTER},
        {"P2_UP_RIGHT", PadInput::P2_UP_RIGHT},
        {"P2_DOWN_RIGHT", PadInput::P2_DOWN_RIGHT},
        {"START", PadInput::START},
        {"BACK", PadInput::BACK},
        {"SELECT", PadInput::SELECT},
        {"COIN", PadInput::COIN},
    };

    auto it = pad_input_map.find(name);
    if (it != pad_input_map.end()) {
        return &it->second;
    }

    return nullptr;
}

} // namespace openitup
