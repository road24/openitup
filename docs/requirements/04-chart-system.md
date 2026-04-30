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

**Dependencies**: REQ-CHT-001, REQ-CHT-016  
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

**Dependencies**: REQ-CHT-001, REQ-CHT-018  
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

**Dependencies**: REQ-CHT-001, REQ-CHT-019, REQ-CHT-020, REQ-CHT-021, REQ-CHT-022  
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

---

## REQ-CHT-016: KSF Format Specification Documentation
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The KSF (Kick It Up) chart format must be documented with a formal specification covering syntax, semantics, and expected parser behavior.

**Acceptance Criteria**:
- Complete format spec written in docs/ covering file structure, metadata fields, timing sections, note encoding
- Specification includes examples of valid KSF files
- Edge cases documented (empty measures, BPM changes, file encoding)
- Spec references original Kick It Up behavior where applicable

**Dependencies**: None  
**Source**: PO conversation 2026-04-29, requirements gap analysis

---

## REQ-CHT-017: KSF Parser Specification Compliance
**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

The existing KSF parser must be audited against the formal specification and any deviations must be fixed.

**Acceptance Criteria**:
- Audit identifies all spec deviations in current parser implementation
- Critical deviations (incorrect note placement, timing errors) are fixed
- Non-critical deviations (metadata handling differences) are documented with rationale
- All spec compliance issues are tracked and resolved

**Dependencies**: REQ-CHT-005, REQ-CHT-016  
**Source**: PO conversation 2026-04-29, requirements gap analysis

---

## REQ-CHT-018: SSC Format Specification Documentation
**Status**: [PLANNED Phase 4]  
**Priority**: Must Have

The SSC (StepMania 5) chart format must be documented with a formal specification covering syntax, semantics, and expected parser behavior.

**Acceptance Criteria**:
- Complete format spec written in docs/ covering file structure, metadata tags, timing events, note encoding
- Specification includes examples of valid SSC files
- Specification covers pump-single5 and pump-double10 chart types specifically
- Edge cases documented (nested timing changes, warp handling, chart-specific vs song-global tags)

**Dependencies**: None  
**Source**: PO conversation 2026-04-29, requirements gap analysis

---

## REQ-CHT-019: SMA Format Specification Documentation
**Status**: [PLANNED Phase 4]  
**Priority**: Should Have

The SMA (StepMania legacy) chart format must be documented with a formal specification covering syntax, semantics, and expected parser behavior.

**Acceptance Criteria**:
- Complete format spec written in docs/ covering file structure, metadata fields, timing sections, note encoding
- Specification includes examples of valid SMA files
- Differences from SSC format clearly documented
- Edge cases documented (legacy BPM handling, measure length variations)

**Dependencies**: None  
**Source**: PO conversation 2026-04-29, requirements gap analysis

---

## REQ-CHT-020: STX Format Specification Documentation
**Status**: [PLANNED Phase 4]  
**Priority**: Should Have

The STX (PIU Pro/Pro2) chart format must be documented with a formal specification covering syntax, semantics, and expected parser behavior.

**Acceptance Criteria**:
- Complete format spec written in docs/ covering file structure, metadata fields, timing sections, note encoding
- Specification includes examples of valid STX files
- Pro vs Pro2 format variations documented
- Edge cases documented (BPM change handling, difficulty encoding)

**Dependencies**: None  
**Source**: PO conversation 2026-04-29, requirements gap analysis

---

## REQ-CHT-021: SEE Format Specification Documentation
**Status**: [PLANNED Phase 4]  
**Priority**: Should Have

The SEE (PIU Exceed/Exceed2 encrypted) chart format must be documented with a formal specification covering encryption, syntax, semantics, and expected parser behavior.

**Acceptance Criteria**:
- Complete format spec written in docs/ covering encryption scheme, decrypted file structure, metadata fields, timing sections, note encoding
- Specification includes decryption algorithm and key management approach
- Specification includes examples of decrypted SEE file structure
- Edge cases documented (corrupted encryption, partial decryption failures)

**Dependencies**: None  
**Source**: PO conversation 2026-04-29, requirements gap analysis

---

## REQ-CHT-022: NX Format Specification Documentation
**Status**: [PLANNED Phase 4]  
**Priority**: Should Have

The NX (PIU NX-Phoenix) chart format must be documented with a formal specification covering syntax, semantics, sub-variant differences, and expected parser behavior.

**Acceptance Criteria**:
- Complete format spec written in docs/ covering file structure, metadata fields, timing sections, note encoding
- Specification covers all NX sub-variants (NX, NX2, NXA, etc.) and their differences
- Specification includes examples of valid NX files for each sub-variant
- Edge cases documented (variant detection, backward compatibility)

**Dependencies**: None  
**Source**: PO conversation 2026-04-29, requirements gap analysis

---

## REQ-CHT-023: Unified Step Format Design
**Status**: [PLANNED Phase 5 or later]  
**Priority**: Must Have

After all legacy format specifications are complete, the engine must design a unified step file format that captures all features from legacy formats and serves as the foundation for gameplay polishing.

**Acceptance Criteria**:
- Design integrates all features from documented legacy formats (KSF, SSC, SMA, STX, SEE, NX)
- Format design addresses gaps and limitations of legacy formats
- Format specification written in docs/ before any gameplay polishing begins
- Format serves as canonical template for OSF implementation (REQ-CHT-008)

**Dependencies**: REQ-CHT-016, REQ-CHT-018, REQ-CHT-019, REQ-CHT-020, REQ-CHT-021, REQ-CHT-022  
**Source**: PO conversation 2026-04-29, PO directive: "make sure we do not start polishing gameplay until we have the format defined"
