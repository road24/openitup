#include <openitup/chart/note_type.h>

#include <stdexcept>

namespace openitup {

const char* note_type_to_string(NoteType type) {
    switch (type) {
        case NoteType::TAP: return "TAP";
        case NoteType::HOLD_HEAD: return "HOLD_HEAD";
        case NoteType::HOLD_TAIL: return "HOLD_TAIL";
        case NoteType::MINE: return "MINE";
        case NoteType::FAKE: return "FAKE";
        case NoteType::LIFT: return "LIFT";
    }
    return "UNKNOWN";
}

NoteType note_type_from_string(const std::string& s) {
    if (s == "TAP") return NoteType::TAP;
    if (s == "HOLD_HEAD") return NoteType::HOLD_HEAD;
    if (s == "HOLD_TAIL") return NoteType::HOLD_TAIL;
    if (s == "MINE") return NoteType::MINE;
    if (s == "FAKE") return NoteType::FAKE;
    if (s == "LIFT") return NoteType::LIFT;

    throw std::invalid_argument("Unknown NoteType string: " + s);
}

} // namespace openitup
