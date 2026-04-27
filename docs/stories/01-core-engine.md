# Core Engine User Stories

This document contains vertically-sliced user stories for the core engine requirements (REQ-ENG-001 through REQ-ENG-008). Stories are organized by requirement and marked with implementation status.

---

## Epic: Fixed-Step Game Loop

### Story ID: US-ENG-001 - Implement Time Accumulator for Fixed Logic Step

**Story Card:**
> **As a** Developer
> **I want** a time accumulator that decouples game logic updates from rendering frame rate
> **So that** the engine produces deterministic gameplay timing regardless of display refresh rate

**References**: REQ-ENG-001, Roadmap Phase 1

**Status**: PLANNED

### 📝 Description
Implement a time accumulator pattern that measures wall-clock time between frames, accumulates it, and steps game logic in fixed 1/60th second increments. This is the foundation for deterministic judge timing and includes drift prevention and spiral-of-death protection for stable operation over multi-hour sessions.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Logic runs at 60 Hz on a 60 Hz display**
    *   **Given** the display refresh rate is 60 Hz
    *   **When** the engine runs for 10 seconds
    *   **Then** exactly 600 logic ticks occur (±1 tick tolerance for startup/shutdown)

*   **Scenario 2: Logic runs at 60 Hz on a 120 Hz display**
    *   **Given** the display refresh rate is 120 Hz
    *   **When** the engine runs for 10 seconds
    *   **Then** exactly 600 logic ticks occur (±1 tick tolerance)
    *   **And** rendering occurs at approximately 1200 frames

*   **Scenario 3: Multiple logic steps per frame under slowdown**
    *   **Given** the previous frame took 50 milliseconds to render
    *   **When** the time accumulator processes the next frame
    *   **Then** 3 logic updates execute (3 × 16.67ms ≈ 50ms)
    *   **And** rendering occurs once after all logic updates complete

*   **Scenario 4: Partial accumulator carries to next frame**
    *   **Given** 20 milliseconds have accumulated
    *   **When** logic steps once (consuming 16.67ms)
    *   **Then** approximately 3.33ms remains in the accumulator
    *   **And** the next frame starts with that remainder

*   **Scenario 5: No drift after simulated 2 hours**
    *   **Given** an injectable test clock is used as the time source
    *   **When** 432,000 ticks are simulated (2 hours at 60 Hz)
    *   **Then** cumulative timing error is under 50 microseconds

*   **Scenario 6: Spiral-of-death guard**
    *   **Given** the previous frame took 500 milliseconds
    *   **When** the time accumulator processes the next frame
    *   **Then** at most 10 logic updates execute
    *   **And** excess time is discarded to prevent death spiral

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points
*   **Dependencies**: US-ENG-021 (Clock utility)
*   **Implementation Note**: Fixed step = 1000ms / 60 = 16.666... milliseconds. Use high-precision timer to avoid drift. Injectable clock for testability without wall-clock delays.

---

### Story ID: US-ENG-003 - Support Uncapped and Variable Refresh Rate Rendering

**Story Card:**
> **As a** Player with a high-refresh-rate monitor
> **I want** smooth rendering at 120+ Hz while maintaining 60 Hz logic
> **So that** I experience fluid visuals without affecting gameplay timing

**References**: REQ-ENG-001

**Status**: PLANNED

### 📝 Description
Allow rendering to run at display refresh rate (60, 120, 144 Hz) or uncapped, independently from the fixed 60 Hz logic tick rate.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Rendering at native display rate**
    *   **Given** an injectable frame-rate target of 144 FPS is configured
    *   **When** the engine runs in test mode
    *   **Then** rendering occurs at approximately 144 FPS
    *   **And** logic continues to tick at exactly 60 Hz

*   **Scenario 2: Uncapped rendering mode**
    *   **Given** the engine is configured for uncapped frame rate
    *   **When** running in test mode with no throttling
    *   **Then** rendering frame rate exceeds the logic tick rate
    *   **And** logic maintains exactly 60 Hz

*   **Scenario 3: Frame limiter applied**
    *   **Given** a frame-rate target is configured
    *   **When** rendering a frame that completes early
    *   **Then** the engine waits until the target frame time has elapsed
    *   **And** logic tick rate is unaffected by frame limiting

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-ENG-001, US-ENG-011 (Engine class)
*   **Implementation Note**: SDL3 renderer configuration for presentation interval. Injectable frame-rate target for unit testing without hardware dependency.

---

### Story ID: US-ENG-004 - Render State Interpolation

**Story Card:**
> **As a** Player with a high-refresh-rate monitor
> **I want** smooth sprite and BGA motion between logic ticks
> **So that** rendering appears fluid at 144 Hz even though logic runs at 60 Hz

**References**: REQ-ENG-001

**Status**: PLANNED

### 📝 Description
Use the accumulator remainder (time between last logic tick and current render) to interpolate visual state between the previous and current logic states. This eliminates judder on high-refresh displays.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Interpolation factor computed from accumulator**
    *   **Given** the accumulator contains 8.33ms after a logic tick
    *   **When** computing the render interpolation factor (alpha)
    *   **Then** alpha = 8.33 / 16.67 ≈ 0.5
    *   **And** rendering interpolates halfway between the previous and current logic states

*   **Scenario 2: Sprite position interpolated**
    *   **Given** a sprite was at (100, 100) last tick and is now at (120, 100)
    *   **When** rendering with alpha = 0.5
    *   **Then** the sprite is drawn at (110, 100)

*   **Scenario 3: BGA layer position interpolated**
    *   **Given** a BGA layer translated from (0, 0) to (40, 0) between ticks
    *   **When** rendering with alpha = 0.25
    *   **Then** the layer is drawn at (10, 0)

*   **Scenario 4: No interpolation when alpha is zero**
    *   **Given** the accumulator is empty immediately after a logic tick
    *   **When** rendering occurs
    *   **Then** alpha = 0.0
    *   **And** rendering uses the current logic state without interpolation

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-ENG-001, US-ENG-003
*   **Implementation Note**: Alpha = accumulator_remainder / fixed_step_ms. Store previous and current positions in render state.

---

## Epic: Engine Architecture

### Story ID: US-ENG-011 - Create Engine Class as Subsystem Owner

**Story Card:**
> **As a** Developer
> **I want** a root Engine class that owns all major subsystems
> **So that** subsystem lifetime and dependencies are clearly managed

**References**: REQ-ENG-002, Roadmap Phase 1

**Status**: PLANNED

### 📝 Description
Create an `Engine` class that serves as the root of the object graph, owning the renderer and scene stack. Audio system, input system integration will be added in their respective story sets (see note below). This centralizes initialization and shutdown sequencing.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Subsystems initialized in correct order**
    *   **Given** the Engine constructor is called
    *   **When** subsystems are initialized
    *   **Then** renderer initializes first (SDL window/context)
    *   **And** scene stack initializes last

*   **Scenario 2: Subsystems shut down in reverse order**
    *   **Given** the Engine destructor is called
    *   **When** subsystems are destroyed
    *   **Then** scene stack destroys first (allowing scenes to clean up)
    *   **And** renderer destroys last

*   **Scenario 3: Subsystems access each other through Engine**
    *   **Given** a scene needs to access the renderer
    *   **When** the scene calls `engine.get_renderer()`
    *   **Then** a valid reference to the renderer is returned
    *   **And** the scene does not directly own or store the renderer

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: None (foundational)
*   **Implementation Note**: Replace prototype loop in main.cpp. Audio/input/scene-stack integration into Engine ownership will be covered in their respective story sets (REQ-AUD, REQ-INP, REQ-SCN).

---

### Story ID: US-ENG-012 - Integrate Renderer into Engine Ownership

**Story Card:**
> **As a** Developer
> **I want** the Engine to own the existing Renderer instance
> **So that** rendering lifecycle is managed centrally

**References**: REQ-ENG-002

**Status**: PLANNED

### 📝 Description
Transfer ownership of the existing `Renderer` class (which already handles SDL_Window and SDL_Renderer) to the `Engine` class.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Engine creates renderer during initialization**
    *   **Given** Engine constructor is called with display configuration
    *   **When** initialization completes
    *   **Then** the Renderer exists with a valid SDL window
    *   **And** the window uses 640×480 logical resolution with letterboxing

*   **Scenario 2: Scenes access renderer through Engine**
    *   **Given** a scene needs to render content
    *   **When** the scene calls `engine.get_renderer()`
    *   **Then** a valid reference to the Renderer is returned

*   **Scenario 3: Renderer destroyed with Engine**
    *   **Given** the Engine is destroyed
    *   **When** the destructor runs
    *   **Then** the Renderer is destroyed before Engine completes destruction
    *   **And** SDL cleanup occurs without errors

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-ENG-011
*   **Implementation Note**: Renderer class already exists and is tested

---

## Epic: Timing System

### Story ID: US-ENG-021 - Create Clock Utility Wrapping SDL Performance Counter

**Story Card:**
> **As a** Developer
> **I want** a Clock utility that provides high-precision timing
> **So that** I can accurately measure frame delta time and fixed step intervals

**References**: REQ-ENG-003, Roadmap Phase 1

**Status**: PLANNED

### 📝 Description
Create a `Clock` class that wraps SDL_GetPerformanceCounter and SDL_GetPerformanceFrequency to provide microsecond-accurate delta time, elapsed time, and fixed step tracking. Must support injectable counter sources for unit testing without wall-clock delays.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Delta time measurement**
    *   **Given** the Clock is started
    *   **When** `get_delta_time()` is called
    *   **Then** the time since the last call is returned in seconds
    *   **And** precision is within 1 microsecond

*   **Scenario 2: Elapsed time tracking**
    *   **Given** the Clock has been running for 5.5 seconds
    *   **When** `get_elapsed_time()` is called
    *   **Then** the value returned is 5.5 seconds (±0.001 tolerance)

*   **Scenario 3: Reset functionality**
    *   **Given** the Clock has been running
    *   **When** `reset()` is called
    *   **Then** elapsed time returns to zero
    *   **And** the next delta time is measured from the reset point

*   **Scenario 4: Precision after simulated 2 hours**
    *   **Given** an injectable counter source is used
    *   **When** 432,000 measurements are taken (2 hours at 60 Hz)
    *   **Then** cumulative measurement error is under 50 microseconds

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: None
*   **Implementation Note**: Use double-precision floating point for time values. Injectable counter source for testability.

---

## Epic: Platform Support

### Story ID: US-ENG-031 - Verify Linux Cross-Distribution Compatibility

**Story Card:**
> **As a** Player on Linux
> **I want** to run the engine on Ubuntu, Debian, Arch, and Fedora without modification
> **So that** I don't need to manually patch or configure the build

**References**: REQ-ENG-004

**Status**: DONE

### 📝 Description
Ensure the engine compiles and runs on major Linux distributions without requiring distribution-specific patches.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Ubuntu/Debian build**
    *   **Given** a clean Ubuntu 22.04 installation with CMake and GCC
    *   **When** running the standard build commands
    *   **Then** compilation succeeds without errors
    *   **And** all tests pass

*   **Scenario 2: Arch Linux build**
    *   **Given** a clean Arch Linux installation with CMake and Clang
    *   **When** running the standard build commands
    *   **Then** compilation succeeds without errors
    *   **And** all tests pass

*   **Scenario 3: No distribution-specific code paths**
    *   **Given** the codebase
    *   **When** searching for distribution detection (e.g., `#ifdef UBUNTU`)
    *   **Then** no such conditionals exist outside of sanctioned abstraction layers

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points (already completed)
*   **Dependencies**: None
*   **Implementation Note**: CI/CD should test multiple distributions

---

### Story ID: US-ENG-032 - Verify Windows Platform Compatibility

**Story Card:**
> **As a** Player on Windows
> **I want** to compile and run the engine using MSVC or MinGW
> **So that** I can play on my Windows gaming PC

**References**: REQ-ENG-004

**Status**: DONE

### 📝 Description
Ensure the engine compiles and runs on Windows using both MSVC (Visual Studio) and MinGW (GCC on Windows) toolchains.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: MSVC compilation**
    *   **Given** a clean Windows 11 installation with Visual Studio 2022
    *   **When** running CMake with the MSVC generator
    *   **Then** compilation succeeds without warnings
    *   **And** all tests pass

*   **Scenario 2: MinGW compilation**
    *   **Given** a Windows installation with MinGW-w64
    *   **When** running CMake with the MinGW generator
    *   **Then** compilation succeeds without warnings
    *   **And** all tests pass

*   **Scenario 3: Platform-specific code isolated**
    *   **Given** platform-specific code exists (file paths, line endings)
    *   **When** reviewing the codebase
    *   **Then** all such code is isolated in abstraction layers or uses cross-platform APIs (std::filesystem)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points (already completed)
*   **Dependencies**: None
*   **Implementation Note**: CI/CD should test both Windows toolchains

---

## Epic: Build System

### Story ID: US-ENG-041 - Configure CMake FetchContent for All Dependencies

**Story Card:**
> **As a** Developer
> **I want** all dependencies fetched automatically via CMake FetchContent
> **So that** I can build the project on a clean machine without manual setup

**References**: REQ-ENG-005

**Status**: DONE

### 📝 Description
Configure CMake to automatically download and build SDL3, SDL3_image, nlohmann/json, spdlog, CLI11, and GoogleTest using FetchContent.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Clean machine build**
    *   **Given** a fresh Ubuntu installation with only CMake 3.20+ and GCC installed
    *   **When** running `cmake -B build && cmake --build build`
    *   **Then** all dependencies are fetched automatically
    *   **And** the build completes successfully within 10 minutes

*   **Scenario 2: No manual dependency installation required**
    *   **Given** the build instructions in the README
    *   **When** a new developer follows them
    *   **Then** no step requires manually downloading, compiling, or installing libraries

*   **Scenario 3: Dependencies cached between builds**
    *   **Given** a successful build has completed
    *   **When** running `cmake --build build` again without changes
    *   **Then** dependencies are not re-downloaded or re-compiled

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points (already completed)
*   **Dependencies**: None
*   **Implementation Note**: Already implemented in current CMakeLists.txt

---

### Story ID: US-ENG-042 - Optimize Incremental Build Performance

**Story Card:**
> **As a** Developer
> **I want** incremental builds to complete in under 30 seconds
> **So that** my edit-compile-test cycle is fast

**References**: REQ-ENG-005

**Status**: DONE

### 📝 Description
Ensure typical source file changes result in fast incremental compilation without re-building large portions of the dependency tree.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Single source file change**
    *   **Given** a successful full build exists
    *   **When** modifying one .cpp file in src/ and rebuilding
    *   **Then** compilation completes in under 10 seconds

*   **Scenario 2: Header change with limited dependencies**
    *   **Given** a successful full build exists
    *   **When** modifying a header file included by 3 source files
    *   **Then** compilation completes in under 30 seconds

*   **Scenario 3: Test file change does not rebuild engine**
    *   **Given** a successful full build exists
    *   **When** modifying a test file in test/
    *   **Then** only the test executable is recompiled
    *   **And** the main engine library is not recompiled

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points (already completed)
*   **Dependencies**: US-ENG-041
*   **Implementation Note**: Proper CMake target separation ensures this

---

## Epic: Error Handling and Logging

### Story ID: US-ENG-061 - Log Asset Loading Errors with Context

**Story Card:**
> **As a** Developer
> **I want** all asset loading failures logged with file paths and error reasons
> **So that** I can diagnose why a song or texture failed to load

**References**: REQ-ENG-007

**Status**: DONE

### 📝 Description
Ensure all asset loading failures (missing files, parse errors, unsupported formats) are logged at ERROR level with sufficient context to diagnose the issue.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Missing texture file logged**
    *   **Given** a sprite references a texture file that doesn't exist
    *   **When** attempting to load the sprite
    *   **Then** an ERROR-level log message appears containing:
        *   The sprite file path
        *   The missing texture file name
        *   The probed paths (.tga, .png, .dds)

*   **Scenario 2: Malformed JSON logged**
    *   **Given** a .bgaj file contains invalid JSON syntax
    *   **When** attempting to load the BGA animation
    *   **Then** an ERROR-level log message appears containing:
        *   The .bgaj file path
        *   The JSON parse error message
        *   The line/column of the error (if available)

*   **Scenario 3: Log messages are searchable**
    *   **Given** an error has occurred
    *   **When** searching the log output
    *   **Then** the error message contains the full file path (not just filename)
    *   **And** the error reason is a human-readable string

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points (already completed)
*   **Dependencies**: None
*   **Implementation Note**: spdlog already integrated

---

### Story ID: US-ENG-062 - Graceful Degradation for Missing Non-Critical Assets

**Story Card:**
> **As a** Player
> **I want** to play a song even if its BGA is missing
> **So that** I'm not blocked from playing due to incomplete asset packs

**References**: REQ-ENG-007

**Status**: DONE

### 📝 Description
Allow gameplay to continue when non-critical assets (BGAs, banners) are missing, while preventing gameplay for missing critical assets (chart, audio).

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Missing BGA allows gameplay**
    *   **Given** a song directory with a chart and audio but no BGA files
    *   **When** attempting to play the song
    *   **Then** an ERROR is logged about the missing BGA
    *   **And** gameplay proceeds with a black background
    *   **And** notes and audio play normally

*   **Scenario 2: Missing chart prevents song from appearing**
    *   **Given** a song directory with audio but no chart file
    *   **When** the song list is built
    *   **Then** an ERROR is logged about the missing chart
    *   **And** the song does not appear in the song selection screen

*   **Scenario 3: Missing audio prevents song from appearing**
    *   **Given** a song directory with a chart but no audio file
    *   **When** the song list is built
    *   **Then** an ERROR is logged about the missing audio
    *   **And** the song does not appear in the song selection screen

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points (already completed)
*   **Dependencies**: REQ-AST-001 (asset management)
*   **Implementation Note**: Already implemented in texture cache and loaders

---

### Story ID: US-ENG-063a - Catch and Log Engine Loop Exceptions

**Story Card:**
> **As a** Player
> **I want** the engine to log exceptions during update or render and continue running
> **So that** a single scene error doesn't crash my entire game session

**References**: REQ-ENG-007

**Status**: PLANNED

### 📝 Description
Implement top-level exception handling in the engine loop (update and render phases) that catches unexpected errors, logs them, and attempts to recover gracefully rather than crashing.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Exception during scene update**
    *   **Given** a scene's update() method throws an exception
    *   **When** the exception propagates to the engine loop
    *   **Then** the exception is caught and logged at ERROR level
    *   **And** the engine continues running

*   **Scenario 2: Exception during scene render**
    *   **Given** a scene's render() method throws an exception
    *   **When** the exception propagates to the engine loop
    *   **Then** the exception is caught and logged at ERROR level
    *   **And** the engine continues running

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-ENG-011 (Engine class)
*   **Implementation Note**: Top-level try-catch in main loop update/render. Scene recovery (pop failing scene → return to menu) deferred to Phase 2.

---

### Story ID: US-ENG-063b - Graceful Startup Failure

**Story Card:**
> **As a** Player
> **I want** clear error messages when the engine fails to start
> **So that** I know what went wrong and can fix it

**References**: REQ-ENG-007

**Status**: PLANNED

### 📝 Description
Handle critical startup failures (SDL initialization, window creation) by logging the error and exiting cleanly with a non-zero exit code.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: SDL initialization failure**
    *   **Given** SDL initialization fails completely
    *   **When** the engine starts
    *   **Then** the error is logged at ERROR level
    *   **And** the engine exits cleanly with a non-zero exit code
    *   **And** an error message is displayed to the user

*   **Scenario 2: Window creation failure**
    *   **Given** SDL window creation fails (e.g., no display available)
    *   **When** the engine starts
    *   **Then** the error is logged at ERROR level
    *   **And** the engine exits cleanly with a non-zero exit code
    *   **And** an error message is displayed to the user

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1 story point
*   **Dependencies**: None
*   **Implementation Note**: Early-exit checks in Engine constructor or main()

---

## Non-Functional Requirements

These are system-wide quality attributes verified through integration testing in Phase 3.

### NFR: Performance Targets (REQ-ENG-008)

**As a** Player with mid-range hardware from 2018
**I want** consistent 60 FPS during gameplay
**So that** the game is responsive and playable

**Target Hardware**: Intel i5-8400 or AMD Ryzen 5 2600 equivalent

**Per-Subsystem Budgets**:
- Logic update: <2 ms per tick
- Note rendering: <4 ms per frame
- BGA rendering: <4 ms per frame
- Audio mixing: <1 ms per tick
- Total frame time: <16.67 ms (60 FPS)

**Acceptance Criteria**:
*   **Scenario 1: Stable frame rate in normal gameplay**
    *   **Given** a song with typical note density (50-70 notes on screen)
    *   **When** playing on target hardware
    *   **Then** frame rate remains at or above 60 FPS for 99% of frames
    *   **And** no frame drops below 55 FPS occur

*   **Scenario 2: Performance in note-dense sections**
    *   **Given** a song section with 100+ notes on screen simultaneously
    *   **When** playing on target hardware
    *   **Then** frame rate remains at or above 60 FPS
    *   **And** no visible stutter occurs

*   **Scenario 3: CPU usage remains efficient**
    *   **Given** typical gameplay on target hardware
    *   **When** measuring CPU usage
    *   **Then** CPU usage is under 30% on average
    *   **And** the game does not peg a single core at 100%

**Testing Note**: Integration test in Phase 3 with profiling on target hardware spec.

---

### NFR: Memory Constraints (REQ-ENG-008)

**As a** Player with 8 GB system RAM
**I want** the engine to use less than 512 MB during gameplay
**So that** I can run the game alongside other applications

**Per-Subsystem Budgets**:
- Texture cache: configurable max (default 256 MB)
- Audio buffers: <36 MB (stereo, 44.1 kHz, 5-song buffer)
- Chart data: <1 MB per loaded chart
- Total gameplay: <512 MB

**Acceptance Criteria**:
*   **Scenario 1: Gameplay memory usage**
    *   **Given** a song is actively playing with BGA, notes, and audio
    *   **When** measuring resident memory usage
    *   **Then** memory usage is under 512 MB
    *   **And** no memory leaks are detected over 5 consecutive songs

*   **Scenario 2: Asset loading does not spike memory**
    *   **Given** a song with large BGA textures is loading
    *   **When** measuring peak memory usage during load
    *   **Then** peak memory usage remains under 700 MB
    *   **And** memory is released after loading completes

*   **Scenario 3: Multiple songs can be played sequentially**
    *   **Given** the player completes one song and selects another
    *   **When** monitoring memory usage across 5 consecutive songs
    *   **Then** memory usage does not grow unbounded
    *   **And** previous song assets are properly unloaded

**Testing Note**: Integration test in Phase 3. Use Valgrind or similar memory profiler.

---

## Story Summary by Status

### DONE (6 stories)
- US-ENG-031: Verify Linux cross-distribution compatibility
- US-ENG-032: Verify Windows platform compatibility
- US-ENG-041: Configure CMake FetchContent for all dependencies
- US-ENG-042: Optimize incremental build performance
- US-ENG-061: Log asset loading errors with context
- US-ENG-062: Graceful degradation for missing non-critical assets

### PLANNED (8 stories)
- US-ENG-001: Implement time accumulator for fixed logic step (merged drift prevention and spiral-of-death guard)
- US-ENG-003: Support uncapped and variable refresh rate rendering
- US-ENG-004: Render state interpolation (NEW - covers accumulator remainder usage)
- US-ENG-011: Create Engine class as subsystem owner
- US-ENG-012: Integrate Renderer into Engine ownership
- US-ENG-021: Create Clock utility wrapping SDL performance counter (merged long-session precision)
- US-ENG-063a: Catch and log engine loop exceptions
- US-ENG-063b: Graceful startup failure

---

## Story Point Summary

| Status | Count | Total Points |
|--------|-------|--------------|
| DONE | 6 | 22 |
| PLANNED | 8 | 21 |
| **Total** | **14** | **43** |

---

## Dependency Graph

```
US-ENG-021 (Clock) ──> US-ENG-001 (Time Accumulator) ─┬─> US-ENG-003 (Variable Refresh)
                                                       └─> US-ENG-004 (Interpolation)

US-ENG-003 + US-ENG-001 ──> US-ENG-004 (Interpolation)

US-ENG-011 (Engine Class) ─┬─> US-ENG-012 (Renderer Integration)
                           └─> US-ENG-063a (Engine Loop Exceptions)

US-ENG-041 (CMake) ──> US-ENG-042 (Build Performance)
US-ENG-061 (Logging) ──> US-ENG-062 (Graceful Degradation)
```

---

## Notes

**Judge determinism stories** (formerly US-ENG-051, US-ENG-052) moved to gameplay judge story set. Frame-rate independence and RNG elimination are judge-specific concerns rather than core engine concerns.

**Audio/Input/Scene-stack integration**: These subsystems will be integrated into Engine ownership in their respective story sets (REQ-AUD, REQ-INP, REQ-SCN) as noted in US-ENG-011.

---

*Generated from docs/requirements/01-core-engine.md*
*Last updated: 2026-04-26*
