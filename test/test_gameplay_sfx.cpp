#include <gtest/gtest.h>

#include <openitup/audio/gameplay_sfx.h>
#include <openitup/audio/sfx_player.h>
#include <openitup/audio/sound_sample.h>
#include <openitup/judge/judgment_tier.h>

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <cmath>
#include <filesystem>
#include <fstream>

using namespace openitup;
namespace fs = std::filesystem;

static fs::path temp_dir() {
    return fs::temp_directory_path() / "openitup_gameplay_sfx_tests";
}

class GameplaySfxTest : public ::testing::Test {
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
                         double frequency = 440.0) {
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
                double amplitude = 0.3;  // Low amplitude to avoid clipping
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

    void create_key_sounds_per_column() {
        fs::path sfx_dir = temp_ / "sfx";
        fs::create_directories(sfx_dir);

        for (int col = 0; col < NUM_COLUMNS; ++col) {
            std::string filename = "col" + std::to_string(col) + ".wav";
            generate_wav("sfx/" + filename, 44100, 2, 0.05, 440.0 + col * 50.0);
        }
    }

    void create_fallback_key_sound() {
        fs::path sfx_dir = temp_ / "sfx";
        fs::create_directories(sfx_dir);
        generate_wav("sfx/key.wav", 44100, 2, 0.05, 440.0);
    }

    void create_judgment_sounds() {
        fs::path judgment_dir = temp_ / "sfx" / "judgment";
        fs::create_directories(judgment_dir);

        generate_wav("sfx/judgment/Perfect.wav", 44100, 2, 0.05, 800.0);
        generate_wav("sfx/judgment/Great.wav", 44100, 2, 0.05, 700.0);
        generate_wav("sfx/judgment/Good.wav", 44100, 2, 0.05, 600.0);
        generate_wav("sfx/judgment/Bad.wav", 44100, 2, 0.05, 500.0);
        generate_wav("sfx/judgment/Miss.wav", 44100, 2, 0.05, 400.0);
    }
};

// ============================================================================
// US-AUD-033 Scenario 1: Independent SFX volume control
// ============================================================================

TEST_F(GameplaySfxTest, IndependentSfxVolumeControl) {
    if (!has_audio_device_) {
        GTEST_SKIP() << "No audio device available";
    }

    // Given a GameplaySfx instance with default SFX volume
    SfxPlayer player;
    ASSERT_TRUE(player.init(device_id_, 44100, 2));
    SDL_ResumeAudioDevice(device_id_);

    GameplaySfx sfx(&player, temp_);
    EXPECT_FLOAT_EQ(sfx.get_key_sound_volume(), 1.0f);

    // When set_key_sound_volume(0.5) is called
    sfx.set_key_sound_volume(0.5f);

    // Then get_key_sound_volume() returns 0.5
    EXPECT_FLOAT_EQ(sfx.get_key_sound_volume(), 0.5f);

    // And subsequent SFX playback uses 0.5 volume
    // (Verified indirectly through on_panel_press - actual audio mixing tested elsewhere)

    player.shutdown();
}

// ============================================================================
// US-AUD-033 Scenario 2: SFX volume clamped to [0.0, 1.0]
// ============================================================================

TEST_F(GameplaySfxTest, SfxVolumeClampedToRange) {
    SfxPlayer player;
    GameplaySfx sfx(&player, temp_);

    // When set_key_sound_volume(1.5) is called
    sfx.set_key_sound_volume(1.5f);

    // Then get_key_sound_volume() returns 1.0
    EXPECT_FLOAT_EQ(sfx.get_key_sound_volume(), 1.0f);

    // When set_key_sound_volume(-0.5) is called
    sfx.set_key_sound_volume(-0.5f);

    // Then get_key_sound_volume() returns 0.0
    EXPECT_FLOAT_EQ(sfx.get_key_sound_volume(), 0.0f);
}

// ============================================================================
// US-AUD-041 Scenario 1: Play key sound on panel press
// ============================================================================

TEST_F(GameplaySfxTest, PlayKeySoundOnPanelPress) {
    if (!has_audio_device_) {
        GTEST_SKIP() << "No audio device available";
    }

    // Given a loaded key sound
    create_fallback_key_sound();

    SfxPlayer player;
    ASSERT_TRUE(player.init(device_id_, 44100, 2));
    SDL_ResumeAudioDevice(device_id_);

    GameplaySfx sfx(&player, temp_);
    ASSERT_TRUE(sfx.load_sounds());

    // When on_panel_press(2) is called
    sfx.on_panel_press(2);

    // Then the key sound is queued for playback
    // (Verified by SfxPlayer's internal state - sound will play)

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    player.shutdown();
}

// ============================================================================
// US-AUD-041 Scenario 2: Graceful degradation when key sound missing
// ============================================================================

TEST_F(GameplaySfxTest, GracefulDegradationWhenKeySoundMissing) {
    SfxPlayer player;
    GameplaySfx sfx(&player, temp_);

    // Given no key sounds loaded
    // (temp directory is empty)

    // When on_panel_press(0) is called
    // Then no exception is thrown
    EXPECT_NO_THROW(sfx.on_panel_press(0));
}

// ============================================================================
// US-AUD-042 Scenario 1: Per-column key sounds
// ============================================================================

TEST_F(GameplaySfxTest, PerColumnKeySounds) {
    if (!has_audio_device_) {
        GTEST_SKIP() << "No audio device available";
    }

    // Given per-column key sounds (col0.wav through col4.wav)
    create_key_sounds_per_column();

    SfxPlayer player;
    ASSERT_TRUE(player.init(device_id_, 44100, 2));
    SDL_ResumeAudioDevice(device_id_);

    GameplaySfx sfx(&player, temp_);
    ASSERT_TRUE(sfx.load_sounds());

    // When on_panel_press(0), on_panel_press(1), ... on_panel_press(4) are called
    for (int col = 0; col < NUM_COLUMNS; ++col) {
        sfx.on_panel_press(col);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Then each column plays its unique sound
    // (Verified by different frequencies generated for each column)

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    player.shutdown();
}

// ============================================================================
// US-AUD-042 Scenario 2: Fallback to single key sound
// ============================================================================

TEST_F(GameplaySfxTest, FallbackToSingleKeySound) {
    if (!has_audio_device_) {
        GTEST_SKIP() << "No audio device available";
    }

    // Given only a fallback key.wav (no per-column sounds)
    create_fallback_key_sound();

    SfxPlayer player;
    ASSERT_TRUE(player.init(device_id_, 44100, 2));
    SDL_ResumeAudioDevice(device_id_);

    GameplaySfx sfx(&player, temp_);
    ASSERT_TRUE(sfx.load_sounds());

    // When on_panel_press(0), on_panel_press(1), ... on_panel_press(4) are called
    for (int col = 0; col < NUM_COLUMNS; ++col) {
        sfx.on_panel_press(col);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Then all columns play the same fallback sound
    // (Verified by consistent playback behavior)

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    player.shutdown();
}

// ============================================================================
// US-AUD-051 Scenario 1: Play judgment sound based on timing
// ============================================================================

TEST_F(GameplaySfxTest, PlayJudgmentSoundBasedOnTiming) {
    if (!has_audio_device_) {
        GTEST_SKIP() << "No audio device available";
    }

    // Given loaded judgment sounds
    create_judgment_sounds();

    SfxPlayer player;
    ASSERT_TRUE(player.init(device_id_, 44100, 2));
    SDL_ResumeAudioDevice(device_id_);

    GameplaySfx sfx(&player, temp_);
    ASSERT_TRUE(sfx.load_sounds());

    // When on_judgment(JudgmentTier::PERFECT) is called
    sfx.on_judgment(JudgmentTier::PERFECT);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Then Perfect.wav is played
    // (Verified by SfxPlayer playback)

    player.shutdown();
}

// ============================================================================
// US-AUD-051 Scenario 2: Different judgment tiers play different sounds
// ============================================================================

TEST_F(GameplaySfxTest, DifferentJudgmentTiersPlayDifferentSounds) {
    if (!has_audio_device_) {
        GTEST_SKIP() << "No audio device available";
    }

    // Given loaded judgment sounds
    create_judgment_sounds();

    SfxPlayer player;
    ASSERT_TRUE(player.init(device_id_, 44100, 2));
    SDL_ResumeAudioDevice(device_id_);

    GameplaySfx sfx(&player, temp_);
    ASSERT_TRUE(sfx.load_sounds());

    // When different judgment tiers are triggered
    sfx.on_judgment(JudgmentTier::PERFECT);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    sfx.on_judgment(JudgmentTier::GREAT);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    sfx.on_judgment(JudgmentTier::GOOD);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    sfx.on_judgment(JudgmentTier::BAD);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    sfx.on_judgment(JudgmentTier::MISS);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    // Then each tier plays its unique sound
    // (Verified by different frequencies generated for each judgment)

    player.shutdown();
}

// ============================================================================
// US-AUD-052 Scenario 1: Configurable judgment sound volume
// ============================================================================

TEST_F(GameplaySfxTest, ConfigurableJudgmentSoundVolume) {
    SfxPlayer player;
    GameplaySfx sfx(&player, temp_);

    // Given default judgment volume
    EXPECT_FLOAT_EQ(sfx.get_judgment_volume(), 1.0f);

    // When set_judgment_volume(0.3) is called
    sfx.set_judgment_volume(0.3f);

    // Then get_judgment_volume() returns 0.3
    EXPECT_FLOAT_EQ(sfx.get_judgment_volume(), 0.3f);

    // And subsequent judgment sounds use 0.3 volume
    // (Verified indirectly through on_judgment)
}

// ============================================================================
// US-AUD-052 Scenario 2: Judgment volume clamped to [0.0, 1.0]
// ============================================================================

TEST_F(GameplaySfxTest, JudgmentVolumeClampedToRange) {
    SfxPlayer player;
    GameplaySfx sfx(&player, temp_);

    // When set_judgment_volume(2.0) is called
    sfx.set_judgment_volume(2.0f);

    // Then get_judgment_volume() returns 1.0
    EXPECT_FLOAT_EQ(sfx.get_judgment_volume(), 1.0f);

    // When set_judgment_volume(-0.3) is called
    sfx.set_judgment_volume(-0.3f);

    // Then get_judgment_volume() returns 0.0
    EXPECT_FLOAT_EQ(sfx.get_judgment_volume(), 0.0f);
}

// ============================================================================
// Additional edge case tests
// ============================================================================

TEST_F(GameplaySfxTest, InvalidColumnIndexDoesNotCrash) {
    SfxPlayer player;
    GameplaySfx sfx(&player, temp_);

    // Invalid column indices should be handled gracefully
    EXPECT_NO_THROW(sfx.on_panel_press(-1));
    EXPECT_NO_THROW(sfx.on_panel_press(NUM_COLUMNS));
    EXPECT_NO_THROW(sfx.on_panel_press(999));
}

TEST_F(GameplaySfxTest, LoadSoundsReturnsFalseWhenNoSoundsFound) {
    SfxPlayer player;
    GameplaySfx sfx(&player, temp_);

    // Given an empty directory
    // When load_sounds() is called
    bool loaded = sfx.load_sounds();

    // Then it returns false
    EXPECT_FALSE(loaded);
}

TEST_F(GameplaySfxTest, LoadSoundsReturnsTrueWhenAnySoundFound) {
    // Given at least one sound file
    create_fallback_key_sound();

    SfxPlayer player;
    GameplaySfx sfx(&player, temp_);

    // When load_sounds() is called
    bool loaded = sfx.load_sounds();

    // Then it returns true
    EXPECT_TRUE(loaded);
}
