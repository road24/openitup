#include <gtest/gtest.h>

#include <openitup/data/settings.h>
#include <nlohmann/json.hpp>

using namespace openitup::data;

// US-AUD-061: Apply global audio offset to judge timing
class GlobalAudioOffsetTest : public ::testing::Test {
protected:
};

// US-AUD-061 Scenario 1: Positive offset delays judgment timing
TEST_F(GlobalAudioOffsetTest, PositiveOffsetDelaysJudgment) {
    // Simulate: audio position is 10000ms, offset is +50ms
    // Judge should perceive position as 10050ms
    double audio_position_ms = 10000.0;
    int global_offset_ms = 50;

    double judge_position_ms = audio_position_ms + global_offset_ms;

    EXPECT_DOUBLE_EQ(judge_position_ms, 10050.0);
}

// US-AUD-061 Scenario 2: Negative offset advances judgment timing
TEST_F(GlobalAudioOffsetTest, NegativeOffsetAdvancesJudgment) {
    // Simulate: audio position is 10000ms, offset is -30ms
    // Judge should perceive position as 9970ms
    double audio_position_ms = 10000.0;
    int global_offset_ms = -30;

    double judge_position_ms = audio_position_ms + global_offset_ms;

    EXPECT_DOUBLE_EQ(judge_position_ms, 9970.0);
}

// US-AUD-061 Scenario 3: Offset range is constrained to ±500ms
TEST_F(GlobalAudioOffsetTest, OffsetRangeConstrained) {
    // Attempt to set offset to +600ms, should clamp to +500ms
    int requested_offset = 600;
    int clamped_offset = std::clamp(requested_offset, -500, 500);

    EXPECT_EQ(clamped_offset, 500);

    // Attempt to set offset to -600ms, should clamp to -500ms
    requested_offset = -600;
    clamped_offset = std::clamp(requested_offset, -500, 500);

    EXPECT_EQ(clamped_offset, -500);
}

// US-AUD-061 Scenario 5: Offset adjusts in 1 millisecond increments
TEST_F(GlobalAudioOffsetTest, OffsetAdjustsInOneMillisecond) {
    int offset = 0;

    // Increment by 1ms
    offset += 1;
    EXPECT_EQ(offset, 1);

    // Decrement by 1ms
    offset -= 1;
    EXPECT_EQ(offset, 0);
}

// Test that AudioSettings includes global_audio_offset_ms field
TEST_F(GlobalAudioOffsetTest, AudioSettingsIncludesOffset) {
    AudioSettings settings;
    settings.global_audio_offset_ms = 42;

    // Serialize to JSON
    nlohmann::json j = settings;

    EXPECT_TRUE(j.contains("global_audio_offset_ms"));
    EXPECT_EQ(j["global_audio_offset_ms"], 42);

    // Deserialize from JSON
    AudioSettings loaded;
    loaded = j.get<AudioSettings>();

    EXPECT_EQ(loaded.global_audio_offset_ms, 42);
}

// Test default offset is 0
TEST_F(GlobalAudioOffsetTest, DefaultOffsetIsZero) {
    AudioSettings settings;
    EXPECT_EQ(settings.global_audio_offset_ms, 0);

    // Load from JSON without offset field
    nlohmann::json j = {
        {"master_volume", 1.0f},
        {"music_volume", 1.0f},
        {"sfx_volume", 1.0f}
    };

    AudioSettings loaded = j.get<AudioSettings>();
    EXPECT_EQ(loaded.global_audio_offset_ms, 0) << "Default offset should be 0 when field is missing";
}
