# Core Engine Requirements

## REQ-ENG-001: Fixed-Step Game Loop
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must implement a fixed 60 Hz logic tick rate decoupled from rendering frame rate. A time accumulator drives fixed-step updates where each frame accumulates wall-clock time and steps logic in 1/60th-second increments.

**Acceptance Criteria**:
- Game logic executes at exactly 60 ticks per second regardless of display refresh rate
- Rendering runs at display refresh rate (60, 120, 144 Hz) or uncapped
- Judge timing is deterministic across all display configurations
- No drift between logic and rendering over extended play sessions

**Dependencies**: REQ-REN-001  
**Source**: Roadmap subsystem 1, Phase 1

---

## REQ-ENG-002: Engine Class Architecture
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must provide an `Engine` class that serves as the root of the object graph, owning the renderer, scene stack, audio system, and input system.

**Acceptance Criteria**:
- Single `Engine` instance coordinates all subsystems
- Clear ownership hierarchy (Engine owns subsystems, not vice versa)
- Initialization and shutdown sequences properly ordered
- Subsystems can access each other through Engine reference

**Dependencies**: None  
**Source**: Roadmap subsystem 1, Phase 1

---

## REQ-ENG-003: High-Precision Timing Utility
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must provide a `Clock` utility wrapping SDL_GetPerformanceCounter to provide delta time, fixed step tracking, and elapsed time accessors.

**Acceptance Criteria**:
- Delta time accuracy within 1 microsecond
- Fixed step accumulator correctly handles multiple updates per frame
- No floating-point drift over extended sessions (hours)
- Works consistently across Linux and Windows

**Dependencies**: None  
**Source**: Roadmap subsystem 1, Phase 1

---

## REQ-ENG-004: Cross-Platform Support
**Status**: [DONE]  
**Priority**: Must Have

The engine must run on both Linux and Windows desktop platforms. Linux is the primary development target.

**Acceptance Criteria**:
- Compiles without modification on Linux (Ubuntu/Debian/Arch)
- Compiles without modification on Windows (MSVC/MinGW)
- All tests pass on both platforms
- No platform-specific code outside designated abstraction layers

**Dependencies**: None  
**Source**: CLAUDE.md, Roadmap scope notes

---

## REQ-ENG-005: CMake Build System with FetchContent
**Status**: [DONE]  
**Priority**: Must Have

The engine must use CMake with FetchContent to automatically fetch all dependencies (SDL3, SDL3_image, nlohmann/json, spdlog, CLI11, GoogleTest).

**Acceptance Criteria**:
- Build succeeds on clean machine with only CMake and compiler installed
- No manual dependency installation required
- Build completes in under 10 minutes on modern hardware (first build)
- Incremental builds under 30 seconds for typical changes

**Dependencies**: None  
**Source**: CLAUDE.md, Roadmap current state

---

## REQ-ENG-006: Deterministic Behavior
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must produce deterministic gameplay results given identical input sequences and chart data, regardless of hardware or frame rate.

**Acceptance Criteria**:
- Same input sequence produces same judgment results across runs
- Judge timing independent of display refresh rate
- Replay functionality produces identical results (when implemented)
- No random number generators in judge or timing code

**Dependencies**: REQ-ENG-001, REQ-JDG-001  
**Source**: Roadmap architecture decisions

---

## REQ-ENG-007: Error Handling and Logging
**Status**: [DONE]  
**Priority**: Must Have

The engine must log errors via spdlog and handle asset loading failures gracefully. Missing critical assets are fatal for that song; missing non-critical assets (BGA) allow gameplay to continue.

**Acceptance Criteria**:
- All errors logged at ERROR level with context (file path, reason)
- Missing chart or audio prevents song from appearing in song list
- Missing BGA logs error but allows gameplay to proceed
- Uncaught exceptions do not crash engine; logged and handled

**Dependencies**: None  
**Source**: Roadmap scope notes, CLAUDE.md

---

## REQ-ENG-008: Performance Target
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must maintain 60 FPS during gameplay on mid-range hardware from 2018 or newer.

**Acceptance Criteria**:
- Consistent 60 FPS on Intel i5-8400 / Ryzen 5 2600 equivalent
- No frame drops during note-dense sections (100+ notes on screen)
- Memory usage under 512 MB during gameplay
- CPU usage under 30% on target hardware

**Dependencies**: REQ-ENG-001, REQ-REN-002  
**Source**: Roadmap scope notes (implied)
