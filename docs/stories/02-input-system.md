# Input System User Stories

This document contains vertically-sliced user stories for the input system requirements (REQ-INP-001 through REQ-INP-012). Stories are organized by requirement and marked with implementation status.

---

## Epic: Input Abstraction

### Story ID: US-INP-001 - Define PadInput Enum for All Game Controls

**Story Card:**
> **As a** Developer
> **I want** a single enum representing all gameplay and menu inputs
> **So that** gameplay code is independent of physical input devices

**References**: REQ-INP-001, Roadmap Phase 1

**Status**: PLANNED (Phase 1)

### 📝 Description
Create a `PadInput` enum covering the 10 PIU dance panels (5 per player: down-left, up-left, center, up-right, down-right) plus menu actions (start, back, select, coin). This is the common vocabulary between all input drivers and gameplay code.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: All gameplay inputs represented**
    *   **Given** the PadInput enum definition
    *   **When** enumerating all values
    *   **Then** the following values exist: P1_DOWN_LEFT, P1_UP_LEFT, P1_CENTER, P1_UP_RIGHT, P1_DOWN_RIGHT, P2_DOWN_LEFT, P2_UP_LEFT, P2_CENTER, P2_UP_RIGHT, P2_DOWN_RIGHT

*   **Scenario 2: All menu inputs represented**
    *   **Given** the PadInput enum definition
    *   **When** enumerating menu-specific values
    *   **Then** the following values exist: START, BACK, SELECT, COIN

*   **Scenario 3: Enum is bitwise-compatible**
    *   **Given** the PadInput enum values
    *   **When** checking the value assignments
    *   **Then** each value is a unique power of two (1, 2, 4, 8, 16, etc.)
    *   **And** multiple inputs can be combined using bitwise OR

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1 story point
*   **Dependencies**: None
*   **Implementation Note**: Use uint32_t underlying type for bitmask operations

---

### Story ID: US-INP-002 - Create InputSnapshot Structure

**Story Card:**
> **As a** Developer
> **I want** an immutable snapshot of input state per tick
> **So that** all subsystems see consistent input state within a single frame

**References**: REQ-INP-001, REQ-INP-002, Roadmap Phase 1

**Status**: PLANNED (Phase 1)

### 📝 Description
Define an `InputSnapshot` structure containing the current held state (bitmask), pressed-this-frame events, and released-this-frame events. Once created, the snapshot is immutable.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Snapshot contains held state**
    *   **Given** an InputSnapshot is created with P1_CENTER held
    *   **When** querying `is_held(PadInput::P1_CENTER)`
    *   **Then** the method returns true
    *   **And** querying other inputs returns false

*   **Scenario 2: Snapshot contains press events**
    *   **Given** P1_CENTER was pressed this frame
    *   **When** querying `is_pressed(PadInput::P1_CENTER)`
    *   **Then** the method returns true
    *   **And** querying other inputs returns false

*   **Scenario 3: Snapshot contains release events**
    *   **Given** P1_CENTER was released this frame
    *   **When** querying `is_released(PadInput::P1_CENTER)`
    *   **Then** the method returns true
    *   **And** querying other inputs returns false

*   **Scenario 4: Snapshot is immutable**
    *   **Given** an InputSnapshot is created
    *   **When** attempting to modify its internal state
    *   **Then** compilation fails (all fields are const or private with no setters)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-INP-001
*   **Implementation Note**: Three bitmasks: held, pressed, released. Add tick timestamp for debug logging.

---

### Story ID: US-INP-003 - Define InputDriver Interface

**Story Card:**
> **As a** Developer
> **I want** a common interface for all input drivers
> **So that** backends can be swapped without changing gameplay code

**References**: REQ-INP-001, Roadmap Phase 1

**Status**: PLANNED (Phase 1)

### 📝 Description
Create an abstract `InputDriver` interface with methods for polling input state and producing an `InputSnapshot` per tick. All backends (keyboard, HID pad, arcade I/O) implement this interface.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Interface defines snapshot production**
    *   **Given** the InputDriver interface definition
    *   **When** reviewing the interface methods
    *   **Then** a `poll()` method exists that returns an InputSnapshot
    *   **And** the method has no side effects on game state

*   **Scenario 2: Interface supports multi-player**
    *   **Given** the InputDriver interface definition
    *   **When** reviewing the interface methods
    *   **Then** `poll()` accepts a player index parameter
    *   **And** the method can return snapshots for P1 or P2 independently

*   **Scenario 3: Interface supports device enumeration**
    *   **Given** the InputDriver interface definition
    *   **When** reviewing the interface methods
    *   **Then** a `get_device_name()` method exists returning a human-readable string

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-INP-002
*   **Implementation Note**: Pure virtual interface. Player index defaults to 0 (single-player).

---

## Epic: Input Snapshot Processing

### Story ID: US-INP-011 - Capture Input Snapshot Once Per Tick

**Story Card:**
> **As a** Developer
> **I want** input captured exactly once per 60 Hz tick
> **So that** all subsystems see the same input state and no events are lost or duplicated

**References**: REQ-INP-002, REQ-INP-007, Roadmap Phase 1

**Status**: PLANNED (Phase 1)

### 📝 Description
Integrate input polling into the fixed-step engine loop. Polling occurs once per tick, immediately before the active scene's update call, ensuring the snapshot represents the state at the beginning of that tick.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Poll occurs once per tick**
    *   **Given** the engine runs for 120 ticks
    *   **When** counting the number of poll calls
    *   **Then** exactly 120 snapshots are produced

*   **Scenario 2: Poll occurs before scene update**
    *   **Given** an injectable mock scene tracking call order
    *   **When** a single tick executes
    *   **Then** the input poll completes before the scene's update method is called
    *   **And** the snapshot is available to the scene

*   **Scenario 3: Poll occurs before judge processes notes**
    *   **Given** gameplay is active
    *   **When** a single tick executes
    *   **Then** the input snapshot is captured before the judge's update method runs
    *   **And** the judge receives the snapshot for timing evaluation

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-INP-003, US-ENG-001 (fixed-step loop)
*   **Implementation Note**: Call `input_driver->poll()` at the start of each fixed-step update

---

### Story ID: US-INP-012 - Detect Press and Release Edge Events

**Story Card:**
> **As a** Developer
> **I want** press and release events correctly identified on frame boundaries
> **So that** the judge can accurately measure timing for notes hit exactly on tick boundaries

**References**: REQ-INP-002, Roadmap Phase 1

**Status**: PLANNED (Phase 1)

### 📝 Description
Implement edge detection logic that compares the current raw input state to the previous tick's state to identify new presses (0→1 transitions) and new releases (1→0 transitions).

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Initial press detected**
    *   **Given** P1_CENTER was not held last tick
    *   **When** P1_CENTER is held this tick
    *   **Then** `is_pressed(P1_CENTER)` returns true
    *   **And** `is_held(P1_CENTER)` returns true
    *   **And** `is_released(P1_CENTER)` returns false

*   **Scenario 2: Continued hold does not trigger press**
    *   **Given** P1_CENTER was held last tick
    *   **When** P1_CENTER remains held this tick
    *   **Then** `is_pressed(P1_CENTER)` returns false
    *   **And** `is_held(P1_CENTER)` returns true

*   **Scenario 3: Release detected**
    *   **Given** P1_CENTER was held last tick
    *   **When** P1_CENTER is not held this tick
    *   **Then** `is_released(P1_CENTER)` returns true
    *   **And** `is_held(P1_CENTER)` returns false
    *   **And** `is_pressed(P1_CENTER)` returns false

*   **Scenario 4: Same-frame press and release**
    *   **Given** P1_CENTER was not held last tick
    *   **When** P1_CENTER is pressed and released within a single polling interval
    *   **Then** both `is_pressed(P1_CENTER)` and `is_released(P1_CENTER)` return true for that tick
    *   **And** `is_held(P1_CENTER)` returns false

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-INP-002, US-INP-011
*   **Implementation Note**: Store previous tick's bitmask. Compare to current to compute edges.

---

### Story ID: US-INP-013 - Verify No Input Events Lost

**Story Card:**
> **As a** Player
> **I want** every panel press I make to be registered
> **So that** I don't miss judgments due to dropped inputs

**References**: REQ-INP-002, Roadmap Phase 1

**Status**: PLANNED (Phase 1)

### 📝 Description
Implement a test harness that injects known input sequences and verifies all events appear in the correct snapshots. This ensures the polling mechanism doesn't drop events between SDL and the snapshot.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Rapid alternating presses registered**
    *   **Given** an injectable input source alternating P1_CENTER on/off each tick for 120 ticks
    *   **When** collecting all snapshots
    *   **Then** exactly 60 press events and 60 release events are recorded
    *   **And** press and release events alternate correctly

*   **Scenario 2: Simultaneous multi-panel presses registered**
    *   **Given** all 5 P1 panels are pressed simultaneously
    *   **When** capturing the snapshot
    *   **Then** `is_pressed()` returns true for all 5 panels
    *   **And** `is_held()` returns true for all 5 panels

*   **Scenario 3: 10-panel rollover supported**
    *   **Given** all 10 panels (both players) are pressed simultaneously
    *   **When** capturing the snapshot
    *   **Then** `is_pressed()` returns true for all 10 panels
    *   **And** no events are lost

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-INP-012
*   **Implementation Note**: Injectable input source for testing. Real hardware verification in Phase 6.

---

## Epic: Keyboard Input

### Story ID: US-INP-021 - Implement KeyboardDriver with Configurable Keymap

**Story Card:**
> **As a** Player using a keyboard
> **I want** to map each panel to a keyboard key
> **So that** I can play without a physical dance pad

**References**: REQ-INP-003, Roadmap Phase 1

**Status**: PLANNED (Phase 1)

### 📝 Description
Implement a `KeyboardDriver` that consumes SDL keyboard events, looks up each key in a configurable keymap, and produces an `InputSnapshot` with the mapped PadInput values.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Keymap translates key to panel**
    *   **Given** the keymap maps SDL_SCANCODE_Q to P1_DOWN_LEFT
    *   **When** the Q key is pressed
    *   **Then** the snapshot contains `is_pressed(P1_DOWN_LEFT) == true`

*   **Scenario 2: Unmapped keys ignored**
    *   **Given** the keymap does not contain SDL_SCANCODE_TAB
    *   **When** the Tab key is pressed
    *   **Then** the snapshot contains no pressed events
    *   **And** no error or warning is logged

*   **Scenario 3: Multiple keys mapped to different panels**
    *   **Given** Q maps to P1_DOWN_LEFT and W maps to P1_UP_LEFT
    *   **When** both Q and W are pressed simultaneously
    *   **Then** the snapshot contains both `is_pressed(P1_DOWN_LEFT)` and `is_pressed(P1_UP_LEFT)` as true

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points
*   **Dependencies**: US-INP-003, US-INP-012
*   **Implementation Note**: SDL_GetKeyboardState for polling. Keymap is std::unordered_map<SDL_Scancode, PadInput>.

---

### Story ID: US-INP-022 - Provide Default QWEASDZXC Keymap

**Story Card:**
> **As a** Player launching the engine for the first time
> **I want** a working default keyboard layout
> **So that** I can play immediately without configuring controls

**References**: REQ-INP-003, Roadmap Phase 1

**Status**: PLANNED (Phase 1)

### 📝 Description
Define a default keymap matching the common QWEASDZXC layout for single-player mode (P1 only). Menu actions are bound to arrow keys and Enter.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Default layout for P1 panels**
    *   **Given** no user keymap exists
    *   **When** the KeyboardDriver initializes
    *   **Then** the following mappings are active:
        *   Q → P1_DOWN_LEFT
        *   W → P1_UP_LEFT
        *   E → P1_CENTER
        *   A → P1_UP_RIGHT
        *   S → P1_DOWN_RIGHT

*   **Scenario 2: Default layout for menu actions**
    *   **Given** no user keymap exists
    *   **When** the KeyboardDriver initializes
    *   **Then** the following mappings are active:
        *   Enter → START
        *   Escape → BACK
        *   Space → SELECT

*   **Scenario 3: Default layout supports gameplay**
    *   **Given** the default keymap is loaded
    *   **When** a test song is played using the default keys
    *   **Then** all panels register correctly
    *   **And** the player can complete the song

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1 story point
*   **Dependencies**: US-INP-021
*   **Implementation Note**: Hardcoded default map in KeyboardDriver constructor

---

### Story ID: US-INP-023 - Persist Keymap to Settings File

**Story Card:**
> **As a** Player
> **I want** my custom key mappings to persist between sessions
> **So that** I don't need to reconfigure controls every time I launch the game

**References**: REQ-INP-003, Roadmap Phase 3

**Status**: PLANNED (Phase 3)

### 📝 Description
Save and load keymap configuration to the settings.json file. The keymap is serialized as a JSON object mapping SDL scancode names to PadInput enum names.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Keymap saved on change**
    *   **Given** the player remaps Q to P1_CENTER
    *   **When** the remapping is confirmed
    *   **Then** settings.json is updated with the new mapping
    *   **And** the file is written to disk immediately

*   **Scenario 2: Keymap loaded on startup**
    *   **Given** settings.json contains custom key mappings
    *   **When** the engine starts
    *   **Then** the KeyboardDriver loads the custom mappings
    *   **And** the custom mappings override the defaults

*   **Scenario 3: Invalid keymap falls back to default**
    *   **Given** settings.json contains an unparseable keymap section
    *   **When** the engine starts
    *   **Then** a warning is logged
    *   **And** the default keymap is used
    *   **And** the engine continues running

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-INP-022, REQ-DAT-002 (settings file)
*   **Phase**: 3 (Phase 1 uses hardcoded keymap only)
*   **Implementation Note**: JSON format: `{"keyboard_map": {"SDL_SCANCODE_Q": "P1_DOWN_LEFT"}}`

---

### Story ID: US-INP-024 - Support 10+ Key Rollover

**Story Card:**
> **As a** Player using a mechanical keyboard
> **I want** all 10 panels plus menu keys to register simultaneously
> **So that** complex double-play patterns don't ghost or block

**References**: REQ-INP-003, Roadmap Phase 1

**Status**: PLANNED (Phase 1)

### 📝 Description
Use SDL_GetKeyboardState for polling rather than event-based input to avoid keyboard rollover limitations in SDL's event queue.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: All 10 panels register simultaneously**
    *   **Given** a keyboard with N-key rollover support
    *   **When** all 10 panel keys are pressed at once
    *   **Then** the snapshot contains all 10 panels as pressed
    *   **And** no inputs are lost

*   **Scenario 2: Panel plus menu key registers**
    *   **Given** P1_CENTER and START are pressed simultaneously
    *   **When** capturing the snapshot
    *   **Then** both inputs are registered
    *   **And** no ghosting occurs

*   **Scenario 3: Polling handles key state correctly**
    *   **Given** the engine is using SDL_GetKeyboardState
    *   **When** measuring input latency
    *   **Then** latency is under 3 milliseconds
    *   **And** no polling overhead affects frame time

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-INP-021
*   **Implementation Note**: SDL_GetKeyboardState returns pointer to state array. Copy state each poll.

---

## Epic: USB Dance Pad Support

### Story ID: US-INP-031 - Implement HidPadDriver Using SDL3 Gamepad API

**Story Card:**
> **As a** Player with a USB dance pad
> **I want** the engine to recognize my pad as a controller
> **So that** I can play on physical hardware

**References**: REQ-INP-004, Roadmap Phase 6

**Status**: PLANNED (Phase 6)

### 📝 Description
Implement a `HidPadDriver` that uses SDL3's gamepad API to read button states and produce `InputSnapshot` instances. The driver enumerates connected gamepads and reads button state each poll.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Detect connected gamepad on startup**
    *   **Given** a USB dance pad is connected before the engine starts
    *   **When** the HidPadDriver initializes
    *   **Then** the device is detected and enumerated
    *   **And** a log message confirms detection with device name and VID/PID

*   **Scenario 2: Read button state each poll**
    *   **Given** a gamepad is connected and mapped
    *   **When** button 0 is pressed
    *   **Then** the corresponding PadInput is set in the snapshot
    *   **And** the state updates correctly on release

*   **Scenario 3: Handle gamepad disconnect gracefully**
    *   **Given** a gamepad is connected and in use
    *   **When** the device is unplugged
    *   **Then** an error is logged
    *   **And** the driver produces empty snapshots until reconnected
    *   **And** the engine does not crash

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points
*   **Dependencies**: US-INP-003
*   **Implementation Note**: SDL_OpenGamepad, SDL_GetGamepadButton. Handle hot-plug via SDL_EVENT_GAMEPAD_ADDED/REMOVED.

---

### Story ID: US-INP-032 - Configure Button-to-Panel Mapping Per Device

**Story Card:**
> **As a** Player with a non-standard dance pad
> **I want** to map each pad sensor to a panel
> **So that** my device works even if buttons are wired differently

**References**: REQ-INP-004, Roadmap Phase 6

**Status**: PLANNED (Phase 6)

### 📝 Description
Store button-to-panel mappings per device, identified by USB VID/PID. The mapping is a JSON object in settings.json specifying which gamepad button index maps to which PadInput.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Mapping saved per VID/PID**
    *   **Given** a device with VID 0x1234 and PID 0x5678 is configured
    *   **When** button 3 is mapped to P1_CENTER
    *   **Then** settings.json contains an entry: `{"1234:5678": {"3": "P1_CENTER"}}`

*   **Scenario 2: Mapping loaded on device connection**
    *   **Given** a device mapping exists in settings.json
    *   **When** the device is connected
    *   **Then** the HidPadDriver loads the saved mapping
    *   **And** button presses are translated according to the mapping

*   **Scenario 3: Unmapped device uses default mapping**
    *   **Given** a device has no saved mapping
    *   **When** the device is connected
    *   **Then** a default sequential mapping is used (button 0 → P1_DOWN_LEFT, etc.)
    *   **And** a warning is logged suggesting configuration

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-INP-031
*   **Implementation Note**: Device ID format: `"{VID:04X}:{PID:04X}"`. Mapping: button index (int) to PadInput string.

---

### Story ID: US-INP-033 - Configure Axis Threshold for Analog Sensors

**Story Card:**
> **As a** Player with analog-sensor pads
> **I want** to configure the activation threshold
> **So that** I can tune sensitivity to match my playstyle

**References**: REQ-INP-004, Roadmap Phase 6

**Status**: PLANNED (Phase 6)

### 📝 Description
Support axis-based sensors (common in some USB pads) by reading SDL gamepad axis values and comparing them to a configurable threshold. If the axis exceeds the threshold, the corresponding panel is considered pressed.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Axis value above threshold registers as pressed**
    *   **Given** the threshold for axis 0 is set to 0.5 (on a -1.0 to 1.0 scale)
    *   **When** axis 0 reads 0.7
    *   **Then** the mapped PadInput is set as pressed in the snapshot

*   **Scenario 2: Axis value below threshold registers as released**
    *   **Given** the threshold for axis 0 is set to 0.5
    *   **When** axis 0 reads 0.3
    *   **Then** the mapped PadInput is not set in the snapshot

*   **Scenario 3: Threshold configurable per device**
    *   **Given** two devices are connected
    *   **When** different thresholds are configured for each device
    *   **Then** each device applies its own threshold independently
    *   **And** the thresholds persist in settings.json

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-INP-032
*   **Implementation Note**: SDL_GetGamepadAxis returns -32768 to 32767. Normalize to -1.0 to 1.0. Default threshold: 0.5.

---

## Epic: Arcade Hardware Support

### Story ID: US-INP-041 - Implement ArcadeIODriver for PIUIO Boards

**Story Card:**
> **As an** Arcade Operator
> **I want** the engine to read PIUIO arcade I/O board sensors
> **So that** I can deploy the engine on real arcade cabinets

**References**: REQ-INP-005, Roadmap Phase 8

**Status**: PLANNED (Phase 8)

### 📝 Description
Implement an `ArcadeIODriver` that communicates with PIUIO v1 and v2 boards via USB bulk transfer. The driver reads the 10 sensor states (5 per player) and produces separate `InputSnapshot` instances for each player.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Detect PIUIO board on startup**
    *   **Given** a PIUIO board is connected
    *   **When** the ArcadeIODriver initializes
    *   **Then** the device is detected via USB enumeration
    *   **And** a log message confirms detection with hardware version

*   **Scenario 2: Read sensor states at 1000 Hz**
    *   **Given** the driver is active
    *   **When** measuring the polling rate over 10 seconds
    *   **Then** at least 10000 reads occur
    *   **And** the polling rate is stable within 5% variance

*   **Scenario 3: Produce separate snapshots per player**
    *   **Given** sensors 0-4 are for P1 and sensors 5-9 are for P2
    *   **When** sensor 0 (P1) and sensor 5 (P2) are pressed
    *   **Then** polling with player=0 returns a snapshot with P1_DOWN_LEFT pressed
    *   **And** polling with player=1 returns a snapshot with P2_DOWN_LEFT pressed
    *   **And** the snapshots are independent

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 8 story points
*   **Dependencies**: US-INP-003
*   **Implementation Note**: Use libusb or platform USB API. PIUIO VID:PID = 0x0547:0x1002. Bulk transfer protocol is documented in community resources.

---

### Story ID: US-INP-042 - Support PIUIO v1 and v2 Hardware

**Story Card:**
> **As an** Arcade Operator with older hardware
> **I want** the engine to work with both PIUIO v1 and v2 boards
> **So that** I don't need to upgrade my I/O hardware

**References**: REQ-INP-005, Roadmap Phase 8

**Status**: PLANNED (Phase 8)

### 📝 Description
Detect PIUIO hardware version during initialization and apply version-specific protocol differences. Both versions use the same VID/PID but differ in packet structure.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Detect v1 board**
    *   **Given** a PIUIO v1 board is connected
    *   **When** the driver queries device version
    *   **Then** the driver identifies it as v1
    *   **And** v1 packet format is used

*   **Scenario 2: Detect v2 board**
    *   **Given** a PIUIO v2 board is connected
    *   **When** the driver queries device version
    *   **Then** the driver identifies it as v2
    *   **And** v2 packet format is used

*   **Scenario 3: Sensor readings correct for both versions**
    *   **Given** a sensor is pressed on either v1 or v2 hardware
    *   **When** polling input
    *   **Then** the corresponding PadInput is set correctly
    *   **And** behavior is identical between versions

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points
*   **Dependencies**: US-INP-041
*   **Implementation Note**: Version detection via initial handshake packet. Community documentation contains packet structure for both versions.

---

### Story ID: US-INP-051 - Control Panel Lamps During Gameplay

**Story Card:**
> **As a** Player on an arcade cabinet
> **I want** panel lamps to illuminate when notes pass the receptor line
> **So that** I get visual feedback matching the original arcade experience

**References**: REQ-INP-011, Roadmap Phase 8

**Status**: PLANNED (Phase 8)

### 📝 Description
Add lamp output control to the ArcadeIODriver. When a note passes the receptor line during gameplay, illuminate the corresponding panel lamp for a configurable duration.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Lamp illuminates on note hit**
    *   **Given** a note reaches the receptor line
    *   **When** the player hits the note
    *   **Then** the corresponding panel lamp turns on
    *   **And** the lamp stays on for the configured duration

*   **Scenario 2: Lamp duration configurable**
    *   **Given** lamp duration is set to 200ms
    *   **When** a lamp is activated
    *   **Then** the lamp turns off after 200ms (±10ms tolerance)

*   **Scenario 3: Multiple lamps can be active simultaneously**
    *   **Given** 5 notes are hit within 100ms of each other
    *   **When** all notes are hit
    *   **Then** all 5 corresponding lamps illuminate
    *   **And** each lamp times out independently

*   **Scenario 4: Lamp control does not affect input polling**
    *   **Given** lamp output is active
    *   **When** measuring input polling rate
    *   **Then** the polling rate remains at 1000 Hz
    *   **And** no input latency increase is measurable

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points
*   **Dependencies**: US-INP-041, REQ-JDG-001 (judge to emit note hit events)
*   **Implementation Note**: Lamp state is written in the same USB packet as input reads. Default duration: 150ms.

---

## Epic: Multi-Player Input

### Story ID: US-INP-061 - Separate Input Snapshots Per Player

**Story Card:**
> **As a** Developer
> **I want** independent InputSnapshot instances for P1 and P2
> **So that** co-op and battle modes can process each player's input separately

**References**: REQ-INP-006, Roadmap Phase 5

**Status**: PLANNED (Phase 5)

### 📝 Description
Modify the input system to produce two `InputSnapshot` instances per tick, one for each player. Each driver is responsible for routing inputs to the correct player.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: InputSystem produces two snapshots per tick**
    *   **Given** the engine is running with multi-player enabled
    *   **When** polling input
    *   **Then** two snapshots are returned: one for P1, one for P2
    *   **And** each snapshot contains only that player's inputs

*   **Scenario 2: P1 and P2 snapshots are independent**
    *   **Given** P1 presses P1_CENTER and P2 presses P2_CENTER simultaneously
    *   **When** polling input
    *   **Then** the P1 snapshot contains only P1_CENTER pressed
    *   **And** the P2 snapshot contains only P2_CENTER pressed
    *   **And** no cross-contamination occurs

*   **Scenario 3: Both snapshots available same tick**
    *   **Given** a scene needs input for both players
    *   **When** the scene's update method is called
    *   **Then** both snapshots are available immediately
    *   **And** both snapshots have the same tick timestamp

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-INP-002, US-INP-011
*   **Implementation Note**: InputSystem::poll() returns std::array<InputSnapshot, 2>. Single-player mode uses only index 0.

---

### Story ID: US-INP-062 - Keyboard Driver Player Assignment

**Story Card:**
> **As a** Player in co-op mode
> **I want** to bind specific keys to P1 or P2
> **So that** two players can use one keyboard simultaneously

**References**: REQ-INP-006, Roadmap Phase 5

**Status**: PLANNED (Phase 5)

### 📝 Description
Extend the keymap to assign each key to a specific player. Keys mapped to P1 inputs only affect the P1 snapshot, and vice versa for P2.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: P1 keys produce P1 snapshot**
    *   **Given** Q is mapped to P1_DOWN_LEFT
    *   **When** Q is pressed
    *   **Then** the P1 snapshot contains P1_DOWN_LEFT pressed
    *   **And** the P2 snapshot is unaffected

*   **Scenario 2: P2 keys produce P2 snapshot**
    *   **Given** Numpad 1 is mapped to P2_DOWN_LEFT
    *   **When** Numpad 1 is pressed
    *   **Then** the P2 snapshot contains P2_DOWN_LEFT pressed
    *   **And** the P1 snapshot is unaffected

*   **Scenario 3: Default keymap includes P2 bindings**
    *   **Given** no user keymap exists
    *   **When** the KeyboardDriver initializes
    *   **Then** default P2 bindings are active (e.g., Numpad 1-9 for P2 panels)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-INP-021, US-INP-061
*   **Implementation Note**: Keymap format: `{"SDL_SCANCODE_Q": "P1_DOWN_LEFT", "SDL_SCANCODE_KP_1": "P2_DOWN_LEFT"}`

---

### Story ID: US-INP-063 - HID Driver Device-to-Player Binding

**Story Card:**
> **As a** Player in co-op mode
> **I want** to assign each USB pad to P1 or P2
> **So that** two pads can be used for co-op play

**References**: REQ-INP-006, Roadmap Phase 6

**Status**: PLANNED (Phase 6)

### 📝 Description
Allow each connected gamepad to be bound to either P1 or P2. The binding is stored per device VID/PID in settings.json.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Device bound to P1**
    *   **Given** device 1234:5678 is bound to P1
    *   **When** button 0 is pressed on that device
    *   **Then** the P1 snapshot contains the mapped input
    *   **And** the P2 snapshot is unaffected

*   **Scenario 2: Device bound to P2**
    *   **Given** device ABCD:EF01 is bound to P2
    *   **When** button 0 is pressed on that device
    *   **Then** the P2 snapshot contains the mapped input
    *   **And** the P1 snapshot is unaffected

*   **Scenario 3: Default assignment is P1 for first device**
    *   **Given** a device has no player binding
    *   **When** the device is connected
    *   **Then** it defaults to P1
    *   **And** a warning is logged suggesting configuration

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-INP-031, US-INP-061
*   **Implementation Note**: Settings format: `{"hid_devices": {"1234:5678": {"player": 0}}}`

---

### Story ID: US-INP-064 - Arcade I/O Native Player Separation

**Story Card:**
> **As a** Developer
> **I want** the PIUIO driver to natively separate P1 and P2 sensors
> **So that** arcade cabinets naturally support two-player input

**References**: REQ-INP-006, Roadmap Phase 8

**Status**: PLANNED (Phase 8)

### 📝 Description
The PIUIO hardware naturally separates sensors into two groups: sensors 0-4 for P1, sensors 5-9 for P2. The driver should map these directly to the corresponding player snapshots.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: P1 sensors map to P1 snapshot**
    *   **Given** sensors 0-4 are P1 on the PIUIO board
    *   **When** sensor 0 is activated
    *   **Then** the P1 snapshot contains P1_DOWN_LEFT pressed
    *   **And** the P2 snapshot is empty

*   **Scenario 2: P2 sensors map to P2 snapshot**
    *   **Given** sensors 5-9 are P2 on the PIUIO board
    *   **When** sensor 5 is activated
    *   **Then** the P2 snapshot contains P2_DOWN_LEFT pressed
    *   **And** the P1 snapshot is empty

*   **Scenario 3: Both players can activate simultaneously**
    *   **Given** sensor 0 (P1) and sensor 5 (P2) are activated simultaneously
    *   **When** polling input
    *   **Then** both snapshots contain their respective inputs
    *   **And** no interference occurs

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-INP-041, US-INP-061
*   **Implementation Note**: Sensor mapping is fixed by hardware. Driver just needs to route to correct snapshot.

---

## Epic: Input Configuration and Calibration

### Story ID: US-INP-071 - Create Input Mapping Configuration Screen

**Story Card:**
> **As a** Player with a new dance pad
> **I want** an in-game screen to configure my pad button mappings
> **So that** I can set up my controller without editing JSON files

**References**: REQ-INP-008, Roadmap Phase 6

**Status**: PLANNED (Phase 6)

### 📝 Description
Implement a configuration screen showing all 10 panels in visual layout. The player selects a panel, then presses the physical button they want to assign to it. The mapping is saved immediately to settings.json.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Visual layout shows all panels**
    *   **Given** the input mapping screen is displayed
    *   **When** viewing the screen
    *   **Then** all 10 panels are shown in the PIU X-pattern layout
    *   **And** current bindings are displayed on each panel

*   **Scenario 2: Enter mapping mode for a panel**
    *   **Given** the input mapping screen is displayed
    *   **When** the player selects P1_CENTER
    *   **Then** the screen prompts "Press button for Center Panel (P1)"
    *   **And** input is captured from all drivers

*   **Scenario 3: Assign button to panel**
    *   **Given** mapping mode is active for P1_CENTER
    *   **When** the player presses gamepad button 5
    *   **Then** button 5 is mapped to P1_CENTER
    *   **And** the mapping is saved immediately to settings.json
    *   **And** the screen returns to the layout view

*   **Scenario 4: Changes take effect immediately**
    *   **Given** a button mapping is changed
    *   **When** the player exits the configuration screen
    *   **Then** the new mapping is active without restarting the engine

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points
*   **Dependencies**: US-INP-031, US-INP-032, REQ-SCN-002 (scene system)
*   **Implementation Note**: Scene renders BGA background + sprite overlays for panel layout. Capture first input event after mode activation.

---

### Story ID: US-INP-072 - Per-Device Input Calibration Offsets

**Story Card:**
> **As a** Player with noticeable input lag on my pad
> **I want** to configure a timing offset per device
> **So that** I can compensate for device-specific latency

**References**: REQ-INP-009, Roadmap Phase 6

**Status**: PLANNED (Phase 6)

### 📝 Description
Store a timing offset per device in settings.json. The offset is applied to the snapshot timestamp before the judge sees it, effectively shifting when the input is considered to have occurred.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Offset stored per device**
    *   **Given** device 1234:5678 is assigned an offset of -15ms
    *   **When** settings are saved
    *   **Then** settings.json contains `{"hid_devices": {"1234:5678": {"offset_ms": -15}}}`

*   **Scenario 2: Negative offset shifts input earlier**
    *   **Given** a device has an offset of -10ms
    *   **When** a button is pressed at tick 60
    *   **Then** the snapshot timestamp reflects tick 60 minus 10ms
    *   **And** the judge sees the input as occurring earlier

*   **Scenario 3: Positive offset shifts input later**
    *   **Given** a device has an offset of +10ms
    *   **When** a button is pressed at tick 60
    *   **Then** the snapshot timestamp reflects tick 60 plus 10ms
    *   **And** the judge sees the input as occurring later

*   **Scenario 4: Offset range validated**
    *   **Given** a player attempts to set an offset of -300ms
    *   **When** the configuration is saved
    *   **Then** the offset is clamped to -200ms
    *   **And** a warning is logged

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-INP-031, REQ-JDG-001 (judge consumes timestamps)
*   **Implementation Note**: Offset range: -200ms to +200ms in 1ms increments. Default: 0ms.

---

### Story ID: US-INP-073 - Calibration Feedback Screen

**Story Card:**
> **As a** Player calibrating input timing
> **I want** real-time feedback on my adjusted timing
> **So that** I can find the optimal offset for my device

**References**: REQ-INP-009, Roadmap Phase 6

**Status**: PLANNED (Phase 6)

### 📝 Description
Create a calibration screen that plays a simple song with a metronome. After each hit, display the raw timing error and the adjusted timing error side-by-side, allowing the player to tune the offset.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Display raw and adjusted timing**
    *   **Given** the calibration screen is active with offset = -10ms
    *   **When** the player hits a note 15ms late
    *   **Then** the screen displays "Raw: +15ms, Adjusted: +5ms"
    *   **And** the adjusted timing is used for the judgment

*   **Scenario 2: Adjust offset in real-time**
    *   **Given** the calibration screen is active
    *   **When** the player increments the offset by 5ms
    *   **Then** the next hit shows the new adjusted timing
    *   **And** no restart is required

*   **Scenario 3: Average timing displayed**
    *   **Given** the player has hit 20 notes
    *   **When** viewing the calibration screen
    *   **Then** the average adjusted timing error is displayed
    *   **And** a suggestion is shown (e.g., "Try -5ms offset")

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 story points
*   **Dependencies**: US-INP-072, REQ-JDG-001 (judge timing)
*   **Implementation Note**: Use a simple song with steady BPM. Metronome sound plays on beat. Display rolling average of last 20 hits.

---

## Epic: Simultaneous Input Devices

### Story ID: US-INP-081 - Merge Input from Multiple Drivers

**Story Card:**
> **As a** Player
> **I want** to use my keyboard for menus and my pad for gameplay simultaneously
> **So that** I don't need to switch between devices mid-session

**References**: REQ-INP-010, Roadmap Phase 6

**Status**: PLANNED (Phase 6)

### 📝 Description
Allow multiple input drivers to be active simultaneously. Each driver produces a snapshot, and the snapshots are merged into a single snapshot per player using a configurable merge strategy.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Multiple drivers active**
    *   **Given** both KeyboardDriver and HidPadDriver are enabled
    *   **When** polling input
    *   **Then** both drivers are polled
    *   **And** snapshots are produced from both

*   **Scenario 2: OR-merge strategy**
    *   **Given** merge strategy is set to OR-merge
    *   **When** keyboard presses P1_CENTER and pad presses P1_DOWN_LEFT simultaneously
    *   **Then** the final snapshot contains both inputs pressed
    *   **And** no input is lost

*   **Scenario 3: First-wins strategy**
    *   **Given** merge strategy is set to first-wins
    *   **When** both keyboard and pad press P1_CENTER simultaneously
    *   **Then** the final snapshot contains P1_CENTER pressed once
    *   **And** the keyboard snapshot is used (higher priority)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 story points
*   **Dependencies**: US-INP-003, US-INP-021, US-INP-031
*   **Implementation Note**: InputSystem owns vector of drivers. Default strategy: OR-merge. Priority order: keyboard > HID > arcade I/O.

---

### Story ID: US-INP-082 - Configure Driver Priority

**Story Card:**
> **As a** Player
> **I want** to configure which input device takes priority
> **So that** I can control conflict resolution when multiple devices trigger the same input

**References**: REQ-INP-010, Roadmap Phase 6

**Status**: PLANNED (Phase 6)

### 📝 Description
Add a priority field to each driver configuration in settings.json. When first-wins merge strategy is active, drivers with lower priority values are consulted first.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Priority configurable in settings**
    *   **Given** settings.json is opened for editing
    *   **When** the player sets keyboard priority to 10 and HID priority to 20
    *   **Then** the settings are persisted
    *   **And** keyboard inputs take precedence over HID inputs on conflicts

*   **Scenario 2: Default priority order**
    *   **Given** no priority configuration exists
    *   **When** multiple drivers are active
    *   **Then** the default order is keyboard (priority 10), HID (priority 20), arcade I/O (priority 30)

*   **Scenario 3: Priority affects only first-wins mode**
    *   **Given** merge strategy is OR-merge
    *   **When** multiple drivers provide input
    *   **Then** all inputs are merged regardless of priority

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-INP-081
*   **Implementation Note**: Settings format: `{"drivers": [{"type": "keyboard", "priority": 10}]}`. Lower number = higher priority.

---

## Non-Functional Requirements

These are system-wide quality attributes for input processing, verified through integration testing.

### NFR: Input Latency Minimization (REQ-INP-012)

**As a** Player in a rhythm game
**I want** minimal delay between pressing a panel and the judge evaluating it
**So that** timing feels responsive and accurate

**Target Latency**: Under 10ms on target hardware

**Per-Component Budgets**:
- USB poll: <3ms
- Snapshot creation: <0.5ms
- Edge detection: <0.5ms
- Driver dispatch: <0.5ms
- Total input path: <5ms

**Acceptance Criteria**:
*   **Scenario 1: Keyboard input latency**
    *   **Given** a high-speed input capture device is monitoring keyboard input
    *   **When** a key is pressed
    *   **Then** the input appears in the next snapshot within 3ms

*   **Scenario 2: USB pad input latency**
    *   **Given** a high-speed input capture device is monitoring USB pad input
    *   **When** a button is pressed
    *   **Then** the input appears in a snapshot within 8ms

*   **Scenario 3: Arcade I/O input latency**
    *   **Given** a PIUIO board is polled at 1000 Hz
    *   **When** a sensor is activated
    *   **Then** the input appears in a snapshot within 2ms

**Testing Note**: Specialized hardware required for accurate measurement. Deferred to Phase 8+ when arcade hardware is available.

---

## Story Summary by Status

### PLANNED (Phase 1) - 12 stories
- US-INP-001: Define PadInput enum for all game controls
- US-INP-002: Create InputSnapshot structure
- US-INP-003: Define InputDriver interface
- US-INP-011: Capture input snapshot once per tick
- US-INP-012: Detect press and release edge events
- US-INP-013: Verify no input events lost
- US-INP-021: Implement KeyboardDriver with configurable keymap
- US-INP-022: Provide default QWEASDZXC keymap
- US-INP-023: Persist keymap to settings file
- US-INP-024: Support 10+ key rollover

### PLANNED (Phase 5) - 3 stories
- US-INP-061: Separate input snapshots per player
- US-INP-062: Keyboard driver player assignment

### PLANNED (Phase 6) - 9 stories
- US-INP-031: Implement HidPadDriver using SDL3 gamepad API
- US-INP-032: Configure button-to-panel mapping per device
- US-INP-033: Configure axis threshold for analog sensors
- US-INP-063: HID driver device-to-player binding
- US-INP-071: Create input mapping configuration screen
- US-INP-072: Per-device input calibration offsets
- US-INP-073: Calibration feedback screen
- US-INP-081: Merge input from multiple drivers
- US-INP-082: Configure driver priority

### PLANNED (Phase 8) - 4 stories
- US-INP-041: Implement ArcadeIODriver for PIUIO boards
- US-INP-042: Support PIUIO v1 and v2 hardware
- US-INP-051: Control panel lamps during gameplay
- US-INP-064: Arcade I/O native player separation

### FUTURE - 0 stories
(REQ-INP-012 input latency minimization is captured as NFR, not stories)

---

## Story Point Summary

| Phase | Count | Total Points |
|-------|-------|--------------|
| Phase 1 | 10 | 25 |
| Phase 5 | 3 | 7 |
| Phase 6 | 9 | 28 |
| Phase 8 | 4 | 22 |
| **Total** | **26** | **82** |

---

## Dependency Graph

```
US-INP-001 (PadInput Enum) ──> US-INP-002 (InputSnapshot)
                               └──> US-INP-003 (InputDriver Interface)

US-INP-002 + US-INP-003 ──> US-INP-011 (Snapshot Per Tick) ──> US-INP-012 (Edge Detection)
                            └──> US-INP-061 (Multi-Player)

US-INP-012 ──> US-INP-013 (No Event Loss)

US-INP-003 ──> US-INP-021 (KeyboardDriver) ──> US-INP-022 (Default Keymap)
                                              └──> US-INP-023 (Keymap Persistence)
                                              └──> US-INP-024 (Key Rollover)
                                              └──> US-INP-062 (Player Assignment)

US-INP-003 ──> US-INP-031 (HidPadDriver) ──> US-INP-032 (Button Mapping)
                                            └──> US-INP-033 (Axis Threshold)
                                            └──> US-INP-063 (Device to Player)
                                            └──> US-INP-071 (Config Screen)
                                            └──> US-INP-072 (Calibration Offsets)

US-INP-072 ──> US-INP-073 (Calibration Feedback)

US-INP-003 ──> US-INP-041 (ArcadeIODriver) ──> US-INP-042 (PIUIO v1/v2)
                                              └──> US-INP-051 (Panel Lamps)
                                              └──> US-INP-064 (Native P1/P2 Separation)

US-INP-021 + US-INP-031 ──> US-INP-081 (Multi-Driver Merge) ──> US-INP-082 (Driver Priority)

US-INP-061 ──> US-INP-062, US-INP-063, US-INP-064 (all require multi-player snapshots)
```

---

## Notes

**Timing Consistency**: All input snapshot timestamps must be relative to the same clock used by the audio system (subsystem 3) to ensure judge timing accuracy.

**Device Hotplug**: All drivers must handle device connect/disconnect gracefully without crashing. Devices connected mid-session should be detected and usable after the next poll.

**Cross-Story Dependencies**: Several stories depend on the judge system (REQ-JDG) for timing feedback (US-INP-072, US-INP-073) and lamp triggering (US-INP-051). These will be blocked until judge stories are implemented.

**Injectable Mocks**: Phase 1 stories use injectable input sources for unit testing without hardware dependencies. Real hardware verification occurs in Phase 6+ when HID and arcade drivers are implemented.

---

*Generated from docs/requirements/02-input-system.md*
*Last updated: 2026-04-26*
