#pragma once

#include <cstdint>
#include <string>

namespace openitup {

enum class PlayMode : uint8_t {
    SINGLE = 0,  // 5 panels, columns 0-4
    DOUBLE = 1,  // 10 panels, columns 0-9
    HALF = 2,    // Half-double mode
};

// Maximum valid column index for the mode.
inline constexpr int max_columns(PlayMode mode) {
    return mode == PlayMode::DOUBLE ? 10 : 5;
}

const char* play_mode_to_string(PlayMode mode);
PlayMode play_mode_from_string(const std::string& s);

} // namespace openitup
