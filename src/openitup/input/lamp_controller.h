#pragma once

#include <chrono>
#include <cstdint>
#include <map>

namespace openitup {

class PiuioDriver;

enum class JudgmentTier : uint8_t;

class LampController {
public:
    explicit LampController(PiuioDriver* driver);

    // Set lamp state directly
    void set_lamp(int column, bool on);

    // Flash lamp on note judgment
    void flash_on_judgment(int column, JudgmentTier tier);

    // Update lamp timers and send state to hardware
    void update(uint64_t current_time_ms);

    // Get configured flash duration for a judgment tier
    uint64_t flash_duration_ms(JudgmentTier tier) const;

    // Set flash duration for a judgment tier
    void set_flash_duration(JudgmentTier tier, uint64_t duration_ms);

private:
    PiuioDriver* driver_;

    // Active lamp flash timers (column -> expiration time in ms)
    std::map<int, uint64_t> active_flashes_;

    // Flash duration per judgment tier (in milliseconds)
    std::map<JudgmentTier, uint64_t> flash_durations_;

    void init_default_durations();
};

} // namespace openitup
