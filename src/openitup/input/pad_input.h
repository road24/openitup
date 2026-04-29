#pragma once

#include <cstdint>
#include <string>

namespace openitup {

enum class PadInput : uint32_t {
    P1_DOWN_LEFT  = 1 << 0,
    P1_UP_LEFT    = 1 << 1,
    P1_CENTER     = 1 << 2,
    P1_UP_RIGHT   = 1 << 3,
    P1_DOWN_RIGHT = 1 << 4,
    P2_DOWN_LEFT  = 1 << 5,
    P2_UP_LEFT    = 1 << 6,
    P2_CENTER     = 1 << 7,
    P2_UP_RIGHT   = 1 << 8,
    P2_DOWN_RIGHT = 1 << 9,
    START         = 1 << 10,
    BACK          = 1 << 11,
    SELECT        = 1 << 12,
    COIN          = 1 << 13,
};

inline constexpr int PAD_INPUT_COUNT = 14;

inline constexpr PadInput ALL_PAD_INPUTS[] = {
    PadInput::P1_DOWN_LEFT,  PadInput::P1_UP_LEFT,    PadInput::P1_CENTER,
    PadInput::P1_UP_RIGHT,   PadInput::P1_DOWN_RIGHT,
    PadInput::P2_DOWN_LEFT,  PadInput::P2_UP_LEFT,    PadInput::P2_CENTER,
    PadInput::P2_UP_RIGHT,   PadInput::P2_DOWN_RIGHT,
    PadInput::START, PadInput::BACK, PadInput::SELECT, PadInput::COIN,
};

inline constexpr uint32_t operator|(PadInput a, PadInput b) {
    return static_cast<uint32_t>(a) | static_cast<uint32_t>(b);
}

inline constexpr uint32_t operator|(uint32_t a, PadInput b) {
    return a | static_cast<uint32_t>(b);
}

const char* pad_input_to_string(PadInput input);

} // namespace openitup
