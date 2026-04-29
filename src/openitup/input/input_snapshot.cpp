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

} // namespace openitup
