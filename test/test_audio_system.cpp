#include <openitup/audio/audio_system.h>
#include <openitup/audio/audio_decoder.h>
#include <openitup/audio/sdl3_audio_system.h>

#include <SDL3/SDL.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <type_traits>

using namespace openitup;
namespace fs = std::filesystem;

// Helper to get test fixture directory
static fs::path fixtures_dir() {
    return fs::path(__FILE__).parent_path() / "fixtures";
}

TEST(AudioSystem, AudioStateEnumValues) {
    AudioState stopped = AudioState::STOPPED;
    AudioState playing = AudioState::PLAYING;
    AudioState paused = AudioState::PAUSED;

    EXPECT_NE(stopped, playing);
    EXPECT_NE(playing, paused);
    EXPECT_NE(paused, stopped);
}

TEST(AudioSystem, InterfaceIsAbstract) {
    EXPECT_TRUE(std::is_abstract_v<AudioSystem>);
}

TEST(AudioDecoder, InterfaceIsAbstract) {
    EXPECT_TRUE(std::is_abstract_v<AudioDecoder>);
}

TEST(AudioDecoder, AudioFormatFields) {
    AudioFormat fmt;
    fmt.sample_rate = 44100;
    fmt.channels = 2;
    fmt.total_samples = 132300;  // 3 seconds stereo

    EXPECT_EQ(fmt.sample_rate, 44100);
    EXPECT_EQ(fmt.channels, 2);
    EXPECT_EQ(fmt.total_samples, 132300u);
}

// Basic SDL3AudioSystem initialization test
TEST(SDL3AudioSystem, CanConstruct) {
    SDL3AudioSystem audio;
    // Constructor succeeds (no crash)
    SUCCEED();
}

// Integration tests - require SDL audio device
// These are guarded by SDL audio availability check

TEST(SDL3AudioSystem, LoadAndPlayOGG) {
    // Guard: skip if no audio device available
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (!SDL_Init(SDL_INIT_AUDIO)) {
            GTEST_SKIP() << "No audio support";
        }
    }

    SDL3AudioSystem sys;
    if (!sys.init()) {
        GTEST_SKIP() << "No audio device";
    }

    // Load OGG test fixture
    bool loaded = sys.load_music(fixtures_dir() / "test_tone_44100.ogg");
    ASSERT_TRUE(loaded) << "Failed to load test OGG file";

    // Play
    sys.play();

    // Verify state is PLAYING
    EXPECT_EQ(sys.get_state(), AudioState::PLAYING);

    // Wait a bit to let audio play
    SDL_Delay(100);

    // Position should be progressing
    double pos = sys.get_position_ms();
    EXPECT_GT(pos, 0.0);
    EXPECT_LT(pos, 200.0);  // Should be less than 200ms

    sys.shutdown();
}

TEST(SDL3AudioSystem, PauseFreezesPosition) {
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (!SDL_Init(SDL_INIT_AUDIO)) {
            GTEST_SKIP() << "No audio support";
        }
    }

    SDL3AudioSystem sys;
    if (!sys.init()) {
        GTEST_SKIP() << "No audio device";
    }

    if (!sys.load_music(fixtures_dir() / "test_tone_44100.ogg")) {
        GTEST_SKIP() << "Failed to load test file";
    }

    // Play for 500ms
    sys.play();
    SDL_Delay(500);

    // Pause
    sys.pause();
    EXPECT_EQ(sys.get_state(), AudioState::PAUSED);

    // Record position
    double paused_pos = sys.get_position_ms();
    EXPECT_GT(paused_pos, 400.0);  // Should be around 500ms
    EXPECT_LT(paused_pos, 600.0);

    // Wait 200ms and verify position is stable
    SDL_Delay(200);
    double pos_after_wait = sys.get_position_ms();

    // Position should be frozen (within 1ms tolerance)
    EXPECT_NEAR(pos_after_wait, paused_pos, 1.0);

    sys.shutdown();
}

TEST(SDL3AudioSystem, StopResetsPosition) {
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (!SDL_Init(SDL_INIT_AUDIO)) {
            GTEST_SKIP() << "No audio support";
        }
    }

    SDL3AudioSystem sys;
    if (!sys.init()) {
        GTEST_SKIP() << "No audio device";
    }

    if (!sys.load_music(fixtures_dir() / "test_tone_44100.ogg")) {
        GTEST_SKIP() << "Failed to load test file";
    }

    // Play for a bit
    sys.play();
    SDL_Delay(200);

    // Verify playing
    EXPECT_GT(sys.get_position_ms(), 0.0);

    // Stop
    sys.stop();
    EXPECT_EQ(sys.get_state(), AudioState::STOPPED);

    // Position should be near 0
    EXPECT_NEAR(sys.get_position_ms(), 0.0, 1.0);

    sys.shutdown();
}

TEST(SDL3AudioSystem, SeekForward) {
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (!SDL_Init(SDL_INIT_AUDIO)) {
            GTEST_SKIP() << "No audio support";
        }
    }

    SDL3AudioSystem sys;
    if (!sys.init()) {
        GTEST_SKIP() << "No audio device";
    }

    if (!sys.load_music(fixtures_dir() / "test_tone_44100.ogg")) {
        GTEST_SKIP() << "Failed to load test file";
    }

    // Play
    sys.play();
    SDL_Delay(100);

    // Seek to 500ms
    sys.seek(500.0);

    // Wait for position to stabilize after seek
    SDL_Delay(100);

    // Position should be near 500ms (within 50ms tolerance for buffer latency)
    double pos = sys.get_position_ms();
    EXPECT_GT(pos, 450.0);
    EXPECT_LT(pos, 650.0);

    sys.shutdown();
}
