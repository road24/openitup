#include <openitup/input/pad_input.h>

namespace openitup {

const char* pad_input_to_string(PadInput input) {
    switch (input) {
        case PadInput::P1_DOWN_LEFT:  return "P1_DOWN_LEFT";
        case PadInput::P1_UP_LEFT:    return "P1_UP_LEFT";
        case PadInput::P1_CENTER:     return "P1_CENTER";
        case PadInput::P1_UP_RIGHT:   return "P1_UP_RIGHT";
        case PadInput::P1_DOWN_RIGHT: return "P1_DOWN_RIGHT";
        case PadInput::P2_DOWN_LEFT:  return "P2_DOWN_LEFT";
        case PadInput::P2_UP_LEFT:    return "P2_UP_LEFT";
        case PadInput::P2_CENTER:     return "P2_CENTER";
        case PadInput::P2_UP_RIGHT:   return "P2_UP_RIGHT";
        case PadInput::P2_DOWN_RIGHT: return "P2_DOWN_RIGHT";
        case PadInput::START:         return "START";
        case PadInput::BACK:          return "BACK";
        case PadInput::SELECT:        return "SELECT";
        case PadInput::COIN:          return "COIN";
    }
    return "UNKNOWN";
}

} // namespace openitup
