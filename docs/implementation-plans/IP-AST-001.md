# IP-AST-001: Asset Management Phase 1 Implementation Plan

**Design**: TD-AST-001
**Stories**: US-AST-009, US-AST-010, US-AST-032, US-AST-033
**Total Steps**: 4
**Estimated Total**: ~2 hours
**Author**: technical-lead agent
**Status**: Draft

## Prerequisites

- **TD-ENG-001 / IP-ENG-001**: `Engine` and `EngineConfig` must exist in `src/openitup/core/`. This plan adds fields to EngineConfig.
- **CLI11**: Already fetched via CMake FetchContent. Must be linked to the `openitup` main executable target.
- **spdlog**: Already linked to `openitup_engine`. Used for all logging.

The entire DataDirectory subsystem is pure C++ with no SDL dependency. All tests run without SDL.

---

## Step 1: Create DataDirectory Value Type

**Files**:
- Create `src/openitup/asset/data_directory.h` — DataDirectory class declaration, resolve_data_directory() declaration
- Create `src/openitup/asset/data_directory.cpp` — Implementation: validate, find_file_by_extension, find_file_ci, resolve_data_directory
- Modify `CMakeLists.txt` — Add `src/openitup/asset/data_directory.cpp` to `openitup_engine` library sources

**What to implement**:

`DataDirectory` class:
- Constructor takes `std::filesystem::path`, resolves to absolute via `std::filesystem::absolute()`
- `validate()`: calls `std::filesystem::exists()` and `std::filesystem::is_directory()`. Logs error with path on failure. Wraps in try-catch for permission errors.
- `path()`: returns const ref to stored absolute path
- `find_file_by_extension(ext)`: iterates directory with `std::filesystem::directory_iterator`, returns first file whose extension (lowercased) matches. Returns `nullopt` if none found.
- `find_file_ci(filename)`: iterates directory, compares each entry's filename (lowercased) against the query (lowercased). Returns first match or `nullopt`.

`resolve_data_directory(cli_path)`:
```cpp
std::optional<DataDirectory> resolve_data_directory(const std::string& cli_path) {
    // Priority 1: CLI argument
    if (!cli_path.empty()) {
        return DataDirectory(cli_path);
    }

    // Priority 2: Environment variable
    const char* env = std::getenv("OPENITUP_DATA_DIR");
    if (env && env[0] != '\0') {
        spdlog::info("Using data directory from OPENITUP_DATA_DIR: {}", env);
        return DataDirectory(env);
    }

    // Priority 3: Error
    spdlog::error("No data directory specified. Use --data-dir <path> or set OPENITUP_DATA_DIR");
    return std::nullopt;
}
```

**Tests**:
- Create `test/test_data_directory.cpp` — Unit tests for DataDirectory
- Modify `CMakeLists.txt` — Add `test/test_data_directory.cpp` to `openitup_tests`

Scoped environment variable helper for tests:
```cpp
struct ScopedEnvVar {
    const char* name_;
    std::string original_;
    bool had_original_;

    ScopedEnvVar(const char* name, const char* value)
        : name_(name) {
        const char* orig = std::getenv(name);
        had_original_ = (orig != nullptr);
        if (had_original_) original_ = orig;
        setenv(name, value, 1);
    }
    ~ScopedEnvVar() {
        if (had_original_) setenv(name_, original_.c_str(), 1);
        else unsetenv(name_);
    }
};
```

Test cases (all use `/tmp` temp directories, no SDL):
- `ValidDirectoryPasses` — Create temp dir, validate() returns true
- `NonexistentDirectoryFails` — validate() returns false for "/tmp/nonexistent_test_dir_xxx"
- `PathResolvedToAbsolute` — DataDirectory("./relative") -> path().is_absolute() == true
- `FindFileByExtensionFound` — Create temp dir with "test.ksf", find_file_by_extension(".ksf") returns it
- `FindFileByExtensionMissing` — Empty dir, find_file_by_extension(".ksf") returns nullopt
- `FindFileByExtensionCaseInsensitive` — Create "test.KSF", find_file_by_extension(".ksf") returns it
- `FindFileCiFound` — Create "SONG.ogg", find_file_ci("song.ogg") returns it
- `FindFileCiMissing` — find_file_ci("missing.ogg") returns nullopt

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R DataDirectory` passes all tests
- [ ] No SDL headers in data_directory.h or data_directory.cpp

**Expected commit message**:
`feat(asset): add DataDirectory for path validation and case-insensitive file discovery`

**Estimated time**: ~40 minutes

---

## Step 2: Implement resolve_data_directory with Environment Variable

**Files**:
- Modify `test/test_data_directory.cpp` — Add resolve_data_directory tests with ScopedEnvVar

**What to implement**:

The `resolve_data_directory()` function is already implemented in Step 1's `.cpp` file. This step focuses on testing the priority logic with environment variable interaction.

Test cases:
- `ResolveCliPathUsed` — resolve_data_directory("/tmp/cli_dir") uses CLI path
- `ResolveEnvFallback` — Set OPENITUP_DATA_DIR, resolve with empty CLI → env path used
- `ResolveCliOverridesEnv` — Set env AND pass CLI → CLI path used, not env
- `ResolveNeitherReturnsNullopt` — Unset env, empty CLI → nullopt returned
- `ResolveEnvEmptyStringReturnsNullopt` — Set OPENITUP_DATA_DIR="" → treated as unset
- `ResolveCliPathValidated` — CLI path to non-existent dir → DataDirectory returned but validate() fails

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R DataDirectory` passes all tests including env var tests
- [ ] ScopedEnvVar correctly restores environment between tests

**Expected commit message**:
`test(asset): add resolve_data_directory tests with environment variable fallback`

**Estimated time**: ~25 minutes

---

## Step 3: Wire CLI Parsing into main.cpp

**Files**:
- Modify `src/openitup/core/engine.h` — Add `data_dir_path` and `chart_path` to EngineConfig
- Modify `src/openitup/main.cpp` — Add CLI11 parsing, env var resolution, validation
- Modify `CMakeLists.txt` — Link `CLI11::CLI11` to `openitup` executable target

**What to implement**:

Add to `EngineConfig`:
```cpp
struct EngineConfig {
    std::string window_title = "openitup";
    int window_width = 1280;
    int window_height = 960;
    double target_fps = 0.0;
    std::string data_dir_path;   // NEW: resolved data directory
    std::string chart_path;       // NEW: optional explicit chart path
};
```

Update `main.cpp`:
```cpp
#include <CLI/CLI.hpp>
#include <openitup/asset/data_directory.h>
#include <openitup/core/engine.h>
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    CLI::App app{"openitup - Pump It Up engine"};

    std::string data_dir_arg;
    std::string chart_arg;
    app.add_option("--data-dir", data_dir_arg, "Path to game data / song directory");
    app.add_option("--chart", chart_arg, "Path to chart file (.ksf)");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    // Resolve data directory
    auto data_dir = openitup::resolve_data_directory(data_dir_arg);
    if (!data_dir.has_value()) {
        return 1;
    }
    if (!data_dir->validate()) {
        return 1;
    }

    try {
        openitup::EngineConfig config;
        config.data_dir_path = data_dir->path().string();
        config.chart_path = chart_arg;

        openitup::Engine engine(config);
        // Phase 1: run_gameplay() will be wired in IP-SCN-001
        return engine.run();
    } catch (const std::exception& e) {
        spdlog::critical("Fatal startup error: {}", e.what());
        return 1;
    }
}
```

Update CMakeLists.txt:
```cmake
target_link_libraries(openitup PRIVATE openitup_engine CLI11::CLI11)
```

**Tests**:

No automated test for CLI parsing (would require launching subprocess). Manual verification:

```bash
# Test --data-dir
./build/openitup --data-dir /path/to/song/
# Expected: engine starts (or logs "No data directory" if path invalid)

# Test env var
export OPENITUP_DATA_DIR=/path/to/song/
./build/openitup
# Expected: engine starts using env var path

# Test no path
unset OPENITUP_DATA_DIR
./build/openitup
# Expected: error log and exit code 1

# Test --help
./build/openitup --help
# Expected: CLI11 help text with --data-dir and --chart options
```

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `./build/openitup --help` shows help with --data-dir and --chart
- [ ] `./build/openitup` (no args, no env) logs error and exits with code 1
- [ ] `./build/openitup --data-dir /nonexistent` logs "not found" error and exits 1

**Expected commit message**:
`feat(asset): wire CLI11 argument parsing and env var fallback into main.cpp`

**Estimated time**: ~30 minutes

---

## Step 4: Missing Asset Logging Framework

**Files**:
- Modify `src/openitup/asset/data_directory.cpp` — Enhance find methods with structured logging
- Modify `test/test_data_directory.cpp` — Add logging verification tests

**What to implement**:

Structured logging pattern for all asset discovery methods:

```cpp
std::optional<std::filesystem::path> DataDirectory::find_file_by_extension(
    const std::string& extension) const {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(path_)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            // Lowercase comparison
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == extension) {
                return entry.path();
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Failed to scan directory '{}': {}", path_.string(), e.what());
    }
    return std::nullopt;  // caller logs context-specific message
}
```

The pattern for all missing assets: the find method returns nullopt, and the CALLER logs the context-specific message. This ensures each log message includes:
1. What type of asset was expected
2. What path was searched
3. What component requested it
4. The severity (ERROR for critical, WARN for degraded, INFO for optional)

Examples that will be used by GameplayScene (TD-SCN-001):
```
spdlog::error("Chart not found: no .ksf files in '{}'", data_dir.path().string());
spdlog::warn("Audio not found: '{}' referenced by chart not in '{}'", audio_file, data_dir.path().string());
spdlog::info("BGA not found in '{}', proceeding with black background", data_dir.path().string());
```

Test cases (verify logging patterns):
- `FindFileMissingNoErrorLogged` — find_file_by_extension on empty dir -> no error logged (caller logs)
- `DirectoryScanErrorLogged` — Permission-denied directory -> error logged with path
- `MultipleFindCallsIndependent` — Two successive find calls don't interfere

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R DataDirectory` passes all tests
- [ ] Structured logging follows US-AST-033 pattern

**Expected commit message**:
`feat(asset): add structured logging for missing asset discovery`

**Estimated time**: ~25 minutes

---

## Summary

| Step | What | Files Created/Modified | Stories Covered | Est. |
|------|------|----------------------|-----------------|------|
| 1 | DataDirectory value type | 2 new + 1 modified | US-AST-009 | 40m |
| 2 | Env var resolution tests | 1 modified | US-AST-010 | 25m |
| 3 | CLI11 wiring in main.cpp | 3 modified | US-AST-009, US-AST-010 | 30m |
| 4 | Missing asset logging | 2 modified | US-AST-032, US-AST-033 | 25m |

**Total new source files**: 2 (1 header + 1 .cpp in `src/openitup/asset/`)
**Total new test files**: 1 (`test/test_data_directory.cpp`)
**CMakeLists.txt modifications**: Add 1 .cpp to `openitup_engine`, add 1 test file to `openitup_tests`, link CLI11 to `openitup` target

Each step is independently committable. Steps 1-2 are fully tested without SDL. Step 3 requires manual CLI verification. Step 4 enhances logging for all future asset discovery paths.

## Acceptance Verification

| Story ID | How to verify |
|----------|--------------|
| US-AST-009 | `./build/openitup --data-dir /path/` loads from specified directory. Missing dir logs error + exits 1. |
| US-AST-010 | `OPENITUP_DATA_DIR=/path/ ./build/openitup` uses env var. CLI overrides env. Neither → error + exit 1. |
| US-AST-032 | `test/test_data_directory.cpp`: `FindFileByExtensionMissing` returns nullopt. GameplayScene (IP-SCN-001) proceeds with black background. |
| US-AST-033 | All missing assets logged with path, context, and severity. `test/test_data_directory.cpp`: `DirectoryScanErrorLogged`. |
