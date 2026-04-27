# Data Management Requirements

## REQ-DAT-001: Player Profile Storage
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

Player profiles must be stored as JSON files in platform-appropriate user data directory (~/.local/share/openitup/profiles/ on Linux).

**Acceptance Criteria**:
- One JSON file per profile
- Directory created automatically if missing
- Cross-platform path resolution (Linux, Windows)
- Profiles readable/writable by user for backup

**Dependencies**: REQ-ENG-002  
**Source**: Roadmap subsystem 9

---

## REQ-DAT-002: Engine Settings File
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

Engine settings must be stored in a separate settings.json file containing video resolution, audio device, key bindings, and global offset.

**Acceptance Criteria**:
- Settings load at engine startup
- Settings save on change (not just on exit)
- Invalid settings log error and use defaults
- Settings file location: user data directory

**Dependencies**: REQ-ENG-002  
**Source**: Roadmap subsystem 9, Phase 3

---

## REQ-DAT-003: Profile Display Name and Metadata
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

Each profile must contain display name, preferred speed mod, note skin selection, calibration offsets, and play statistics.

**Acceptance Criteria**:
- Display name 3-20 characters, UTF-8
- Speed mod preference (C-mod or M-mod with value)
- Note skin selection (directory name)
- Input and audio calibration offsets
- Play statistics: songs played, total time, total score

**Dependencies**: REQ-DAT-001  
**Source**: Roadmap subsystem 9

---

## REQ-DAT-004: Multiple Profile Support
**Status**: [PLANNED Phase 5]  
**Priority**: Should Have

The engine must support multiple local profiles with per-profile high scores and settings.

**Acceptance Criteria**:
- Profile selection at startup or from menu
- Create new profile functionality
- Delete profile functionality with confirmation
- Active profile persists across sessions
- Switching profiles does not lose data

**Dependencies**: REQ-DAT-001, REQ-SCN-013  
**Source**: Roadmap subsystem 9, Phase 5

---

## REQ-DAT-005: Per-Chart High Score Storage
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

Profiles must store high scores per chart using content hash as identifier, including score, grade, max combo, and judgment counts.

**Acceptance Criteria**:
- Scores keyed by chart content hash (SHA-256)
- Stores: score, grade, max combo, perfect/great/good/bad/miss counts
- Stores: date achieved, judge profile used
- Multiple scores per chart (top 5 or top 10)
- Scores survive chart file moves and format conversions

**Dependencies**: REQ-DAT-001, REQ-CHT-010  
**Source**: Roadmap subsystem 9, Phase 5

---

## REQ-DAT-006: Profile Service Object
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

The engine must provide a profile service object owned by Engine, accessible from C++ screens and Lua scripts.

**Acceptance Criteria**:
- Singleton or service pattern (one instance)
- API: load profile, save profile, get high scores, set high score
- API: get/set settings, get play stats
- Thread-safe if accessed from multiple contexts
- Lua bindings for profile access

**Dependencies**: REQ-DAT-001, REQ-ENG-002  
**Source**: Roadmap subsystem 9

---

## REQ-DAT-007: Settings Validation
**Status**: [PLANNED Phase 3]  
**Priority**: Should Have

Settings file loading must validate all values and use defaults for invalid or missing entries.

**Acceptance Criteria**:
- Resolution validated against supported values
- Offsets clamped to valid ranges (-500 to +500 ms)
- Key bindings validated (no duplicate assignments)
- Invalid JSON logs error and uses full default settings

**Dependencies**: REQ-DAT-002  
**Source**: Roadmap subsystem 9 (implied)

---

## REQ-DAT-008: Profile Statistics Tracking
**Status**: [PLANNED Phase 7]  
**Priority**: Should Have

Profiles must track play statistics including total songs played, total play time, total score, and judgment distribution.

**Acceptance Criteria**:
- Total songs played (count)
- Total play time (hours)
- Total score across all plays
- Total judgments: perfect/great/good/bad/miss counts
- Average accuracy percentage
- Statistics displayed in profile screen

**Dependencies**: REQ-DAT-003  
**Source**: Roadmap subsystem 9, Phase 7

---

## REQ-DAT-009: Data Migration Support
**Status**: [PLANNED Phase 5]  
**Priority**: Should Have

The profile and settings system must support version migration as schema evolves.

**Acceptance Criteria**:
- Schema version number in JSON files
- Migration functions for old versions to current
- Unknown fields preserved for forward compatibility
- Migration errors logged but don't prevent loading

**Dependencies**: REQ-DAT-001, REQ-DAT-002  
**Source**: Roadmap subsystem 9 (implied)

---

## REQ-DAT-010: Backup and Export
**Status**: [PLANNED Phase 7]  
**Priority**: Could Have

The engine should provide functionality to backup/export and restore/import profiles.

**Acceptance Criteria**:
- Export profile to ZIP or single JSON
- Import profile from ZIP or single JSON
- Export includes all high scores and settings
- Import validates data before applying

**Dependencies**: REQ-DAT-001  
**Source**: Roadmap subsystem 9 (implied)

---

## REQ-DAT-011: Atomic File Writes
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

Profile and settings saves must use atomic file writes (write to temp, rename) to prevent corruption on crash.

**Acceptance Criteria**:
- Write to temporary file first
- Atomic rename on successful write
- Original preserved if write fails
- No partial or corrupted saves on power loss (OS dependent)

**Dependencies**: REQ-DAT-001, REQ-DAT-002  
**Source**: Roadmap subsystem 9 (data integrity best practice)

---

## REQ-DAT-012: Chart Metadata Cache
**Status**: [PLANNED Phase 3]  
**Priority**: Should Have

The engine must cache chart metadata and file paths to avoid full directory re-scans on every launch.

**Acceptance Criteria**:
- Cache file in user data directory
- Cache invalidated on directory mtime change
- Cache stores: file path, hash, title, artist, difficulties
- Cache loads in under 1 second for 1000+ songs

**Dependencies**: REQ-AST-003  
**Source**: Roadmap subsystem 11 (implied for performance)

---

## REQ-DAT-013: User Data Directory Auto-Creation
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

The engine must automatically create user data directories on first run if they don't exist.

**Acceptance Criteria**:
- ~/.local/share/openitup/ on Linux
- %APPDATA%/openitup/ on Windows
- profiles/ and cache/ subdirectories created
- Appropriate permissions set
- Fails gracefully if creation not possible (read-only filesystem)

**Dependencies**: REQ-DAT-001  
**Source**: Roadmap subsystem 9 (implied)
