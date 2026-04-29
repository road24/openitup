#pragma once

#include <string>

namespace openitup {

enum class AudioState {
    STOPPED,
    PLAYING,
    PAUSED
};

class AudioSystem {
public:
    virtual ~AudioSystem() = default;

    // Music playback
    virtual bool load_music(const std::string& filepath) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek(double position_ms) = 0;

    // Query
    virtual AudioState state() const = 0;
    virtual double position() const = 0;  // milliseconds
    virtual double duration() const = 0;  // milliseconds

    // Volume control
    virtual void set_music_volume(float volume) = 0;  // 0.0-1.0
    virtual float music_volume() const = 0;

    // SFX (Phase 3, no-op in Phase 1)
    virtual void set_sfx_volume(float volume) = 0;
    virtual float sfx_volume() const = 0;
};

}  // namespace openitup
