# Audio System User Stories

This document contains vertically-sliced user stories for the audio system requirements (REQ-AUD-001 through REQ-AUD-011). Stories are organized by requirement and marked with implementation status.

---

## Epic: Music Stream Playback

### Story ID: US-AUD-001 - Load OGG Vorbis Music Files

**Story Card:**
> **As a** Player
> **I want** the engine to load and play OGG Vorbis audio files
> **So that** I can play songs with compressed music

**References**: REQ-AUD-001, REQ-AUD-011, Roadmap Phase 1

**Status**: PLANNED

### 📝 Description
Implement OGG Vorbis file decoding to memory or streaming buffers. Supports all bitrates at 44.1 kHz and 48 kHz sample rates, stereo and mono channels.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Load stereo 44.1 kHz OGG file**
    *   **Given** a valid OGG Vorbis file with 44.1 kHz stereo audio
    *   **When** the file is loaded by the audio system
    *   **Then** the system reports successful loading
    *   **And** playback starts without errors

*   **Scenario 2: Load mono 48 kHz OGG file**
    *   **Given** a valid OGG Vorbis file with 48 kHz mono audio
    *   **When** the file is loaded by the audio system
    *   **Then** the system reports successful loading
    *   **And** playback starts without errors

*   **Scenario 3: Load variable bitrate OGG file**
    *   **Given** a variable bitrate OGG Vorbis file
    *   **When** the file is loaded and played to completion
    *   **Then** playback proceeds without glitches
    *   **And** position tracking remains accurate throughout

*   **Scenario 4: Reject corrupted OGG file**
    *   **Given** a file with .ogg extension containing invalid data
    *   **When** attempting to load the file
    *   **Then** loading fails with an error code
    *   **And** an ERROR-level log message specifies the file path and reason

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-ENG-011 (Engine class), REQ-AST-001 (asset discovery)
*   **Implementation Note**: Use stb_vorbis or SDL3_mixer for decoding. Must support streaming to avoid loading entire file into memory.

---

### Story ID: US-AUD-002 - Load MP3 Music Files

**Story Card:**
> **As a** Player
> **I want** the engine to load and play MP3 audio files
> **So that** I can play songs from game versions that use MP3 format

**References**: REQ-AUD-001, REQ-AUD-011, Roadmap Phase 1

**Status**: PLANNED

### 📝 Description
Implement MP3 file decoding supporting both constant bitrate (CBR) and variable bitrate (VBR) files at 44.1 kHz and 48 kHz sample rates.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Load CBR MP3 file**
    *   **Given** a 192 kbps constant bitrate MP3 file
    *   **When** the file is loaded by the audio system
    *   **Then** the system reports successful loading
    *   **And** playback starts without errors

*   **Scenario 2: Load VBR MP3 file**
    *   **Given** a variable bitrate MP3 file with XING header
    *   **When** the file is loaded and played to completion
    *   **Then** playback proceeds without glitches
    *   **And** position tracking accounts for VBR frame timing

*   **Scenario 3: Handle MP3 without ID3 tags**
    *   **Given** an MP3 file with no ID3 metadata tags
    *   **When** the file is loaded
    *   **Then** loading succeeds
    *   **And** playback is unaffected by missing metadata

*   **Scenario 4: Reject unsupported MP3 format**
    *   **Given** a file with .mp3 extension containing unsupported encoding
    *   **When** attempting to load the file
    *   **Then** loading fails with an error code
    *   **And** an ERROR-level log message specifies the file path and unsupported format

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-AUD-001
*   **Implementation Note**: Use minimp3 or SDL3_mixer. VBR timing requires parsing XING/VBRI headers.

---

### Story ID: US-AUD-003 - Implement Music Playback Controls

**Story Card:**
> **As a** Player
> **I want** to play, pause, resume, and stop music during gameplay
> **So that** I can control song playback during practice mode

**References**: REQ-AUD-001, Roadmap Phase 1

**Status**: PLANNED

### 📝 Description
Implement basic transport controls for music streams: play from beginning, pause, resume from current position, and stop with buffer cleanup.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Play starts from beginning**
    *   **Given** a loaded music file
    *   **When** play() is called
    *   **Then** playback begins from position 0 milliseconds
    *   **And** audio output is audible within 100 milliseconds

*   **Scenario 2: Pause stops output and preserves position**
    *   **Given** music is playing at position 5000 milliseconds
    *   **When** pause() is called
    *   **Then** audio output stops immediately
    *   **And** position remains at 5000 milliseconds
    *   **And** subsequent get_position_ms() returns 5000

*   **Scenario 3: Resume continues from paused position**
    *   **Given** music is paused at position 5000 milliseconds
    *   **When** resume() is called
    *   **Then** playback continues from 5000 milliseconds
    *   **And** audio output is audible within 100 milliseconds

*   **Scenario 4: Stop resets position and releases buffers**
    *   **Given** music is playing or paused
    *   **When** stop() is called
    *   **Then** audio output stops immediately
    *   **And** position resets to 0 milliseconds
    *   **And** audio buffers are released

*   **Scenario 5: Multiple pause/resume cycles work correctly**
    *   **Given** music is playing
    *   **When** pause() and resume() are called 10 times in sequence
    *   **Then** playback position progresses monotonically
    *   **And** no audio artifacts occur

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-AUD-001, US-AUD-002
*   **Implementation Note**: State machine: STOPPED → PLAYING → PAUSED → PLAYING → STOPPED

---

### Story ID: US-AUD-004 - Implement Seek to Millisecond Position

**Story Card:**
> **As a** Player
> **I want** to seek to a specific position in a song
> **So that** I can practice difficult sections without playing from the start

**References**: REQ-AUD-001, Roadmap Phase 1

**Status**: PLANNED

### 📝 Description
Implement seek functionality that allows jumping to any millisecond position within a loaded music file, working correctly with both OGG and MP3 formats.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Seek forward while playing**
    *   **Given** music is playing at position 2000 milliseconds
    *   **When** seek(10000) is called
    *   **Then** playback jumps to position 10000 milliseconds
    *   **And** subsequent get_position_ms() returns values starting from 10000
    *   **And** audio output resumes within 200 milliseconds

*   **Scenario 2: Seek backward while playing**
    *   **Given** music is playing at position 30000 milliseconds
    *   **When** seek(15000) is called
    *   **Then** playback jumps to position 15000 milliseconds
    *   **And** subsequent get_position_ms() returns values starting from 15000

*   **Scenario 3: Seek while paused**
    *   **Given** music is paused at position 5000 milliseconds
    *   **When** seek(20000) is called
    *   **Then** position changes to 20000 milliseconds
    *   **And** music remains paused
    *   **And** resume() begins playback from 20000 milliseconds

*   **Scenario 4: Seek beyond end of file**
    *   **Given** music file duration is 180000 milliseconds
    *   **When** seek(200000) is called
    *   **Then** position clamps to 180000 milliseconds
    *   **And** playback stops automatically

*   **Scenario 5: Seek to negative position**
    *   **Given** music is playing
    *   **When** seek(-1000) is called
    *   **Then** position clamps to 0 milliseconds
    *   **And** playback continues from the beginning

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-AUD-003
*   **Implementation Note**: VBR MP3 requires frame index for accurate seeking. OGG Vorbis supports direct sample seeking.

---

## Epic: Authoritative Audio Position

### Story ID: US-AUD-011 - Report Hardware Sample-Accurate Position

**Story Card:**
> **As a** Developer
> **I want** get_position_ms() to return position based on samples consumed by audio hardware
> **So that** the judge can synchronize timing to what the player actually hears

**References**: REQ-AUD-002, Roadmap Phase 1, Subsystem 3 (Audio System)

**Status**: PLANNED

### 📝 Description
Implement sample-accurate position tracking that accounts for the audio hardware buffer. This is the authoritative time source for gameplay, eliminating drift between audio output and game logic.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Position accuracy within 2 milliseconds**
    *   **Given** music is playing at exactly 60000 samples consumed (1360 ms at 44.1 kHz)
    *   **When** get_position_ms() is called
    *   **Then** the returned value is between 1358 and 1362 milliseconds

*   **Scenario 2: Position updates at 60 Hz query rate**
    *   **Given** music is playing
    *   **When** get_position_ms() is called every 16.67 milliseconds for 10 seconds
    *   **Then** each call returns a monotonically increasing value
    *   **And** no two consecutive calls return the same value
    *   **And** the total elapsed time is within 20 milliseconds of expected

*   **Scenario 3: No drift over 5 minute song**
    *   **Given** music is playing for exactly 300 seconds (13,230,000 samples at 44.1 kHz)
    *   **When** comparing get_position_ms() to expected time
    *   **Then** the difference is under 50 milliseconds
    *   **And** drift does not accumulate linearly over time

*   **Scenario 4: Position accounts for hardware buffer size**
    *   **Given** the audio hardware has a 512-sample output buffer
    *   **When** 4096 samples have been queued but only 3584 have been consumed
    *   **Then** get_position_ms() reflects the 3584 consumed samples
    *   **And** not the 4096 queued samples

*   **Scenario 5: Position remains stable during pause**
    *   **Given** music is paused at position 45000 milliseconds
    *   **When** get_position_ms() is called repeatedly over 2 seconds
    *   **Then** all calls return 45000 milliseconds
    *   **And** no drift occurs during pause

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points
*   **Dependencies**: US-AUD-003
*   **Implementation Note**: Track samples consumed by hardware callback, convert to milliseconds using actual sample rate. Requires lock-free atomic counter for callback→main thread communication.

---

### Story ID: US-AUD-012 - Maintain Position Accuracy Across Seek Operations

**Story Card:**
> **As a** Developer
> **I want** position tracking to remain accurate immediately after seeking
> **So that** the judge does not receive stale timestamps after practice mode seeks

**References**: REQ-AUD-002

**Status**: PLANNED

### 📝 Description
Ensure that sample-accurate position tracking resets correctly when seeking, accounting for buffer refill latency and decoder state changes.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Position accurate immediately after seek**
    *   **Given** music is playing at position 2000 milliseconds
    *   **When** seek(60000) is called
    *   **Then** the next get_position_ms() call returns a value within 50 milliseconds of 60000
    *   **And** subsequent calls show monotonic progression from the seek target

*   **Scenario 2: Seek does not introduce timing discontinuity**
    *   **Given** the judge is tracking notes at the seek target position
    *   **When** seek(60000) completes
    *   **Then** the judge receives position 60000 within 100 milliseconds
    *   **And** no negative time deltas occur

*   **Scenario 3: Buffer refill does not report stale position**
    *   **Given** a seek operation is in progress
    *   **When** the audio buffer is being refilled from the new position
    *   **Then** get_position_ms() does not return the pre-seek position
    *   **And** position jumps directly to the seek target

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-AUD-004, US-AUD-011
*   **Implementation Note**: Reset sample counter atomically when decoder jumps to new position. Flush hardware buffer.

---

## Epic: Judge Timing Integration

### Story ID: US-AUD-021 - Provide Audio Position to Judge Each Tick

**Story Card:**
> **As a** Developer
> **I want** the judge to query audio position each tick
> **So that** note timing is synchronized to what the player hears

**References**: REQ-AUD-003, Roadmap Phase 1, Subsystem 3 + Subsystem 5

**Status**: PLANNED

### 📝 Description
Integrate audio position as the authoritative time source for the gameplay judge. The judge queries get_position_ms() each logic tick instead of using wall-clock time, ensuring perfect synchronization between audio and judgment.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Judge queries audio position each tick**
    *   **Given** gameplay is active with music playing
    *   **When** the judge updates each logic tick for 600 ticks
    *   **Then** get_position_ms() is called exactly 600 times
    *   **And** the judge uses the returned position for note timing calculations

*   **Scenario 2: Judge does not accumulate drift**
    *   **Given** music has been playing for 5 minutes
    *   **When** comparing judge time to audio position
    *   **Then** the difference is under 10 milliseconds
    *   **And** drift does not accumulate over time

*   **Scenario 3: Pause stops judge timing**
    *   **Given** music is paused at position 45000 milliseconds
    *   **When** the judge updates for 60 ticks while paused
    *   **Then** all judge timing calculations use position 45000 milliseconds
    *   **And** notes do not auto-miss due to time progression

*   **Scenario 4: Seek updates judge position immediately**
    *   **Given** music is playing at position 30000 milliseconds
    *   **When** seek(60000) is called
    *   **Then** the next judge update uses position 60000 milliseconds
    *   **And** notes between 30000 and 60000 are not judged during the gap

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-AUD-011, REQ-JDG-001 (judge implementation)
*   **Implementation Note**: GameplayScene passes audio position to Judge::update(). Judge stores no internal time accumulator.

---

## Epic: Sound Effects System

### Story ID: US-AUD-031 - Load Short Audio Samples into Memory

**Story Card:**
> **As a** Developer
> **I want** to load short audio samples entirely into memory
> **So that** sound effects can be triggered with minimal latency

**References**: REQ-AUD-004, Roadmap Phase 3

**Status**: PLANNED

### 📝 Description
Implement memory-resident audio sample loading for WAV, OGG, and MP3 files under 1 second duration. Samples are fully decoded to PCM on load for instant playback.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Load WAV file under 1 second**
    *   **Given** a 500 millisecond WAV file at 44.1 kHz
    *   **When** the file is loaded as a sound effect
    *   **Then** the entire file is decoded to memory
    *   **And** playback can be triggered without further disk I/O

*   **Scenario 2: Load short OGG file**
    *   **Given** a 300 millisecond OGG Vorbis file
    *   **When** the file is loaded as a sound effect
    *   **Then** the entire file is decoded to memory
    *   **And** memory usage increases by approximately 50 KB (stereo 44.1 kHz PCM)

*   **Scenario 3: Reject files over 1 second**
    *   **Given** a 2 second audio file
    *   **When** attempting to load as a sound effect
    *   **Then** loading fails with an error code
    *   **And** an ERROR-level log message specifies the file exceeds duration limit

*   **Scenario 4: Multiple samples can be loaded concurrently**
    *   **Given** 20 sound effect files
    *   **When** all are loaded
    *   **Then** all samples remain in memory
    *   **And** each can be triggered independently

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-AUD-001, US-AUD-002
*   **Implementation Note**: Store decoded PCM in std::vector<int16_t>. Memory budget: 20 samples × ~100 KB = 2 MB typical.

---

### Story ID: US-AUD-032 - Play Sound Effects with Low Latency

**Story Card:**
> **As a** Player
> **I want** sound effects to play immediately when triggered
> **So that** audio feedback feels responsive to my input

**References**: REQ-AUD-004, Roadmap Phase 3

**Status**: PLANNED

### 📝 Description
Implement fire-and-forget sound effect playback that triggers within 10 milliseconds of the trigger call, supporting multiple concurrent instances.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: SFX triggers within 10 milliseconds**
    *   **Given** a loaded sound effect sample
    *   **When** play_sfx() is called with an injectable timestamp
    *   **Then** the audio callback begins outputting samples within 10 milliseconds
    *   **And** the sample plays to completion without interruption

*   **Scenario 2: Multiple concurrent SFX play without clipping**
    *   **Given** 10 loaded sound effect samples
    *   **When** all 10 are triggered simultaneously
    *   **Then** all 10 play concurrently
    *   **And** audio output remains within the -1.0 to +1.0 range (no hard clipping)
    *   **And** no audio artifacts occur

*   **Scenario 3: SFX does not interrupt music playback**
    *   **Given** music is playing at normal volume
    *   **When** a sound effect is triggered
    *   **Then** music continues uninterrupted
    *   **And** both music and SFX are audible

*   **Scenario 4: Same SFX can be triggered multiple times**
    *   **Given** a single loaded sound effect sample
    *   **When** play_sfx() is called 5 times with 50 millisecond spacing
    *   **Then** 5 overlapping instances play concurrently
    *   **And** all complete without audio artifacts

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points
*   **Dependencies**: US-AUD-031
*   **Implementation Note**: Mix multiple voices in audio callback. Use software mixing with saturation to prevent clipping.

---

### Story ID: US-AUD-033 - Independent SFX Volume Control

**Story Card:**
> **As a** Player
> **I want** to control sound effects volume separately from music volume
> **So that** I can balance audio feedback with song audio

**References**: REQ-AUD-004

**Status**: PLANNED

### 📝 Description
Implement independent volume controls for music and sound effects, each ranging from 0 (silent) to 100 (full volume).

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: SFX volume adjusts without affecting music**
    *   **Given** music is playing at volume 80 and SFX volume is 50
    *   **When** SFX volume is changed to 100
    *   **Then** subsequent sound effects play at full volume
    *   **And** music volume remains at 80

*   **Scenario 2: Music volume adjusts without affecting SFX**
    *   **Given** music is playing at volume 80 and SFX volume is 50
    *   **When** music volume is changed to 30
    *   **Then** music plays at reduced volume
    *   **And** subsequent sound effects still play at volume 50

*   **Scenario 3: SFX volume at 0 silences effects**
    *   **Given** SFX volume is set to 0
    *   **When** a sound effect is triggered
    *   **Then** no audio output occurs for the SFX
    *   **And** music continues playing

*   **Scenario 4: Volume changes apply immediately**
    *   **Given** a sound effect is currently playing
    *   **When** SFX volume is changed mid-playback
    *   **Then** the playing SFX volume adjusts within 20 milliseconds
    *   **And** no audio pops or clicks occur

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-AUD-032
*   **Implementation Note**: Apply volume scaling during mix. Volume 0–100 maps to gain 0.0–1.0.

---

## Epic: Key Sound Playback

### Story ID: US-AUD-041 - Trigger Key Sounds on Panel Press

**Story Card:**
> **As a** Player
> **I want** to hear immediate audio feedback when I press a panel
> **So that** I can feel connected to the game through tactile audio response

**References**: REQ-AUD-005, Roadmap Phase 3

**Status**: PLANNED

### 📝 Description
Play a key sound sample immediately when a panel is pressed during gameplay, with latency under 5 milliseconds from input to audio output.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Key sound triggers on panel press**
    *   **Given** gameplay is active with key sounds loaded
    *   **When** a panel press event is received
    *   **Then** a key sound plays within 5 milliseconds
    *   **And** the sound corresponds to the pressed panel

*   **Scenario 2: Key sound does not trigger on note miss**
    *   **Given** no note is near the judgment line
    *   **When** a panel is pressed
    *   **Then** a key sound plays
    *   **And** no judgment sound plays

*   **Scenario 3: Rapid panel presses play distinct sounds**
    *   **Given** a player presses 5 panels within 100 milliseconds
    *   **When** all input events are processed
    *   **Then** 5 distinct key sound instances play
    *   **And** all are audible without artifacts

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-AUD-032, REQ-INP-002 (input system)
*   **Implementation Note**: Hook input edge events in GameplayScene. Trigger SFX before judge processes input.

---

### Story ID: US-AUD-042 - Support Per-Column Key Sounds

**Story Card:**
> **As a** Player
> **I want** different audio feedback for each panel direction
> **So that** I can distinguish panel presses by sound

**References**: REQ-AUD-005

**Status**: PLANNED

### 📝 Description
Allow loading and triggering unique key sound samples for each of the 5 panel directions (down-left, up-left, center, up-right, down-right).

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Center panel has distinct sound**
    *   **Given** unique key sounds are loaded for each panel
    *   **When** the center panel is pressed
    *   **Then** the center key sound plays
    *   **And** the sound is distinguishable from arrow panel sounds

*   **Scenario 2: Arrow panels have directional sounds**
    *   **Given** unique key sounds are loaded for each arrow direction
    *   **When** up-left panel is pressed
    *   **Then** the up-left key sound plays
    *   **And** pressing up-right plays a different sound

*   **Scenario 3: Fallback to single key sound if no per-column sounds**
    *   **Given** only a default key sound is loaded
    *   **When** any panel is pressed
    *   **Then** the default key sound plays for all panels

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-AUD-041
*   **Implementation Note**: Asset manifest specifies per-column key sounds with fallback to default.

---

## Epic: Judgment Sound Effects

### Story ID: US-AUD-051 - Play Judgment Sounds Based on Timing

**Story Card:**
> **As a** Player
> **I want** to hear distinct audio feedback for each judgment type
> **So that** I can learn my timing accuracy through sound

**References**: REQ-AUD-006, Roadmap Phase 3

**Status**: PLANNED

### 📝 Description
Play unique sound effects when notes are judged as Perfect, Great, Good, Bad, or Miss, triggered immediately after the judge issues the judgment.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Perfect judgment plays perfect sound**
    *   **Given** the judge issues a Perfect judgment
    *   **When** the judgment event is processed
    *   **Then** the Perfect judgment sound plays within 10 milliseconds
    *   **And** no other judgment sound plays

*   **Scenario 2: Miss judgment plays miss sound**
    *   **Given** a note passes the judgment window without being hit
    *   **When** the judge auto-issues a Miss
    *   **Then** the Miss judgment sound plays
    *   **And** the sound is distinct from hit judgment sounds

*   **Scenario 3: Judgment sounds do not overlap same type**
    *   **Given** two notes are hit with Perfect timing within 30 milliseconds
    *   **When** both judgments are issued
    *   **Then** two Perfect sound instances play concurrently
    *   **And** both are audible without artifacts

*   **Scenario 4: Different judgment types are distinguishable**
    *   **Given** Perfect, Great, Good, Bad, and Miss sounds are loaded
    *   **When** each judgment type occurs in sequence
    *   **Then** each sound is audibly distinct from the others

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-AUD-032, REQ-JDG-001 (judge emits judgment events)
*   **Implementation Note**: GameplayScene subscribes to JudgmentEvent and triggers SFX.

---

### Story ID: US-AUD-052 - Configurable Judgment Sound Volume

**Story Card:**
> **As a** Player
> **I want** to adjust volume for each judgment type independently
> **So that** I can emphasize Perfect judgments and reduce Miss sound harshness

**References**: REQ-AUD-006

**Status**: PLANNED

### 📝 Description
Allow per-judgment-type volume control, each ranging from 0 (silent) to 100 (full volume), saved in user profile.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Perfect volume adjusted independently**
    *   **Given** Perfect judgment volume is set to 100 and Miss volume is set to 20
    *   **When** a Perfect judgment occurs
    *   **Then** the Perfect sound plays at full volume
    *   **And** subsequent Miss judgments play at 20% volume

*   **Scenario 2: Judgment silenced when volume is 0**
    *   **Given** Bad judgment volume is set to 0
    *   **When** a Bad judgment occurs
    *   **Then** no Bad judgment sound plays
    *   **And** other judgment sounds remain audible

*   **Scenario 3: Volume settings persist across sessions**
    *   **Given** judgment volumes are configured in the profile
    *   **When** the game is restarted
    *   **Then** the configured volumes are restored
    *   **And** judgments play at the saved volumes

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-AUD-051, REQ-DAT-001 (profile system)
*   **Implementation Note**: Scale SFX gain per judgment type before mixing.

---

## Epic: Global Audio Offset Calibration

### Story ID: US-AUD-061 - Apply Global Audio Offset to Judge Timing

**Story Card:**
> **As a** Player
> **I want** to configure a global audio offset in milliseconds
> **So that** I can compensate for my audio hardware's inherent latency

**References**: REQ-AUD-007, Roadmap Phase 5

**Status**: PLANNED

### 📝 Description
Implement a configurable global audio offset between -500 and +500 milliseconds that shifts judge timing calculations to account for hardware-specific audio latency.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Positive offset delays judgment timing**
    *   **Given** global offset is set to +50 milliseconds
    *   **When** the audio position is 10000 milliseconds
    *   **Then** the judge perceives the position as 10050 milliseconds
    *   **And** notes are judged 50 milliseconds later than without offset

*   **Scenario 2: Negative offset advances judgment timing**
    *   **Given** global offset is set to -30 milliseconds
    *   **When** the audio position is 10000 milliseconds
    *   **Then** the judge perceives the position as 9970 milliseconds
    *   **And** notes are judged 30 milliseconds earlier than without offset

*   **Scenario 3: Offset range is constrained to ±500 milliseconds**
    *   **Given** the user attempts to set offset to +600 milliseconds
    *   **When** the value is validated
    *   **Then** the offset clamps to +500 milliseconds
    *   **And** a warning is logged

*   **Scenario 4: Offset changes apply immediately**
    *   **Given** gameplay is active with offset +20 milliseconds
    *   **When** offset is changed to -10 milliseconds
    *   **Then** subsequent judgments use the new -10 millisecond offset
    *   **And** no audio discontinuity occurs

*   **Scenario 5: Offset adjusts in 1 millisecond increments**
    *   **Given** the calibration screen is active
    *   **When** the user increments the offset
    *   **Then** the value changes by exactly 1 millisecond per step

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-AUD-021 (judge queries audio position), REQ-DAT-001 (profile system)
*   **Implementation Note**: Add offset before passing position to judge. Store in user profile.

---

### Story ID: US-AUD-062 - Persist Audio Offset in User Profile

**Story Card:**
> **As a** Player
> **I want** my audio offset saved in my profile
> **So that** I don't need to recalibrate every time I play

**References**: REQ-AUD-007

**Status**: PLANNED

### 📝 Description
Save the global audio offset value in the user's profile JSON file and restore it on next launch.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Offset saved to profile**
    *   **Given** global offset is set to +45 milliseconds
    *   **When** the profile is saved
    *   **Then** the profile JSON file contains `"audio_offset_ms": 45`

*   **Scenario 2: Offset restored on load**
    *   **Given** the profile contains `"audio_offset_ms": -25`
    *   **When** the profile is loaded at startup
    *   **Then** the global offset is set to -25 milliseconds
    *   **And** subsequent gameplay uses this offset

*   **Scenario 3: Default offset is 0 for new profiles**
    *   **Given** a newly created profile with no audio_offset_ms field
    *   **When** the profile is loaded
    *   **Then** the global offset defaults to 0 milliseconds

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1 story point
*   **Dependencies**: US-AUD-061, REQ-DAT-001 (profile system)
*   **Implementation Note**: Single integer field in profile JSON.

---

## Epic: Audio Calibration Screen

### Story ID: US-AUD-071 - Create Calibration Screen with Metronome

**Story Card:**
> **As a** Player
> **I want** a calibration screen that plays a metronome and visual cues
> **So that** I can determine the correct audio offset for my setup

**References**: REQ-AUD-008, Roadmap Phase 5

**Status**: PLANNED

### 📝 Description
Implement a calibration screen that plays a 60 BPM metronome click track with synchronized visual timing bars, allowing the player to adjust the audio offset until audio and visuals align.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Metronome plays at 60 BPM**
    *   **Given** the calibration screen is active
    *   **When** the metronome starts
    *   **Then** audio clicks occur at exactly 1000 millisecond intervals
    *   **And** timing remains consistent over 60 seconds

*   **Scenario 2: Visual bar synchronized to metronome**
    *   **Given** the metronome is playing with 0 offset
    *   **When** an audio click occurs
    *   **Then** a visual timing bar hits the target line simultaneously
    *   **And** the visual and audio remain synchronized over time

*   **Scenario 3: Offset adjustment shifts visual relative to audio**
    *   **Given** the metronome is playing
    *   **When** the offset is adjusted to +50 milliseconds
    *   **Then** the visual bar hits the target line 50 milliseconds before the audio click
    *   **And** this offset remains consistent

*   **Scenario 4: Real-time offset adjustment preview**
    *   **Given** the metronome is playing
    *   **When** the player increments the offset by 10 milliseconds
    *   **Then** the next visual bar reflects the new offset
    *   **And** no metronome restart is required

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points
*   **Dependencies**: US-AUD-061, REQ-SCN-002 (scene system)
*   **Implementation Note**: Use BGA animation for visual bar. Metronome as looping SFX sample. Offset shifts visual timing, not audio.

---

### Story ID: US-AUD-072 - Save Calibration and Return to Previous Screen

**Story Card:**
> **As a** Player
> **I want** to save my offset setting and return to the previous screen
> **So that** I can test the calibration in actual gameplay

**References**: REQ-AUD-008

**Status**: PLANNED

### 📝 Description
Allow the player to save the adjusted offset to their profile and return to the previous screen (typically options menu).

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Save button persists offset**
    *   **Given** the calibration screen has offset adjusted to +35 milliseconds
    *   **When** the player presses the save button
    *   **Then** the profile is saved with audio_offset_ms set to 35
    *   **And** the screen returns to the options menu

*   **Scenario 2: Cancel button discards changes**
    *   **Given** the calibration screen has offset adjusted to +35 milliseconds
    *   **And** the profile previously had offset +10 milliseconds
    *   **When** the player presses cancel
    *   **Then** the offset reverts to +10 milliseconds
    *   **And** the screen returns to the options menu

*   **Scenario 3: Offset applies immediately after save**
    *   **Given** calibration is saved with offset +40 milliseconds
    *   **When** the player enters gameplay
    *   **Then** the judge uses the +40 millisecond offset
    *   **And** timing matches the calibration screen preview

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1 story point
*   **Dependencies**: US-AUD-071, US-AUD-062
*   **Implementation Note**: Standard scene stack pop with state restoration on cancel.

---

## Epic: SDL3 Audio Backend

### Story ID: US-AUD-081 - Initialize SDL3 Audio Subsystem

**Story Card:**
> **As a** Developer
> **I want** to initialize SDL3 audio on Linux and Windows
> **So that** the engine can output audio on supported platforms

**References**: REQ-AUD-009, Roadmap Phase 1

**Status**: PLANNED

### 📝 Description
Initialize SDL3's audio subsystem during engine startup, selecting appropriate audio device, configuring output format, and establishing audio callback.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: SDL3 audio initializes on Linux**
    *   **Given** a Linux system with ALSA or PulseAudio
    *   **When** the audio system initializes
    *   **Then** SDL_InitSubSystem(SDL_INIT_AUDIO) succeeds
    *   **And** an audio device is opened successfully
    *   **And** no ERROR-level log messages occur

*   **Scenario 2: SDL3 audio initializes on Windows**
    *   **Given** a Windows system with WASAPI audio
    *   **When** the audio system initializes
    *   **Then** SDL_InitSubSystem(SDL_INIT_AUDIO) succeeds
    *   **And** an audio device is opened successfully

*   **Scenario 3: Output latency under 50 milliseconds**
    *   **Given** SDL3 audio is initialized with default settings
    *   **When** measuring time between audio submission and output
    *   **Then** latency is under 50 milliseconds
    *   **And** no audio pops or clicks occur

*   **Scenario 4: Audio initialization failure is logged**
    *   **Given** no audio device is available
    *   **When** audio system initialization is attempted
    *   **Then** initialization fails gracefully
    *   **And** an ERROR-level log message specifies the SDL error string
    *   **And** the engine continues without audio

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-ENG-011 (Engine class)
*   **Implementation Note**: Use SDL_AudioStream for push-based streaming. Request 44.1 kHz stereo float output.

---

### Story ID: US-AUD-082 - Verify Stable Operation Over Extended Sessions

**Story Card:**
> **As a** Player
> **I want** audio to remain stable over multi-hour play sessions
> **So that** I don't experience audio dropouts or crashes during long sessions

**References**: REQ-AUD-009

**Status**: PLANNED

### 📝 Description
Ensure the SDL3 audio backend operates without memory leaks, buffer overruns, or degraded performance over extended runtime.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: No audio dropouts over 2 hours**
    *   **Given** music has been playing for 2 hours continuously
    *   **When** monitoring audio output
    *   **Then** no dropouts or glitches occur
    *   **And** output latency remains under 50 milliseconds

*   **Scenario 2: Memory usage remains stable**
    *   **Given** the audio system has been running for 4 hours
    *   **When** measuring memory usage
    *   **Then** audio subsystem memory usage has not increased more than 10%
    *   **And** no memory leaks are detected by Valgrind

*   **Scenario 3: Audio thread does not block**
    *   **Given** the audio callback is running
    *   **When** main thread logic experiences frame drops
    *   **Then** audio output continues without interruption
    *   **And** no audio artifacts occur

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-AUD-081
*   **Implementation Note**: Long-running integration test. Monitor with system profiler and memory checker.

---

## Epic: Swappable Audio Backend

### Story ID: US-AUD-091 - Define AudioSystem Interface

**Story Card:**
> **As a** Developer
> **I want** an abstract AudioSystem interface
> **So that** backend implementations can be swapped without changing calling code

**References**: REQ-AUD-010, Roadmap Phase 1

**Status**: PLANNED

### 📝 Description
Define a pure virtual AudioSystem interface that hides backend-specific details (SDL3, SoLoud, miniaudio) behind a common API for music and SFX playback.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Interface defines all music operations**
    *   **Given** the AudioSystem interface
    *   **When** reviewing the interface definition
    *   **Then** methods exist for load_music(), play(), pause(), resume(), stop(), seek(), and get_position_ms()
    *   **And** no SDL3-specific types appear in the interface

*   **Scenario 2: Interface defines all SFX operations**
    *   **Given** the AudioSystem interface
    *   **When** reviewing the interface definition
    *   **Then** methods exist for load_sfx(), play_sfx(), set_music_volume(), and set_sfx_volume()
    *   **And** no backend-specific types appear in the interface

*   **Scenario 3: GameplayScene only depends on interface**
    *   **Given** the GameplayScene class
    *   **When** reviewing its dependencies
    *   **Then** it includes only AudioSystem interface header
    *   **And** it does not include any backend-specific headers

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: None (foundational)
*   **Implementation Note**: Pure virtual interface with no SDL3/SoLoud includes in header.

---

### Story ID: US-AUD-092 - Implement SDL3AudioSystem Backend

**Story Card:**
> **As a** Developer
> **I want** an SDL3-based implementation of the AudioSystem interface
> **So that** the engine has a working audio backend for Phase 1

**References**: REQ-AUD-010

**Status**: PLANNED

### 📝 Description
Implement the AudioSystem interface using SDL3 audio APIs, wrapping SDL_AudioStream and SDL_Mixer functionality.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: SDL3AudioSystem implements interface**
    *   **Given** the SDL3AudioSystem class
    *   **When** reviewing the class declaration
    *   **Then** it inherits from AudioSystem
    *   **And** it implements all pure virtual methods

*   **Scenario 2: Backend passes music playback test**
    *   **Given** an SDL3AudioSystem instance
    *   **When** running music playback tests
    *   **Then** all tests pass with the same behavior as the interface specification

*   **Scenario 3: Backend passes SFX test**
    *   **Given** an SDL3AudioSystem instance
    *   **When** running SFX tests
    *   **Then** all tests pass with the same behavior as the interface specification

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points
*   **Dependencies**: US-AUD-091, US-AUD-081
*   **Implementation Note**: Combine US-AUD-001 through US-AUD-033 implementations under interface.

---

### Story ID: US-AUD-093 - Select Audio Backend at Compile Time

**Story Card:**
> **As a** Developer
> **I want** to select the audio backend via CMake option
> **So that** I can test alternative backends without changing source code

**References**: REQ-AUD-010

**Status**: FUTURE

### 📝 Description
Add CMake configuration to select between SDL3, SoLoud, or miniaudio backends at compile time, with SDL3 as the default.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Default builds use SDL3**
    *   **Given** no audio backend is explicitly specified
    *   **When** running CMake configuration
    *   **Then** SDL3AudioSystem is selected
    *   **And** the engine compiles successfully

*   **Scenario 2: CMake option selects backend**
    *   **Given** `-DAUDIO_BACKEND=SDL3` is passed to CMake
    *   **When** the project is configured and built
    *   **Then** SDL3AudioSystem is compiled and linked
    *   **And** no SoLoud or miniaudio code is compiled

*   **Scenario 3: GameplayScene works with any backend**
    *   **Given** the engine is built with SDL3 backend
    *   **When** gameplay is tested
    *   **Then** all audio functionality works correctly
    *   **And** rebuilding with a different backend does not require code changes

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-AUD-092
*   **Implementation Note**: CMake preprocessor defines. Backend factory function returns concrete implementation.

---

## Non-Functional Requirements

### NFR: Audio Position Accuracy (REQ-AUD-002)

**As a** Player
**I want** sample-accurate audio timing
**So that** judgments feel fair and consistent

**Acceptance Criteria**:
*   **Scenario 1: Position accuracy within 2 milliseconds**
    *   **Given** audio has been playing for 60 seconds
    *   **When** comparing get_position_ms() to actual consumed samples
    *   **Then** error is under 2 milliseconds
    *   **And** error does not accumulate over time

*   **Scenario 2: No drift over 5 minute songs**
    *   **Given** audio has been playing for 300 seconds
    *   **When** comparing get_position_ms() to expected time
    *   **Then** cumulative drift is under 50 milliseconds

*   **Scenario 3: Position updates at 60 Hz**
    *   **Given** get_position_ms() is called every 16.67 milliseconds
    *   **When** measuring 600 consecutive calls
    *   **Then** every call returns a unique, increasing value
    *   **And** no calls return stale cached values

**Testing Note**: Integration test in Phase 1. Use high-precision timer to verify sample counter accuracy.

---

### NFR: Sound Effect Latency (REQ-AUD-004, REQ-AUD-005)

**As a** Player
**I want** instant audio feedback when I press panels
**So that** the game feels responsive

**Acceptance Criteria**:
*   **Scenario 1: Key sound latency under 5 milliseconds**
    *   **Given** a panel press event occurs
    *   **When** measuring time from input to audio output
    *   **Then** latency is under 5 milliseconds
    *   **And** latency is consistent across all panels

*   **Scenario 2: SFX trigger latency under 10 milliseconds**
    *   **Given** a judgment sound is triggered
    *   **When** measuring time from trigger call to audio output
    *   **Then** latency is under 10 milliseconds

*   **Scenario 3: No latency increase under load**
    *   **Given** 10 concurrent sound effects are playing
    *   **When** an 11th sound effect is triggered
    *   **Then** latency remains under 10 milliseconds

**Testing Note**: Integration test in Phase 3. Requires hardware latency measurement with loopback or oscilloscope for full validation.

---

## Story Summary by Status

### PLANNED (29 stories)
- US-AUD-001: Load OGG Vorbis music files
- US-AUD-002: Load MP3 music files
- US-AUD-003: Implement music playback controls
- US-AUD-004: Implement seek to millisecond position
- US-AUD-011: Report hardware sample-accurate position
- US-AUD-012: Maintain position accuracy across seek operations
- US-AUD-021: Provide audio position to judge each tick
- US-AUD-031: Load short audio samples into memory
- US-AUD-032: Play sound effects with low latency
- US-AUD-033: Independent SFX volume control
- US-AUD-041: Trigger key sounds on panel press
- US-AUD-042: Support per-column key sounds
- US-AUD-051: Play judgment sounds based on timing
- US-AUD-052: Configurable judgment sound volume
- US-AUD-061: Apply global audio offset to judge timing
- US-AUD-062: Persist audio offset in user profile
- US-AUD-071: Create calibration screen with metronome
- US-AUD-072: Save calibration and return to previous screen
- US-AUD-081: Initialize SDL3 audio subsystem
- US-AUD-082: Verify stable operation over extended sessions
- US-AUD-091: Define AudioSystem interface
- US-AUD-092: Implement SDL3AudioSystem backend

### FUTURE (1 story)
- US-AUD-093: Select audio backend at compile time

---

## Story Point Summary

| Status | Count | Total Points |
|--------|-------|--------------|
| PLANNED | 22 | 63 |
| FUTURE | 1 | 2 |
| **Total** | **23** | **65** |

---

## Dependency Graph

```
Phase 1 Foundation (Music Playback):
US-AUD-001 (OGG) ──┬──> US-AUD-003 (Controls) ──> US-AUD-004 (Seek)
US-AUD-002 (MP3) ──┘                                    │
                                                        ↓
                                          US-AUD-011 (Sample-accurate position)
                                                        │
                         ┌──────────────────────────────┴────────────────────────┐
                         ↓                                                        ↓
              US-AUD-012 (Position after seek)                    US-AUD-021 (Judge integration)
                                                                                  │
                                                                   REQ-JDG-001 (Judge) ←┘

Phase 1 Backend:
US-ENG-011 (Engine) ──> US-AUD-081 (SDL3 init) ──> US-AUD-082 (Stability)

US-AUD-091 (Interface) ──> US-AUD-092 (SDL3 impl) ──> US-AUD-093 (Compile-time selection)

Phase 3 Sound Effects:
US-AUD-001 + US-AUD-002 ──> US-AUD-031 (Load SFX) ──> US-AUD-032 (Play SFX) ──┬──> US-AUD-033 (Volume)
                                                                                │
                                     REQ-INP-002 (Input) ──> US-AUD-041 (Key sounds) ──> US-AUD-042 (Per-column)
                                                                                │
                                                  REQ-JDG-001 (Judge) ──> US-AUD-051 (Judgment SFX) ──> US-AUD-052 (Judgment volume)

Phase 5 Calibration:
US-AUD-021 (Judge timing) + REQ-DAT-001 (Profile) ──> US-AUD-061 (Global offset) ──> US-AUD-062 (Persist offset)
                                                                                                │
                   REQ-SCN-002 (Scenes) ──> US-AUD-071 (Calibration screen) ──> US-AUD-072 (Save/cancel)
                                                        ↑
                                      US-AUD-061 ──────┘
```

---

## Cross-Subsystem Dependencies

- **REQ-ENG-011 (Engine class)**: Required for AudioSystem ownership and lifecycle
- **REQ-INP-002 (Input system)**: Required for key sound triggering on panel press
- **REQ-JDG-001 (Judge)**: Required for judgment sound triggering and audio-based timing
- **REQ-DAT-001 (Profile system)**: Required for persisting audio offset and volume settings
- **REQ-AST-001 (Asset discovery)**: Required for locating music and SFX files
- **REQ-SCN-002 (Scene system)**: Required for calibration screen implementation

---

## Implementation Phases

### Phase 1: Basic Music Playback (16 points)
Stories: US-AUD-001, US-AUD-002, US-AUD-003, US-AUD-004, US-AUD-011, US-AUD-081, US-AUD-091, US-AUD-092

**Exit Criteria**: Engine loads OGG/MP3 files, plays/pauses/seeks music, reports sample-accurate position, judge queries position each tick.

### Phase 3: Sound Effects (21 points)
Stories: US-AUD-031, US-AUD-032, US-AUD-033, US-AUD-041, US-AUD-042, US-AUD-051, US-AUD-052

**Exit Criteria**: Key sounds trigger on panel press, judgment sounds play on note hits, SFX volume is independently controlled, 10+ concurrent sounds play without artifacts.

### Phase 5: Calibration (14 points)
Stories: US-AUD-012, US-AUD-021, US-AUD-061, US-AUD-062, US-AUD-071, US-AUD-072, US-AUD-082

**Exit Criteria**: Calibration screen helps users determine offset, offset persists in profile, judge timing compensates for hardware latency, audio stable over multi-hour sessions.

### Future: Backend Flexibility (2 points)
Stories: US-AUD-093

**Exit Criteria**: Compile-time backend selection works without code changes.

---

## Notes

**Music vs SFX separation**: Music uses streaming (disk → decode → buffer → hardware), SFX uses memory-resident PCM (load to memory → mix → hardware). This architectural boundary is critical for meeting latency requirements.

**Sample-accurate position**: The authoritative timing design (REQ-AUD-002, REQ-AUD-003) eliminates an entire class of rhythm game bugs where audio and judgments drift over time. This is the foundation for judge fairness.

**Testability without hardware**: All stories require injectable mock backends or timestamp sources for CI/CD testing. True end-to-end latency validation (NFR tests) requires physical hardware measurement.

**Backend swapping rationale**: SDL3 is the Phase 1 choice for simplicity and existing project dependency. If latency proves insufficient in practice testing, SoLoud or miniaudio can be swapped in Phase 6+ without changing judge, scene, or input code.

---

*Generated from docs/requirements/03-audio-system.md*
*Last updated: 2026-04-26*
