# IP-SCN-002: Scene Stack Core Infrastructure Implementation Plan

**Design**: TD-SCN-002
**Stories**: US-SCN-001, US-SCN-002
**Total Steps**: 4
**Estimated Total**: ~2.5 hours
**Author**: technical-lead agent (via delivery-coordinator)
**Status**: Draft

## Prerequisites

The following must be implemented before this plan starts:

- **TD-ENG-001 / IP-ENG-001**: `Engine`, `Clock`, `compute_fixed_steps()` — the game loop
- **TD-INP-001 / IP-INP-001**: `InputSystem`, `InputSnapshot` — input handling

## Overview

This plan implements the Scene Stack — the foundational state management system for all game screens. The implementation is straightforward: define the `Scene` interface, implement `SceneStack` with push/pop/replace operations, integrate it into `Engine`, and add comprehensive unit tests with mock scenes.

---

## Step 1: Create Scene Interface and SceneStack Skeleton

**Files**:
- Create `src/openitup/scene/scene.h` — Scene abstract base class
- Create `src/openitup/scene/scene_stack.h` — SceneStack class declaration
- Create `src/openitup/scene/scene_stack.cpp` — SceneStack implementation (constructor, destructor, empty stubs)
- Modify `CMakeLists.txt` — Add `src/openitup/scene/scene_stack.cpp` to `openitup_engine`

**What to implement**:

### `scene.h` — The Interface Contract

```cpp
// src/openitup/scene/scene.h
#pragma once

#include <openitup/input/input_snapshot.h>

namespace openitup {

// Abstract base class for all game screens.
// Lifecycle: on_enter → (update/handle_input/render loop) → on_exit
// Overlays: on_pause when covered, on_resume when uncovered
class Scene {
public:
    virtual ~Scene() = default;

    // Called when scene becomes active (pushed or revealed after pop)
    virtual void on_enter() = 0;
    
    // Called before scene is destroyed (popped or replaced)
    virtual void on_exit() = 0;
    
    // Called when another scene is pushed on top (this scene is covered)
    virtual void on_pause() = 0;
    
    // Called when the scene above is popped (this scene is uncovered)
    virtual void on_resume() = 0;

    // Called once per logic tick (60 Hz) — only on the topmost scene
    virtual void update(double dt) = 0;
    
    // Called with the current input snapshot — only on the topmost scene
    virtual void handle_input(const InputSnapshot& input) = 0;

    // Called every render frame — on ALL scenes, bottom-to-top
    virtual void render() = 0;

protected:
    Scene() = default;
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
};

} // namespace openitup
```

### `scene_stack.h` — Stack Operations

```cpp
// src/openitup/scene/scene_stack.h
#pragma once

#include <memory>
#include <vector>

#include <openitup/input/input_snapshot.h>
#include <openitup/scene/scene.h>

namespace openitup {

// Manages a stack of scenes with push/pop/replace operations.
// Renders all scenes bottom-to-top. Updates and routes input only to the top scene.
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

### `scene_stack.cpp` — Skeleton Implementation

```cpp
// src/openitup/scene/scene_stack.cpp
#include <openitup/scene/scene_stack.h>

#include <spdlog/spdlog.h>

namespace openitup {

SceneStack::SceneStack() {
    spdlog::debug("SceneStack created");
}

SceneStack::~SceneStack() {
    // Call on_exit() on all remaining scenes in reverse order
    while (!scenes_.empty()) {
        pop();
    }
    spdlog::debug("SceneStack destroyed");
}

void SceneStack::push(std::unique_ptr<Scene> scene) {
    // Implemented in Step 2
}

void SceneStack::pop() {
    // Implemented in Step 2
}

void SceneStack::replace(std::unique_ptr<Scene> scene) {
    // Implemented in Step 2
}

void SceneStack::update(double dt) {
    // Implemented in Step 2
}

void SceneStack::handle_input(const InputSnapshot& input) {
    // Implemented in Step 2
}

void SceneStack::render() {
    // Implemented in Step 2
}

} // namespace openitup
```

### CMakeLists.txt modification

Add to the `openitup_engine` sources (around line 103, after `minimal_gameplay_scene.cpp`):

```cmake
    src/openitup/scene/scene_stack.cpp
```

**Tests**:

No tests yet — the methods are stubs. This step verifies the interface compiles.

**Definition of done**:
- [ ] `cmake --build build -j$(nproc)` succeeds
- [ ] All existing tests still pass (`cd build && ctest --output-on-failure`)
- [ ] Scene interface and SceneStack skeleton compile

**Expected commit message**:
```
feat(scene): add Scene interface and SceneStack skeleton

Defines the pure virtual Scene base class with lifecycle hooks
(on_enter, on_exit, on_pause, on_resume, update, handle_input, render).

Adds SceneStack class with push/pop/replace operations (stub
implementations). This is the foundation for Phase 2 screen flow.

Covers US-SCN-001 (infrastructure), US-SCN-002 (lifecycle interface).

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

**Estimated time**: ~30 minutes

---

## Step 2: Implement SceneStack Operations

**Files**:
- Modify `src/openitup/scene/scene_stack.cpp` — Implement push, pop, replace, update, handle_input, render

**What to implement**:

Replace the stub methods with full implementations:

```cpp
void SceneStack::push(std::unique_ptr<Scene> scene) {
    if (!scene) {
        spdlog::warn("SceneStack::push called with nullptr, ignoring");
        return;
    }

    // Pause the current top scene (if any)
    if (!scenes_.empty()) {
        scenes_.back()->on_pause();
    }

    // Add new scene and call its on_enter
    scenes_.push_back(std::move(scene));
    scenes_.back()->on_enter();
    
    spdlog::debug("Scene pushed, stack size: {}", scenes_.size());
}

void SceneStack::pop() {
    if (scenes_.empty()) {
        return;  // No-op
    }

    // Call on_exit and destroy the top scene
    scenes_.back()->on_exit();
    scenes_.pop_back();
    
    spdlog::debug("Scene popped, stack size: {}", scenes_.size());

    // Resume the new top scene (if any)
    if (!scenes_.empty()) {
        scenes_.back()->on_resume();
    }
}

void SceneStack::replace(std::unique_ptr<Scene> scene) {
    if (!scene) {
        spdlog::warn("SceneStack::replace called with nullptr, ignoring");
        return;
    }

    if (scenes_.empty()) {
        // Empty stack: replace is equivalent to push
        push(std::move(scene));
        return;
    }

    // Call on_exit and destroy the current top scene
    scenes_.back()->on_exit();
    scenes_.pop_back();

    // Add new scene and call its on_enter
    scenes_.push_back(std::move(scene));
    scenes_.back()->on_enter();
    
    spdlog::debug("Scene replaced, stack size: {}", scenes_.size());
}

void SceneStack::update(double dt) {
    if (!scenes_.empty()) {
        scenes_.back()->update(dt);
    }
}

void SceneStack::handle_input(const InputSnapshot& input) {
    if (!scenes_.empty()) {
        scenes_.back()->handle_input(input);
    }
}

void SceneStack::render() {
    // Render all scenes bottom-to-top (painter's algorithm)
    for (auto& scene : scenes_) {
        scene->render();
    }
}
```

**Key implementation notes**:

- `push()`: Pause old top (if any) → add new scene → call new scene's `on_enter()`
- `pop()`: Call top's `on_exit()` → remove from vector → resume new top (if any)
- `replace()`: Exit old top → remove → add new → enter new (no pause/resume)
- `update()` and `handle_input()`: Only call on `scenes_.back()` (top scene)
- `render()`: Forward iterate (index 0 to N-1) so scenes render in order
- Null checks and empty stack guards prevent crashes

**Tests**:

Still no automated tests — we need mock scenes which are created in Step 3.

**Definition of done**:
- [ ] `cmake --build build -j$(nproc)` succeeds
- [ ] All existing tests still pass
- [ ] SceneStack methods have full implementations

**Expected commit message**:
```
feat(scene): implement SceneStack push/pop/replace and frame operations

Implements lifecycle orchestration:
- push: pauses current top, adds new scene, calls on_enter
- pop: calls on_exit, removes scene, resumes new top
- replace: exits old top, adds new scene (no pause/resume)
- update/handle_input: routes to top scene only
- render: calls all scenes bottom-to-top

Empty stack operations are safe no-ops.

Covers US-SCN-001 acceptance criteria 1-6.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

**Estimated time**: ~30 minutes

---

## Step 3: Add Unit Tests with Mock Scenes

**Files**:
- Create `test/test_scene_stack.cpp` — Comprehensive unit tests with mock scenes
- Modify `CMakeLists.txt` — Add test file to `openitup_tests`

**What to implement**:

### Mock Scene for Testing

```cpp
// test/test_scene_stack.cpp
#include <gtest/gtest.h>
#include <openitup/scene/scene_stack.h>

using namespace openitup;

// Mock scene that logs all lifecycle calls
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

### Test Cases

Implement all acceptance criteria from US-SCN-001 and US-SCN-002:

```cpp
TEST(SceneStackTest, PushSceneCreatesOverlay) {
    // US-SCN-001 SC1
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();
    
    stack.push(std::make_unique<MockScene>(&log, "B"));
    
    EXPECT_EQ(stack.size(), 2);
    ASSERT_EQ(log.size(), 2);
    EXPECT_EQ(log[0], "A::on_pause");
    EXPECT_EQ(log[1], "B::on_enter");
    
    // Verify input routes to B
    log.clear();
    InputSnapshot input;
    stack.handle_input(input);
    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "B::handle_input");
}

TEST(SceneStackTest, PopSceneReturnsToPrevious) {
    // US-SCN-001 SC2
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    stack.push(std::make_unique<MockScene>(&log, "B"));
    log.clear();
    
    stack.pop();
    
    EXPECT_EQ(stack.size(), 1);
    ASSERT_EQ(log.size(), 2);
    EXPECT_EQ(log[0], "B::on_exit");
    EXPECT_EQ(log[1], "A::on_resume");
    
    // Verify input now routes to A
    log.clear();
    InputSnapshot input;
    stack.handle_input(input);
    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "A::handle_input");
}

TEST(SceneStackTest, ReplaceSceneTransitions) {
    // US-SCN-001 SC3
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();
    
    stack.replace(std::make_unique<MockScene>(&log, "B"));
    
    EXPECT_EQ(stack.size(), 1);
    ASSERT_EQ(log.size(), 2);
    EXPECT_EQ(log[0], "A::on_exit");
    EXPECT_EQ(log[1], "B::on_enter");
    
    // No pause or resume calls (replace, not overlay)
    for (const auto& entry : log) {
        EXPECT_TRUE(entry.find("pause") == std::string::npos);
        EXPECT_TRUE(entry.find("resume") == std::string::npos);
    }
}

TEST(SceneStackTest, RenderOrderBottomToTop) {
    // US-SCN-001 SC4
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    stack.push(std::make_unique<MockScene>(&log, "B"));
    stack.push(std::make_unique<MockScene>(&log, "C"));
    log.clear();
    
    stack.render();
    
    ASSERT_EQ(log.size(), 3);
    EXPECT_EQ(log[0], "A::render");
    EXPECT_EQ(log[1], "B::render");
    EXPECT_EQ(log[2], "C::render");
}

TEST(SceneStackTest, OnlyTopSceneReceivesUpdate) {
    // US-SCN-001 SC5
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    stack.push(std::make_unique<MockScene>(&log, "B"));
    log.clear();
    
    stack.update(1.0 / 60.0);
    
    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "B::update");
}

TEST(SceneStackTest, EmptyStackIsValid) {
    // US-SCN-001 SC6
    SceneStack stack;
    
    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(stack.size(), 0);
    
    // All operations should be no-ops, no crashes
    stack.update(1.0 / 60.0);
    InputSnapshot input;
    stack.handle_input(input);
    stack.render();
    stack.pop();
    
    EXPECT_TRUE(stack.empty());
}

TEST(SceneStackTest, LifecycleOnEnterCalledOnPush) {
    // US-SCN-002 SC1
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    
    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "A::on_enter");
}

TEST(SceneStackTest, LifecycleOnExitCalledOnPop) {
    // US-SCN-002 SC2
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();
    
    stack.pop();
    
    ASSERT_GE(log.size(), 1);
    EXPECT_EQ(log[0], "A::on_exit");
    EXPECT_TRUE(stack.empty());
}

TEST(SceneStackTest, LifecycleOnPauseCalledOnOverlay) {
    // US-SCN-002 SC3
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();
    
    stack.push(std::make_unique<MockScene>(&log, "B"));
    
    ASSERT_GE(log.size(), 1);
    EXPECT_EQ(log[0], "A::on_pause");
}

TEST(SceneStackTest, LifecycleOnResumeCalledOnUncover) {
    // US-SCN-002 SC4
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    stack.push(std::make_unique<MockScene>(&log, "B"));
    log.clear();
    
    stack.pop();
    
    ASSERT_GE(log.size(), 2);
    EXPECT_EQ(log[1], "A::on_resume");
}

TEST(SceneStackTest, UpdateAndRenderCalledEachFrame) {
    // US-SCN-002 SC5
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();
    
    stack.update(1.0 / 60.0);
    stack.render();
    
    ASSERT_EQ(log.size(), 2);
    EXPECT_EQ(log[0], "A::update");
    EXPECT_EQ(log[1], "A::render");
}

TEST(SceneStackTest, InputRoutedToTopScene) {
    // US-SCN-002 SC6
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    stack.push(std::make_unique<MockScene>(&log, "B"));
    log.clear();
    
    InputSnapshot input;
    stack.handle_input(input);
    
    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "B::handle_input");
}

TEST(SceneStackTest, DestructorCallsOnExitOnRemainingScenes) {
    std::vector<std::string> log;
    
    {
        SceneStack stack;
        stack.push(std::make_unique<MockScene>(&log, "A"));
        stack.push(std::make_unique<MockScene>(&log, "B"));
        log.clear();
        // Destructor runs here
    }
    
    // Both scenes should have on_exit called (in reverse order via pops)
    ASSERT_GE(log.size(), 2);
    EXPECT_EQ(log[0], "B::on_exit");
    EXPECT_EQ(log[1], "A::on_exit");
}

TEST(SceneStackTest, ReplaceOnEmptyStackEquivalentToPush) {
    std::vector<std::string> log;
    SceneStack stack;
    
    EXPECT_TRUE(stack.empty());
    
    stack.replace(std::make_unique<MockScene>(&log, "A"));
    
    EXPECT_EQ(stack.size(), 1);
    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "A::on_enter");
}

TEST(SceneStackTest, PushNullptrIsNoOp) {
    SceneStack stack;
    stack.push(nullptr);
    
    EXPECT_TRUE(stack.empty());
}

TEST(SceneStackTest, ReplaceNullptrIsNoOp) {
    std::vector<std::string> log;
    SceneStack stack;
    
    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();
    
    stack.replace(nullptr);
    
    // Original scene should still be there
    EXPECT_EQ(stack.size(), 1);
    EXPECT_TRUE(log.empty());  // No lifecycle calls
}
```

### CMakeLists.txt modification

Add to the `openitup_tests` sources (around line 183, after `test_noteskin.cpp`):

```cmake
    test/test_scene_stack.cpp
```

**Definition of done**:
- [ ] `cmake --build build -j$(nproc)` succeeds
- [ ] `cd build && ctest --output-on-failure -R SceneStack` passes all tests
- [ ] All existing tests still pass

**Expected commit message**:
```
test(scene): add comprehensive SceneStack unit tests with mock scenes

Tests all acceptance criteria:
- US-SCN-001: push/pop/replace, render order, update routing, empty stack
- US-SCN-002: lifecycle hooks (on_enter/exit/pause/resume), input routing

Uses MockScene that logs all method calls for verification.
Covers all edge cases including nullptr guards and destructor cleanup.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

**Estimated time**: ~1 hour

---

## Step 4: Integrate SceneStack into Engine

**Files**:
- Modify `src/openitup/core/engine.h` — Add scene_stack_ member and accessor
- Modify `src/openitup/core/engine.cpp` — Initialize scene stack, modify run() to use it

**What to implement**:

### engine.h modifications

Add member and accessor (after line 61, before `private:`):

```cpp
    SceneStack* get_scene_stack() const { return scene_stack_.get(); }
```

Add member variable in the private section (after line 61, after `audio_`):

```cpp
    std::unique_ptr<SceneStack> scene_stack_;
```

Add include at the top (after existing includes):

```cpp
#include <openitup/scene/scene_stack.h>
```

### engine.cpp modifications

In the constructor (after line 27, after `init_audio();`):

```cpp
    // Initialize scene stack
    scene_stack_ = std::make_unique<SceneStack>();
    spdlog::debug("scene stack initialized");
```

Modify `Engine::run()` to delegate to the scene stack. Replace the existing loop logic (starting around line 84) with:

```cpp
int Engine::run() {
    running_ = true;
    tick_count_ = 0;
    accumulator_ = 0.0;
    clock_->reset();

    while (running_ && !scene_stack_->empty()) {
        process_events();

        double delta = clock_->tick();

        auto result = compute_fixed_steps(delta, accumulator_);
        accumulator_ = result.new_accumulator;
        render_alpha_ = result.alpha;

        if (result.spiral_guard_triggered) {
            spdlog::warn("spiral-of-death guard: discarded excess time");
        }

        for (int i = 0; i < result.num_steps; i++) {
            try {
                if (input_system_) {
                    input_system_->poll(tick_count_);
                    scene_stack_->handle_input(input_system_->snapshot());
                }
                scene_stack_->update(FIXED_STEP);
            } catch (const std::exception& e) {
                spdlog::error("exception in scene update: {}", e.what());
            }
            tick_count_++;
        }

        renderer_->begin_frame();
        try {
            scene_stack_->render();
        } catch (const std::exception& e) {
            spdlog::error("exception in scene render: {}", e.what());
        }
        renderer_->end_frame();

        if (target_frame_time_ > 0.0) {
            double frame_time = clock_->elapsed();
            if (frame_time < target_frame_time_) {
                SDL_Delay(static_cast<uint32_t>((target_frame_time_ - frame_time) * 1000.0));
            }
        }
    }

    spdlog::info("scene stack empty or quit requested, exiting");
    return 0;
}
```

**Key changes**:

- Loop condition: `!scene_stack_->empty()` ensures loop exits when all scenes are gone
- Input: `scene_stack_->handle_input(input_system_->snapshot())` before update
- Update: `scene_stack_->update(FIXED_STEP)` instead of calling a single scene
- Render: `scene_stack_->render()` instead of a single scene

**Note about run_gameplay()**:

The existing `Engine::run_gameplay()` method (Phase 1) is still present and functional. It doesn't use the scene stack. This is intentional — we'll phase it out when we refactor `MinimalGameplayScene` to implement the `Scene` interface (a future story, not part of this IP).

**Tests**:

No new tests — the existing engine tests still pass. Integration testing happens in subsequent stories when concrete scenes (BootScene, TitleScene) are implemented.

**Definition of done**:
- [ ] `cmake --build build -j$(nproc)` succeeds
- [ ] `cd build && ctest --output-on-failure` passes all tests
- [ ] Engine owns a SceneStack and delegates to it in run()
- [ ] Existing run_gameplay() still works (backward compatibility)

**Expected commit message**:
```
feat(engine): integrate SceneStack into Engine main loop

Engine now owns a SceneStack and delegates update/render/input to it.
Main loop exits when the scene stack is empty or quit is requested.

Engine::run_gameplay() is unchanged (Phase 1 backward compatibility).

Completes US-SCN-001 and US-SCN-002 infrastructure. Concrete scenes
(BootScene, TitleScene, etc.) can now be implemented in follow-up stories.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

**Estimated time**: ~30 minutes

---

## Summary

| Step | What | Files Modified | Stories Covered | Est. |
|------|------|---------------|-----------------|------|
| 1 | Scene interface + SceneStack skeleton | 3 new + 1 modified | US-SCN-001, US-SCN-002 (structure) | 30m |
| 2 | SceneStack implementation | 1 modified | US-SCN-001 SC1-6 | 30m |
| 3 | Unit tests with mock scenes | 1 new + 1 modified | US-SCN-001 (all), US-SCN-002 (all) | 1h |
| 4 | Engine integration | 2 modified | US-SCN-001, US-SCN-002 (complete) | 30m |

**Total new source files**: 3 (scene.h, scene_stack.h, scene_stack.cpp)  
**Total new test files**: 1 (test_scene_stack.cpp)  
**CMakeLists.txt modifications**: Add 1 .cpp to `openitup_engine`, add 1 test to `openitup_tests`

## Build Verification

After each step:

```bash
# Compile
cmake --build build -j$(nproc)

# Run all tests
cd build && ctest --output-on-failure

# Run scene stack tests specifically (after Step 3)
cd build && ctest --output-on-failure -R SceneStack
```

## Acceptance Verification

After Step 4 completes, all acceptance criteria from US-SCN-001 and US-SCN-002 are verified:

| Story | AC | How Verified |
|-------|-----|--------------|
| US-SCN-001 | SC1: Push creates overlay | `test_scene_stack.cpp::PushSceneCreatesOverlay` |
| US-SCN-001 | SC2: Pop returns to previous | `test_scene_stack.cpp::PopSceneReturnsToPrevious` |
| US-SCN-001 | SC3: Replace transitions | `test_scene_stack.cpp::ReplaceSceneTransitions` |
| US-SCN-001 | SC4: Render order bottom-to-top | `test_scene_stack.cpp::RenderOrderBottomToTop` |
| US-SCN-001 | SC5: Only top receives update | `test_scene_stack.cpp::OnlyTopSceneReceivesUpdate` |
| US-SCN-001 | SC6: Empty stack is valid | `test_scene_stack.cpp::EmptyStackIsValid` |
| US-SCN-002 | SC1: on_enter on push | `test_scene_stack.cpp::LifecycleOnEnterCalledOnPush` |
| US-SCN-002 | SC2: on_exit on pop | `test_scene_stack.cpp::LifecycleOnExitCalledOnPop` |
| US-SCN-002 | SC3: on_pause on overlay | `test_scene_stack.cpp::LifecycleOnPauseCalledOnOverlay` |
| US-SCN-002 | SC4: on_resume on uncover | `test_scene_stack.cpp::LifecycleOnResumeCalledOnUncover` |
| US-SCN-002 | SC5: update/render each frame | `test_scene_stack.cpp::UpdateAndRenderCalledEachFrame` |
| US-SCN-002 | SC6: input to top scene | `test_scene_stack.cpp::InputRoutedToTopScene` |

## PR Strategy

**Single PR**: All 4 steps go into one PR since they build on each other and together form a cohesive feature (Scene Stack infrastructure). The PR is self-contained — it adds the scene stack but doesn't modify any existing game screens.

**PR Title**: `feat(scene): add Scene Stack infrastructure for Phase 2 screen flow`

**PR Description**:
```
Implements the Scene Stack — the foundational state management system for all
game screens (boot, title, mode select, gameplay, pause overlays, etc.).

Features:
- Scene abstract base class with lifecycle hooks (on_enter/exit/pause/resume)
- SceneStack with push/pop/replace operations
- Renders all scenes bottom-to-top, updates only the top scene
- Comprehensive unit tests with mock scenes
- Integrated into Engine main loop

Covers:
- US-SCN-001: Scene stack core infrastructure (5 pts)
- US-SCN-002: Scene lifecycle interface (3 pts)

Testing:
- 15+ unit tests covering all acceptance criteria
- All existing tests pass (354 tests)
- No behavioral changes to Phase 1 gameplay (run_gameplay still works)

Next steps: Implement concrete scenes (BootScene, TitleScene, etc.)
```

## Notes

- The scene stack is ready for use but inactive until a scene is pushed. The next stories (US-SCN-003, US-SCN-004, US-SCN-005) will implement concrete scene classes.
- Phase 1's `MinimalGameplayScene` doesn't implement the `Scene` interface yet. Refactoring it is a future task (likely when implementing US-SCN-007b).
- The scene stack is not thread-safe. All operations must happen on the main thread (the engine loop thread).
