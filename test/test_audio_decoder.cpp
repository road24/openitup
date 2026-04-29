#include <gtest/gtest.h>

#include <openitup/audio/vorbis_decoder.h>
#include <openitup/audio/mp3_decoder.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

using namespace openitup;
namespace fs = std::filesystem;

static fs::path fixtures_dir() {
    const char* env = std::getenv("OPENITUP_FIXTURES_DIR");
    if (env) return fs::path(env);
    return fs::path(__FILE__).parent_path() / "fixtures";
}

class AudioDecoderTest : public ::testing::Test {
protected:
    fs::path fixtures_;

    void SetUp() override {
        fixtures_ = fixtures_dir();
    }
};

// ============================================================================
// VorbisDecoder Tests
// ============================================================================

TEST_F(AudioDecoderTest, VorbisOpensStereo44100) {
    VorbisDecoder decoder;
    fs::path path = fixtures_ / "test_tone_44100.ogg";

    bool opened = decoder.open(path.string());
    ASSERT_TRUE(opened) << "Failed to open: " << path;
    EXPECT_TRUE(decoder.is_open());

    const AudioFormat& fmt = decoder.format();
    EXPECT_EQ(fmt.sample_rate, 44100);
    EXPECT_EQ(fmt.channels, 2);
    // 1 second stereo at 44100 Hz = 44100 samples per channel
    EXPECT_NEAR(fmt.total_samples, 44100, 100);

    decoder.close();
    EXPECT_FALSE(decoder.is_open());
}

TEST_F(AudioDecoderTest, VorbisOpensMono48000) {
    VorbisDecoder decoder;
    fs::path path = fixtures_ / "test_tone_48000.ogg";

    bool opened = decoder.open(path.string());
    ASSERT_TRUE(opened) << "Failed to open: " << path;
    EXPECT_TRUE(decoder.is_open());

    const AudioFormat& fmt = decoder.format();
    EXPECT_EQ(fmt.sample_rate, 48000);
    EXPECT_EQ(fmt.channels, 1);
    // 1 second mono at 48000 Hz = 48000 samples per channel
    EXPECT_NEAR(fmt.total_samples, 48000, 100);

    decoder.close();
}

TEST_F(AudioDecoderTest, VorbisDecodesFullFile) {
    VorbisDecoder decoder;
    fs::path path = fixtures_ / "test_tone_44100.ogg";

    ASSERT_TRUE(decoder.open(path.string()));

    const AudioFormat& fmt = decoder.format();
    uint64_t expected_samples = fmt.total_samples;

    // Decode entire file
    std::vector<float> buffer(4096 * fmt.channels);
    uint64_t total_decoded = 0;

    while (true) {
        int frames = decoder.decode(buffer.data(), 4096);
        if (frames == 0) break;
        total_decoded += frames;
    }

    // Should be within 1% of expected
    double percent_error = std::abs(static_cast<double>(total_decoded) - static_cast<double>(expected_samples))
                          / static_cast<double>(expected_samples);
    EXPECT_LT(percent_error, 0.01)
        << "Decoded " << total_decoded << " frames, expected ~" << expected_samples;

    decoder.close();
}

TEST_F(AudioDecoderTest, VorbisSeekToMiddle) {
    VorbisDecoder decoder;
    fs::path path = fixtures_ / "test_tone_44100.ogg";

    ASSERT_TRUE(decoder.open(path.string()));

    const AudioFormat& fmt = decoder.format();
    uint64_t middle = fmt.total_samples / 2;

    // Seek to middle
    bool seek_ok = decoder.seek_to_sample(middle);
    ASSERT_TRUE(seek_ok);

    // Decode 100 frames after seek
    std::vector<float> buffer(100 * fmt.channels);
    int frames = decoder.decode(buffer.data(), 100);

    EXPECT_GT(frames, 0) << "Should decode frames after seek";

    // Verify non-zero output (silence would be all zeros)
    bool has_nonzero = false;
    for (size_t i = 0; i < frames * fmt.channels; ++i) {
        if (std::abs(buffer[i]) > 0.001f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero) << "Expected non-zero audio after seek";

    decoder.close();
}

TEST_F(AudioDecoderTest, VorbisRejectsInvalidFile) {
    VorbisDecoder decoder;
    // Try to open a C++ source file as OGG
    fs::path invalid = fixtures_.parent_path() / "test_audio_decoder.cpp";

    bool opened = decoder.open(invalid.string());
    EXPECT_FALSE(opened) << "Should reject non-OGG file";
    EXPECT_FALSE(decoder.is_open());
}

TEST_F(AudioDecoderTest, VorbisRejectsNonexistentFile) {
    VorbisDecoder decoder;
    fs::path nonexistent = fixtures_ / "does_not_exist.ogg";

    bool opened = decoder.open(nonexistent.string());
    EXPECT_FALSE(opened) << "Should reject nonexistent file";
    EXPECT_FALSE(decoder.is_open());
}

// ============================================================================
// Mp3Decoder Tests
// ============================================================================

TEST_F(AudioDecoderTest, Mp3OpensCBR) {
    Mp3Decoder decoder;
    fs::path path = fixtures_ / "test_tone_44100.mp3";

    bool opened = decoder.open(path.string());
    ASSERT_TRUE(opened) << "Failed to open: " << path;
    EXPECT_TRUE(decoder.is_open());

    const AudioFormat& fmt = decoder.format();
    EXPECT_EQ(fmt.sample_rate, 44100);
    EXPECT_EQ(fmt.channels, 2);
    // MP3 encoding may add a bit of padding, so allow more tolerance
    EXPECT_NEAR(fmt.total_samples, 44100, 2500);

    decoder.close();
    EXPECT_FALSE(decoder.is_open());
}

TEST_F(AudioDecoderTest, Mp3DecodesFullFile) {
    Mp3Decoder decoder;
    fs::path path = fixtures_ / "test_tone_44100.mp3";

    ASSERT_TRUE(decoder.open(path.string()));

    const AudioFormat& fmt = decoder.format();
    uint64_t expected_samples = fmt.total_samples;

    // Decode entire file
    std::vector<float> buffer(4096 * fmt.channels);
    uint64_t total_decoded = 0;

    while (true) {
        int frames = decoder.decode(buffer.data(), 4096);
        if (frames == 0) break;
        total_decoded += frames;
    }

    // MP3 can have more variation, allow 5% tolerance
    double percent_error = std::abs(static_cast<double>(total_decoded) - static_cast<double>(expected_samples))
                          / static_cast<double>(expected_samples);
    EXPECT_LT(percent_error, 0.05)
        << "Decoded " << total_decoded << " frames, expected ~" << expected_samples;

    decoder.close();
}

TEST_F(AudioDecoderTest, Mp3SeekToMiddle) {
    Mp3Decoder decoder;
    fs::path path = fixtures_ / "test_tone_44100.mp3";

    ASSERT_TRUE(decoder.open(path.string()));

    const AudioFormat& fmt = decoder.format();
    uint64_t middle = fmt.total_samples / 2;

    // Seek to middle
    bool seek_ok = decoder.seek_to_sample(middle);
    ASSERT_TRUE(seek_ok);

    // Decode 100 frames after seek
    std::vector<float> buffer(100 * fmt.channels);
    int frames = decoder.decode(buffer.data(), 100);

    EXPECT_GT(frames, 0) << "Should decode frames after seek";

    // Verify non-zero output
    bool has_nonzero = false;
    for (size_t i = 0; i < frames * fmt.channels; ++i) {
        if (std::abs(buffer[i]) > 0.001f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero) << "Expected non-zero audio after seek";

    decoder.close();
}

TEST_F(AudioDecoderTest, Mp3RejectsInvalidFile) {
    Mp3Decoder decoder;
    // Try to open a C++ source file as MP3
    fs::path invalid = fixtures_.parent_path() / "test_audio_decoder.cpp";

    bool opened = decoder.open(invalid.string());
    EXPECT_FALSE(opened) << "Should reject non-MP3 file";
    EXPECT_FALSE(decoder.is_open());
}

// ============================================================================
// Output Format Tests
// ============================================================================

TEST_F(AudioDecoderTest, DecoderOutputIsNormalizedFloat) {
    VorbisDecoder decoder;
    fs::path path = fixtures_ / "test_tone_44100.ogg";

    ASSERT_TRUE(decoder.open(path.string()));

    const AudioFormat& fmt = decoder.format();
    std::vector<float> buffer(1000 * fmt.channels);

    int frames = decoder.decode(buffer.data(), 1000);
    ASSERT_GT(frames, 0);

    // All samples should be in [-1.0, 1.0] range
    for (int i = 0; i < frames * fmt.channels; ++i) {
        EXPECT_GE(buffer[i], -1.0f) << "Sample " << i << " below -1.0";
        EXPECT_LE(buffer[i], 1.0f) << "Sample " << i << " above 1.0";
    }

    decoder.close();
}
