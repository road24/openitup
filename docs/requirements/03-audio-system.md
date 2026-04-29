# Audio System Requirements

## REQ-AUD-001: Music Stream Playback
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The audio system must support seekable, pausable music streams for song audio files (OGG, MP3).

**Acceptance Criteria**:
- Load and play OGG Vorbis and MP3 files
- Play, pause, resume, and stop controls work correctly
- Seek to specific position in milliseconds
- Handles variable bitrate files correctly

**Dependencies**: REQ-ENG-002  
**Source**: Roadmap subsystem 3, Phase 1

---

## REQ-AUD-002: Authoritative Audio Position
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The audio system must provide `get_position_ms()` returning current playback position based on samples consumed by audio hardware. This is the authoritative time source for gameplay.

**Acceptance Criteria**:
- Position accuracy within 2ms of actual audio output
- Position updates at least every frame (60+ Hz query rate)
- No drift between position and actual audio over 5+ minute songs
- Position accounts for hardware buffer size

**Dependencies**: REQ-AUD-001  
**Source**: Roadmap subsystem 3, architecture decisions

---

## REQ-AUD-003: Judge Timing from Audio Position
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The gameplay judge must read audio position, not wall-clock time, eliminating drift between what the player hears and what the game judges.

**Note**: The authoritative requirement for judge-reads-audio-position is REQ-JDG-001. This requirement specifies the audio system's obligation to provide accurate position data.

**Acceptance Criteria**:
- Judge queries audio position each tick
- No separate timing drift accumulates
- Pausing audio pauses judge correctly
- Seeking audio updates judge position correctly
- Cumulative timing drift between reported position and actual playback must not exceed 1ms over a 5-minute song

**Dependencies**: REQ-AUD-002, REQ-JDG-001  
**Source**: Roadmap subsystem 3, architecture decisions

---

## REQ-AUD-004: Sound Effects System
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

The audio system must support fire-and-forget short sound effects loaded entirely into memory for instant playback.

**Acceptance Criteria**:
- Load WAV, OGG, or MP3 files under 1 second duration
- Play SFX with under 10ms latency from trigger
- Multiple simultaneous SFX play without clipping (10+ concurrent)
- SFX volume independently controllable from music volume

**Dependencies**: REQ-AUD-001  
**Source**: Roadmap subsystem 3, Phase 3

---

## REQ-AUD-005: Key Sound Playback
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

The engine must play key sounds (panel press feedback audio) when panels are pressed during gameplay.

**Acceptance Criteria**:
- Key sound triggers immediately on panel press
- Key sound latency under 5ms
- Per-column key sounds supported
- Key sounds mix with music and judgment SFX

**Dependencies**: REQ-AUD-004, REQ-INP-002  
**Source**: Roadmap subsystem 3, Phase 3

---

## REQ-AUD-006: Judgment Sound Effects
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

The engine must play judgment sounds when notes are hit (Perfect, Great, Good, Bad, Miss).

**Acceptance Criteria**:
- Different sound per judgment type
- Sound triggers immediately after judgment issued
- Volume configurable per judgment type
- No audio artifacts when many notes hit simultaneously

**Dependencies**: REQ-AUD-004, REQ-JDG-001  
**Source**: Roadmap subsystem 3, Phase 3

---

## REQ-AUD-007: Global Audio Offset Calibration
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

The engine must provide a configurable global audio offset (positive or negative milliseconds) to calibrate for hardware-specific audio latency.

**Acceptance Criteria**:
- Offset range -500ms to +500ms in 1ms increments
- Offset applied before judge timing calculations
- Offset persists in user profile
- Calibration screen helps user determine correct offset

**Dependencies**: REQ-AUD-002, REQ-DAT-001  
**Source**: Roadmap subsystem 3, Phase 5

---

## REQ-AUD-008: Audio Calibration Screen
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

The engine must provide a calibration screen where users can adjust audio offset and receive visual/audio feedback.

**Acceptance Criteria**:
- Plays metronome click or simple pattern
- Shows visual timing bar synchronized to audio
- User adjusts offset until visual/audio align
- Real-time offset adjustment preview
- Saves offset to profile on exit

**Dependencies**: REQ-AUD-007, REQ-SCN-002  
**Source**: Roadmap Phase 5

---

## REQ-AUD-009: SDL3 Audio Backend
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

Initial audio implementation must use SDL3's audio API (SDL_AudioStream) for push-based streaming.

**Acceptance Criteria**:
- Compiles and links against SDL3 audio module
- Works on Linux (ALSA/PulseAudio) and Windows (WASAPI)
- Output latency under 50ms on default settings
- Stable operation over extended sessions (hours)

**Dependencies**: REQ-AUD-001  
**Source**: Roadmap subsystem 3

---

## REQ-AUD-010: Audio Backend Swappable
**Status**: [PLANNED Phase 3]  
**Priority**: Should Have

The audio backend must be swappable (SDL3, SoLoud, miniaudio) behind a common interface without changing calling code. Phase 1 implements SDL3 backend only. The swappable interface is designed in Phase 1 but proving a second backend is deferred.

**Acceptance Criteria**:
- AudioSystem interface hides backend details (Phase 1)
- SDL3 backend fully functional (Phase 1)
- Backend selection at compile time or runtime (Phase 3)
- All backends pass same test suite (Phase 3)
- Backend switching does not break saved profiles (Phase 3)

**Dependencies**: REQ-AUD-001  
**Source**: Roadmap subsystem 3

---

## REQ-AUD-011: Audio File Format Support
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must support OGG Vorbis and MP3 audio formats for music files.

**Acceptance Criteria**:
- Loads OGG Vorbis files (any bitrate, 44.1/48 kHz)
- Loads MP3 files (CBR and VBR, 44.1/48 kHz)
- Handles stereo and mono files
- Rejects or logs error for unsupported formats

**Dependencies**: REQ-AUD-001  
**Source**: Roadmap Phase 1 (implied)
