#include <openitup/audio/mp3_decoder.h>

#include <spdlog/spdlog.h>

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

namespace openitup {

Mp3Decoder::Mp3Decoder() = default;

Mp3Decoder::~Mp3Decoder() {
    close();
}

bool Mp3Decoder::open(const std::string& filepath) {
    // Close any existing file
    close();

    // Heap-allocate drmp3 struct (it's ~45KB)
    drmp3* mp3 = new drmp3;
    mp3_ = mp3;

    // Initialize with file
    if (!drmp3_init_file(mp3, filepath.c_str(), nullptr)) {
        spdlog::error("Failed to open MP3 file: {}", filepath.c_str());
        delete mp3;
        mp3_ = nullptr;
        return false;
    }

    // Extract format information
    format_.sample_rate = mp3->sampleRate;
    format_.channels = mp3->channels;

    // Get total sample count
    drmp3_uint64 total_pcm_frames = drmp3_get_pcm_frame_count(mp3);
    format_.total_samples = total_pcm_frames;

    spdlog::debug("Opened MP3 file: {} ({} Hz, {} ch, {} samples)",
                  filepath.c_str(), format_.sample_rate, format_.channels, format_.total_samples);

    return true;
}

int Mp3Decoder::decode(float* buffer, int max_frames) {
    if (!mp3_) {
        return 0;
    }

    drmp3* mp3 = static_cast<drmp3*>(mp3_);
    // drmp3_read_pcm_frames_f32 returns number of frames read
    drmp3_uint64 frames_read = drmp3_read_pcm_frames_f32(mp3, max_frames, buffer);
    return static_cast<int>(frames_read);
}

bool Mp3Decoder::seek_to_sample(uint64_t sample_index) {
    if (!mp3_) {
        return false;
    }

    drmp3* mp3 = static_cast<drmp3*>(mp3_);
    // drmp3_seek_to_pcm_frame takes a frame index (per-channel sample index)
    drmp3_bool32 result = drmp3_seek_to_pcm_frame(mp3, sample_index);

    if (!result) {
        spdlog::warn("Seek failed in MP3 file to sample {}", sample_index);
        return false;
    }

    return true;
}

void Mp3Decoder::close() {
    if (mp3_) {
        drmp3* mp3 = static_cast<drmp3*>(mp3_);
        drmp3_uninit(mp3);
        delete mp3;
        mp3_ = nullptr;
    }
    format_ = {};
}

const AudioFormat& Mp3Decoder::format() const {
    return format_;
}

bool Mp3Decoder::is_open() const {
    return mp3_ != nullptr;
}

}  // namespace openitup
