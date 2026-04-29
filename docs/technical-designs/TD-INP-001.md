# TD-INP-001: Input System — Abstraction, Edge Detection, and Keyboard Driver

**Stories**: US-INP-001, US-INP-002, US-INP-003, US-INP-011, US-INP-012, US-INP-013, US-INP-021, US-INP-022, US-INP-024
**Phase**: 1
**Author**: technical-architect agent
**Status**: Draft

## Overview

This design introduces the input subsystem for Phase 1: a `PadInput` enum defining all game controls, an `InputSnapshot` value type for per-tick input state, an `InputDriver` interface for backend abstraction, an `InputSystem` coordinator owned by `Engine`, and a `KeyboardDriver` that maps SDL keyboard state to pad inputs. The subsystem integrates into the fixed-step game loop defined in TD-ENG-001, polling once per tick before scene update. The design prioritizes testability — every component is usable without SDL through injectable event sources.

The design follows patterns established in the existing codebase: `std::unique_ptr` ownership (as in `Engine`→`Clock`), `std::function` injection (as in `TextureCache::ImageLoaderFn`), enum-to-string conversion helpers (as in `BlendEffect` in `keyframe.h`), and header-only value types (as in `types.h`).

## Architecture

### Component Diagram

```
Engine (src/openitup/core/engine.h)
  |  owns (unique_ptr)
  |
  └── InputSystem (src/openitup/input/input_system.h)
        |  owns (unique_ptr)
        |
        └── InputDriver* (interface)
              |
              └── KeyboardDriver (src/openitup/input/keyboard_driver.h)
                    |  uses
                    └── KeyboardStateFn (injectable, defaults to SDL_GetKeyboardState)

Engine::update(dt):
  1. input_system_->poll()       // produces InputSnapshot for this tick
  2. scene_stack_->top().update(dt, input_system_->snapshot())
```

### New Types

#### `PadInput` (`src/openitup/input/pad_input.h`)

A bitmask-compatible enum representing all gameplay and menu inputs. Each value is a unique power of two, enabling bitwise OR combination into a `uint32_t` bitmask. The enum has 14 values requiring 14 bits, well within `uint32_t` range.

```cpp
// src/openitup/input/pad_input.h
#pragma once

#include <cstdint>
#include <string>

namespace openitup {

enum class PadInput : uint32_t {
    P1_DOWN_LEFT  = 1 << 0,   // 0x0001
    P1_UP_LEFT    = 1 << 1,   // 0x0002
    P1_CENTER     = 1 << 2,   // 0x0004
    P1_UP_RIGHT   = 1 << 3,   // 0x0008
    P1_DOWN_RIGHT = 1 << 4,   // 0x0010
    P2_DOWN_LEFT  = 1 << 5,   // 0x0020
    P2_UP_LEFT    = 1 << 6,   // 0x0040
    P2_CENTER     = 1 << 7,   // 0x0080
    P2_UP_RIGHT   = 1 << 8,   // 0x0100
    P2_DOWN_RIGHT = 1 << 9,   // 0x0200
    START         = 1 << 10,  // 0x0400
    BACK          = 1 << 11,  // 0x0800
    SELECT        = 1 << 12,  // 0x1000
    COIN          = 1 << 13,  // 0x2000
};

// Total number of defined PadInput values.
inline constexpr int PAD_INPUT_COUNT = 14;

// All PadInput values in declaration order, for iteration.
inline constexpr PadInput ALL_PAD_INPUTS[] = {
    PadInput::P1_DOWN_LEFT,  PadInput::P1_UP_LEFT,    PadInput::P1_CENTER,
    PadInput::P1_UP_RIGHT,   PadInput::P1_DOWN_RIGHT,
    PadInput::P2_DOWN_LEFT,  PadInput::P2_UP_LEFT,    PadInput::P2_CENTER,
    PadInput::P2_UP_RIGHT,   PadInput::P2_DOWN_RIGHT,
    PadInput::START, PadInput::BACK, PadInput::SELECT, PadInput::COIN,
};

// Bitwise OR for combining PadInput values into a bitmask.
inline constexpr uint32_t operator|(PadInput a, PadInput b) {
    return static_cast<uint32_t>(a) | static_cast<uint32_t>(b);
}

inline constexpr uint32_t operator|(uint32_t a, PadInput b) {
    return a | static_cast<uint32_t>(b);
}

// String conversion for logging and debugging.
const char* pad_input_to_string(PadInput input);
PadInput pad_input_from_string(const std::string& name);

} // namespace openitup
```

**Key decisions**:

- `enum class` (scoped) rather than plain enum, for type safety. Bitwise operators are provided as free functions.
- P1 panels occupy bits 0-4, P2 panels occupy bits 5-9, menu actions occupy bits 10-13. This grouping is deliberate: extracting P1 or P2 panels from a bitmask is a single mask operation (e.g., `held & 0x001F` for P1 panels).
- `ALL_PAD_INPUTS` array enables iteration without reflection. Follows the pattern of the existing `BlendEffect` string conversion in `keyframe.h`.

---

#### `InputSnapshot` (`src/openitup/input/input_snapshot.h`)

An immutable value type holding the complete input state for a single tick: which inputs are currently held, which were newly pressed this tick, and which were newly released this tick. All three fields are `uint32_t` bitmasks using `PadInput` bit positions.

```cpp
// src/openitup/input/input_snapshot.h
#pragma once

#include <cstdint>

#include <openitup/input/pad_input.h>

namespace openitup {

class InputSnapshot {
public:
    // Construct a snapshot. All fields are set at construction and never change.
    // tick_number: the engine tick this snapshot corresponds to (for debug/logging).
    InputSnapshot(uint32_t held, uint32_t pressed, uint32_t released,
                  uint64_t tick_number);

    // Default: empty snapshot (nothing held, pressed, or released).
    InputSnapshot();

    // Query single-input state.
    bool is_held(PadInput input) const;
    bool is_pressed(PadInput input) const;
    bool is_released(PadInput input) const;

    // Raw bitmask access for bulk operations and driver merging.
    uint32_t held_mask() const;
    uint32_t pressed_mask() const;
    uint32_t released_mask() const;

    // The engine tick this snapshot was captured at.
    uint64_t tick_number() const;

    // True if no inputs are held, pressed, or released.
    bool empty() const;

private:
    uint32_t held_ = 0;
    uint32_t pressed_ = 0;
    uint32_t released_ = 0;
    uint64_t tick_number_ = 0;
};

} // namespace openitup
```

**Key decisions**:

- The snapshot is a value type (copyable, movable). It is made effectively immutable by providing no setters and declaring all fields private with const-returning accessors. We do NOT use `const` member variables because that would delete the move assignment operator, making the type unusable in `std::vector` and `std::optional`.
- `tick_number` is included for debugging and logging, not for gameplay logic. It comes from `Engine::tick_count()`.
- `empty()` is a convenience for short-circuiting in scenes that need to check "did anything happen this tick."

---

#### `InputDriver` (`src/openitup/input/input_driver.h`)

An abstract interface that all input backends implement. Each driver produces a raw held-state bitmask each time it is polled. The `InputSystem` (not the driver) handles edge detection.

```cpp
// src/openitup/input/input_driver.h
#pragma once

#include <cstdint>
#include <string>

namespace openitup {

class InputDriver {
public:
    virtual ~InputDriver() = default;

    // Poll the device and return a bitmask of currently held PadInput values.
    // Called once per tick by InputSystem.
    // The driver must not perform edge detection — that is InputSystem's job.
    virtual uint32_t poll_held() = 0;

    // Human-readable name for logging and configuration UI.
    virtual std::string device_name() const = 0;
};

} // namespace openitup
```

**Key decisions**:

- The driver returns a raw `uint32_t` held mask, NOT an `InputSnapshot`. Edge detection (pressed/released computation) belongs in `InputSystem` because it requires comparing the current state against the previous tick's state. Drivers don't know about previous state; they only know what is physically held right now. This separation means:
  - Drivers are simpler (just read hardware state).
  - Edge detection logic is centralized and testable in one place.
  - When multiple drivers are merged in Phase 6 (US-INP-081), the merge happens on raw held masks before edge detection, producing correct edges for the combined state.
- No `player_index` parameter on `poll_held()`. US-INP-003 Scenario 2 mentions per-player polling, but Phase 1 is single-player only. Phase 5 (US-INP-061) will extend the architecture to per-player snapshots. For Phase 1, all inputs (P1 and P2 bits) live in a single bitmask. The KeyboardDriver maps P1 keys and menu keys; P2 bits are simply unused.

---

#### `InputSystem` (`src/openitup/input/input_system.h`)

The coordinator owned by `Engine`. It holds the active driver, performs per-tick polling, computes edge detection, and produces the `InputSnapshot` consumed by scenes.

```cpp
// src/openitup/input/input_system.h
#pragma once

#include <memory>

#include <openitup/input/input_driver.h>
#include <openitup/input/input_snapshot.h>

namespace openitup {

class InputSystem {
public:
    // Construct with an input driver. InputSystem takes ownership.
    explicit InputSystem(std::unique_ptr<InputDriver> driver);

    ~InputSystem();

    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;

    // Poll the driver and compute edge events.
    // tick_number: current engine tick, stored in the snapshot for debugging.
    // Must be called exactly once per tick, before scene update.
    void poll(uint64_t tick_number);

    // The snapshot for the current tick. Valid after poll() has been called.
    const InputSnapshot& snapshot() const;

    // The snapshot from the previous tick. Useful for debugging.
    const InputSnapshot& previous_snapshot() const;

    // Access the underlying driver (non-owning, for logging/config).
    InputDriver& driver();
    const InputDriver& driver() const;

private:
    std::unique_ptr<InputDriver> driver_;
    InputSnapshot current_;
    InputSnapshot previous_;
    uint32_t previous_held_ = 0;
};

} // namespace openitup
```

**Key decisions**:

- `InputSystem` owns the driver via `unique_ptr`, following TD-ENG-001's ownership pattern.
- `previous_held_` is a raw `uint32_t` stored separately from `previous_` snapshot. This is because edge detection needs only the previous held mask, and storing it as a separate field avoids extracting it from the previous snapshot every frame.
- The `poll()` method accepts `tick_number` from Engine so the snapshot can carry it. This keeps InputSystem decoupled from Engine (no back-reference).
- Phase 6 extension point: when multi-driver support is added (US-INP-081), `InputSystem` will hold a `std::vector<std::unique_ptr<InputDriver>>` instead of a single driver. The `poll()` method will OR-merge all drivers' held masks before edge detection. The interface to consumers (`snapshot()`) does not change.

---

#### `KeyboardDriver` (`src/openitup/input/keyboard_driver.h`)

The Phase 1 input backend. It reads SDL keyboard state through an injectable function, looks up each key in a configurable keymap, and produces a held-state bitmask.

```cpp
// src/openitup/input/keyboard_driver.h
#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL_scancode.h>

#include <openitup/input/input_driver.h>
#include <openitup/input/pad_input.h>

namespace openitup {

// Type alias for the SDL keyboard state function.
// Default: SDL_GetKeyboardState. Injectable for testing.
// Returns a pointer to an array of key states indexed by SDL_Scancode.
// The int* parameter receives the array length.
using KeyboardStateFn = std::function<const bool*(int*)>;

// A single key-to-pad mapping entry.
struct KeyMapping {
    SDL_Scancode scancode;
    PadInput input;
};

class KeyboardDriver : public InputDriver {
public:
    // Construct with default QWEASDZXC keymap and real SDL keyboard state.
    KeyboardDriver();

    // Construct with custom keymap and injectable keyboard state source.
    KeyboardDriver(std::vector<KeyMapping> keymap, KeyboardStateFn state_fn);

    // InputDriver interface.
    uint32_t poll_held() override;
    std::string device_name() const override;

    // Access the current keymap (for configuration UI in later phases).
    const std::vector<KeyMapping>& keymap() const;

    // Replace the keymap at runtime (for Phase 3 keymap persistence).
    void set_keymap(std::vector<KeyMapping> keymap);

    // Build the default QWEASDZXC keymap for P1 + menu actions.
    static std::vector<KeyMapping> default_keymap();

private:
    std::vector<KeyMapping> keymap_;
    KeyboardStateFn state_fn_;
};

} // namespace openitup
```

**Key decisions**:

- The keymap is a `std::vector<KeyMapping>` rather than `std::unordered_map<SDL_Scancode, PadInput>`. Reasons:
  - The keymap is small (9 panel keys + 3 menu keys = 12 entries in Phase 1). A linear scan of 12 entries per poll is faster than hash lookup due to cache locality.
  - `SDL_GetKeyboardState` returns a contiguous bool array. The poll loop iterates the keymap and indexes into this array, which is O(n) in keymap size regardless of data structure.
  - A vector preserves insertion order, which is useful for the configuration UI in Phase 6.
  - `std::vector<KeyMapping>` is trivially serializable to JSON for Phase 3 persistence.
- The injection point is `KeyboardStateFn`, matching the `TextureCache::ImageLoaderFn` pattern. The default constructor wraps `SDL_GetKeyboardState`. Tests provide a lambda that returns a test-controlled array.
- `default_keymap()` is a static factory method, not a global constant. This avoids static initialization order issues and keeps the mapping easily discoverable.
- We use `SDL_Scancode` (physical key position), NOT `SDL_Keycode` (logical key). Scancodes are layout-independent — pressing the physical Q key produces `SDL_SCANCODE_Q` regardless of whether the OS keyboard layout is QWERTY, AZERTY, or Dvorak. This matches PIU arcade behavior where panel positions are physical.

---

### Modified Types

#### `Engine` (`src/openitup/core/engine.h`)

- Add member: `std::unique_ptr<InputSystem> input_system_` -- Owned input subsystem, declared after `renderer_` and before the future `scene_stack_`.
- Add constructor parameter: Accept `std::unique_ptr<InputSystem>` for injection (testing).
- Add accessor: `InputSystem& get_input_system()` / `const InputSystem& get_input_system() const` -- Non-owning reference for scenes.
- Modify `update()`: Call `input_system_->poll(tick_count_)` as the first operation, before scene update.
- Modify `process_events()`: Continue to call `SDL_PollEvent` to handle `SDL_EVENT_QUIT` and window events. Input keys are NOT consumed here -- the KeyboardDriver reads state via `SDL_GetKeyboardState` which reflects the state after `SDL_PumpEvents` (called by `SDL_PollEvent`).
- Reason: US-INP-011 requires input polling integrated into the fixed-step loop, and TD-ENG-001 already designated `Engine` as the owner of InputSystem.

#### `Engine` (`src/openitup/core/engine.cpp`)

- Modify `update()` body to call `input_system_->poll()` before any scene update.
- Reason: US-INP-011 Scenario 2 requires input poll completes before scene update.

## File Plan

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/openitup/input/pad_input.h` | PadInput enum, bitwise operators, string conversion declarations |
| Create | `src/openitup/input/pad_input.cpp` | pad_input_to_string/from_string implementations |
| Create | `src/openitup/input/input_snapshot.h` | InputSnapshot class declaration |
| Create | `src/openitup/input/input_snapshot.cpp` | InputSnapshot method implementations |
| Create | `src/openitup/input/input_driver.h` | InputDriver abstract interface (header-only) |
| Create | `src/openitup/input/input_system.h` | InputSystem class declaration |
| Create | `src/openitup/input/input_system.cpp` | InputSystem poll + edge detection implementation |
| Create | `src/openitup/input/keyboard_driver.h` | KeyboardDriver class declaration |
| Create | `src/openitup/input/keyboard_driver.cpp` | KeyboardDriver poll + default keymap implementation |
| Modify | `src/openitup/core/engine.h` | Add InputSystem member, constructor overload, accessor |
| Modify | `src/openitup/core/engine.cpp` | Wire input_system_->poll() into update() |
| Modify | `CMakeLists.txt` | Add pad_input.cpp, input_snapshot.cpp, input_system.cpp, keyboard_driver.cpp to openitup_engine sources |
| Create | `test/test_input.cpp` | Unit tests for PadInput, InputSnapshot, InputSystem edge detection, KeyboardDriver |
| Modify | `CMakeLists.txt` | Add test_input.cpp to openitup_tests sources |

## Data Flow

### Normal Tick: Key Pressed

```
1. Engine::process_events()
   - SDL_PollEvent() drains SDL event queue
   - SDL internally updates keyboard state array via SDL_PumpEvents

2. Engine::update(FIXED_STEP)
   - Calls input_system_->poll(tick_count_)

3. InputSystem::poll(tick_number)
   a. previous_held_ = current_.held_mask()   // save previous state
   b. previous_ = current_                     // save previous snapshot
   c. uint32_t held = driver_->poll_held()     // ask driver for raw held state

4. KeyboardDriver::poll_held()
   a. const bool* keys = state_fn_(nullptr)    // SDL_GetKeyboardState
   b. uint32_t held = 0
   c. for (const auto& mapping : keymap_):
        if (keys[mapping.scancode]):
          held |= static_cast<uint32_t>(mapping.input)
   d. return held                              // e.g., 0x0004 if P1_CENTER held

5. Back in InputSystem::poll():
   e. uint32_t pressed  = held & ~previous_held_   // 0→1 transitions
   f. uint32_t released = ~held & previous_held_    // 1→0 transitions
   g. current_ = InputSnapshot(held, pressed, released, tick_number)

6. Engine::update() continues:
   - scene_stack_->top().update(dt, input_system_->snapshot())
   - Scene reads snapshot.is_pressed(PadInput::P1_CENTER) == true
```

### Edge Detection: Same-Frame Press and Release (US-INP-012 Scenario 4)

This scenario occurs when a key is pressed and released within a single polling interval (between two consecutive `poll()` calls). Since `KeyboardDriver` uses `SDL_GetKeyboardState` (which reflects the state at the moment of the call, not accumulated events), a sub-frame press-release appears as: the key is NOT held at poll time.

However, SDL's event queue DOES contain both the KEY_DOWN and KEY_UP events. To capture sub-frame press-release, the `KeyboardDriver` must also scan the SDL event queue for key events that occurred between the previous and current poll. This hybrid approach (state array + event scan) is necessary for correctness:

```
KeyboardDriver::poll_held():
  a. Scan any buffered key-down events to set transient_pressed_ bits
  b. Read SDL_GetKeyboardState for currently held state
  c. held = state_held | transient_pressed_
  d. Clear transient_pressed_
  e. Return held
```

Wait -- this introduces complexity and coupling to SDL events. Let me reconsider.

**Revised approach**: The `KeyboardDriver` reads only `SDL_GetKeyboardState`. If a key is pressed and released within a single poll interval, `poll_held()` returns 0 for that key at the next poll. The InputSystem sees no held state and no change from the previous held state, so it produces no press or release event. This means **sub-frame press-release events are lost for keyboard input.**

This is acceptable for Phase 1 because:
1. At 60 Hz, the polling interval is 16.7ms. A press-release within 16.7ms is physically implausible for a dance pad (typical foot contact is 50-100ms).
2. US-INP-024 specifically says to use `SDL_GetKeyboardState` for rollover support.
3. US-INP-012 Scenario 4 ("same-frame press and release") describes the edge detection algorithm behavior, not a physical requirement. If the held mask shows both press and release (via accumulated events), the edge detection correctly identifies both. But generating that situation from keyboard polling alone is not required.
4. Phase 6's `HidPadDriver` polls at a higher rate (1000 Hz for arcade), making this a non-issue for real hardware.

The edge detection algorithm in InputSystem correctly handles the case where `held` has a bit that `previous_held_` also has but with an intermediate off state -- it just cannot detect it from state-based polling alone. This is documented as a known limitation.

### Normal Tick: Key Released

```
Previous tick: held = 0x0004 (P1_CENTER)
This tick:     held = 0x0000

pressed  = 0x0000 & ~0x0004 = 0x0000   // nothing newly pressed
released = ~0x0000 & 0x0004 = 0x0004   // P1_CENTER newly released

Snapshot: held=0, pressed=0, released=P1_CENTER
```

### Continued Hold (No Edge)

```
Previous tick: held = 0x0004 (P1_CENTER)
This tick:     held = 0x0004 (P1_CENTER)

pressed  = 0x0004 & ~0x0004 = 0x0000   // not newly pressed
released = ~0x0004 & 0x0004 = 0x0000   // not released

Snapshot: held=P1_CENTER, pressed=0, released=0
```

### 10-Key Simultaneous Press (US-INP-024)

```
Previous tick: held = 0x0000
This tick:     held = 0x03FF (all 10 panel bits set)

pressed  = 0x03FF & ~0x0000 = 0x03FF   // all 10 newly pressed
released = ~0x03FF & 0x0000 = 0x0000   // nothing released

Snapshot: held=0x03FF, pressed=0x03FF, released=0
```

This works because `SDL_GetKeyboardState` returns the full keyboard state array. Each key is an independent bool -- no rollover limitation in the SDL API. The hardware rollover limit depends on the physical keyboard, but that is outside engine scope (US-INP-024 notes this).

## Dependencies

### Internal
- **Engine** (`src/openitup/core/engine.h`) -- Owns InputSystem via `unique_ptr`. Calls `poll()` in the fixed-step loop.
- **spdlog** -- Used for warning/error logging (unmapped key warnings in debug mode, driver initialization).
- **SDL3** -- `SDL_GetKeyboardState`, `SDL_Scancode` definitions. Already linked via `openitup_engine`.

### External (new libraries)
None. All dependencies already exist in the project.

## Architectural Decisions

### ADR-1: Edge Detection in InputSystem, Not in Drivers

- **Context**: Edge events (pressed/released) could be computed by each driver or by the InputSystem. US-INP-012 requires correct edge detection. Phase 6 adds multi-driver merging (US-INP-081).
- **Decision**: Drivers return raw held masks. InputSystem computes edges centrally.
- **Alternatives considered**: (a) Each driver returns a full `InputSnapshot` with edges -- this requires each driver to track its own previous state, and multi-driver merging in Phase 6 would need to re-merge edges, which is error-prone (OR-merging pressed masks from two drivers that press the same button at different times would double-count). (b) Drivers return events (press/release) rather than state -- this requires event buffering and ordering, adds complexity, and doesn't work with `SDL_GetKeyboardState` (which is state-based, not event-based).
- **Consequences**: Drivers are simpler. Edge detection is tested once. Multi-driver merging in Phase 6 is straightforward: OR-merge held masks, then compute edges on the merged result.

### ADR-2: SDL_GetKeyboardState Over SDL Event Queue

- **Context**: US-INP-024 requires 10+ key rollover. US-INP-021 notes to use `SDL_GetKeyboardState` for this.
- **Decision**: `KeyboardDriver::poll_held()` reads `SDL_GetKeyboardState` to build the held mask.
- **Alternatives considered**: (a) Process `SDL_EVENT_KEY_DOWN`/`SDL_EVENT_KEY_UP` events to track state -- this works but requires careful state management (must handle focus loss, alt-tab, etc.) and the SDL event queue has platform-dependent limits that could drop events under extreme input. (b) Hybrid approach (events for edges, state for held) -- adds complexity and SDL coupling for minimal benefit at 60 Hz polling.
- **Consequences**: Sub-frame press-release events are invisible to the keyboard driver. This is acceptable at 60 Hz (see Data Flow discussion above). `SDL_GetKeyboardState` is called after `SDL_PumpEvents` (done by `Engine::process_events()` via `SDL_PollEvent`), so it always reflects the latest state.

### ADR-3: Vector of KeyMapping Over unordered_map

- **Context**: US-INP-021 needs a keymap data structure. US-INP-022 hardcodes a default. Phase 3 (US-INP-023) will persist it.
- **Decision**: `std::vector<KeyMapping>` where each entry is a `{scancode, PadInput}` pair.
- **Alternatives considered**: (a) `std::unordered_map<SDL_Scancode, PadInput>` -- hash overhead for 12 entries exceeds linear scan cost; harder to serialize; no ordering for UI display. (b) `std::array<PadInput, SDL_SCANCODE_COUNT>` (direct-indexed array) -- fast O(1) lookup but wastes ~1.2 KB (512 scancodes * ~4 bytes) for 12 active entries, and makes iteration over active mappings awkward.
- **Consequences**: `poll_held()` iterates 12 entries per tick (720 entries/second at 60 Hz). This is trivially fast. The vector is directly serializable to JSON for Phase 3.

### ADR-4: Scancode (Physical Position) Over Keycode (Logical Character)

- **Context**: SDL offers both `SDL_Scancode` (physical key position on the keyboard) and `SDL_Keycode` (logical character after OS layout mapping).
- **Decision**: Use `SDL_Scancode` for key mapping.
- **Alternatives considered**: `SDL_Keycode` -- would respect the user's OS keyboard layout, so "Q" on an AZERTY keyboard (physical A key) would map correctly. However, PIU keyboard mapping is about physical hand position, not letter labels. A player using an AZERTY keyboard would expect the QWEASDZXC physical positions, not the letters.
- **Consequences**: The default keymap works correctly on any keyboard layout. Users with non-QWERTY layouts may need to mentally translate the physical positions, but the actual finger positions are the same. If a user explicitly wants layout-aware mapping, Phase 3 (keymap persistence) allows custom remapping.

### ADR-5: InputSystem Takes Ownership of a Single Driver (Phase 1)

- **Context**: Phase 6 (US-INP-081) requires multiple simultaneous drivers. Phase 1 only has keyboard.
- **Decision**: InputSystem owns `std::unique_ptr<InputDriver>` (single driver) for Phase 1.
- **Alternatives considered**: (a) `std::vector<std::unique_ptr<InputDriver>>` from the start -- adds complexity that Phase 1 doesn't need, and the multi-driver merge strategy (OR vs first-wins) isn't specified until Phase 6. (b) No InputSystem, just a driver injected into Engine directly -- loses the edge-detection centralization benefit.
- **Consequences**: Phase 6 changes `unique_ptr<InputDriver>` to `vector<unique_ptr<InputDriver>>` in InputSystem. The `poll()` method changes from `driver_->poll_held()` to a loop that OR-merges all drivers. The public API (`snapshot()`) does not change. Scenes are unaffected.

## Edge Detection Algorithm

The core algorithm, implemented in `InputSystem::poll()`:

```
Input:
  previous_held_  : uint32_t  (held mask from the previous tick)
  current_held    : uint32_t  (held mask from driver_->poll_held())

Output:
  pressed  = current_held & ~previous_held_    // bits that are 1 now but were 0 before
  released = ~current_held & previous_held_    // bits that are 0 now but were 1 before

State update:
  previous_held_ = current_held
```

Truth table for a single bit position:

| previous | current | pressed | released | held |
|----------|---------|---------|----------|------|
| 0        | 0       | 0       | 0        | 0    |
| 0        | 1       | 1       | 0        | 1    |
| 1        | 0       | 0       | 1        | 0    |
| 1        | 1       | 0       | 0        | 1    |

This is two bitwise operations per tick, constant time regardless of how many inputs are active. It operates on the entire bitmask simultaneously, so 10 simultaneous key presses are detected in the same two operations as a single key press.

## Default Keymap (US-INP-022)

The `KeyboardDriver::default_keymap()` factory method returns:

| SDL Scancode | PadInput | Physical Key (QWERTY) | Panel Position |
|---|---|---|---|
| `SDL_SCANCODE_Q` | `P1_DOWN_LEFT` | Q | Bottom-left |
| `SDL_SCANCODE_W` | `P1_UP_LEFT` | W | Top-left |
| `SDL_SCANCODE_E` | `P1_CENTER` | E | Center |
| `SDL_SCANCODE_A` | `P1_UP_RIGHT` | A | Top-right |
| `SDL_SCANCODE_S` | `P1_DOWN_RIGHT` | S | Bottom-right |
| `SDL_SCANCODE_RETURN` | `START` | Enter | Start |
| `SDL_SCANCODE_ESCAPE` | `BACK` | Escape | Back |
| `SDL_SCANCODE_SPACE` | `SELECT` | Space | Select |

Note: The story specifies Q=DL, W=UL, E=Center, A=UR, S=DR. This maps the X-pattern of a PIU pad to the QWEASD cluster on the left hand. No COIN mapping in Phase 1 (COIN is for arcade cabinets, Phase 8). No P2 mappings in Phase 1 (deferred to Phase 5, US-INP-062).

## Engine Integration

### How Engine Calls InputSystem Per Tick

TD-ENG-001 defines `Engine::update(double dt)` as the fixed-step logic call. The InputSystem integration point is at the beginning of this method:

```cpp
void Engine::update(double dt) {
    // 1. Poll input FIRST — all subsystems see the same snapshot this tick
    if (input_system_) {
        input_system_->poll(tick_count_);
    }

    // 2. Update active scene (future — scene receives snapshot via Engine reference)
    // if (scene_stack_ && !scene_stack_->empty()) {
    //     scene_stack_->top().update(dt);
    // }
}
```

The `if (input_system_)` null check allows Engine to run without an input system (e.g., during Engine unit tests from TD-ENG-001 that don't need input). When InputSystem is present, it is always polled before anything else in the update path.

### How Scenes Access Input

Scenes access input through the Engine reference they already receive (per TD-ENG-001's scene integration plan):

```cpp
// In a scene's update method (future):
void GameplayScene::update(double dt) {
    const auto& input = engine_.get_input_system().snapshot();
    if (input.is_pressed(PadInput::P1_CENTER)) {
        // Handle center panel press
    }
}
```

### Constructor Wiring

```cpp
// In Engine constructor (production):
Engine::Engine(const EngineConfig& config)
    : clock_(std::make_unique<Clock>()),
      renderer_(/* ... */),
      input_system_(std::make_unique<InputSystem>(
          std::make_unique<KeyboardDriver>())) {
    // ...
}

// In Engine constructor (test):
Engine::Engine(const EngineConfig& config,
               std::unique_ptr<Clock> clock,
               std::unique_ptr<InputSystem> input_system)
    : clock_(std::move(clock)),
      renderer_(/* ... */),
      input_system_(std::move(input_system)) {
    // ...
}
```

## Dependency Injection for Testing

### Testing Without SDL Events

Every component in the input system is testable without SDL:

**1. PadInput and InputSnapshot**: Pure value types. No SDL dependency. Test directly.

**2. InputSystem with Mock Driver**:
```cpp
class MockDriver : public InputDriver {
public:
    uint32_t held = 0;  // test sets this before each poll
    uint32_t poll_held() override { return held; }
    std::string device_name() const override { return "MockDriver"; }
};

// Test edge detection:
auto mock = std::make_unique<MockDriver>();
auto* mock_ptr = mock.get();  // keep non-owning ref
InputSystem sys(std::move(mock));

mock_ptr->held = static_cast<uint32_t>(PadInput::P1_CENTER);
sys.poll(1);
EXPECT_TRUE(sys.snapshot().is_pressed(PadInput::P1_CENTER));
EXPECT_TRUE(sys.snapshot().is_held(PadInput::P1_CENTER));

sys.poll(2);  // same held state
EXPECT_FALSE(sys.snapshot().is_pressed(PadInput::P1_CENTER));  // not newly pressed
EXPECT_TRUE(sys.snapshot().is_held(PadInput::P1_CENTER));       // still held
```

**3. KeyboardDriver with Injectable State**:
```cpp
bool fake_keys[SDL_SCANCODE_COUNT] = {};

KeyboardDriver driver(
    KeyboardDriver::default_keymap(),
    [&](int* count) -> const bool* {
        if (count) *count = SDL_SCANCODE_COUNT;
        return fake_keys;
    }
);

// Simulate Q key press:
fake_keys[SDL_SCANCODE_Q] = true;
uint32_t held = driver.poll_held();
EXPECT_EQ(held, static_cast<uint32_t>(PadInput::P1_DOWN_LEFT));
```

**4. Engine with Injected InputSystem**:

The Engine's InputSystem slot can be set via the injectable constructor. For tests that only need to verify the poll-before-update ordering, a mock InputSystem or a real InputSystem with a MockDriver can be injected alongside the fake Clock from TD-ENG-001.

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| SDL_GetKeyboardState not updated when Engine::process_events() changes in future | High | Low | Document that process_events() must call SDL_PollEvent (which calls SDL_PumpEvents) before update(). Add a comment in Engine::run_loop_body(). |
| Sub-frame press-release lost at 60 Hz polling | Low | Low | Physically implausible for dance input (50-100ms contact time). Documented as known limitation. Phase 6 HID driver polls at higher rate. |
| KeyMapping vector linear scan becomes slow if keymap grows very large | Low | Very Low | Phase 1 has 8-12 entries. Even Phase 5 (with P2) has ~20 entries. Switch to flat_map if profiling ever shows this as a hotspot. |
| SDL_Scancode enum values change between SDL3 releases | Med | Low | SDL scancodes are based on USB HID usage table, which is standardized. Changes are extremely rare. Phase 3 persistence will store scancode names (strings), not numeric values. |
| Engine null-checks InputSystem every tick | Low | None | Branch predictor handles this trivially. The check allows Engine tests from TD-ENG-001 to continue working without modification. |

## Testing Strategy

### Unit Tests (`test/test_input.cpp`) -- Pure Logic, No SDL

All tests use mock drivers and injectable keyboard state. No SDL initialization required.

**PadInput Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `AllValuesUniquePowerOfTwo` | Each PadInput value is a distinct power of 2 | US-INP-001 SC3 |
| `AllGameplayInputsExist` | 10 panel values exist (P1 and P2, 5 each) | US-INP-001 SC1 |
| `AllMenuInputsExist` | START, BACK, SELECT, COIN exist | US-INP-001 SC2 |
| `BitwiseOrCombines` | `P1_CENTER | P1_DOWN_LEFT` produces combined mask | US-INP-001 SC3 |
| `StringRoundTrip` | pad_input_to_string and pad_input_from_string are inverse | -- |

**InputSnapshot Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `DefaultSnapshotEmpty` | Default-constructed snapshot has no held/pressed/released | US-INP-002 SC1 |
| `IsHeldReturnsTrue` | Snapshot constructed with P1_CENTER held returns true for is_held | US-INP-002 SC1 |
| `IsHeldReturnsFalseForOthers` | Querying unheld input returns false | US-INP-002 SC1 |
| `IsPressedReturnsTrue` | Snapshot with P1_CENTER pressed returns true | US-INP-002 SC2 |
| `IsReleasedReturnsTrue` | Snapshot with P1_CENTER released returns true | US-INP-002 SC3 |
| `SnapshotIsEffectivelyImmutable` | No public method modifies snapshot state after construction | US-INP-002 SC4 |
| `TickNumberPreserved` | tick_number() returns the value passed at construction | US-INP-002 |

**InputSystem Edge Detection Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `InitialPressDetected` | First poll with held key → is_pressed true, is_held true | US-INP-012 SC1 |
| `ContinuedHoldNoPress` | Second poll same key → is_pressed false, is_held true | US-INP-012 SC2 |
| `ReleaseDetected` | Key released → is_released true, is_held false | US-INP-012 SC3 |
| `PollCountMatchesTickCount` | 120 polls produce 120 snapshots with sequential tick numbers | US-INP-011 SC1 |
| `AlternatingPressRelease120Ticks` | 120 ticks alternating on/off → 60 presses, 60 releases | US-INP-013 SC1 |
| `Simultaneous5PanelPress` | All 5 P1 panels pressed → all report is_pressed | US-INP-013 SC2 |
| `Simultaneous10PanelPress` | All 10 panels pressed → all report is_pressed | US-INP-013 SC3 |
| `MultipleSequentialEdges` | Press A, then press B next tick → A held+not pressed, B pressed | -- |
| `EmptySnapshotWhenNothingHeld` | No keys held → snapshot.empty() is true | -- |

**KeyboardDriver Tests**:

| Test | What It Verifies | Story |
|------|-----------------|-------|
| `DefaultKeymapQMapsToP1DL` | Q scancode → P1_DOWN_LEFT | US-INP-022 SC1 |
| `DefaultKeymapWMapsToP1UL` | W scancode → P1_UP_LEFT | US-INP-022 SC1 |
| `DefaultKeymapEMapsToP1Center` | E scancode → P1_CENTER | US-INP-022 SC1 |
| `DefaultKeymapAMapsToP1UR` | A scancode → P1_UP_RIGHT | US-INP-022 SC1 |
| `DefaultKeymapSMapsToP1DR` | S scancode → P1_DOWN_RIGHT | US-INP-022 SC1 |
| `DefaultKeymapEnterMapsToStart` | Enter → START | US-INP-022 SC2 |
| `DefaultKeymapEscapeMapsToBack` | Escape → BACK | US-INP-022 SC2 |
| `DefaultKeymapSpaceMapsToSelect` | Space → SELECT | US-INP-022 SC2 |
| `UnmappedKeyIgnored` | Pressing unmapped key produces 0 held mask | US-INP-021 SC2 |
| `MultipleKeysSimultaneous` | Q and W pressed → both P1_DL and P1_UL in mask | US-INP-021 SC3 |
| `All10PanelKeysSimultaneous` | All mapped keys pressed → all bits set in mask | US-INP-024 SC1 |
| `CustomKeymapOverridesDefault` | Custom keymap maps Z to P1_CENTER → Z press produces P1_CENTER | US-INP-021 SC1 |
| `DeviceNameReturnsKeyboard` | device_name() returns "Keyboard" | US-INP-003 SC3 |
| `SetKeymapReplacesExisting` | set_keymap() with new map → old mappings gone | US-INP-021 |

### Integration Tests (SDL-dependent)

These would be added to `test_integration.cpp` or a new `test_input_integration.cpp` once the Engine with InputSystem is running. They require an SDL context:

| Test | What It Verifies |
|------|-----------------|
| `KeyboardDriverWithRealSDL` | Default-constructed KeyboardDriver produces valid held masks |
| `EngineWithInputSystemRuns` | Engine with real KeyboardDriver starts and stops cleanly |
| `PollOccursBeforeUpdate` | Mock scene verifies input poll timestamp < scene update timestamp |

### What Is NOT Tested in Phase 1

- Multi-driver merging (Phase 6, US-INP-081)
- Per-player snapshots (Phase 5, US-INP-061)
- Keymap persistence to JSON (Phase 3, US-INP-023)
- HidPadDriver (Phase 6)
- ArcadeIODriver (Phase 8)
- Sub-frame press-release via keyboard (accepted limitation, see Data Flow section)

---

*Generated from stories in docs/stories/02-input-system.md (Phase 1 subset)*
*Last updated: 2026-04-28*
