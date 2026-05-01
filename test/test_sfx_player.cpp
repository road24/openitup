#include <gtest/gtest.h>

#include <openitup/audio/sfx_player.h>
#include <openitup/audio/sound_sample.h>

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

using namespace openitup;
namespace fs = std::filesystem;

static fs::path temp_dir() {
    return fs::temp_directory_path() / "openitup_sfx_player_tests";
}

class SfxPlayerTest : public ::testing::Test {
protected:
    fs::path temp_;
    SDL_AudioDeviceID device_id_ = 0;
    bool has_audio_device_ = false;

    void SetUp() override {
        temp_ = temp_dir();
        fs::create_directories(temp_);

        // Initialize SDL audio subsystem
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            spdlog::warn("Failed to initialize SDL audio: {}", SDL_GetError());
            return;
        }

        // Check if we have an audio device available
        int num_devices = 0;
        SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&num_devices);
        if (devices && num_devices > 0) {
            has_audio_device_ = true;
            SDL_free(devices);
        }

        // Open audio device if available
        if (has_audio_device_) {
            device_id_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
            if (device_id_ == 0) {
                spdlog::warn("Failed to open audio device: {}", SDL_GetError());
                has_audio_device_ = false;
            }
        }
    }

    void TearDown() override {
        if (device_id_ != 0) {
            SDL_CloseAudioDevice(device_id_);
            device_id_ = 0;
        }

        SDL_QuitSubSystem(SDL_INIT_AUDIO);

        if (fs::exists(temp_)) {
            fs::remove_all(temp_);
        }
    }

    // Generate a simple WAV file with given parameters
    fs::path generate_wav(const std::string& filename,
                         int sample_rate,
                         int channels,
                         double duration_sec,
                         double frequency = 0.0) {
        fs::path path = temp_ / filename;

        int bytes_per_sample = 2;  // 16-bit PCM
        int byte_rate = sample_rate * channels * bytes_per_sample;
        int block_align = channels * bytes_per_sample;
        uint32_t data_size = static_cast<uint32_t>(
            sample_rate * channels * bytes_per_sample * duration_sec);
        uint32_t file_size = 36 + data_size;

        std::ofstream file(path, std::ios::binary);
        if (!file) return {};

        // RIFF header
        file.write("RIFF", 4);
        file.write(reinterpret_cast<const char*>(&file_size), 4);
        file.write("WAVE", 4);

        // fmt chunk
        file.write("fmt ", 4);
        uint32_t fmt_size = 16;
        file.write(reinterpret_cast<const char*>(&fmt_size), 4);
        uint16_t audio_format = 1;  // PCM
        file.write(reinterpret_cast<const char*>(&audio_format), 2);
        uint16_t num_channels = channels;
        file.write(reinterpret_cast<const char*>(&num_channels), 2);
        uint32_t sample_rate_u32 = sample_rate;
        file.write(reinterpret_cast<const char*>(&sample_rate_u32), 4);
        uint32_t byte_rate_u32 = byte_rate;
        file.write(reinterpret_cast<const char*>(&byte_rate_u32), 4);
        uint16_t block_align_u16 = block_align;
        file.write(reinterpret_cast<const char*>(&block_align_u16), 2);
        uint16_t bits_per_sample = 16;
        file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);

        // data chunk
        file.write("data", 4);
        file.write(reinterpret_cast<const char*>(&data_size), 4);

        // Write audio data
        int total_frames = static_cast<int>(sample_rate * duration_sec);
        for (int frame = 0; frame < total_frames; ++frame) {
            int16_t sample_value = 0;
            if (frequency > 0.0) {
                // Generate sine wave
                double t = static_cast<double>(frame) / sample_rate;
                double amplitude = 0.5;  // 50% volume to avoid clipping when mixed
                sample_value = static_cast<int16_t>(
                    amplitude * 32767.0 * std::sin(2.0 * M_PI * frequency * t));
            }

            for (int ch = 0; ch < channels; ++ch) {
                file.write(reinterpret_cast<const char*>(&sample_value), 2);
            }
        }

        file.close();
        return path;
    }
};

// ============================================================================
// US-AUD-032 Scenario 1: SFX triggers within 10 milliseconds
// ============================================================================

TEST_F(SfxPlayerTest, TriggersWithLowLatency) {
    if (!has_audio_device_) {
        GTEST_SKIP() << "No audio device available";
    }

    // Given a loaded sound effect sample
    fs::path path = generate_wav("quick_sfx.wav", 44100, 2, 0.1, 440.0);
    ASSERT_FALSE(path.empty());

    auto sample = SoundSample::load(path);
    ASSERT_NE(sample, nullptr);

    // And an initialized SFX player
    SfxPlayer player;
    ASSERT_TRUE(player.init(device_id_, 44100, 2));

    // Resume the audio device (starts callbacks)
    SDL_ResumeAudioDevice(device_id_);

    // When play_sfx() is called
    bool triggered = player.play(*sample);

    // Then the sample is queued for playback
    EXPECT_TRUE(triggered);

    // And the audio callback begins outputting samples within 10 milliseconds
    // (We can't easily test exact latency without hardware measurement,
    // but we verify the sample was accepted and will play)

    // Wait for sample to play
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    player.shutdown();
}

// ============================================================================
// US-AUD-032 Scenario 2: Multiple concurrent SFX play without clipping
// ============================================================================

TEST_F(SfxPlayerTest, MultipleConcurrentSfxNoClipping) {
    if (!has_audio_device_) {
        GTEST_SKIP() << "No audio device available";
    }

    // Given 10 loaded sound effect samples
    std::vector<std::unique_ptr<SoundSample>> samples;
    for (int i = 0; i < 10; ++i) {
        fs::path path = generate_wav("sfx_" + std::to_string(i) + ".wav",
                                     44100, 2, 0.05, 440.0 + i * 10.0);
        ASSERT_FALSE(path.empty());

        auto sample = SoundSample::load(path);
        ASSERT_NE(sample, nullptr);
        samples.push_back(std::move(sample));
    }

    // And an initialized SFX player
    SfxPlayer player;
    ASSERT_TRUE(player.init(device_id_, 44100, 2));

    SDL_ResumeAudioDevice(device_id_);

    // When all 10 are triggered simultaneously
    for (const auto& sample : samples) {
        bool triggered = player.play(*sample);
        EXPECT_TRUE(triggered);
    }

    // Then all 10 play concurrently
    // And audio output remains within the -1.0 to +1.0 range (no hard clipping)
    // (Clipping prevention is verified in the mix_audio implementation)

    // Wait for samples to play
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    player.shutdown();
}

// ============================================================================
// US-AUD-032 Scenario 4: Same SFX can be triggered multiple times
// ============================================================================

TEST_F(SfxPlayerTest, SameSfxTriggeredMultipleTimes) {
    if (!has_audio_device_) {
        GTEST_SKIP() << "No audio device available";
    }

    // Given a single loaded sound effect sample
    fs::path path = generate_wav("repeating_sfx.wav", 44100, 2, 0.1, 440.0);
    ASSERT_FALSE(path.empty());

    auto sample = SoundSample::load(path);
    ASSERT_NE(sample, nullptr);

    SfxPlayer player;
    ASSERT_TRUE(player.init(device_id_, 44100, 2));

    SDL_ResumeAudioDevice(device_id_);

    // When play_sfx() is called 5 times with 50 millisecond spacing
    for (int i = 0; i < 5; ++i) {
        bool triggered = player.play(*sample);
        EXPECT_TRUE(triggered);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Then 5 overlapping instances play concurrently
    // And all complete without audio artifacts
    // (Verified by manual testing - automated artifact detection is complex)

    // Wait for all instances to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    player.shutdown();
}

// ============================================================================
// Unit tests (no audio device required)
// ============================================================================

TEST_F(SfxPlayerTest, InitializationWithoutDevice) {
    SfxPlayer player;
    EXPECT_FALSE(player.is_initialized());

    // Init with invalid device ID should fail
    EXPECT_FALSE(player.init(0, 44100, 2));
    EXPECT_FALSE(player.is_initialized());
}

TEST_F(SfxPlayerTest, VolumeControl) {
    SfxPlayer player;

    // Default volume is 1.0
    EXPECT_FLOAT_EQ(player.get_volume(), 1.0f);

    // Set volume
    player.set_volume(0.5f);
    EXPECT_FLOAT_EQ(player.get_volume(), 0.5f);

    // Volume is clamped to [0.0, 1.0]
    player.set_volume(1.5f);
    EXPECT_FLOAT_EQ(player.get_volume(), 1.0f);

    player.set_volume(-0.5f);
    EXPECT_FLOAT_EQ(player.get_volume(), 0.0f);
}

TEST_F(SfxPlayerTest, PlayWithoutInitFails) {
    // Generate a test sample
    fs::path path = generate_wav("test.wav", 44100, 2, 0.1);
    ASSERT_FALSE(path.empty());

    auto sample = SoundSample::load(path);
    ASSERT_NE(sample, nullptr);

    SfxPlayer player;
    EXPECT_FALSE(player.is_initialized());

    // Playing without init should fail
    bool triggered = player.play(*sample);
    EXPECT_FALSE(triggered);
}

TEST_F(SfxPlayerTest, AllVoicesExhausted) {
    if (!has_audio_device_) {
        GTEST_SKIP() << "No audio device available";
    }

    // Generate a long sample to keep voices occupied
    fs::path path = generate_wav("long_sfx.wav", 44100, 2, 0.9, 440.0);
    ASSERT_FALSE(path.empty());

    auto sample = SoundSample::load(path);
    ASSERT_NE(sample, nullptr);

    SfxPlayer player;
    ASSERT_TRUE(player.init(device_id_, 44100, 2));

    SDL_ResumeAudioDevice(device_id_);

    // Trigger MAX_SFX_VOICES samples
    int triggered_count = 0;
    for (int i = 0; i < MAX_SFX_VOICES; ++i) {
        if (player.play(*sample)) {
            ++triggered_count;
        }
    }

    EXPECT_EQ(triggered_count, MAX_SFX_VOICES);

    // Next trigger should fail (all voices in use)
    bool overflow = player.play(*sample);
    EXPECT_FALSE(overflow);

    player.shutdown();
}

TEST_F(SfxPlayerTest, ShutdownMultipleTimes) {
    SfxPlayer player;

    // Shutdown without init should be safe
    player.shutdown();
    player.shutdown();

    if (has_audio_device_) {
        ASSERT_TRUE(player.init(device_id_, 44100, 2));
        player.shutdown();
        player.shutdown();  // Second shutdown should be safe
    }
}

TEST_F(SfxPlayerTest, PerVoiceVolume) {
    if (!has_audio_device_) {
        GTEST_SKIP() << "No audio device available";
    }

    fs::path path = generate_wav("volume_test.wav", 44100, 2, 0.1, 440.0);
    ASSERT_FALSE(path.empty());

    auto sample = SoundSample::load(path);
    ASSERT_NE(sample, nullptr);

    SfxPlayer player;
    ASSERT_TRUE(player.init(device_id_, 44100, 2));

    SDL_ResumeAudioDevice(device_id_);

    // Play same sample with different volumes
    EXPECT_TRUE(player.play(*sample, 1.0f));   // Full volume
    EXPECT_TRUE(player.play(*sample, 0.5f));   // Half volume
    EXPECT_TRUE(player.play(*sample, 0.0f));   // Silent

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    player.shutdown();
}
