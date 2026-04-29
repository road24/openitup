# Chart System Requirements

## REQ-CHT-001: Unified Chart Data Model
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must define a unified internal `Chart` structure containing metadata, timing data, and note data that all parsers produce.

**Acceptance Criteria**:
- Single Chart struct used throughout engine
- No parser-specific data structures leak to gameplay code
- Chart is immutable after loading
- Chart supports all features of supported formats

**Dependencies**: None  
**Source**: Roadmap subsystem 4

---

## REQ-CHT-002: Chart Metadata
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The chart must contain metadata including title, artist, BPM, banner path, and other display information.

**Acceptance Criteria**:
- Title, artist, genre fields (UTF-8 strings)
- Banner/jacket/background image paths
- Difficulty rating and difficulty name
- Charter/creator name
- Minimum required BPM displayed (for display speed)

**Dependencies**: REQ-CHT-001  
**Source**: Roadmap subsystem 4

---

## REQ-CHT-003: Timing Data Model
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The chart must contain a sorted list of timing events (BPM changes, stops, warps, speed changes, delays) with functions to convert between beat time and real time.

**Acceptance Criteria**:
- `time_at_beat(beat) -> seconds` conversion function
- `beat_at_time(seconds) -> beat` conversion function
- Handles BPM changes, stops, warps, delays correctly
- Uses sorted vector + binary search for O(log n) lookup
- Accuracy within 0.1ms for any beat position

**Dependencies**: REQ-CHT-001  
**Source**: Roadmap subsystem 4

---

## REQ-CHT-004: Note Data Model
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The chart must contain a flat, time-sorted vector of note events with beat position, column index, and note type.

**Acceptance Criteria**:
- Note types: tap, hold head, hold tail, mine, fake, lift
- Beat position stored as double with sub-beat precision
- Column index 0-4 for single, 0-9 for double charts
- Sorted by beat position for efficient scanning

**Dependencies**: REQ-CHT-001  
**Source**: Roadmap subsystem 4

---

## REQ-CHT-005: KSF Parser
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must parse Kick It Up .ksf chart files.

**Acceptance Criteria**:
- Loads .ksf files from original Kick It Up
- Extracts all metadata fields
- Parses BPM and timing information
- Converts note data to internal format
- KSF parser extracts the audio filename from the chart metadata
- Engine resolves audio filename relative to the song directory
- Handles malformed files with error logging

**Dependencies**: REQ-CHT-001  
**Source**: Roadmap subsystem 4, Phase 1

---

## REQ-CHT-006: SSC Parser
**Status**: [PLANNED Phase 4]  
**Priority**: Must Have

The engine must parse StepMania .ssc chart files.

**Acceptance Criteria**:
- Loads .ssc files from StepMania 5.x
- Handles all timing events (BPMs, stops, warps, delays)
- Parses pump-single5 and pump-double10 charts
- Ignores unsupported chart types gracefully

**Dependencies**: REQ-CHT-001  
**Source**: Roadmap subsystem 4, Phase 4

---

## REQ-CHT-007: Additional Format Parsers
**Status**: [PLANNED Phase 4]  
**Priority**: Should Have

The engine must parse .sma (StepMania legacy), .stx (PIU Pro/Pro2), .see (PIU Exceed/Exceed2 encrypted), and .nx (PIU NX-Phoenix) formats.

**Acceptance Criteria**:
- Each parser produces valid Chart struct
- .see parser requires decryption key from settings
- .nx parser handles multiple sub-variants (NX, NX2, NXA, etc)
- All parsers log errors for unsupported features

**Dependencies**: REQ-CHT-001  
**Source**: Roadmap subsystem 4, Phase 4

---

## REQ-CHT-008: OSF Format Specification
**Status**: [PLANNED Phase 4]  
**Priority**: Should Have

The engine must define a new .osf (OpenItUp Step File) JSON-based format representing all features of legacy formats plus new capabilities.

**Acceptance Criteria**:
- JSON schema document written in docs/
- Supports arbitrary scroll speed changes
- Supports per-note metadata
- Supports Lua hooks for custom behavior
- Human-readable and diff-friendly

**Dependencies**: REQ-CHT-001  
**Source**: Roadmap subsystem 4, Phase 4

---

## REQ-CHT-009: OSF Parser and Writer
**Status**: [PLANNED Phase 4]  
**Priority**: Should Have

The engine must load and save .osf format charts.

**Acceptance Criteria**:
- Parser produces valid Chart struct from .osf file
- Writer serializes Chart to .osf with all data preserved
- Round-trip preserves all data (load -> save -> load identical)
- Validation rejects malformed .osf files with clear errors

**Dependencies**: REQ-CHT-008  
**Source**: Roadmap subsystem 4, Phase 4

---

## REQ-CHT-010: Chart Content Hash
**Status**: [PLANNED Phase 4]  
**Priority**: Must Have

Charts must be identified by a SHA-256 content hash of note data and timing data (excluding metadata) to maintain score identity across file moves and format conversions.

**Acceptance Criteria**:
- Hash computed once at load time
- Hash stable across file renames and directory moves
- Same chart in different formats produces same hash
- Hash stored with high scores in profile

**Dependencies**: REQ-CHT-001, REQ-DAT-005  
**Source**: Roadmap subsystem 4, architecture decisions

---

## REQ-CHT-011: Canonical Binary Representation
**Status**: [PLANNED Phase 4]  
**Priority**: Must Have

Chart hashing must use a canonical binary representation ensuring identical charts produce identical hashes.

**Acceptance Criteria**:
- Binary format specified in documentation
- Metadata excluded from hash
- Floating-point values normalized (precision limits)
- Byte order defined (little-endian)

**Dependencies**: REQ-CHT-010  
**Source**: Roadmap subsystem 9

---

## REQ-CHT-012: Chart Validation
**Status**: [PLANNED Phase 4]  
**Priority**: Should Have

The engine must validate loaded charts for common errors and inconsistencies.

**Acceptance Criteria**:
- Detects hold notes without tail
- Detects overlapping notes in same column
- Detects invalid beat positions (negative)
- Detects timing data inconsistencies
- Validation warnings logged but don't prevent loading

**Dependencies**: REQ-CHT-001  
**Source**: Roadmap subsystem 4 (implied)

---

## REQ-CHT-013: Chart Difficulty Classification
**Status**: [PLANNED Phase 4]  
**Priority**: Should Have

Charts must be classified by play mode (Single, Double, Co-op) and difficulty level.

**Acceptance Criteria**:
- Modes: Single (5 columns), Double (10 columns), Co-op (5+5 separate)
- Difficulty levels: Beginner, Easy, Normal, Hard, Crazy, Nightmare
- Numeric difficulty rating (1-28 for classic, higher for modern)
- Mode and difficulty correctly parsed from all formats

**Dependencies**: REQ-CHT-001  
**Source**: Roadmap subsystem 4 (implied from PIU conventions)

---

## REQ-CHT-014: Chart Preview Information
**Status**: [PLANNED Phase 3]  
**Priority**: Should Have

Charts must provide preview information for song select display (preview audio position, sample length).

**Acceptance Criteria**:
- Preview start position in seconds
- Preview length (default 10-15 seconds)
- Falls back to middle of song if not specified
- Preview data stored in metadata

**Dependencies**: REQ-CHT-002  
**Source**: Roadmap Phase 3 (song select screen)

---

## REQ-CHT-015: Chart Editor Support
**Status**: [FUTURE]  
**Priority**: Could Have

The .osf format must be designed to support future chart editor implementation.

**Acceptance Criteria**:
- Format supports undo/redo metadata
- Format preserves editor-specific annotations
- Format is human-editable in text editor if needed
- Format validates before saving

**Dependencies**: REQ-CHT-008  
**Source**: Roadmap scope notes
