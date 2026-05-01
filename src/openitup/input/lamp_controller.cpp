#include <openitup/input/lamp_controller.h>

#include <openitup/input/piuio_driver.h>
#include <openitup/judge/judgment_tier.h>

namespace openitup {

LampController::LampController(PiuioDriver* driver)
    : driver_(driver) {
    init_default_durations();
}

void LampController::set_lamp(int column, bool on) {
    if (!driver_) {
        return;
    }

    driver_->set_lamp(column, on);
}

void LampController::flash_on_judgment(int column, JudgmentTier tier) {
    if (!driver_ || column < 0 || column >= 10) {
        return;
    }

    // Get flash duration for this tier
    uint64_t duration = flash_duration_ms(tier);

    // Record when this flash should expire (will be set by update())
    // For now, mark it as active with duration 0 as placeholder
    active_flashes_[column] = duration;
}

void LampController::update(uint64_t current_time_ms) {
    if (!driver_) {
        return;
    }

    // Process active flashes
    std::vector<int> expired;
    for (auto& [column, expiration_time] : active_flashes_) {
        if (expiration_time == 0) {
            // New flash, set expiration time
            active_flashes_[column] = current_time_ms + flash_durations_[JudgmentTier::PERFECT];
            driver_->set_lamp(column, true);
        } else if (current_time_ms >= expiration_time) {
            // Flash expired, turn off lamp
            driver_->set_lamp(column, false);
            expired.push_back(column);
        } else {
            // Flash still active
            driver_->set_lamp(column, true);
        }
    }

    // Remove expired flashes
    for (int column : expired) {
        active_flashes_.erase(column);
    }

    // Send updated lamp state to hardware
    driver_->update_lamps();
}

uint64_t LampController::flash_duration_ms(JudgmentTier tier) const {
    auto it = flash_durations_.find(tier);
    if (it != flash_durations_.end()) {
        return it->second;
    }
    return 150; // Default 150ms
}

void LampController::set_flash_duration(JudgmentTier tier, uint64_t duration_ms) {
    flash_durations_[tier] = duration_ms;
}

void LampController::init_default_durations() {
    // Default flash durations based on judgment quality
    // Better judgments get longer flashes for visual feedback
    flash_durations_[JudgmentTier::PERFECT] = 200;
    flash_durations_[JudgmentTier::GREAT] = 150;
    flash_durations_[JudgmentTier::GOOD] = 100;
    flash_durations_[JudgmentTier::BAD] = 75;
    flash_durations_[JudgmentTier::MISS] = 50;
}

} // namespace openitup
