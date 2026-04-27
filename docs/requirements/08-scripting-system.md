# Scripting System Requirements

## REQ-SCR-001: Lua Integration via sol2
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

The engine must integrate Lua scripting using sol2 header-only binding library fetched via CMake FetchContent.

**Acceptance Criteria**:
- sol2 integrated into build system
- Lua state initialized at engine startup
- No manual Lua C API calls in engine code
- Lua errors caught and logged without crashing engine

**Dependencies**: REQ-ENG-002  
**Source**: Roadmap subsystem 8, Phase 5

---

## REQ-SCR-002: C++ API Bindings for Lua
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

The engine must expose stable C++ API to Lua including input queries, audio control, sprite/BGA rendering, scene stack navigation, and profile access.

**Acceptance Criteria**:
- Input: query current panel state, check if pressed this frame
- Audio: play/pause/seek music, play SFX, query position
- Rendering: draw sprite, draw BGA, draw text, draw primitive shapes
- Scene: push scene, pop scene, replace scene, get scene parameters
- Profile: get high scores, get settings, save data
- Timer: get current time, create delay timers

**Dependencies**: REQ-SCR-001  
**Source**: Roadmap subsystem 8, Phase 5

---

## REQ-SCR-003: Lua Cannot Bypass Core Systems
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

Lua scripts must not be able to bypass judge, audio sync, or other performance-critical systems. These remain C++-only.

**Acceptance Criteria**:
- No Lua bindings for judge timing calculation
- No Lua bindings for direct audio buffer manipulation
- No Lua bindings for note renderer frame logic
- Judge, audio, and renderer operate independently of Lua execution time

**Dependencies**: REQ-SCR-002  
**Source**: Roadmap subsystem 8, architecture decisions

---

## REQ-SCR-004: Game Definition Directory Structure
**Status**: [PLANNED Phase 7]  
**Priority**: Must Have

Each Pump It Up game version must be defined as a Lua "game" directory containing scripts for each screen plus asset references and judge profile.

**Acceptance Criteria**:
- Game directory contains manifest.lua defining metadata
- Separate Lua files per screen (boot.lua, title.lua, select.lua, etc)
- References to judge profile JSON file
- References to asset directories (sprites, BGAs, audio)
- One game definition loaded at startup

**Dependencies**: REQ-SCR-001  
**Source**: Roadmap subsystem 8, Phase 7

---

## REQ-SCR-005: All Screen Logic in Lua
**Status**: [PLANNED Phase 7]  
**Priority**: Must Have

All screen flow and UI logic must be implemented in Lua, not C++, enabling different game versions without engine recompilation.

**Acceptance Criteria**:
- Boot, title, mode select, song select, result, name entry screens in Lua
- Scene lifecycle methods (on_enter, update, render, etc) implemented in Lua
- Input handling routed through Lua for screen logic
- No screen-specific logic remains in C++ engine code

**Dependencies**: REQ-SCR-004, REQ-SCN-002  
**Source**: Roadmap subsystem 8, Phase 7

---

## REQ-SCR-006: Multi-Version Game Support
**Status**: [PLANNED Phase 7]  
**Priority**: Must Have

The engine must support multiple game versions (Exceed, NX, Phoenix) as selectable Lua game packages on the same engine.

**Acceptance Criteria**:
- At least two complete game definitions implemented
- Game selection at startup or from menu
- Switching games reloads Lua scripts and assets
- No cross-contamination between game definitions

**Dependencies**: REQ-SCR-004, REQ-SCR-005  
**Source**: Roadmap subsystem 8, Phase 7

---

## REQ-SCR-007: Lua Performance Constraints
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

Lua execution must not cause frame drops or introduce input latency. Per-frame Lua execution budget enforced.

**Acceptance Criteria**:
- Lua update() completes in under 5ms per frame
- Lua render() completes in under 5ms per frame
- Timeout kills long-running Lua scripts with error log
- Heavy computation moves to background threads or C++

**Dependencies**: REQ-SCR-001, REQ-ENG-001  
**Source**: Roadmap architecture decisions

---

## REQ-SCR-008: Lua Script Hot Reloading
**Status**: [PLANNED Phase 7]  
**Priority**: Should Have

The engine should support hot reloading of Lua scripts during development without restarting.

**Acceptance Criteria**:
- File watcher detects Lua script changes
- Modified scripts reloaded automatically
- Scene state preserved or gracefully reinitialized
- Syntax errors logged without crashing
- Feature disable-able for production builds

**Dependencies**: REQ-SCR-005  
**Source**: Roadmap subsystem 8 (implied for development workflow)

---

## REQ-SCR-009: Lua Error Reporting
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

Lua runtime errors must be caught, logged with stack traces, and reported to user without crashing engine.

**Acceptance Criteria**:
- Stack traces show Lua file and line numbers
- Error messages displayed in-engine (dev mode)
- Errors logged to file with timestamp
- Engine continues running after Lua error (fallback to safe state)

**Dependencies**: REQ-SCR-001  
**Source**: Roadmap subsystem 8 (implied)

---

## REQ-SCR-010: Lua Sandbox Security
**Status**: [PLANNED Phase 7]  
**Priority**: Should Have

Lua scripts should run in a sandboxed environment preventing access to filesystem, network, or OS calls.

**Acceptance Criteria**:
- Lua io and os libraries disabled or sandboxed
- File access only through engine-provided API
- No arbitrary code execution outside Lua VM
- Network access only through engine-provided API (if needed)

**Dependencies**: REQ-SCR-001  
**Source**: Roadmap subsystem 8 (security best practice)
