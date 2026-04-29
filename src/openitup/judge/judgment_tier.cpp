#include <openitup/judge/judgment_tier.h>

#include <stdexcept>

namespace openitup {

const char* judgment_tier_to_string(JudgmentTier tier) {
    switch (tier) {
        case JudgmentTier::PERFECT: return "PERFECT";
        case JudgmentTier::GREAT: return "GREAT";
        case JudgmentTier::GOOD: return "GOOD";
        case JudgmentTier::BAD: return "BAD";
        case JudgmentTier::MISS: return "MISS";
    }
    return "UNKNOWN";
}

JudgmentTier judgment_tier_from_string(const std::string& s) {
    if (s == "PERFECT") return JudgmentTier::PERFECT;
    if (s == "GREAT") return JudgmentTier::GREAT;
    if (s == "GOOD") return JudgmentTier::GOOD;
    if (s == "BAD") return JudgmentTier::BAD;
    if (s == "MISS") return JudgmentTier::MISS;

    throw std::invalid_argument("Unknown JudgmentTier string: " + s);
}

} // namespace openitup
