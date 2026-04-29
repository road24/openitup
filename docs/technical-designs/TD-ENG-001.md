# TD-ENG-001: Core Engine Loop, Clock, and Engine Class

**Stories**: US-ENG-021, US-ENG-001, US-ENG-003, US-ENG-004, US-ENG-011, US-ENG-012, US-ENG-063a, US-ENG-063b
**Phase**: 1
**Author**: technical-architect agent
**Status**: Draft

## Overview

This design introduces the foundational `Engine` class, a `Clock` timing utility, and a fixed-step game loop with render interpolation. Together they replace the prototype loop in `main.cpp` with a production architecture that enforces deterministic 60 Hz logic ticks, supports variable-rate rendering with interpolation, and establishes the ownership hierarchy for all future subsystems (audio, input, scene stack).

The design integrates with the existing `Renderer` class (`src/openitup/gfx/renderer.h`) and follows the established patterns: `std::unique_ptr` ownership, `snake_case` methods with trailing-underscore members, spdlog for logging, and function-based dependency injection (as seen in `TextureCache`'s `ImageLoaderFn`).

## Architecture

### Component Diagram

```
main()
  |
  v
Engine  ─────────────────────────────────────────────┐
  |  owns (unique_ptr)                               |
  |                                                  |
  ├── Clock (timing source)                          |
  |     |  uses CounterFn / FrequencyFn              |
  |     |  (injectable, defaults to SDL perf counter) |
  |                                                  |
  ├── Renderer (existing, moved into Engine)         |
  |     |  SDL_Window + SDL_Renderer, 640x480        |
  |                                                  |
  ├── [future: AudioSystem]                          |
  ├── [future: InputSystem]                          |
  └── [future: SceneStack]                           |
       |                                             |
       └── Scene::update(double dt)                  |
           Scene::render(double alpha)               |
                                                     |
  Engine::run()                                      |
    while running:                                   |
      delta = clock_.delta()                         |
      accumulator_ += delta                          |
      clamp accumulator_ to MAX_FRAME_TIME           |
      while accumulator_ >= FIXED_STEP:              |
        [scene->update(FIXED_STEP)]   // 60 Hz logic |
        accumulator_ -= FIXED_STEP                   |
      alpha = accumulator_ / FIXED_STEP              |
      renderer_->begin_frame()                       |
      [scene->render(alpha)]          // variable Hz |
      renderer_->end_frame()                         |
      [optional frame limiter]                       |
```

### New Types

#### `Clock` (`src/openitup/core/clock.h`)

The Clock wraps high-resolution time measurement behind injectable function pointers, enabling deterministic unit testing. It uses `double` throughout to avoid float precision issues over multi-hour sessions (a `double` has 52 mantissa bits -- more than sufficient for microsecond precision over days).

The injection pattern matches `TextureCache::ImageLoaderFn`: `std::function` wrappers for the two SDL calls the Clock needs. Default-constructed `Clock` uses real SDL performance counters. Tests pass lambdas that advance a synthetic counter.

```cpp
// src/openitup/core/clock.h
#pragma once

#include <cstdint>
#include <functional>

namespace openitup {

class Clock {
public:
    // Injectable time sources. Default to SDL performance counter.
    using CounterFn = std::function<uint64_t()>;
    using FrequencyFn = std::function<uint64_t()>;

    // Default constructor: uses SDL_GetPerformanceCounter/Frequency.
    Clock();

    // Injectable constructor for testing.
    Clock(CounterFn counter_fn, FrequencyFn frequency_fn);

    // Call once per frame (or per measurement point).
    // Returns the time in seconds since the previous tick() call.
    // First call returns 0.0.
    double tick();

    // Time in seconds since construction or last reset().
    double elapsed() const;

    // Reset elapsed time to zero. Next tick() measures from this point.
    void reset();

    // Read the raw counter value (exposed for testing verification).
    uint64_t raw_counter() const;

    // Read the frequency (exposed for testing verification).
    uint64_t frequency() const;

private:
    CounterFn counter_fn_;
    FrequencyFn frequency_fn_;

    uint64_t frequency_value_;    // cached from frequency_fn_() at construction
    uint64_t start_counter_;      // counter at construction / reset
    uint64_t last_counter_;       // counter at previous tick()
    bool first_tick_;             // true until first tick() call
};

} // namespace openitup
```

**Key decisions**:

- `tick()` returns `double` (seconds), not `float`. Float loses precision after ~4 hours of uptime at microsecond resolution. Double is safe for months.
- The frequency is cached once at construction. SDL's performance frequency does not change at runtime, and caching eliminates a function call per frame.
- `first_tick_` ensures the first call to `tick()` returns 0.0 rather than the time since some arbitrary program start point. This prevents a large spurious delta on the first frame.

---

#### `EngineConfig` (`src/openitup/core/engine.h`, defined alongside Engine)

A plain struct passed to the Engine constructor, replacing scattered hardcoded values.

```cpp
struct EngineConfig {
    std::string window_title = "openitup";
    int window_width = 1280;
    int window_height = 960;
    double target_fps = 0.0;  // 0 = uncapped (vsync or as fast as possible)
};
```

---

#### `Engine` (`src/openitup/core/engine.h` / `src/openitup/core/engine.cpp`)

The Engine is the root of the object graph. It owns the Clock and Renderer now, with slots for AudioSystem, InputSystem, and SceneStack to be added in their respective story sets.

```cpp
// src/openitup/core/engine.h
#pragma once

#include <memory>
#include <string>

#include <openitup/core/clock.h>
#include <openitup/gfx/renderer.h>

namespace openitup {

struct EngineConfig {
    std::string window_title = "openitup";
    int window_width = 1280;
    int window_height = 960;
    double target_fps = 0.0;  // 0 = uncapped
};

class Engine {
public:
    // Fixed logic step: 1/60th second.
    static constexpr double FIXED_STEP = 1.0 / 60.0;

    // Spiral-of-death guard: cap accumulated time to this many steps.
    // At 10 steps, this allows catching up from ~167ms stalls.
    static constexpr int MAX_STEPS_PER_FRAME = 10;

    // Construct with configuration. Throws on fatal init failure.
    explicit Engine(const EngineConfig& config);

    // Construct with injectable clock (for testing).
    Engine(const EngineConfig& config, std::unique_ptr<Clock> clock);

    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Run the main loop. Blocks until quit.
    // Returns exit code (0 = normal, 1 = error).
    int run();

    // Request shutdown (callable from any subsystem or signal handler).
    void request_quit();

    // Is the engine currently running?
    bool is_running() const;

    // --- Subsystem accessors (non-owning references) ---

    Renderer& get_renderer();
    const Renderer& get_renderer() const;

    Clock& get_clock();
    const Clock& get_clock() const;

    // --- Loop state accessors (for testing/diagnostics) ---

    // Total number of logic ticks executed since run() was called.
    uint64_t tick_count() const;

    // Current render interpolation alpha [0.0, 1.0).
    double render_alpha() const;

private:
    void init_renderer(const EngineConfig& config);
    void process_events();
    void update(double dt);
    void render(double alpha);

    bool running_ = false;
    uint64_t tick_count_ = 0;
    double accumulator_ = 0.0;
    double render_alpha_ = 0.0;

    // Target frame time in seconds. 0.0 = uncapped.
    double target_frame_time_ = 0.0;

    // Subsystems — destruction order is reverse of declaration order.
    // Renderer must outlive anything that references it, so it's declared first
    // among the subsystems.
    std::unique_ptr<Clock> clock_;
    std::unique_ptr<Renderer> renderer_;

    // Future subsystems will be added here in their respective stories:
    // std::unique_ptr<AudioSystem> audio_;
    // std::unique_ptr<InputSystem> input_;
    // std::unique_ptr<SceneStack> scene_stack_;  // declared last, destroyed first
};

} // namespace openitup
```

**Key decisions**:

- The Engine takes ownership of Clock via `std::unique_ptr<Clock>`. The two-constructor pattern (default creates a real Clock, second accepts an injected one) enables testing the full game loop with synthetic time.
- `FIXED_STEP` and `MAX_STEPS_PER_FRAME` are `static constexpr` -- they are compile-time constants that the game logic depends on. Making them runtime-configurable would risk accidental desync.
- `accumulator_`, `render_alpha_`, and `tick_count_` are private state exposed read-only for testing/diagnostics.
- `request_quit()` is the only way to exit `run()`, keeping the control flow predictable.

---

### Modified Types

#### `Renderer` (`src/openitup/gfx/renderer.h`)

No interface changes needed. The Renderer's existing `init()`/`shutdown()`/`begin_frame()`/`end_frame()` API maps directly to how Engine will use it. The only change is that ownership moves from `main()` to `Engine`.

One minor improvement worth noting: `Renderer::init()` currently calls `SDL_Init(SDL_INIT_VIDEO)`. When the Engine eventually owns an AudioSystem that needs `SDL_INIT_AUDIO`, the SDL initialization should be centralized. For Phase 1, since the Engine only owns the Renderer and no audio system exists yet, the existing pattern is fine. When the AudioSystem story is implemented, `SDL_Init` should be lifted to Engine and Renderer should stop calling it.

#### `main.cpp` (`src/openitup/main.cpp`)

The current prototype loop (50+ lines of manual SDL event handling and timing) is replaced with:

```cpp
#include <openitup/core/engine.h>

#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    try {
        openitup::EngineConfig config;
        // Future: parse argc/argv for config overrides
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

## File Plan

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/openitup/core/clock.h` | Clock class declaration with injectable counter source |
| Create | `src/openitup/core/clock.cpp` | Clock implementation (tick, elapsed, reset) |
| Create | `src/openitup/core/engine.h` | Engine class + EngineConfig struct declaration |
| Create | `src/openitup/core/engine.cpp` | Engine implementation (init, run loop, shutdown, error handling) |
| Modify | `src/openitup/main.cpp` | Replace prototype loop with Engine construction + run() |
| Modify | `CMakeLists.txt` | Add `clock.cpp` and `engine.cpp` to `openitup_engine` sources |
| Create | `test/test_clock.cpp` | Unit tests for Clock (delta, elapsed, reset, precision, injection) |
| Create | `test/test_engine.cpp` | Unit tests for Engine (tick count, accumulator, spiral guard, alpha, error handling) |
| Modify | `CMakeLists.txt` | Add `test_clock.cpp` and `test_engine.cpp` to `openitup_tests` sources |

## Data Flow

### Normal Frame (60 Hz display, single logic step)

```
1. Engine::run() enters main loop iteration
2. clock_->tick() returns delta ~0.01667s (16.67ms)
3. accumulator_ += 0.01667  -> accumulator_ = 0.01667
4. accumulator_ (0.01667) >= FIXED_STEP (0.01667) -> true
5.   update(FIXED_STEP) called
6.     [scene->update(FIXED_STEP)]  // future: currently a no-op
7.     tick_count_++
8.   accumulator_ -= FIXED_STEP  -> accumulator_ ~= 0.0
9. accumulator_ (0.0) >= FIXED_STEP -> false, exit inner loop
10. render_alpha_ = accumulator_ / FIXED_STEP = 0.0
11. renderer_->begin_frame()
12. render(0.0)
13.   [scene->render(0.0)]  // future: currently a no-op
14. renderer_->end_frame()
```

### High Refresh (120 Hz display, render between logic steps)

```
Frame A (logic tick occurs):
  delta = 0.00833s (8.33ms)
  accumulator_ = 0.0 + 0.00833 = 0.00833
  0.00833 < FIXED_STEP (0.01667) -> no logic step
  alpha = 0.00833 / 0.01667 = 0.5
  render(0.5)  // interpolate halfway between previous and current state

Frame B (logic tick occurs):
  delta = 0.00833s
  accumulator_ = 0.00833 + 0.00833 = 0.01667
  0.01667 >= FIXED_STEP -> one logic step
  accumulator_ = 0.01667 - 0.01667 = 0.0
  alpha = 0.0
  render(0.0)  // render at current state exactly
```

### Spiral-of-Death Guard (500ms stall)

```
1. clock_->tick() returns delta = 0.500s (500ms stall)
2. accumulator_ += 0.500
3. steps_this_frame = 0
4. while accumulator_ >= FIXED_STEP && steps_this_frame < MAX_STEPS_PER_FRAME:
     update(FIXED_STEP)
     accumulator_ -= FIXED_STEP
     steps_this_frame++
5. After 10 iterations: 10 * 0.01667 = 0.1667s consumed
6. Remaining: 0.500 - 0.1667 = 0.3333s
7. accumulator_ = 0.3333 is DISCARDED (set to 0.0)
8. spdlog::warn("Spiral-of-death guard: discarded {:.1f}ms", 333.3)
9. Single render call with alpha = 0.0
```

### Startup Failure Path (US-ENG-063b)

```
1. main() constructs Engine(config)
2. Engine constructor calls init_renderer(config)
3. init_renderer() creates Renderer and calls renderer_->init(...)
4. If init() returns false:
     spdlog::error("Failed to initialize renderer: <SDL error>")
     throw std::runtime_error("Renderer initialization failed")
5. Exception propagates to main()
6. main() catches std::exception, logs critical message
7. return 1
```

### Exception in Update/Render (US-ENG-063a)

```
Engine::run() main loop:
  try {
      update(FIXED_STEP);
  } catch (const std::exception& e) {
      spdlog::error("Exception in update: {}", e.what());
      // Continue running -- scene recovery deferred to Phase 2
  }

  renderer_->begin_frame();
  try {
      render(render_alpha_);
  } catch (const std::exception& e) {
      spdlog::error("Exception in render: {}", e.what());
      // Continue running -- frame will show whatever was rendered before exception
  }
  renderer_->end_frame();
```

## Dependencies

### Internal
- **Renderer** (`src/openitup/gfx/renderer.h`) -- Engine owns it via `unique_ptr`, delegates `begin_frame()`/`end_frame()` calls. No modifications to its interface.
- **spdlog** -- Used for all logging in Clock and Engine. Already linked via `openitup_engine` target.
- **SDL3** -- Clock's default constructor uses `SDL_GetPerformanceCounter()` and `SDL_GetPerformanceFrequency()`. Already linked.

### External (new libraries)
None. All dependencies are already in the project.

## Key Algorithms

### Time Accumulator with Spiral-of-Death Guard

This is the core of `Engine::run()`. Pseudocode with actual types:

```cpp
void Engine::run_loop_body() {
    double delta = clock_->tick();

    accumulator_ += delta;

    // Spiral-of-death guard: if we're too far behind, give up catching up.
    double max_accumulation = FIXED_STEP * MAX_STEPS_PER_FRAME;
    if (accumulator_ > max_accumulation) {
        double discarded = accumulator_ - max_accumulation;
        spdlog::warn("Spiral-of-death guard: discarded {:.1f}ms of accumulated time",
                     discarded * 1000.0);
        accumulator_ = max_accumulation;
    }

    // Fixed-step logic updates
    while (accumulator_ >= FIXED_STEP) {
        try {
            update(FIXED_STEP);
        } catch (const std::exception& e) {
            spdlog::error("Exception in update: {}", e.what());
        }
        accumulator_ -= FIXED_STEP;
        tick_count_++;
    }

    // Render interpolation
    render_alpha_ = accumulator_ / FIXED_STEP;

    renderer_->begin_frame();
    try {
        render(render_alpha_);
    } catch (const std::exception& e) {
        spdlog::error("Exception in render: {}", e.what());
    }
    renderer_->end_frame();

    // Optional frame limiter
    if (target_frame_time_ > 0.0) {
        // Busy-wait is more precise than SDL_Delay for sub-ms accuracy.
        // SDL_Delay has ~1ms granularity on most platforms.
        // Hybrid approach: SDL_Delay for the bulk, busy-wait for the tail.
        // For Phase 1, a simple SDL_Delay is acceptable; refinement in Phase 2
        // if profiling shows issues.
        double frame_elapsed = clock_->tick();
        double remaining = target_frame_time_ - frame_elapsed;
        if (remaining > 0.001) {  // only delay if > 1ms remaining
            SDL_Delay(static_cast<uint32_t>(remaining * 1000.0));
        }
    }
}
```

**Why clamp-then-iterate instead of clamping the step count?**
Clamping `accumulator_` before the loop is simpler and prevents a subtle bug: if you count steps inside the loop and break at MAX_STEPS, the leftover accumulator carries into the next frame, where it might trigger another MAX_STEPS burst -- a rolling spiral. Clamping the accumulator to `FIXED_STEP * MAX_STEPS` guarantees at most `MAX_STEPS` iterations and discards excess cleanly.

### Render Interpolation Alpha

```
alpha = accumulator_ / FIXED_STEP
```

Where `accumulator_` is the time remaining after all logic steps have been consumed. `alpha` is always in `[0.0, 1.0)`:
- 0.0 means render at the current logic state (no interpolation needed)
- 0.5 means render halfway between previous and current logic state
- approaching 1.0 means nearly at the next logic tick

This alpha is passed to `render(alpha)` which will eventually forward it to `Scene::render(alpha)`. The scene is responsible for interpolating its visual state between the previous tick's state and the current tick's state using this alpha. This is the same pattern as the existing `interpolate_keyframes()` function in `keyframe.h` -- the BGA system already handles sub-tick interpolation via its `dt` parameter.

### Clock Precision Strategy

The Clock computes delta time using integer counter differences divided by the integer frequency:

```cpp
double Clock::tick() {
    uint64_t now = counter_fn_();
    if (first_tick_) {
        last_counter_ = now;
        first_tick_ = false;
        return 0.0;
    }
    uint64_t diff = now - last_counter_;
    last_counter_ = now;
    return static_cast<double>(diff) / static_cast<double>(frequency_value_);
}
```

**Why integer arithmetic before the division?**
If you convert each counter reading to `double` seconds and then subtract, you lose precision as the counter grows large (after hours of runtime, the counter is a huge number and the difference between two frames is tiny relative to it). By subtracting the raw `uint64_t` values first, you get a small integer difference that converts to `double` with full precision.

`elapsed()` uses the same principle:

```cpp
double Clock::elapsed() const {
    uint64_t now = counter_fn_();
    uint64_t diff = now - start_counter_;
    return static_cast<double>(diff) / static_cast<double>(frequency_value_);
}
```

## Dependency Injection Strategy

### Clock Injection

Two constructors, matching the pattern established by `TextureCache` (which takes an `ImageLoaderFn`):

```cpp
// Production code -- uses real SDL timers
Clock clock;

// Test code -- synthetic time, advances only when you say so
uint64_t fake_counter = 0;
uint64_t fake_freq = 1000000;  // 1 MHz = 1 microsecond resolution
Clock clock(
    [&]() { return fake_counter; },
    [&]() { return fake_freq; }
);
// Advance 16667 microseconds (one 60Hz frame)
fake_counter += 16667;
double delta = clock.tick();  // returns 0.016667
```

### Engine Injection

Engine accepts an optional `std::unique_ptr<Clock>` to its constructor. This allows testing the entire game loop with synthetic time:

```cpp
// Production
Engine engine(config);  // creates real Clock internally

// Test
auto fake_clock = std::make_unique<Clock>(counter_fn, freq_fn);
Engine engine(config, std::move(fake_clock));
```

### Future Subsystem Injection Points

When AudioSystem, InputSystem, and SceneStack are added, they will follow the same pattern:

```cpp
// Each subsystem gets an overloaded constructor or a setter:
Engine(const EngineConfig& config,
       std::unique_ptr<Clock> clock = nullptr,
       std::unique_ptr<Renderer> renderer = nullptr);
// Or: engine.set_scene_stack(std::move(mock_stack));
```

The specific injection mechanism for future subsystems will be decided in their respective technical designs. The important constraint is: **Engine always owns subsystems via `unique_ptr`, and subsystems access each other through non-owning references obtained from Engine**.

## Integration Points

### How Future Subsystems Connect

The Engine class is designed with explicit extension points for the subsystems planned in the roadmap:

| Subsystem | Owned By | Initialized After | Destroyed Before | How It Connects |
|-----------|----------|-------------------|------------------|----------------|
| Clock | Engine | (first) | (last) | `engine.get_clock()` |
| Renderer | Engine | Clock | SceneStack | `engine.get_renderer()` |
| AudioSystem | Engine | Renderer | InputSystem | `engine.get_audio()` (future) |
| InputSystem | Engine | AudioSystem | SceneStack | `engine.get_input()` (future) |
| SceneStack | Engine | InputSystem | (first to destroy) | `engine.get_scene_stack()` (future) |

The initialization and destruction orders follow from dependency constraints:
- Renderer needs SDL initialized (it handles this itself currently)
- AudioSystem will need SDL audio initialized
- InputSystem needs SDL events initialized
- SceneStack depends on all of the above being ready
- Destruction is reverse: SceneStack first (scenes may reference audio/renderer), then InputSystem, AudioSystem, Renderer, Clock last

### How the Game Loop Will Evolve

Phase 1 (this design): `update()` and `render()` are stubs that do nothing. The loop proves timing correctness.

Phase 1 (later stories): When InputSystem and a minimal GameplayScene are added:
```cpp
void Engine::update(double dt) {
    input_->poll();                    // US-INP stories
    scene_stack_->top().update(dt);    // US-SCN stories
}

void Engine::render(double alpha) {
    for (auto& scene : scene_stack_->visible_scenes()) {
        scene.render(alpha);           // US-SCN stories
    }
}
```

### How Render Alpha Reaches the BGA System

The existing BGA animation system (`animation.h`) renders at a float `tick` value. Currently, `main.cpp` computes `animation_time += delta * 60.0f`. In the new architecture:

1. `GameplayScene` (future) receives `alpha` in its `render(alpha)` method
2. It computes the visual tick: `visual_tick = current_logic_tick + alpha`
3. It passes `visual_tick` to `BgaAnimation::render()` as the `tick` parameter
4. The existing keyframe interpolation (`evaluate_keyframes`) handles sub-tick precision -- it already supports fractional ticks (proven by `SubTickPrecision` test in `test_keyframe_interp.cpp`)

No changes to the BGA system are needed. The alpha is consumed at the scene level and translated into the tick domain that the animation system already understands.

## Error Handling Strategy

### Startup Errors (US-ENG-063b)

The Engine constructor performs all critical initialization. If any step fails, it throws `std::runtime_error`. The `main()` function wraps construction in a try-catch that logs the error and returns exit code 1.

Specific failure points:
- **SDL_Init failure**: Renderer::init() already logs the error. Engine catches the `false` return and throws.
- **Window creation failure**: Same path through Renderer::init().
- **Renderer creation failure**: Same path.

The throw-from-constructor pattern is idiomatic C++ and ensures no half-initialized Engine object exists.

### Runtime Errors (US-ENG-063a)

The main loop wraps `update()` and `render()` in separate try-catch blocks:

- **Exception in update()**: Logged at ERROR level. The current frame's logic state may be inconsistent, but the engine continues. Scene recovery (popping the failing scene) is deferred to Phase 2 per the story's technical notes.
- **Exception in render()**: Logged at ERROR level. The frame may show partial rendering. `end_frame()` is called outside the try-catch to ensure the renderer always completes its present cycle.
- **Unknown exceptions** (`catch (...)`): Logged as "unknown exception" at ERROR level. Engine continues.

The error handling is deliberately minimal for Phase 1. It prevents crashes but does not attempt sophisticated recovery. When the SceneStack is implemented, the error handler can be enhanced to pop the failing scene and fall back to a safe state.

### What is NOT Caught

- `begin_frame()` and `end_frame()` are NOT wrapped in try-catch. If the renderer itself is broken, continuing is pointless. A renderer failure at this level is treated as fatal -- the exception propagates out of `run()` and the program exits.
- Clock failures (if `SDL_GetPerformanceCounter` returns something broken) are not caught. These are platform-level failures that cannot be recovered from.

## Architectural Decisions

### ADR-1: Double-Precision Time Throughout

- **Context**: The existing codebase uses `float` for animation ticks and delta time (see `main.cpp` line 50: `float delta = ...`). The stories require microsecond precision over 2-hour sessions (US-ENG-021 Scenario 4, US-ENG-001 Scenario 5).
- **Decision**: All time values in Clock and Engine use `double`. The interface between Engine and the existing float-based animation system (BGA ticks) converts at the boundary.
- **Alternatives considered**: (a) `float` everywhere -- fails the 2-hour precision requirement. A 32-bit float at 7200 seconds has ~0.5ms precision, well above the 50us target. (b) Fixed-point integer microseconds -- precise but awkward arithmetic and harder to test.
- **Consequences**: One `double`-to-`float` cast where render alpha meets BGA tick arithmetic. This is fine because BGA animations are typically < 60000 ticks, well within float range. The Engine's own timing math stays precise.

### ADR-2: Clamp-Then-Iterate for Spiral Guard

- **Context**: The spiral-of-death guard (US-ENG-001 Scenario 6) must cap logic steps when the system falls behind.
- **Decision**: Clamp the accumulator to `FIXED_STEP * MAX_STEPS_PER_FRAME` before the step loop. Excess time is logged and discarded.
- **Alternatives considered**: (a) Count steps in the loop and break at MAX_STEPS -- leaves excess in the accumulator, which can cause a rolling spiral in subsequent frames. (b) Track a "debt" counter that gradually pays back excess time -- adds complexity and can cause sustained micro-stuttering.
- **Consequences**: A large stall (e.g., window drag on Windows) causes a visible time skip. Game logic jumps forward by at most `MAX_STEPS_PER_FRAME` ticks, then resumes at the current wall-clock time. This is the standard behavior in most game engines (Unity, Unreal, Godot all use similar guards).

### ADR-3: Engine Owns Clock via unique_ptr (Not Composition)

- **Context**: Clock could be a direct member of Engine (composition) or a `unique_ptr` member (owned pointer).
- **Decision**: `std::unique_ptr<Clock>` so tests can inject a pre-configured Clock.
- **Alternatives considered**: (a) Direct member with template-based injection -- more complex, header-only constraint, harder to integrate with the existing non-template codebase. (b) Clock as a global/singleton -- untestable, violates ownership principles.
- **Consequences**: One heap allocation for Clock (negligible). The injectable constructor enables full game-loop testing with deterministic time.

### ADR-4: Throw from Constructor for Fatal Init Failures

- **Context**: US-ENG-063b requires clear error reporting when startup fails. The existing Renderer uses a `bool init()` pattern.
- **Decision**: Engine's constructor calls `Renderer::init()` and throws `std::runtime_error` on failure. This ensures no half-constructed Engine can reach `run()`.
- **Alternatives considered**: (a) Two-phase init (`Engine::init()` returning bool) -- caller must check and handle the false case, which is easy to forget. (b) Factory function returning `std::optional<Engine>` -- Engine is non-copyable and non-movable (owns SDL resources), so optional doesn't work cleanly.
- **Consequences**: `main()` needs a try-catch. This is already the standard C++ pattern for unrecoverable initialization errors. The Renderer's existing `bool init()` pattern is preserved internally -- Engine wraps it.

### ADR-5: Frame Limiter Uses SDL_Delay (Phase 1 Simplicity)

- **Context**: US-ENG-003 requires supporting target frame rates. Precise frame limiting is complex (hybrid sleep + busy-wait, or platform-specific timers).
- **Decision**: Phase 1 uses `SDL_Delay` for frame limiting, which has ~1ms granularity. This is acceptable for target FPS values up to ~200 Hz.
- **Alternatives considered**: (a) Busy-wait loop -- wastes CPU, bad for laptops and power consumption. (b) Hybrid `SDL_Delay` + busy-wait -- optimal but adds complexity. (c) Rely on vsync only -- doesn't support the injectable target FPS requirement.
- **Consequences**: Frame timing may jitter by up to ~1ms. For a 60 Hz target this is a 6% variation, for 144 Hz it's 14%. If profiling shows this is perceptible, the hybrid approach can be implemented as a refinement without changing any interfaces.

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| SDL_Quit in Renderer::shutdown() conflicts with future AudioSystem that also needs SDL | Med | High | Phase 1: leave SDL_Quit in Renderer. When AudioSystem story is implemented, lift SDL_Init/SDL_Quit to Engine. The Renderer interface does not change. |
| Frame limiter jitter at high refresh rates (144+ Hz) | Low | Med | Phase 1 uses simple SDL_Delay. Document that hybrid sleep+busy-wait refinement is available if needed. No interface changes required. |
| Clock precision under Windows (QPC has known issues with CPU frequency scaling on older hardware) | Low | Low | SDL wraps QPC with its own mitigation. If issues appear, the injectable Clock allows per-platform workarounds without changing Engine. |
| Accumulator uses double subtraction in a loop, which could accumulate error over millions of frames | Low | Low | The error per subtraction is ~1e-16 seconds. Over 432,000 ticks (2 hours), cumulative error is ~4.3e-11 seconds -- far below the 50us threshold. Verified by the unit test. |
| Engine constructor throws, but SDL resources were partially allocated (leak) | Med | Med | Renderer::init() is called inside Engine constructor. If it fails, the Renderer destructor is NOT called (unique_ptr is not yet fully constructed). Mitigation: Engine::init_renderer() must catch failures and clean up before throwing. OR: use two-phase init within the constructor body where the unique_ptr is already managing the Renderer. |

## Testing Strategy

### Unit Tests (`test/test_clock.cpp`) -- Pure Logic, No SDL

All Clock tests use the injectable constructor with a fake counter:

| Test | What It Verifies |
|------|-----------------|
| `FirstTickReturnsZero` | First `tick()` call returns 0.0, not time since program start |
| `DeltaTimeAccuracy` | Advancing fake counter by known amount produces correct delta |
| `ElapsedTimeTracking` | `elapsed()` returns cumulative time since construction |
| `ResetClearsElapsed` | After `reset()`, elapsed returns to zero and next tick measures from reset point |
| `PrecisionAfter2Hours` | Simulate 432,000 ticks at 16667us each. Verify cumulative error < 50us |
| `HighFrequencyCounter` | Test with realistic SDL frequency (10 GHz range on modern hardware) |
| `ZeroDeltaWhenCounterUnchanged` | If counter doesn't advance between ticks, delta is 0.0 |

### Unit Tests (`test/test_engine.cpp`) -- Engine with Injected Clock

These tests construct Engine with a fake Clock and verify loop behavior without rendering (the Renderer will fail in headless test environments, so these tests must either mock it or test only the timing logic). Two approaches:

**Approach A (preferred)**: Test the timing accumulator logic in isolation by extracting it into a testable helper, then test Engine integration only in environments where SDL is available.

**Approach B**: Create Engine in test mode where renderer initialization is skipped.

For Phase 1, we use Approach A -- the timing accumulator logic is simple enough to verify through the Clock tests plus a dedicated accumulator test:

| Test | What It Verifies |
|------|-----------------|
| `SingleStepAt60Hz` | 16.67ms delta produces exactly 1 logic step |
| `NoStepUnder16ms` | 8ms delta produces 0 logic steps, accumulator carries |
| `MultipleStepsOnSlowFrame` | 50ms delta produces 3 logic steps |
| `SpiralGuardCapsAt10Steps` | 500ms delta produces exactly 10 logic steps, excess discarded |
| `AccumulatorCarriesRemainder` | 20ms delta: 1 step, ~3.33ms remainder carried |
| `AlphaComputedCorrectly` | After partial accumulation, alpha = remainder / FIXED_STEP |
| `AlphaZeroAfterExactStep` | After exactly 16.67ms, alpha = 0.0 |
| `TickCountIncrements` | tick_count() matches expected number of logic steps |
| `TwoHourDriftTest` | 432,000 ticks via fake clock: tick_count == 432,000, total elapsed within 50us of expected |

### Integration Tests (SDL-dependent)

These require a running SDL context (display, renderer) and belong in `test_integration.cpp` or a new `test_engine_integration.cpp`:

| Test | What It Verifies |
|------|-----------------|
| `EngineStartsAndStopsCleanly` | Construct Engine, call `request_quit()` immediately, verify `run()` returns 0 |
| `RendererOwnedByEngine` | `engine.get_renderer().get()` returns a valid `SDL_Renderer*` |
| `StartupFailureExitsWithCode1` | If SDL cannot create a window (e.g., no display), constructor throws, main returns 1 |

### Test Architecture Note

The timing accumulator logic (accumulate, step, clamp, compute alpha) can be extracted into a free function or a small `GameLoop` helper class that takes a delta and returns `{num_steps, alpha}`. This would make it trivially unit-testable without needing Engine at all. If the implementer prefers this approach, the Engine simply delegates to this helper. The decision is left to the implementer as it does not affect the public API.

---

*Generated from stories in docs/stories/01-core-engine.md*
*Last updated: 2026-04-28*
