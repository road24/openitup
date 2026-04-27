# Asset Management Requirements

## REQ-AST-001: Texture Cache with Format Probing
**Status**: [DONE]  
**Priority**: Must Have

The engine must cache loaded textures and probe for .tga, .png, .dds formats in case-insensitive manner.

**Acceptance Criteria**:
- Textures loaded once and cached for reuse
- Case-insensitive file lookup works on Linux
- Probes .tga, .png, .dds in order
- Extension stripped before probing
- Missing textures logged at error level

**Dependencies**: REQ-REN-001  
**Source**: CLAUDE.md, Roadmap subsystem 11

---

## REQ-AST-002: Game Data Directory Configuration
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must discover game assets from one or more configured game data directories at startup.

**Acceptance Criteria**:
- Command-line argument for data directory path (Phase 1)
- Config file for multiple data directories (Phase 3+)
- Directories scanned recursively for song folders
- Invalid directories logged but don't prevent startup
- Environment variable override supported

**Dependencies**: None  
**Source**: Roadmap subsystem 11, Phase 1

---

## REQ-AST-003: Song Database Generation
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

The engine must build a song database during startup by scanning all song directories, with results cached to disk.

**Acceptance Criteria**:
- Recursive scan of data directories
- Each song folder parsed for charts, audio, banner, BGA
- Database contains: file paths, metadata, chart hashes
- Database cached to disk (rebuild only on directory change)
- Startup time under 5 seconds for 1000+ songs with warm cache

**Dependencies**: REQ-AST-002, REQ-CHT-001  
**Source**: Roadmap subsystem 11, Phase 3

---

## REQ-AST-004: Version Detection via Directory Structure
**Status**: [PLANNED Phase 7]  
**Priority**: Should Have

The asset manager must detect game version (Exceed, NX, Phoenix) using directory structure heuristics and file format presence.

**Acceptance Criteria**:
- .see files indicate Exceed-era data
- .nx files indicate NX-era data
- Directory naming conventions recognized
- Version detection per song folder (mixed versions supported)
- Version displayed in song select UI

**Dependencies**: REQ-AST-003  
**Source**: Roadmap subsystem 11, Phase 7

---

## REQ-AST-005: Note Skin Loading
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

The asset manager must load note skins from configured directories with manifest files defining sprite paths.

**Acceptance Criteria**:
- Skin directory contains manifest.json
- Manifest specifies sprites for each column, receptor, hold parts
- Multiple skins can be installed
- Skin selection persists in profile
- Missing skin falls back to default

**Dependencies**: REQ-AST-002, REQ-REN-009  
**Source**: Roadmap subsystem 11, Phase 5

---

## REQ-AST-006: Lazy Resource Loading
**Status**: [PLANNED Phase 2]  
**Priority**: Should Have

Resource loading must be lazy where possible: songs load on selection, not at startup.

**Acceptance Criteria**:
- Song BGAs loaded when song selected
- Song audio loaded when gameplay begins
- Note skins loaded once at gameplay start
- Chart data loaded when song selected
- Texture cache evicts unused textures (LRU)

**Dependencies**: REQ-AST-003  
**Source**: Roadmap subsystem 11

---

## REQ-AST-007: System Asset Discovery
**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

The asset manager must discover system assets (UI sprites, fonts, sound effects) from system asset directory.

**Acceptance Criteria**:
- System assets in dedicated directory (data/system/)
- Fonts for text rendering
- UI sprites and BGAs
- Judgment and menu sound effects
- System assets loaded at startup

**Dependencies**: REQ-AST-002  
**Source**: Roadmap subsystem 11 (implied)

---

## REQ-AST-008: Missing Asset Fallbacks
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must handle missing assets gracefully with appropriate fallbacks or disabling optional features.

**Acceptance Criteria**:
- Missing chart or audio: song not listed in database
- Missing banner: placeholder banner displayed
- Missing BGA: gameplay proceeds without BGA
- Missing note skin: default skin used
- All missing assets logged at ERROR or WARN level

**Dependencies**: REQ-AST-003  
**Source**: Roadmap scope notes

---

## REQ-AST-009: Multi-Version Data Coexistence
**Status**: [PLANNED Phase 7]  
**Priority**: Should Have

The asset manager must support multiple PIU version data directories simultaneously without conflicts.

**Acceptance Criteria**:
- Song databases from multiple versions merged
- Duplicate songs (same hash) handled gracefully
- Version-specific assets isolated
- User can filter song list by version
- No cross-version asset contamination

**Dependencies**: REQ-AST-003, REQ-AST-004  
**Source**: Roadmap subsystem 11, Phase 7

---

## REQ-AST-010: Asset Path Resolution
**Status**: [DONE]  
**Priority**: Must Have

Asset paths in data files must be resolved relative to the data file's location, supporting nested directory structures.

**Acceptance Criteria**:
- Relative paths work correctly
- Absolute paths supported but discouraged
- Parent directory references (..) work
- Path separators normalized (cross-platform)

**Dependencies**: REQ-AST-001  
**Source**: CLAUDE.md (texture loading)

---

## REQ-AST-011: Format Converter Tools
**Status**: [DONE]  
**Priority**: Should Have

The engine must provide standalone tools to convert original binary formats to JSON: spr2sprj, sp2->sprj, bga2bgaj.

**Acceptance Criteria**:
- spr2sprj converts SPR to SPRJ (requires textures)
- spr2sprj converts SP2 to SPRJ (no textures needed)
- bga2bgaj converts binary BGA to BGAJ
- Converters log errors for invalid input
- Converters produce valid JSON output

**Dependencies**: REQ-REN-003, REQ-REN-004  
**Source**: CLAUDE.md, Roadmap current state
