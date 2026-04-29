#include <openitup/audio/sdl3_audio_system.h>

#include <algorithm>
#include <cmath>

#include <spdlog/spdlog.h>

#include <openitup/audio/vorbis_decoder.h>
#include <openitup/audio/mp3_decoder.h>

namespace openitup {

SDL3AudioSystem::SDL3AudioSystem() = default;

SDL3AudioSystem::~SDL3AudioSystem() {
    shutdown();
}

bool SDL3AudioSystem::init() {
    if (initialized_) {
        spdlog::warn("SDL3AudioSystem::init() called when already initialized");
        return true;
    }

    // Open default playback device
    device_id_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (device_id_ == 0) {
        spdlog::error("Failed to open audio device: {}", SDL_GetError());
        return false;
    }

    // Create audio stream with float format (will determine channels and sample rate per file)
    // For now, use a placeholder spec - will be reconfigured per music file
    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = 44100;

    stream_ = SDL_CreateAudioStream(&spec, nullptr);
    if (!stream_) {
        spdlog::error("Failed to create audio stream: {}", SDL_GetError());
        SDL_CloseAudioDevice(device_id_);
        device_id_ = 0;
        return false;
    }

    // Set callback for when stream needs data
    if (!SDL_SetAudioStreamGetCallback(stream_, audio_callback, this)) {
        spdlog::error("Failed to set audio stream callback: {}", SDL_GetError());
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
        SDL_CloseAudioDevice(device_id_);
        device_id_ = 0;
        return false;
    }

    // Bind stream to device
    if (!SDL_BindAudioStream(device_id_, stream_)) {
        spdlog::error("Failed to bind audio stream to device: {}", SDL_GetError());
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
        SDL_CloseAudioDevice(device_id_);
        device_id_ = 0;
        return false;
    }

    // Get output device format for resampling correction
    SDL_AudioSpec output_spec;
    if (SDL_GetAudioDeviceFormat(device_id_, &output_spec, nullptr)) {
        output_sample_rate_ = output_spec.freq;
        output_channels_ = output_spec.channels;
        spdlog::info("Output device format: {} Hz, {} channels",
                     output_sample_rate_, output_channels_);
    } else {
        spdlog::warn("Failed to get output device format: {}", SDL_GetError());
        // Assume same as input (will be close enough for most cases)
        output_sample_rate_ = 44100;
        output_channels_ = 2;
    }

    initialized_ = true;
    spdlog::info("SDL3AudioSystem initialized");
    return true;
}

void SDL3AudioSystem::shutdown() {
    if (!initialized_) {
        return;
    }

    // Stop playback
    if (state_ != AudioState::STOPPED) {
        stop();
    }

    // Unbind and destroy stream
    if (stream_) {
        SDL_UnbindAudioStream(stream_);
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }

    // Close device
    if (device_id_ != 0) {
        SDL_CloseAudioDevice(device_id_);
        device_id_ = 0;
    }

    // Close decoder
    if (decoder_) {
        decoder_->close();
        decoder_.reset();
    }

    initialized_ = false;
    spdlog::info("SDL3AudioSystem shut down");
}

std::unique_ptr<AudioDecoder> SDL3AudioSystem::create_decoder(
    const std::filesystem::path& path) {

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".ogg") {
        return std::make_unique<VorbisDecoder>();
    } else if (ext == ".mp3") {
        return std::make_unique<Mp3Decoder>();
    } else {
        spdlog::error("Unsupported audio format: {}", ext);
        return nullptr;
    }
}

bool SDL3AudioSystem::load_music(const std::filesystem::path& path) {
    if (!initialized_) {
        spdlog::error("Cannot load music: audio system not initialized");
        return false;
    }

    std::lock_guard<std::mutex> lock(music_mutex_);

    // Stop current playback if any
    if (state_ != AudioState::STOPPED) {
        state_ = AudioState::STOPPED;
        SDL_PauseAudioDevice(device_id_);
    }

    // Close existing decoder
    if (decoder_) {
        decoder_->close();
        decoder_.reset();
    }

    // Create decoder for this file
    decoder_ = create_decoder(path);
    if (!decoder_) {
        return false;
    }

    // Open the file
    if (!decoder_->open(path.string())) {
        spdlog::error("Failed to open audio file: {}", path.string());
        decoder_.reset();
        return false;
    }

    // Get format info
    source_format_ = decoder_->format();
    spdlog::info("Loaded music: {} ({} Hz, {} ch, {:.2f}s)",
                 path.filename().string(),
                 source_format_.sample_rate,
                 source_format_.channels,
                 static_cast<double>(source_format_.total_samples) /
                     source_format_.sample_rate);

    // Resize decode buffer for one callback worth of data (2048 frames is typical)
    const int buffer_frames = 2048;
    decode_buffer_.resize(buffer_frames * source_format_.channels);

    // Reset position tracking
    seek_base_ = 0;
    samples_fed_ = 0;
    paused_position_ms_ = 0.0;

    state_ = AudioState::STOPPED;
    return true;
}

void SDL3AudioSystem::play() {
    if (!initialized_ || !decoder_ || !decoder_->is_open()) {
        spdlog::warn("Cannot play: no music loaded");
        return;
    }

    std::lock_guard<std::mutex> lock(music_mutex_);

    // Seek to beginning
    decoder_->seek_to_sample(0);

    // Reset position tracking
    seek_base_.store(0, std::memory_order_relaxed);
    samples_fed_.store(0, std::memory_order_relaxed);
    paused_position_ms_ = 0.0;

    // Flush any buffered data
    SDL_FlushAudioStream(stream_);

    // Resume device (starts calling audio_callback)
    SDL_ResumeAudioDevice(device_id_);

    // Set state last so callback sees consistent state
    state_.store(AudioState::PLAYING, std::memory_order_relaxed);

    spdlog::debug("Playback started");
}

void SDL3AudioSystem::pause() {
    if (state_.load(std::memory_order_relaxed) != AudioState::PLAYING) {
        return;
    }

    // Snapshot position before pausing
    paused_position_ms_ = get_position_ms();

    // Pause device (stops calling audio_callback)
    SDL_PauseAudioDevice(device_id_);

    // Set state
    state_.store(AudioState::PAUSED, std::memory_order_relaxed);

    spdlog::debug("Playback paused at {:.2f}ms", paused_position_ms_);
}

void SDL3AudioSystem::resume() {
    if (state_.load(std::memory_order_relaxed) != AudioState::PAUSED) {
        return;
    }

    // Resume device
    SDL_ResumeAudioDevice(device_id_);

    // Set state
    state_.store(AudioState::PLAYING, std::memory_order_relaxed);

    spdlog::debug("Playback resumed");
}

void SDL3AudioSystem::stop() {
    if (!initialized_) {
        return;
    }

    std::lock_guard<std::mutex> lock(music_mutex_);

    // Pause device
    SDL_PauseAudioDevice(device_id_);

    // Reset position tracking
    seek_base_.store(0, std::memory_order_relaxed);
    samples_fed_.store(0, std::memory_order_relaxed);
    paused_position_ms_ = 0.0;

    // Flush stream
    if (stream_) {
        SDL_FlushAudioStream(stream_);
    }

    // Set state
    state_.store(AudioState::STOPPED, std::memory_order_relaxed);

    spdlog::debug("Playback stopped");
}

void SDL3AudioSystem::seek(double position_ms) {
    if (!initialized_ || !decoder_ || !decoder_->is_open()) {
        spdlog::warn("Cannot seek: no music loaded");
        return;
    }

    // Clamp position to valid range
    double duration_ms = get_duration_ms();
    position_ms = std::clamp(position_ms, 0.0, duration_ms);

    std::lock_guard<std::mutex> lock(music_mutex_);

    // Convert ms to sample index
    int64_t target_sample = static_cast<int64_t>(
        (position_ms / 1000.0) * source_format_.sample_rate);

    // Clamp to valid sample range
    if (source_format_.total_samples >= 0) {
        target_sample = std::clamp(target_sample, int64_t(0),
                                    static_cast<int64_t>(source_format_.total_samples));
    }

    // Seek decoder
    if (!decoder_->seek_to_sample(target_sample)) {
        spdlog::error("Decoder seek failed");
        return;
    }

    // Update position tracking
    seek_base_.store(target_sample, std::memory_order_relaxed);
    samples_fed_.store(0, std::memory_order_relaxed);

    // Flush buffered audio (prevent stale audio from playing)
    SDL_FlushAudioStream(stream_);

    // Update paused position if paused
    if (state_.load(std::memory_order_relaxed) == AudioState::PAUSED) {
        paused_position_ms_ = position_ms;
    }

    spdlog::debug("Seeked to {:.2f}ms (sample {})", position_ms, target_sample);
}

double SDL3AudioSystem::get_position_ms() const {
    AudioState current_state = state_.load(std::memory_order_relaxed);

    // If stopped, position is 0
    if (current_state == AudioState::STOPPED) {
        return 0.0;
    }

    // If paused, return frozen position
    if (current_state == AudioState::PAUSED) {
        return paused_position_ms_;
    }

    // Read atomic counters (relaxed ordering is fine - we only need eventual consistency)
    int64_t fed = samples_fed_.load(std::memory_order_relaxed);
    int64_t base = seek_base_.load(std::memory_order_relaxed);

    // Query how many bytes are queued in the SDL stream but not yet consumed
    int queued_bytes = SDL_GetAudioStreamQueued(stream_);

    // Convert queued output bytes to output frames
    int queued_output_frames = queued_bytes / (output_channels_ * sizeof(float));

    // Convert output frames to source frames (account for resampling)
    int64_t queued_source_frames = 0;
    if (output_sample_rate_ > 0 && source_format_.sample_rate > 0) {
        queued_source_frames = static_cast<int64_t>(
            static_cast<double>(queued_output_frames) *
            static_cast<double>(source_format_.sample_rate) /
            static_cast<double>(output_sample_rate_));
    }

    // The position of audio coming out of the speaker right now
    int64_t consumed_samples = base + fed - queued_source_frames;

    // Clamp to non-negative (can briefly go negative during seek transitions)
    if (consumed_samples < 0) {
        consumed_samples = 0;
    }

    // Convert to milliseconds
    if (source_format_.sample_rate == 0) {
        return 0.0;
    }

    return (static_cast<double>(consumed_samples) * 1000.0) /
           static_cast<double>(source_format_.sample_rate);
}

double SDL3AudioSystem::get_duration_ms() const {
    if (!decoder_ || !decoder_->is_open()) {
        return 0.0;
    }
    return (static_cast<double>(source_format_.total_samples) * 1000.0) /
           static_cast<double>(source_format_.sample_rate);
}

AudioState SDL3AudioSystem::get_state() const {
    return state_.load(std::memory_order_relaxed);
}

bool SDL3AudioSystem::is_music_loaded() const {
    return decoder_ && decoder_->is_open();
}

void SDL3AudioSystem::set_music_volume(float volume) {
    music_volume_.store(std::clamp(volume, 0.0f, 1.0f),
                        std::memory_order_relaxed);
}

float SDL3AudioSystem::get_music_volume() const {
    return music_volume_.load(std::memory_order_relaxed);
}

uint32_t SDL3AudioSystem::load_sfx(const std::filesystem::path& path) {
    static bool warned = false;
    if (!warned) {
        spdlog::warn("SDL3AudioSystem::load_sfx() not implemented (Phase 3 feature)");
        warned = true;
    }
    return 0;
}

void SDL3AudioSystem::play_sfx(uint32_t handle) {
    static bool warned = false;
    if (!warned) {
        spdlog::warn("SDL3AudioSystem::play_sfx() not implemented (Phase 3 feature)");
        warned = true;
    }
}

void SDL3AudioSystem::set_sfx_volume(float volume) {
    sfx_volume_.store(std::clamp(volume, 0.0f, 1.0f),
                      std::memory_order_relaxed);
}

float SDL3AudioSystem::get_sfx_volume() const {
    return sfx_volume_.load(std::memory_order_relaxed);
}

void SDL3AudioSystem::audio_callback(void* userdata, SDL_AudioStream* stream,
                                     int additional_amount, int total_amount) {
    auto* self = static_cast<SDL3AudioSystem*>(userdata);
    self->feed_audio(stream, additional_amount);
}

void SDL3AudioSystem::feed_audio(SDL_AudioStream* stream, int requested_bytes) {
    // If not playing, output silence (do nothing - SDL will handle silence)
    if (state_.load(std::memory_order_relaxed) != AudioState::PLAYING) {
        return;
    }

    // Try to acquire mutex - if we can't (main thread is seeking/loading),
    // output silence for this callback rather than blocking
    std::unique_lock<std::mutex> lock(music_mutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
        // Main thread is seeking/loading - skip this callback
        return;
    }

    // Check we have a valid decoder
    if (!decoder_ || !decoder_->is_open()) {
        return;
    }

    // Calculate how many frames we need to decode
    int bytes_per_frame = source_format_.channels * sizeof(float);
    int frames_needed = requested_bytes / bytes_per_frame;

    // Decode loop - keep decoding until we've filled the request
    while (frames_needed > 0) {
        // Decode into our buffer (capped by buffer size)
        int frames_to_decode = std::min(frames_needed,
            static_cast<int>(decode_buffer_.size() / source_format_.channels));

        int64_t frames_decoded = decoder_->decode(decode_buffer_.data(),
                                                   frames_to_decode);

        // End of file reached
        if (frames_decoded == 0) {
            state_.store(AudioState::STOPPED, std::memory_order_relaxed);
            return;
        }

        // Apply volume to each sample
        float volume = music_volume_.load(std::memory_order_relaxed);
        int samples = frames_decoded * source_format_.channels;
        for (int i = 0; i < samples; ++i) {
            decode_buffer_[i] *= volume;
        }

        // Put decoded data into SDL stream
        int bytes_to_put = frames_decoded * bytes_per_frame;
        if (!SDL_PutAudioStreamData(stream, decode_buffer_.data(), bytes_to_put)) {
            spdlog::error("Failed to put audio data into stream: {}", SDL_GetError());
            return;
        }

        // Update samples fed counter (atomic)
        samples_fed_.fetch_add(frames_decoded, std::memory_order_relaxed);

        // Update how many frames we still need
        frames_needed -= frames_decoded;
    }
}

} // namespace openitup
