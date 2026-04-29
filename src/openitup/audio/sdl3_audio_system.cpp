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
    spdlog::warn("SDL3AudioSystem::play() not yet implemented (stub)");
}

void SDL3AudioSystem::pause() {
    spdlog::warn("SDL3AudioSystem::pause() not yet implemented (stub)");
}

void SDL3AudioSystem::resume() {
    spdlog::warn("SDL3AudioSystem::resume() not yet implemented (stub)");
}

void SDL3AudioSystem::stop() {
    spdlog::warn("SDL3AudioSystem::stop() not yet implemented (stub)");
}

void SDL3AudioSystem::seek(double position_ms) {
    spdlog::warn("SDL3AudioSystem::seek() not yet implemented (stub)");
}

double SDL3AudioSystem::get_position_ms() const {
    // Stub: return 0.0 for now
    return 0.0;
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
    // Stub: do nothing for now
    // Will be implemented in Step 8
}

void SDL3AudioSystem::feed_audio(SDL_AudioStream* stream, int requested_bytes) {
    // Stub: do nothing for now
    // Will be implemented in Step 8
}

} // namespace openitup
