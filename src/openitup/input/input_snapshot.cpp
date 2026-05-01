#include <openitup/input/input_snapshot.h>

namespace openitup {

InputSnapshot::InputSnapshot(uint32_t held, uint32_t pressed, uint32_t released,
                             uint64_t tick_number)
    : held_(held), pressed_(pressed), released_(released), tick_number_(tick_number) {}

InputSnapshot::InputSnapshot() = default;

bool InputSnapshot::is_held(PadInput input) const {
    return (held_ & static_cast<uint32_t>(input)) != 0;
}

bool InputSnapshot::is_pressed(PadInput input) const {
    return (pressed_ & static_cast<uint32_t>(input)) != 0;
}

bool InputSnapshot::is_released(PadInput input) const {
    return (released_ & static_cast<uint32_t>(input)) != 0;
}

bool InputSnapshot::empty() const {
    return held_ == 0 && pressed_ == 0 && released_ == 0;
}

InputSnapshot InputSnapshot::get_player_snapshot(int player) const {
    // P1 uses bits 0-4 (panels) + START/BACK/SELECT/COIN (bits 10-13)
    // P2 uses bits 5-9 (panels) + START/BACK/SELECT/COIN (bits 10-13)
    // Menu inputs (START, BACK, SELECT, COIN) are shared across both players

    constexpr uint32_t P1_PANEL_MASK = 0x1F;        // Bits 0-4: P1 panels
    constexpr uint32_t P2_PANEL_MASK = 0x3E0;       // Bits 5-9: P2 panels
    constexpr uint32_t MENU_MASK = 0x3C00;          // Bits 10-13: START, BACK, SELECT, COIN

    uint32_t mask;
    if (player == 0) {
        // P1: panels 0-4 + menu inputs
        mask = P1_PANEL_MASK | MENU_MASK;
    } else {
        // P2: panels 5-9 + menu inputs
        mask = P2_PANEL_MASK | MENU_MASK;
    }

    return InputSnapshot(
        held_ & mask,
        pressed_ & mask,
        released_ & mask,
        tick_number_
    );
}

} // namespace openitup
