#include <openitup/input/input_system.h>

#include <spdlog/spdlog.h>

namespace openitup {

InputSystem::InputSystem(std::unique_ptr<InputDriver> driver)
    : driver_(std::move(driver)) {
    spdlog::info("input system initialized: {}", driver_->device_name());
}

InputSystem::~InputSystem() = default;

void InputSystem::poll(uint64_t tick_number) {
    previous_ = current_;

    uint32_t held = driver_->poll_held();
    uint32_t pressed = held & ~previous_held_;
    uint32_t released = ~held & previous_held_;

    current_ = InputSnapshot(held, pressed, released, tick_number);
    previous_held_ = held;
}

} // namespace openitup
