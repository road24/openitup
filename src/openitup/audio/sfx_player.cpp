#include <openitup/audio/sfx_player.h>

#include <algorithm>
#include <cmath>

#include <spdlog/spdlog.h>

namespace openitup {

SfxPlayer::SfxPlayer() = default;

SfxPlayer::~SfxPlayer() {
    shutdown();
}

bool SfxPlayer::init(SDL_AudioDeviceID device_id, int sample_rate, int channels) {
    if (initialized_) {
        spdlog::warn("SfxPlayer::init() called when already initialized");
        return true;
    }

    if (device_id == 0) {
        spdlog::error("SfxPlayer::init() failed: invalid device_id");
        return false;
    }

    device_id_ = device_id;
    sample_rate_ = sample_rate;
    channels_ = channels;

    // Create audio stream with float format
    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = channels;
    spec.freq = sample_rate;

    stream_ = SDL_CreateAudioStream(&spec, &spec);
    if (!stream_) {
        spdlog::error("Failed to create SFX audio stream: {}", SDL_GetError());
        return false;
    }

    // Set callback for when stream needs data
    if (!SDL_SetAudioStreamGetCallback(stream_, audio_callback, this)) {
        spdlog::error("Failed to set SFX audio stream callback: {}", SDL_GetError());
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
        return false;
    }

    // Bind stream to device
    if (!SDL_BindAudioStream(device_id_, stream_)) {
        spdlog::error("Failed to bind SFX audio stream to device: {}", SDL_GetError());
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
        return false;
    }

    // Allocate mix buffer (2048 frames is typical callback size)
    const int buffer_frames = 2048;
    mix_buffer_.resize(buffer_frames * channels_);

    initialized_ = true;
    spdlog::info("SfxPlayer initialized: {} Hz, {} channels, {} voices",
                 sample_rate_, channels_, MAX_SFX_VOICES);
    return true;
}

void SfxPlayer::shutdown() {
    if (!initialized_) {
        return;
    }

    // Unbind and destroy stream
    if (stream_) {
        SDL_UnbindAudioStream(stream_);
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }

    // Clear all voices
    std::lock_guard<std::mutex> lock(voice_mutex_);
    for (auto& voice : voices_) {
        voice.active = false;
        voice.sample = nullptr;
        voice.position = 0;
    }

    initialized_ = false;
    spdlog::info("SfxPlayer shut down");
}

bool SfxPlayer::play(const SoundSample& sample, float volume) {
    if (!initialized_) {
        spdlog::warn("Cannot play SFX: SfxPlayer not initialized");
        return false;
    }

    // Clamp volume
    volume = std::clamp(volume, 0.0f, 1.0f);

    // Allocate a voice
    SfxVoice* voice = allocate_voice();
    if (!voice) {
        spdlog::warn("Cannot play SFX: all {} voices in use", MAX_SFX_VOICES);
        return false;
    }

    // Set up the voice
    voice->sample = &sample;
    voice->position = 0;
    voice->volume = volume;
    voice->active = true;

    return true;
}

void SfxPlayer::set_volume(float volume) {
    volume_.store(std::clamp(volume, 0.0f, 1.0f), std::memory_order_relaxed);
}

float SfxPlayer::get_volume() const {
    return volume_.load(std::memory_order_relaxed);
}

SfxVoice* SfxPlayer::allocate_voice() {
    std::lock_guard<std::mutex> lock(voice_mutex_);

    // Find an inactive voice
    for (auto& voice : voices_) {
        if (!voice.active) {
            return &voice;
        }
    }

    return nullptr;  // All voices in use
}

void SfxPlayer::audio_callback(void* userdata, SDL_AudioStream* stream,
                                int additional_amount, int total_amount) {
    auto* self = static_cast<SfxPlayer*>(userdata);
    self->mix_audio(stream, additional_amount);
}

void SfxPlayer::mix_audio(SDL_AudioStream* stream, int requested_bytes) {
    // Calculate number of frames to mix
    int bytes_per_frame = channels_ * sizeof(float);
    int frames_to_mix = requested_bytes / bytes_per_frame;

    // Clamp to our buffer size
    frames_to_mix = std::min(frames_to_mix,
                             static_cast<int>(mix_buffer_.size() / channels_));

    if (frames_to_mix <= 0) {
        return;
    }

    // Clear mix buffer
    std::fill(mix_buffer_.begin(),
              mix_buffer_.begin() + frames_to_mix * channels_,
              0.0f);

    // Get global volume
    float global_volume = volume_.load(std::memory_order_relaxed);

    // Mix all active voices
    // We don't lock here because:
    // 1. Reading voice.active is atomic (bool read is atomic on all platforms)
    // 2. Once a voice is set active with sample pointer, those fields don't change
    // 3. We only write to position, which is our own field
    // 4. Setting active=false at the end is safe (simple write)
    for (auto& voice : voices_) {
        if (!voice.active) {
            continue;
        }

        const SoundSample* sample = voice.sample;
        if (!sample || !sample->data()) {
            voice.active = false;
            continue;
        }

        // Check if sample format matches our output format
        // If not, we need to resample (for now, skip incompatible samples)
        if (sample->sample_rate() != sample_rate_ || sample->channels() != channels_) {
            spdlog::warn("SFX sample format mismatch: {}Hz/{}ch vs output {}Hz/{}ch - skipping",
                        sample->sample_rate(), sample->channels(),
                        sample_rate_, channels_);
            voice.active = false;
            continue;
        }

        const float* sample_data = sample->data();
        size_t sample_frames = sample->frame_count();
        size_t voice_position = voice.position;

        // Calculate how many frames we can play from this sample
        size_t frames_available = sample_frames - voice_position;
        size_t frames_to_play = std::min(static_cast<size_t>(frames_to_mix),
                                        frames_available);

        // Mix this voice into the output buffer
        float voice_volume = voice.volume * global_volume;
        for (size_t frame = 0; frame < frames_to_play; ++frame) {
            size_t sample_offset = (voice_position + frame) * channels_;
            size_t mix_offset = frame * channels_;

            for (int ch = 0; ch < channels_; ++ch) {
                mix_buffer_[mix_offset + ch] +=
                    sample_data[sample_offset + ch] * voice_volume;
            }
        }

        // Update voice position
        voice.position += frames_to_play;

        // Deactivate voice if sample finished
        if (voice.position >= sample_frames) {
            voice.active = false;
            voice.sample = nullptr;
            voice.position = 0;
        }
    }

    // Apply soft clipping to prevent hard clipping artifacts
    // Using tanh for smooth saturation
    int total_samples = frames_to_mix * channels_;
    for (int i = 0; i < total_samples; ++i) {
        float sample = mix_buffer_[i];

        // Clamp to [-1.0, 1.0] to prevent hard clipping
        // For better quality, we could use soft clipping (tanh), but simple clamp
        // is sufficient and cheaper
        if (sample > 1.0f) {
            sample = 1.0f;
        } else if (sample < -1.0f) {
            sample = -1.0f;
        }

        mix_buffer_[i] = sample;
    }

    // Put mixed data into SDL stream
    int bytes_to_put = frames_to_mix * bytes_per_frame;
    if (!SDL_PutAudioStreamData(stream, mix_buffer_.data(), bytes_to_put)) {
        spdlog::error("Failed to put SFX audio data into stream: {}", SDL_GetError());
    }
}

}  // namespace openitup
