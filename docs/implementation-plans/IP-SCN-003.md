# Implementation Plan: IP-SCN-003

**Story**: US-SCN-003 - Boot Scene with Logo Display  
**Estimate**: 3 points  
**Dependencies**: US-SCN-001 (DONE), US-AST-022 (DONE)

## Context

Create BootScene that displays centered "openitup" text for 3 seconds, then replaces itself with TitleScene. This is the first concrete Scene subclass.

## Technical Design (Inline)

```cpp
class BootScene : public Scene {
public:
    BootScene(Renderer* renderer, TextRenderer* text_renderer, SceneStack* scene_stack);
    // Scene interface overrides
private:
    Renderer* renderer_;
    TextRenderer* text_;
    SceneStack* stack_;
    double elapsed_ = 0.0;
};
```

## Implementation Steps

### Step 1: Implement BootScene

**Files to create**:
- `src/openitup/scene/boot_scene.h`
- `src/openitup/scene/boot_scene.cpp`

**Implementation**:
- Constructor stores pointers
- `on_enter()`: logs "Boot scene entered"
- `update(dt)`: accumulates time, when elapsed >= 3.0s logs "Transitioning to Title" (actual transition deferred to US-SCN-004)
- `render()`: clears to black, draws "openitup" white text at (320, 240)
- Other lifecycle: empty implementations

**Tests**: Create `test/test_boot_scene.cpp` with:
- Timer reaches 3 seconds
- Render doesn't crash

**Commit**:
```
feat(scene): add BootScene with 3-second splash screen

Displays "openitup" centered for 3 seconds. Transition to
TitleScene will be wired in US-SCN-004.

Story: US-SCN-003

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

### Step 2: Add to build system

**Files**:
- `CMakeLists.txt`

**Changes**:
- Add `src/openitup/scene/boot_scene.cpp` to openitup_engine sources

**Commit**:
```
build: add BootScene to CMakeLists

Related: US-SCN-003

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

## AC Mapping

- AC1 (logo displays): text renders
- AC2 (init < 5s): no init, instant
- AC3 (progress): text only (stub)
- AC4 (transition): timer triggers (actual push happens in SCN-004)
- AC5 (error): deferred Phase 3
