#include <openitup/audio/vorbis_decoder.h>

#include <spdlog/spdlog.h>

#define STB_VORBIS_IMPLEMENTATION
#include "stb_vorbis.h"

namespace openitup {

VorbisDecoder::VorbisDecoder() = default;

VorbisDecoder::~VorbisDecoder() {
    close();
}

bool VorbisDecoder::open(const std::string& filepath) {
    // Close any existing file
    close();

    // Open the vorbis file
    int error = 0;
    vorbis_ = stb_vorbis_open_filename(filepath.c_str(), &error, nullptr);

    if (!vorbis_) {
        spdlog::error("Failed to open OGG file: {} (error code: {})", filepath.c_str(), error);
        return false;
    }

    // Extract format information
    stb_vorbis_info info = stb_vorbis_get_info(vorbis_);
    format_.sample_rate = info.sample_rate;
    format_.channels = info.channels;

    // Get total sample count
    uint64_t total_samples_per_channel = stb_vorbis_stream_length_in_samples(vorbis_);
    format_.total_samples = total_samples_per_channel;

    spdlog::debug("Opened OGG file: {} ({} Hz, {} ch, {} samples)",
                  filepath.c_str(), format_.sample_rate, format_.channels, format_.total_samples);

    return true;
}

int VorbisDecoder::decode(float* buffer, int max_frames) {
    if (!vorbis_) {
        return 0;
    }

    // stb_vorbis_get_samples_float_interleaved returns the number of samples PER CHANNEL decoded
    int frames_decoded = stb_vorbis_get_samples_float_interleaved(
        vorbis_,
        format_.channels,
        buffer,
        max_frames * format_.channels
    );

    return frames_decoded;
}

bool VorbisDecoder::seek_to_sample(uint64_t sample_index) {
    if (!vorbis_) {
        return false;
    }

    // stb_vorbis_seek takes a per-channel sample index
    int result = stb_vorbis_seek(vorbis_, static_cast<unsigned int>(sample_index));

    if (!result) {
        spdlog::warn("Seek failed in OGG file to sample {}", sample_index);
        return false;
    }

    return true;
}

void VorbisDecoder::close() {
    if (vorbis_) {
        stb_vorbis_close(vorbis_);
        vorbis_ = nullptr;
    }
    format_ = {};
}

const AudioFormat& VorbisDecoder::format() const {
    return format_;
}

bool VorbisDecoder::is_open() const {
    return vorbis_ != nullptr;
}

}  // namespace openitup
