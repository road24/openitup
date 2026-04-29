#pragma once

#include <functional>
#include <vector>

#include <SDL3/SDL_scancode.h>

#include <openitup/input/input_driver.h>
#include <openitup/input/pad_input.h>

namespace openitup {

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

private:
    std::vector<KeyMapping> keymap_;
    KeyboardStateFn state_fn_;
};

} // namespace openitup
