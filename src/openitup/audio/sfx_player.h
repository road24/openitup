#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include <SDL3/SDL.h>

#include <openitup/audio/sound_sample.h>

namespace openitup {

// Maximum number of simultaneous sound effect voices
constexpr int MAX_SFX_VOICES = 32;

// A single playing instance of a sound effect
struct SfxVoice {
    const SoundSample* sample = nullptr;  // Non-owning pointer to sample data
    size_t position = 0;                  // Current frame position in the sample
    float volume = 1.0f;                  // Per-voice volume multiplier
    bool active = false;                  // Whether this voice is currently playing
};

// Low-latency sound effect player using SDL3 audio streams.
// Mixes multiple concurrent sound samples into a single output stream.
// Thread-safe: play() can be called from any thread, mixing happens on audio thread.
class SfxPlayer {
public:
    SfxPlayer();
    ~SfxPlayer();

    SfxPlayer(const SfxPlayer&) = delete;
    SfxPlayer& operator=(const SfxPlayer&) = delete;

    // Initialize the SFX player with the given audio device.
    // Creates an SDL audio stream for mixing and binds it to the device.
    // Returns true on success, false on failure (logs ERROR).
    bool init(SDL_AudioDeviceID device_id, int sample_rate, int channels);

    // Shut down the SFX player. Safe to call multiple times.
    void shutdown();

    // Play a sound effect sample. Fire-and-forget.
    // The sample must remain valid for the duration of playback.
    // Volume range: 0.0 (silent) to 1.0 (full). Clamped.
    // Returns true if a voice was allocated, false if all voices are in use.
    bool play(const SoundSample& sample, float volume = 1.0f);

    // Set global SFX volume. Applied to all voices during mixing.
    // Range: 0.0 to 1.0. Clamped.
    void set_volume(float volume);

    // Get global SFX volume.
    float get_volume() const;

    // Check if the player is initialized.
    bool is_initialized() const { return initialized_; }

private:
    // SDL3 audio stream callback. Called on the audio thread.
    static void audio_callback(void* userdata, SDL_AudioStream* stream,
                               int additional_amount, int total_amount);

    // Mix active voices into output buffer. Called from audio_callback.
    void mix_audio(SDL_AudioStream* stream, int requested_bytes);

    // Find an inactive voice slot, or return nullptr if all voices are in use.
    SfxVoice* allocate_voice();

    // --- State ---

    SDL_AudioStream* stream_ = nullptr;
    SDL_AudioDeviceID device_id_ = 0;

    int sample_rate_ = 0;
    int channels_ = 0;

    // Voice pool for concurrent sound effects
    SfxVoice voices_[MAX_SFX_VOICES];

    // Mutex protects voice allocation (play() can be called from any thread)
    std::mutex voice_mutex_;

    // Global SFX volume (atomic for cross-thread access)
    std::atomic<float> volume_{1.0f};

    // Mix buffer for output (sized during init)
    std::vector<float> mix_buffer_;

    bool initialized_ = false;
};

}  // namespace openitup
