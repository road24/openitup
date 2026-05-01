#include <openitup/input/input_merger.h>

#include <algorithm>

#include <openitup/input/hid_pad_driver.h>
#include <openitup/input/keyboard_driver.h>

namespace openitup {

uint32_t InputMerger::poll_held() {
    uint32_t merged = 0;

    if (strategy_ == MergeStrategy::OR_MERGE) {
        // US-INP-081: OR-merge all driver inputs
        for (InputDriver* driver : drivers_) {
            merged |= driver->poll_held();
        }
    } else if (strategy_ == MergeStrategy::FIRST_WINS) {
        // US-INP-082: Priority-based first-wins
        // Drivers are sorted by priority in add_driver
        for (InputDriver* driver : drivers_) {
            uint32_t driver_input = driver->poll_held();
            // For each input bit, if not already set, accept from this driver
            uint32_t new_bits = driver_input & ~merged;
            merged |= new_bits;
        }
    }

    return merged;
}

std::string InputMerger::device_name() const {
    if (drivers_.empty()) {
        return "InputMerger (no drivers)";
    }
    if (drivers_.size() == 1) {
        return drivers_[0]->device_name();
    }
    return "InputMerger (" + std::to_string(drivers_.size()) + " drivers)";
}

void InputMerger::add_driver(InputDriver* driver) {
    if (driver) {
        drivers_.push_back(driver);
        sort_by_priority();
    }
}

void InputMerger::remove_driver(InputDriver* driver) {
    drivers_.erase(
        std::remove(drivers_.begin(), drivers_.end(), driver),
        drivers_.end()
    );
}

void InputMerger::set_merge_strategy(MergeStrategy strategy) {
    strategy_ = strategy;
}

void InputMerger::sort_by_priority() {
    // US-INP-082: Sort drivers by priority (lower = higher priority)
    std::sort(drivers_.begin(), drivers_.end(), [this](InputDriver* a, InputDriver* b) {
        return get_driver_priority(a) < get_driver_priority(b);
    });
}

int InputMerger::get_driver_priority(InputDriver* driver) const {
    // US-INP-082 Scenario 2: Default priority order
    // Keyboard: 10, HID: 20, Arcade I/O: 30
    if (auto* hid = dynamic_cast<HidPadDriver*>(driver)) {
        return hid->priority();
    }
    if (dynamic_cast<KeyboardDriver*>(driver)) {
        return 10;  // Default keyboard priority
    }
    return 50;  // Default for unknown drivers
}

} // namespace openitup
