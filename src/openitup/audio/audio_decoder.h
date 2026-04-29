#pragma once

#include <cstdint>
#include <string>

namespace openitup {

struct AudioFormat {
    int sample_rate = 0;       // Hz (e.g., 44100)
    int channels = 0;          // 1 = mono, 2 = stereo
    uint64_t total_samples = 0;  // Total samples per channel
};

class AudioDecoder {
public:
    virtual ~AudioDecoder() = default;

    // Open audio file and prepare for decoding
    virtual bool open(const std::string& filepath) = 0;

    // Decode frames into interleaved float buffer [-1.0, 1.0]
    // Returns number of frames decoded (one frame = one sample per channel)
    virtual int decode(float* buffer, int max_frames) = 0;

    // Seek to sample index (per-channel count)
    virtual bool seek_to_sample(uint64_t sample_index) = 0;

    // Close and release resources
    virtual void close() = 0;

    // Query format
    virtual const AudioFormat& format() const = 0;

    // Is the decoder currently open?
    virtual bool is_open() const = 0;
};

}  // namespace openitup
