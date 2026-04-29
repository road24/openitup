# IP-ENG-001: Core Engine Loop, Clock, and Engine Class Implementation Plan

**Design**: TD-ENG-001
**Stories**: US-ENG-001, US-ENG-003, US-ENG-004, US-ENG-011, US-ENG-012, US-ENG-021, US-ENG-063a, US-ENG-063b
**Total Steps**: 8
**Estimated Total**: ~4 hours
**Author**: technical-lead agent
**Status**: Draft

## Step 1: Create Clock with Dependency Injection

**Files**:
- Create `src/openitup/core/clock.h` — Clock class declaration with injectable counter/frequency functions
- Create `src/openitup/core/clock.cpp` — Clock implementation (tick, elapsed, reset)
- Modify `CMakeLists.txt` — Add `src/openitup/core/clock.cpp` to `openitup_engine` library sources

**What to implement**:

The Clock is a pure timing utility with no SDL dependencies by default. It wraps high-resolution time measurement behind injectable function pointers (`CounterFn`, `FrequencyFn`) for testing.

Key implementation details from TD-ENG-001:
- Use `double` throughout (not `float`) for precision over multi-hour sessions
- Cache frequency value at construction (it never changes)
- `first_tick_` flag ensures first `tick()` returns 0.0, not time since program start
- Integer arithmetic (subtract counters, then convert) preserves precision
- Default constructor uses `SDL_GetPerformanceCounter()` and `SDL_GetPerformanceFrequency()`
- Injectable constructor accepts `std::function` wrappers for testing

Interface:
```cpp
namespace openitup {
class Clock {
public:
    using CounterFn = std::function<uint64_t()>;
    using FrequencyFn = std::function<uint64_t()>;
    
    Clock();  // defaults to SDL perf counter
    Clock(CounterFn counter_fn, FrequencyFn frequency_fn);  // injectable
    
    double tick();           // returns delta since last tick, first call returns 0.0
    double elapsed() const;  // time since construction/reset
    void reset();            // reset elapsed to zero
    uint64_t raw_counter() const;   // for test verification
    uint64_t frequency() const;     // for test verification
};
}
```

**Tests**:
- Create `test/test_clock.cpp` — Unit tests for Clock with injectable fake counter
- Add `test/test_clock.cpp` to `openitup_tests` in `CMakeLists.txt`

Test cases (all use fake counter, no SDL dependencies):
- `FirstTickReturnsZero` — First tick() returns 0.0
- `DeltaTimeAccuracy` — Advancing fake counter produces correct delta
- `ElapsedTimeTracking` — elapsed() returns cumulative time since construction
- `ResetClearsElapsed` — After reset(), elapsed is zero and tick measures from reset
- `PrecisionAfter2Hours` — Simulate 432,000 ticks (2 hours at 60Hz), verify cumulative error < 50 microseconds
- `HighFrequencyCounter` — Test with realistic 10 GHz frequency
- `ZeroDeltaWhenCounterUnchanged` — If counter doesn't advance, delta is 0.0

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Clock` passes all Clock tests
- [ ] No SDL calls in tests (pure fake counter injection)

**Expected commit message**:
`feat(core): add Clock with injectable time source for deterministic testing`

**Estimated time**: ~45 minutes

---

## Step 2: Create Accumulator Logic Helper

**Files**:
- Modify `src/openitup/core/clock.h` — Add free function `compute_fixed_steps()` for accumulator logic
- Modify `src/openitup/core/clock.cpp` — Implement accumulator logic with spiral-of-death guard
- Modify `test/test_clock.cpp` — Add unit tests for accumulator logic

**What to implement**:

Extract the timing accumulator logic into a pure, testable function. This allows testing the fixed-step loop math without needing an Engine or Renderer.

Function signature:
```cpp
namespace openitup {

struct FixedStepResult {
    int num_steps;          // how many fixed steps to execute
    double new_accumulator; // remaining time after steps
    double alpha;           // render interpolation factor [0.0, 1.0)
    bool spiral_guard_triggered;  // true if time was discarded
};

// Compute fixed-step logic given a time delta and current accumulator.
// FIXED_STEP = 1.0/60.0, MAX_STEPS = 10.
FixedStepResult compute_fixed_steps(double delta, double accumulator);

} // namespace openitup
```

Logic (from TD-ENG-001 "Key Algorithms" section):
1. Add delta to accumulator
2. Clamp accumulator to `FIXED_STEP * MAX_STEPS` (spiral guard)
3. Count steps: `while (accumulator >= FIXED_STEP) { steps++; accumulator -= FIXED_STEP; }`
4. Compute alpha: `accumulator / FIXED_STEP`

**Tests**:

Add to `test/test_clock.cpp`:
- `SingleStepAt60Hz` — 16.67ms delta produces exactly 1 step
- `NoStepUnder16ms` — 8ms delta produces 0 steps, accumulator carries
- `MultipleStepsOnSlowFrame` — 50ms delta produces 3 steps
- `SpiralGuardCapsAt10Steps` — 500ms delta produces exactly 10 steps, excess discarded
- `AccumulatorCarriesRemainder` — 20ms delta: 1 step, ~3.33ms remainder carried
- `AlphaComputedCorrectly` — After partial accumulation, alpha = remainder / FIXED_STEP
- `AlphaZeroAfterExactStep` — After exactly 16.67ms, alpha = 0.0
- `CarryAccumulatorAcrossFrames` — Test that accumulator carries correctly across multiple frames

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Clock` passes all tests including accumulator tests
- [ ] Function is pure (no state, no SDL)

**Expected commit message**:
`feat(core): add fixed-step accumulator logic with spiral-of-death guard`

**Estimated time**: ~30 minutes

---

## Step 3: Create Engine Skeleton with EngineConfig

**Files**:
- Create `src/openitup/core/engine.h` — Engine class declaration with EngineConfig struct
- Create `src/openitup/core/engine.cpp` — Engine stub implementation (constructor, destructor, accessors)
- Modify `CMakeLists.txt` — Add `src/openitup/core/engine.cpp` to `openitup_engine` library sources

**What to implement**:

The Engine class skeleton with:
- `EngineConfig` struct (window_title, window_width, window_height, target_fps)
- Two constructors: default creates real Clock, second accepts injected Clock
- Member variables: `clock_`, `renderer_`, `running_`, `tick_count_`, `accumulator_`, `render_alpha_`, `target_frame_time_`
- `init_renderer()` private method that creates Renderer and calls `renderer_->init()`, throws on failure
- Accessors: `get_renderer()`, `get_clock()`, `tick_count()`, `render_alpha()`, `is_running()`, `request_quit()`
- Stub methods: `run()` returns 0 immediately, `process_events()` empty, `update()` empty, `render()` empty

Constants:
```cpp
static constexpr double FIXED_STEP = 1.0 / 60.0;
static constexpr int MAX_STEPS_PER_FRAME = 10;
```

Constructor logic:
1. Store config
2. Create Clock (real or injected)
3. Compute `target_frame_time_` from config.target_fps
4. Call `init_renderer(config)` — throws std::runtime_error on failure

**Tests**:

Not yet — this step just sets up the structure. Tests come in Step 5 when we can construct Engine with a mock.

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] Code compiles and links with openitup_engine
- [ ] Renderer is owned via `unique_ptr`, Clock is owned via `unique_ptr`

**Expected commit message**:
`feat(core): add Engine skeleton with EngineConfig and subsystem ownership`

**Estimated time**: ~30 minutes

---

## Step 4: Implement Engine Main Loop

**Files**:
- Modify `src/openitup/core/engine.cpp` — Implement `run()`, `process_events()`, `update()`, `render()`

**What to implement**:

Replace the stub `run()` with the full fixed-step game loop from TD-ENG-001:

```cpp
int Engine::run() {
    running_ = true;
    tick_count_ = 0;
    accumulator_ = 0.0;
    clock_->reset();
    
    while (running_) {
        process_events();
        
        double delta = clock_->tick();
        
        auto result = compute_fixed_steps(delta, accumulator_);
        accumulator_ = result.new_accumulator;
        render_alpha_ = result.alpha;
        
        if (result.spiral_guard_triggered) {
            spdlog::warn("Spiral-of-death guard: discarded time");
        }
        
        // Fixed-step updates
        for (int i = 0; i < result.num_steps; i++) {
            try {
                update(FIXED_STEP);
            } catch (const std::exception& e) {
                spdlog::error("Exception in update: {}", e.what());
            }
            tick_count_++;
        }
        
        // Render with interpolation
        renderer_->begin_frame();
        try {
            render(render_alpha_);
        } catch (const std::exception& e) {
            spdlog::error("Exception in render: {}", e.what());
        }
        renderer_->end_frame();
        
        // Optional frame limiter
        if (target_frame_time_ > 0.0) {
            double frame_elapsed = clock_->tick();
            double remaining = target_frame_time_ - frame_elapsed;
            if (remaining > 0.001) {
                SDL_Delay(static_cast<uint32_t>(remaining * 1000.0));
            }
        }
    }
    
    return 0;
}

void Engine::process_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            request_quit();
        }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
            request_quit();
        }
    }
}

void Engine::update(double dt) {
    // Future: scene_stack_->top().update(dt);
}

void Engine::render(double alpha) {
    // Future: for (auto& scene : scene_stack_->visible_scenes()) scene.render(alpha);
}

void Engine::request_quit() {
    running_ = false;
}
```

**Tests**:

Not yet — the loop needs SDL/Renderer which we'll test in the integration step. This step just implements the logic.

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] Code compiles and links
- [ ] `run()` contains the full loop with try-catch, accumulator, spiral guard, and frame limiter

**Expected commit message**:
`feat(core): implement Engine main loop with fixed-step updates and render interpolation`

**Estimated time**: ~30 minutes

---

## Step 5: Add Engine Unit Tests (Headless)

**Files**:
- Create `test/test_engine.cpp` — Unit tests for Engine timing logic without rendering
- Modify `CMakeLists.txt` — Add `test/test_engine.cpp` to `openitup_tests`

**What to implement**:

Test the Engine's timing behavior using an injectable fake Clock. The challenge is that Engine owns a Renderer which requires SDL. Two approaches:

**Approach A (simpler for Phase 1)**: Test accumulator logic directly via `compute_fixed_steps()` (already done in Step 2). Add a minimal Engine integration test that constructs Engine with a fake Clock, calls `request_quit()` immediately, and verifies `run()` returns 0.

**Approach B**: Mock the Renderer by having Engine accept an optional injected Renderer (same pattern as Clock). For Phase 1, we defer this.

Tests for Step 5:
- `EngineConstructsWithDefaults` — Create Engine with default config and fake Clock (will fail Renderer init in headless env, so catch exception and verify error message)
- `RequestQuitExitsLoop` — Fake Clock, request_quit() immediately, verify run() processes and exits cleanly

These tests will be minimal. The real loop behavior testing comes in Step 6 (integration).

**Tests**:

Unit tests in `test/test_engine.cpp`:
- `TickCountIncrements` — Use fake Clock, advance time to trigger steps, verify tick_count matches expected
- `AlphaComputedCorrectly` — Verify render_alpha() matches expected interpolation factor
- `TwoHourDriftTest` — Simulate 432,000 ticks, verify tick_count == 432,000

NOTE: These tests cannot construct a full Engine in headless mode. Instead, they test the timing logic by:
1. Creating a standalone Clock with fake counter
2. Manually calling `compute_fixed_steps()` (tested in Step 2)
3. Verifying tick_count increment logic

Or, document that these tests require SDL (run only on systems with display).

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Engine` passes (may require DISPLAY for SDL)
- [ ] Tests verify tick_count, alpha, and 2-hour drift

**Expected commit message**:
`test(core): add Engine timing tests with fake Clock`

**Estimated time**: ~30 minutes

---

## Step 6: Add Engine Integration Tests (SDL-dependent)

**Files**:
- Modify `test/test_integration.cpp` — Add Engine integration tests (requires SDL/display)

**What to implement**:

Add to the existing `IntegrationTest` fixture:

Tests:
- `EngineStartsAndStopsCleanly` — Construct Engine with default config, immediately call request_quit(), verify run() returns 0 and no crashes
- `RendererOwnedByEngine` — Construct Engine, verify get_renderer().get() returns valid SDL_Renderer*
- `EngineRunsOneFrame` — Construct Engine with fake Clock, advance time by exactly 16.67ms, request_quit, verify tick_count == 1
- `MultipleLogicStepsInOneFrame` — Fake Clock, advance by 50ms, verify tick_count == 3 (three 60Hz steps)
- `RenderAlphaInterpolates` — Fake Clock, advance by 8ms (half a frame), request_quit before next frame, verify render_alpha close to 0.5

These tests prove the loop runs correctly with real SDL context.

**Tests**:

Add to `test/test_integration.cpp` (existing SDL test fixture):
- `EngineStartsAndStops`
- `EngineOwnsRenderer`
- `EngineSingleFrame`
- `EngineMultipleSteps`

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `cd build && ctest --output-on-failure -R Integration` passes
- [ ] Tests verify Engine loop behavior with real Renderer

**Expected commit message**:
`test(core): add Engine integration tests with SDL Renderer`

**Estimated time**: ~30 minutes

---

## Step 7: Replace main.cpp with Engine

**Files**:
- Modify `src/openitup/main.cpp` — Replace prototype loop with Engine construction and run()

**What to implement**:

Replace the current ~60 lines of manual SDL event handling and timing with:

```cpp
#include <openitup/core/engine.h>

#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    try {
        openitup::EngineConfig config;
        config.window_title = "openitup";
        config.window_width = 1280;
        config.window_height = 960;
        config.target_fps = 0.0;  // uncapped
        
        openitup::Engine engine(config);
        return engine.run();
    } catch (const std::exception& e) {
        spdlog::critical("Fatal startup error: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::critical("Fatal startup error: unknown exception");
        return 1;
    }
}
```

The existing `main.cpp` has:
- Manual BGA animation loading from argv[1]
- Manual timing loop with `SDL_GetPerformanceCounter()`
- Manual event polling

We're removing all of this for Phase 1. The BGA playback will return in Phase 2 when the SceneStack is implemented. For Phase 1, the Engine runs with empty `update()` and `render()` methods — it just proves the loop timing is correct.

**Tests**:

Manual verification:
```bash
cd build
./openitup
# Should open window, show black screen, respond to ESC and window close
```

**Definition of done**:
- [ ] `cmake --build build` succeeds
- [ ] `./build/openitup` runs without crashing
- [ ] Window opens and closes cleanly
- [ ] ESC key exits the application
- [ ] No BGA playback (empty frame)

**Expected commit message**:
`refactor(core): replace main.cpp prototype loop with Engine`

**Estimated time**: ~15 minutes

---

## Step 8: Add Error Handling Tests

**Files**:
- Modify `test/test_engine.cpp` — Add error handling tests for US-ENG-063a, US-ENG-063b

**What to implement**:

Tests for the error handling stories:

**US-ENG-063a: Runtime Exception Handling**
- `UpdateExceptionDoesNotCrash` — Inject an update() that throws std::runtime_error, verify loop continues
- `RenderExceptionDoesNotCrash` — Inject a render() that throws std::runtime_error, verify loop continues

**US-ENG-063b: Startup Failure Reporting**
- `RendererInitFailureThrows` — Mock Renderer::init() to return false, verify Engine constructor throws std::runtime_error with message
- `MainCatchesStartupException` — Verify main.cpp catch block logs error and returns 1

Since we can't easily mock Renderer::init() without refactoring, document these as manual tests or use environment variables to trigger failure paths.

For Phase 1, add tests that verify:
1. Exception in a mock update callback is caught and logged
2. Exception in a mock render callback is caught and logged

Mock approach: Add temporary hooks to Engine for testing:
```cpp
// In engine.h (for testing only, will be removed when SceneStack is added)
#ifdef OPENITUP_TESTING
    std::function<void(double)> test_update_hook_;
    std::function<void(double)> test_render_hook_;
#endif
```

Then in update/render, call the hook if set. Tests can inject throwing lambdas.

Alternatively, skip this step and document that error handling is verified manually. The try-catch blocks are visible in the code (Step 4).

**Tests**:

Manual verification (no new test code needed — the try-catch is in place):
1. Modify `update()` to `throw std::runtime_error("test");` temporarily
2. Run `./build/openitup`
3. Verify ERROR log appears but program doesn't crash
4. Repeat for `render()`

Or, add simple tests to `test/test_engine.cpp` that verify exception paths (if hooks are added).

**Definition of done**:
- [ ] Exception handling code present in Engine::run()
- [ ] Manual test confirms exceptions don't crash the loop
- [ ] spdlog ERROR messages appear when exceptions are thrown

**Expected commit message**:
`test(core): verify exception handling in Engine loop`

**Estimated time**: ~20 minutes

---

## PR Strategy

- [ ] **Single PR recommended** — All 8 steps are tightly coupled (Clock → Engine → main.cpp). Splitting would create intermediate states where the new code exists but isn't used.
- [ ] **Review checkpoint**: After Step 6 (integration tests pass), before Step 7 (main.cpp replacement). This ensures the Engine is fully tested before switching over.
- [ ] **Risk mitigation**: Step 7 removes the existing BGA playback from main.cpp. Document that this is intentional for Phase 1 — BGA playback returns in Phase 2 via SceneStack.

Alternative: Split into two PRs:
1. Steps 1-6: Add Clock and Engine without changing main.cpp
2. Steps 7-8: Replace main.cpp and add error handling tests

This allows reviewing the new architecture before removing the old prototype.

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
# Should open window, show black screen, exit on ESC
```

Verify logs show clean startup and shutdown with no errors.

## Acceptance Verification

| Story ID | How to verify |
|----------|--------------|
| US-ENG-001 | `test/test_clock.cpp`: `PrecisionAfter2Hours`, `AccumulatorCarriesRemainder`, `SpiralGuardCapsAt10Steps` |
| US-ENG-003 | Manual test: Set config.target_fps = 60.0, verify frame timing with logs |
| US-ENG-004 | `test/test_clock.cpp`: `DeltaTimeAccuracy`, `ElapsedTimeTracking` |
| US-ENG-011 | `test/test_engine.cpp`: `TickCountIncrements`, verify tick_count() accessor |
| US-ENG-012 | `test/test_engine.cpp`: `AlphaComputedCorrectly`, verify render_alpha() in [0.0, 1.0) |
| US-ENG-021 | `test/test_clock.cpp`: `PrecisionAfter2Hours` with 432,000 ticks, error < 50μs |
| US-ENG-063a | Manual test: Inject exception in update(), verify ERROR log and continued execution |
| US-ENG-063b | Manual test: Run with no display (headless), verify constructor throws and main() returns 1 |

## Notes

**Why Step 2 (accumulator logic) is separate from Clock**: The accumulator logic depends on `FIXED_STEP` and `MAX_STEPS_PER_FRAME` which are Engine constants, not Clock concerns. Extracting it as a free function makes it trivially testable and keeps Clock focused on time measurement.

**Why main.cpp loses BGA playback in Step 7**: Phase 1 is about proving the timing infrastructure works. BGA animations will be integrated via the SceneStack in Phase 2 (story US-SCN-0xx). The existing `bga_player` tool remains functional for BGA testing.

**Testing in headless environments**: Steps 5-6 require SDL and a display. CI systems should either:
1. Use Xvfb (virtual framebuffer)
2. Skip integration tests with `ctest -E Integration`
3. Use SDL's offscreen rendering backend (SDL_RENDER_DRIVER=software)

**Renderer ownership note**: TD-ENG-001 flags that `SDL_Init(SDL_INIT_VIDEO)` is currently in `Renderer::init()`. When AudioSystem is added (Phase 2), SDL initialization should be lifted to Engine constructor. No changes needed for Phase 1.
