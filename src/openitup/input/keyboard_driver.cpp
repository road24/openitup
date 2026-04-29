#include <openitup/input/keyboard_driver.h>

#include <SDL3/SDL.h>

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

} // namespace openitup
