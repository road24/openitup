# IP-INP-001: Input System — Abstraction, Edge Detection, and Keyboard Driver Implementation Plan

**Design**: TD-INP-001
**Stories**: US-INP-001, US-INP-002, US-INP-003, US-INP-011, US-INP-012, US-INP-013, US-INP-021, US-INP-022, US-INP-024
**Total Steps**: 7
**Estimated Total**: ~3.5 hours
**Author**: technical-lead agent
**Status**: Draft
**Prerequisite**: IP-ENG-001 must be complete (Engine and Clock exist)

## Step 1: Create PadInput Enum and String Conversion

**Files**:
- Create `src/openitup/input/pad_input.h` — PadInput enum class, bitwise operators, ALL_PAD_INPUTS array, string conversion declarations
- Create `src/openitup/input/pad_input.cpp` — String conversion implementations (pad_input_to_string, pad_input_from_string)
- Modify `CMakeLists.txt` — Add `src/openitup/input/pad_input.cpp` to `openitup_engine` library sources

**What to implement**:

Create the foundational PadInput enum following the bitmask design from TD-INP-001. Each value is a power of two enabling bitwise OR combination into a uint32_t.

Key details:
- `enum class PadInput : uint32_t` for type safety with explicit casting operators
- 14 values: P1_DOWN_LEFT (0x0001) through COIN (0x2000)
- P1 panels occupy bits 0-4, P2 panels occupy bits 5-9, menu actions occupy bits 10-13
- `inline constexpr PadInput ALL_PAD_INPUTS[]` array enables iteration without reflection
- Bitwise OR operators: `operator|(PadInput, PadInput)` and `operator|(uint32_t, PadInput)`
- String conversion for logging: map each enum value to its name (e.g., "P1_CENTER")

Implementation follows the BlendEffect string conversion pattern in `src/openitup/bga/keyframe.h`.

**Tests**:
- Create `test/test_input.cpp` — Pure unit tests, no SDL dependencies
- Add to CMakeLists.txt in `openitup_tests`

Test cases (US-INP-001):
- `AllValuesUniquePowerOfTwo` — Each PadInput value is a distinct power of 2
- `AllGameplayInputsExist` — 10 panel values exist (P1 and P2, 5 each)
- `AllMenuInputsExist` — START, BACK, SELECT, COIN exist
- `BitwiseOrCombinesTwoInputs` — P1_CENTER | P1_DOWN_LEFT produces both bits set
- `BitwiseOrCombinesMaskAndInput` — uint32_t mask | PadInput works
- `StringRoundTrip` — pad_input_to_string and pad_input_from_string are inverse
- `InvalidStringReturnsP1DownLeft` — Unknown string returns first enum value (defensive default)

**Definition of done**:
- [ ] `cmake --build build -j$(nproc)` succeeds
- [ ] `cd build && ctest --output-on-failure -R Input` passes all PadInput tests
- [ ] No SDL dependencies in test (pure enum and string logic)

**Expected commit message**:
`feat(input): add PadInput enum with bitwise operators and string conversion`

**Estimated time**: ~30 minutes

---

## Step 2: Create InputSnapshot Value Type

**Files**:
- Create `src/openitup/input/input_snapshot.h` — InputSnapshot class declaration
- Create `src/openitup/input/input_snapshot.cpp` — InputSnapshot method implementations
- Modify `CMakeLists.txt` — Add `src/openitup/input/input_snapshot.cpp` to `openitup_engine`
- Modify `test/test_input.cpp` — Add InputSnapshot unit tests

**What to implement**:

Create the immutable value type that holds per-tick input state. Three bitmasks (held, pressed, released) plus a tick_number for debugging.

Key details from TD-INP-001:
- Constructor: `InputSnapshot(uint32_t held, uint32_t pressed, uint32_t released, uint64_t tick_number)`
- Default constructor produces empty snapshot (all zeros)
- Query methods: `is_held(PadInput)`, `is_pressed(PadInput)`, `is_released(PadInput)` — check if the specific bit is set
- Bitmask accessors: `held_mask()`, `pressed_mask()`, `released_mask()` for bulk operations
- Utility: `empty()` returns true if all three masks are zero
- `tick_number()` returns the engine tick this snapshot was captured at

The snapshot is effectively immutable — all fields are private, no setters. Do NOT use `const` member variables (breaks move assignment).

**Tests**:

Add to `test/test_input.cpp` (US-INP-002):
- `DefaultSnapshotEmpty` — Default-constructed snapshot returns empty() == true
- `IsHeldReturnsTrue` — Snapshot with P1_CENTER bit set in held mask returns true for is_held(P1_CENTER)
- `IsHeldReturnsFalseForOthers` — Querying unheld input returns false
- `IsPressedReturnsTrue` — Snapshot with P1_CENTER in pressed mask returns true
- `IsReleasedReturnsTrue` — Snapshot with P1_CENTER in released mask returns true
- `TickNumberPreserved` — tick_number() returns the value passed at construction
- `HeldMaskAccessor` — held_mask() returns the raw bitmask
- `MultipleInputsHeld` — Snapshot with multiple bits set queries correctly
- `EmptyReturnsFalseWhenHeld` — empty() is false if any mask is non-zero

**Definition of done**:
- [ ] `cmake --build build -j$(nproc)` succeeds
- [ ] `cd build && ctest --output-on-failure -R Input` passes all InputSnapshot tests
- [ ] No setters or mutable state (effectively immutable design)

**Expected commit message**:
`feat(input): add InputSnapshot immutable value type for per-tick state`

**Estimated time**: ~25 minutes

---

## Step 3: Create InputDriver Interface and InputSystem

**Files**:
- Create `src/openitup/input/input_driver.h` — InputDriver abstract interface (header-only)
- Create `src/openitup/input/input_system.h` — InputSystem class declaration
- Create `src/openitup/input/input_system.cpp` — InputSystem implementation (poll, edge detection)
- Modify `CMakeLists.txt` — Add `src/openitup/input/input_system.cpp` to `openitup_engine`
- Modify `test/test_input.cpp` — Add InputSystem and edge detection tests with MockDriver

**What to implement**:

**InputDriver** (header-only interface):
- Pure virtual `poll_held()` returns `uint32_t` held bitmask (NOT an InputSnapshot)
- Pure virtual `device_name()` returns `std::string`
- Virtual destructor
- No player_index parameter in Phase 1 (deferred to Phase 5)

**InputSystem** (coordinator owned by Engine):
- Constructor accepts `std::unique_ptr<InputDriver>`
- `poll(uint64_t tick_number)` — Calls driver->poll_held(), performs edge detection, stores snapshot
- Edge detection algorithm: `pressed = held & ~previous_held_`, `released = ~held & previous_held_`
- `snapshot()` returns const reference to current InputSnapshot
- `previous_snapshot()` returns const reference to previous InputSnapshot (for debugging)
- `driver()` non-owning accessor to underlying driver
- Member: `previous_held_` as `uint32_t` for edge detection (separate from previous_ snapshot)

Key decisions from TD-INP-001 ADR-1:
- Drivers return raw held masks, NOT snapshots
- InputSystem centralizes edge detection logic
- This design enables Phase 6 multi-driver merging (OR-merge held masks before edge detection)

**Tests**:

Create MockDriver in `test/test_input.cpp`:
```cpp
class MockDriver : public InputDriver {
public:
    uint32_t held = 0;  // test sets this before each poll
    uint32_t poll_held() override { return held; }
    std::string device_name() const override { return "MockDriver"; }
};
```

Add tests (US-INP-012, US-INP-013):
- `InitialPressDetected` — First poll with held key produces is_pressed true, is_held true
- `ContinuedHoldNoPress` — Second poll same key produces is_pressed false, is_held true
- `ReleaseDetected` — Key released produces is_released true, is_held false
- `PollCountMatchesTickCount` — 120 polls produce 120 snapshots with sequential tick numbers
- `AlternatingPressRelease` — 60 ticks on/off produces 30 presses, 30 releases
- `Simultaneous5PanelPress` — All P1 panel bits set, all report is_pressed
- `Simultaneous10PanelPress` — All 10 panel bits set, all report is_pressed
- `MultipleSequentialEdges` — Press A tick 1, press B tick 2: A held+not pressed, B pressed
- `PressAndReleaseInSuccessiveTicks` — Press tick 1, release tick 2, verify both edges
- `EmptySnapshotWhenNothingHeld` — No keys held produces snapshot.empty() == true

**Definition of done**:
- [ ] `cmake --build build -j$(nproc)` succeeds
- [ ] `cd build && ctest --output-on-failure -R Input` passes all InputSystem tests
- [ ] Edge detection logic matches TD-INP-001 algorithm exactly

**Expected commit message**:
`feat(input): add InputDriver interface and InputSystem with edge detection`

**Estimated time**: ~35 minutes

---

## Step 4: Create KeyboardDriver with Injectable State

**Files**:
- Create `src/openitup/input/keyboard_driver.h` — KeyboardDriver class declaration
- Create `src/openitup/input/keyboard_driver.cpp` — KeyboardDriver implementation (poll_held, default_keymap)
- Modify `CMakeLists.txt` — Add `src/openitup/input/keyboard_driver.cpp` to `openitup_engine`
- Modify `test/test_input.cpp` — Add KeyboardDriver tests with fake keyboard state

**What to implement**:

The Phase 1 input backend that reads SDL keyboard state through an injectable function.

Key details from TD-INP-001:
- `KeyboardStateFn = std::function<const bool*(int*)>` — Injectable keyboard state source
- `struct KeyMapping { SDL_Scancode scancode; PadInput input; }`
- `std::vector<KeyMapping> keymap_` — Linear vector (12 entries in Phase 1, fast enough)
- Default constructor wraps SDL_GetKeyboardState, calls default_keymap()
- Injectable constructor accepts custom keymap and KeyboardStateFn for testing
- `poll_held()` iterates keymap, checks state_fn()[scancode], OR-accumulates bits
- `device_name()` returns "Keyboard"
- `keymap()` accessor returns const reference
- `set_keymap()` replaces keymap at runtime (for Phase 3 persistence)
- `static default_keymap()` factory method returns QWEASD + Enter/Esc/Space

Use SDL_Scancode (physical position), NOT SDL_Keycode (logical character) per TD-INP-001 ADR-4.

Default keymap (US-INP-022):
- Q → P1_DOWN_LEFT
- W → P1_UP_LEFT
- E → P1_CENTER
- A → P1_UP_RIGHT
- S → P1_DOWN_RIGHT
- Enter → START
- Escape → BACK
- Space → SELECT
- (No P2 or COIN in Phase 1)

**Tests**:

Add to `test/test_input.cpp`:

Create fake keyboard state array:
```cpp
bool fake_keys[512] = {};  // SDL_SCANCODE_COUNT is ~512
KeyboardDriver driver(
    KeyboardDriver::default_keymap(),
    [&](int* count) -> const bool* {
        if (count) *count = 512;
        return fake_keys;
    }
);
```

Test cases (US-INP-021, US-INP-022, US-INP-024):
- `DefaultKeymapQMapsToP1DL` — Set fake_keys[SDL_SCANCODE_Q], verify poll_held() returns P1_DOWN_LEFT bit
- `DefaultKeymapWMapsToP1UL` — Test W → P1_UP_LEFT
- `DefaultKeymapEMapsToP1Center` — Test E → P1_CENTER
- `DefaultKeymapAMapsToP1UR` — Test A → P1_UP_RIGHT
- `DefaultKeymapSMapsToP1DR` — Test S → P1_DOWN_RIGHT
- `DefaultKeymapEnterMapsToStart` — Test Enter → START
- `DefaultKeymapEscapeMapsToBack` — Test Escape → BACK
- `DefaultKeymapSpaceMapsToSelect` — Test Space → SELECT
- `UnmappedKeyIgnored` — Press unmapped key (e.g., Z), verify held == 0
- `MultipleKeysSimultaneous` — Press Q and W, verify both bits set
- `All5PanelKeysSimultaneous` — Press QWEAS, verify all P1 panel bits set
- `CustomKeymapOverridesDefault` — Custom keymap maps Z to P1_CENTER, verify Z press produces P1_CENTER
- `DeviceNameReturnsKeyboard` — device_name() == "Keyboard"
- `SetKeymapReplacesExisting` — set_keymap() with new map, verify old mappings gone

**Definition of done**:
- [ ] `cmake --build build -j$(nproc)` succeeds
- [ ] `cd build && ctest --output-on-failure -R Input` passes all KeyboardDriver tests
- [ ] All tests use injectable fake keyboard state (no real SDL input)

**Expected commit message**:
`feat(input): add KeyboardDriver with injectable state and default QWEASD keymap`

**Estimated time**: ~35 minutes

---

## Step 5: Integrate InputSystem into Engine

**Files**:
- Modify `src/openitup/core/engine.h` — Add InputSystem member, constructor parameter, accessor
- Modify `src/openitup/core/engine.cpp` — Wire input_system_->poll() into update()
- Modify `test/test_input.cpp` — Add integration test with real Engine (SDL-dependent)

**What to implement**:

Wire InputSystem into the Engine's fixed-step loop following TD-INP-001 integration plan.

Changes to `engine.h`:
- Add member: `std::unique_ptr<InputSystem> input_system_` (declared after renderer_, before future scene_stack_)
- Modify constructor: Accept optional `std::unique_ptr<InputSystem>` for injection (testing)
- Add accessors: `InputSystem& get_input_system()` and const overload

Changes to `engine.cpp`:
- Production constructor: Create InputSystem with KeyboardDriver if not injected
  ```cpp
  if (!input_system_) {
      input_system_ = std::make_unique<InputSystem>(
          std::make_unique<KeyboardDriver>());
  }
  ```
- Modify `update(double dt)`: Call `input_system_->poll(tick_count_)` as first operation, before scene update placeholder
- `process_events()` continues to call SDL_PollEvent for SDL_EVENT_QUIT — input keys are consumed via SDL_GetKeyboardState (which reflects state after SDL_PumpEvents)

The null check `if (input_system_)` allows Engine tests from IP-ENG-001 to continue working without modification.

**Tests**:

Add to existing `test/test_integration.cpp` (SDL-dependent):
- `EngineWithInputSystemStarts` — Construct Engine with default config (creates real KeyboardDriver), immediately request_quit(), verify run() returns 0
- `InputSystemOwnedByEngine` — Construct Engine, verify get_input_system() returns valid reference
- `PollOccursBeforeUpdate` — Inject MockDriver with tick-counting, verify poll happens once per update call

Add to `test/test_input.cpp` (unit test with mock):
- `EnginePollsInputSystemPerTick` — Inject InputSystem with MockDriver, run 10 ticks, verify 10 polls occurred

**Definition of done**:
- [ ] `cmake --build build -j$(nproc)` succeeds
- [ ] `cd build && ctest --output-on-failure` passes all tests
- [ ] Engine with KeyboardDriver runs without crashes
- [ ] input_system_->poll() is first operation in update()

**Expected commit message**:
`feat(input): integrate InputSystem into Engine fixed-step loop`

**Estimated time**: ~30 minutes

---

## Step 6: Add End-to-End Input Integration Test

**Files**:
- Modify `test/test_integration.cpp` — Add full input flow test with Engine, InputSystem, KeyboardDriver

**What to implement**:

Create a comprehensive test that verifies the entire input pipeline: fake keyboard state → KeyboardDriver → InputSystem → Engine → snapshot queries.

This test proves US-INP-011 (poll once per tick, before scene update) and US-INP-012 (edge detection) in the real Engine environment.

Test strategy:
1. Create fake keyboard state array
2. Create KeyboardDriver with injectable state
3. Create InputSystem with that driver
4. Inject into Engine
5. Simulate key press across multiple frames
6. Verify snapshots at each tick

**Tests**:

Add to `test/test_integration.cpp`:

`InputFlowEndToEnd`:
- Setup: fake_keys array, KeyboardDriver with injected state, InputSystem, Engine with injected InputSystem
- Tick 1: No keys pressed → snapshot empty
- Tick 2: Press Q (P1_DOWN_LEFT) → is_pressed true, is_held true
- Tick 3: Hold Q → is_pressed false (not newly pressed), is_held true
- Tick 4: Release Q → is_released true, is_held false
- Tick 5: Press W and E simultaneously → both is_pressed true
- Verify tick_number increments correctly in each snapshot

`InputRolloverTest`:
- Setup: same as above
- Press all 5 P1 panel keys simultaneously (QWEAS)
- Verify all 5 inputs report is_pressed and is_held
- Validates US-INP-024 (10-key rollover, tested with 5)

**Definition of done**:
- [ ] `cmake --build build -j$(nproc)` succeeds
- [ ] `cd build && ctest --output-on-failure -R Integration` passes
- [ ] End-to-end test covers press, hold, release, multi-input scenarios

**Expected commit message**:
`test(input): add end-to-end integration test for input pipeline`

**Estimated time**: ~30 minutes

---

## Step 7: Manual Verification and Documentation

**Files**:
- Modify `src/openitup/main.cpp` — Add logging of input snapshot for manual testing (temporary, removed before commit)

**What to implement**:

Add temporary logging to verify keyboard input works in the running application. This is not committed — it's for manual verification only.

Temporary addition to `main.cpp`:
```cpp
// In Engine::update() or a test scene:
const auto& input = input_system_->snapshot();
if (!input.empty()) {
    // Log which inputs are pressed this tick
    for (const auto& pad_input : ALL_PAD_INPUTS) {
        if (input.is_pressed(pad_input)) {
            spdlog::info("Pressed: {}", pad_input_to_string(pad_input));
        }
    }
}
```

Manual tests:
1. Build and run `./build/openitup`
2. Press Q, W, E, A, S — verify spdlog INFO messages appear
3. Press Escape — verify window closes
4. Hold Q, verify only one "Pressed" message (not repeated)
5. Press multiple keys simultaneously, verify all appear

After verification, remove the logging code. Commit only the documentation update.

**Tests**:

Manual verification checklist:
- [ ] Q key triggers P1_DOWN_LEFT press
- [ ] W key triggers P1_UP_LEFT press
- [ ] E key triggers P1_CENTER press
- [ ] A key triggers P1_UP_RIGHT press
- [ ] S key triggers P1_DOWN_RIGHT press
- [ ] Escape key exits application
- [ ] Held key does not repeat press events
- [ ] Multiple keys pressed simultaneously all detected

**Definition of done**:
- [ ] Manual tests pass
- [ ] No crashes during keyboard input
- [ ] Edge detection works correctly (press once per key press, not repeated)
- [ ] Temporary logging code removed before commit

**Expected commit message**:
`docs(input): verify keyboard input works in running application`

**Estimated time**: ~15 minutes

---

## PR Strategy

- [ ] **Single PR recommended** — All 7 steps build on each other: PadInput → InputSnapshot → InputDriver/InputSystem → KeyboardDriver → Engine integration. Splitting would create unused components.
- [ ] **Review checkpoint**: After Step 4 (KeyboardDriver with tests), before Step 5 (Engine integration). This ensures the input abstraction layer is fully tested independently before touching Engine.
- [ ] **Manual testing**: Step 7 verification can be done by the reviewer — just run `./build/openitup` and press keys to verify input works.

Alternative: Split into two PRs:
1. Steps 1-4: Input abstraction layer (PadInput, InputSnapshot, InputDriver, InputSystem, KeyboardDriver) with unit tests
2. Steps 5-7: Engine integration and end-to-end tests

This allows reviewing the input system design independently before modifying Engine.

## Build Verification

After all steps complete:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DSDL_X11_XSCRNSAVER=OFF
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

All tests should pass. Run the main executable:

```bash
./build/openitup
# Window opens, black screen
# Press QWEAS keys — no visible response yet (scenes not implemented)
# Press Escape — window closes cleanly
```

Verify logs show clean startup and no input errors.

## Acceptance Verification

| Story ID | How to verify |
|----------|--------------|
| US-INP-001 | `test/test_input.cpp`: PadInput enum tests (AllValuesUniquePowerOfTwo, AllGameplayInputsExist, AllMenuInputsExist) |
| US-INP-002 | `test/test_input.cpp`: InputSnapshot tests (IsHeldReturnsTrue, IsPressedReturnsTrue, IsReleasedReturnsTrue) |
| US-INP-003 | `test/test_input.cpp`: MockDriver implements InputDriver interface, device_name() test |
| US-INP-011 | `test/test_integration.cpp`: InputFlowEndToEnd verifies poll once per tick |
| US-INP-012 | `test/test_input.cpp`: InitialPressDetected, ContinuedHoldNoPress, ReleaseDetected |
| US-INP-013 | `test/test_input.cpp`: Simultaneous5PanelPress, Simultaneous10PanelPress |
| US-INP-021 | `test/test_input.cpp`: CustomKeymapOverridesDefault, SetKeymapReplacesExisting |
| US-INP-022 | `test/test_input.cpp`: DefaultKeymap* tests (Q/W/E/A/S, Enter/Esc/Space) |
| US-INP-024 | `test/test_integration.cpp`: InputRolloverTest with 5+ simultaneous keys |

## Notes

**Prerequisite check**: Before starting Step 1, verify IP-ENG-001 is complete:
- `src/openitup/core/engine.h` exists
- `src/openitup/core/engine.cpp` exists
- `src/openitup/core/clock.h` exists
- Engine has `update(double dt)` method
- Engine has `tick_count()` accessor

**Why InputDriver returns held mask, not InputSnapshot**: Edge detection requires comparing current held state to previous held state. If each driver computed its own edges, merging multiple drivers (Phase 6) would double-count edges. Centralizing edge detection in InputSystem makes multi-driver merging straightforward: OR-merge held masks, then compute edges on the merged result.

**Why vector of KeyMapping over unordered_map**: The keymap has 12 entries in Phase 1. Linear scan of 12 entries (720 iterations/second at 60 Hz) is faster than hash lookup due to cache locality. The vector is also trivially serializable to JSON for Phase 3 persistence.

**Sub-frame press-release limitation**: If a key is pressed and released between two poll() calls (within 16.7ms at 60 Hz), the event is lost. This is acceptable because: (1) physically implausible for dance input (typical foot contact 50-100ms), (2) US-INP-024 specifies SDL_GetKeyboardState (state-based, not event-based), (3) Phase 6 HID driver polls at 1000 Hz where this is not an issue.

**Testing in headless environments**: Steps 5-7 require SDL and a display. CI systems should:
1. Use Xvfb (virtual framebuffer): `xvfb-run ctest`
2. Skip integration tests: `ctest -E Integration`
3. Use SDL offscreen rendering: `SDL_RENDER_DRIVER=software ctest`

**Phase 6 extension point**: When multi-driver support is added (US-INP-081), InputSystem will hold `std::vector<std::unique_ptr<InputDriver>>` instead of a single driver. The poll() method will OR-merge all drivers' held masks before edge detection. The public API (snapshot()) does not change — scenes are unaffected.

**Manual testing note**: Step 7 is manual verification only. No code is committed in this step — it's purely to confirm keyboard input works as expected in the running application. The temporary logging code should be removed before creating the commit.
