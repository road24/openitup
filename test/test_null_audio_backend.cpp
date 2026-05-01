#include <gtest/gtest.h>
#include <openitup/audio/null_audio_system.h>
#include <filesystem>

using namespace openitup;
namespace fs = std::filesystem;

// US-AUD-093: Select audio backend at compile time
// Tests for the NULL audio backend (no-op for headless testing)

TEST(NullAudioBackendTest, InitSucceeds) {
    NullAudioSystem audio;
    EXPECT_TRUE(audio.init());
}

TEST(NullAudioBackendTest, LoadMusicSucceeds) {
    NullAudioSystem audio;
    audio.init();

    // NULL backend accepts any path
    EXPECT_TRUE(audio.load_music("/fake/path/music.ogg"));
    EXPECT_TRUE(audio.is_music_loaded());
}

TEST(NullAudioBackendTest, PlaybackStateTransitions) {
    NullAudioSystem audio;
    audio.init();
    audio.load_music("/fake/music.ogg");

    EXPECT_EQ(audio.get_state(), AudioState::STOPPED);

    audio.play();
    EXPECT_EQ(audio.get_state(), AudioState::PLAYING);

    audio.pause();
    EXPECT_EQ(audio.get_state(), AudioState::PAUSED);

    audio.resume();
    EXPECT_EQ(audio.get_state(), AudioState::PLAYING);

    audio.stop();
    EXPECT_EQ(audio.get_state(), AudioState::STOPPED);
}

TEST(NullAudioBackendTest, SeekClampsToRange) {
    NullAudioSystem audio;
    audio.init();
    audio.load_music("/fake/music.ogg");

    double duration = audio.get_duration_ms();
    EXPECT_GT(duration, 0.0);

    // Seek beyond end clamps to duration
    audio.seek(duration + 1000.0);
    EXPECT_DOUBLE_EQ(audio.get_position_ms(), duration);

    // Seek to negative clamps to 0
    audio.seek(-100.0);
    EXPECT_DOUBLE_EQ(audio.get_position_ms(), 0.0);

    // Normal seek works
    audio.seek(5000.0);
    EXPECT_DOUBLE_EQ(audio.get_position_ms(), 5000.0);
}

TEST(NullAudioBackendTest, VolumeControlWorks) {
    NullAudioSystem audio;
    audio.init();

    EXPECT_FLOAT_EQ(audio.get_music_volume(), 1.0f);

    audio.set_music_volume(0.5f);
    EXPECT_FLOAT_EQ(audio.get_music_volume(), 0.5f);

    // Clamps to 0.0-1.0
    audio.set_music_volume(1.5f);
    EXPECT_FLOAT_EQ(audio.get_music_volume(), 1.0f);

    audio.set_music_volume(-0.5f);
    EXPECT_FLOAT_EQ(audio.get_music_volume(), 0.0f);
}

TEST(NullAudioBackendTest, SFXOperationsWork) {
    NullAudioSystem audio;
    audio.init();

    // Load SFX returns a handle
    uint32_t handle1 = audio.load_sfx("/fake/sfx1.wav");
    uint32_t handle2 = audio.load_sfx("/fake/sfx2.wav");

    EXPECT_GT(handle1, 0u);
    EXPECT_GT(handle2, 0u);
    EXPECT_NE(handle1, handle2);

    // Play SFX doesn't crash
    audio.play_sfx(handle1);
    audio.play_sfx(handle2);

    // SFX volume works
    audio.set_sfx_volume(0.7f);
    EXPECT_FLOAT_EQ(audio.get_sfx_volume(), 0.7f);
}

TEST(NullAudioBackendTest, ShutdownSafe) {
    NullAudioSystem audio;
    audio.init();

    // Safe to call multiple times
    audio.shutdown();
    audio.shutdown();
}
