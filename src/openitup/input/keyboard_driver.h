#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <SDL3/SDL_scancode.h>

#include <openitup/input/input_driver.h>
#include <openitup/input/pad_input.h>

namespace openitup {
namespace data {
    struct InputSettings;
}

using KeyboardStateFn = std::function<const bool*(int*)>;

struct KeyMapping {
    SDL_Scancode scancode;
    PadInput input;
};

class KeyboardDriver : public InputDriver {
public:
    KeyboardDriver();
    KeyboardDriver(std::vector<KeyMapping> keymap, KeyboardStateFn state_fn);

    uint32_t poll_held() override;
    std::string device_name() const override;

    const std::vector<KeyMapping>& keymap() const { return keymap_; }
    void set_keymap(std::vector<KeyMapping> keymap) { keymap_ = std::move(keymap); }

    static std::vector<KeyMapping> default_keymap();

    // Load keymap from settings
    void load_keymap(const data::InputSettings& settings);

    // Export current keymap to settings format
    data::InputSettings save_keymap() const;

    // Convert SDL_Scancode to string name (e.g., SDL_SCANCODE_Q -> "SDL_SCANCODE_Q")
    static std::string scancode_to_string(SDL_Scancode scancode);

    // Convert string name to SDL_Scancode (returns SDL_SCANCODE_UNKNOWN if invalid)
    static SDL_Scancode string_to_scancode(const std::string& name);

    // Convert string name to PadInput (returns nullptr if invalid)
    static const PadInput* string_to_pad_input(const std::string& name);

private:
    std::vector<KeyMapping> keymap_;
    KeyboardStateFn state_fn_;
};

} // namespace openitup
