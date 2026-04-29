#pragma once

#include <cstdint>
#include <string>

namespace openitup {

enum class JudgmentTier : uint8_t {
    PERFECT = 0,
    GREAT = 1,
    GOOD = 2,
    BAD = 3,
    MISS = 4,
};

inline constexpr int JUDGMENT_TIER_COUNT = 5;

inline constexpr bool tier_maintains_combo(JudgmentTier t) {
    return t == JudgmentTier::PERFECT || t == JudgmentTier::GREAT || t == JudgmentTier::GOOD;
}

const char* judgment_tier_to_string(JudgmentTier tier);
JudgmentTier judgment_tier_from_string(const std::string& s);

} // namespace openitup
