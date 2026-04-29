#include <openitup/chart/play_mode.h>

#include <stdexcept>

namespace openitup {

const char* play_mode_to_string(PlayMode mode) {
    switch (mode) {
        case PlayMode::SINGLE: return "SINGLE";
        case PlayMode::DOUBLE: return "DOUBLE";
    }
    return "UNKNOWN";
}

PlayMode play_mode_from_string(const std::string& s) {
    if (s == "SINGLE") return PlayMode::SINGLE;
    if (s == "DOUBLE") return PlayMode::DOUBLE;

    throw std::invalid_argument("Unknown PlayMode string: " + s);
}

} // namespace openitup
