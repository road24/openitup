#pragma once

#include <cstdint>
#include <string>

namespace openitup {

enum class NoteType : uint8_t {
    TAP = 0,
    HOLD_HEAD = 1,
    HOLD_TAIL = 2,
    MINE = 3,
    FAKE = 4,
    LIFT = 5,
};

const char* note_type_to_string(NoteType type);
NoteType note_type_from_string(const std::string& s);

} // namespace openitup
