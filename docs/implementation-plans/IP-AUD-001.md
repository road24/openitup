# IP-AUD-001: Audio System Implementation Plan

**Design**: TD-AUD-001
**Stories**: US-AUD-091, US-AUD-081, US-AUD-092, US-AUD-001, US-AUD-002, US-AUD-003, US-AUD-004, US-AUD-011, US-AUD-021
**Total Steps**: 9
**Estimated Total**: ~4.5 hours
**Author**: technical-lead agent
**Status**: Draft

## Prerequisite

IP-ENG-001 must be complete. The Engine class must exist with the ownership pattern established (`std::unique_ptr` members, dependency injection support). The audio system will follow this exact pattern.

---

## Step 1: Vendor Audio Decoder Libraries

**Files**:
- Create `third_party/stb_vorbis.h` — stb_vorbis v1.22 single-header library (public domain)
- Create `third_party/dr_mp3.h` — dr_mp3 v0.6.39 single-header library (public domain)
- Modify `CMakeLists.txt` — Add `include_directories(third_party)` to make headers available

**What to implement**:

Download and vendor two battle-tested single-header audio decoders:

1. **stb_vorbis**: OGG Vorbis decoder
   - Source: https://github.com/nothings/stb/blob/master/stb_vorbis.c
   - License: Public domain / MIT
   - Size: ~5500 lines
   - Copy as-is, no modifications

2. **dr_mp3**: MP3 decoder
   - Source: https://github.com/mackron/dr_libs/blob/master/dr_mp3.h
   - License: Public domain / MIT
   - Size: ~4600 lines
   - Copy as-is, no modifications

Add include directory to CMakeLists.txt after the `openitup_engine` target:
```cmake
target_include_directories(openitup_engine PUBLIC 
    src
    third_party
)
```

**Tests**:

Create `test/test_vendor_headers.cpp` to verify headers are includable:
```cpp
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.h>
#undef STB_VORBIS_HEADER_ONLY

#define DR_MP3_HEADER_ONLY
#include <dr_mp3.h>
#undef DR_MP3_HEADER_ONLY

#include <gtest/gtest.h>

TEST(VendorHeaders, StbVorbisHeaderCompiles) {
    SUCCEED() << "stb_vorbis.h included without errors";
}

TEST(VendorHeaders, DrMp3HeaderCompiles) {
    SUCCEED() << "dr_mp3.h included without errors";
}
```

Add to `CMakeLists.txt`:
```cmake
target_sources(openitup_tests PRIVATE
    test/test_vendor_headers.cpp
)
```

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R VendorHeaders` passes
- [ ] Headers are includable without errors
- [ ] No implementation compiled yet (header-only mode test)

**Expected commit message**:
`feat(audio): vendor stb_vorbis and dr_mp3 single-header libraries`

**Estimated time**: ~20 minutes

---

## Step 2: Create AudioSystem Interface and AudioState Enum

**Files**:
- Create `src/openitup/audio/audio_system.h` — Pure virtual interface + AudioState enum
- Modify `CMakeLists.txt` — Create `openitup_audio` library target (header-only for now)

**What to implement**:

Implement the complete `AudioSystem` interface from TD-AUD-001 sections "New Types" → "AudioSystem". This is the public API that all consumers see.

Key interface design points:
- Pure virtual (no implementation)
- No SDL3 types in the header
- `double` for position/duration (millisecond precision)
- `float` for volume (0.0-1.0 range)
- Enum for AudioState: STOPPED, PLAYING, PAUSED
- SFX methods declared but will be no-ops in Phase 1
- No `update()` method — audio runs on its own thread

Add to `CMakeLists.txt`:
```cmake
add_library(openitup_audio INTERFACE)
target_include_directories(openitup_audio INTERFACE src)
target_link_libraries(openitup_audio INTERFACE openitup_engine)
```

**Tests**:

Create `test/test_audio_system.cpp`:
```cpp
#include <gtest/gtest.h>
#include <openitup/audio/audio_system.h>

using namespace openitup;

// Verify enum values exist
TEST(AudioSystem, AudioStateEnumValues) {
    AudioState stopped = AudioState::STOPPED;
    AudioState playing = AudioState::PLAYING;
    AudioState paused = AudioState::PAUSED;
    
    EXPECT_NE(stopped, playing);
    EXPECT_NE(playing, paused);
    EXPECT_NE(paused, stopped);
}

// Verify interface is abstract (compile-time check)
TEST(AudioSystem, InterfaceIsAbstract) {
    EXPECT_TRUE(std::is_abstract_v<AudioSystem>);
}
```

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R AudioSystem` passes
- [ ] Interface compiles without SDL3 headers
- [ ] All methods are pure virtual

**Expected commit message**:
`feat(audio): add AudioSystem interface and AudioState enum`

**Estimated time**: ~25 minutes

---

## Step 3: Create AudioDecoder Interface and Format Struct

**Files**:
- Create `src/openitup/audio/audio_decoder.h` — Internal decoder interface + AudioFormat struct
- Modify `test/test_audio_system.cpp` — Add interface verification tests

**What to implement**:

Implement the `AudioDecoder` interface from TD-AUD-001 sections "New Types" → "AudioDecoder". This is an internal interface (not public API) for format-agnostic decoding.

Key design points:
- `AudioFormat` struct: sample_rate, channels, total_samples
- Decoder outputs interleaved float samples [-1.0, 1.0]
- Frame-based API (one frame = one sample per channel)
- Seek by sample index (per-channel count)
- Pure virtual interface

**Tests**:

Add to `test/test_audio_system.cpp`:
```cpp
#include <openitup/audio/audio_decoder.h>

TEST(AudioDecoder, InterfaceIsAbstract) {
    EXPECT_TRUE(std::is_abstract_v<AudioDecoder>);
}

TEST(AudioDecoder, AudioFormatFields) {
    AudioFormat fmt;
    fmt.sample_rate = 44100;
    fmt.channels = 2;
    fmt.total_samples = 132300;  // 3 seconds stereo
    
    EXPECT_EQ(fmt.sample_rate, 44100);
    EXPECT_EQ(fmt.channels, 2);
    EXPECT_EQ(fmt.total_samples, 132300);
}
```

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R AudioDecoder` passes
- [ ] Interface is pure virtual
- [ ] AudioFormat struct compiles

**Expected commit message**:
`feat(audio): add AudioDecoder interface and AudioFormat struct`

**Estimated time**: ~20 minutes

---

## Step 4: Implement VorbisDecoder

**Files**:
- Create `src/openitup/audio/vorbis_decoder.h` — VorbisDecoder class declaration
- Create `src/openitup/audio/vorbis_decoder.cpp` — Implementation using stb_vorbis
- Modify `CMakeLists.txt` — Add vorbis_decoder.cpp to openitup_audio library (convert to static library)

**What to implement**:

Implement VorbisDecoder following TD-AUD-001 section "VorbisDecoder". Use stb_vorbis API to open, decode, and seek OGG files.

Key implementation details:
- `#define STB_VORBIS_IMPLEMENTATION` at top of .cpp before include
- Forward-declare `stb_vorbis` in header to avoid exposing stb types
- `open()`: Call `stb_vorbis_open_filename()`, extract format info
- `decode()`: Call `stb_vorbis_get_samples_float_interleaved()`, return frames decoded
- `seek_to_sample()`: Call `stb_vorbis_seek()`, return success
- `close()`: Call `stb_vorbis_close()`
- Handle errors: return false on failure, log with spdlog

stb_vorbis API reference:
```cpp
stb_vorbis* stb_vorbis_open_filename(const char* filename, int* error, const stb_vorbis_alloc* alloc);
void stb_vorbis_close(stb_vorbis* v);
stb_vorbis_info stb_vorbis_get_info(stb_vorbis* v);
int stb_vorbis_get_samples_float_interleaved(stb_vorbis* v, int channels, float* buffer, int num_floats);
int stb_vorbis_seek(stb_vorbis* v, unsigned int sample_number);
```

Update `CMakeLists.txt`:
```cmake
add_library(openitup_audio STATIC
    src/openitup/audio/vorbis_decoder.cpp
)
target_include_directories(openitup_audio PUBLIC src third_party)
target_link_libraries(openitup_audio PUBLIC openitup_engine spdlog::spdlog)
```

**Tests**:

Will be tested in Step 6 (decoder integration tests). This step focuses on implementation correctness via compilation.

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] VorbisDecoder compiles and links
- [ ] No stb_vorbis types leak into header

**Expected commit message**:
`feat(audio): implement VorbisDecoder using stb_vorbis`

**Estimated time**: ~35 minutes

---

## Step 5: Implement Mp3Decoder

**Files**:
- Create `src/openitup/audio/mp3_decoder.h` — Mp3Decoder class declaration
- Create `src/openitup/audio/mp3_decoder.cpp` — Implementation using dr_mp3
- Modify `CMakeLists.txt` — Add mp3_decoder.cpp to openitup_audio library

**What to implement**:

Implement Mp3Decoder following TD-AUD-001 section "Mp3Decoder". Use dr_mp3 API to open, decode, and seek MP3 files.

Key implementation details:
- `#define DR_MP3_IMPLEMENTATION` at top of .cpp before include
- Heap-allocate `drmp3` struct (it's ~45 KB) to avoid header bloat
- Forward-declare `drmp3` in header
- `open()`: Allocate `drmp3`, call `drmp3_init_file()`, extract format info
- `decode()`: Call `drmp3_read_pcm_frames_f32()`, return frames decoded
- `seek_to_sample()`: Call `drmp3_seek_to_pcm_frame()`, return success
- `close()`: Call `drmp3_uninit()`, delete struct
- Handle errors: return false on failure, log with spdlog

dr_mp3 API reference:
```cpp
drmp3_bool32 drmp3_init_file(drmp3* pMp3, const char* pFilePath, const drmp3_config* pConfig);
void drmp3_uninit(drmp3* pMp3);
drmp3_uint64 drmp3_read_pcm_frames_f32(drmp3* pMp3, drmp3_uint64 framesToRead, float* pBufferOut);
drmp3_bool32 drmp3_seek_to_pcm_frame(drmp3* pMp3, drmp3_uint64 frameIndex);
drmp3_uint64 drmp3_get_pcm_frame_count(drmp3* pMp3);
```

Add to `CMakeLists.txt`:
```cmake
target_sources(openitup_audio PRIVATE
    src/openitup/audio/vorbis_decoder.cpp
    src/openitup/audio/mp3_decoder.cpp
)
```

**Tests**:

Will be tested in Step 6 (decoder integration tests). This step focuses on implementation correctness via compilation.

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] Mp3Decoder compiles and links
- [ ] No dr_mp3 types leak into header
- [ ] `drmp3` struct is heap-allocated

**Expected commit message**:
`feat(audio): implement Mp3Decoder using dr_mp3`

**Estimated time**: ~35 minutes

---

## Step 6: Add Decoder Unit Tests with Test Fixtures

**Files**:
- Create `test/test_audio_decoder.cpp` — Unit tests for VorbisDecoder and Mp3Decoder
- Create `test/fixtures/test_tone_44100.ogg` — 1-second 440 Hz sine wave, stereo, 44.1 kHz
- Create `test/fixtures/test_tone_48000.ogg` — 1-second 440 Hz sine wave, mono, 48 kHz
- Create `test/fixtures/test_tone_44100.mp3` — 1-second 440 Hz sine wave, stereo, 44.1 kHz
- Modify `CMakeLists.txt` — Add test_audio_decoder.cpp to openitup_tests

**What to implement**:

Test fixture generation: Use `ffmpeg` to generate test audio files (run these commands manually, commit the generated files):

```bash
cd test/fixtures

# 440 Hz sine, 1 second, stereo, 44.1 kHz OGG
ffmpeg -f lavfi -i "sine=frequency=440:duration=1:sample_rate=44100" \
  -ac 2 -c:a libvorbis -q:a 5 test_tone_44100.ogg

# 440 Hz sine, 1 second, mono, 48 kHz OGG
ffmpeg -f lavfi -i "sine=frequency=440:duration=1:sample_rate=48000" \
  -ac 1 -c:a libvorbis -q:a 5 test_tone_48000.ogg

# 440 Hz sine, 1 second, stereo, 44.1 kHz MP3
ffmpeg -f lavfi -i "sine=frequency=440:duration=1:sample_rate=44100" \
  -ac 2 -c:a libmp3lame -b:a 192k test_tone_44100.mp3
```

Test cases from TD-AUD-001 "Testing Strategy" section:
- `VorbisOpensStereo44100` — Verify format: 44100 Hz, 2 channels, ~44100 samples
- `VorbisOpensMono48000` — Verify format: 48000 Hz, 1 channel, ~48000 samples
- `VorbisDecodesFullFile` — Decode entire file, verify sample count within 1%
- `VorbisSeekToMiddle` — Seek to 50%, decode 100 samples, verify position changed
- `VorbisRejectsInvalidFile` — Pass `.cpp` file as OGG, verify `open()` returns false
- `VorbisRejectsNonexistentFile` — Pass nonexistent path, verify `open()` returns false
- `Mp3OpensCBR` — Verify format fields for test_tone_44100.mp3
- `Mp3DecodesFullFile` — Decode entire file, verify sample count within 1%
- `Mp3SeekToMiddle` — Seek to 50%, decode 100 samples, verify position changed
- `Mp3RejectsInvalidFile` — Pass `.cpp` file as MP3, verify `open()` returns false
- `DecoderOutputIsNormalizedFloat` — Decode 1000 samples, verify all in [-1.0, 1.0]

**Tests**:

Create `test/test_audio_decoder.cpp` with all test cases listed above. Use the pattern from `test_texture_cache.cpp` for fixture setup.

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R AudioDecoder` passes all tests
- [ ] Test fixtures are committed to `test/fixtures/`
- [ ] Both VorbisDecoder and Mp3Decoder open, decode, and seek correctly

**Expected commit message**:
`test(audio): add decoder tests with OGG and MP3 fixtures`

**Estimated time**: ~40 minutes

---

## Step 7: Implement SDL3AudioSystem Core

**Files**:
- Create `src/openitup/audio/sdl3_audio_system.h` — SDL3AudioSystem class declaration
- Create `src/openitup/audio/sdl3_audio_system.cpp` — Core implementation (init, shutdown, load, create_decoder)
- Modify `CMakeLists.txt` — Add sdl3_audio_system.cpp to openitup_audio library

**What to implement**:

Implement SDL3AudioSystem following TD-AUD-001 sections "SDL3AudioSystem" and "SDL3 Audio API Usage Notes". This step implements initialization, decoder factory, and music loading. **Transport and position tracking come in Step 8.**

Implement these methods:
- **Constructor/destructor**: Initialize atomics, destroy stream/device
- **init()**: Call SDL3 audio initialization sequence from TD-AUD-001 "Initialization Sequence"
  - `SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr)`
  - Create `SDL_AudioStream` with `SDL_AUDIO_F32` format
  - `SDL_SetAudioStreamGetCallback(stream_, audio_callback, this)`
  - `SDL_BindAudioStream(device_id_, stream_)`
- **shutdown()**: Unbind stream, destroy stream, close device
- **create_decoder()**: Factory method that checks file extension and returns `VorbisDecoder` or `Mp3Decoder`
- **load_music()**: Lock mutex, stop if playing, create decoder, open file, resize decode buffer, reset position state
- **Stub implementations**: All transport methods (`play()`, `pause()`, `resume()`, `stop()`, `seek()`) and accessors just log "not implemented" for now

SDL3AudioSystem member variables (from TD-AUD-001):
- `SDL_AudioStream* stream_`
- `SDL_AudioDeviceID device_id_`
- `std::unique_ptr<AudioDecoder> decoder_`
- `AudioFormat source_format_`
- `std::atomic<AudioState> state_`
- `std::atomic<int64_t> samples_fed_`, `seek_base_`
- `double paused_position_ms_`
- `std::vector<float> decode_buffer_`
- `std::atomic<float> music_volume_`, `sfx_volume_`
- `std::mutex music_mutex_`
- `bool initialized_`

Link SDL3 in CMakeLists.txt:
```cmake
target_link_libraries(openitup_audio PUBLIC 
    openitup_engine 
    spdlog::spdlog
    SDL3::SDL3-static
)
```

**Tests**:

Will be tested in Step 8 with full transport implementation. This step ensures compilation and linking.

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] SDL3AudioSystem compiles and links against SDL3
- [ ] `init()` and `load_music()` are implemented
- [ ] Transport methods are stubs

**Expected commit message**:
`feat(audio): implement SDL3AudioSystem initialization and music loading`

**Estimated time**: ~35 minutes

---

## Step 8: Implement Transport Control and Sample-Accurate Position Tracking

**Files**:
- Modify `src/openitup/audio/sdl3_audio_system.cpp` — Implement audio callback, transport methods, and position tracking
- Modify `test/test_audio_system.cpp` — Add integration tests for position tracking

**What to implement**:

This is the most critical step. Implement the audio callback and position tracking algorithm from TD-AUD-001 sections "Audio Callback", "Sample-Accurate Position", "Seek", and "Pause/Resume".

Implement these methods:
- **audio_callback()**: Static callback that casts userdata and calls `feed_audio()`
- **feed_audio()**: The core data path from TD-AUD-001 "Audio Callback — The Core Data Path"
  1. Check state (return silence if not PLAYING)
  2. Try to lock mutex (return silence if locked)
  3. Decode requested frames
  4. Apply volume
  5. Put data into SDL stream
  6. Increment `samples_fed_` atomically
  7. Repeat until buffer filled
- **get_position_ms()**: THE critical algorithm from TD-AUD-001 "Sample-Accurate Position"
  - Read `samples_fed_` and `seek_base_` atomically
  - Query `SDL_GetAudioStreamQueued(stream_)`
  - Convert queued bytes to source frames (accounting for resampling ratio)
  - Compute: `consumed_samples = seek_base_ + samples_fed_ - queued_source_frames`
  - Clamp to non-negative
  - Convert to milliseconds
- **play()**: Seek to 0, reset counters, resume device, set state to PLAYING
- **pause()**: Snapshot position, pause device, set state to PAUSED
- **resume()**: Resume device, set state to PLAYING
- **stop()**: Lock mutex, pause device, reset position, set state to STOPPED
- **seek()**: Lock mutex, call decoder seek, set `seek_base_`, reset `samples_fed_`, flush stream
- **get_duration_ms()**: Convert `source_format_.total_samples` to milliseconds
- **Volume accessors**: Load/store atomics

Handle resampling ratio per TD-AUD-001 "Position Tracking: Resampling Correction":
- Query output device format: `SDL_GetAudioDeviceFormat(device_id_, &output_spec, nullptr)`
- Store `output_sample_rate_` and `output_channels_`
- In `get_position_ms()`, convert queued output frames back to source frames

**Tests**:

Add to `test/test_audio_system.cpp` (requires SDL audio device, guarded by `SDL_GetNumAudioDevices() > 0`):

Integration tests from TD-AUD-001 "Testing Strategy":
- `LoadAndPlayOGG` — Load OGG, play(), verify state == PLAYING
- `LoadAndPlayMP3` — Load MP3, play(), verify state == PLAYING
- `PauseFreezesPosition` — Play, wait 500ms, pause, wait 200ms, verify position stable
- `ResumeFromPause` — Pause then resume, verify position progresses
- `StopResetsPosition` — Play, stop, verify position == 0
- `SeekForward` — Play, seek to 500ms, verify position near 500ms within 50ms
- `SeekWhilePaused` — Pause, seek, verify paused position updates
- `PositionMonotonicallyIncreases` — Query position 60 times over 1 second, verify no regressions

Use test fixtures from Step 6 (`test_tone_44100.ogg`, `test_tone_44100.mp3`).

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R AudioSystem` passes
- [ ] Position tracking is sample-accurate (within 2ms)
- [ ] Transport controls work correctly
- [ ] Audio plays without glitches

**Expected commit message**:
`feat(audio): implement transport controls and sample-accurate position tracking`

**Estimated time**: ~50 minutes (this is the hardest step — sample-accurate position is complex)

---

## Step 9: Integrate AudioSystem into Engine

**Files**:
- Modify `src/openitup/core/engine.h` — Add `std::unique_ptr<AudioSystem> audio_` member and accessor
- Modify `src/openitup/core/engine.cpp` — Add audio initialization, shutdown, and SDL_INIT_AUDIO flag
- Modify `src/openitup/gfx/renderer.cpp` — Remove SDL_Init/SDL_Quit calls (lift to Engine per TD-AUD-001 "Modified Types")
- Modify `test/test_engine.cpp` — Add Engine audio accessor test

**What to implement**:

Integrate AudioSystem into Engine following TD-AUD-001 "Modified Types" → "Engine".

Changes to `engine.h`:
```cpp
#include <openitup/audio/audio_system.h>

class Engine {
public:
    // Add optional audio injection to constructor
    Engine(const EngineConfig& config, 
           std::unique_ptr<Clock> clock = nullptr,
           std::unique_ptr<AudioSystem> audio = nullptr);
    
    // Add accessor (nullable — init can fail gracefully)
    AudioSystem* get_audio() { return audio_.get(); }
    const AudioSystem* get_audio() const { return audio_.get(); }

private:
    std::unique_ptr<AudioSystem> audio_;
    
    void init_sdl();    // Centralized SDL init
    void init_audio();  // Audio subsystem init
};
```

Changes to `engine.cpp`:
1. Add `init_sdl()` private method:
   - Call `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)`
   - Called at start of constructor, before renderer init
2. Add `init_audio()` private method:
   - If `audio_` was injected, call `audio_->init()` and return
   - Otherwise, create `SDL3AudioSystem`, call `init()`
   - On failure, log ERROR and set `audio_ = nullptr` (graceful degradation)
   - Called after renderer init
3. Destructor/shutdown:
   - Call `audio_->shutdown()` if `audio_` is non-null
   - Call `SDL_Quit()` after all subsystems shut down

Changes to `renderer.cpp`:
- Remove `SDL_Init(SDL_INIT_VIDEO)` from `Renderer::init()`
- Remove `SDL_Quit()` from `Renderer::shutdown()`
- SDL lifecycle is now owned by Engine

**Tests**:

Add to `test/test_engine.cpp`:
```cpp
TEST(Engine, AudioSystemAccessor) {
    EngineConfig config;
    config.window_title = "audio_test";
    config.window_width = 640;
    config.window_height = 480;
    
    Engine engine(config);
    
    // Audio may be null if no device available
    if (SDL_GetNumAudioDevices(SDL_FALSE) > 0) {
        EXPECT_NE(engine.get_audio(), nullptr);
    }
}
```

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure` passes all tests
- [ ] `./build/openitup` runs without audio errors
- [ ] Engine owns AudioSystem via unique_ptr
- [ ] SDL init/quit is centralized in Engine

**Expected commit message**:
`feat(audio): integrate AudioSystem into Engine with centralized SDL lifecycle`

**Estimated time**: ~30 minutes

---

## PR Strategy

- [ ] **Single PR recommended** — All 9 steps build on each other. Splitting would create intermediate states where decoders exist but aren't used, or SDL3AudioSystem exists but isn't integrated.
- [ ] **Review checkpoint**: After Step 6 (decoder tests pass), pause for review of the decoding layer before proceeding to SDL3AudioSystem.
- [ ] **Second checkpoint**: After Step 8 (position tracking works), verify the audio system is correct before Engine integration.

Alternative: Split into two PRs:
1. Steps 1-6: Decoders and tests (audio decoding without SDL audio output)
2. Steps 7-9: SDL3AudioSystem and Engine integration

## Build Verification

After all steps complete:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DSDL_X11_XSCRNSAVER=OFF
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

All tests should pass. Verify audio playback:

```bash
# Test with bga_player (once audio is integrated)
./build/openitup
# Should open window with no audio errors in logs
```

Check logs for audio initialization:
```
[info] SDL3AudioSystem initialized: 48000 Hz, 2 channels
[info] Loaded music: test_tone_44100.ogg (44100 Hz, 2 ch, 1.00s)
```

## Acceptance Verification

| Story ID | How to verify |
|----------|--------------|
| US-AUD-091 | Interface compiles without SDL headers: `test/test_audio_system.cpp` |
| US-AUD-081 | `SDL3AudioSystem::init()` returns true on systems with audio, false otherwise |
| US-AUD-092 | `test/test_audio_decoder.cpp`: All decoder tests pass |
| US-AUD-001 | `VorbisOpensStereo44100`, `VorbisDecodesFullFile` tests pass |
| US-AUD-002 | `Mp3OpensCBR`, `Mp3DecodesFullFile` tests pass |
| US-AUD-003 | `LoadAndPlayOGG`, `PauseFreezesPosition`, `ResumeFromPause` tests pass |
| US-AUD-004 | `SeekForward`, `SeekWhilePaused`, `SeekBeyondEnd` tests pass |
| US-AUD-011 | `PositionMonotonicallyIncreases`, position accuracy within 2ms |
| US-AUD-021 | Engine exposes `get_audio()`, GameplayScene can call `get_position_ms()` |

## Notes

**Why Step 8 is 50 minutes**: Sample-accurate position tracking is the hardest part of the audio system. The algorithm requires:
1. Atomic counter updates in the audio callback
2. Querying SDL's internal buffer state
3. Accounting for resampling ratio between source and output formats
4. Ensuring monotonic progression across seek boundaries
5. Handling pause state snapshot

This deserves extra time and careful testing.

**Test fixture generation**: The three audio files (`test_tone_44100.ogg`, `test_tone_48000.ogg`, `test_tone_44100.mp3`) are small (30-50 KB each) and should be committed. They provide deterministic test cases with known format parameters.

**Headless CI**: Tests in Steps 6 and 8 require an audio device. Use `if (SDL_GetNumAudioDevices(SDL_FALSE) > 0)` guards or skip with `GTEST_SKIP()` on headless systems.

**SFX stubs**: All SFX methods (`load_sfx()`, `play_sfx()`, `set_sfx_volume()`, `get_sfx_volume()`) are implemented as no-ops that log a single warning on first call. This satisfies US-AUD-091 (interface stability) while deferring SFX implementation to Phase 3.

**Rollback guidance**: If Step 8 (position tracking) proves too complex:
1. Simplify `get_position_ms()` to return `samples_fed_ / sample_rate * 1000` (ignoring queued samples)
2. Document known inaccuracy (~5-10ms lag)
3. File a tech debt story to implement full algorithm later
4. This gives basic functionality while deferring the hardest part

**Engine ownership pattern**: AudioSystem follows the exact pattern established by IP-ENG-001: Engine owns via `unique_ptr`, optional injection for testing, nullable accessor for graceful degradation. This consistency makes the codebase easier to understand.
