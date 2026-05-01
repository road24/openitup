#include <gtest/gtest.h>

#include <openitup/audio/sdl3_audio_system.h>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace openitup;

// US-AUD-012: Maintain position accuracy across seek operations
class AudioSeekAccuracyTest : public ::testing::Test {
protected:
    void SetUp() override {
        audio_system_ = std::make_unique<SDL3AudioSystem>();
        ASSERT_TRUE(audio_system_->init());
    }

    void TearDown() override {
        if (audio_system_) {
            audio_system_->shutdown();
        }
    }

    std::unique_ptr<SDL3AudioSystem> audio_system_;
};

// US-AUD-012 Scenario 1: Position accurate immediately after seek
TEST_F(AudioSeekAccuracyTest, PositionAccurateAfterSeek) {
    std::filesystem::path test_file = "test/fixtures/test_music.ogg";
    if (!std::filesystem::exists(test_file)) {
        GTEST_SKIP() << "Test fixture not available: " << test_file;
    }

    ASSERT_TRUE(audio_system_->load_music(test_file));
    audio_system_->play();

    // Let it play for a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Seek to 2000ms
    audio_system_->seek(2000.0);

    // Allow buffer refill time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Position should be within 50ms of seek target (US-AUD-012 SC1)
    double pos = audio_system_->get_position_ms();
    EXPECT_NEAR(pos, 2000.0, 50.0) << "Position after seek should be within 50ms of target";
}

// US-AUD-012 Scenario 2: Seek does not introduce timing discontinuity
TEST_F(AudioSeekAccuracyTest, SeekNoTimingDiscontinuity) {
    std::filesystem::path test_file = "test/fixtures/test_music.ogg";
    if (!std::filesystem::exists(test_file)) {
        GTEST_SKIP() << "Test fixture not available: " << test_file;
    }

    ASSERT_TRUE(audio_system_->load_music(test_file));
    audio_system_->play();

    // Let it play
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    double before_seek = audio_system_->get_position_ms();

    // Seek forward
    audio_system_->seek(3000.0);

    // Allow buffer refill
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    double after_seek = audio_system_->get_position_ms();

    // Should have jumped forward, not have negative delta (US-AUD-012 SC2)
    EXPECT_GT(after_seek, before_seek) << "Position should jump forward after seek";
    EXPECT_NEAR(after_seek, 3000.0, 100.0) << "Position should be near seek target";
}

// US-AUD-012 Scenario 3: Buffer refill does not report stale position
TEST_F(AudioSeekAccuracyTest, NoStalePositionDuringSeek) {
    std::filesystem::path test_file = "test/fixtures/test_music.ogg";
    if (!std::filesystem::exists(test_file)) {
        GTEST_SKIP() << "Test fixture not available: " << test_file;
    }

    ASSERT_TRUE(audio_system_->load_music(test_file));
    audio_system_->play();

    // Let it play to 1000ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Seek to 5000ms
    audio_system_->seek(5000.0);

    // Immediately query position (buffer may still be refilling)
    double pos_immediate = audio_system_->get_position_ms();

    // Position should NOT be near the old position (US-AUD-012 SC3)
    // It should either be at the new position or very close to it
    // NOT back at ~1000ms
    EXPECT_GT(pos_immediate, 4000.0) << "Position should jump to near seek target, not remain at pre-seek position";
}
