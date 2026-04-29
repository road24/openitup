#include <openitup/audio/audio_system.h>

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
