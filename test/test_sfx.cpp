#include <gtest/gtest.h>

#include <openitup/audio/sound_sample.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace openitup;
namespace fs = std::filesystem;

static fs::path fixtures_dir() {
    const char* env = std::getenv("OPENITUP_FIXTURES_DIR");
    if (env) return fs::path(env);
    return fs::path(__FILE__).parent_path() / "fixtures";
}

static fs::path temp_dir() {
    return fs::temp_directory_path() / "openitup_sfx_tests";
}

class SfxTest : public ::testing::Test {
protected:
    fs::path fixtures_;
    fs::path temp_;

    void SetUp() override {
        fixtures_ = fixtures_dir();
        temp_ = temp_dir();
        fs::create_directories(temp_);
    }

    void TearDown() override {
        // Clean up temp files
        if (fs::exists(temp_)) {
            fs::remove_all(temp_);
        }
    }

    // Generate a simple WAV file with given parameters
    // Returns path to generated file
    fs::path generate_wav(const std::string& filename,
                         int sample_rate,
                         int channels,
                         double duration_sec) {
        fs::path path = temp_ / filename;

        // WAV format parameters
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

        // Write silence (all zeros)
        std::vector<int16_t> silence(data_size / 2, 0);
        file.write(reinterpret_cast<const char*>(silence.data()), data_size);

        file.close();
        return path;
    }
};

// ============================================================================
// US-AUD-031 Scenario 1: Load WAV file under 1 second
// ============================================================================

TEST_F(SfxTest, LoadWavFileUnder1Second) {
    // Given a 500 millisecond WAV file at 44.1 kHz
    fs::path path = generate_wav("short_sfx.wav", 44100, 2, 0.5);
    ASSERT_FALSE(path.empty());

    // When the file is loaded as a sound effect
    auto sample = SoundSample::load(path);

    // Then the entire file is decoded to memory
    ASSERT_NE(sample, nullptr);
    EXPECT_EQ(sample->sample_rate(), 44100);
    EXPECT_EQ(sample->channels(), 2);
    EXPECT_NEAR(sample->duration_sec(), 0.5, 0.01);

    // And playback can be triggered without further disk I/O
    // (data is in memory)
    EXPECT_GT(sample->sample_count(), 0u);
    EXPECT_NE(sample->data(), nullptr);

    // Verify memory usage is reasonable
    // 0.5s * 44100 Hz * 2 channels * 4 bytes/float = ~176 KB
    size_t expected_bytes = 0.5 * 44100 * 2 * sizeof(float);
    EXPECT_NEAR(sample->memory_bytes(), expected_bytes, expected_bytes * 0.1);
}

// ============================================================================
// US-AUD-031 Scenario 2: Load short OGG file
// ============================================================================

// Note: This test is marked as disabled because we don't have OGG generation
// in the test fixture yet. WAV support is sufficient for Phase 3.
TEST_F(SfxTest, DISABLED_LoadShortOggFile) {
    // Given a 300 millisecond OGG Vorbis file
    // (would need test fixture or OGG generation)

    // When the file is loaded as a sound effect
    // Then the entire file is decoded to memory
    // And memory usage increases by approximately 50 KB (stereo 44.1 kHz PCM)
}

// ============================================================================
// US-AUD-031 Scenario 3: Reject files over 1 second
// ============================================================================

TEST_F(SfxTest, RejectFilesOver1Second) {
    // Given a 2 second audio file
    fs::path path = generate_wav("long_sfx.wav", 44100, 2, 2.0);
    ASSERT_FALSE(path.empty());

    // When attempting to load as a sound effect
    auto sample = SoundSample::load(path);

    // Then loading fails with an error code
    EXPECT_EQ(sample, nullptr);

    // And an ERROR-level log message specifies the file exceeds duration limit
    // (verified by manual inspection of log output)
}

// ============================================================================
// US-AUD-031 Scenario 4: Multiple samples can be loaded concurrently
// ============================================================================

TEST_F(SfxTest, MultipleSamplesLoadedConcurrently) {
    // Given 20 sound effect files
    std::vector<fs::path> paths;
    for (int i = 0; i < 20; ++i) {
        fs::path path = generate_wav("sfx_" + std::to_string(i) + ".wav",
                                     44100, 2, 0.1);
        ASSERT_FALSE(path.empty());
        paths.push_back(path);
    }

    // When all are loaded
    std::vector<std::unique_ptr<SoundSample>> samples;
    for (const auto& path : paths) {
        auto sample = SoundSample::load(path);
        ASSERT_NE(sample, nullptr);
        samples.push_back(std::move(sample));
    }

    // Then all samples remain in memory
    ASSERT_EQ(samples.size(), 20u);

    // And each can be triggered independently
    for (size_t i = 0; i < samples.size(); ++i) {
        EXPECT_NE(samples[i], nullptr);
        EXPECT_GT(samples[i]->sample_count(), 0u);
        EXPECT_NE(samples[i]->data(), nullptr);
    }

    // Verify total memory usage
    size_t total_bytes = 0;
    for (const auto& sample : samples) {
        total_bytes += sample->memory_bytes();
    }

    // 20 samples * 0.1s * 44100 Hz * 2 channels * 4 bytes = ~705 KB
    size_t expected_total = 20 * 0.1 * 44100 * 2 * sizeof(float);
    EXPECT_NEAR(total_bytes, expected_total, expected_total * 0.1);
}

// ============================================================================
// Edge cases
// ============================================================================

TEST_F(SfxTest, RejectsNonExistentFile) {
    auto sample = SoundSample::load("/nonexistent/path/to/file.wav");
    EXPECT_EQ(sample, nullptr);
}

TEST_F(SfxTest, RejectsInvalidFormat) {
    // Create a file that's not a valid WAV
    fs::path path = temp_ / "invalid.wav";
    std::ofstream file(path);
    file << "This is not a WAV file";
    file.close();

    auto sample = SoundSample::load(path);
    EXPECT_EQ(sample, nullptr);
}

TEST_F(SfxTest, HandlesMonoAudio) {
    // Generate mono WAV
    fs::path path = generate_wav("mono.wav", 44100, 1, 0.5);
    ASSERT_FALSE(path.empty());

    auto sample = SoundSample::load(path);
    ASSERT_NE(sample, nullptr);
    EXPECT_EQ(sample->channels(), 1);
    EXPECT_GT(sample->sample_count(), 0u);
}

TEST_F(SfxTest, HandlesDifferentSampleRates) {
    // Test 48 kHz
    fs::path path = generate_wav("48khz.wav", 48000, 2, 0.5);
    ASSERT_FALSE(path.empty());

    auto sample = SoundSample::load(path);
    ASSERT_NE(sample, nullptr);
    EXPECT_EQ(sample->sample_rate(), 48000);
}

TEST_F(SfxTest, LoadsExactly1SecondFile) {
    // Edge case: exactly at the limit
    fs::path path = generate_wav("exact_1s.wav", 44100, 2, 1.0);
    ASSERT_FALSE(path.empty());

    auto sample = SoundSample::load(path);
    ASSERT_NE(sample, nullptr);
    EXPECT_NEAR(sample->duration_sec(), 1.0, 0.01);
}

TEST_F(SfxTest, RejectsJustOver1SecondFile) {
    // Just over the limit
    fs::path path = generate_wav("just_over_1s.wav", 44100, 2, 1.01);
    ASSERT_FALSE(path.empty());

    auto sample = SoundSample::load(path);
    EXPECT_EQ(sample, nullptr);
}
