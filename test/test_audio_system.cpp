#include <openitup/audio/audio_system.h>
#include <openitup/audio/audio_decoder.h>
#include <openitup/audio/sdl3_audio_system.h>

#include <gtest/gtest.h>

#include <type_traits>

using namespace openitup;

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
