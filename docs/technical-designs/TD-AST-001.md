# TD-AST-001: Asset Management — CLI Data Directory, Environment Variable, and Missing Asset Handling

**Stories**: US-AST-009, US-AST-010, US-AST-032, US-AST-033
**Phase**: 1
**Author**: technical-architect agent
**Status**: Draft

## Overview

This design introduces the Phase 1 asset management subsystem: CLI argument parsing for a `--data-dir` path, environment variable fallback via `OPENITUP_DATA_DIR`, path validation with structured error logging, and graceful handling of missing optional assets (BGA files). The design extends the existing CLI11 dependency (already used by `bga_player` and `spr2sprj`) to the main `openitup` executable and adds a lightweight `DataDirectory` value type that validates and resolves the game data path at startup.

This is deliberately minimal — Phase 1 requires a single path to one song directory, not a full song database scan. The `DataDirectory` class resolves and validates the path, then the `GameplayScene` (TD-SCN-001) uses it to locate the chart, audio, and optional BGA files within that directory.

## Architecture

### Component Diagram

```
main.cpp
  |
  |  CLI11 parses --data-dir and --chart args
  |  Falls back to OPENITUP_DATA_DIR env var
  |
  v
DataDirectory (src/openitup/asset/data_directory.h)
  |  validates path, stores resolved absolute path
  |
  v
Engine
  |  stores DataDirectory in EngineConfig
  |
  v
GameplayScene (TD-SCN-001)
  |  uses data_dir to locate files:
  |  - chart: data_dir / "*.ksf"
  |  - audio: resolved from chart metadata
  |  - bga: data_dir / "*.bgaj" (optional, null if missing)
```

### New Types

#### `DataDirectory` (`src/openitup/asset/data_directory.h`)

A value type that validates and resolves a game data directory path. Provides helper methods to locate specific asset types within the directory.

```cpp
// src/openitup/asset/data_directory.h
#pragma once

#include <filesystem>
#include <string>
#include <optional>

namespace openitup {

class DataDirectory {
public:
    // Construct from a path. Resolves to absolute. Does NOT validate existence
    // at construction (validation is a separate step for clearer error reporting).
    explicit DataDirectory(std::filesystem::path path);

    // Validate that the directory exists and is accessible.
    // Returns true if valid. Logs ERROR with path if not.
    bool validate() const;

    // The resolved absolute path to the data directory.
    const std::filesystem::path& path() const;

    // Find the first file matching a glob pattern in the directory.
    // Returns nullopt if no match found. Logs WARN with context.
    std::optional<std::filesystem::path> find_file_by_extension(
        const std::string& extension) const;

    // Find a specific file by name (case-insensitive).
    // Returns nullopt if not found.
    std::optional<std::filesystem::path> find_file_ci(
        const std::string& filename) const;

private:
    std::filesystem::path path_;
};

// Resolve the data directory from CLI argument and environment variable.
// Priority: cli_path > OPENITUP_DATA_DIR > empty (error).
// Returns nullopt if no path available (logs ERROR with instructions).
std::optional<DataDirectory> resolve_data_directory(
    const std::string& cli_path);

} // namespace openitup
```

**Key decisions**:

- `DataDirectory` is a value type (copyable, movable). It stores a resolved `std::filesystem::path`. Construction does not validate — `validate()` is a separate method so the caller can report errors at the appropriate point (before Engine construction).
- `resolve_data_directory()` is a free function, not a method. It encapsulates the priority logic (CLI > env var > error) and is the single point where the environment variable `OPENITUP_DATA_DIR` is read.
- `find_file_by_extension()` scans the directory for the first file with a given extension (e.g., `.ksf`, `.ogg`, `.bgaj`). This is how the GameplayScene discovers the chart and audio files in Phase 1 without a song database.
- `find_file_ci()` does case-insensitive filename matching, following the existing `TextureCache` pattern (`src/openitup/gfx/texture_cache.cpp`) where filenames from original game data may have inconsistent casing.
- No recursive scanning. Phase 1 assumes the data directory IS the song directory (contains chart, audio, and optional BGA directly). Phase 3 (US-AST-012) adds recursive scanning.

---

### Modified Types

#### `EngineConfig` (`src/openitup/core/engine.h`)

- Add field: `std::string data_dir_path` — The resolved data directory path, passed from main.cpp after CLI parsing.
- Add field: `std::string chart_path` — Optional explicit chart file path (for `--chart` flag).
- Reason: GameplayScene needs access to the data directory to find assets. Storing the path in EngineConfig avoids global state and follows the DI pattern from TD-ENG-001.

#### `main.cpp` (`src/openitup/main.cpp`)

- Add CLI11 argument parsing: `--data-dir <path>`, `--chart <path>`
- Add environment variable fallback for `OPENITUP_DATA_DIR`
- Add validation before Engine construction
- Reason: US-AST-009 and US-AST-010 require command-line and environment variable support.

## File Plan

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/openitup/asset/data_directory.h` | DataDirectory class + resolve_data_directory() declaration |
| Create | `src/openitup/asset/data_directory.cpp` | DataDirectory implementation, env var reading, path validation |
| Modify | `src/openitup/core/engine.h` | Add data_dir_path and chart_path to EngineConfig |
| Modify | `src/openitup/main.cpp` | Add CLI11 parsing, env var fallback, validation |
| Modify | `CMakeLists.txt` | Add data_directory.cpp to openitup_engine, link CLI11 to openitup executable |
| Create | `test/test_data_directory.cpp` | Unit tests for DataDirectory and resolve_data_directory |
| Modify | `CMakeLists.txt` | Add test_data_directory.cpp to openitup_tests |

## Data Flow

### Startup: CLI Argument Provided

```
1. main(argc, argv):
   CLI11::App app("openitup")
   std::string data_dir_arg;
   app.add_option("--data-dir", data_dir_arg, "Path to game data directory")
   app.parse(argc, argv)

2. auto data_dir = resolve_data_directory(data_dir_arg)
   a. data_dir_arg is non-empty → DataDirectory(data_dir_arg)
   b. validate() → true
   c. return DataDirectory

3. EngineConfig config;
   config.data_dir_path = data_dir->path().string();
   Engine engine(config);
```

### Startup: Environment Variable Fallback

```
1. main(argc, argv):
   data_dir_arg is empty (no --data-dir flag)

2. resolve_data_directory(""):
   a. cli_path is empty
   b. Read OPENITUP_DATA_DIR: "/home/user/piu/"
   c. DataDirectory("/home/user/piu/")
   d. validate() → true
   e. return DataDirectory
```

### Startup: No Path Specified

```
1. resolve_data_directory(""):
   a. cli_path is empty
   b. OPENITUP_DATA_DIR is unset (getenv returns nullptr)
   c. spdlog::error("No data directory specified. Use --data-dir or set OPENITUP_DATA_DIR")
   d. return std::nullopt

2. main():
   if (!data_dir.has_value()) return 1;
```

### Missing BGA (US-AST-032)

```
GameplayScene init:
  auto bgaj = data_dir.find_file_by_extension(".bgaj");
  if (bgaj.has_value()) {
      load_bga(*bgaj);  // optional background
  } else {
      spdlog::info("No BGA found in {}, playing with black background",
                   data_dir.path().string());
      // bga_ remains nullptr — render skips it
  }
```

### Missing Asset Logging (US-AST-033)

```
All asset loading paths follow the same pattern:

1. Attempt to find/load asset
2. On failure, log structured message:
   spdlog::error("asset type not found: 'filename' in {} (context: {})",
                 directory, requesting_component);

Examples:
  "chart not found: no .ksf files in /data/Pumptris/ (GameplayScene init)"
  "audio not found: 'pumptris.ogg' in /data/Pumptris/ (chart references this file)"
  "BGA not found: no .bgaj files in /data/Pumptris/ (optional, proceeding without)"
```

## Dependencies

### Internal
- **Engine/EngineConfig** (`src/openitup/core/engine.h`, TD-ENG-001) — EngineConfig carries the data directory path to subsystems.
- **spdlog** — All logging. Already linked via `openitup_engine`.
- **std::filesystem** — Path resolution, directory existence checking, directory iteration. Part of C++17 standard library.

### External (new libraries)
- **CLI11** — Already in the project (used by `bga_player`, `spr2sprj`, `bga2bgaj`). Must be linked to the `openitup` main executable target, not just tool targets.

## Architectural Decisions

### ADR-1: DataDirectory as Value Type, Not Service

- **Context**: Asset management could be a singleton service, an Engine-owned subsystem, or a simple value type passed through config.
- **Decision**: `DataDirectory` is a plain value type. It validates a path and provides file-finding helpers. No singleton, no service registry.
- **Alternatives considered**: (a) `AssetManager` class owned by Engine — over-engineered for Phase 1's single-directory needs. Phase 3 introduces a proper SongDatabase. (b) Global `g_data_dir` path — violates the project's no-globals convention. (c) Pass raw `std::string` everywhere — loses type safety and validation guarantees.
- **Consequences**: Clean, testable, minimal. Phase 3's SongDatabase can consume DataDirectory as input without changing the Phase 1 interface. No service lifecycle to manage.

### ADR-2: CLI11 for Main Executable

- **Context**: CLI argument parsing is needed. CLI11 is already in the project for tools.
- **Decision**: Link CLI11 to the `openitup` main executable and use it for `--data-dir` and `--chart` parsing.
- **Alternatives considered**: (a) Manual `argc`/`argv` parsing — error-prone, reinvents help text. (b) A different library — CLI11 is already fetched and proven.
- **Consequences**: One line in CMakeLists.txt to link `CLI11::CLI11` to the `openitup` target. Consistent with the existing tools.

### ADR-3: Environment Variable as Secondary Source

- **Context**: US-AST-010 requires an environment variable fallback with CLI override.
- **Decision**: Priority order is: `--data-dir` flag > `OPENITUP_DATA_DIR` env var > error. Implemented in `resolve_data_directory()`.
- **Alternatives considered**: (a) Config file as additional source — deferred to Phase 3 (US-AST-011). (b) Default `./data/` directory — too implicit, could silently load wrong data.
- **Consequences**: One function encapsulates all resolution logic. Easy to extend with config file support in Phase 3 by adding another fallback step.

### ADR-4: Missing BGA Is Info, Not Error

- **Context**: US-AST-032 says missing BGA should not prevent gameplay. US-AST-033 says all missing assets should be logged.
- **Decision**: Missing BGA logs at INFO level. Missing chart or audio logs at ERROR level.
- **Alternatives considered**: (a) All missing assets at WARN — conflates critical (chart) with optional (BGA). (b) No logging for optional assets — violates US-AST-033.
- **Consequences**: Log output clearly distinguishes critical from optional. Developers can filter logs to see only errors for critical debugging.

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Environment variable not set on some platforms (e.g., systemd service) | Low | Low | Error message explicitly mentions both `--data-dir` and `OPENITUP_DATA_DIR`. Phase 3 config file provides another path. |
| Case-insensitive file search slow on large directories (1000+ files) | Low | Low | Phase 1 song directories typically have < 20 files. `find_file_ci()` does a single directory scan. Phase 3 caches results. |
| CLI11 version incompatibility with existing tools | Low | Very Low | All use the same FetchContent-pinned version (v2.4.2). |
| std::filesystem::exists() throws on permission errors | Med | Low | Wrap in try-catch in validate(). Log the specific filesystem error. |

## Testing Strategy

### Unit Tests (`test/test_data_directory.cpp`) — Pure Logic, No SDL

All tests use temporary directories created in `/tmp` via `std::filesystem::create_directories()`.

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `ValidDirectoryPasses` | validate() returns true for existing directory | US-AST-009 SC1 |
| `NonexistentDirectoryFails` | validate() returns false for missing path | US-AST-009 SC2 |
| `PathResolvedToAbsolute` | Relative path "./data" resolved to absolute | US-AST-009 |
| `FindFileByExtension` | .ksf file found in temp directory | US-AST-009 SC1 |
| `FindFileByExtensionMissing` | No .ksf returns nullopt | US-AST-009 SC3 |
| `FindFileCaseInsensitive` | "SONG.OGG" found when searching for "song.ogg" | US-AST-033 |
| `ResolveCliOverridesEnv` | CLI path used when both CLI and env are set | US-AST-010 SC2 |
| `ResolveEnvFallback` | Env var used when CLI is empty | US-AST-010 SC1 |
| `ResolveNeitherReturnsNullopt` | No CLI, no env var -> nullopt | US-AST-010 SC3 |
| `MissingBgaReturnsNullopt` | No .bgaj file -> find returns nullopt, no error logged | US-AST-032 SC1 |
| `MissingAssetLoggedWithContext` | Missing file logs with path and component context | US-AST-033 SC1 |

### Integration Notes

Environment variable tests use a helper that temporarily sets/unsets `OPENITUP_DATA_DIR`:
```cpp
struct ScopedEnvVar {
    ScopedEnvVar(const char* name, const char* value);
    ~ScopedEnvVar();  // restores original value
};
```

This avoids polluting the test environment across test cases.

---

*Generated from stories in docs/stories/11-asset-management.md (Phase 1 subset)*
*Last updated: 2026-04-28*
