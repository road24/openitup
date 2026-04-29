#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace openitup {

enum class AudioState : uint8_t {
    STOPPED,
    PLAYING,
    PAUSED
};

class AudioSystem {
public:
    virtual ~AudioSystem() = default;

    // --- Lifecycle ---

    // Initialize the audio backend. Returns true on success.
    // On failure, logs ERROR and returns false (engine continues without audio).
    virtual bool init() = 0;

    // Shut down the audio backend. Safe to call multiple times.
    virtual void shutdown() = 0;

    // --- Music stream ---

    // Load a music file (OGG or MP3). Replaces any currently loaded music.
    // Returns true on success. On failure, logs ERROR with path and reason.
    virtual bool load_music(const std::filesystem::path& path) = 0;

    // Transport controls.
    virtual void play() = 0;      // Play from beginning (position 0).
    virtual void pause() = 0;     // Pause at current position.
    virtual void resume() = 0;    // Resume from paused position.
    virtual void stop() = 0;      // Stop and reset position to 0.

    // Seek to a millisecond position. Clamps to [0, duration].
    // Works in any state (PLAYING, PAUSED, STOPPED).
    virtual void seek(double position_ms) = 0;

    // THE critical method. Returns playback position in milliseconds based
    // on samples consumed by audio hardware (not samples submitted).
    // Returns 0.0 if stopped. Returns frozen position if paused.
    virtual double get_position_ms() const = 0;

    // Duration of loaded music in milliseconds. Returns 0.0 if no music loaded.
    virtual double get_duration_ms() const = 0;

    // Current transport state.
    virtual AudioState get_state() const = 0;

    // Is a music file currently loaded (regardless of play state)?
    virtual bool is_music_loaded() const = 0;

    // --- Volume ---

    // Set music volume. Range: 0.0 (silent) to 1.0 (full). Clamped.
    virtual void set_music_volume(float volume) = 0;

    // Get current music volume.
    virtual float get_music_volume() const = 0;

    // --- SFX (Phase 3 — declared here for interface stability) ---

    // Load a short audio sample for fire-and-forget playback.
    // Returns an opaque handle, or 0 on failure.
    virtual uint32_t load_sfx(const std::filesystem::path& path) = 0;

    // Play a loaded SFX sample. Fire-and-forget.
    virtual void play_sfx(uint32_t handle) = 0;

    // Set SFX volume. Range: 0.0 to 1.0. Clamped.
    virtual void set_sfx_volume(float volume) = 0;

    // Get current SFX volume.
    virtual float get_sfx_volume() const = 0;
};

}  // namespace openitup
