#include <openitup/audio/sound_sample.h>

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

namespace openitup {

std::unique_ptr<SoundSample> SoundSample::load(const std::filesystem::path& path) {
    // Check file exists
    if (!std::filesystem::exists(path)) {
        spdlog::error("SoundSample::load() file not found: {}", path.string());
        return nullptr;
    }

    // Load audio file using SDL3
    SDL_AudioSpec spec;
    Uint8* audio_buf = nullptr;
    Uint32 audio_len = 0;

    if (!SDL_LoadWAV(path.string().c_str(), &spec, &audio_buf, &audio_len)) {
        spdlog::error("SoundSample::load() failed to load WAV file {}: {}",
                     path.string(), SDL_GetError());
        return nullptr;
    }

    // Calculate duration
    int bytes_per_sample = SDL_AUDIO_BITSIZE(spec.format) / 8;
    int bytes_per_frame = spec.channels * bytes_per_sample;

    if (bytes_per_frame == 0) {
        spdlog::error("SoundSample::load() invalid audio format: {} (zero frame size)",
                     path.string());
        SDL_free(audio_buf);
        return nullptr;
    }

    uint32_t frame_count = audio_len / bytes_per_frame;
    double duration_sec = static_cast<double>(frame_count) / spec.freq;

    // Check duration limit
    if (duration_sec > MAX_SFX_DURATION_SEC) {
        spdlog::error("SoundSample::load() file exceeds duration limit: {} "
                     "({:.2f}s > {:.2f}s)",
                     path.string(), duration_sec, MAX_SFX_DURATION_SEC);
        SDL_free(audio_buf);
        return nullptr;
    }

    // Create sample object
    auto sample = std::unique_ptr<SoundSample>(new SoundSample());
    sample->sample_rate_ = spec.freq;
    sample->channels_ = spec.channels;

    // Convert to float32 [-1.0, 1.0] format
    // SDL3 provides float conversion via audio stream
    SDL_AudioSpec src_spec = spec;
    SDL_AudioSpec dst_spec;
    dst_spec.format = SDL_AUDIO_F32;
    dst_spec.channels = spec.channels;
    dst_spec.freq = spec.freq;

    SDL_AudioStream* stream = SDL_CreateAudioStream(&src_spec, &dst_spec);
    if (!stream) {
        spdlog::error("SoundSample::load() failed to create conversion stream: {}",
                     SDL_GetError());
        SDL_free(audio_buf);
        return nullptr;
    }

    // Put source data into stream
    if (!SDL_PutAudioStreamData(stream, audio_buf, audio_len)) {
        spdlog::error("SoundSample::load() failed to put audio data: {}",
                     SDL_GetError());
        SDL_DestroyAudioStream(stream);
        SDL_free(audio_buf);
        return nullptr;
    }

    // Flush to ensure all data is converted
    SDL_FlushAudioStream(stream);

    // Get converted float data
    int available_bytes = SDL_GetAudioStreamAvailable(stream);
    int float_sample_count = available_bytes / sizeof(float);

    sample->pcm_data_.resize(float_sample_count);

    int bytes_read = SDL_GetAudioStreamData(stream,
                                            sample->pcm_data_.data(),
                                            available_bytes);

    if (bytes_read != available_bytes) {
        spdlog::error("SoundSample::load() failed to read converted data: expected {} bytes, got {}",
                     available_bytes, bytes_read);
        SDL_DestroyAudioStream(stream);
        SDL_free(audio_buf);
        return nullptr;
    }

    // Cleanup
    SDL_DestroyAudioStream(stream);
    SDL_free(audio_buf);

    spdlog::info("Loaded SFX sample: {} ({} Hz, {} ch, {:.3f}s, {} KB)",
                path.filename().string(),
                sample->sample_rate_,
                sample->channels_,
                sample->duration_sec(),
                sample->memory_bytes() / 1024);

    return sample;
}

}  // namespace openitup
