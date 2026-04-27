# Epic: Chart System — Unified Chart Representation and Multi-Format Parsing

This epic covers the internal chart data model, timing/note representations, parsers for all supported chart formats, the new OSF format specification, chart validation, and content hashing for stable score identity across format conversions.

---

## Story ID: US-CHT-001 — Define Internal Chart Structure

**Story Card:**
> **As a** Developer
> **I want** a unified internal Chart data structure
> **So that** gameplay code is decoupled from file format details and parsers can be added without touching the judge or renderer

### 📝 Description
Create a single `Chart` struct in C++ that represents all chart data. This struct serves as the contract between parsers and all downstream systems (judge, note renderer, audio sync). The structure must be immutable after loading to prevent accidental modification during gameplay.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Chart struct exists with required fields**
    *   **Given** the Chart header is included in a compilation unit
    *   **When** a Chart instance is constructed
    *   **Then** it exposes metadata, timing_data, and note_data fields

*   **Scenario 2: Chart is immutable after construction**
    *   **Given** a Chart instance has been constructed
    *   **When** the instance is passed to gameplay systems
    *   **Then** all fields are const-qualified or return const references

*   **Scenario 3: Chart supports all note types**
    *   **Given** the NoteData component of Chart
    *   **When** notes are added during parsing
    *   **Then** the following note types are representable: tap, hold_head, hold_tail, mine, fake, lift

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Small (3 points)
*   **Dependencies**: None — this is the foundation story
*   **Implementation Notes**: Use std::vector for note data, ensure move semantics for efficient loading

---

## Story ID: US-CHT-002 — Implement Chart Metadata Model

**Story Card:**
> **As a** Player
> **I want** song titles, artists, and difficulty information
> **So that** I can identify songs in the song select screen and understand their challenge level

### 📝 Description
Define the metadata component of the Chart struct containing all display and descriptive information. All string fields must support UTF-8 encoding for international song titles and artist names.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Required metadata fields are present**
    *   **Given** a Chart metadata instance
    *   **When** accessed by UI rendering code
    *   **Then** the following fields are available: title, artist, genre, banner_path, difficulty_name, difficulty_rating, charter_name

*   **Scenario 2: UTF-8 strings are preserved**
    *   **Given** a chart file with Japanese characters in the title
    *   **When** the chart is parsed and metadata extracted
    *   **Then** the title string contains valid UTF-8 and displays correctly when rendered

*   **Scenario 3: Optional fields have safe defaults**
    *   **Given** a chart file missing the genre field
    *   **When** the parser constructs the metadata
    *   **Then** the genre field is an empty string and the chart loads successfully

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Small (2 points)
*   **Dependencies**: US-CHT-001
*   **Implementation Notes**: Use std::string for all text fields, consider std::optional for truly optional fields like background_path

---

## Story ID: US-CHT-003 — Implement Timing Data Model

**Story Card:**
> **As a** Developer
> **I want** functions to convert between beat positions and real time
> **So that** the judge and note renderer can accurately translate musical timing to audio timestamps

### 📝 Description
Create a TimingData structure that stores all timing events (BPM changes, stops, warps, delays) in a sorted vector and provides efficient bidirectional conversion functions. The pattern follows the existing keyframe interpolation system (sorted vector with binary search).

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Beat-to-time conversion with single BPM**
    *   **Given** a TimingData with one BPM event at beat 0.0 = 120 BPM
    *   **When** `time_at_beat(4.0)` is called
    *   **Then** the result is 2.0 seconds (4 beats at 120 BPM = 2 seconds)

*   **Scenario 2: Time-to-beat conversion with single BPM**
    *   **Given** a TimingData with one BPM event at beat 0.0 = 120 BPM
    *   **When** `beat_at_time(2.0)` is called
    *   **Then** the result is 4.0 beats

*   **Scenario 3: Conversion accuracy with BPM changes**
    *   **Given** a TimingData with BPM change: beat 0.0 = 120 BPM, beat 8.0 = 180 BPM
    *   **When** `time_at_beat(12.0)` is called
    *   **Then** the result is within 0.1 milliseconds of the mathematically correct value (4.0 + 1.333... = 5.333 seconds)

*   **Scenario 4: Stop event extends time**
    *   **Given** a TimingData with 120 BPM and a stop at beat 4.0 lasting 1.0 seconds
    *   **When** `time_at_beat(5.0)` is called
    *   **Then** the result includes the 1-second stop (3.0 seconds elapsed time)

*   **Scenario 5: Lookup performance is logarithmic**
    *   **Given** a TimingData with 100 BPM change events
    *   **When** `time_at_beat()` is called 10,000 times with random beat positions
    *   **Then** all calls complete in under 10 milliseconds total (average < 1 microsecond per call)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium (5 points)
*   **Dependencies**: US-CHT-001
*   **Implementation Notes**: Use std::lower_bound for binary search, store cumulative time offsets at each event to avoid O(n) traversal

---

## Story ID: US-CHT-004 — Implement Note Data Model

**Story Card:**
> **As a** Developer
> **I want** a flat sorted vector of note events
> **So that** the judge can efficiently scan upcoming notes and the note renderer can iterate over visible notes without unnecessary lookups

### 📝 Description
Define the NoteData component as a sorted vector of NoteEvent structs. Each event contains a beat position (double precision for sub-beat timing), column index, and note type. The data structure must support efficient range queries for "all notes between beat X and beat Y."

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: NoteEvent contains required fields**
    *   **Given** a NoteEvent struct
    *   **When** constructed with beat=4.5, column=2, type=tap
    *   **Then** all three fields are accessible and correct

*   **Scenario 2: Notes are sorted by beat position**
    *   **Given** a NoteData vector with notes at beats [8.0, 2.5, 4.0]
    *   **When** a parser adds these notes and finalizes the chart
    *   **Then** the vector is sorted in ascending order: [2.5, 4.0, 8.0]

*   **Scenario 3: Column indices are valid for single mode**
    *   **Given** a chart designated as single mode (5 panels)
    *   **When** notes are added with column indices 0 through 4
    *   **Then** all notes are accepted

*   **Scenario 4: Column indices are valid for double mode**
    *   **Given** a chart designated as double mode (10 panels)
    *   **When** notes are added with column indices 0 through 9
    *   **Then** all notes are accepted

*   **Scenario 5: Invalid column index is rejected**
    *   **Given** a chart designated as single mode (5 panels)
    *   **When** a parser attempts to add a note with column index 7
    *   **Then** the parser logs an error and either rejects the note or clamps it to a valid column

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Small (3 points)
*   **Dependencies**: US-CHT-001
*   **Implementation Notes**: Consider std::vector with a final std::sort call after parsing, or use std::multiset for automatic sorting

---

## Story ID: US-CHT-005 — Parse KSF Chart Format

**Story Card:**
> **As a** Content Creator
> **I want** KSF chart files from Kick It Up to load
> **So that** I can play classic songs from the earliest PIU versions

### 📝 Description
Implement a parser for the Kick It Up .ksf text format. KSF is the simplest PIU chart format and serves as the initial test case for the chart system. The parser reads metadata, BPM, and note data, then constructs a Chart struct.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Valid KSF file is parsed successfully**
    *   **Given** a valid .ksf file with metadata, BPM 140, and 50 tap notes
    *   **When** the parser processes the file
    *   **Then** a Chart struct is returned with title, artist, BPM 140, and exactly 50 notes in NoteData

*   **Scenario 2: KSF metadata fields are extracted**
    *   **Given** a .ksf file with TITLE and ARTIST lines
    *   **When** the parser reads the file
    *   **Then** the Chart metadata contains the exact title and artist strings from the file

*   **Scenario 3: Note positions are converted to beats**
    *   **Given** a .ksf file with notes at tick 0, tick 48, tick 96 (4/4 time, 192 ticks per beat)
    *   **When** the parser processes the note data
    *   **Then** the resulting NoteData contains notes at beat 0.0, beat 0.25, and beat 0.5

*   **Scenario 4: Malformed KSF file logs error and fails gracefully**
    *   **Given** a .ksf file with a truncated note section
    *   **When** the parser attempts to load the file
    *   **Then** an error is logged via spdlog and the parser throws a ChartLoadException

*   **Scenario 5: Missing required field logs error**
    *   **Given** a .ksf file missing the TITLE line
    *   **When** the parser reads the file
    *   **Then** an error is logged and the parser either throws an exception or sets a default title

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium (5 points)
*   **Dependencies**: US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004
*   **Implementation Notes**: KSF uses tick-based timing (192 ticks per quarter note is common), convert to beat-based internal representation

---

## Story ID: US-CHT-006 — Parse SSC Chart Format

**Story Card:**
> **As a** Player
> **I want** StepMania .ssc chart files to load
> **So that** I can play community-created content from the most popular rhythm game editor

### 📝 Description
Implement a parser for the StepMania .ssc format. SSC is widely used in the community and supports complex timing (BPM changes, stops, warps, delays). The parser must handle pump-single5 and pump-double10 chart types and ignore unsupported types gracefully.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: SSC file with single BPM is parsed**
    *   **Given** a .ssc file with #BPMS:0.000=120.000;
    *   **When** the parser loads the file
    *   **Then** the Chart TimingData contains one BPM event at beat 0.0 with BPM 120.0

*   **Scenario 2: SSC file with BPM changes is parsed**
    *   **Given** a .ssc file with #BPMS:0.000=120.000,8.000=180.000;
    *   **When** the parser loads the file
    *   **Then** the Chart TimingData contains two BPM events and time_at_beat(12.0) accounts for both

*   **Scenario 3: SSC stops are converted to timing events**
    *   **Given** a .ssc file with #STOPS:4.000=1.000;
    *   **When** the parser processes timing data
    *   **Then** a stop event at beat 4.0 lasting 1.0 seconds is added to TimingData

*   **Scenario 4: Pump-single5 chart is parsed**
    *   **Given** a .ssc file with #STEPSTYPE:pump-single5; and note data
    *   **When** the parser loads the chart
    *   **Then** notes are created with column indices 0-4

*   **Scenario 5: Pump-double10 chart is parsed**
    *   **Given** a .ssc file with #STEPSTYPE:pump-double10; and note data
    *   **When** the parser loads the chart
    *   **Then** notes are created with column indices 0-9

*   **Scenario 6: Unsupported chart type is skipped**
    *   **Given** a .ssc file with #STEPSTYPE:dance-single;
    *   **When** the parser scans the file
    *   **Then** the parser logs a warning and skips that chart definition without failing

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Large (8 points)
*   **Dependencies**: US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004
*   **Implementation Notes**: SSC uses semicolon-delimited key=value pairs, supports per-chart timing overrides

---

## Story ID: US-CHT-007 — Parse SMA Chart Format

**Story Card:**
> **As a** Content Creator
> **I want** legacy StepMania .sma chart files to load
> **So that** I can access older community charts created before the SSC format was introduced

### 📝 Description
Implement a parser for the legacy .sma format used by early StepMania versions. SMA is simpler than SSC but still supports BPM changes and stops.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: SMA file with basic metadata loads**
    *   **Given** a .sma file with #TITLE, #ARTIST, and #BPMS tags
    *   **When** the parser processes the file
    *   **Then** a Chart struct is returned with metadata populated

*   **Scenario 2: SMA note data is converted correctly**
    *   **Given** a .sma file with measure-based note data (4/4 time)
    *   **When** the parser reads the notes
    *   **Then** notes are converted to beat positions matching the measure subdivisions

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium (5 points)
*   **Dependencies**: US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004
*   **Implementation Notes**: SMA uses measure notation (e.g., 4 lines = 4 quarter notes), convert to absolute beat positions

---

## Story ID: US-CHT-008 — Parse STX Chart Format

**Story Card:**
> **As a** Player
> **I want** PIU Pro/Pro2 .stx chart files to load
> **So that** I can play official arcade content from the Pro era

### 📝 Description
Implement a parser for the .stx format used in PIU Pro and Pro2. This format is binary and requires reverse-engineering or reference documentation.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: STX file is recognized by magic header**
    *   **Given** a binary file with a valid STX header
    *   **When** the parser opens the file
    *   **Then** the parser identifies it as STX format

*   **Scenario 2: STX metadata is extracted**
    *   **Given** a valid .stx file
    *   **When** the parser reads the metadata section
    *   **Then** title, artist, and difficulty information are populated

*   **Scenario 3: STX note data is decoded**
    *   **Given** a .stx file with binary note data
    *   **When** the parser decodes the note section
    *   **Then** notes appear in NoteData with correct beat positions and column indices

*   **Scenario 4: Unrecognized STX variant logs warning**
    *   **Given** a .stx file with an unknown version byte
    *   **When** the parser attempts to load it
    *   **Then** a warning is logged and the parser attempts best-effort parsing or fails gracefully

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Large (8 points)
*   **Dependencies**: US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004
*   **Implementation Notes**: Binary format requires careful byte-order handling, consider endianness

---

## Story ID: US-CHT-009 — Parse SEE Chart Format

**Story Card:**
> **As a** Player
> **I want** PIU Exceed/Exceed2 .see chart files to load
> **So that** I can play official arcade content from the Exceed era

### 📝 Description
Implement a parser for the .see encrypted format used in PIU Exceed and Exceed2. This requires a decryption key configured in the engine settings.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: SEE file is decrypted with valid key**
    *   **Given** a .see file and a correct decryption key in settings.json
    *   **When** the parser loads the file
    *   **Then** the file is decrypted and parsed successfully

*   **Scenario 2: SEE file without key fails gracefully**
    *   **Given** a .see file and no decryption key in settings
    *   **When** the parser attempts to load the file
    *   **Then** an error is logged stating "SEE decryption key not configured" and the parser returns failure

*   **Scenario 3: Incorrect key produces readable error**
    *   **Given** a .see file and an incorrect decryption key
    *   **When** the parser attempts to decrypt
    *   **Then** the parser detects corrupt data and logs "SEE decryption failed: invalid key or corrupted file"

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Large (8 points)
*   **Dependencies**: US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004, REQ-SET-001 (settings system for key storage)
*   **Implementation Notes**: Encryption algorithm must be reverse-engineered or documented, handle key management securely

---

## Story ID: US-CHT-010 — Parse NX Chart Format

**Story Card:**
> **As a** Player
> **I want** PIU NX-Phoenix .nx chart files to load
> **So that** I can play official arcade content from NX through Phoenix versions

### 📝 Description
Implement a parser for the .nx format family used from PIU NX through Phoenix. This format has multiple sub-variants (NX, NX2, NXA, Fiesta, XX, Phoenix). The parser must detect the variant and handle differences in structure.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: NX file variant is detected**
    *   **Given** a .nx file with a version identifier in the header
    *   **When** the parser opens the file
    *   **Then** the parser logs the detected variant (e.g., "Loading NX2 chart")

*   **Scenario 2: NX metadata is extracted**
    *   **Given** a valid .nx file from the NX2 version
    *   **When** the parser processes the file
    *   **Then** title, artist, and BPM are populated in Chart metadata

*   **Scenario 3: NX note data is decoded**
    *   **Given** a .nx file with note data
    *   **When** the parser reads the note section
    *   **Then** notes are added to NoteData with correct timing and columns

*   **Scenario 4: Unsupported NX sub-variant logs warning**
    *   **Given** a .nx file with an unknown sub-variant version byte
    *   **When** the parser attempts to load it
    *   **Then** a warning is logged and the parser attempts best-effort parsing with the most similar known variant

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Large (13 points) — complexity due to multiple sub-variants
*   **Dependencies**: US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004
*   **Implementation Notes**: May require separate sub-parsers per variant, or a configurable parser with version-specific branches

---

## Story ID: US-CHT-011 — Specify OSF JSON Chart Format

**Story Card:**
> **As a** Developer
> **I want** a documented JSON-based chart format specification
> **So that** the engine has a canonical, human-readable, diff-friendly format for storing charts and future tools can implement it

### 📝 Description
Write a comprehensive format specification document for .osf (OpenItUp Step File) in the docs/ directory. This format must represent all features of legacy formats plus extensibility for future capabilities (arbitrary scroll speed changes, per-note metadata, Lua hooks).

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Specification document exists**
    *   **Given** the docs/ directory
    *   **When** the specification is written
    *   **Then** a file named osf-format-spec.md is present and contains a JSON schema

*   **Scenario 2: Specification includes required fields**
    *   **Given** the OSF specification document
    *   **When** reviewed by a developer
    *   **Then** the document specifies: metadata object, timing_events array, notes array, and optional extensions object

*   **Scenario 3: Specification supports all note types**
    *   **Given** the OSF notes array schema
    *   **When** a chart with taps, holds, mines, fakes, and lifts is represented
    *   **Then** each note type has a defined type field value and required properties

*   **Scenario 4: Specification supports timing events**
    *   **Given** the OSF timing_events array schema
    *   **When** a chart with BPM changes, stops, warps, and delays is represented
    *   **Then** each event type has a defined structure with beat position and event-specific parameters

*   **Scenario 5: Specification is human-readable**
    *   **Given** an .osf file created according to the specification
    *   **When** opened in a text editor
    *   **Then** all fields are named clearly and values are not encoded or obfuscated

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium (5 points)
*   **Dependencies**: US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004 (to understand all requirements)
*   **Implementation Notes**: Use JSON Schema for formal validation, include examples in the document

---

## Story ID: US-CHT-012 — Parse OSF JSON Chart Format

**Story Card:**
> **As a** Developer
> **I want** the engine to load .osf JSON chart files
> **So that** I can use the canonical format for testing and future chart creation

### 📝 Description
Implement a parser that reads .osf files and produces Chart structs. The parser must validate the JSON against the schema and provide clear error messages for malformed files.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Valid OSF file is parsed**
    *   **Given** a .osf file conforming to the specification
    *   **When** the parser loads the file
    *   **Then** a Chart struct is returned with all metadata, timing, and note data populated

*   **Scenario 2: OSF metadata is extracted**
    *   **Given** a .osf file with a metadata object containing title and artist
    *   **When** the parser processes the file
    *   **Then** the Chart metadata contains the correct values

*   **Scenario 3: OSF timing events are loaded**
    *   **Given** a .osf file with a timing_events array containing BPM changes and stops
    *   **When** the parser processes the timing data
    *   **Then** the Chart TimingData contains all events and conversions are accurate

*   **Scenario 4: OSF notes are loaded**
    *   **Given** a .osf file with a notes array
    *   **When** the parser processes the note data
    *   **Then** the Chart NoteData contains all notes sorted by beat position

*   **Scenario 5: Invalid OSF file produces clear error**
    *   **Given** a .osf file with a missing required field (e.g., no "notes" array)
    *   **When** the parser validates the JSON
    *   **Then** a validation error is logged with the message "Missing required field: notes"

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium (5 points)
*   **Dependencies**: US-CHT-011
*   **Implementation Notes**: Use nlohmann/json for parsing (already in the project), consider a JSON schema validator library

---

## Story ID: US-CHT-013 — Write Charts to OSF Format

**Story Card:**
> **As a** Developer
> **I want** the engine to save Chart structs to .osf files
> **So that** I can convert legacy formats to the canonical format and verify round-trip correctness

### 📝 Description
Implement a writer that serializes Chart structs to .osf JSON format. The output must be valid according to the OSF specification and human-readable (pretty-printed with indentation).

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Chart is serialized to OSF**
    *   **Given** a Chart struct with metadata, timing data, and notes
    *   **When** the writer serializes it to .osf
    *   **Then** a valid JSON file is produced with all data present

*   **Scenario 2: OSF output is pretty-printed**
    *   **Given** a Chart serialized to .osf
    *   **When** the file is opened in a text editor
    *   **Then** the JSON is indented with 2 or 4 spaces and fields are on separate lines

*   **Scenario 3: Round-trip preserves all data**
    *   **Given** a Chart loaded from a .osf file
    *   **When** the Chart is written back to .osf and then re-loaded
    *   **Then** the two Chart structs are identical (metadata, timing events, notes all match)

*   **Scenario 4: Empty optional fields are omitted**
    *   **Given** a Chart with no background_path metadata
    *   **When** serialized to .osf
    *   **Then** the background_path field is either omitted or set to null, not an empty string

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Small (3 points)
*   **Dependencies**: US-CHT-011, US-CHT-012
*   **Implementation Notes**: Use nlohmann/json with dump(2) or dump(4) for pretty-printing

---

## Story ID: US-CHT-014 — Compute Chart Content Hash

**Story Card:**
> **As a** Player
> **I want** my scores to persist across file renames and format conversions
> **So that** I don't lose my high scores when I reorganize my song library or convert charts to a different format

### 📝 Description
Implement a SHA-256 content hash for charts based only on note data and timing data (excluding metadata like title or artist). This hash serves as the stable identity for a chart across all file operations and format conversions.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Hash is computed at load time**
    *   **Given** a Chart is loaded by any parser
    *   **When** the Chart struct is constructed
    *   **Then** a content_hash field is populated with a 64-character hexadecimal string

*   **Scenario 2: Same chart in different formats produces same hash**
    *   **Given** the same note data and timing data represented as .ksf and .osf files
    *   **When** both are loaded and hashed
    *   **Then** both Chart instances have identical content_hash values

*   **Scenario 3: Metadata changes do not affect hash**
    *   **Given** two .osf files with identical notes and timing but different titles
    *   **When** both are loaded and hashed
    *   **Then** both produce the same content_hash

*   **Scenario 4: Note data changes produce different hash**
    *   **Given** two charts differing by a single note
    *   **When** both are hashed
    *   **Then** the content_hash values are different

*   **Scenario 5: Timing changes produce different hash**
    *   **Given** two charts with identical notes but one BPM value differs
    *   **When** both are hashed
    *   **Then** the content_hash values are different

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium (5 points)
*   **Dependencies**: US-CHT-001, US-CHT-003, US-CHT-004, US-CHT-015 (canonical binary representation)
*   **Implementation Notes**: Use OpenSSL or a lightweight SHA-256 library, hash order must be deterministic

---

## Story ID: US-CHT-015 — Define Canonical Binary Representation for Hashing

**Story Card:**
> **As a** Developer
> **I want** a documented canonical binary format for chart hashing
> **So that** identical charts always produce identical hashes regardless of which parser or system loaded them

### 📝 Description
Specify the exact byte-level representation used for hashing chart content. This includes field ordering, floating-point precision, and byte order (endianness). Document this in the docs/ directory.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Specification document exists**
    *   **Given** the docs/ directory
    *   **When** the specification is written
    *   **Then** a file named chart-hashing-spec.md is present

*   **Scenario 2: Specification defines field order**
    *   **Given** the hashing specification
    *   **When** reviewed by a developer
    *   **Then** the document specifies the exact order in which timing events and notes are serialized for hashing

*   **Scenario 3: Specification defines floating-point precision**
    *   **Given** the hashing specification
    *   **When** a BPM value of 120.00000001 is encountered
    *   **Then** the document specifies how many decimal places are retained (e.g., round to 3 decimal places)

*   **Scenario 4: Specification defines byte order**
    *   **Given** the hashing specification
    *   **When** multi-byte values are serialized
    *   **Then** the document specifies little-endian byte order

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Small (2 points)
*   **Dependencies**: US-CHT-001, US-CHT-003, US-CHT-004
*   **Implementation Notes**: Consider versioning the hash algorithm in case changes are needed later

---

## Story ID: US-CHT-016 — Validate Charts for Common Errors

**Story Card:**
> **As a** Content Creator
> **I want** the engine to warn me about chart errors
> **So that** I can identify and fix issues like orphaned hold tails or overlapping notes without the chart failing to load

### 📝 Description
Implement a validation pass that runs after chart parsing and checks for common authoring errors. Validation warnings are logged but do not prevent the chart from loading, allowing players to attempt the chart even if it has minor issues.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Hold note without tail is detected**
    *   **Given** a chart with a hold_head at beat 4.0 column 2 but no hold_tail
    *   **When** the validator runs
    *   **Then** a warning is logged: "Hold note at beat 4.0 column 2 has no tail"

*   **Scenario 2: Overlapping notes in same column are detected**
    *   **Given** a chart with two tap notes at beat 4.0 column 1
    *   **When** the validator runs
    *   **Then** a warning is logged: "Overlapping notes at beat 4.0 column 1"

*   **Scenario 3: Negative beat position is detected**
    *   **Given** a chart with a note at beat -1.0
    *   **When** the validator runs
    *   **Then** a warning is logged: "Invalid negative beat position: -1.0"

*   **Scenario 4: BPM is zero or negative**
    *   **Given** a chart with a BPM event setting BPM to 0
    *   **When** the validator runs
    *   **Then** a warning is logged: "Invalid BPM value: 0.0 at beat X"

*   **Scenario 5: Validation warnings do not prevent loading**
    *   **Given** a chart with one validation warning
    *   **When** the chart is loaded
    *   **Then** the Chart struct is still returned and the warning is logged at WARN level

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium (5 points)
*   **Dependencies**: US-CHT-001, US-CHT-003, US-CHT-004
*   **Implementation Notes**: Run validation as a separate function after parsing, collect all warnings and log them together

---

## Story ID: US-CHT-017 — Classify Chart Difficulty and Mode

**Story Card:**
> **As a** Player
> **I want** to see whether a chart is Single or Double and its difficulty level
> **So that** I can choose charts appropriate for my skill level and play style

### 📝 Description
Add mode (Single, Double, Co-op) and difficulty classification to the Chart metadata. Mode is determined by the number of columns (5 = Single, 10 = Double). Difficulty includes both a named level (Beginner, Easy, Normal, Hard, Crazy, Nightmare) and a numeric rating (1-28 for classic, higher for modern).

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Single mode is identified**
    *   **Given** a chart with 5 columns (indices 0-4)
    *   **When** the chart is loaded
    *   **Then** the Chart metadata mode field is "Single"

*   **Scenario 2: Double mode is identified**
    *   **Given** a chart with 10 columns (indices 0-9)
    *   **When** the chart is loaded
    *   **Then** the Chart metadata mode field is "Double"

*   **Scenario 3: Difficulty name is parsed from chart file**
    *   **Given** a .ssc file with #DIFFICULTY:Hard;
    *   **When** the parser loads the chart
    *   **Then** the Chart metadata difficulty_name is "Hard"

*   **Scenario 4: Numeric difficulty rating is parsed**
    *   **Given** a .ssc file with #METER:15;
    *   **When** the parser loads the chart
    *   **Then** the Chart metadata difficulty_rating is 15

*   **Scenario 5: Unknown difficulty name is preserved**
    *   **Given** a chart file with a custom difficulty name "Challenge"
    *   **When** the parser loads the chart
    *   **Then** the Chart metadata difficulty_name is "Challenge" (not normalized or rejected)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Small (3 points)
*   **Dependencies**: US-CHT-002, US-CHT-004
*   **Implementation Notes**: Mode can be derived from NoteData max column index, difficulty must be parsed from metadata

---

## Story ID: US-CHT-018 — Store Preview Audio Information

**Story Card:**
> **As a** Player
> **I want** to hear a preview of the song
> **So that** I can recognize songs in the song select screen and decide which one to play

### 📝 Description
Add preview audio metadata to the Chart struct: a start position in seconds and a preview length. If not specified in the chart file, fall back to the middle of the song with a 15-second duration.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Preview start is parsed from chart**
    *   **Given** a .ssc file with #SAMPLESTART:30.5;
    *   **When** the parser loads the chart
    *   **Then** the Chart metadata preview_start_seconds is 30.5

*   **Scenario 2: Preview length is parsed from chart**
    *   **Given** a .ssc file with #SAMPLELENGTH:12.0;
    *   **When** the parser loads the chart
    *   **Then** the Chart metadata preview_length_seconds is 12.0

*   **Scenario 3: Missing preview defaults to middle of song**
    *   **Given** a chart with no preview metadata and an audio file of 180 seconds
    *   **When** the chart is loaded
    *   **Then** the Chart metadata preview_start_seconds is 90.0 (half of 180)

*   **Scenario 4: Missing preview length defaults to 15 seconds**
    *   **Given** a chart with preview_start specified but no preview_length
    *   **When** the chart is loaded
    *   **Then** the Chart metadata preview_length_seconds is 15.0

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Small (2 points)
*   **Dependencies**: US-CHT-002
*   **Implementation Notes**: Audio system (subsystem 3) will use these values, but they are stored in Chart metadata

---

## 🔍 Story ID: US-CHT-019 — Handle Multi-Chart Files (Inferred)

**Story Card:**
> **As a** Player
> **I want** all difficulty charts for a song to load from a single file
> **So that** I can choose between Easy, Normal, Hard, and Crazy difficulties without managing separate files

### 📝 Description
Many chart formats (SSC, SMA, NX) embed multiple difficulty charts in a single file. The parser must detect and extract all charts, returning a vector of Chart structs rather than a single Chart. Asset management will then present each difficulty as a selectable option in the song select screen.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Multi-chart SSC file returns multiple Charts**
    *   **Given** a .ssc file with 3 pump-single5 charts (Easy, Normal, Hard)
    *   **When** the parser loads the file
    *   **Then** a vector of 3 Chart structs is returned

*   **Scenario 2: Each chart has correct metadata**
    *   **Given** a .ssc file with 2 charts at different difficulty levels
    *   **When** both are loaded
    *   **Then** each Chart has a unique difficulty_name and difficulty_rating

*   **Scenario 3: Shared metadata is not duplicated**
    *   **Given** a .ssc file with global metadata (title, artist) and per-chart difficulty
    *   **When** charts are loaded
    *   **Then** all Chart structs share the same title and artist values

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium (5 points)
*   **Dependencies**: US-CHT-006 (SSC parser), US-CHT-017 (difficulty classification)
*   **Implementation Notes**: Parser API may need to return std::vector<Chart> instead of single Chart

---

## Non-Functional Requirements

### NFR-CHT-001: Parser Performance

**Story Card:**
> **As a** Player
> **I want** charts to load quickly during song selection
> **So that** I don't experience delays when browsing the music wheel

**Acceptance Criteria:**
*   **Given** a song database with 500 charts
*   **When** the song select screen is opened
*   **Then** all chart metadata (title, artist, difficulty) is available within 2 seconds
*   **And when** a specific chart is selected for gameplay
*   **Then** the full chart (with timing and note data) loads within 500 milliseconds

**Dependencies**: US-CHT-005, US-CHT-006  
**Estimation Pointer**: Validated through testing, no separate implementation story

---

### NFR-CHT-002: Timing Accuracy

**Story Card:**
> **As a** Player
> **I want** note timing to be accurate to within 1 millisecond
> **So that** the game judges my input fairly and consistently across all BPM ranges and timing events

**Acceptance Criteria:**
*   **Given** a chart with timing conversions (beat ↔ time)
*   **When** any conversion is performed
*   **Then** the result is accurate to within 0.1 milliseconds compared to the mathematically correct value

**Dependencies**: US-CHT-003  
**Estimation Pointer**: Validated through unit tests, specified in US-CHT-003 already

---

### NFR-CHT-003: Memory Efficiency

**Story Card:**
> **As a** Developer
> **I want** Chart structs to use minimal memory
> **So that** the engine can hold hundreds of charts in the song database without excessive RAM usage

**Acceptance Criteria:**
*   **Given** a typical chart with 500 notes and 10 timing events
*   **When** the Chart struct is measured
*   **Then** it occupies less than 50 KB in memory

**Dependencies**: US-CHT-001, US-CHT-003, US-CHT-004  
**Estimation Pointer**: Validated through profiling, no separate implementation story

---

## Story Status Summary

### DONE
*None — this epic is entirely in PLANNED or FUTURE status*

### PLANNED (Phase 1)
- US-CHT-001 (3 pts) — Define Internal Chart Structure
- US-CHT-002 (2 pts) — Implement Chart Metadata Model
- US-CHT-003 (5 pts) — Implement Timing Data Model
- US-CHT-004 (3 pts) — Implement Note Data Model
- US-CHT-005 (5 pts) — Parse KSF Chart Format

**Phase 1 Subtotal: 18 points**

### PLANNED (Phase 3)
- US-CHT-018 (2 pts) — Store Preview Audio Information

**Phase 3 Subtotal: 2 points**

### PLANNED (Phase 4)
- US-CHT-006 (8 pts) — Parse SSC Chart Format
- US-CHT-007 (5 pts) — Parse SMA Chart Format
- US-CHT-008 (8 pts) — Parse STX Chart Format
- US-CHT-009 (8 pts) — Parse SEE Chart Format
- US-CHT-010 (13 pts) — Parse NX Chart Format
- US-CHT-011 (5 pts) — Specify OSF JSON Chart Format
- US-CHT-012 (5 pts) — Parse OSF JSON Chart Format
- US-CHT-013 (3 pts) — Write Charts to OSF Format
- US-CHT-014 (5 pts) — Compute Chart Content Hash
- US-CHT-015 (2 pts) — Define Canonical Binary Representation for Hashing
- US-CHT-016 (5 pts) — Validate Charts for Common Errors
- US-CHT-017 (3 pts) — Classify Chart Difficulty and Mode
- US-CHT-019 (5 pts) — Handle Multi-Chart Files (Inferred)

**Phase 4 Subtotal: 75 points**

### FUTURE
*None explicitly marked FUTURE in this epic — all stories are assigned to phases*

---

## Cross-Epic Dependencies

- **US-CHT-009** (SEE parser) depends on **US-SET-001** (settings system) for decryption key storage
- **US-CHT-014** (content hash) depends on **US-DAT-005** (profile high score storage) as mentioned in REQ-CHT-010
- All parsers depend on **US-AST-001** (asset discovery) for locating chart files in song directories (subsystem 11)
- **US-CHT-018** (preview audio) feeds into **US-AUD-003** (audio preview playback) in the audio system

---

## Personas

- **Player**: Wants to play songs, see accurate timing, and maintain high scores across file operations
- **Content Creator**: Wants to author charts in various formats and have them validated before distribution
- **Developer**: Wants a clean internal API, efficient data structures, and comprehensive test coverage