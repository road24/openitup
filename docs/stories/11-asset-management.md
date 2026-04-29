# Asset Management User Stories

## Status Legend
- **DONE**: Implemented and tested in codebase
- **PLANNED**: Scheduled for implementation in a specific phase
- **FUTURE**: Defined but not yet scheduled

---

# Epic: Texture Management

## Story ID: US-AST-001 - Texture cache for reuse

**Story Card:**
> **As a** Developer
> **I want** texture memory management with automatic caching
> **So that** identical textures load once and reuse reduces memory consumption

### Description
The engine must cache loaded textures keyed by canonical file path. Repeated loads of the same texture return the cached handle without disk I/O or GPU allocation.

### Acceptance Criteria

*   **Scenario 1: First load creates cache entry**
    *   **Given** the texture cache is empty
    *   **When** a sprite requests texture "arrow.tga" from "/data/sprites/"
    *   **Then** the system loads the file, uploads to GPU, stores the handle, and returns width 64 height 64

*   **Scenario 2: Second load returns cached handle**
    *   **Given** texture "arrow.tga" from "/data/sprites/" is already cached
    *   **When** another sprite requests the same texture
    *   **Then** the system returns the cached handle without file I/O within 1 millisecond

*   **Scenario 3: Cache tracks unique textures**
    *   **Given** 3 sprites reference "arrow.tga" and 2 sprites reference "note.png"
    *   **When** all sprites are loaded
    *   **Then** the cache contains exactly 2 texture entries

### Technical Notes & Constraints
*   **Status**: DONE
*   **Estimation Pointer**: N/A (already implemented)
*   **Dependencies**: None
*   **Implementation**: `src/openitup/gfx/texture_cache.h`, `src/openitup/gfx/texture_cache.cpp`
*   **Tests**: `test/test_texture_cache.cpp`, 41 integration tests

---

## Story ID: US-AST-002 - Case-insensitive texture file lookup

**Story Card:**
> **As a** Content Creator
> **I want** texture file names matched without case sensitivity
> **So that** assets from Windows game data work on Linux without renaming files

### Description
Original game assets use mixed-case file names inconsistently. The texture cache must probe for files case-insensitively on case-sensitive filesystems.

### Acceptance Criteria

*   **Scenario 1: Exact case match preferred**
    *   **Given** "/data/" contains "Arrow.tga" and "arrow.tga"
    *   **When** a sprite requests "arrow.tga"
    *   **Then** the system loads "/data/arrow.tga" without scanning the directory

*   **Scenario 2: Case-insensitive fallback**
    *   **Given** "/data/" contains only "ARROW.TGA"
    *   **When** a sprite requests "arrow.tga"
    *   **Then** the system scans the directory, finds "ARROW.TGA", and loads it successfully

*   **Scenario 3: Case-insensitive applies to extension**
    *   **Given** "/data/" contains "note.PNG"
    *   **When** a sprite requests "note.tga"
    *   **Then** the system finds "note.PNG" during probe and loads it

### Technical Notes & Constraints
*   **Status**: DONE
*   **Estimation Pointer**: N/A (already implemented)
*   **Dependencies**: US-AST-001
*   **Implementation**: `TextureCache::probe()` in `texture_cache.cpp` lines 26-54

---

## Story ID: US-AST-003 - Format probing for texture extensions

**Story Card:**
> **As a** Content Creator
> **I want** texture references probed across multiple formats
> **So that** sprite files reference base names without hardcoding extensions

### Description
Format files reference textures with .tga extension as a hint, but actual files may be .tga, .png, or .dds. The loader must probe in priority order.

### Acceptance Criteria

*   **Scenario 1: Probe order is .tga then .png then .dds**
    *   **Given** "/data/" contains "arrow.png" and "arrow.dds"
    *   **When** a sprite requests "arrow.tga"
    *   **Then** the system loads "/data/arrow.png" before checking .dds

*   **Scenario 2: Extension stripped before probing**
    *   **Given** a sprite references "note.TGA"
    *   **When** the texture is requested
    *   **Then** the system strips ".TGA", then probes "note.tga", "note.png", "note.dds" in order

*   **Scenario 3: Missing texture logs error**
    *   **Given** "/data/" contains no files matching "missing" with any probed extension
    *   **When** a sprite requests "missing.tga"
    *   **Then** the system logs "texture not found: 'missing.tga' in /data/ (probed .tga/.png/.dds, case-insensitive)" at ERROR level

### Technical Notes & Constraints
*   **Status**: DONE
*   **Estimation Pointer**: N/A (already implemented)
*   **Dependencies**: US-AST-002
*   **Probed extensions**: .tga, .png, .dds (defined in `kProbeExtensions`)

---

## Story ID: US-AST-004 - Texture cache memory release

**Story Card:**
> **As a** Player
> **I want** unused textures freed from GPU memory
> **So that** long play sessions do not exhaust VRAM

### Description
The texture cache must provide a method to destroy all cached textures, releasing GPU resources. Individual texture eviction is out of scope for Phase 1.

### Acceptance Criteria

*   **Scenario 1: Clear destroys all textures**
    *   **Given** the cache contains 5 textures consuming 2 MB GPU memory
    *   **When** `clear()` is called
    *   **Then** all SDL_Texture objects are destroyed and cache size returns 0

*   **Scenario 2: Clear allows reload**
    *   **Given** the cache was cleared after loading "arrow.tga"
    *   **When** a sprite requests "arrow.tga" again
    *   **Then** the system loads and uploads the texture as a new cache entry

### Technical Notes & Constraints
*   **Status**: DONE
*   **Estimation Pointer**: N/A (already implemented)
*   **Dependencies**: US-AST-001
*   **Future work**: LRU eviction for automatic memory management (US-AST-018)

---

# Epic: Asset Path Resolution

## Story ID: US-AST-005 - Relative path resolution from data files

**Story Card:**
> **As a** Content Creator
> **I want** texture paths resolved relative to sprite file location
> **So that** nested directory structures work without absolute paths

### Description
When a .sprj file references "textures/arrow.tga", the path must resolve relative to the .sprj file's directory, not the engine's working directory.

### Acceptance Criteria

*   **Scenario 1: Relative path from sprite directory**
    *   **Given** "/data/sprites/arrows.sprj" references "textures/arrow.tga"
    *   **When** the sprite is loaded
    *   **Then** the system resolves to "/data/sprites/textures/arrow.tga"

*   **Scenario 2: Parent directory references work**
    *   **Given** "/data/stage1/sprites/bg.sprj" references "../../shared/sky.png"
    *   **When** the sprite is loaded
    *   **Then** the system resolves to "/data/shared/sky.png"

*   **Scenario 3: Path separators normalized**
    *   **Given** a .sprj file on Linux contains Windows path "textures\arrow.tga"
    *   **When** the sprite is loaded
    *   **Then** the system converts backslashes to forward slashes and resolves correctly

### Technical Notes & Constraints
*   **Status**: DONE
*   **Estimation Pointer**: N/A (already implemented)
*   **Dependencies**: US-AST-001
*   **Implementation**: `std::filesystem::path` handles normalization automatically

---

# Epic: Format Conversion Tools

## Story ID: US-AST-006 - SPR to SPRJ converter

**Story Card:**
> **As a** Content Creator
> **I want** a command-line tool converting SPR to SPRJ
> **So that** original Exceed-era sprites work in the engine

### Description
The `spr2sprj` tool must parse text SPR format and output JSON SPRJ format. UV normalization requires actual texture dimensions, so texture files must be present.

### Acceptance Criteria

*   **Scenario 1: Successful conversion with textures present**
    *   **Given** "arrow.spr" references "arrow.tga" with pixel coords and "/data/textures/arrow.tga" exists with dimensions 256x256
    *   **When** the command `spr2sprj arrow.spr arrow.sprj --asset-dir /data/textures/` runs
    *   **Then** the tool writes valid "arrow.sprj" with UVs normalized to 0.0–1.0 range

*   **Scenario 2: Missing texture causes error**
    *   **Given** "bg.spr" references "sky.tga" and the file does not exist in search paths
    *   **When** `spr2sprj bg.spr bg.sprj` runs
    *   **Then** the tool logs "texture not found: 'sky.tga'" at ERROR level and exits with code 1

*   **Scenario 3: Invalid SPR format logged**
    *   **Given** "corrupt.spr" contains non-numeric values in PICTURE coordinate lines
    *   **When** `spr2sprj corrupt.spr output.sprj` runs
    *   **Then** the tool logs parse error with line number and exits with code 1

### Technical Notes & Constraints
*   **Status**: DONE
*   **Estimation Pointer**: N/A (already implemented)
*   **Dependencies**: US-AST-001, US-AST-002, US-AST-003
*   **Binary**: `build/spr2sprj`

---

## Story ID: US-AST-007 - SP2 to SPRJ converter

**Story Card:**
> **As a** Content Creator
> **I want** a command-line tool converting SP2 to SPRJ
> **So that** later-era sprites with named pictures work in the engine

### Description
SP2 uses fixed 256x256 grid UVs, so texture files are not required for conversion. The same `spr2sprj` binary handles both formats via file extension detection.

### Acceptance Criteria

*   **Scenario 1: SP2 conversion without texture files**
    *   **Given** "notes.sp2" uses 256-grid UV encoding
    *   **When** `spr2sprj notes.sp2 notes.sprj` runs
    *   **Then** the tool writes valid "notes.sprj" with normalized UVs without requiring texture files

*   **Scenario 2: Named pictures preserved**
    *   **Given** "combo.sp2" contains PICTURE entries with names "0", "1", "2"
    *   **When** converted to SPRJ
    *   **Then** the JSON "pictures" array contains objects with "name" fields "0", "1", "2"

*   **Scenario 3: PATTERN grid dimensions converted**
    *   **Given** "digits.sp2" has `TYPE PATTERN RIGHT 2 5`
    *   **When** converted to SPRJ
    *   **Then** the JSON contains `"grid_rows": 2, "grid_cols": 5, "direction": "right"`

### Technical Notes & Constraints
*   **Status**: DONE
*   **Estimation Pointer**: N/A (already implemented)
*   **Dependencies**: None (pure text processing)
*   **Binary**: `build/spr2sprj` (same binary handles both formats)

---

## Story ID: US-AST-008 - BGA binary to JSON converter

**Story Card:**
> **As a** Content Creator
> **I want** a command-line tool converting binary BGA to BGAJ
> **So that** original animation files work with the JSON-based engine

### Description
The `bga2bgaj` tool parses binary BGA format and outputs BGAJ. No external dependencies required since it is pure binary parsing.

### Acceptance Criteria

*   **Scenario 1: Complete BGA conversion**
    *   **Given** "title.bga" is a valid 1024-byte BGA file with 50 layers and 5 keyframes
    *   **When** `bga2bgaj title.bga title.bgaj` runs
    *   **Then** the tool writes "title.bgaj" with all layers, keyframes, and properties correctly converted

*   **Scenario 2: Invalid magic number rejected**
    *   **Given** "bad.bga" has header magic "BGAX" instead of "BGA2"
    *   **When** `bga2bgaj bad.bga out.bgaj` runs
    *   **Then** the tool logs "invalid BGA magic" and exits with code 1

*   **Scenario 3: Inactive layers omitted**
    *   **Given** "partial.bga" has 50 layers where layers 10–49 have empty sprite names
    *   **When** converted to BGAJ
    *   **Then** the JSON "layers" array contains only 10 entries (active layers)

### Technical Notes & Constraints
*   **Status**: DONE
*   **Estimation Pointer**: N/A (already implemented)
*   **Dependencies**: None
*   **Binary**: `build/bga2bgaj`

---

# Epic: Game Data Directory Discovery

## Story ID: US-AST-009 - Command-line data directory argument

**Story Card:**
> **As a** Player
> **I want** the engine to accept a data directory path at launch
> **So that** I can point it at my extracted game files

### Description
Phase 1 requires a single hardcoded or command-line specified path to a song directory. This unblocks the first playable build.

### Acceptance Criteria

*   **Scenario 1: Launch with data directory**
    *   **Given** "/opt/piu/songs/Pumptris/" contains "pumptris.ksf" and "pumptris.ogg"
    *   **When** the command `./openitup --data-dir /opt/piu/songs/Pumptris/` runs
    *   **Then** the engine loads the chart and audio without scanning other directories

*   **Scenario 2: Missing directory logs error**
    *   **Given** "/opt/piu/songs/Missing/" does not exist
    *   **When** `./openitup --data-dir /opt/piu/songs/Missing/` runs
    *   **Then** the engine logs "data directory not found: /opt/piu/songs/Missing/" at ERROR level and exits with code 1

*   **Scenario 3: Directory without assets logs warning**
    *   **Given** "/empty/" exists but contains no .ksf or .ogg files
    *   **When** `./openitup --data-dir /empty/` runs
    *   **Then** the engine logs "no valid songs found in /empty/" at WARN level and continues to title screen

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 1
*   **Estimation Pointer**: 2
*   **Dependencies**: None
*   **Future work**: US-AST-010 adds config file for multiple directories

---

## Story ID: US-AST-010 - Environment variable for data directory

**Story Card:**
> **As a** Player
> **I want** the engine to read data directory from environment variable
> **So that** I do not repeat the path argument every launch

### Description
If `OPENITUP_DATA_DIR` is set, use it as the default data directory. Command-line argument overrides environment variable.

### Acceptance Criteria

*   **Scenario 1: Environment variable used**
    *   **Given** `OPENITUP_DATA_DIR=/home/user/piu/`
    *   **When** `./openitup` runs without `--data-dir`
    *   **Then** the engine loads songs from "/home/user/piu/"

*   **Scenario 2: Command-line overrides environment**
    *   **Given** `OPENITUP_DATA_DIR=/home/user/piu/` and command `./openitup --data-dir /opt/piu/`
    *   **When** the engine starts
    *   **Then** it uses "/opt/piu/" and ignores the environment variable

*   **Scenario 3: No directory specified logs error**
    *   **Given** `OPENITUP_DATA_DIR` is unset and no `--data-dir` argument
    *   **When** `./openitup` runs
    *   **Then** the engine logs "no data directory specified (use --data-dir or set OPENITUP_DATA_DIR)" at ERROR level and exits with code 1

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 1
*   **Estimation Pointer**: 1
*   **Dependencies**: US-AST-009

---

## Story ID: US-AST-011 - Config file for multiple data directories

**Story Card:**
> **As a** Player
> **I want** a config file listing multiple data directories
> **So that** the engine discovers songs from all my PIU installations

### Description
The config file `~/.config/openitup/settings.json` contains a "data_dirs" array. All directories are scanned and merged into the song database.

### Acceptance Criteria

*   **Scenario 1: Multiple directories scanned**
    *   **Given** settings.json contains `"data_dirs": ["/opt/exceed/", "/home/user/nx/"]`
    *   **When** the engine starts
    *   **Then** the song database includes songs from both directories

*   **Scenario 2: Invalid directory logged but not fatal**
    *   **Given** settings.json contains `"data_dirs": ["/valid/", "/missing/", "/another/"]` where "/missing/" does not exist
    *   **When** the engine starts
    *   **Then** it logs "directory not found: /missing/" at WARN level and scans "/valid/" and "/another/"

*   **Scenario 3: Empty config array uses fallback**
    *   **Given** settings.json contains `"data_dirs": []`
    *   **When** the engine starts
    *   **Then** it falls back to command-line `--data-dir` or `OPENITUP_DATA_DIR` environment variable

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 3
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-009, US-AST-012 (song database)

---

# Epic: Song Database

## Story ID: US-AST-012 - Recursive directory scan for songs

**Story Card:**
> **As a** Player
> **I want** the engine to find all songs in nested directories
> **So that** I do not manually list each song folder

### Description
The engine scans configured data directories recursively. Any directory containing at least one chart file (.ksf, .ssc, .see, .nx) is a song folder.

### Acceptance Criteria

*   **Scenario 1: Nested song folders discovered**
    *   **Given** "/data/" contains "exceed/Pumptris/pumptris.ksf" and "nx/Sorceress/sorceress.nx"
    *   **When** the engine scans "/data/"
    *   **Then** the song database contains 2 entries: "Pumptris" and "Sorceress"

*   **Scenario 2: Multiple chart files in one folder**
    *   **Given** "/data/Pumptris/" contains "pumptris.ksf" and "pumptris.ssc"
    *   **When** the engine scans "/data/"
    *   **Then** the song "Pumptris" has 2 chart entries (one per format)

*   **Scenario 3: Directories without charts skipped**
    *   **Given** "/data/" contains subdirectories "songs/", "videos/", "system/" where only "songs/" contains .ksf files
    *   **When** the engine scans "/data/"
    *   **Then** the database includes only songs from "songs/" subdirectory

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 3
*   **Estimation Pointer**: 5
*   **Dependencies**: US-AST-011 (config file for dirs)
*   **Chart formats**: .ksf (Phase 1), .ssc/.sma/.stx/.see/.nx (Phase 4)

---

## Story ID: US-AST-013 - Song metadata extraction during scan

**Story Card:**
> **As a** Player
> **I want** song titles, artists, and BPM displayed in song select
> **So that** I choose songs without opening each file

### Description
During directory scan, the engine parses each chart file's header to extract metadata without loading full note data. Results are stored in the song database.

### Acceptance Criteria

*   **Scenario 1: KSF metadata extracted**
    *   **Given** "pumptris.ksf" contains `#TITLE:Pumptris;`, `#ARTIST:BanYa;`, `#BPM:145;`
    *   **When** the song folder is scanned
    *   **Then** the database entry contains title="Pumptris", artist="BanYa", bpm=145.0

*   **Scenario 2: Metadata with special characters**
    *   **Given** a .ssc file contains `#TITLE:Āçčėñt Tëst;`
    *   **When** scanned
    *   **Then** the database preserves UTF-8 title "Āçčėñt Tëst" without corruption

*   **Scenario 3: Missing metadata uses defaults**
    *   **Given** "old.ksf" has no `#ARTIST` tag
    *   **When** scanned
    *   **Then** the database entry contains artist="Unknown"

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 3
*   **Estimation Pointer**: 5
*   **Dependencies**: US-AST-012, Chart Parser (subsystem 4 Phase 1)

---

## Story ID: US-AST-014 - Cached song database for startup performance

**Story Card:**
> **As a** Player
> **I want** the engine to skip directory scan when data has not changed
> **So that** startup takes under 3 seconds for 1000 songs

### Description
After first scan, the engine writes a cache file containing the full song database. On next launch, cache is loaded if data directories have not changed.

### Acceptance Criteria

*   **Scenario 1: Cold start scans and caches**
    *   **Given** no cache exists and "/data/" contains 1000 song folders
    *   **When** the engine starts
    *   **Then** it scans all directories in under 10 seconds and writes cache to "~/.cache/openitup/songdb.json"

*   **Scenario 2: Warm start loads cache**
    *   **Given** cache exists and data directories have not changed since last run
    *   **When** the engine starts
    *   **Then** it loads the cache in under 500 milliseconds without scanning directories

*   **Scenario 3: Cache invalidated on directory change**
    *   **Given** cache exists but user added new directory to settings.json
    *   **When** the engine starts
    *   **Then** it detects cache is stale, rescans all directories, and updates the cache

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 3
*   **Estimation Pointer**: 5
*   **Dependencies**: US-AST-013
*   **Cache invalidation**: directory list change or mtime of any data directory newer than cache file

---

## Story ID: US-AST-015 - Banner and audio file discovery

**Story Card:**
> **As a** Player
> **I want** song banners displayed in song select
> **So that** I identify songs visually

### Description
During directory scan, the engine searches for banner image files and audio files in each song folder. Paths are stored in the database.

### Acceptance Criteria

*   **Scenario 1: Banner discovered by naming convention**
    *   **Given** "/data/Pumptris/" contains "pumptris.ksf", "pumptris.ogg", and "pumptris_banner.png"
    *   **When** the song is scanned
    *   **Then** the database entry contains banner_path="/data/Pumptris/pumptris_banner.png"

*   **Scenario 2: Audio file matched by extension**
    *   **Given** a song folder contains "song.ksf" and "song.mp3" (no .ogg)
    *   **When** scanned
    *   **Then** the database entry contains audio_path="song.mp3"

*   **Scenario 3: Missing banner uses placeholder**
    *   **Given** a song folder contains only .ksf and .ogg files (no banner)
    *   **When** scanned
    *   **Then** the database entry contains banner_path=null and the UI displays a default placeholder

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 3
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-012
*   **Naming conventions**: `<stem>_banner.{png,jpg}`, fallback to `banner.{png,jpg}`

---

## Story ID: US-AST-016 - BGA file discovery per song

**Story Card:**
> **As a** Player
> **I want** background animations to play during gameplay
> **So that** the game experience matches the original

### Description
The scan discovers .bga or .bgaj files in song folders and stores the path. BGAs are loaded lazily when the player selects the song.

### Acceptance Criteria

*   **Scenario 1: BGA discovered and associated with chart**
    *   **Given** "/data/Pumptris/" contains "pumptris.ksf" and "pumptris.bga"
    *   **When** scanned
    *   **Then** the database entry contains bga_path="/data/Pumptris/pumptris.bga"

*   **Scenario 2: Missing BGA is not an error**
    *   **Given** a song folder contains only .ksf and .ogg (no .bga or .bgaj)
    *   **When** scanned
    *   **Then** the database entry contains bga_path=null and gameplay proceeds without background animation

*   **Scenario 3: BGAJ preferred over BGA**
    *   **Given** a song folder contains both "song.bga" and "song.bgaj"
    *   **When** scanned
    *   **Then** the database stores bga_path="song.bgaj" (JSON format preferred)

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 3
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-012
*   **Lazy loading**: BGAs are not loaded during scan, only when selected in song select (US-AST-020)

---

# Epic: Lazy Resource Loading

## Story ID: US-AST-017 - Chart loaded on song selection

**Story Card:**
> **As a** Player
> **I want** chart data loaded when I select a song
> **So that** startup completes quickly without loading all charts

### Description
The song database scan extracts only metadata. Full chart data (note positions, timing events) is loaded when the player selects the song in song select.

### Acceptance Criteria

*   **Scenario 1: Chart not loaded during startup**
    *   **Given** the engine starts with 1000 songs in the database
    *   **When** the song database finishes loading
    *   **Then** memory usage is under 50 MB (metadata only, no note data)

*   **Scenario 2: Chart loaded on selection**
    *   **Given** the player navigates to "Pumptris" in song select
    *   **When** the player presses SELECT
    *   **Then** the engine loads "pumptris.ksf" and parses full note data within 200 milliseconds

*   **Scenario 3: Invalid chart logged on load attempt**
    *   **Given** database contains entry for "corrupt.ksf" with invalid syntax
    *   **When** the player selects it
    *   **Then** the engine logs "failed to parse chart: corrupt.ksf" at ERROR level and returns to song select

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 2
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-013, Chart System (subsystem 4 Phase 1)

---

## Story ID: US-AST-018 - Texture cache LRU eviction

**Story Card:**
> **As a** Player
> **I want** old textures freed when memory is constrained
> **So that** playing many songs does not cause out-of-memory crashes

### Description
The texture cache tracks access times and evicts least-recently-used textures when total GPU memory usage exceeds a threshold.

### Acceptance Criteria

*   **Scenario 1: Eviction triggered by memory threshold**
    *   **Given** the cache contains 50 textures consuming 100 MB and the threshold is 80 MB
    *   **When** a new texture is loaded requiring 5 MB
    *   **Then** the cache evicts least-recently-used textures until memory stays below the threshold before loading the new one

*   **Scenario 2: Evicted texture reloads on next use**
    *   **Given** texture "arrow.tga" was evicted 5 minutes ago
    *   **When** a sprite requests "arrow.tga"
    *   **Then** the cache treats it as a cache miss, reloads from disk, and returns a new handle

*   **Scenario 3: Actively rendered textures not evicted**
    *   **Given** a BGA animation has been rendering for 60 seconds using 10 textures
    *   **When** memory pressure triggers eviction
    *   **Then** the cache evicts only textures not used in the last 5 seconds

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 2
*   **Estimation Pointer**: 5
*   **Dependencies**: US-AST-001
*   **Default threshold**: 200 MB GPU memory, configurable in settings.json

---

## Story ID: US-AST-019 - Audio loaded at gameplay start

**Story Card:**
> **As a** Player
> **I want** song audio loaded when gameplay begins
> **So that** song selection is instantaneous

### Description
Audio files are not loaded when the song is selected. Loading happens during the transition from song select to gameplay, covering load time with a fade animation.

### Acceptance Criteria

*   **Scenario 1: Audio not loaded during song select**
    *   **Given** the player navigates through 20 songs in song select
    *   **When** memory usage is measured
    *   **Then** no audio data is loaded (only metadata)

*   **Scenario 2: Audio loaded during gameplay transition**
    *   **Given** the player selects "Pumptris" and presses START
    *   **When** the transition animation begins
    *   **Then** the engine loads "pumptris.ogg" (15 MB) during the 2-second fade

*   **Scenario 3: Audio ready before gameplay begins**
    *   **Given** gameplay transition is complete
    *   **When** the note field appears
    *   **Then** audio playback starts immediately within 16 milliseconds

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 2
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-017, Audio System (subsystem 3 Phase 1)

---

## Story ID: US-AST-020 - BGA loaded on song selection

**Story Card:**
> **As a** Player
> **I want** background animation ready when gameplay starts
> **So that** the BGA plays in sync with audio from the first frame

### Description
BGA files are loaded when the player selects a song, not during directory scan. This balances responsiveness (loaded before gameplay) with startup speed (not loaded until needed).

### Acceptance Criteria

*   **Scenario 1: BGA loaded after song selection**
    *   **Given** the player selects "Pumptris" which has a 500 KB .bgaj file
    *   **When** the song select screen displays the banner
    *   **Then** the engine loads and parses the BGA within 100 milliseconds

*   **Scenario 2: BGA textures loaded during selection**
    *   **Given** the BGA references 5 sprite files requiring 10 textures
    *   **When** the BGA is loaded
    *   **Then** all textures are loaded and cached before gameplay starts

*   **Scenario 3: Missing BGA handled gracefully**
    *   **Given** database entry has bga_path=null
    *   **When** the player selects the song
    *   **Then** no BGA loading is attempted and gameplay proceeds with black background

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 2
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-016, BGA Animation System (exists)

---

# Epic: System Assets

## Story ID: US-AST-021 - System asset directory structure

**Story Card:**
> **As a** Developer
> **I want** system assets separated from song data
> **So that** UI sprites and fonts are not mixed with user content

### Description
The engine defines a system asset directory `data/system/` containing UI sprites, fonts, judgment sound effects, and menu BGAs. This directory is distributed with the engine.

### Acceptance Criteria

*   **Scenario 1: System directory loaded at startup**
    *   **Given** "data/system/" contains "fonts/default.ttf" and "sprites/menu.sprj"
    *   **When** the engine starts
    *   **Then** system assets are loaded before the title screen appears

*   **Scenario 2: Missing system assets are fatal**
    *   **Given** "data/system/fonts/" is empty
    *   **When** the engine starts
    *   **Then** it logs "system assets not found: fonts/default.ttf" at ERROR level and exits with code 1

*   **Scenario 3: System assets cached separately**
    *   **Given** system textures consume 10 MB
    *   **When** song textures trigger LRU eviction
    *   **Then** system textures are marked as pinned and never evicted

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 2
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-001, US-AST-009
*   **Directory location**: Relative to engine binary or configurable via `--system-dir`

---

## Story ID: US-AST-022 - Font loading for text rendering

**Story Card:**
> **As a** Player
> **I want** text rendered for song titles, scores, and menus
> **So that** the interface is readable

### Description
The engine loads TrueType fonts from `data/system/fonts/` at startup. Text rendering uses SDL3_ttf (or similar) for rasterization.

### Acceptance Criteria

*   **Scenario 1: Default font loaded**
    *   **Given** "data/system/fonts/default.ttf" exists
    *   **When** the engine starts
    *   **Then** the font is loaded and available to all screens

*   **Scenario 2: Text rendered with correct glyph coverage**
    *   **Given** a song title contains "Āçčėñt Tëst Sǒng 日本語"
    *   **When** displayed in song select
    *   **Then** all glyphs render correctly without missing character boxes

*   **Scenario 3: Missing font falls back to system default**
    *   **Given** "data/system/fonts/default.ttf" does not exist
    *   **When** the engine starts
    *   **Then** it logs "font not found, using system default" at WARN level and uses platform fallback font

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 2
*   **Estimation Pointer**: 5
*   **Dependencies**: US-AST-021
*   **Font rendering**: Consider SDL3_ttf or FreeType directly

---

## Story ID: US-AST-023 - Judgment and menu sound effects

**Story Card:**
> **As a** Player
> **I want** sound feedback for panel presses and menu navigation
> **So that** interaction feels responsive

### Description
System sound effects are loaded from `data/system/sfx/` at startup. Short samples (under 500 KB each) are fully loaded into memory for instant playback.

### Acceptance Criteria

*   **Scenario 1: Key sound plays on panel press**
    *   **Given** "data/system/sfx/key.wav" is loaded
    *   **When** the player presses a dance panel during gameplay
    *   **Then** the sound plays within 10 milliseconds of the press

*   **Scenario 2: Judgment sounds match timing**
    *   **Given** "data/system/sfx/perfect.wav" and "sfx/miss.wav" are loaded
    *   **When** the judge returns a Perfect judgment
    *   **Then** "perfect.wav" plays immediately

*   **Scenario 3: Menu navigation sounds**
    *   **Given** "data/system/sfx/cursor.wav" and "sfx/select.wav" are loaded
    *   **When** the player presses UP in song select
    *   **Then** "cursor.wav" plays within 10 milliseconds

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 3
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-021, Audio System (subsystem 3 Phase 3)

---

# Epic: Note Skin Management

## Story ID: US-AST-024 - Note skin directory structure

**Story Card:**
> **As a** Content Creator
> **I want** note skins packaged as self-contained directories
> **So that** I distribute custom skins without modifying the engine

### Description
Each note skin is a directory containing a manifest.json file plus sprite files. The manifest declares paths to sprites for each column, receptors, holds, and judgment displays.

### Acceptance Criteria

*   **Scenario 1: Manifest declares required sprites**
    *   **Given** "data/skins/default/manifest.json" contains `"note_sprites": {"down_left": "arrows/dl.sprj", ...}`
    *   **When** the skin is loaded
    *   **Then** the engine loads all 5 column sprites for single mode

*   **Scenario 2: Missing sprite in manifest is fatal for that skin**
    *   **Given** "data/skins/broken/manifest.json" references "arrows/missing.sprj" which does not exist
    *   **When** the player selects "broken" skin
    *   **Then** the engine logs "skin error: missing sprite arrows/missing.sprj" at ERROR level and falls back to default skin

*   **Scenario 3: Manifest specifies receptor positions**
    *   **Given** manifest contains `"receptor_y": 400, "note_spacing": 64`
    *   **When** the note renderer is initialized
    *   **Then** receptors are positioned at Y=400 with 64 pixels between columns

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 5
*   **Estimation Pointer**: 5
*   **Dependencies**: US-AST-021, Note Renderer (subsystem 6 Phase 2)

---

## Story ID: US-AST-025 - Multiple note skins installed

**Story Card:**
> **As a** Player
> **I want** the engine to discover all installed note skins
> **So that** I choose my preferred visual style

### Description
The engine scans `data/skins/` at startup and enumerates all valid skin directories. The list is presented in settings.

### Acceptance Criteria

*   **Scenario 1: All valid skins discovered**
    *   **Given** "data/skins/" contains "default/", "pixel/", "neon/" where all have valid manifest.json
    *   **When** the engine starts
    *   **Then** the settings screen displays 3 skins: "default", "pixel", "neon"

*   **Scenario 2: Invalid skins skipped with warning**
    *   **Given** "data/skins/broken/" exists but manifest.json is malformed
    *   **When** the engine scans skins
    *   **Then** it logs "invalid skin manifest: data/skins/broken/manifest.json" at WARN level and excludes it from the list

*   **Scenario 3: Empty skins directory uses default**
    *   **Given** "data/skins/" is empty
    *   **When** the engine starts
    *   **Then** it logs "no skins found, using built-in default" at WARN level and uses hardcoded placeholder graphics

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 5
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-024

---

## Story ID: US-AST-026 - Selected note skin persisted in profile

**Story Card:**
> **As a** Player
> **I want** my note skin choice saved
> **So that** the engine remembers my preference across sessions

### Description
The player profile stores the selected note skin name. On next launch, the engine loads that skin for gameplay.

### Acceptance Criteria

*   **Scenario 1: Skin selection saved**
    *   **Given** the player selects "neon" skin in settings
    *   **When** the player exits the game
    *   **Then** the profile file contains `"note_skin": "neon"`

*   **Scenario 2: Saved skin loaded on startup**
    *   **Given** the profile contains `"note_skin": "pixel"`
    *   **When** the engine starts
    *   **Then** the "pixel" skin is loaded and used in gameplay

*   **Scenario 3: Missing saved skin falls back to default**
    *   **Given** the profile contains `"note_skin": "deleted"` but that skin no longer exists
    *   **When** the engine starts
    *   **Then** it logs "skin not found: deleted, using default" at WARN level and loads "default" skin

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 5
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-025, Profile System (subsystem 9 Phase 5)

---

# Epic: Version Detection

## Story ID: US-AST-027 - Exceed-era detection via .see files

**Story Card:**
> **As a** Content Creator
> **I want** Exceed-era songs recognized automatically
> **So that** version-appropriate judge rules apply

### Description
The presence of .see chart files indicates Exceed or Exceed2 era data. The song database tags these songs with version metadata.

### Acceptance Criteria

*   **Scenario 1: .see file tags song as Exceed**
    *   **Given** "/data/songs/Canon-D/" contains "canon.see"
    *   **When** the directory is scanned
    *   **Then** the database entry contains version="Exceed"

*   **Scenario 2: Version displayed in song select**
    *   **Given** the player navigates to a song with version="Exceed"
    *   **When** the banner is displayed
    *   **Then** the UI shows an "Exceed" badge in the corner

*   **Scenario 3: Multiple versions in same folder**
    *   **Given** a song folder contains both "song.see" and "song.ssc"
    *   **When** scanned
    *   **Then** the database creates 2 chart entries: one tagged "Exceed", one tagged "StepMania"

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 7
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-012
*   **Version tags**: Exceed, NX, Phoenix, StepMania (for .ssc/.ksf), Unknown

---

## Story ID: US-AST-028 - NX-era detection via .nx files

**Story Card:**
> **As a** Content Creator
> **I want** NX-era songs recognized automatically
> **So that** NX-specific chart features are supported

### Description
The presence of .nx files indicates NX, NX2, NXA, or Fiesta data. Directory naming conventions further refine the version.

### Acceptance Criteria

*   **Scenario 1: .nx file tags song as NX**
    *   **Given** "/data/songs/Sorceress/" contains "sorceress.nx"
    *   **When** scanned
    *   **Then** the database entry contains version="NX"

*   **Scenario 2: Subdirectory naming refines version**
    *   **Given** "/data/NX2/Sorceress/" contains "sorceress.nx"
    *   **When** scanned
    *   **Then** the database entry contains version="NX2" based on parent directory name

*   **Scenario 3: Version filter in song select**
    *   **Given** the database contains 50 Exceed songs and 30 NX songs
    *   **When** the player enables "Show only: NX" filter
    *   **Then** song select displays only the 30 NX songs

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 7
*   **Estimation Pointer**: 2
*   **Dependencies**: US-AST-027

---

## Story ID: US-AST-029 - Multi-version data coexistence

**Story Card:**
> **As a** Player
> **I want** songs from different PIU versions in the same database
> **So that** I play any song without swapping data directories

### Description
The song database merges results from all configured data directories, preserving version tags. Duplicate detection uses chart content hash (defined in subsystem 4).

### Acceptance Criteria

*   **Scenario 1: Songs from multiple versions merged**
    *   **Given** "/opt/exceed/" contains 100 Exceed songs and "/opt/nx/" contains 80 NX songs
    *   **When** both directories are configured and scanned
    *   **Then** the song database contains 180 total entries with correct version tags

*   **Scenario 2: Duplicate chart detected by hash**
    *   **Given** "/opt/exceed/Pumptris/" and "/opt/stepmania/Pumptris/" contain identical note data with different file formats
    *   **When** scanned
    *   **Then** the database contains 1 chart entry with 2 file paths (allowing format preference)

*   **Scenario 3: Version-specific assets isolated**
    *   **Given** Exceed and NX data directories both contain "system/bgm/title.ogg"
    *   **When** loading title screen assets for Exceed game mode
    *   **Then** the engine loads from the Exceed data directory, not NX

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 7
*   **Estimation Pointer**: 5
*   **Dependencies**: US-AST-028, Chart System (subsystem 4 Phase 4)

---

# Epic: Missing Asset Handling

## Story ID: US-AST-030 - Missing chart or audio excludes song

**Story Card:**
> **As a** Player
> **I want** songs without playable charts hidden from song select
> **So that** I do not select unplayable entries

### Description
During directory scan, if a song folder has no valid chart file or no audio file, it is excluded from the database.

### Acceptance Criteria

*   **Scenario 1: Song with chart and audio included**
    *   **Given** "/data/Pumptris/" contains "pumptris.ksf" and "pumptris.ogg"
    *   **When** scanned
    *   **Then** "Pumptris" appears in song select

*   **Scenario 2: Song without audio excluded**
    *   **Given** "/data/Broken/" contains "broken.ksf" but no .ogg or .mp3 file
    *   **When** scanned
    *   **Then** "Broken" is excluded from song select and logged "song excluded: missing audio file" at WARN level

*   **Scenario 3: Song without chart excluded**
    *   **Given** "/data/AudioOnly/" contains "song.ogg" but no chart files
    *   **When** scanned
    *   **Then** the folder is skipped during scan (not recognized as a song folder)

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 3
*   **Estimation Pointer**: 1
*   **Dependencies**: US-AST-012 (song database scanning)

---

## Story ID: US-AST-031 - Missing banner shows placeholder

**Story Card:**
> **As a** Player
> **I want** a placeholder image for songs without banners
> **So that** song select layout remains consistent

### Description
If a song has no banner file, the UI displays a default placeholder image with the song title rendered as text.

### Acceptance Criteria

*   **Scenario 1: Placeholder displayed**
    *   **Given** database entry for "Pumptris" has banner_path=null
    *   **When** the player navigates to "Pumptris" in song select
    *   **Then** a 256x80 gray rectangle with text "Pumptris" is displayed

*   **Scenario 2: Placeholder does not break layout**
    *   **Given** the music wheel displays 5 banners simultaneously
    *   **When** 2 songs have real banners and 3 use placeholders
    *   **Then** all 5 entries have consistent size and alignment

*   **Scenario 3: Missing banner logged once**
    *   **Given** "Pumptris" has no banner
    *   **When** song select opens
    *   **Then** the engine logs "song 'Pumptris' has no banner, using placeholder" at INFO level only on first view

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 3
*   **Estimation Pointer**: 1
*   **Dependencies**: US-AST-015 (banner discovery), US-SCN-006 (song select)

---

## Story ID: US-AST-032 - Missing BGA allows gameplay without background

**Story Card:**
> **As a** Player
> **I want** gameplay to proceed when a song has no BGA
> **So that** I play charts without requiring video files

### Description
If a song has no BGA file, Phase 1 gameplay renders the note field on a black background without error. Phase 2+ can load optional fallback backgrounds.

### Acceptance Criteria

*   **Scenario 1: Gameplay without BGA (Phase 1)**
    *   **Given** a chart with no associated BGA file
    *   **When** the player starts gameplay
    *   **Then** the note field renders on a black background and gameplay proceeds normally

*   **Scenario 2: No error logged during gameplay**
    *   **Given** a song without BGA is playing
    *   **When** 60 seconds of gameplay complete
    *   **Then** no BGA-related errors are logged

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 1
*   **Estimation Pointer**: 1
*   **Dependencies**: None (Phase 1 just checks for null and renders black)
*   **Phase 2 Extension**: US-REN-027 will handle optional fallback backgrounds

---

## Story ID: US-AST-033 - All missing assets logged

**Story Card:**
> **As a** Developer
> **I want** all missing asset paths logged with context
> **So that** I diagnose content problems without guessing

### Description
Every missing asset (texture, audio, chart, BGA) logs the file name, expected location, and the component that requested it.

### Acceptance Criteria

*   **Scenario 1: Missing texture logged**
    *   **Given** sprite "arrows.sprj" references "missing.tga"
    *   **When** the sprite is loaded
    *   **Then** the log contains "texture not found: 'missing.tga' in /data/sprites/ (probed .tga/.png/.dds, case-insensitive)" at ERROR level

*   **Scenario 2: Missing chart logged**
    *   **Given** database entry references "/data/Broken/broken.ksf" which was deleted
    *   **When** the player selects "Broken"
    *   **Then** the log contains "failed to load chart: /data/Broken/broken.ksf (file not found)" at ERROR level

*   **Scenario 3: Logs include context**
    *   **Given** a BGA animation references sprite "bg_layer.sprj"
    *   **When** the sprite is missing
    *   **Then** the log contains "BGA 'title.bgaj' layer 3 failed: sprite 'bg_layer.sprj' not found"

### Technical Notes & Constraints
*   **Status**: PLANNED Phase 1
*   **Estimation Pointer**: 1
*   **Dependencies**: US-AST-001 (texture logging already implemented)
*   **Log levels**: ERROR for critical assets (chart, audio), WARN for optional (banner, BGA)

---

# Non-Functional Requirements

## NFR-AST-001: Startup Performance

**Requirement**: Engine startup to title screen completes in under 5 seconds with warm song database cache for 1000+ songs.

**Acceptance Criteria**:
*   **Given** cache file exists and is valid
*   **When** measured on reference hardware (4-core CPU, SSD)
*   **Then** startup time from process launch to title screen render is under 5 seconds

**Dependencies**: US-AST-014

---

## NFR-AST-002: Memory Efficiency

**Requirement**: Song database metadata consumes under 100 KB per song entry.

**Acceptance Criteria**:
*   **Given** database contains 1000 song entries
*   **When** memory usage is measured after database load
*   **Then** total database memory is under 100 MB

**Dependencies**: US-AST-013

---

## NFR-AST-003: Case-Insensitive Scan Performance

**Requirement**: Directory scan with case-insensitive file matching completes in under 10 seconds for 1000 song folders.

**Acceptance Criteria**:
*   **Given** cold start with no cache
*   **When** scanning 1000 song folders on Linux ext4 filesystem
*   **Then** scan completes in under 10 seconds

**Dependencies**: US-AST-012

---

## NFR-AST-004: Texture Probe Latency

**Requirement**: Texture probing (extension fallback and case-insensitive match) completes in under 5 milliseconds per texture.

**Acceptance Criteria**:
*   **Given** a directory with 50 files
*   **When** probing for a texture that requires case-insensitive fallback
*   **Then** probe completes in under 5 milliseconds

**Dependencies**: US-AST-002, US-AST-003

---

## NFR-AST-005: Format Converter Correctness

**Requirement**: Format converters produce output that round-trips identically (original binary -> JSON -> binary produces identical binary).

**Acceptance Criteria**:
*   **Given** a valid .bga file
*   **When** converted to .bgaj, then manually reconstructed to binary
*   **Then** the reconstructed binary matches the original byte-for-byte

**Dependencies**: US-AST-006, US-AST-007, US-AST-008

---

# Story Summary

## DONE (8 stories)
- US-AST-001: Texture cache for reuse
- US-AST-002: Case-insensitive texture file lookup
- US-AST-003: Format probing for texture extensions
- US-AST-004: Texture cache memory release
- US-AST-005: Relative path resolution from data files
- US-AST-006: SPR to SPRJ converter
- US-AST-007: SP2 to SPRJ converter
- US-AST-008: BGA binary to JSON converter

## PLANNED (25 stories)

### Phase 1 (6 stories, 8 points)
- US-AST-009: Command-line data directory argument (2)
- US-AST-010: Environment variable for data directory (1)
- US-AST-030: Missing chart or audio excludes song (1)
- US-AST-031: Missing banner shows placeholder (1)
- US-AST-032: Missing BGA allows gameplay without background (1)
- US-AST-033: All missing assets logged (1)

### Phase 2 (6 stories, 18 points)
- US-AST-017: Chart loaded on song selection (2)
- US-AST-018: Texture cache LRU eviction (5)
- US-AST-019: Audio loaded at gameplay start (2)
- US-AST-020: BGA loaded on song selection (2)
- US-AST-021: System asset directory structure (2)
- US-AST-022: Font loading for text rendering (5)

### Phase 3 (7 stories, 23 points)
- US-AST-011: Config file for multiple data directories (2)
- US-AST-012: Recursive directory scan for songs (5)
- US-AST-013: Song metadata extraction during scan (5)
- US-AST-014: Cached song database for startup performance (5)
- US-AST-015: Banner and audio file discovery (2)
- US-AST-016: BGA file discovery per song (2)
- US-AST-023: Judgment and menu sound effects (2)

### Phase 5 (3 stories, 9 points)
- US-AST-024: Note skin directory structure (5)
- US-AST-025: Multiple note skins installed (2)
- US-AST-026: Selected note skin persisted in profile (2)

### Phase 7 (3 stories, 9 points)
- US-AST-027: Exceed-era detection via .see files (2)
- US-AST-028: NX-era detection via .nx files (2)
- US-AST-029: Multi-version data coexistence (5)

## Story Point Totals

**Phase 1**: 8 points  
**Phase 2**: 18 points  
**Phase 3**: 23 points  
**Phase 5**: 9 points  
**Phase 7**: 9 points  

**Total PLANNED**: 67 story points

---

# Cross-Reference: Requirements to Stories

| Requirement | Stories |
|-------------|---------|
| REQ-AST-001 | US-AST-001, US-AST-002, US-AST-003, US-AST-004 |
| REQ-AST-002 | US-AST-009, US-AST-010, US-AST-011 |
| REQ-AST-003 | US-AST-012, US-AST-013, US-AST-014, US-AST-015, US-AST-016 |
| REQ-AST-004 | US-AST-027, US-AST-028 |
| REQ-AST-005 | US-AST-024, US-AST-025, US-AST-026 |
| REQ-AST-006 | US-AST-017, US-AST-018, US-AST-019, US-AST-020 |
| REQ-AST-007 | US-AST-021, US-AST-022, US-AST-023 |
| REQ-AST-008 | US-AST-030, US-AST-031, US-AST-032, US-AST-033 |
| REQ-AST-009 | US-AST-029 |
| REQ-AST-010 | US-AST-005 |
| REQ-AST-011 | US-AST-006, US-AST-007, US-AST-008 |
