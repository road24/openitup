#pragma once

#include <cstdint>

namespace openitup {

// US-JDG-018: Co-op life gauge mode
enum class CoopLifeMode : uint8_t {
    SHARED,    // Both players share one life gauge
    SEPARATE   // Each player has their own gauge, both must survive
};

} // namespace openitup
