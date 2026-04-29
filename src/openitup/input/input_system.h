#pragma once

#include <memory>

#include <openitup/input/input_driver.h>
#include <openitup/input/input_snapshot.h>

namespace openitup {

class InputSystem {
public:
    explicit InputSystem(std::unique_ptr<InputDriver> driver);
    ~InputSystem();

    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;

    void poll(uint64_t tick_number);

    const InputSnapshot& snapshot() const { return current_; }
    const InputSnapshot& previous_snapshot() const { return previous_; }

    InputDriver& driver() { return *driver_; }
    const InputDriver& driver() const { return *driver_; }

private:
    std::unique_ptr<InputDriver> driver_;
    InputSnapshot current_;
    InputSnapshot previous_;
    uint32_t previous_held_ = 0;
};

} // namespace openitup
