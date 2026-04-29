# Input System Requirements

## REQ-INP-001: Input Abstraction Layer
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The input system must abstract all physical input devices behind a common `PadInput` enum representing the 10 PIU dance panels (5 per player) plus menu actions (start, back, select, coin).

**Acceptance Criteria**:
- Single enum covers all gameplay and menu inputs
- No gameplay code accesses SDL events directly
- Backend can be swapped without changing gameplay code
- Input snapshots immutable once created

**Dependencies**: REQ-ENG-002  
**Source**: Roadmap subsystem 2

---

## REQ-INP-002: Input Snapshot Per Tick
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

Each tick must produce an `InputSnapshot` containing a bitmask of currently held panels and edge events (pressed-this-frame, released-this-frame).

**Acceptance Criteria**:
- Snapshot captured exactly once per 60 Hz tick
- Edge events correctly identify press/release on frame boundary
- Held state persists correctly across frames
- No input events lost or duplicated

**Dependencies**: REQ-INP-001, REQ-ENG-001  
**Source**: Roadmap subsystem 2

---

## REQ-INP-003: Keyboard Input Driver
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must support keyboard input via a `KeyboardDriver` mapping SDL key events to PadInput through a configurable keymap.

**Acceptance Criteria**:
- All 10 panels plus menu actions mappable to keyboard keys
- Default keymap provided for QWEASDZXC layout
- Phase 1 uses a hardcoded default keymap
- Configurable keymap persistence deferred to Phase 3 when settings file exists
- Simultaneous key presses registered correctly (10+ key rollover)

**Dependencies**: REQ-INP-001  
**Source**: Roadmap subsystem 2, Phase 1

---

## REQ-INP-004: USB Dance Pad Driver
**Status**: [PLANNED Phase 6]  
**Priority**: Must Have

The engine must support USB dance pads via `HidPadDriver` using SDL3's gamepad API.

**Acceptance Criteria**:
- Plug-and-play detection of USB dance pads
- Configurable device mapping per USB VID/PID
- Axis threshold configuration for analog sensors
- Button to panel mapping configurable

**Dependencies**: REQ-INP-001  
**Source**: Roadmap subsystem 2, Phase 6

---

## REQ-INP-005: Arcade I/O Board Driver
**Status**: [PLANNED Phase 8]  
**Priority**: Should Have

The engine must support PIUIO arcade I/O boards via `ArcadeIODriver` using raw USB bulk transfer protocol.

**Acceptance Criteria**:
- Detects PIUIO boards on USB bus
- Reads sensor states at 1000 Hz or higher
- Correctly interprets 10 sensors (5 per player)
- Compatible with PIUIO v1 and v2 hardware

**Dependencies**: REQ-INP-001  
**Source**: Roadmap subsystem 2, Phase 8

---

## REQ-INP-006: Multi-Player Input Separation
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

For co-op and battle modes, the input system must produce separate `InputSnapshot` instances per player.

**Acceptance Criteria**:
- Keyboard driver binds keys to P1 or P2
- HID driver binds devices to P1 or P2
- Arcade I/O driver natively separates cabinet sides
- Both snapshots available same tick

**Dependencies**: REQ-INP-002, REQ-JDG-010  
**Source**: Roadmap subsystem 2, Phase 5

---

## REQ-INP-007: Input Polling Timing
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

Input polling must occur once per fixed-step update, before the active scene's `update()` call.

**Acceptance Criteria**:
- Poll happens exactly once per 60 Hz tick
- Poll completes before judge processes notes
- Poll order deterministic relative to audio position query
- No input lag introduced by polling mechanism

**Dependencies**: REQ-ENG-001, REQ-INP-002  
**Source**: Roadmap subsystem 2

---

## REQ-INP-008: Input Configuration Screen
**Status**: [PLANNED Phase 6]  
**Priority**: Should Have

The engine must provide a screen for configuring input device mappings where users press each panel to assign it.

**Acceptance Criteria**:
- Shows all 10 panels in visual layout
- Press panel to enter mapping mode for that panel
- Next input received is assigned to that panel
- Changes saved immediately to settings file

**Dependencies**: REQ-INP-003, REQ-INP-004, REQ-SCN-002  
**Source**: Roadmap Phase 6

---

## REQ-INP-009: Per-Device Calibration
**Status**: [PLANNED Phase 6]  
**Priority**: Should Have

The engine must support per-device input calibration offsets to compensate for device-specific latency.

**Acceptance Criteria**:
- Offset stored per device VID/PID or identifier
- Offset applied before judge sees input
- Offset range -200ms to +200ms in 1ms increments
- Calibration screen provides feedback on adjusted timing

**Dependencies**: REQ-INP-004, REQ-JDG-001  
**Source**: Roadmap Phase 6

---

## REQ-INP-010: Simultaneous Device Support
**Status**: [PLANNED Phase 6]  
**Priority**: Should Have

The engine must support keyboard and dance pad input simultaneously (useful for navigating menus with keyboard while playing on pad).

**Acceptance Criteria**:
- Multiple drivers active at once
- Input snapshots merged from all active drivers
- No conflicts when same panel triggered from multiple devices
- Driver priority configurable (first-wins or OR-merge)

**Dependencies**: REQ-INP-001  
**Source**: Roadmap Phase 6

---

## REQ-INP-011: Arcade Cabinet Lamp Output
**Status**: [PLANNED Phase 8]  
**Priority**: Could Have

When using arcade I/O hardware, the engine must control panel lighting during gameplay.

**Acceptance Criteria**:
- Lamps illuminate when note passes receptor line
- Lamp duration configurable (0-500ms)
- Lamp brightness controllable if hardware supports
- No lamp control interferes with input polling

**Dependencies**: REQ-INP-005  
**Source**: Roadmap Phase 8

---

## REQ-INP-012: Input Latency Minimization
**Status**: [FUTURE]  
**Priority**: Could Have

The engine should minimize input latency from physical press to judge evaluation.

**Acceptance Criteria**:
- Total latency under 10ms on target hardware
- Latency measurement tooling available for testing
- Polling directly from USB raw HID when possible
- SDL event queue bypassed for critical input paths

**Dependencies**: REQ-INP-001  
**Source**: Roadmap architecture goals (implied)
