# TD-SCN-002: Scene Stack Core Infrastructure

**Stories**: US-SCN-001, US-SCN-002
**Phase**: 2
**Author**: technical-architect agent (via delivery-coordinator)
**Status**: Draft

## Overview

This design introduces the **Scene Stack** — the foundational state management system for all game screens (boot, title, mode select, song select, gameplay, result, pause overlays, settings). The stack manages a vector of `Scene` objects with push, pop, and replace operations. It renders all scenes bottom-to-top (painter's algorithm) but only updates and routes input to the topmost scene. This enables modal overlays (pause menu, dialogs) without destroying the underlying state.

The Scene Stack replaces the Phase 1 `Engine::run_gameplay()` method with a proper screen flow architecture. Phase 1's `MinimalGameplayScene` can be refactored to implement the new `Scene` interface in a follow-up story.

## Architecture

### Component Diagram

```
Engine (TD-ENG-001)
  |  owns (unique_ptr)
  ├── Clock
  ├── Renderer
  ├── AudioSystem
  ├── InputSystem
  └── SceneStack (NEW)
       |
       |  owns (vector<unique_ptr<Scene>>)
       ├── Scene A (bottom)
       ├── Scene B
       └── Scene C (top) ← receives input, gets update() calls
       
SceneStack operations:
  - push(scene)    → C goes on top, B calls on_pause()
  - pop()          → C destroyed, B calls on_resume()
  - replace(scene) → C destroyed, D takes its place
  - update(dt)     → calls top scene's update(dt)
  - render()       → calls all scenes' render() bottom-to-top
  - handle_input() → calls top scene's handle_input()
```

### New Types

#### `Scene` (abstract base class, `src/openitup/scene/scene.h`)

The interface contract for all screens. Every scene implements these lifecycle hooks:

```cpp
// src/openitup/scene/scene.h
#pragma once

#include <openitup/input/input_snapshot.h>

namespace openitup {

class Scene {
public:
    virtual ~Scene() = default;

    // Lifecycle hooks
    virtual void on_enter() = 0;   // Called when scene becomes active (pushed or revealed)
    virtual void on_exit() = 0;    // Called before scene is destroyed (popped or replaced)
    virtual void on_pause() = 0;   // Called when another scene is pushed on top
    virtual void on_resume() = 0;  // Called when the scene above is popped

    // Per-frame operations (only called on the topmost scene)
    virtual void update(double dt) = 0;
    virtual void handle_input(const InputSnapshot& input) = 0;

    // Rendering (called on ALL scenes, bottom-to-top)
    virtual void render() = 0;

protected:
    Scene() = default;
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
};

} // namespace openitup
```

**Key design decisions**:

- Pure virtual base class — no default implementations. Every scene must explicitly handle all lifecycle events.
- `render()` takes no parameters. Scenes access the renderer via a reference passed in their constructor (from Engine).
- `update(dt)` uses variable `dt` (not fixed-step). The scene stack update happens once per logic tick with `dt = FIXED_STEP` in normal conditions, but the scene doesn't assume this.
- `handle_input()` receives the `InputSnapshot` produced by the input system. This is called before `update()`.

#### `SceneStack` (`src/openitup/scene/scene_stack.h`)

The stack manager. Owns all scenes via `std::unique_ptr`. Orchestrates lifecycle calls.

```cpp
// src/openitup/scene/scene_stack.h
#pragma once

#include <memory>
#include <vector>

#include <openitup/input/input_snapshot.h>
#include <openitup/scene/scene.h>

namespace openitup {

class SceneStack {
public:
    SceneStack();
    ~SceneStack();

    SceneStack(const SceneStack&) = delete;
    SceneStack& operator=(const SceneStack&) = delete;

    // Stack operations
    // push: new scene goes on top, previous top (if any) is paused
    void push(std::unique_ptr<Scene> scene);

    // pop: top scene is destroyed, new top (if any) is resumed
    // No-op if stack is empty.
    void pop();

    // replace: top scene is destroyed, new scene takes its place
    // If stack is empty, this is equivalent to push.
    void replace(std::unique_ptr<Scene> scene);

    // Frame operations
    // update: calls update(dt) on the topmost scene only
    void update(double dt);

    // handle_input: calls handle_input() on the topmost scene only
    void handle_input(const InputSnapshot& input);

    // render: calls render() on ALL scenes, bottom-to-top (painter's algorithm)
    void render();

    // State queries
    bool empty() const { return scenes_.empty(); }
    size_t size() const { return scenes_.size(); }

private:
    std::vector<std::unique_ptr<Scene>> scenes_;
};

} // namespace openitup
```

**Key design decisions**:

- `std::vector<std::unique_ptr<Scene>>` provides RAII ownership. Popping a scene immediately destroys it.
- Operations are safe on an empty stack (pop and replace are no-ops if empty).
- No scene pointer access — the stack owns scenes, external code cannot query "what's on the stack." Scenes communicate via engine-level state or event queues (future work).

### Modified Types

#### `Engine` (`src/openitup/core/engine.h`)

- Add member: `std::unique_ptr<SceneStack> scene_stack_;`
- Remove method: `int run_gameplay(...)` (Phase 1 only)
- Modify `run()`: Call `scene_stack_->update(FIXED_STEP)`, `scene_stack_->handle_input(...)`, `scene_stack_->render()` in the main loop
- Add method: `SceneStack* get_scene_stack()` — for tests and initial scene setup in `main.cpp`

#### `Engine::run()` implementation changes:

The fixed-step loop now delegates to the scene stack:

```cpp
// In Engine::run()
while (running_ && !scene_stack_->empty()) {
    process_events();

    double delta = clock_->tick();
    auto result = compute_fixed_steps(delta, accumulator_);
    accumulator_ = result.new_accumulator;
    render_alpha_ = result.alpha;

    if (result.spiral_guard_triggered) {
        spdlog::warn("Spiral-of-death guard triggered");
    }

    for (int i = 0; i < result.num_steps; i++) {
        try {
            if (input_system_) {
                input_system_->poll(tick_count_);
                scene_stack_->handle_input(input_system_->snapshot());
            }
            scene_stack_->update(FIXED_STEP);
        } catch (const std::exception& e) {
            spdlog::error("Exception in scene update: {}", e.what());
        }
        tick_count_++;
    }

    renderer_->begin_frame();
    try {
        scene_stack_->render();
    } catch (const std::exception& e) {
        spdlog::error("Exception in scene render: {}", e.what());
    }
    renderer_->end_frame();
}

spdlog::info("Scene stack empty or quit requested. Exiting.");
```

**Rationale**: The engine loop remains unchanged structurally. It just delegates to the scene stack instead of a single scene. The loop terminates when the scene stack is empty (player quit or all scenes popped) or `request_quit()` is called.

## File Plan

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/openitup/scene/scene.h` | Scene abstract base class |
| Create | `src/openitup/scene/scene_stack.h` | SceneStack class declaration |
| Create | `src/openitup/scene/scene_stack.cpp` | SceneStack implementation |
| Modify | `src/openitup/core/engine.h` | Add scene_stack_ member, add get_scene_stack() accessor |
| Modify | `src/openitup/core/engine.cpp` | Initialize scene_stack_, modify run() to use it |
| Modify | `CMakeLists.txt` | Add scene_stack.cpp to openitup_engine |
| Create | `test/test_scene_stack.cpp` | Unit tests for scene stack operations |
| Modify | `CMakeLists.txt` | Add test file to openitup_tests |

## Data Flow

### Scenario 1: Push Scene Creates Overlay (US-SCN-001 SC1)

```
Initial state: stack = [SceneA]

scene_stack_->push(std::make_unique<SceneB>())
  1. SceneA->on_pause() is called
  2. SceneB is moved into scenes_[1]
  3. SceneB->on_enter() is called
  
New state: stack = [SceneA, SceneB]

Next update():
  - Only SceneB->update(dt) is called
  - SceneA->update(dt) is NOT called

Next handle_input(input):
  - Only SceneB->handle_input(input) is called

Next render():
  - SceneA->render() is called first (background)
  - SceneB->render() is called second (overlay on top)
```

### Scenario 2: Pop Scene Returns to Previous (US-SCN-001 SC2)

```
Initial state: stack = [SceneA, SceneB]

scene_stack_->pop()
  1. SceneB->on_exit() is called
  2. SceneB's unique_ptr is destroyed (destructor runs)
  3. scenes_.pop_back() removes it from the vector
  4. SceneA->on_resume() is called
  
New state: stack = [SceneA]

Next update():
  - SceneA->update(dt) is called
  
Next handle_input(input):
  - SceneA->handle_input(input) is called
```

### Scenario 3: Replace Scene Transitions (US-SCN-001 SC3)

```
Initial state: stack = [SceneA]

scene_stack_->replace(std::make_unique<SceneB>())
  1. SceneA->on_exit() is called
  2. SceneA's unique_ptr is destroyed
  3. SceneB is moved into scenes_[0]
  4. SceneB->on_enter() is called
  5. No on_pause() or on_resume() calls (replacement, not overlay)
  
New state: stack = [SceneB]
```

### Scenario 4: Rendering Order is Bottom-to-Top (US-SCN-001 SC4)

```
State: stack = [SceneA, SceneB, SceneC]

scene_stack_->render()
  for (size_t i = 0; i < scenes_.size(); i++)
    scenes_[i]->render()
  
Render calls in order:
  1. SceneA->render()  // index 0
  2. SceneB->render()  // index 1
  3. SceneC->render()  // index 2
```

### Scenario 5: Only Topmost Scene Receives Updates (US-SCN-001 SC5)

```
State: stack = [SceneA, SceneB]

scene_stack_->update(dt)
  if (!scenes_.empty())
    scenes_.back()->update(dt)
  
Result: SceneB->update(dt) is called
        SceneA->update(dt) is NOT called
```

### Scenario 6: Empty Stack is Valid (US-SCN-001 SC6)

```
State: stack = []

scene_stack_->update(dt)
  if (!scenes_.empty()) ...  // guard passes, no-op

scene_stack_->render()
  for (size_t i = 0; i < 0; i++) ...  // loop never executes

scene_stack_->pop()
  if (scenes_.empty()) return;  // no-op

Result: No crashes, all operations complete immediately
```

## Dependencies

### Internal
- **InputSnapshot** (`src/openitup/input/input_snapshot.h`, TD-INP-001) — Scene interface receives input snapshots
- **Engine** (`src/openitup/core/engine.h`, TD-ENG-001) — Owns the scene stack
- **Renderer** (`src/openitup/gfx/renderer.h`) — Scenes access renderer for drawing (via constructor reference)

### External (new libraries)
None. Uses standard library `<vector>`, `<memory>`.

## Architectural Decisions

### ADR-1: Pure Virtual Scene Base Class

- **Context**: Should `Scene` provide default no-op implementations for lifecycle hooks, or force every scene to implement them?
- **Decision**: All methods are pure virtual (`= 0`). No default implementations.
- **Alternatives considered**: (a) Provide empty defaults for `on_pause()`/`on_resume()` since not all scenes need them — but this makes it easy to forget implementing critical hooks. (b) Use CRTP to avoid vtable overhead — premature optimization, and vtables are negligible compared to SDL rendering.
- **Consequences**: Every scene must explicitly implement all 7 methods. This increases boilerplate but ensures scenes don't accidentally skip lifecycle logic (e.g., forgetting to pause audio in `on_pause()`).

### ADR-2: Stack Owns Scenes via unique_ptr

- **Context**: Who owns scene objects? How long do they live?
- **Decision**: `SceneStack` owns scenes via `std::vector<std::unique_ptr<Scene>>`. Popping a scene immediately destroys it.
- **Alternatives considered**: (a) `shared_ptr` — no other system needs to own scenes. (b) Raw pointers with manual delete — violates RAII. (c) Value semantics (`vector<Scene>`) — Scene is abstract, can't be stored by value.
- **Consequences**: When a scene is popped, its destructor runs immediately. Scenes must clean up resources (stop audio, release textures) in their destructor or `on_exit()`. No dangling pointers possible.

### ADR-3: Render All Scenes, Update Only Top

- **Context**: When a pause overlay is pushed, should the gameplay scene below keep updating? Should it keep rendering?
- **Decision**: Render ALL scenes (painter's algorithm). Update/input only to the top scene.
- **Alternatives considered**: (a) Only render top scene — pause overlays would need to manually render the gameplay scene behind them (duplicates logic). (b) Update all scenes — breaks pause semantics (gameplay would continue while paused). (c) Flag-based per-scene control (render_below, update_below) — adds complexity for rare use cases.
- **Consequences**: Pause overlays get the frozen gameplay scene rendered automatically. Gameplay scenes must explicitly pause their timers/audio in `on_pause()` to avoid logic continuing during overlays. All scenes must be render-safe when paused (no state mutation in `render()`).

### ADR-4: No Scene Pointer Access from External Code

- **Context**: Should external code (e.g., Engine, main.cpp) be able to query "what scene is on top?" or access scene objects?
- **Decision**: No. The stack is opaque. You can push, pop, replace, and query size/empty, but you cannot get a `Scene*` back out.
- **Alternatives considered**: (a) Add `Scene* top()` accessor — enables external code to downcast and manipulate scenes directly, breaks encapsulation. (b) Add `template<typename T> T* find<T>()` to search the stack — complex, encourages tight coupling.
- **Consequences**: Scenes must communicate via engine-owned shared state (e.g., "selected song ID" stored in Engine or a future GameSession object) or event queues (future work). This is a cleaner architecture than scenes reaching into each other.

### ADR-5: Scene::render() Takes No Parameters

- **Context**: Should `render()` receive `Renderer*` as a parameter, or should scenes store a reference passed during construction?
- **Decision**: `render()` takes no parameters. Scenes store a `Renderer*` (or `Renderer&`) passed to their constructor.
- **Alternatives considered**: (a) Pass `Renderer*` to every render() call — matches the parameter-passing style of `update(dt)` and `handle_input(snapshot)`. (b) Pass SDL_Renderer* directly — bypasses our Renderer abstraction.
- **Consequences**: Scene constructors take `Renderer*` as a parameter. The renderer is stable (owned by Engine) so storing a pointer is safe. This matches the pattern used in Phase 1's `MinimalGameplayScene` (which stored `renderer_`).

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Scene forgets to implement on_pause, gameplay continues during pause | High | Low | Pure virtual interface forces implementation. Code review catches empty stubs. |
| Scene stores raw pointer to another scene, use-after-free when popped | High | Low | ADR-4 prevents scene pointer access. Scenes cannot hold pointers to each other. |
| Empty stack causes crash in Engine::run() loop | Med | Low | Loop condition checks `!scene_stack_->empty()`. Stack operations are no-ops when empty. |
| Scene's on_exit() throws, subsequent scenes not cleaned up | Med | Low | Engine loop has try-catch around update/render. Destructors must not throw (standard C++ rule). |
| Render state mutation in Scene::render() causes paused scene to drift | Low | Med | Document that render() must not mutate logical state. Testing strategy includes rendering multiple scenes. |

## Testing Strategy

### Unit Tests (`test/test_scene_stack.cpp`)

Mock scenes for testing stack operations:

```cpp
class MockScene : public Scene {
public:
    MockScene(std::vector<std::string>* log, const std::string& name)
        : log_(log), name_(name) {}
    
    void on_enter() override { log_->push_back(name_ + "::on_enter"); }
    void on_exit() override { log_->push_back(name_ + "::on_exit"); }
    void on_pause() override { log_->push_back(name_ + "::on_pause"); }
    void on_resume() override { log_->push_back(name_ + "::on_resume"); }
    void update(double dt) override { log_->push_back(name_ + "::update"); }
    void handle_input(const InputSnapshot& input) override { log_->push_back(name_ + "::handle_input"); }
    void render() override { log_->push_back(name_ + "::render"); }

private:
    std::vector<std::string>* log_;
    std::string name_;
};
```

Test cases covering all acceptance criteria:

| Test | What It Verifies | Story AC |
|------|-----------------|----------|
| `PushSceneCreatesOverlay` | Push B on A: A.on_pause, B.on_enter, input goes to B | US-SCN-001 SC1 |
| `PopSceneReturnsToPrevious` | Pop B: B.on_exit, A.on_resume, input goes to A | US-SCN-001 SC2 |
| `ReplaceSceneTransitions` | Replace A with B: A.on_exit, B.on_enter, no pause/resume | US-SCN-001 SC3 |
| `RenderOrderBottomToTop` | Stack [A, B, C], render calls A→B→C in order | US-SCN-001 SC4 |
| `OnlyTopSceneReceivesUpdate` | Stack [A, B], only B.update() is called | US-SCN-001 SC5 |
| `EmptyStackIsValid` | Empty stack: update/render/pop are no-ops, no crash | US-SCN-001 SC6 |
| `InputRoutedToTopScene` | Stack [A, B], only B.handle_input() is called | US-SCN-002 SC6 |
| `LifecycleOnEnterCalledOnPush` | Push scene, verify on_enter() is called exactly once | US-SCN-002 SC1 |
| `LifecycleOnExitCalledOnPop` | Pop scene, verify on_exit() is called before destruction | US-SCN-002 SC2 |
| `LifecycleOnPauseCalledOnOverlay` | Push B on A, verify A.on_pause() is called | US-SCN-002 SC3 |
| `LifecycleOnResumeCalledOnUncover` | Pop B from [A,B], verify A.on_resume() is called | US-SCN-002 SC4 |
| `UpdateAndRenderCalledEachFrame` | Update, render, update, render: verify order | US-SCN-002 SC5 |

### Integration Test (manual)

After implementing a concrete scene (e.g., `BootScene` in US-SCN-003), verify:

```bash
./build/openitup
# Expected: Boot scene displays, scene stack operates correctly
# Test push: navigate to title screen (replace)
# Test pop: ESC from title returns to boot (or exits if boot is bottom)
# Test overlay: pause during gameplay (future)
```

## Implementation Notes

### Initializing the Scene Stack in Engine

In `Engine::Engine()`:

```cpp
Engine::Engine(const EngineConfig& config)
    : config_(config),
      clock_(std::make_unique<Clock>()),
      renderer_(/* ... */),
      input_system_(/* ... */),
      audio_(/* ... */),
      scene_stack_(std::make_unique<SceneStack>())
{
    // ...
}
```

In `main.cpp`, after engine initialization:

```cpp
openitup::Engine engine(config);

// Push initial scene (BootScene, TitleScene, or directly to GameplayScene)
// This is story US-SCN-003's responsibility, but the API is:
engine.get_scene_stack()->push(std::make_unique<BootScene>(...));

return engine.run();
```

### Thread Safety

Not thread-safe. All scene stack operations happen on the main thread (the engine loop thread). Scenes that spawn background threads (asset loading, etc.) must synchronize access to shared state via mutexes or lock-free queues.

---

*Generated from stories in docs/stories/07-screen-flow.md (Phase 2: US-SCN-001, US-SCN-002)*  
*Last updated: 2026-04-29*
