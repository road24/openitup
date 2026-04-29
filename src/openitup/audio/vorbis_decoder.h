#pragma once

#include <openitup/audio/audio_decoder.h>

// Forward declaration - stb_vorbis.h included only in .cpp
struct stb_vorbis;

namespace openitup {

class VorbisDecoder : public AudioDecoder {
public:
    VorbisDecoder();
    ~VorbisDecoder() override;

    VorbisDecoder(const VorbisDecoder&) = delete;
    VorbisDecoder& operator=(const VorbisDecoder&) = delete;

    bool open(const std::string& filepath) override;
    int decode(float* buffer, int max_frames) override;
    bool seek_to_sample(uint64_t sample_index) override;
    void close() override;
    const AudioFormat& format() const override;
    bool is_open() const override;

private:
    stb_vorbis* vorbis_ = nullptr;
    AudioFormat format_;
};

}  // namespace openitup
