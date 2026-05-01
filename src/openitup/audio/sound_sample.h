#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace openitup {

// Maximum duration for SFX samples in seconds (1 second)
constexpr double MAX_SFX_DURATION_SEC = 1.0;

// In-memory audio sample for low-latency playback.
// Stores decoded PCM data as float32 [-1.0, 1.0] interleaved.
class SoundSample {
public:
    // Load an audio file into memory.
    // Supports WAV format via SDL3.
    // Returns nullptr if:
    //   - File does not exist
    //   - File format unsupported
    //   - File duration exceeds MAX_SFX_DURATION_SEC
    //   - Decoding fails
    // Logs ERROR on failure.
    static std::unique_ptr<SoundSample> load(const std::filesystem::path& path);

    // Get PCM data (interleaved float32, range [-1.0, 1.0])
    const float* data() const { return pcm_data_.data(); }

    // Get total number of float samples (channels * frames)
    size_t sample_count() const { return pcm_data_.size(); }

    // Get number of frames (sample_count / channels)
    size_t frame_count() const { return pcm_data_.size() / channels_; }

    // Get sample rate in Hz
    int sample_rate() const { return sample_rate_; }

    // Get channel count (1 = mono, 2 = stereo)
    int channels() const { return channels_; }

    // Get duration in seconds
    double duration_sec() const {
        if (sample_rate_ == 0) return 0.0;
        return static_cast<double>(frame_count()) / sample_rate_;
    }

    // Get memory usage in bytes
    size_t memory_bytes() const {
        return pcm_data_.size() * sizeof(float);
    }

private:
    SoundSample() = default;

    std::vector<float> pcm_data_;  // Interleaved PCM data
    int sample_rate_ = 0;           // Hz (e.g., 44100)
    int channels_ = 0;              // 1 = mono, 2 = stereo
};

}  // namespace openitup
