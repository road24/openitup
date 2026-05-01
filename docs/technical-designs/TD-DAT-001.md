# TD-DAT-001: Data Management -- User Data Directory, Settings, and Chart Metadata Cache

**Stories**: US-DAT-001, US-DAT-002, US-DAT-003, US-DAT-004, US-DAT-005, US-DAT-006, US-DAT-007, US-DAT-031, US-DAT-032, US-DAT-033
**Phase**: 3
**Author**: technical-architect agent
**Status**: Draft

## Overview

This design introduces the Phase 3 data management subsystem: platform-appropriate user data directories, a JSON settings system with validation and atomic writes, and a chart metadata cache for fast startup. The design builds on the existing `DataDirectory` value type (`src/openitup/asset/data_directory.h`) for game asset paths, and adds a parallel `UserDataDir` concept for writable user data (settings, profiles, cache). It follows the project's established patterns: `std::unique_ptr` ownership by Engine, injectable dependencies for testing, `nlohmann::json` for serialization, and spdlog for logging.

The subsystem comprises three cohesive concerns: (1) finding and creating the user data directory, (2) loading/validating/saving engine settings with crash-safe writes, and (3) caching chart metadata to avoid directory re-scanning on startup.

## Architecture

### Component Diagram

```
main.cpp
  |
  v
Engine (owns SettingsManager via unique_ptr)
  |
  +--> UserDataDir (value type, resolved at startup)
  |      |  resolves ~/.local/share/openitup/ (Linux)
  |      |  resolves %APPDATA%/openitup/ (Windows)
  |      |  creates profiles/, cache/ subdirectories
  |
  +--> SettingsManager (owned by Engine)
  |      |  loads/saves settings.json via atomic_write_json()
  |      |  validates all values via SettingsValidator
  |      |  holds current SettingsData in memory
  |      |  notifies Engine on change (future: observer)
  |
  +--> SongCache (standalone, created during song loading)
         |  loads/saves cache/chart_metadata.json
         |  checks directory mtime for invalidation
         |  uses atomic_write_json() for writes
         |  stores ChartCacheEntry array

Shared utility:
  atomic_write_json()  -- free function used by SettingsManager and SongCache
```

### New Types

#### `UserDataDir` (`src/openitup/data/user_data_dir.h`)

A value type that resolves and creates the platform-appropriate writable user data directory. Analogous to the existing `DataDirectory` for game assets but for user-writable data. This is intentionally not a class with virtual methods -- it is a simple data holder with creation logic.

```cpp
// src/openitup/data/user_data_dir.h
#pragma once

#include <filesystem>

namespace openitup::data {

class UserDataDir {
public:
    // Resolve the platform-appropriate user data directory.
    // Linux: $XDG_DATA_HOME/openitup/ (defaults to ~/.local/share/openitup/)
    // Windows: %APPDATA%/openitup/
    // The path is resolved but NOT created at construction.
    UserDataDir();

    // Injectable constructor for testing: use a custom base path.
    explicit UserDataDir(std::filesystem::path base_path);

    // Ensure the directory tree exists. Creates:
    //   <base>/
    //   <base>/profiles/
    //   <base>/cache/
    // Returns true if all directories exist or were created.
    // On failure: logs ERROR, returns false. Engine proceeds with
    // in-memory-only operation.
    bool ensure_directories() const;

    // Resolved absolute path to the user data root.
    const std::filesystem::path& path() const;

    // Convenience accessors for subdirectories.
    std::filesystem::path settings_file() const;    // <base>/settings.json
    std::filesystem::path profiles_dir() const;     // <base>/profiles/
    std::filesystem::path cache_dir() const;        // <base>/cache/
    std::filesystem::path cache_file() const;       // <base>/cache/chart_metadata.json

    // Is the path valid and writable?
    bool is_valid() const;

private:
    std::filesystem::path path_;
};

} // namespace openitup::data
```

**Key decisions**:

- Two constructors: default resolves the real platform path; the explicit one accepts a custom path for testing (matching the Clock/Engine injection pattern). Tests use a temp directory under `/tmp`.
- `ensure_directories()` is separate from construction so the caller can handle failure gracefully (log and continue with in-memory defaults) rather than throwing.
- The XDG_DATA_HOME variable is respected on Linux per the XDG Base Directory Specification. If unset, `~/.local/share/` is the fallback.
- On Windows, `std::getenv("APPDATA")` is used. SDL3's `SDL_GetPrefPath()` is intentionally NOT used because it requires SDL to be initialized, and we want UserDataDir resolution to happen before Engine construction for settings loading.

---

#### `SettingsData` (`src/openitup/data/settings_data.h`)

A plain data struct holding all engine settings. This is the in-memory representation. Serialization is separate.

```cpp
// src/openitup/data/settings_data.h
#pragma once

#include <cstdint>
#include <map>
#include <string>

#include <openitup/input/pad_input.h>

namespace openitup::data {

struct VideoSettings {
    int width = 1920;
    int height = 1080;
};

struct AudioSettings {
    std::string device = "default";
    int global_offset_ms = 0;   // range: [-500, +500]
    float music_volume = 1.0f;  // [0.0, 1.0]
    float sfx_volume = 1.0f;    // [0.0, 1.0]
};

struct SettingsData {
    int schema_version = 1;

    VideoSettings video;
    AudioSettings audio;

    // Map from PadInput enum name (string) to SDL keycode (string).
    // e.g. {"P1_DOWN_LEFT": "SDLK_z", "P1_UP_LEFT": "SDLK_q", ...}
    std::map<std::string, std::string> input_bindings;

    // Persisted active profile name (US-DAT-027, Phase 5).
    // Empty string means "use default profile".
    std::string active_profile;

    // Returns defaults for all fields.
    static SettingsData make_default();
};

} // namespace openitup::data
```

The `make_default()` factory populates the standard PIU keyboard layout (Z/Q/S/E/C for P1, numpad for P2, Enter for START, Escape for BACK).

---

#### `SettingsValidator` (`src/openitup/data/settings_validator.h`)

Pure-function validation of settings values. Separated from SettingsManager for testability -- validation is pure logic with no I/O.

```cpp
// src/openitup/data/settings_validator.h
#pragma once

#include <string>
#include <vector>

#include <openitup/data/settings_data.h>

namespace openitup::data {

struct ValidationResult {
    bool valid = true;
    std::vector<std::string> warnings;  // human-readable messages for each fix applied
};

// Validate and fix a SettingsData in-place.
// Invalid values are replaced with defaults. Each replacement generates a warning.
// Returns the list of warnings (empty if all values were valid).
ValidationResult validate_settings(SettingsData& settings);

// Individual validators (exposed for unit testing).

// Resolution: min 640x480, max 7680x4320.
bool validate_resolution(int& width, int& height, std::string& warning);

// Offset: clamp to [-500, +500] ms.
bool validate_offset(int& offset_ms, std::string& warning);

// Volume: clamp to [0.0, 1.0].
bool validate_volume(float& volume, const std::string& name, std::string& warning);

// Key bindings: reject duplicate SDL keycodes. Second duplicate gets default.
bool validate_bindings(std::map<std::string, std::string>& bindings,
                       std::string& warning);

} // namespace openitup::data
```

---

#### `SettingsManager` (`src/openitup/data/settings_manager.h`)

Owns the current `SettingsData` and manages load/save. Owned by Engine via `unique_ptr`.

```cpp
// src/openitup/data/settings_manager.h
#pragma once

#include <filesystem>
#include <functional>
#include <memory>

#include <openitup/data/settings_data.h>

namespace openitup::data {

class SettingsManager {
public:
    // Injectable file I/O for testing.
    using FileReaderFn = std::function<std::string(const std::filesystem::path&)>;
    using FileWriterFn = std::function<bool(const std::filesystem::path&,
                                            const std::string&)>;

    // Construct with a path to settings.json.
    // Uses default file I/O (std::ifstream/atomic_write_json).
    explicit SettingsManager(std::filesystem::path settings_path);

    // Injectable constructor for testing.
    SettingsManager(std::filesystem::path settings_path,
                    FileReaderFn reader,
                    FileWriterFn writer);

    // Load settings from disk. If file missing, creates with defaults.
    // If file corrupt, logs ERROR and uses defaults.
    // Validates all loaded values (US-DAT-007).
    // Returns true if a file was successfully loaded (even with corrections).
    bool load();

    // Save current settings to disk using atomic write.
    // Returns true on success. On failure, logs ERROR and returns false.
    bool save() const;

    // Read-only access to current settings.
    const SettingsData& settings() const;

    // Modify a setting. Automatically saves to disk (US-DAT-005).
    // Returns true if save succeeded.
    bool update(const SettingsData& new_settings);

    // Modify video settings specifically (convenience).
    bool update_video(int width, int height);
    bool update_audio(const AudioSettings& audio);
    bool update_binding(const std::string& input_name,
                        const std::string& keycode);
    bool update_active_profile(const std::string& profile_name);

private:
    std::filesystem::path settings_path_;
    SettingsData data_;
    FileReaderFn reader_;
    FileWriterFn writer_;
};

} // namespace openitup::data
```

**Key decisions**:

- `FileReaderFn` / `FileWriterFn` injection follows the project's established pattern (CounterFn, ImageLoaderFn, FileReaderFn in KsfParser). Default implementations use std::ifstream for reading and `atomic_write_json()` for writing.
- `update()` methods immediately save to disk (US-DAT-005 requirement). The AC says "within 100 milliseconds" -- atomic rename on a local filesystem is effectively instant.
- The manager does not cache the JSON AST. It re-serializes from `SettingsData` on every save. This keeps the code simple and the struct is small.

---

#### `atomic_write_json()` (`src/openitup/data/atomic_write.h`)

A free function implementing the write-to-temp-then-rename pattern. Used by both SettingsManager and SongCache.

```cpp
// src/openitup/data/atomic_write.h
#pragma once

#include <filesystem>
#include <string>

namespace openitup::data {

// Atomically write content to a file.
// 1. Writes to <path>.tmp in the same directory.
// 2. On success, renames <path>.tmp to <path> (atomic on POSIX).
// 3. On failure, removes <path>.tmp (if it exists) and returns false.
//
// Logs ERROR on failure with path and error details.
// Returns true on success.
bool atomic_write_file(const std::filesystem::path& path,
                       const std::string& content);

} // namespace openitup::data
```

---

#### `ChartCacheEntry` / `SongCache` (`src/openitup/data/song_cache.h`)

Caches chart metadata to avoid re-scanning song directories on startup.

```cpp
// src/openitup/data/song_cache.h
#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace openitup::data {

struct DifficultyEntry {
    std::string mode;       // "single" or "double"
    int level = 0;          // numeric difficulty rating
    std::string name;       // "Easy", "Normal", "Hard", etc.
};

struct ChartCacheEntry {
    std::filesystem::path file_path;    // absolute path to chart file
    std::string content_hash;           // hex-encoded SHA-256 (Phase 5, empty in Phase 3)
    std::string title;
    std::string artist;
    double display_bpm = 0.0;
    std::vector<DifficultyEntry> difficulties;
    std::string last_modified;          // ISO 8601 timestamp of chart file mtime
};

class SongCache {
public:
    // Injectable file I/O for testing.
    using FileReaderFn = std::function<std::string(const std::filesystem::path&)>;
    using FileWriterFn = std::function<bool(const std::filesystem::path&,
                                            const std::string&)>;

    // Construct with the path to the cache file.
    explicit SongCache(std::filesystem::path cache_path);

    // Injectable constructor for testing.
    SongCache(std::filesystem::path cache_path,
              FileReaderFn reader,
              FileWriterFn writer);

    // Load the cache from disk.
    // Returns true if the cache was loaded and is fresh.
    // Returns false if the cache is missing, corrupt, or stale.
    bool load();

    // Save the cache to disk using atomic write.
    bool save() const;

    // Check if the cache is still valid against the given song directories.
    // Compares directory mtime against the cache's last_updated timestamp.
    bool is_fresh(const std::vector<std::filesystem::path>& song_dirs) const;

    // Replace the cache contents (called after a full directory scan).
    void set_entries(std::vector<ChartCacheEntry> entries);

    // Read-only access to cached entries.
    const std::vector<ChartCacheEntry>& entries() const;

    // Number of cached entries.
    size_t size() const;

    // The timestamp when the cache was last updated.
    const std::string& last_updated() const;

private:
    std::filesystem::path cache_path_;
    std::string last_updated_;          // ISO 8601 timestamp
    int schema_version_ = 1;
    std::vector<ChartCacheEntry> entries_;
    FileReaderFn reader_;
    FileWriterFn writer_;
};

} // namespace openitup::data
```

**Key decisions**:

- `content_hash` is present in the cache entry but will be empty in Phase 3. Chart hashing is Phase 5 (US-DAT-014). The field is included now to avoid a schema migration when it is populated later.
- `is_fresh()` accepts a list of song directories rather than storing them. This keeps the cache file format simple (it stores chart data, not configuration) and avoids coupling the cache to settings.
- Cache invalidation uses directory `mtime` comparison, which is fast (one `stat()` call per song directory) and reliable on all target platforms.
- `last_modified` per entry records the chart file's mtime at scan time. This enables future per-file incremental re-scanning (not in scope, but the data is there).

---

### Modified Types

#### `EngineConfig` (`src/openitup/core/engine.h`)

- Add field: `std::filesystem::path user_data_path` -- Optional override for user data directory (for testing). If empty, `UserDataDir` resolves the platform default.
- Reason: Enables Engine tests to use a temp directory without environment variable hacks.

#### `Engine` (`src/openitup/core/engine.h` / `engine.cpp`)

- Add member: `std::unique_ptr<data::SettingsManager> settings_manager_` -- Owned settings subsystem.
- Add member: `data::UserDataDir user_data_dir_` -- Resolved user data directory (value type, stored by value).
- Add accessor: `data::SettingsManager* get_settings() const` -- Non-owning access, following existing pattern (`get_audio()`, `get_input_system()`).
- Add accessor: `const data::UserDataDir& get_user_data_dir() const` -- Read-only reference.
- Modify constructor: After SDL init but before scene stack creation, resolve user data dir and load settings.
- Reason: Settings must be available before scenes are constructed (scenes may read resolution, key bindings, audio settings).

#### `main.cpp` (`src/openitup/main.cpp`)

- No changes needed. UserDataDir resolution happens inside Engine constructor, not in main. The `EngineConfig::user_data_path` override can be set from a future `--user-data-dir` CLI argument if needed, but Phase 3 does not require it.

## JSON Schemas

### `settings.json`

```json
{
    "schema_version": 1,
    "video": {
        "width": 1920,
        "height": 1080
    },
    "audio": {
        "device": "default",
        "global_offset_ms": 0,
        "music_volume": 1.0,
        "sfx_volume": 1.0
    },
    "input": {
        "P1_DOWN_LEFT": "SDLK_z",
        "P1_UP_LEFT": "SDLK_q",
        "P1_CENTER": "SDLK_s",
        "P1_UP_RIGHT": "SDLK_e",
        "P1_DOWN_RIGHT": "SDLK_c",
        "P2_DOWN_LEFT": "SDLK_KP_1",
        "P2_UP_LEFT": "SDLK_KP_7",
        "P2_CENTER": "SDLK_KP_5",
        "P2_UP_RIGHT": "SDLK_KP_9",
        "P2_DOWN_RIGHT": "SDLK_KP_3",
        "START": "SDLK_RETURN",
        "BACK": "SDLK_ESCAPE",
        "SELECT": "SDLK_TAB",
        "COIN": "SDLK_F1"
    },
    "active_profile": ""
}
```

**Field rules**:

| Field | Type | Default | Constraints |
|-------|------|---------|-------------|
| `schema_version` | int | 1 | Must be >= 1 |
| `video.width` | int | 1920 | 640 -- 7680 |
| `video.height` | int | 1080 | 480 -- 4320 |
| `audio.device` | string | "default" | Non-empty |
| `audio.global_offset_ms` | int | 0 | -500 -- +500 |
| `audio.music_volume` | float | 1.0 | 0.0 -- 1.0 |
| `audio.sfx_volume` | float | 1.0 | 0.0 -- 1.0 |
| `input.*` | string | (layout) | Valid SDL keycode name, no duplicates |
| `active_profile` | string | "" | Filename stem (no extension) |

**Missing section behavior** (US-DAT-003 Scenario 2): If a top-level section (`video`, `audio`, `input`) is absent, its defaults are used in full. Individual missing fields within a present section also get defaults.

**Invalid JSON behavior** (US-DAT-003 Scenario 3): The entire file is discarded. All defaults are used. A new valid file is written immediately.

---

### `cache/chart_metadata.json`

```json
{
    "schema_version": 1,
    "last_updated": "2026-04-26T10:00:00Z",
    "charts": [
        {
            "file_path": "/home/user/songs/Pumptris/pumptris_crazy.ksf",
            "content_hash": "",
            "title": "Pumptris Quattro",
            "artist": "BanYa",
            "display_bpm": 145.0,
            "last_modified": "2026-04-20T08:30:00Z",
            "difficulties": [
                {
                    "mode": "single",
                    "level": 18,
                    "name": "Crazy"
                }
            ]
        }
    ]
}
```

**Field rules**:

| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `schema_version` | int | 1 | Must be >= 1 |
| `last_updated` | string | (now) | ISO 8601 UTC |
| `charts[].file_path` | string | -- | Absolute path, must exist for cache to be valid |
| `charts[].content_hash` | string | "" | Empty in Phase 3, populated in Phase 5 |
| `charts[].title` | string | -- | From chart metadata |
| `charts[].artist` | string | -- | From chart metadata |
| `charts[].display_bpm` | float | 0.0 | From chart metadata |
| `charts[].last_modified` | string | -- | ISO 8601, file mtime at scan time |
| `charts[].difficulties[].mode` | string | -- | "single" or "double" |
| `charts[].difficulties[].level` | int | 0 | Difficulty rating |
| `charts[].difficulties[].name` | string | -- | Difficulty name |

## File Plan

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/openitup/data/user_data_dir.h` | UserDataDir class declaration |
| Create | `src/openitup/data/user_data_dir.cpp` | Platform path resolution, directory creation |
| Create | `src/openitup/data/settings_data.h` | SettingsData / VideoSettings / AudioSettings structs |
| Create | `src/openitup/data/settings_data.cpp` | SettingsData::make_default(), JSON serialization |
| Create | `src/openitup/data/settings_validator.h` | Validation function declarations |
| Create | `src/openitup/data/settings_validator.cpp` | All validation logic |
| Create | `src/openitup/data/settings_manager.h` | SettingsManager class declaration |
| Create | `src/openitup/data/settings_manager.cpp` | Load, save, update with atomic write |
| Create | `src/openitup/data/atomic_write.h` | atomic_write_file() declaration |
| Create | `src/openitup/data/atomic_write.cpp` | Write-to-temp-then-rename implementation |
| Create | `src/openitup/data/song_cache.h` | SongCache / ChartCacheEntry declarations |
| Create | `src/openitup/data/song_cache.cpp` | Cache load, save, freshness check |
| Modify | `src/openitup/core/engine.h` | Add user_data_dir_, settings_manager_, accessors, user_data_path config field |
| Modify | `src/openitup/core/engine.cpp` | Initialize UserDataDir and SettingsManager in constructor |
| Modify | `CMakeLists.txt` | Add new .cpp files to openitup_engine target |
| Create | `test/test_user_data_dir.cpp` | Unit tests for UserDataDir |
| Create | `test/test_settings.cpp` | Unit tests for SettingsData, SettingsValidator, SettingsManager |
| Create | `test/test_atomic_write.cpp` | Unit tests for atomic_write_file() |
| Create | `test/test_song_cache.cpp` | Unit tests for SongCache |
| Modify | `CMakeLists.txt` | Add new test files to openitup_tests target |

## Data Flow

### Startup: First Run (No User Data Directory)

```
1. Engine constructor:
2.   UserDataDir user_data_dir(config.user_data_path)
3.   // config.user_data_path is empty -> resolves platform default
4.   // Linux: ~/.local/share/openitup/
5.
6.   user_data_dir.ensure_directories()
7.   // Creates ~/.local/share/openitup/
8.   // Creates ~/.local/share/openitup/profiles/
9.   // Creates ~/.local/share/openitup/cache/
10.  // Returns true
11.
12.  SettingsManager settings_mgr(user_data_dir.settings_file())
13.  settings_mgr.load()
14.  // File does not exist -> creates settings.json with defaults
15.  // Logs INFO: "Created default settings at ~/.local/share/openitup/settings.json"
```

### Startup: Valid Settings File Exists

```
1. settings_mgr.load():
2.   content = reader_(settings_path_)
3.   json j = json::parse(content)
4.   // Parse each section, filling SettingsData fields
5.   // For each missing field, use default
6.   data_ = parsed_settings
7.
8.   auto result = validate_settings(data_)
9.   if (!result.warnings.empty()):
10.    for (auto& w : result.warnings):
11.      spdlog::warn("{}", w)
12.  // e.g. "Video resolution 1x1 out of range, using 1920x1080"
13.
14.  spdlog::info("Loaded settings from {}", settings_path_)
15.  return true
```

### Settings Change Triggers Save

```
1. Some UI code calls:
2.   engine.get_settings()->update_audio(new_audio_settings)
3.
4. SettingsManager::update_audio():
5.   data_.audio = new_audio_settings
6.   auto result = validate_settings(data_)
7.   // Log any warnings
8.   return save()
9.
10. SettingsManager::save():
11.   json j = serialize(data_)
12.   std::string content = j.dump(4)  // pretty-print, 4-space indent
13.   return writer_(settings_path_, content)
14.   // Default writer calls atomic_write_file()
```

### Atomic Write Sequence

```
1. atomic_write_file("/home/user/.local/share/openitup/settings.json", content):
2.   temp_path = "/home/user/.local/share/openitup/settings.json.tmp"
3.   std::ofstream out(temp_path, std::ios::binary)
4.   out.write(content.data(), content.size())
5.   out.close()
6.   if (!out.good()):
7.     std::filesystem::remove(temp_path)
8.     spdlog::error("Failed to write temp file {}: I/O error", temp_path)
9.     return false
10.  std::error_code ec;
11.  std::filesystem::rename(temp_path, target_path, ec)
12.  if (ec):
13.    std::filesystem::remove(temp_path)
14.    spdlog::error("Failed to rename {} to {}: {}", temp_path, target_path, ec.message())
15.    return false
16.  return true
```

### Cache Load and Invalidation Check

```
1. SongCache cache(user_data_dir.cache_file())
2. bool loaded = cache.load()
3.
4. if (loaded):
5.   bool fresh = cache.is_fresh(song_directories)
6.   if (fresh):
7.     spdlog::debug("Loaded chart cache with {} entries in {}ms", cache.size(), elapsed_ms)
8.     // Use cached entries directly
9.   else:
10.    spdlog::info("Chart cache invalidated due to directory modification, rebuilding")
11.    // Fall through to full scan
12.
13. // Full scan path:
14. auto entries = scan_song_directories(song_directories)  // Phase 3 song scanner
15. cache.set_entries(std::move(entries))
16. cache.save()
```

### Read-Only Filesystem Fallback

```
1. user_data_dir.ensure_directories():
2.   try:
3.     std::filesystem::create_directories(path_)
4.   catch (std::filesystem::filesystem_error& e):
5.     spdlog::error("Failed to create user data directory {}: {}", path_, e.what())
6.     return false
7.
8. // Engine continues with in-memory defaults
9. // SettingsManager load() returns false (no file), uses defaults
10. // SettingsManager save() returns false (can't write), logs error
11. // All settings changes remain in-memory only for the session
```

## Dependencies

### Internal
- **Engine** (`src/openitup/core/engine.h`) -- Engine owns SettingsManager, stores UserDataDir. Settings loaded during Engine construction.
- **PadInput** (`src/openitup/input/pad_input.h`) -- Key binding validation references PadInput enum names. `pad_input_to_string()` is used for default binding generation.
- **ChartMetadata** (`src/openitup/chart/chart_metadata.h`) -- ChartCacheEntry mirrors a subset of ChartMetadata fields. The song scanner (Phase 3) will populate cache entries from parsed chart metadata.
- **nlohmann::json** -- Already linked via `openitup_engine`. Used for settings and cache serialization.
- **spdlog** -- Already linked. Used for all logging.

### External (new libraries)
None. All dependencies are already in the project. `std::filesystem` (C++17) provides path manipulation, directory creation, and `last_write_time()` for mtime checks.

## Architectural Decisions

### ADR-1: UserDataDir as Value Type, Not Engine Service

- **Context**: The user data directory could be an Engine-owned service, a static singleton, or a simple value type.
- **Decision**: `UserDataDir` is a value type stored by value in Engine. It has no virtual methods, no internal state beyond the path, and no lifecycle to manage.
- **Alternatives considered**: (a) `SDL_GetPrefPath()` -- requires SDL to be initialized first, but we want settings available before renderer init. (b) Static utility function -- loses the injectable constructor for testing. (c) Singleton -- violates the project's no-globals convention.
- **Consequences**: Clean, testable, minimal. The injectable constructor allows tests to use `/tmp/test_openitup/` without modifying environment variables.

### ADR-2: std::getenv for Platform Paths, Not SDL_GetPrefPath

- **Context**: SDL3 provides `SDL_GetPrefPath("openitup", "openitup")` which returns the platform-appropriate path. However, the Engine needs settings loaded early -- before `SDL_Init()` has been called -- because settings contain video resolution that affects renderer initialization.
- **Decision**: Use `std::getenv("XDG_DATA_HOME")` on Linux and `std::getenv("APPDATA")` on Windows. Fall back to `~/.local/share/` and compile-time platform defaults.
- **Alternatives considered**: (a) `SDL_GetPrefPath()` -- requires SDL init first, creates chicken-and-egg with resolution settings. (b) Reorder init so SDL comes first, then settings, then renderer -- changes the existing Engine initialization sequence and couples settings to SDL lifecycle. (c) Two-pass init (SDL first with defaults, then re-init with loaded settings) -- fragile and wasteful.
- **Consequences**: Slightly more platform code than SDL_GetPrefPath, but self-contained and testable. The `#ifdef _WIN32` blocks are limited to one function in `user_data_dir.cpp`.

### ADR-3: SettingsManager Saves Immediately on Every Change

- **Context**: US-DAT-005 requires "settings saved immediately when changed." Alternatives include deferred/batched saves or save-on-exit.
- **Decision**: Every `update_*()` call triggers an immediate `save()`. The atomic write ensures no corruption.
- **Alternatives considered**: (a) Dirty flag with deferred save in game loop -- adds complexity, risks data loss on crash. (b) Save-on-exit only -- directly contradicts US-DAT-005. (c) Debounced save (coalesce rapid changes) -- over-engineered for settings that change at most a few times per session.
- **Consequences**: One atomic rename per setting change. At typical usage (a few changes per play session), this is negligible. If a settings UI allows rapid slider adjustments, the UI layer can batch changes into a single `update()` call.

### ADR-4: Shared atomic_write_file Free Function

- **Context**: US-DAT-006 (settings) and US-DAT-012 (profiles, Phase 5) both need atomic writes. The implementation is identical.
- **Decision**: Extract `atomic_write_file()` as a free function in `src/openitup/data/atomic_write.h`. Both SettingsManager and the future ProfileService use it.
- **Alternatives considered**: (a) Duplicate the logic in each class -- violates DRY. (b) Base class with atomic write method -- introduces unnecessary inheritance.
- **Consequences**: One implementation, tested once, reused everywhere. The function is stateless and pure (given path and content, produces a file). US-DAT-012 (Phase 5) requires no new write logic.

### ADR-5: SongCache Stores ISO 8601 Timestamps as Strings

- **Context**: The cache needs a `last_updated` timestamp for freshness checks. Options include Unix epoch (int), ISO 8601 string, or std::filesystem::file_time_type.
- **Decision**: Store ISO 8601 UTC strings in JSON. Convert to `std::filesystem::file_time_type` at runtime for comparison.
- **Alternatives considered**: (a) Unix epoch integer -- not human-readable in the JSON file, makes debugging harder. (b) `file_time_type` directly -- not JSON-serializable. (c) Separate int and string fields -- redundant.
- **Consequences**: Slight overhead converting string to time_point during freshness check, but this is a once-per-startup operation. The JSON file is human-readable and debuggable.

### ADR-6: Injectable FileReaderFn/FileWriterFn for SettingsManager and SongCache

- **Context**: Both classes need file I/O that must be testable without touching the real filesystem.
- **Decision**: Follow the project's established injection pattern (KsfParser's FileReaderFn, Clock's CounterFn). Both SettingsManager and SongCache accept optional FileReaderFn/FileWriterFn.
- **Alternatives considered**: (a) Virtual interface for file I/O -- heavier than needed for two functions. (b) Template parameter -- complicates headers, departs from codebase style. (c) Always use real filesystem, test with temp dirs -- viable but slower and less isolated.
- **Consequences**: Unit tests inject lambdas that return preset strings or record writes to an in-memory buffer. Tests are fast and deterministic. Default constructors use real I/O for production.

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| XDG_DATA_HOME unset on some Linux configurations | Low | Low | Fallback to `~/.local/share/` per XDG spec. This is the standard default. |
| `std::filesystem::rename()` not atomic across filesystems | Med | Low | Temp file is in the same directory as target (same filesystem). Document this requirement. |
| Large settings.json from future features slows save | Low | Very Low | Settings struct is small (< 2KB serialized). Even with Phase 5 additions, well under 10KB. |
| Cache mtime check has 1-second granularity on some filesystems (FAT32, HFS+) | Low | Low | mtime comparison is >= not >. Worst case: cache is rebuilt unnecessarily once. No data loss. |
| Race condition if two engine instances write settings simultaneously | Med | Very Low | Not a supported use case (single-instance game). Document: not multi-instance safe. |
| `std::filesystem::create_directories()` throws on permission error | Med | Low | Wrapped in try-catch in `ensure_directories()`. Returns false, engine continues with in-memory defaults. |
| Cache loading of 1000 entries might exceed performance budget on slow hardware | Med | Low | nlohmann::json parses ~50MB/s. A 1000-entry cache is ~500KB. Parse time ~10ms, well under the 200ms target. Benchmark test verifies. |

## Traceability Matrix

| Requirement | Story | Acceptance Criterion | Test Case | Source File |
|-------------|-------|---------------------|-----------|-------------|
| REQ-DAT-013 | US-DAT-001 | SC1: Linux user data path | UserDataDir::ResolvesLinuxPath | user_data_dir.cpp |
| REQ-DAT-013 | US-DAT-001 | SC2: Windows user data path | UserDataDir::ResolvesWindowsPath | user_data_dir.cpp |
| REQ-DAT-013 | US-DAT-001 | SC3: Path is absolute | UserDataDir::PathIsAbsolute | user_data_dir.cpp |
| REQ-DAT-013 | US-DAT-002 | SC1: Create missing user data dir | UserDataDir::CreatesMissingDirectory | user_data_dir.cpp |
| REQ-DAT-013 | US-DAT-002 | SC2: Create profiles subdir | UserDataDir::CreatesProfilesSubdir | user_data_dir.cpp |
| REQ-DAT-013 | US-DAT-002 | SC3: Create cache subdir | UserDataDir::CreatesCacheSubdir | user_data_dir.cpp |
| REQ-DAT-013 | US-DAT-002 | SC4: Read-only filesystem error | UserDataDir::ReadOnlyFallback | user_data_dir.cpp |
| REQ-DAT-002 | US-DAT-003 | SC1: Valid settings all fields | Settings::ParseAllFields | settings_data.cpp |
| REQ-DAT-002 | US-DAT-003 | SC2: Missing optional section uses defaults | Settings::MissingSectionUsesDefaults | settings_data.cpp |
| REQ-DAT-002 | US-DAT-003 | SC3: Invalid JSON uses defaults | SettingsManager::InvalidJsonUsesDefaults | settings_manager.cpp |
| REQ-DAT-002 | US-DAT-004 | SC1: Load existing valid settings | SettingsManager::LoadExistingValid | settings_manager.cpp |
| REQ-DAT-002 | US-DAT-004 | SC2: Missing file creates defaults | SettingsManager::MissingFileCreatesDefaults | settings_manager.cpp |
| REQ-DAT-002 | US-DAT-004 | SC3: Logging confirmation | SettingsManager::LogsLoadPath | settings_manager.cpp |
| REQ-DAT-002 | US-DAT-005 | SC1: Key binding change triggers save | SettingsManager::UpdateBindingSaves | settings_manager.cpp |
| REQ-DAT-002 | US-DAT-005 | SC2: Offset change triggers save | SettingsManager::UpdateAudioSaves | settings_manager.cpp |
| REQ-DAT-002 | US-DAT-005 | SC3: Write failure logged | SettingsManager::WriteFailureLogged | settings_manager.cpp |
| REQ-DAT-011 | US-DAT-006 | SC1: Write to temp file | AtomicWrite::WritesToTempFile | atomic_write.cpp |
| REQ-DAT-011 | US-DAT-006 | SC2: Atomic rename on success | AtomicWrite::RenamesOnSuccess | atomic_write.cpp |
| REQ-DAT-011 | US-DAT-006 | SC3: Original preserved on failure | AtomicWrite::PreservesOriginalOnFailure | atomic_write.cpp |
| REQ-DAT-007 | US-DAT-007 | SC1: Resolution validated | Validator::ResolutionOutOfRange | settings_validator.cpp |
| REQ-DAT-007 | US-DAT-007 | SC2: Offset clamped | Validator::OffsetClamped | settings_validator.cpp |
| REQ-DAT-007 | US-DAT-007 | SC3: Duplicate key binding | Validator::DuplicateBindingRejected | settings_validator.cpp |
| REQ-DAT-012 | US-DAT-031 | SC1: Valid cache with entries | SongCache::LoadValidCache | song_cache.cpp |
| REQ-DAT-012 | US-DAT-031 | SC2: Corrupt cache rebuilt | SongCache::CorruptCacheReturnsInvalid | song_cache.cpp |
| REQ-DAT-012 | US-DAT-032 | SC1: Cache is fresh | SongCache::FreshCacheValid | song_cache.cpp |
| REQ-DAT-012 | US-DAT-032 | SC2: Cache is stale | SongCache::StaleCacheInvalid | song_cache.cpp |
| REQ-DAT-012 | US-DAT-032 | SC3: Invalidation logged | SongCache::InvalidationLogged | song_cache.cpp |
| REQ-DAT-012 | US-DAT-033 | SC1: 1000-entry load under 200ms | SongCache::Load1000EntriesPerformance | song_cache.cpp |
| REQ-DAT-012 | US-DAT-033 | SC2: Load time logged | SongCache::LogsLoadTime | song_cache.cpp |

## Testing Strategy

### Unit Tests (`test/test_user_data_dir.cpp`) -- Pure Logic, No SDL

All tests use the injectable constructor with a temp directory under `/tmp/test_openitup_XXXXXX/`.

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `ResolvesLinuxPath` | Default constructor resolves to `~/.local/share/openitup/` (mocked via env) | US-DAT-001 SC1 |
| `RespectsXDGDataHome` | If XDG_DATA_HOME is set, uses that instead of default | US-DAT-001 SC1 |
| `PathIsAbsolute` | Returned path is always absolute, no tildes or env vars | US-DAT-001 SC3 |
| `CreatesMissingDirectory` | ensure_directories() creates the base dir | US-DAT-002 SC1 |
| `CreatesProfilesSubdir` | ensure_directories() creates profiles/ | US-DAT-002 SC2 |
| `CreatesCacheSubdir` | ensure_directories() creates cache/ | US-DAT-002 SC3 |
| `ReadOnlyFallback` | ensure_directories() returns false on read-only path, logs error | US-DAT-002 SC4 |
| `SettingsFilePath` | settings_file() returns correct path | US-DAT-003 |
| `CacheFilePath` | cache_file() returns correct path | US-DAT-031 |
| `InjectablePathOverride` | Injectable constructor uses provided path | (testability) |

Environment variable tests use a RAII guard:

```cpp
struct ScopedEnvVar {
    ScopedEnvVar(const char* name, const char* value);
    ~ScopedEnvVar();  // restores original value or unsets
};
```

### Unit Tests (`test/test_settings.cpp`) -- Pure Logic, No SDL

Tests for SettingsData serialization, SettingsValidator, and SettingsManager with injected I/O.

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `DefaultSettingsRoundTrip` | Serialize defaults to JSON, parse back, values match | US-DAT-003 SC1 |
| `ParseAllFields` | JSON with all fields populates SettingsData correctly | US-DAT-003 SC1 |
| `MissingSectionUsesDefaults` | JSON missing "input" section -> default bindings | US-DAT-003 SC2 |
| `InvalidJsonUsesDefaults` | Malformed JSON -> full defaults, error logged | US-DAT-003 SC3 |
| `LoadExistingValid` | Injected reader returns valid JSON -> settings applied | US-DAT-004 SC1 |
| `MissingFileCreatesDefaults` | Reader throws (file not found) -> defaults written | US-DAT-004 SC2 |
| `LogsLoadPath` | After load, INFO log contains settings path | US-DAT-004 SC3 |
| `UpdateBindingSaves` | update_binding() triggers writer call | US-DAT-005 SC1 |
| `UpdateAudioSaves` | update_audio() triggers writer call | US-DAT-005 SC2 |
| `WriteFailureLogged` | Writer returns false -> error logged, settings kept in memory | US-DAT-005 SC3 |
| `ResolutionTooSmall` | 1x1 -> corrected to 1920x1080, warning | US-DAT-007 SC1 |
| `ResolutionTooLarge` | 99999x99999 -> corrected, warning | US-DAT-007 SC1 |
| `ResolutionMinimumBoundary` | 640x480 accepted without warning | US-DAT-007 SC1 |
| `OffsetClamped` | -10000 -> clamped to -500, warning | US-DAT-007 SC2 |
| `OffsetBoundaryAccepted` | -500 and +500 accepted without warning | US-DAT-007 SC2 |
| `DuplicateBindingRejected` | Same keycode for two inputs -> second gets default | US-DAT-007 SC3 |
| `VolumeClampedToRange` | 1.5 -> 1.0, -0.5 -> 0.0, warnings | US-DAT-007 |
| `SchemaVersionPreserved` | schema_version field persists through round-trip | US-DAT-003 |
| `UnknownFieldsPreserved` | Extra JSON fields survive load + save cycle | (forward compat) |

### Unit Tests (`test/test_atomic_write.cpp`) -- Filesystem, No SDL

Tests use a temp directory under `/tmp`.

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `WritesToTempFile` | During write, .tmp file exists | US-DAT-006 SC1 |
| `RenamesOnSuccess` | After write, target has correct content, no .tmp | US-DAT-006 SC2 |
| `PreservesOriginalOnFailure` | If write fails (simulated), original unchanged | US-DAT-006 SC3 |
| `CreatesNewFile` | If target does not exist, creates it | US-DAT-006 SC2 |
| `OverwritesExistingFile` | If target exists, replaced with new content | US-DAT-006 SC2 |

### Unit Tests (`test/test_song_cache.cpp`) -- Pure Logic, No SDL

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `LoadValidCache` | Injected reader returns valid JSON -> entries populated | US-DAT-031 SC1 |
| `CorruptCacheReturnsFalse` | Invalid JSON -> load() returns false | US-DAT-031 SC2 |
| `FreshCacheValid` | dir mtime older than last_updated -> is_fresh returns true | US-DAT-032 SC1 |
| `StaleCacheInvalid` | dir mtime newer than last_updated -> is_fresh returns false | US-DAT-032 SC2 |
| `MissingCacheReturnsFalse` | Reader throws (no file) -> load() returns false | US-DAT-031 |
| `SaveWritesValidJson` | After set_entries + save, writer receives valid JSON | US-DAT-031 |
| `Load1000EntriesPerformance` | Generate 1000-entry JSON, load, verify < 200ms | US-DAT-033 SC1 |
| `EntryFieldsPreserved` | All ChartCacheEntry fields survive round-trip | US-DAT-031 |
| `EmptyCacheValid` | Cache with zero entries is valid | US-DAT-031 |

### Performance Benchmark (`test/test_song_cache.cpp`)

The 1000-entry performance test generates a synthetic JSON string (not loaded from disk) to isolate parse time from I/O time. The benchmark:

1. Generates a JSON string with 1000 ChartCacheEntry objects (realistic field sizes).
2. Measures wall-clock time to construct SongCache with injected reader and call `load()`.
3. Asserts elapsed < 200ms (conservative; expect ~10-20ms on modern hardware).
4. Logs actual elapsed time for CI visibility.

The story AC specifies "under 1 second for 1000 songs" (US-DAT-033), but the test uses a tighter 200ms threshold to leave headroom for I/O overhead in production. The AC says "under 1000ms" which includes disk I/O; the unit test isolates parse-only.

### Integration Notes

- UserDataDir tests that set `XDG_DATA_HOME` use the `ScopedEnvVar` RAII guard to avoid polluting the test environment.
- Atomic write tests create and clean up temp directories via `std::filesystem::create_directories()` and `std::filesystem::remove_all()` in `SetUp()`/`TearDown()`.
- None of these tests require SDL initialization. All tests are tier 1 (pure logic).

---

*Generated from stories in docs/stories/09-data-management.md (Phase 3 subset)*
*Last updated: 2026-04-30*
