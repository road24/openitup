#pragma once

#include <openitup/audio/audio_system.h>

#include <algorithm>

namespace openitup {

// Null audio backend for headless testing or builds without audio support.
// All operations succeed silently without producing sound.
class NullAudioSystem : public AudioSystem {
public:
    NullAudioSystem() = default;
    ~NullAudioSystem() override = default;

    // --- Lifecycle ---
    bool init() override { return true; }
    void shutdown() override {}

    // --- Music stream ---
    bool load_music(const std::filesystem::path& path) override {
        music_loaded_ = true;
        duration_ms_ = 180000.0;  // Fake 3-minute duration
        return true;
    }

    void play() override { state_ = AudioState::PLAYING; position_ms_ = 0.0; }
    void pause() override { state_ = AudioState::PAUSED; }
    void resume() override { state_ = AudioState::PLAYING; }
    void stop() override { state_ = AudioState::STOPPED; position_ms_ = 0.0; }

    void seek(double position_ms) override {
        position_ms_ = std::clamp(position_ms, 0.0, duration_ms_);
    }

    double get_position_ms() const override { return position_ms_; }
    double get_duration_ms() const override { return duration_ms_; }
    AudioState get_state() const override { return state_; }
    bool is_music_loaded() const override { return music_loaded_; }

    // --- Volume ---
    void set_music_volume(float volume) override {
        music_volume_ = std::clamp(volume, 0.0f, 1.0f);
    }
    float get_music_volume() const override { return music_volume_; }

    // --- SFX ---
    uint32_t load_sfx(const std::filesystem::path& path) override {
        return ++sfx_handle_counter_;
    }
    void play_sfx(uint32_t handle) override {}
    void set_sfx_volume(float volume) override {
        sfx_volume_ = std::clamp(volume, 0.0f, 1.0f);
    }
    float get_sfx_volume() const override { return sfx_volume_; }

private:
    bool music_loaded_ = false;
    AudioState state_ = AudioState::STOPPED;
    double position_ms_ = 0.0;
    double duration_ms_ = 0.0;
    float music_volume_ = 1.0f;
    float sfx_volume_ = 1.0f;
    uint32_t sfx_handle_counter_ = 0;
};

}  // namespace openitup
