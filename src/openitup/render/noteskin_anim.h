#pragma once

#include <cmath>

namespace openitup {

// Animation constants from noteskin-format-spec.md.
inline constexpr double NOTESKIN_FRAME_DURATION_MS = 50.0;
inline constexpr int    NOTESKIN_FRAME_COUNT = 6;
inline constexpr double NOTESKIN_LOOP_DURATION_MS =
    NOTESKIN_FRAME_DURATION_MS * NOTESKIN_FRAME_COUNT;  // 300.0

// Compute the looping normalized t for a global timestamp.
// Returns a value in [0.0, 1.0) that cycles every 300ms.
// Pure function — no state.
inline float noteskin_loop_t(double global_time_ms) {
    double phase = std::fmod(global_time_ms, NOTESKIN_LOOP_DURATION_MS);
    if (phase < 0.0) phase += NOTESKIN_LOOP_DURATION_MS;
    return static_cast<float>(phase / NOTESKIN_LOOP_DURATION_MS);
}

// Compute the one-shot normalized t for an animation that started at trigger_time_ms.
// Returns [0.0, 1.0] clamped, where 1.0 means the animation has finished.
inline float noteskin_oneshot_t(double global_time_ms, double trigger_time_ms) {
    double elapsed = global_time_ms - trigger_time_ms;
    if (elapsed < 0.0) return 0.0f;
    if (elapsed >= NOTESKIN_LOOP_DURATION_MS) return 1.0f;
    return static_cast<float>(elapsed / NOTESKIN_LOOP_DURATION_MS);
}

// True if a one-shot animation triggered at trigger_time_ms is still active.
inline bool noteskin_oneshot_active(double global_time_ms, double trigger_time_ms) {
    return (global_time_ms - trigger_time_ms) < NOTESKIN_LOOP_DURATION_MS
        && (global_time_ms >= trigger_time_ms);
}

} // namespace openitup
