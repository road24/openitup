#pragma once

#include <cstdint>

#include <openitup/input/pad_input.h>

namespace openitup {

class InputSnapshot {
public:
    InputSnapshot(uint32_t held, uint32_t pressed, uint32_t released,
                  uint64_t tick_number);
    InputSnapshot();

    bool is_held(PadInput input) const;
    bool is_pressed(PadInput input) const;
    bool is_released(PadInput input) const;

    uint32_t held_mask() const { return held_; }
    uint32_t pressed_mask() const { return pressed_; }
    uint32_t released_mask() const { return released_; }
    uint64_t tick_number() const { return tick_number_; }
    bool empty() const;

    // Get per-player snapshot (0 = P1, 1 = P2)
    // Filters the full snapshot to only include inputs for the specified player
    InputSnapshot get_player_snapshot(int player) const;

private:
    uint32_t held_ = 0;
    uint32_t pressed_ = 0;
    uint32_t released_ = 0;
    uint64_t tick_number_ = 0;
};

} // namespace openitup
