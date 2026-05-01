# Implementation Plan: IP-SCN-004
## Story: US-SCN-004 - Title Scene with Attract Mode Loop

### Overview
Create TitleScene that loops a BGA animation and transitions on input.

### References
- **Story**: US-SCN-004 in docs/stories/07-screen-flow.md
- **Dependencies**: US-SCN-001 (Scene stack), BGA system, InputSystem

---

## Implementation Steps

### Step 1: Create TitleScene class
**Commit message**: `feat(scene): add TitleScene with BGA attract loop`

**Changes**:
- Create `src/openitup/scene/title_scene.h`
  - Class `TitleScene` implementing Scene interface
  - Constructor: `TitleScene(Renderer* renderer, InputSystem* input, const std::filesystem::path& attract_bga_path)`
  - `void update(double dt)` override — increment tick, check input, handle timeout
  - `void render(double alpha)` override — render BGA at current tick
  - Private: `std::unique_ptr<BgaAnimation> bga_`, `TextureCache texture_cache_`, `float current_tick_`, `double inactivity_timer_`
  - Constants: `INACTIVITY_TIMEOUT = 30.0` seconds, `BGA_TICK_RATE = 60.0`

- Create `src/openitup/scene/title_scene.cpp`
  - Constructor: load BGA, initialize texture cache, set tick=0, timer=0
  - `update()`: 
    - Increment tick by dt * BGA_TICK_RATE
    - If tick > bga max tick, reset to 0 (loop)
    - Check input for "start" or "coin" — if pressed, signal transition
    - Increment inactivity timer — if > 30s, signal timeout
  - `render()`: call `bga_->render()` at current_tick_

**Tests**:
- Unit test: BGA loops when reaching end tick
- Unit test: inactivity timer triggers at 30s

**Build verification**: Compile, tests pass.

---

### Step 2: Integrate TitleScene into Engine
**Commit message**: `feat(core): add TitleScene to engine scene stack`

**Changes**:
- Modify `src/openitup/core/engine.cpp`
  - At startup, after MinimalGameplayScene (or instead of it):
    - Push TitleScene onto scene_stack
    - For now, hardcode a test BGA path or use a placeholder

- Add CMake entry for TitleScene source files

**Tests**:
- Manual: run engine, observe title BGA looping
- Verify AC: BGA loops, input transitions (stubbed for now)

**Build verification**: Run engine, observe title scene.

---

### Step 3: Handle scene transitions (deferred or minimal stub)
**Commit message**: `feat(scene): stub scene transition handling in TitleScene`

**Changes**:
- Add scene transition mechanism:
  - TitleScene signals "transition_requested_" flag
  - Engine checks flag after update, replaces scene
  - For now, transition goes to MinimalGameplayScene (placeholder)

**Tests**:
- Manual: press start in title scene, observe transition

**Build verification**: Run, verify transition works.

---

## Acceptance Mapping

| Scenario | Verified By |
|----------|-------------|
| 1. BGA loops continuously | Step 1 (tick reset logic) |
| 2. Start input proceeds | Step 3 (transition on input) |
| 3. Inactivity timeout | Step 1 (timer logic), Step 3 (transition) |
| 4. Audio plays (optional) | DEFERRED — story says optional, skip for Phase 2 |
| 5. Input resets timer | Step 1 (reset timer on input) |

---

## Risk Assessment

**MEDIUM RISK**
- Scene transition API not fully defined yet
- BGA path hardcoded for now (no asset directory yet)
- Placeholder transitions (no ModeSelectScene yet)

---

## Estimated Effort

**3 story points** — simple scene, existing infrastructure.
