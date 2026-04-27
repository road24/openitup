# Data Management User Stories

This document decomposes the data management requirements (REQ-DAT-001 through REQ-DAT-013) into developer-ready user stories. Stories are organized by epic area and include dependencies, acceptance criteria, and estimation guidance.

---

## Epic: User Data Persistence

Stories related to storing and retrieving player profiles, settings, and persistent data on disk.

---

### Story ID: US-DAT-001 - Platform-Appropriate User Data Directory Location

**Story Card:**
> **As a** Developer
> **I want** automatic resolution of the user data directory path for the target platform
> **So that** player profiles and settings are stored in the correct location without requiring manual configuration

#### 📝 Description
The engine must determine the appropriate user data directory based on the operating system at runtime. On Linux this is `~/.local/share/openitup/`, on Windows it is `%APPDATA%/openitup/`. The path must be accessible for read and write operations.

#### ✅ Acceptance Criteria

*   **Scenario 1: Resolve Linux user data path**
    *   **Given** the engine is running on a Linux system
    *   **When** the engine queries the user data directory path
    *   **Then** the path returned is `~/.local/share/openitup/` with the tilde expanded to the user's home directory

*   **Scenario 2: Resolve Windows user data path**
    *   **Given** the engine is running on a Windows system
    *   **When** the engine queries the user data directory path
    *   **Then** the path returned is `%APPDATA%/openitup/` with the environment variable expanded

*   **Scenario 3: Path is absolute and valid**
    *   **Given** the engine has resolved the user data directory path
    *   **When** the path is returned to calling code
    *   **Then** the path is an absolute filesystem path with no environment variables or tilde prefixes remaining

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: None
*   **Implementation Note**: Use SDL3's `SDL_GetPrefPath()` or platform-specific APIs. Must handle home directory expansion.

---

### Story ID: US-DAT-002 - Automatic User Data Directory Creation

**Story Card:**
> **As a** Player
> **I want** the engine to create required directories on first launch
> **So that** I do not need to manually set up the file structure before using the engine

#### 📝 Description
On first run, the engine must create the user data directory and its subdirectories (`profiles/`, `cache/`) if they do not exist. Directory creation must set appropriate filesystem permissions.

#### ✅ Acceptance Criteria

*   **Scenario 1: Create missing user data directory**
    *   **Given** the user data directory does not exist
    *   **When** the engine initializes at startup
    *   **Then** the directory `~/.local/share/openitup/` (or platform equivalent) is created with read/write permissions for the current user

*   **Scenario 2: Create profiles subdirectory**
    *   **Given** the user data directory exists but `profiles/` does not
    *   **When** the engine initializes at startup
    *   **Then** the subdirectory `profiles/` is created inside the user data directory

*   **Scenario 3: Create cache subdirectory**
    *   **Given** the user data directory exists but `cache/` does not
    *   **When** the engine initializes at startup
    *   **Then** the subdirectory `cache/` is created inside the user data directory

*   **Scenario 4: Read-only filesystem error**
    *   **Given** the user data directory location is on a read-only filesystem
    *   **When** the engine attempts to create the directory
    *   **Then** an error is logged at ERROR level stating directory creation failed, and the engine continues with in-memory-only operation

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-001
*   **Implementation Note**: Use `std::filesystem::create_directories()` with error handling

---

### Story ID: US-DAT-003 - Settings JSON File Structure

**Story Card:**
> **As a** Developer
> **I want** a defined JSON schema for the settings file
> **So that** settings can be loaded, validated, and saved consistently

#### 📝 Description
The engine stores global settings in `settings.json` located in the user data directory. The schema includes video resolution (width, height), audio device identifier, key bindings (map from PadInput enum to SDL keycodes), and global audio offset in milliseconds.

#### ✅ Acceptance Criteria

*   **Scenario 1: Valid settings file with all fields**
    *   **Given** a `settings.json` file contains `{"video": {"width": 1920, "height": 1080}, "audio": {"device": "default", "global_offset_ms": 0}, "input": {"P1_DOWN_LEFT": "SDLK_z"}}`
    *   **When** the settings file is parsed
    *   **Then** all fields are loaded into the engine's settings object with the specified values

*   **Scenario 2: Settings file missing optional field**
    *   **Given** a `settings.json` file contains video and audio sections but no input section
    *   **When** the settings file is parsed
    *   **Then** the missing input section is populated with default key bindings

*   **Scenario 3: Invalid JSON syntax**
    *   **Given** a `settings.json` file contains malformed JSON (e.g., trailing comma, missing brace)
    *   **When** the settings file is parsed
    *   **Then** an error is logged at ERROR level stating "Failed to parse settings.json: [parse error]" and all settings use default values

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-DAT-002
*   **Schema Definition**: Must document the exact field names, types, and constraints in a separate spec file or inline comments

---

### Story ID: US-DAT-004 - Settings Load at Startup

**Story Card:**
> **As a** Player
> **I want** my saved settings to be applied when the engine starts
> **So that** I do not need to reconfigure video, audio, and input each time I play

#### 📝 Description
During engine initialization, the settings file is read from disk and applied. If the file does not exist, the engine creates it with default values.

#### ✅ Acceptance Criteria

*   **Scenario 1: Load existing valid settings**
    *   **Given** a valid `settings.json` exists in the user data directory
    *   **When** the engine initializes
    *   **Then** the video resolution, audio device, key bindings, and global offset from the file are applied to the engine's runtime configuration

*   **Scenario 2: Settings file does not exist**
    *   **Given** no `settings.json` exists in the user data directory
    *   **When** the engine initializes
    *   **Then** a new `settings.json` file is created with default values (1920x1080, default audio device, standard keyboard layout, 0ms offset)

*   **Scenario 3: Logging confirmation**
    *   **Given** the settings file is successfully loaded
    *   **When** the engine initializes
    *   **Then** a log message at INFO level states "Loaded settings from [path]"

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-003

---

### Story ID: US-DAT-005 - Settings Save on Change

**Story Card:**
> **As a** Player
> **I want** settings changes to be saved immediately
> **So that** my configuration is not lost if the engine crashes or I exit unexpectedly

#### 📝 Description
Whenever a setting is modified through the UI or API, the change is written to disk immediately rather than waiting for the engine to exit.

#### ✅ Acceptance Criteria

*   **Scenario 1: Key binding change triggers save**
    *   **Given** the player modifies a key binding in the settings menu
    *   **When** the change is confirmed
    *   **Then** the `settings.json` file is updated on disk within 100 milliseconds

*   **Scenario 2: Global offset change triggers save**
    *   **Given** the player adjusts the global audio offset in the calibration screen
    *   **When** the adjustment is applied
    *   **Then** the `settings.json` file is updated with the new offset value

*   **Scenario 3: Write failure is logged**
    *   **Given** the `settings.json` file is read-only or the disk is full
    *   **When** the engine attempts to save settings
    *   **Then** an error is logged at ERROR level stating "Failed to save settings.json: [error]" and the in-memory settings remain updated

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-003, US-DAT-004

---

### Story ID: US-DAT-006 - Atomic File Write for Settings

**Story Card:**
> **As a** Developer
> **I want** atomic file writes for the settings file
> **So that** a crash or power loss during save does not corrupt the file

#### 📝 Description
When saving settings, write to a temporary file first, then atomically rename it to `settings.json`. This ensures the file is never in a half-written state.

#### ✅ Acceptance Criteria

*   **Scenario 1: Write to temporary file**
    *   **Given** the engine is saving settings
    *   **When** the write operation begins
    *   **Then** the data is written to a temporary file named `settings.json.tmp` in the same directory

*   **Scenario 2: Atomic rename on success**
    *   **Given** the temporary file write completes successfully
    *   **When** the write operation finishes
    *   **Then** the temporary file is atomically renamed to `settings.json`, replacing any existing file

*   **Scenario 3: Original preserved on failure**
    *   **Given** the write to the temporary file fails (e.g., disk full)
    *   **When** the write operation encounters an error
    *   **Then** the temporary file is deleted if it exists, and the original `settings.json` remains unchanged

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-005
*   **Implementation Note**: Use `std::filesystem::rename()` for atomic operation

---

### Story ID: US-DAT-007 - Settings Value Validation

**Story Card:**
> **As a** Developer
> **I want** settings values to be validated on load
> **So that** invalid or out-of-range values do not cause runtime errors

#### 📝 Description
When loading settings, each field is validated against its expected type and range. Invalid values are replaced with defaults and a warning is logged.

#### ✅ Acceptance Criteria

*   **Scenario 1: Resolution validated against supported values**
    *   **Given** the settings file specifies a video resolution of 1x1 pixels
    *   **When** the settings are loaded
    *   **Then** the resolution is rejected, replaced with 1920x1080, and a warning is logged at WARN level

*   **Scenario 2: Offset clamped to valid range**
    *   **Given** the settings file specifies a global offset of -10000 milliseconds
    *   **When** the settings are loaded
    *   **Then** the offset is clamped to -500 milliseconds and a warning is logged stating "Global offset out of range, clamped to -500ms"

*   **Scenario 3: Key binding duplicate detection**
    *   **Given** the settings file assigns the same SDL keycode to two different PadInput values
    *   **When** the settings are loaded
    *   **Then** the duplicate is rejected, the second assignment uses a default key, and a warning is logged

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-DAT-004
*   **Validation Ranges**: Resolution: minimum 640x480, maximum 7680x4320; Offset: -500ms to +500ms

---

### Story ID: US-DAT-008 - Profile JSON File Structure

**Story Card:**
> **As a** Developer
> **I want** a defined JSON schema for player profile files
> **So that** profile data can be loaded, validated, and saved consistently

#### 📝 Description
Each profile is stored as a separate JSON file in `profiles/[profile_name].json`. The schema includes display name, preferred speed mod (type and value), note skin directory name, input calibration offset, audio calibration offset, play statistics, and high scores keyed by chart content hash.

#### ✅ Acceptance Criteria

*   **Scenario 1: Valid profile with all required fields**
    *   **Given** a profile file contains `{"display_name": "Player1", "speed_mod": {"type": "C", "value": 450}, "note_skin": "default", "input_offset_ms": 0, "audio_offset_ms": 0, "statistics": {"songs_played": 0, "total_time_hours": 0.0, "total_score": 0}, "high_scores": {}}`
    *   **When** the profile file is parsed
    *   **Then** all fields are loaded into a Profile object with the specified values

*   **Scenario 2: High score entry structure**
    *   **Given** a profile file contains a high score entry `"abc123...": [{"score": 950000, "grade": "S", "max_combo": 500, "judgments": {"perfect": 480, "great": 20, "good": 0, "bad": 0, "miss": 0}, "date": "2026-04-15T10:30:00Z", "judge_profile": "exceed"}]`
    *   **When** the profile is loaded
    *   **Then** the high score is accessible by chart hash `abc123...` with all judgment counts and metadata intact

*   **Scenario 3: Invalid display name**
    *   **Given** a profile file contains `"display_name": "AB"` (2 characters, below minimum)
    *   **When** the profile file is parsed
    *   **Then** the display name is rejected and replaced with "Player", and a warning is logged

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-DAT-002
*   **Display Name Constraints**: 3-20 characters, UTF-8 encoded

---

### Story ID: US-DAT-009 - Default Profile Creation

**Story Card:**
> **As a** Player
> **I want** a default profile to be created on first launch
> **So that** I can start playing immediately without setting up a profile

#### 📝 Description
If no profiles exist in the `profiles/` directory, the engine creates a default profile named "default.json" with standard initial values.

#### ✅ Acceptance Criteria

*   **Scenario 1: No profiles exist**
    *   **Given** the `profiles/` directory is empty
    *   **When** the engine initializes
    *   **Then** a file `profiles/default.json` is created with display name "Player", speed mod M3.0, default note skin, zero offsets, and empty statistics

*   **Scenario 2: Default profile loaded automatically**
    *   **Given** the default profile was just created
    *   **When** the engine finishes initialization
    *   **Then** the default profile is set as the active profile for the session

*   **Scenario 3: Logging confirmation**
    *   **Given** the default profile is created
    *   **When** the creation completes
    *   **Then** a log message at INFO level states "Created default profile at [path]"

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-008

---

### Story ID: US-DAT-010 - Profile Load and Activation

**Story Card:**
> **As a** Player
> **I want** a specific profile loaded at startup
> **So that** my personal settings and high scores are available during gameplay

#### 📝 Description
The engine loads the profile that was active in the previous session. If no previous session exists, the default profile is loaded.

#### ✅ Acceptance Criteria

*   **Scenario 1: Load previously active profile**
    *   **Given** the file `profiles/alice.json` was the active profile when the engine last exited
    *   **When** the engine starts
    *   **Then** the profile "alice" is loaded and set as the active profile

*   **Scenario 2: Profile file missing**
    *   **Given** the previously active profile was "bob.json" but that file no longer exists
    *   **When** the engine starts
    *   **Then** the default profile is loaded instead, and a warning is logged stating "Previously active profile not found, using default"

*   **Scenario 3: Profile load triggers statistics initialization**
    *   **Given** a profile is loaded successfully
    *   **When** the profile is set as active
    *   **Then** the engine's internal statistics counters (songs played, total time, etc.) are initialized from the profile data

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-008, US-DAT-009

---

### Story ID: US-DAT-011 - Profile Save After Gameplay

**Story Card:**
> **As a** Player
> **I want** my profile to be saved after each song
> **So that** high scores and play statistics are not lost if the engine crashes

#### 📝 Description
After gameplay concludes (whether completed, failed, or manually exited), the active profile is written to disk with updated statistics and high scores.

#### ✅ Acceptance Criteria

*   **Scenario 1: Save after song completion**
    *   **Given** the player completes a song and views the result screen
    *   **When** the result screen exits
    *   **Then** the active profile is saved to disk with incremented songs played count and updated statistics

*   **Scenario 2: High score updates trigger save**
    *   **Given** the player achieves a new high score for a chart
    *   **When** the gameplay session ends
    *   **Then** the profile file is updated with the new high score entry keyed by chart content hash

*   **Scenario 3: Save on gameplay exit**
    *   **Given** the player exits gameplay mid-song by pressing the back button
    *   **When** the gameplay scene is destroyed
    *   **Then** the profile is saved with updated play time statistics (even though no high score is recorded)

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-010

---

### Story ID: US-DAT-012 - Atomic File Write for Profiles

**Story Card:**
> **As a** Developer
> **I want** atomic file writes for profile files
> **So that** a crash during save does not corrupt the profile

#### 📝 Description
When saving a profile, write to a temporary file first, then atomically rename it to the profile filename. This ensures the file is never in a half-written state.

#### ✅ Acceptance Criteria

*   **Scenario 1: Write to temporary file**
    *   **Given** the engine is saving profile "alice.json"
    *   **When** the write operation begins
    *   **Then** the data is written to a temporary file named `alice.json.tmp` in the same directory

*   **Scenario 2: Atomic rename on success**
    *   **Given** the temporary file write completes successfully
    *   **When** the write operation finishes
    *   **Then** the temporary file is atomically renamed to `alice.json`, replacing the existing profile file

*   **Scenario 3: Original preserved on failure**
    *   **Given** the write to the temporary file fails (e.g., disk full)
    *   **When** the write operation encounters an error
    *   **Then** the temporary file is deleted if it exists, and the original `alice.json` remains unchanged

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-011
*   **Implementation Note**: Reuse the atomic write implementation from US-DAT-006

---

### Story ID: US-DAT-013 - Profile Service Object API

**Story Card:**
> **As a** Developer
> **I want** a service object for accessing profile data
> **So that** C++ screens and Lua scripts can load, save, and query profile information through a single interface

#### 📝 Description
The `ProfileService` class is owned by the `Engine` and provides APIs for loading/saving profiles, querying high scores, updating statistics, and accessing player settings.

#### ✅ Acceptance Criteria

*   **Scenario 1: Get active profile**
    *   **Given** a profile is loaded and active
    *   **When** a screen calls `profile_service.get_active_profile()`
    *   **Then** a const reference to the Profile object is returned

*   **Scenario 2: Update play statistics**
    *   **Given** a gameplay session completes
    *   **When** the gameplay scene calls `profile_service.record_play_session(seconds_played, score, judgments)`
    *   **Then** the active profile's statistics are updated in memory and the profile is saved to disk

*   **Scenario 3: Query high score by chart hash**
    *   **Given** a chart has a content hash of "abc123..."
    *   **When** a screen calls `profile_service.get_high_scores("abc123...")`
    *   **Then** a vector of HighScore entries sorted by score (descending) is returned

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5
*   **Dependencies**: US-DAT-010, US-DAT-011
*   **Thread Safety**: Does not need to be thread-safe in Phase 3 (single-threaded game loop only)

---

## Epic: High Score Tracking

Stories related to recording, storing, and querying per-chart high scores.

---

### Story ID: US-DAT-014 - Chart Content Hash Computation

**Story Card:**
> **As a** Developer
> **I want** a deterministic content hash for each chart
> **So that** high scores are keyed to the chart's actual content rather than its filename or metadata

#### 📝 Description
Each chart is identified by a SHA-256 hash of its note data and timing data in a canonical binary representation. Metadata fields (title, artist, banner path) are excluded. The hash is computed once at chart load time and cached.

#### ✅ Acceptance Criteria

*   **Scenario 1: Same chart in different formats produces same hash**
    *   **Given** a chart is parsed from a KSF file with note data [tap at beat 1, tap at beat 2] and BPM 130
    *   **And** the same chart is parsed from an SSC file with identical note data and BPM
    *   **When** the content hash is computed for both
    *   **Then** the resulting SHA-256 hashes are identical

*   **Scenario 2: Metadata change does not affect hash**
    *   **Given** a chart has title "Song A" and note data [tap at beat 1]
    *   **When** the title is changed to "Song B" but note data remains the same
    *   **Then** the content hash remains unchanged

*   **Scenario 3: Note data change affects hash**
    *   **Given** a chart has note data [tap at beat 1, tap at beat 2]
    *   **When** a third tap is added at beat 3
    *   **Then** the content hash changes to a different SHA-256 value

*   **Scenario 4: Hash computed once and cached**
    *   **Given** a chart is loaded from disk
    *   **When** the chart object is accessed 100 times during gameplay
    *   **Then** the hash computation occurs exactly once during the initial load

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5
*   **Dependencies**: REQ-CHT-010 (chart system must be implemented)
*   **Canonical Form**: Must document the exact byte layout used for hashing

---

### Story ID: US-DAT-015 - Record High Score After Gameplay

**Story Card:**
> **As a** Player
> **I want** my score to be saved if it is a new personal best
> **So that** I can track my progress over time

#### 📝 Description
After a song is completed (not failed, not exited early), the final score is compared to existing high scores for that chart. If it qualifies, it is inserted into the profile's high score list for that chart.

#### ✅ Acceptance Criteria

*   **Scenario 1: New high score in empty list**
    *   **Given** the player completes a chart for the first time with a score of 850000
    *   **When** the gameplay session ends
    *   **Then** the high score entry is recorded in the profile under the chart's content hash with score 850000, grade, combo, and judgment counts

*   **Scenario 2: Score higher than existing best**
    *   **Given** the player's previous high score for a chart is 900000
    *   **When** the player completes the chart with a score of 950000
    *   **Then** the new score is inserted as the top entry, and the list is sorted by score descending

*   **Scenario 3: Score lower than existing best**
    *   **Given** the player's existing high score for a chart is 950000
    *   **When** the player completes the chart with a score of 800000
    *   **Then** the score is still recorded (if the list has fewer than 10 entries), but does not replace the top score

*   **Scenario 4: Failed song does not record score**
    *   **Given** the player's life gauge reaches zero and gameplay ends in failure
    *   **When** the result screen is displayed
    *   **Then** no high score entry is created for that play

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-013, US-DAT-014
*   **List Size**: Store up to 10 high scores per chart

---

### Story ID: US-DAT-016 - High Score Entry Metadata

**Story Card:**
> **As a** Player
> **I want** high scores to include judgment counts and date achieved
> **So that** I can review the details of my best performances

#### 📝 Description
Each high score entry includes score, grade, max combo, judgment counts (perfect/great/good/bad/miss), date achieved (ISO 8601 timestamp), and the judge profile used (e.g., "exceed", "nx2").

#### ✅ Acceptance Criteria

*   **Scenario 1: All judgment counts recorded**
    *   **Given** a player completes a chart with 480 perfects, 20 greats, 0 goods, 0 bads, and 0 misses
    *   **When** the high score is saved
    *   **Then** the profile file contains an entry with `"judgments": {"perfect": 480, "great": 20, "good": 0, "bad": 0, "miss": 0}`

*   **Scenario 2: Date recorded in ISO 8601 format**
    *   **Given** a player achieves a high score on April 26, 2026 at 3:45 PM UTC
    *   **When** the high score is saved
    *   **Then** the entry contains `"date": "2026-04-26T15:45:00Z"`

*   **Scenario 3: Judge profile identifier recorded**
    *   **Given** the player completed the chart using the "nx2" judge profile
    *   **When** the high score is saved
    *   **Then** the entry contains `"judge_profile": "nx2"`

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1
*   **Dependencies**: US-DAT-015

---

### Story ID: US-DAT-017 - Query Top High Scores for Chart

**Story Card:**
> **As a** Developer
> **I want** an API to retrieve the top N high scores for a given chart
> **So that** screens can display personal best information

#### 📝 Description
The `ProfileService` provides a query method that returns a sorted list of high score entries for a specified chart hash. Results are sorted by score descending.

#### ✅ Acceptance Criteria

*   **Scenario 1: Query returns sorted scores**
    *   **Given** a chart has three high score entries with scores 950000, 920000, and 880000
    *   **When** the query `get_high_scores(chart_hash, limit=3)` is called
    *   **Then** the result is a vector of three entries ordered [950000, 920000, 880000]

*   **Scenario 2: Query with limit smaller than available scores**
    *   **Given** a chart has 10 recorded high scores
    *   **When** the query `get_high_scores(chart_hash, limit=5)` is called
    *   **Then** exactly 5 entries are returned, sorted by score descending

*   **Scenario 3: Query for chart with no scores**
    *   **Given** a chart has never been played
    *   **When** the query `get_high_scores(chart_hash, limit=10)` is called
    *   **Then** an empty vector is returned

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1
*   **Dependencies**: US-DAT-015

---

## Epic: Play Statistics

Stories related to tracking and displaying aggregate play statistics.

---

### Story ID: US-DAT-018 - Track Total Songs Played

**Story Card:**
> **As a** Player
> **I want** the engine to count how many songs I have played
> **So that** I can see my overall play activity

#### 📝 Description
The profile maintains a counter for total songs played. This counter increments by 1 each time a song is completed (passed or failed, but not exited early).

#### ✅ Acceptance Criteria

*   **Scenario 1: Counter increments on song completion**
    *   **Given** the profile's songs played counter is at 42
    *   **When** the player completes a song and views the result screen
    *   **Then** the counter increments to 43

*   **Scenario 2: Counter does not increment on early exit**
    *   **Given** the player starts a song and exits gameplay by pressing the back button before the song ends
    *   **When** the gameplay scene is destroyed
    *   **Then** the songs played counter remains unchanged

*   **Scenario 3: Counter persists across sessions**
    *   **Given** the profile's songs played counter is 100 when the engine exits
    *   **When** the engine is restarted and the profile is loaded
    *   **Then** the songs played counter is still 100

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1
*   **Dependencies**: US-DAT-008, US-DAT-011

---

### Story ID: US-DAT-019 - Track Total Play Time

**Story Card:**
> **As a** Player
> **I want** the engine to track how much time I have spent playing
> **So that** I can see my overall playtime

#### 📝 Description
The profile maintains a counter for total play time in hours (stored as a floating-point value). This counter accumulates the duration of each completed gameplay session, measured from song start to song end.

#### ✅ Acceptance Criteria

*   **Scenario 1: Playtime increments by song duration**
    *   **Given** the profile's total playtime is 10.5 hours
    *   **And** the player completes a song lasting 1 minute 30 seconds (0.025 hours)
    *   **When** the gameplay session ends
    *   **Then** the total playtime is updated to 10.525 hours

*   **Scenario 2: Playtime includes failed songs**
    *   **Given** the player starts a song and fails at the 45-second mark
    *   **When** the result screen is displayed
    *   **Then** the total playtime is incremented by 0.0125 hours (45 seconds)

*   **Scenario 3: Early exit does not count toward playtime**
    *   **Given** the player exits gameplay after 10 seconds
    *   **When** the gameplay scene is destroyed
    *   **Then** the total playtime counter remains unchanged

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1
*   **Dependencies**: US-DAT-008, US-DAT-011

---

### Story ID: US-DAT-020 - Track Total Score Accumulated

**Story Card:**
> **As a** Player
> **I want** the engine to sum all scores I have earned
> **So that** I can see my lifetime total score

#### 📝 Description
The profile maintains a counter for total score accumulated across all plays. This counter adds the final score from each completed gameplay session, regardless of whether it was a new high score.

#### ✅ Acceptance Criteria

*   **Scenario 1: Score accumulated on completion**
    *   **Given** the profile's total score is 5000000
    *   **When** the player completes a song with a score of 920000
    *   **Then** the total score is updated to 5920000

*   **Scenario 2: Low scores still accumulate**
    *   **Given** the player completes a song with a failing score of 200000
    *   **When** the gameplay session ends
    *   **Then** the total score counter is incremented by 200000

*   **Scenario 3: Early exit does not accumulate score**
    *   **Given** the player exits gameplay before the song ends
    *   **When** the gameplay scene is destroyed
    *   **Then** the total score counter remains unchanged

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1
*   **Dependencies**: US-DAT-008, US-DAT-011

---

### Story ID: US-DAT-021 - Track Judgment Distribution

**Story Card:**
> **As a** Player
> **I want** the engine to count my lifetime perfect/great/good/bad/miss judgments
> **So that** I can see my overall accuracy trends

#### 📝 Description
The profile maintains cumulative counters for each judgment type (perfect, great, good, bad, miss). After each completed song, the session's judgment counts are added to these lifetime totals.

#### ✅ Acceptance Criteria

*   **Scenario 1: Judgment counts added after song**
    *   **Given** the profile's lifetime judgments are {perfect: 10000, great: 500, good: 50, bad: 10, miss: 5}
    *   **And** the player completes a song with {perfect: 480, great: 20, good: 0, bad: 0, miss: 0}
    *   **When** the gameplay session ends
    *   **Then** the profile's lifetime judgments are updated to {perfect: 10480, great: 520, good: 50, bad: 10, miss: 5}

*   **Scenario 2: Failed song still adds judgments**
    *   **Given** the player fails a song mid-way with judgments {perfect: 100, great: 10, good: 5, bad: 8, miss: 12}
    *   **When** the result screen is displayed
    *   **Then** the profile's lifetime judgment counters are incremented by those values

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1
*   **Dependencies**: US-DAT-008, US-DAT-011

---

### Story ID: US-DAT-022 - Calculate Average Accuracy

**Story Card:**
> **As a** Player
> **I want** the engine to display my average accuracy percentage
> **So that** I can see a single metric representing my overall performance

#### 📝 Description
Average accuracy is computed from the lifetime judgment distribution. This is a derived statistic, not stored separately.

#### ✅ Acceptance Criteria

*   **Scenario 1: Accuracy computed from lifetime judgments**
    *   **Given** the profile's lifetime judgments are {perfect: 9500, great: 400, good: 80, bad: 15, miss: 5}
    *   **When** the profile screen queries the average accuracy
    *   **Then** the accuracy is displayed as a percentage value based on the weighted judgment counts

*   **Scenario 2: New profile with zero plays**
    *   **Given** a profile has never completed a song (all judgment counts are zero)
    *   **When** the profile screen queries the average accuracy
    *   **Then** the result is 0.0% (or displayed as "N/A")

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1
*   **Dependencies**: US-DAT-021

---

## Epic: Profile Management UI

Stories related to creating, selecting, deleting, and switching between player profiles.

---

### Story ID: US-DAT-023 - Profile Selection at Startup

**Story Card:**
> **As a** Player
> **I want** a menu to select which profile to use
> **So that** multiple people can maintain separate high scores and settings on the same system

#### 📝 Description
On startup (or from a settings menu), a profile selection screen displays a list of available profiles. The player can choose one to activate for the session.

#### ✅ Acceptance Criteria

*   **Scenario 1: Display list of profiles**
    *   **Given** the `profiles/` directory contains files `alice.json`, `bob.json`, and `charlie.json`
    *   **When** the profile selection screen is displayed
    *   **Then** the screen shows three entries labeled "alice", "bob", and "charlie"

*   **Scenario 2: Select a profile**
    *   **Given** the profile selection screen is displayed
    *   **When** the player highlights "bob" and presses the confirm button
    *   **Then** the profile "bob" is loaded and set as the active profile for the session

*   **Scenario 3: Previously active profile is highlighted by default**
    *   **Given** the last active profile was "alice"
    *   **When** the profile selection screen is displayed
    *   **Then** the entry "alice" is highlighted by default

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5
*   **Dependencies**: US-DAT-010, REQ-SCN-013 (scene system must exist)

---

### Story ID: US-DAT-024 - Create New Profile

**Story Card:**
> **As a** Player
> **I want** a way to create a new profile
> **So that** I can maintain separate statistics and high scores from other players

#### 📝 Description
From the profile selection screen, the player can choose "Create New Profile", enter a display name, and confirm. A new profile file is created and set as the active profile.

#### ✅ Acceptance Criteria

*   **Scenario 1: Create profile with valid name**
    *   **Given** the player selects "Create New Profile" from the profile selection screen
    *   **And** enters the display name "NewPlayer"
    *   **When** the player confirms the entry
    *   **Then** a file `profiles/newplayer.json` is created with default values and display name "NewPlayer", and the profile is set as active

*   **Scenario 2: Reject duplicate profile name**
    *   **Given** a profile named "alice" already exists
    *   **When** the player attempts to create a new profile with the display name "alice"
    *   **Then** an error message is displayed stating "A profile with that name already exists" and the profile is not created

*   **Scenario 3: Reject invalid display name**
    *   **Given** the player enters a display name "XY" (2 characters, below the 3-character minimum)
    *   **When** the player attempts to confirm
    *   **Then** an error message is displayed stating "Display name must be 3-20 characters" and the profile is not created

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-DAT-023

---

### Story ID: US-DAT-025 - Delete Profile with Confirmation

**Story Card:**
> **As a** Player
> **I want** a way to delete a profile
> **So that** I can remove unused profiles from the system

#### 📝 Description
From the profile selection screen, the player can select a profile and choose "Delete Profile". A confirmation dialog is displayed. If confirmed, the profile file is permanently deleted.

#### ✅ Acceptance Criteria

*   **Scenario 1: Delete profile after confirmation**
    *   **Given** the profile "bob" is selected on the profile selection screen
    *   **When** the player chooses "Delete Profile" and confirms the deletion
    *   **Then** the file `profiles/bob.json` is removed from disk and no longer appears in the profile list

*   **Scenario 2: Cancel deletion**
    *   **Given** the player chooses "Delete Profile" for profile "alice"
    *   **When** the player selects "Cancel" on the confirmation dialog
    *   **Then** the file `profiles/alice.json` remains on disk and the profile list is unchanged

*   **Scenario 3: Cannot delete the last remaining profile**
    *   **Given** only one profile exists ("default")
    *   **When** the player attempts to delete it
    *   **Then** an error message is displayed stating "Cannot delete the last profile" and the deletion is blocked

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-023

---

### Story ID: US-DAT-026 - Switch Active Profile Without Losing Data

**Story Card:**
> **As a** Player
> **I want** to switch between profiles during a session
> **So that** multiple players can take turns without restarting the engine

#### 📝 Description
When the active profile is changed, the current profile is saved to disk, the new profile is loaded from disk, and all engine state (settings, high scores) is updated to reflect the new profile.

#### ✅ Acceptance Criteria

*   **Scenario 1: Save current profile before switch**
    *   **Given** the active profile is "alice" with unsaved changes (e.g., a recently completed song updated statistics)
    *   **When** the player switches to profile "bob"
    *   **Then** the file `profiles/alice.json` is updated on disk before "bob" is loaded

*   **Scenario 2: Load new profile settings**
    *   **Given** profile "bob" has a preferred speed mod of C450 and note skin "retro"
    *   **When** the active profile is switched to "bob"
    *   **Then** the gameplay configuration is updated to use C450 speed mod and the "retro" note skin

*   **Scenario 3: High scores reflect active profile**
    *   **Given** profile "alice" has a high score of 950000 on chart "xyz123"
    *   **And** profile "bob" has never played chart "xyz123"
    *   **When** the active profile is switched from "alice" to "bob"
    *   **Then** querying high scores for chart "xyz123" returns an empty list

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-023, US-DAT-011

---

### Story ID: US-DAT-027 - Persist Active Profile Across Sessions

**Story Card:**
> **As a** Player
> **I want** the engine to remember which profile I was using
> **So that** I do not need to select it again every time I launch the engine

#### 📝 Description
The last active profile name is stored in the settings file. When the engine starts, it loads the profile specified in settings.

#### ✅ Acceptance Criteria

*   **Scenario 1: Save active profile name on exit**
    *   **Given** the active profile is "alice" when the engine exits
    *   **When** the engine shuts down
    *   **Then** the `settings.json` file is updated with `"active_profile": "alice"`

*   **Scenario 2: Load active profile from settings on startup**
    *   **Given** the `settings.json` file contains `"active_profile": "bob"`
    *   **When** the engine starts
    *   **Then** the profile "bob" is loaded automatically

*   **Scenario 3: Fallback to default if active profile missing**
    *   **Given** the `settings.json` file specifies `"active_profile": "charlie"`
    *   **And** the file `profiles/charlie.json` does not exist
    *   **When** the engine starts
    *   **Then** the default profile is loaded instead and a warning is logged

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-026, US-DAT-004

---

## Epic: Data Integrity and Migration

Stories related to versioning, migration, and data corruption prevention.

---

### Story ID: US-DAT-028 - Schema Version Number in JSON Files

**Story Card:**
> **As a** Developer
> **I want** a schema version field in all profile and settings files
> **So that** future versions of the engine can detect when migration is needed

#### 📝 Description
All profile and settings JSON files include a top-level `"schema_version"` field (integer). The current schema version is 1.

#### ✅ Acceptance Criteria

*   **Scenario 1: New files include schema version**
    *   **Given** a new profile is created
    *   **When** the profile file is written to disk
    *   **Then** the JSON contains `"schema_version": 1` as the first field

*   **Scenario 2: Loading a file without schema version**
    *   **Given** a profile file from an older version does not contain a `"schema_version"` field
    *   **When** the file is loaded
    *   **Then** the schema version is assumed to be 0 and a warning is logged

*   **Scenario 3: Schema version logged on load**
    *   **Given** a profile file contains `"schema_version": 1`
    *   **When** the file is loaded
    *   **Then** a log message at DEBUG level states "Loaded profile with schema version 1"

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1
*   **Dependencies**: US-DAT-008, US-DAT-003

---

### Story ID: US-DAT-029 - Migration Function for Schema Version 0 to 1

**Story Card:**
> **As a** Developer
> **I want** a migration function that updates old profile files to the current schema
> **So that** profiles created in earlier engine versions remain usable

#### 📝 Description
When a profile with schema version 0 (or missing schema version) is loaded, a migration function is applied that adds any missing fields with default values and updates the schema version to 1.

#### ✅ Acceptance Criteria

*   **Scenario 1: Migrate version 0 profile**
    *   **Given** a profile file with no `"schema_version"` field and missing the `"statistics"` object
    *   **When** the file is loaded
    *   **Then** the `"statistics"` object is added with zero values, `"schema_version": 1` is added, and the file is saved with the updated schema

*   **Scenario 2: Migration logged**
    *   **Given** a profile file is migrated from version 0 to version 1
    *   **When** the migration completes
    *   **Then** a log message at INFO level states "Migrated profile [name] from schema version 0 to 1"

*   **Scenario 3: Migration failure does not block loading**
    *   **Given** a profile file has schema version 0 and the migration function encounters an unexpected field type
    *   **When** the migration is attempted
    *   **Then** an error is logged, the unexpected field is left unchanged, and the profile is loaded with partial migration applied

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-DAT-028

---

### Story ID: US-DAT-030 - Preserve Unknown JSON Fields for Forward Compatibility

**Story Card:**
> **As a** Developer
> **I want** unknown fields in JSON files to be preserved when loading and saving
> **So that** profiles can be used across different engine versions without data loss

#### 📝 Description
When parsing a profile or settings file, any JSON fields not recognized by the current schema are retained in memory and written back to disk when the file is saved.

#### ✅ Acceptance Criteria

*   **Scenario 1: Unknown field preserved**
    *   **Given** a profile file contains an unknown field `"future_feature": {"data": 123}`
    *   **When** the profile is loaded, modified, and saved
    *   **Then** the saved file still contains `"future_feature": {"data": 123}`

*   **Scenario 2: Warning logged for unknown fields**
    *   **Given** a profile file contains an unknown field `"experimental_setting"`
    *   **When** the file is loaded
    *   **Then** a log message at WARN level states "Unknown field 'experimental_setting' in profile, preserving for forward compatibility"

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-008
*   **Implementation Note**: Use `nlohmann::json` with custom serialization that preserves unrecognized keys

---

## Epic: Chart Metadata Caching

Stories related to caching song and chart metadata to improve startup performance.

---

### Story ID: US-DAT-031 - Chart Metadata Cache File Structure

**Story Card:**
> **As a** Developer
> **I want** a defined JSON schema for the chart metadata cache
> **So that** the engine can load song lists quickly without re-scanning directories

#### 📝 Description
The cache file (`cache/chart_metadata.json`) stores an array of chart entries. Each entry includes file path, content hash, title, artist, and difficulty information. A top-level `"last_updated"` timestamp tracks the cache age.

#### ✅ Acceptance Criteria

*   **Scenario 1: Valid cache file with entries**
    *   **Given** a cache file contains `{"schema_version": 1, "last_updated": "2026-04-26T10:00:00Z", "charts": [{"path": "/songs/song1/chart.ksf", "hash": "abc123...", "title": "Song 1", "artist": "Artist 1", "difficulties": [{"type": "single", "level": 5}]}]}`
    *   **When** the cache is loaded
    *   **Then** the engine's song database contains one entry with the specified metadata

*   **Scenario 2: Cache with invalid JSON is rebuilt**
    *   **Given** the cache file is corrupted or contains invalid JSON
    *   **When** the engine attempts to load the cache
    *   **Then** the file is deleted, an error is logged, and the song database is rebuilt from disk scan

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5
*   **Dependencies**: US-DAT-002, REQ-AST-003 (asset management system must exist)

---

### Story ID: US-DAT-032 - Cache Invalidation on Directory Modification

**Story Card:**
> **As a** Developer
> **I want** the cache to be invalidated when song directories are modified
> **So that** new or changed songs are detected automatically

#### 📝 Description
When loading the cache, the engine checks the modification time (`mtime`) of the configured song directories. If any directory's `mtime` is newer than the cache's `"last_updated"` timestamp, the cache is invalidated and rebuilt.

#### ✅ Acceptance Criteria

*   **Scenario 1: Cache is fresh**
    *   **Given** the cache was updated at "2026-04-26T10:00:00Z"
    *   **And** the song directory's `mtime` is "2026-04-26T09:00:00Z"
    *   **When** the engine starts
    *   **Then** the cache is used without re-scanning the directory

*   **Scenario 2: Cache is stale**
    *   **Given** the cache was updated at "2026-04-26T10:00:00Z"
    *   **And** the song directory's `mtime` is "2026-04-26T11:00:00Z"
    *   **When** the engine starts
    *   **Then** the cache is invalidated, the directory is re-scanned, and the cache is rebuilt

*   **Scenario 3: Cache invalidation logged**
    *   **Given** the cache is invalidated due to directory modification
    *   **When** the engine starts
    *   **Then** a log message at INFO level states "Chart cache invalidated due to directory modification, rebuilding"

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-031

---

### Story ID: US-DAT-033 - Fast Cache Loading Performance

**Story Card:**
> **As a** Player
> **I want** the engine to start quickly when I have a large song library
> **So that** I can begin playing without waiting for directory scans

#### 📝 Description
The chart metadata cache must load in under 1 second for a library of 1000+ songs. This is verified with a performance benchmark test.

#### ✅ Acceptance Criteria

*   **Scenario 1: Load 1000-entry cache under 1 second**
    *   **Given** a cache file contains 1000 chart entries
    *   **When** the engine loads the cache at startup
    *   **Then** the operation completes in less than 1000 milliseconds

*   **Scenario 2: Cache load time logged**
    *   **Given** the cache is loaded successfully
    *   **When** the load operation completes
    *   **Then** a log message at DEBUG level states "Loaded chart cache with [N] entries in [X]ms"

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-031
*   **Performance Target**: <1000ms for 1000 entries on HDD, <500ms on SSD

---

## Epic: Backup and Export (Optional - Phase 7)

Stories related to backing up and restoring player data.

---

### Story ID: US-DAT-034 - Export Profile to Single File

**Story Card:**
> **As a** Player
> **I want** a way to export my profile to a single file
> **So that** I can back it up or transfer it to another system

#### 📝 Description
From the profile management screen, the player can select "Export Profile". This creates a standalone JSON file containing all profile data (display name, settings, high scores, statistics) and saves it to a location chosen by the player.

#### ✅ Acceptance Criteria

*   **Scenario 1: Export profile to chosen location**
    *   **Given** the active profile is "alice"
    *   **When** the player selects "Export Profile" and chooses the destination `/home/user/backup/alice_backup.json`
    *   **Then** a file is created at that path containing all data from `profiles/alice.json`

*   **Scenario 2: Export includes all high scores**
    *   **Given** the profile has 50 high score entries across multiple charts
    *   **When** the profile is exported
    *   **Then** all 50 high score entries are included in the export file

*   **Scenario 3: Export confirmation**
    *   **Given** the export completes successfully
    *   **When** the operation finishes
    *   **Then** a message is displayed stating "Profile exported to [path]"

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-008, US-DAT-023
*   **Phase**: 7 (Could Have priority)

---

### Story ID: US-DAT-035 - Import Profile from Single File

**Story Card:**
> **As a** Player
> **I want** a way to import a profile from a backup file
> **So that** I can restore my data after a system reinstall or transfer it from another system

#### 📝 Description
From the profile management screen, the player can select "Import Profile", choose a previously exported JSON file, and load it into the engine. If a profile with the same name exists, the player is prompted to overwrite or rename.

#### ✅ Acceptance Criteria

*   **Scenario 1: Import valid profile**
    *   **Given** the player selects a valid profile export file `/home/user/backup/alice_backup.json`
    *   **When** the import is confirmed
    *   **Then** a new profile file `profiles/alice.json` is created with all data from the backup

*   **Scenario 2: Overwrite existing profile**
    *   **Given** a profile named "alice" already exists
    *   **When** the player imports a backup named "alice" and chooses "Overwrite"
    *   **Then** the existing `profiles/alice.json` is replaced with the imported data

*   **Scenario 3: Rename on conflict**
    *   **Given** a profile named "alice" already exists
    *   **When** the player imports a backup named "alice" and chooses "Rename"
    *   **Then** the imported profile is saved as `profiles/alice_2.json` with display name "alice_2"

*   **Scenario 4: Validation failure**
    *   **Given** the player selects a file that is not a valid profile export (e.g., corrupted JSON)
    *   **When** the import is attempted
    *   **Then** an error message is displayed stating "Failed to import profile: invalid file format" and no profile is created

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-DAT-034
*   **Phase**: 7 (Could Have priority)

---

## Non-Functional Requirements

### Story ID: US-DAT-NFR-001 - Data Corruption Prevention

**Story Card:**
> **As a** Developer
> **I want** atomic file writes for all persistent data
> **So that** the engine can survive crashes and power failures without data loss

#### 📝 Description
All profile and settings saves use atomic write operations (write to temp file, then rename). This applies to US-DAT-006 and US-DAT-012.

#### ✅ Acceptance Criteria

*   **Scenario 1: Crash during write preserves original**
    *   **Given** the engine begins writing a profile file and crashes mid-write
    *   **When** the engine is restarted
    *   **Then** the original profile file is intact and the temporary file (if any) is ignored or cleaned up

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: N/A (implemented in US-DAT-006 and US-DAT-012)
*   **Dependencies**: US-DAT-006, US-DAT-012

---

### Story ID: US-DAT-NFR-002 - Thread Safety for Profile Service

**Story Card:**
> **As a** Developer
> **I want** the profile service to handle concurrent access safely
> **So that** future asynchronous features (e.g., background score submission) do not corrupt profile data

#### 📝 Description
The `ProfileService` is accessed from the main game loop in Phase 3 (single-threaded). In Phase 8, network operations may read profile data from a background thread. The service must be made thread-safe before Phase 8.

#### ✅ Acceptance Criteria

*   **Scenario 1: Concurrent read access**
    *   **Given** a background thread is reading high scores for network submission
    *   **When** the main thread queries the active profile's display name
    *   **Then** both operations complete successfully without data races

*   **Scenario 2: Concurrent write access**
    *   **Given** the main thread is saving the profile after a completed song
    *   **When** a background thread attempts to read the profile for network submission
    *   **Then** the read blocks until the save completes, and the read returns the updated data

#### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-DAT-013
*   **Phase**: Must be completed before Phase 8

---

## Status Summary

| Story ID | Title | Phase | Points | Status | Dependencies |
|----------|-------|-------|--------|--------|--------------|
| US-DAT-001 | Platform-Appropriate User Data Directory Location | 3 | 2 | PLANNED | None |
| US-DAT-002 | Automatic User Data Directory Creation | 3 | 2 | PLANNED | US-DAT-001 |
| US-DAT-003 | Settings JSON File Structure | 3 | 3 | PLANNED | US-DAT-002 |
| US-DAT-004 | Settings Load at Startup | 3 | 2 | PLANNED | US-DAT-003 |
| US-DAT-005 | Settings Save on Change | 3 | 2 | PLANNED | US-DAT-003, US-DAT-004 |
| US-DAT-006 | Atomic File Write for Settings | 3 | 2 | PLANNED | US-DAT-005 |
| US-DAT-007 | Settings Value Validation | 3 | 3 | PLANNED | US-DAT-004 |
| US-DAT-008 | Profile JSON File Structure | 5 | 3 | PLANNED | US-DAT-002 |
| US-DAT-009 | Default Profile Creation | 5 | 2 | PLANNED | US-DAT-008 |
| US-DAT-010 | Profile Load and Activation | 5 | 2 | PLANNED | US-DAT-008, US-DAT-009 |
| US-DAT-011 | Profile Save After Gameplay | 5 | 2 | PLANNED | US-DAT-010 |
| US-DAT-012 | Atomic File Write for Profiles | 5 | 2 | PLANNED | US-DAT-011 |
| US-DAT-013 | Profile Service Object API | 5 | 5 | PLANNED | US-DAT-010, US-DAT-011 |
| US-DAT-014 | Chart Content Hash Computation | 5 | 5 | PLANNED | REQ-CHT-010 |
| US-DAT-015 | Record High Score After Gameplay | 5 | 2 | PLANNED | US-DAT-013, US-DAT-014 |
| US-DAT-016 | High Score Entry Metadata | 5 | 1 | PLANNED | US-DAT-015 |
| US-DAT-017 | Query Top High Scores for Chart | 5 | 1 | PLANNED | US-DAT-015 |
| US-DAT-018 | Track Total Songs Played | 7 | 1 | PLANNED | US-DAT-008, US-DAT-011 |
| US-DAT-019 | Track Total Play Time | 7 | 1 | PLANNED | US-DAT-008, US-DAT-011 |
| US-DAT-020 | Track Total Score Accumulated | 7 | 1 | PLANNED | US-DAT-008, US-DAT-011 |
| US-DAT-021 | Track Judgment Distribution | 7 | 1 | PLANNED | US-DAT-008, US-DAT-011 |
| US-DAT-022 | Calculate Average Accuracy | 7 | 1 | PLANNED | US-DAT-021 |
| US-DAT-023 | Profile Selection at Startup | 5 | 5 | PLANNED | US-DAT-010, REQ-SCN-013 |
| US-DAT-024 | Create New Profile | 5 | 3 | PLANNED | US-DAT-023 |
| US-DAT-025 | Delete Profile with Confirmation | 5 | 2 | PLANNED | US-DAT-023 |
| US-DAT-026 | Switch Active Profile Without Losing Data | 5 | 2 | PLANNED | US-DAT-023, US-DAT-011 |
| US-DAT-027 | Persist Active Profile Across Sessions | 5 | 2 | PLANNED | US-DAT-026, US-DAT-004 |
| US-DAT-028 | Schema Version Number in JSON Files | 5 | 1 | PLANNED | US-DAT-008, US-DAT-003 |
| US-DAT-029 | Migration Function for Schema Version 0 to 1 | 5 | 3 | PLANNED | US-DAT-028 |
| US-DAT-030 | Preserve Unknown JSON Fields for Forward Compatibility | 5 | 2 | PLANNED | US-DAT-008 |
| US-DAT-031 | Chart Metadata Cache File Structure | 3 | 5 | PLANNED | US-DAT-002, REQ-AST-003 |
| US-DAT-032 | Cache Invalidation on Directory Modification | 3 | 2 | PLANNED | US-DAT-031 |
| US-DAT-033 | Fast Cache Loading Performance | 3 | 2 | PLANNED | US-DAT-031 |
| US-DAT-034 | Export Profile to Single File | 7 | 2 | FUTURE | US-DAT-008, US-DAT-023 |
| US-DAT-035 | Import Profile from Single File | 7 | 3 | FUTURE | US-DAT-034 |
| US-DAT-NFR-001 | Data Corruption Prevention | 3-5 | N/A | PLANNED | US-DAT-006, US-DAT-012 |
| US-DAT-NFR-002 | Thread Safety for Profile Service | 8 | 2 | PLANNED | US-DAT-013 |

### Story Point Totals by Phase

**Phase 3**: 20 points (US-DAT-001 through US-DAT-007, US-DAT-031 through US-DAT-033)
**Phase 5**: 35 points (US-DAT-008 through US-DAT-017, US-DAT-023 through US-DAT-030)
**Phase 7**: 11 points (US-DAT-018 through US-DAT-022, US-DAT-034 through US-DAT-035)
**Phase 8**: 2 points (US-DAT-NFR-002)

**Total**: 68 story points

---

## Implementation Order Recommendation

### Phase 3 (Settings and Cache)
1. US-DAT-001, US-DAT-002 (directory infrastructure)
2. US-DAT-003, US-DAT-004 (settings load)
3. US-DAT-005, US-DAT-006 (settings save with atomic write)
4. US-DAT-007 (settings validation)
5. US-DAT-031, US-DAT-032, US-DAT-033 (chart cache)

### Phase 5 (Profiles and High Scores)
1. US-DAT-008 (profile schema)
2. US-DAT-009, US-DAT-010 (profile load)
3. US-DAT-011, US-DAT-012 (profile save with atomic write)
4. US-DAT-013 (profile service API)
5. US-DAT-014 (chart hashing)
6. US-DAT-015, US-DAT-016, US-DAT-017 (high scores)
7. US-DAT-023, US-DAT-024, US-DAT-025 (profile UI)
8. US-DAT-026, US-DAT-027 (profile switching)
9. US-DAT-028, US-DAT-029, US-DAT-030 (migration support)

### Phase 7 (Statistics)
1. US-DAT-018, US-DAT-019, US-DAT-020 (basic statistics)
2. US-DAT-021, US-DAT-022 (judgment distribution and accuracy)
3. US-DAT-034, US-DAT-035 (backup/restore - optional)

### Phase 8 (Network Readiness)
1. US-DAT-NFR-002 (thread safety)
