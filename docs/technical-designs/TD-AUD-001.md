# TD-AUD-001: Audio System — Interface, SDL3 Backend, and Sample-Accurate Position Tracking

**Stories**: US-AUD-091, US-AUD-081, US-AUD-092, US-AUD-001, US-AUD-002, US-AUD-003, US-AUD-004, US-AUD-011, US-AUD-021
**Phase**: 1
**Author**: technical-architect agent
**Status**: Draft

## Overview

This design introduces the audio subsystem for Phase 1: an abstract `AudioSystem` interface hiding all backend details, an `SDL3AudioSystem` concrete implementation using SDL3's `SDL_AudioStream` push-based API, format-agnostic music decoding (OGG Vorbis via stb_vorbis, MP3 via dr_mp3), and sample-accurate position tracking via atomic counters in the audio callback. The audio system is owned by `Engine` as `std::unique_ptr<AudioSystem>`, following the ownership pattern established in TD-ENG-001. The design prioritizes the single most important requirement in a rhythm game engine: `get_position_ms()` must return the position of audio **consumed by hardware**, not audio submitted to a buffer.

Phase 1 covers music playback only. SFX methods are declared in the interface (for Phase 3 forward compatibility) but their implementations in `SDL3AudioSystem` are no-ops that log a warning. This avoids breaking the interface contract when SFX stories are implemented later.

## Architecture

### Component Diagram

```
Engine (src/openitup/core/engine.h)
  |  owns (unique_ptr)
  |
  └── AudioSystem* (interface: src/openitup/audio/audio_system.h)
        |
        └── SDL3AudioSystem (src/openitup/audio/sdl3_audio_system.h)
              |  owns
              ├── SDL_AudioStream* (output stream to hardware)
              ├── AudioDecoder* (interface)
              |     ├── VorbisDecoder (stb_vorbis)
              |     └── Mp3Decoder (dr_mp3)
              └── Atomic position tracking state
                    ├── samples_consumed_ (std::atomic<int64_t>)
                    ├── sample_rate_ (int)
                    └── paused_position_ms_ (double)

GameplayScene::update(dt):
  1. double song_ms = audio_->get_position_ms();
  2. judge_.update(song_ms, input_snapshot);
```

### New Types

#### `AudioSystem` (`src/openitup/audio/audio_system.h`)

The pure virtual interface. No SDL3 types, no backend headers. Any consumer that needs audio includes only this file.

```cpp
// src/openitup/audio/audio_system.h
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace openitup {

// Current state of the audio system's music transport.
enum class AudioState : uint8_t {
    STOPPED,
    PLAYING,
    PAUSED,
};

class AudioSystem {
public:
    virtual ~AudioSystem() = default;

    // --- Lifecycle ---

    // Initialize the audio backend. Returns true on success.
    // On failure, logs ERROR and returns false (engine continues without audio).
    virtual bool init() = 0;

    // Shut down the audio backend. Safe to call multiple times.
    virtual void shutdown() = 0;

    // --- Music stream ---

    // Load a music file (OGG or MP3). Replaces any currently loaded music.
    // Returns true on success. On failure, logs ERROR with path and reason.
    virtual bool load_music(const std::filesystem::path& path) = 0;

    // Transport controls.
    virtual void play() = 0;      // Play from beginning (position 0).
    virtual void pause() = 0;     // Pause at current position.
    virtual void resume() = 0;    // Resume from paused position.
    virtual void stop() = 0;      // Stop and reset position to 0.

    // Seek to a millisecond position. Clamps to [0, duration].
    // Works in any state (PLAYING, PAUSED, STOPPED).
    virtual void seek(double position_ms) = 0;

    // THE critical method. Returns playback position in milliseconds based
    // on samples consumed by audio hardware (not samples submitted).
    // Returns 0.0 if stopped. Returns frozen position if paused.
    virtual double get_position_ms() const = 0;

    // Duration of loaded music in milliseconds. Returns 0.0 if no music loaded.
    virtual double get_duration_ms() const = 0;

    // Current transport state.
    virtual AudioState get_state() const = 0;

    // Is a music file currently loaded (regardless of play state)?
    virtual bool is_music_loaded() const = 0;

    // --- Volume ---

    // Set music volume. Range: 0.0 (silent) to 1.0 (full). Clamped.
    virtual void set_music_volume(float volume) = 0;

    // Get current music volume.
    virtual float get_music_volume() const = 0;

    // --- SFX (Phase 3 — declared here for interface stability) ---

    // Load a short audio sample for fire-and-forget playback.
    // Returns an opaque handle, or 0 on failure.
    virtual uint32_t load_sfx(const std::filesystem::path& path) = 0;

    // Play a loaded SFX sample. Fire-and-forget.
    virtual void play_sfx(uint32_t handle) = 0;

    // Set SFX volume. Range: 0.0 to 1.0. Clamped.
    virtual void set_sfx_volume(float volume) = 0;

    // Get current SFX volume.
    virtual float get_sfx_volume() const = 0;
};

} // namespace openitup
```

**Key decisions**:

- `double` for `get_position_ms()` and `seek()`: milliseconds as floating-point avoids integer truncation at sample boundaries. At 44.1 kHz, one sample is ~0.0227 ms -- `double` preserves this precision over multi-hour sessions.
- `AudioState` enum is separate from implementation state machines. It represents the three states visible to consumers. The implementation may have internal sub-states (e.g., "seeking" is a transient state within PLAYING).
- SFX methods are in the interface now. Phase 1 SDL3AudioSystem implements them as logged no-ops. This avoids an interface-breaking change in Phase 3.
- `float` for volume (0.0-1.0) is sufficient. Volume is a perceptual parameter with ~6 decimal digits of range, well within float precision.
- No `update()` method on AudioSystem. The audio callback runs on its own thread and pushes data to hardware independently of the game loop. The game loop only reads position via `get_position_ms()`. This is a critical architectural boundary: the audio thread is never gated by the game loop, preventing audio glitches during frame drops.

---

#### `AudioDecoder` (`src/openitup/audio/audio_decoder.h`)

An internal interface for format-agnostic audio decoding. Not part of the public API -- only used within `SDL3AudioSystem`.

```cpp
// src/openitup/audio/audio_decoder.h
#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace openitup {

// Decoded audio format info, set after open().
struct AudioFormat {
    int sample_rate;     // e.g., 44100, 48000
    int channels;        // 1 (mono) or 2 (stereo)
    int64_t total_samples; // Total samples per channel. -1 if unknown.
};

class AudioDecoder {
public:
    virtual ~AudioDecoder() = default;

    // Open a file for decoding. Returns true on success.
    virtual bool open(const std::filesystem::path& path) = 0;

    // Close the file and release decoder resources.
    virtual void close() = 0;

    // Get the audio format of the opened file.
    virtual AudioFormat format() const = 0;

    // Decode up to max_frames frames of interleaved float samples into buffer.
    // A "frame" is one sample per channel (so stereo = 2 floats per frame).
    // Returns number of frames actually decoded. Returns 0 at end of file.
    virtual int64_t decode(float* buffer, int64_t max_frames) = 0;

    // Seek to a sample position (per-channel sample index).
    // Returns true on success.
    virtual bool seek_to_sample(int64_t sample_index) = 0;

    // Is the decoder currently open with a valid file?
    virtual bool is_open() const = 0;
};

} // namespace openitup
```

**Key decisions**:

- Decoder outputs interleaved float samples normalized to [-1.0, 1.0]. This is the native format SDL3 audio streams work with (`SDL_AUDIO_F32`), eliminating a format conversion step.
- `decode()` uses a "pull N frames" model. The SDL3AudioSystem callback calls this to fill its audio buffer. Frames (not samples) are the unit because it simplifies stereo/mono handling.
- `total_samples` can be -1 for formats where total length is unknown until the file is fully decoded (rare for music files, but defensive).

---

#### `VorbisDecoder` (`src/openitup/audio/vorbis_decoder.h`)

OGG Vorbis decoding via stb_vorbis (single-header, public domain).

```cpp
// src/openitup/audio/vorbis_decoder.h
#pragma once

#include <openitup/audio/audio_decoder.h>

// Forward declaration -- stb_vorbis is included only in the .cpp
struct stb_vorbis;

namespace openitup {

class VorbisDecoder : public AudioDecoder {
public:
    VorbisDecoder();
    ~VorbisDecoder() override;

    VorbisDecoder(const VorbisDecoder&) = delete;
    VorbisDecoder& operator=(const VorbisDecoder&) = delete;

    bool open(const std::filesystem::path& path) override;
    void close() override;
    AudioFormat format() const override;
    int64_t decode(float* buffer, int64_t max_frames) override;
    bool seek_to_sample(int64_t sample_index) override;
    bool is_open() const override;

private:
    stb_vorbis* vorbis_ = nullptr;
    AudioFormat format_ = {};
};

} // namespace openitup
```

---

#### `Mp3Decoder` (`src/openitup/audio/mp3_decoder.h`)

MP3 decoding via dr_mp3 (single-header, public domain).

```cpp
// src/openitup/audio/mp3_decoder.h
#pragma once

#include <openitup/audio/audio_decoder.h>

// Forward declaration -- dr_mp3 types included only in the .cpp
struct drmp3;

namespace openitup {

class Mp3Decoder : public AudioDecoder {
public:
    Mp3Decoder();
    ~Mp3Decoder() override;

    Mp3Decoder(const Mp3Decoder&) = delete;
    Mp3Decoder& operator=(const Mp3Decoder&) = delete;

    bool open(const std::filesystem::path& path) override;
    void close() override;
    AudioFormat format() const override;
    int64_t decode(float* buffer, int64_t max_frames) override;
    bool seek_to_sample(int64_t sample_index) override;
    bool is_open() const override;

private:
    drmp3* mp3_ = nullptr;  // Heap-allocated to avoid including dr_mp3.h
    AudioFormat format_ = {};
};

} // namespace openitup
```

**Key decision**: `drmp3* mp3_` is heap-allocated (via `new drmp3` in open()) rather than a direct member. The `drmp3` struct is defined in `dr_mp3.h` and is fairly large (~45 KB). Forward-declaring and heap-allocating keeps the header clean without requiring callers to include dr_mp3.

---

#### `SDL3AudioSystem` (`src/openitup/audio/sdl3_audio_system.h`)

The SDL3-backed implementation of `AudioSystem`. This is the only Phase 1 backend.

```cpp
// src/openitup/audio/sdl3_audio_system.h
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include <SDL3/SDL.h>

#include <openitup/audio/audio_system.h>
#include <openitup/audio/audio_decoder.h>

namespace openitup {

class SDL3AudioSystem : public AudioSystem {
public:
    SDL3AudioSystem();
    ~SDL3AudioSystem() override;

    SDL3AudioSystem(const SDL3AudioSystem&) = delete;
    SDL3AudioSystem& operator=(const SDL3AudioSystem&) = delete;

    // --- AudioSystem interface ---

    bool init() override;
    void shutdown() override;

    bool load_music(const std::filesystem::path& path) override;
    void play() override;
    void pause() override;
    void resume() override;
    void stop() override;
    void seek(double position_ms) override;

    double get_position_ms() const override;
    double get_duration_ms() const override;
    AudioState get_state() const override;
    bool is_music_loaded() const override;

    void set_music_volume(float volume) override;
    float get_music_volume() const override;

    // Phase 3 stubs
    uint32_t load_sfx(const std::filesystem::path& path) override;
    void play_sfx(uint32_t handle) override;
    void set_sfx_volume(float volume) override;
    float get_sfx_volume() const override;

private:
    // SDL3 audio stream callback. Called on the audio thread.
    static void audio_callback(void* userdata, SDL_AudioStream* stream,
                               int additional_amount, int total_amount);

    // Feed decoded audio data to the SDL stream. Called from audio_callback.
    void feed_audio(SDL_AudioStream* stream, int requested_bytes);

    // Create an AudioDecoder for the given file based on extension.
    static std::unique_ptr<AudioDecoder> create_decoder(
        const std::filesystem::path& path);

    // --- State ---

    SDL_AudioStream* stream_ = nullptr;
    SDL_AudioDeviceID device_id_ = 0;

    std::unique_ptr<AudioDecoder> decoder_;
    AudioFormat source_format_ = {};

    // Transport state (main-thread owned, atomic for cross-thread reads).
    std::atomic<AudioState> state_{AudioState::STOPPED};

    // --- Sample-accurate position tracking ---

    // Samples fed to the SDL audio stream (per-channel count).
    // Written by audio callback thread.
    std::atomic<int64_t> samples_fed_{0};

    // Base sample offset after seek. When we seek to sample N,
    // we set seek_base_ = N and reset samples_fed_ = 0.
    // Position = seek_base_ + samples_fed_ - queued_samples.
    std::atomic<int64_t> seek_base_{0};

    // Position snapshot frozen at pause time (in milliseconds).
    // Avoids returning a stale position while the stream drains.
    double paused_position_ms_ = 0.0;

    // --- Decode buffer ---

    // Intermediate buffer for decoding. Sized for one callback worth of data.
    std::vector<float> decode_buffer_;

    // --- Volume ---

    std::atomic<float> music_volume_{1.0f};
    std::atomic<float> sfx_volume_{1.0f};

    // --- Mutex for seek/load operations ---
    // Protects decoder_ and state transitions that must be atomic
    // with respect to the audio callback.
    std::mutex music_mutex_;

    bool initialized_ = false;
};

} // namespace openitup
```

**Key decisions**:

- `std::atomic` for all state shared between the main thread and the audio callback thread. The callback runs on SDL's audio thread -- standard mutex locking inside the callback would risk priority inversion and audio glitches. Atomics are lock-free on all target platforms for `int64_t` and `float`.
- `music_mutex_` is used ONLY for operations that need to synchronize with the audio callback in a non-trivial way: `load_music()`, `seek()`, and `stop()`. These operations modify the decoder state and reset the sample counter. The mutex is held briefly, and the audio callback uses `try_lock()` -- if it cannot acquire the lock, it outputs silence for that callback instead of blocking. This prevents the audio thread from ever blocking.
- `samples_fed_` + `seek_base_` track position. On seek, `seek_base_` is set to the target sample and `samples_fed_` resets to 0. The position formula is: `position_samples = seek_base_ + samples_fed_ - queued_but_not_consumed`. This separation prevents the need for a 64-bit atomic read-modify-write on the hot path.
- The decode buffer (`decode_buffer_`) is pre-allocated to avoid allocations in the audio callback.

---

### Modified Types

#### `Engine` (`src/openitup/core/engine.h`)

- Add member: `std::unique_ptr<AudioSystem> audio_` -- Audio subsystem owned by Engine
- Add accessor: `AudioSystem* get_audio()` / `const AudioSystem* get_audio() const` -- Returns raw pointer (nullptr if no audio). Pointer, not reference, because audio init can fail gracefully.
- Add to constructor: Accept optional `std::unique_ptr<AudioSystem>` for injection
- Add to `init_audio()` private method: Initialize SDL3AudioSystem if not injected
- Add to destructor/shutdown: `audio_->shutdown()` before Renderer shutdown
- Reason: Engine is the root of the object graph. Per TD-ENG-001, audio is destroyed after SceneStack but before Renderer. The null-check pattern (`if (audio_)`) matches the null-check pattern planned for InputSystem in TD-INP-001.

#### `Renderer` (`src/openitup/gfx/renderer.cpp`)

- Lift `SDL_Init(SDL_INIT_VIDEO)` out of `Renderer::init()` into `Engine::init_sdl()`. Engine calls `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)` centrally, and `SDL_Quit()` in Engine's destructor. Renderer::init() no longer calls `SDL_Init()`, and Renderer::shutdown() no longer calls `SDL_Quit()`.
- Reason: TD-ENG-001 explicitly called this out as a required change when AudioSystem is added. Both video and audio SDL subsystems must be initialized, and they share `SDL_Quit()`.

## File Plan

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/openitup/audio/audio_system.h` | AudioSystem pure virtual interface + AudioState enum |
| Create | `src/openitup/audio/audio_decoder.h` | AudioDecoder internal interface + AudioFormat struct |
| Create | `src/openitup/audio/vorbis_decoder.h` | VorbisDecoder class declaration |
| Create | `src/openitup/audio/vorbis_decoder.cpp` | VorbisDecoder implementation using stb_vorbis |
| Create | `src/openitup/audio/mp3_decoder.h` | Mp3Decoder class declaration |
| Create | `src/openitup/audio/mp3_decoder.cpp` | Mp3Decoder implementation using dr_mp3 |
| Create | `src/openitup/audio/sdl3_audio_system.h` | SDL3AudioSystem class declaration |
| Create | `src/openitup/audio/sdl3_audio_system.cpp` | SDL3AudioSystem full implementation |
| Create | `third_party/stb_vorbis.h` | stb_vorbis single-header library (vendored) |
| Create | `third_party/dr_mp3.h` | dr_mp3 single-header library (vendored) |
| Modify | `src/openitup/core/engine.h` | Add `unique_ptr<AudioSystem> audio_`, accessor, injection ctor param |
| Modify | `src/openitup/core/engine.cpp` | Add `init_audio()`, centralized `SDL_Init`, audio shutdown |
| Modify | `src/openitup/gfx/renderer.cpp` | Remove `SDL_Init()`/`SDL_Quit()` calls |
| Modify | `CMakeLists.txt` | Add audio source files, include third_party/ |
| Create | `test/test_audio_decoder.cpp` | Unit tests for VorbisDecoder and Mp3Decoder |
| Create | `test/test_audio_system.cpp` | Unit + integration tests for SDL3AudioSystem |
| Create | `test/fixtures/test_tone_44100.ogg` | 1-second 440 Hz sine wave, 44.1 kHz stereo OGG |
| Create | `test/fixtures/test_tone_48000.ogg` | 1-second 440 Hz sine wave, 48 kHz mono OGG |
| Create | `test/fixtures/test_tone_44100.mp3` | 1-second 440 Hz sine wave, 44.1 kHz stereo MP3 |

## Data Flow

### Music Loading (US-AUD-001, US-AUD-002)

```
1. Caller: audio_->load_music("/path/to/song.ogg")
2. SDL3AudioSystem::load_music():
   a. Determine format from file extension (.ogg → VorbisDecoder, .mp3 → Mp3Decoder)
   b. Lock music_mutex_
   c. If music is currently playing, stop() first
   d. Create new AudioDecoder via create_decoder(path)
   e. Call decoder_->open(path)
   f. On failure: log error, return false
   g. On success: store decoder_, read source_format_
   h. Resize decode_buffer_ for callback buffer size
   i. Reset position state: seek_base_ = 0, samples_fed_ = 0
   j. Set state_ = STOPPED
   k. Unlock music_mutex_
   l. Return true
```

### Playback Start (US-AUD-003)

```
1. Caller: audio_->play()
2. SDL3AudioSystem::play():
   a. If no music loaded, log warning, return
   b. Lock music_mutex_
   c. decoder_->seek_to_sample(0)  // rewind to beginning
   d. seek_base_ = 0, samples_fed_ = 0
   e. Flush the SDL_AudioStream (clear any buffered data)
   f. Unlock music_mutex_
   g. SDL_ResumeAudioStreamDevice(stream_)  // start hardware output
   h. Set state_ = PLAYING
```

### Audio Callback — The Core Data Path

```
SDL audio thread calls audio_callback() when hardware needs more data:

1. audio_callback(userdata, stream, additional_amount, total_amount):
   a. Cast userdata to SDL3AudioSystem*
   b. Call self->feed_audio(stream, additional_amount)

2. feed_audio(stream, requested_bytes):
   a. If state_ != PLAYING, put silence and return
   b. Try to acquire music_mutex_ (try_lock)
      - If lock fails: put silence for this callback (seek/load in progress)
      - If lock succeeds:
   c. Calculate frames_needed = requested_bytes / (channels * sizeof(float))
   d. Decode loop:
      i.   frames_decoded = decoder_->decode(decode_buffer_.data(), frames_needed)
      ii.  If frames_decoded == 0: end of file reached
           - Set state_ = STOPPED
           - Unlock mutex, return
      iii. Apply volume: multiply each sample by music_volume_.load()
      iv.  SDL_PutAudioStreamData(stream, decode_buffer_.data(),
               frames_decoded * channels * sizeof(float))
      v.   samples_fed_.fetch_add(frames_decoded, std::memory_order_relaxed)
      vi.  frames_needed -= frames_decoded
      vii. If frames_needed > 0, loop (partial decode)
   e. Unlock music_mutex_
```

### Sample-Accurate Position (US-AUD-011) — THE Critical Algorithm

This is the most important algorithm in the audio system. The goal: return the millisecond position of the audio sample currently leaving the speaker, not the sample most recently decoded.

```
SDL3AudioSystem::get_position_ms() const:

  1. If state_ == STOPPED, return 0.0
  2. If state_ == PAUSED, return paused_position_ms_

  3. // Read atomics (relaxed order is fine — we only need eventual consistency
  4. // within one frame, and these are monotonically increasing)
  5. int64_t fed = samples_fed_.load(std::memory_order_relaxed);
  6. int64_t base = seek_base_.load(std::memory_order_relaxed);

  7. // Query how many bytes are queued in the SDL stream but not yet consumed.
  8. // SDL_GetAudioStreamQueued() returns bytes.
  9. int queued_bytes = SDL_GetAudioStreamQueued(stream_);
  10. int queued_frames = queued_bytes / (source_format_.channels * sizeof(float));

  11. // The position of audio coming out of the speaker right now:
  12. int64_t consumed_samples = base + fed - queued_frames;

  13. // Clamp to non-negative (can briefly go negative during seek transitions)
  14. if (consumed_samples < 0) consumed_samples = 0;

  15. // Convert to milliseconds
  16. return (static_cast<double>(consumed_samples) * 1000.0)
         / static_cast<double>(source_format_.sample_rate);
```

**Why this works**:

- `samples_fed_` counts how many frames we have pushed into the SDL audio stream since the last seek.
- `SDL_GetAudioStreamQueued()` tells us how many bytes are sitting in the stream buffer waiting to be consumed by hardware.
- The difference `fed - queued_frames` is the number of frames that have actually left the buffer and been sent to the DAC.
- Adding `seek_base_` gives the absolute position within the song.
- This accounts for the hardware buffer size automatically: if we've pushed 4096 frames but 512 are still queued, the position reflects the 3584 that have been consumed.

**Why relaxed memory ordering is sufficient**:

- `get_position_ms()` is called from the main thread at ~60 Hz.
- `samples_fed_` is incremented on the audio thread at ~375 Hz (for a 2048-sample buffer at 48 kHz).
- A one-callback delay in visibility means position is stale by ~5.3ms at most (2048 samples / 48000 * 1000).
- The `SDL_GetAudioStreamQueued()` call is on the main thread and reads SDL's internal state, which naturally synchronizes to the most recent callback.
- At 60 Hz query rate, each query sees 6-7 callbacks worth of updates. The monotonically increasing nature of the counter ensures no regressions.

### Seek (US-AUD-004)

```
SDL3AudioSystem::seek(double position_ms):

  1. If no music loaded, return
  2. Clamp position_ms to [0, duration_ms]

  3. Lock music_mutex_

  4. // Convert ms to sample index
  5. int64_t target_sample = static_cast<int64_t>(
         (position_ms / 1000.0) * source_format_.sample_rate);
  6. Clamp target_sample to [0, total_samples]

  7. decoder_->seek_to_sample(target_sample)
  8. seek_base_ = target_sample
  9. samples_fed_ = 0

  10. // Flush buffered data in the SDL stream so stale audio doesn't play
  11. SDL_FlushAudioStream(stream_)

  12. Unlock music_mutex_

  13. // If we were paused, update the frozen position
  14. if (state_ == PAUSED) {
        paused_position_ms_ = position_ms;
      }
```

**Why flush the stream**: Without flushing, the stream contains audio from the pre-seek position. That audio would play for one buffer duration before the new audio arrives, causing an audible artifact. Flushing clears the stream, and the next callback decodes from the new position.

**Why position tracking stays accurate after seek**: By atomically setting `seek_base_ = target_sample` and `samples_fed_ = 0` under the mutex (which the audio callback respects via `try_lock`), the position formula `seek_base_ + samples_fed_ - queued` immediately reflects the new position. The audio callback cannot increment `samples_fed_` during this window because it cannot acquire the mutex.

### Pause / Resume (US-AUD-003)

```
pause():
  1. If state_ != PLAYING, return
  2. paused_position_ms_ = get_position_ms()  // snapshot current position
  3. SDL_PauseAudioStreamDevice(stream_)       // stop hardware consumption
  4. state_ = PAUSED

resume():
  1. If state_ != PAUSED, return
  2. SDL_ResumeAudioStreamDevice(stream_)      // resume hardware consumption
  3. state_ = PLAYING
```

Pause snapshots the position BEFORE stopping hardware. This ensures `get_position_ms()` returns a stable value while paused (it checks `state_ == PAUSED` and returns the snapshot). On resume, the hardware picks up where it left off -- the buffered data in the stream is still valid.

### Judge Integration (US-AUD-021)

```
GameplayScene::update(double dt):
  1. double song_position_ms = engine_.get_audio()->get_position_ms();
  2. judge_.update(song_position_ms, input_snapshot);
     // Judge uses song_position_ms as the sole time source.
     // Judge has NO internal time accumulator.
     // When audio is paused, position is frozen → judge is frozen.
     // When audio seeks, position jumps → judge jumps.
```

This is a one-line integration. The judge accepts a `double` position in milliseconds. It does not know or care whether the position came from an audio system, a mock, or a replay file.

## Dependencies

### Internal
- **Engine** (`src/openitup/core/engine.h`) -- Owns AudioSystem via `unique_ptr`. Calls `init()` during startup, `shutdown()` during teardown.
- **spdlog** -- All logging. Already linked via `openitup_engine` target.
- **SDL3** -- `SDL3AudioSystem` uses `SDL_AudioStream`, `SDL_OpenAudioDevice`, etc. Already linked.

### External (new libraries)

#### stb_vorbis (vendored single-header)
- **Purpose**: OGG Vorbis decoding.
- **Integration**: Vendored into `third_party/stb_vorbis.h`. Implementation compiled in `vorbis_decoder.cpp` via `#define STB_VORBIS_IMPLEMENTATION`.
- **License**: Public domain / MIT.
- **Why not SDL3_mixer**: SDL3_mixer is a higher-level library that manages its own audio device and mixing. We need lower-level control for sample-accurate position tracking. Using SDL3_mixer would mean fighting its abstractions rather than building on them.
- **Size**: ~5500 lines, header-only.

#### dr_mp3 (vendored single-header)
- **Purpose**: MP3 decoding.
- **Integration**: Vendored into `third_party/dr_mp3.h`. Implementation compiled in `mp3_decoder.cpp` via `#define DR_MP3_IMPLEMENTATION`.
- **License**: Public domain / MIT.
- **Why not minimp3**: dr_mp3 wraps minimp3 internally and provides a higher-level file I/O and seeking API. Using dr_mp3 directly saves us from writing the file-level seeking logic ourselves (especially VBR seek table handling).
- **Size**: ~4600 lines, header-only.

Both libraries are widely used in game engines (Raylib, miniaudio, Godot) and have proven track records for correctness and performance.

## Architectural Decisions

### ADR-1: Push-Based SDL_AudioStream Over Callback Mixing

- **Context**: SDL3 offers two audio models: (a) the classic callback model where SDL calls your function to fill a buffer, and (b) `SDL_AudioStream` which lets you push data into a stream that SDL consumes. The callback model requires you to produce audio synchronously within a deadline. The push model decouples production from consumption.
- **Decision**: Use `SDL_AudioStream` with a stream callback (`SDL_SetAudioStreamGetCallback`). The callback fires when SDL needs more data, and we push decoded audio into the stream. This gives us the timing signal of the callback model (we know exactly when hardware needs data) with the flexibility of the push model (we push variable amounts).
- **Alternatives considered**: (a) Pure push with a timer -- no callback, just push data periodically from the game loop. Risk: if the game loop stalls, audio underruns. (b) Classic SDL2-style callback -- works but SDL3's `SDL_AudioStream` handles format conversion (sample rate, channel count) automatically, which we need for files that don't match the output device format.
- **Consequences**: The audio path is: `decoder → decode_buffer_ → SDL_PutAudioStreamData → SDL resamples if needed → hardware`. SDL handles sample rate conversion transparently. We track samples in the source format (pre-resampling) and SDL_GetAudioStreamQueued reports bytes in the output format, so we need to account for the resampling ratio in position calculation. **Update**: SDL_GetAudioStreamQueued returns queued bytes in the output format. We must convert back to source frames for accurate position tracking. The position formula becomes: `consumed_source_frames = samples_fed_ - (queued_output_bytes / output_frame_size) * (source_rate / output_rate)`.

### ADR-2: Vendored Single-Header Libraries Over SDL3_mixer

- **Context**: We need OGG and MP3 decoding. Options: (a) SDL3_mixer (high-level mixing library), (b) vendored single-header decoders (stb_vorbis + dr_mp3), (c) system libraries (libvorbis + libmpg123).
- **Decision**: Vendor stb_vorbis and dr_mp3 as single-header files in `third_party/`.
- **Alternatives considered**: (a) SDL3_mixer -- it owns the audio device and mixing pipeline, conflicting with our need for sample-level control. We'd have to work around its abstractions for position tracking. (b) System libraries -- adds external dependencies, complicating cross-platform builds. The whole project uses FetchContent/vendored deps for reproducibility.
- **Consequences**: We control the entire decode → output pipeline. No dependency conflicts. The decode code is simple (~50 lines per decoder). The trade-off is we handle format conversion ourselves, but SDL_AudioStream does this for us.

### ADR-3: Atomic Counters Over Lock-Free Ring Buffer for Position Tracking

- **Context**: The audio callback (audio thread) must communicate the playback position to `get_position_ms()` (main thread). Options: (a) atomic counter, (b) lock-free ring buffer of position events, (c) mutex-guarded shared state.
- **Decision**: `std::atomic<int64_t> samples_fed_` updated via `fetch_add` on the audio thread. Main thread reads with relaxed ordering.
- **Alternatives considered**: (a) Lock-free ring buffer -- over-engineered for a single monotonically increasing counter. Adds complexity with no benefit. (b) Mutex -- audio callbacks must never block on a mutex. Even `try_lock` adds overhead per callback. (c) `SDL_GetAudioStreamQueued` alone -- not sufficient because it doesn't tell us the total fed count, only the current queue depth.
- **Consequences**: Lock-free, wait-free on both threads. One atomic increment per callback (~375 Hz at 48 kHz with 128-frame callbacks). One atomic load per `get_position_ms()` call (~60 Hz). Zero contention.

### ADR-4: try_lock in Audio Callback for Seek/Load Synchronization

- **Context**: When the main thread seeks or loads new music, it must reset the decoder and sample counter. The audio callback must not decode from a half-reset decoder.
- **Decision**: The audio callback uses `music_mutex_.try_lock()`. If it fails (main thread is seeking/loading), the callback outputs silence for that iteration. The main thread uses `music_mutex_.lock()` normally.
- **Alternatives considered**: (a) Double buffering with atomic swap -- decoder swap is complex and the decoder itself is not trivially swappable. (b) Atomic flag that pauses the callback -- similar to try_lock but less standard. (c) Stop the audio device during seek -- causes an audible click/pop on some hardware.
- **Consequences**: During a seek operation (which takes ~1ms for OGG, ~5ms for VBR MP3), the audio output is silence for 1-2 callback periods (~5-10ms). This is imperceptible. The trade-off is that the audio callback is never blocked.

### ADR-5: SFX Stubs in Phase 1 Interface

- **Context**: US-AUD-091 specifies the interface must define SFX methods. US-AUD-092 only implements music. Phase 3 adds SFX.
- **Decision**: Declare SFX methods in the interface now. `SDL3AudioSystem` implements them as no-ops that log a single warning on first call.
- **Alternatives considered**: (a) Separate `SfxSystem` interface -- would require Phase 3 to add a new subsystem to Engine, changing the ownership model. (b) Omit SFX from interface until Phase 3 -- would require an interface-breaking change.
- **Consequences**: The interface is stable from Phase 1 onward. Phase 3 only changes `SDL3AudioSystem` internals.

### ADR-6: No update() Method on AudioSystem

- **Context**: Other subsystems (InputSystem) have an `update()` or `poll()` called each tick. Should AudioSystem?
- **Decision**: No. The audio callback runs on SDL's audio thread independently of the game loop. The game thread only reads state via `get_position_ms()`. There is no per-tick work to do.
- **Alternatives considered**: (a) `update()` that pre-caches position for this frame -- adds one level of staleness with no benefit, since `get_position_ms()` is already O(1). (b) `update()` that feeds the stream from the main thread -- would couple audio to frame rate, causing underruns during frame drops.
- **Consequences**: Audio never stutters due to game loop performance. The audio thread is completely independent. This is the standard architecture for professional audio engines (FMOD, Wwise, SoLoud).

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| SDL_GetAudioStreamQueued returns bytes in output format, not source format -- position calculation drift if source and output sample rates differ | High | High | Account for resampling ratio: `queued_source_frames = (queued_output_bytes / output_bytes_per_frame) * (source_rate / output_rate)`. Test with 48 kHz source on 44.1 kHz output device. |
| stb_vorbis seek is slow for large VBR files (linear scan) | Med | Low | stb_vorbis uses a binary search on Ogg pages for seeking. For files under 10 minutes (typical song length), seek latency is under 50ms. If profiling shows issues, switch to libvorbisfile which maintains a seek table. |
| dr_mp3 VBR seek accuracy -- XING/VBRI header parsing edge cases | Med | Med | dr_mp3 reads XING headers for VBR seek tables. Test with a known VBR MP3 and verify position accuracy after seek. If accuracy is insufficient, build a frame index on load (one-time scan). |
| Audio callback `try_lock` failure during normal playback (not just seek) due to mutex contention | Med | Low | The mutex is only held during load/seek/stop, each lasting < 10ms. Normal playback never takes the mutex on the main thread. If contention appears in profiling, replace mutex with a lock-free state swap (ADR-4 fallback). |
| SDL_Init(SDL_INIT_AUDIO) fails on headless CI servers | Low | High | Integration tests that need audio output are guarded by a runtime check (`SDL_GetNumAudioDevices() > 0`). Unit tests of decoder and position logic do not need SDL audio at all. |
| Position monotonicity violation during seek (brief negative delta) | Low | Med | Clamp `consumed_samples` to non-negative in `get_position_ms()`. Document that after seek, position may lag the seek target by up to one buffer period (~5ms) before converging. Judge should tolerate this. |
| SDL3 AudioStream API is still evolving (SDL3 is relatively new) | Med | Med | Pin SDL3 to a specific commit in CMakeLists.txt FetchContent rather than tracking `main`. Currently pinned to `main` -- recommend pinning to a release tag when one is available. |

## Testing Strategy

### Unit Tests (`test/test_audio_decoder.cpp`) -- Pure Logic, No SDL Audio

Tests for VorbisDecoder and Mp3Decoder. These tests use actual audio files committed as test fixtures but do not require SDL audio initialization -- they only test the decode-to-float-buffer path.

| Test | Stories | What It Verifies |
|------|---------|-----------------|
| `VorbisOpensStereo44100` | US-AUD-001 | Open a stereo 44.1 kHz OGG, verify format fields |
| `VorbisOpensMono48000` | US-AUD-001 | Open a mono 48 kHz OGG, verify format fields |
| `VorbisDecodesFullFile` | US-AUD-001 | Decode entire file, verify sample count matches expected |
| `VorbisSeekToMiddle` | US-AUD-004 | Seek to 50% of file, decode, verify position |
| `VorbisRejectsInvalidFile` | US-AUD-001 | Pass a non-OGG file, verify open() returns false |
| `VorbisRejectsNonexistentFile` | US-AUD-001 | Pass a path that doesn't exist, verify open() returns false |
| `Mp3OpensCBR` | US-AUD-002 | Open a CBR MP3, verify format fields |
| `Mp3OpensVBR` | US-AUD-002 | Open a VBR MP3, verify format fields |
| `Mp3DecodesFullFile` | US-AUD-002 | Decode entire file, verify sample count within 1% of expected |
| `Mp3SeekToMiddle` | US-AUD-004 | Seek to 50%, decode, verify position |
| `Mp3RejectsInvalidFile` | US-AUD-002 | Pass a non-MP3 file, verify open() returns false |
| `DecoderOutputIsNormalizedFloat` | US-AUD-001/002 | All decoded samples are in [-1.0, 1.0] range |

### Unit Tests (`test/test_audio_system.cpp`) -- Position Logic, No Hardware

Position tracking logic can be tested without hardware by testing the arithmetic in isolation:

| Test | Stories | What It Verifies |
|------|---------|-----------------|
| `PositionFormulaBasic` | US-AUD-011 | Given known `samples_fed`, `seek_base`, `queued`, verify ms calculation |
| `PositionFormulaAfterSeek` | US-AUD-004/011 | After seek: `seek_base_` = target, `samples_fed_` = 0, position = target_ms |
| `PositionClampsToZero` | US-AUD-011 | When queued > fed (transient after seek), position is 0, not negative |
| `PositionWithResamplingRatio` | US-AUD-011 | Source 48 kHz, output 44.1 kHz, verify ratio correction |
| `MockAudioSystemReturnsControlledPosition` | US-AUD-021 | MockAudioSystem with settable position, judge receives it |

### Integration Tests (`test/test_audio_system.cpp`) -- SDL Audio Required

These tests require a working SDL audio device. They are guarded by a runtime availability check and skipped in headless CI.

| Test | Stories | What It Verifies |
|------|---------|-----------------|
| `InitSucceedsWithAudioDevice` | US-AUD-081 | SDL3AudioSystem::init() returns true on systems with audio |
| `InitFailsGracefullyWithoutDevice` | US-AUD-081 | Simulated: verify error logged, engine continues |
| `LoadAndPlayOGG` | US-AUD-001/003 | Load OGG, play(), verify state == PLAYING |
| `LoadAndPlayMP3` | US-AUD-002/003 | Load MP3, play(), verify state == PLAYING |
| `PauseFreezesPosition` | US-AUD-003/011 | Play, wait 500ms, pause, verify position stable over 200ms |
| `ResumeFromPause` | US-AUD-003 | Pause then resume, verify position progresses |
| `StopResetsPosition` | US-AUD-003 | Play, stop, verify position == 0 |
| `SeekForward` | US-AUD-004 | Play, seek to 5000ms, verify position near 5000ms |
| `SeekWhilePaused` | US-AUD-004 | Pause, seek, verify paused position updates |
| `SeekBeyondEnd` | US-AUD-004 | Seek past duration, verify clamp to duration |
| `SeekNegative` | US-AUD-004 | Seek to -1000, verify clamp to 0 |
| `PositionMonotonicallyIncreases` | US-AUD-011 | Query position 60 times over 1 second, verify monotonic |
| `PositionAccuracyWithin2ms` | US-AUD-011 | Play for 1 second, compare position to wall-clock, verify < 2ms error |
| `VolumeAffectsOutput` | US-AUD-003 | Set volume to 0, verify (via position) that playback proceeds (volume doesn't affect timing) |

### MockAudioSystem for Judge Testing (US-AUD-021)

A test-only implementation for judge unit tests that need a controllable audio position without any SDL dependency:

```cpp
// test/mock_audio_system.h (test-only, not in src/)
class MockAudioSystem : public AudioSystem {
public:
    // Test control: set what get_position_ms() returns.
    void set_position(double ms) { position_ms_ = ms; }
    void set_state(AudioState s) { state_ = s; }

    // AudioSystem interface - minimal stubs
    bool init() override { return true; }
    void shutdown() override {}
    bool load_music(const std::filesystem::path&) override { return true; }
    void play() override { state_ = AudioState::PLAYING; }
    void pause() override { state_ = AudioState::PAUSED; }
    void resume() override { state_ = AudioState::PLAYING; }
    void stop() override { state_ = AudioState::STOPPED; position_ms_ = 0; }
    void seek(double ms) override { position_ms_ = ms; }
    double get_position_ms() const override { return position_ms_; }
    double get_duration_ms() const override { return duration_ms_; }
    AudioState get_state() const override { return state_; }
    bool is_music_loaded() const override { return true; }
    void set_music_volume(float) override {}
    float get_music_volume() const override { return 1.0f; }
    uint32_t load_sfx(const std::filesystem::path&) override { return 0; }
    void play_sfx(uint32_t) override {}
    void set_sfx_volume(float) override {}
    float get_sfx_volume() const override { return 1.0f; }

    double duration_ms_ = 180000.0;  // 3 minutes default
private:
    double position_ms_ = 0.0;
    AudioState state_ = AudioState::STOPPED;
};
```

This mock is the primary tool for judge testing in future Phase 1 stories. The judge never needs real audio -- it only needs a controllable position source.

### Test Fixture Generation

Test audio fixtures (1-second sine waves) will be generated by a small Python script or committed as binary files. The files are small (~50 KB each for compressed audio) and are committed to `test/fixtures/`. This matches the existing pattern of committed BGAJ/SPRJ/PNG fixtures.

A `generate_test_audio` CMake target (similar to the existing `generate_samples` target) can generate these programmatically using the vendored decoders in reverse (encode a known waveform). However, for Phase 1, pre-generated committed files are simpler and more reliable.

## SDL3 Audio API Usage Notes

The SDL3 audio API has evolved from SDL2. Key differences relevant to this design:

1. **`SDL_OpenAudioDevice` returns `SDL_AudioDeviceID`** -- no more "default device = 0" magic. Use `SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK`.
2. **`SDL_AudioStream`** is the primary audio API. Create a stream, bind it to a device, push data into it. SDL handles buffer management and resampling.
3. **`SDL_SetAudioStreamGetCallback`** replaces the SDL2 audio callback. It fires on the audio thread when the stream needs more data.
4. **`SDL_GetAudioStreamQueued`** returns the number of bytes currently queued in the stream (output format). This is the key function for position tracking.
5. **`SDL_FlushAudioStream`** clears all queued data -- essential for seek.
6. **`SDL_PauseAudioStreamDevice` / `SDL_ResumeAudioStreamDevice`** -- pause/resume the device bound to a stream.
7. **Audio format**: Request `SDL_AUDIO_F32` (32-bit float) at the source sample rate. SDL_AudioStream handles conversion to the device's native format.

### Initialization Sequence

```
1. SDL_Init(SDL_INIT_AUDIO)                    // Done by Engine
2. SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr)
                                                // Returns device_id_
3. SDL_AudioSpec spec = { SDL_AUDIO_F32, 2, 44100 };
   stream_ = SDL_CreateAudioStream(&spec, nullptr);
                                                // Source format → device format
4. SDL_SetAudioStreamGetCallback(stream_, audio_callback, this);
5. SDL_BindAudioStream(device_id_, stream_);
                                                // Bind stream to device
6. Device starts paused. SDL_ResumeAudioStreamDevice when play() is called.
```

### Shutdown Sequence

```
1. SDL_UnbindAudioStream(device_id_, stream_);
2. SDL_DestroyAudioStream(stream_);
3. SDL_CloseAudioDevice(device_id_);
4. // SDL_QuitSubSystem(SDL_INIT_AUDIO) done by Engine
```

## Position Tracking: Resampling Correction

When the source sample rate differs from the output device sample rate, `SDL_GetAudioStreamQueued` returns bytes in the **output** format. We need to convert back to source frames for the position formula.

```
// During init, after binding stream to device:
SDL_AudioSpec output_spec;
SDL_GetAudioDeviceFormat(device_id_, &output_spec, nullptr);
output_sample_rate_ = output_spec.freq;
output_channels_ = output_spec.channels;

// In get_position_ms():
int queued_output_bytes = SDL_GetAudioStreamQueued(stream_);
int queued_output_frames = queued_output_bytes
    / (output_channels_ * sizeof(float));
// Convert output frames to source frames:
int64_t queued_source_frames = static_cast<int64_t>(
    static_cast<double>(queued_output_frames)
    * static_cast<double>(source_format_.sample_rate)
    / static_cast<double>(output_sample_rate_));

int64_t consumed = seek_base_ + samples_fed_ - queued_source_frames;
```

This correction is essential. Without it, position would drift by the ratio `(source_rate - output_rate) / output_rate`. For a 48 kHz source on a 44.1 kHz output device, that's ~8.8% drift -- catastrophic for a rhythm game.

---

*Generated from stories in docs/stories/03-audio-system.md (Phase 1 subset)*
*Last updated: 2026-04-28*
