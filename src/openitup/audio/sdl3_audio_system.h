#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include <SDL3/SDL.h>

#include <openitup/audio/audio_system.h>
#include <openitup/audio/audio_decoder.h>

namespace openitup {

class SDL3AudioSystem : public AudioSystem {
public:
    SDL3AudioSystem();
    ~SDL3AudioSystem() override;

    SDL3AudioSystem(const SDL3AudioSystem&) = delete;
    SDL3AudioSystem& operator=(const SDL3AudioSystem&) = delete;

    // --- AudioSystem interface ---

    bool init() override;
    void shutdown() override;

    bool load_music(const std::filesystem::path& path) override;
    void play() override;
    void pause() override;
    void resume() override;
    void stop() override;
    void seek(double position_ms) override;

    double get_position_ms() const override;
    double get_duration_ms() const override;
    AudioState get_state() const override;
    bool is_music_loaded() const override;

    void set_music_volume(float volume) override;
    float get_music_volume() const override;

    // Phase 3 stubs
    uint32_t load_sfx(const std::filesystem::path& path) override;
    void play_sfx(uint32_t handle) override;
    void set_sfx_volume(float volume) override;
    float get_sfx_volume() const override;

private:
    // SDL3 audio stream callback. Called on the audio thread.
    static void audio_callback(void* userdata, SDL_AudioStream* stream,
                               int additional_amount, int total_amount);

    // Feed decoded audio data to the SDL stream. Called from audio_callback.
    void feed_audio(SDL_AudioStream* stream, int requested_bytes);

    // Create an AudioDecoder for the given file based on extension.
    static std::unique_ptr<AudioDecoder> create_decoder(
        const std::filesystem::path& path);

    // --- State ---

    SDL_AudioStream* stream_ = nullptr;
    SDL_AudioDeviceID device_id_ = 0;

    std::unique_ptr<AudioDecoder> decoder_;
    AudioFormat source_format_ = {};

    // Output device format (for resampling correction in position tracking).
    int output_sample_rate_ = 0;
    int output_channels_ = 0;

    // Transport state (main-thread owned, atomic for cross-thread reads).
    std::atomic<AudioState> state_{AudioState::STOPPED};

    // --- Sample-accurate position tracking ---

    // Samples fed to the SDL audio stream (per-channel count).
    // Written by audio callback thread.
    std::atomic<int64_t> samples_fed_{0};

    // Base sample offset after seek. When we seek to sample N,
    // we set seek_base_ = N and reset samples_fed_ = 0.
    // Position = seek_base_ + samples_fed_ - queued_samples.
    std::atomic<int64_t> seek_base_{0};

    // Position snapshot frozen at pause time (in milliseconds).
    // Avoids returning a stale position while the stream drains.
    double paused_position_ms_ = 0.0;

    // --- Decode buffer ---

    // Intermediate buffer for decoding. Sized for one callback worth of data.
    std::vector<float> decode_buffer_;

    // --- Volume ---

    std::atomic<float> music_volume_{1.0f};
    std::atomic<float> sfx_volume_{1.0f};

    // --- Mutex for seek/load operations ---
    // Protects decoder_ and state transitions that must be atomic
    // with respect to the audio callback.
    std::mutex music_mutex_;

    bool initialized_ = false;
};

} // namespace openitup
