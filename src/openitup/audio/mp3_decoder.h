#pragma once

#include <openitup/audio/audio_decoder.h>

namespace openitup {

class Mp3Decoder : public AudioDecoder {
public:
    Mp3Decoder();
    ~Mp3Decoder() override;

    Mp3Decoder(const Mp3Decoder&) = delete;
    Mp3Decoder& operator=(const Mp3Decoder&) = delete;

    bool open(const std::string& filepath) override;
    int decode(float* buffer, int max_frames) override;
    bool seek_to_sample(uint64_t sample_index) override;
    void close() override;
    const AudioFormat& format() const override;
    bool is_open() const override;

private:
    void* mp3_ = nullptr;  // Opaque pointer to drmp3 (heap-allocated, ~45KB struct)
    AudioFormat format_;
};

}  // namespace openitup
