#pragma once

#include <cstdint>
#include <string>

namespace openitup {

class InputDriver {
public:
    virtual ~InputDriver() = default;
    virtual uint32_t poll_held() = 0;
    virtual std::string device_name() const = 0;
};

} // namespace openitup
