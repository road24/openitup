# Implementation Plan: IP-SCN-007b

**Story**: US-SCN-007b — Full gameplay scene orchestration  
**Story Points**: 8  
**Technical Design**: Wire scene flow and upgrade MinimalGameplayScene  
**Total Steps**: 4

---

## Overview

Complete the scene flow: BootScene → TitleScene → ModeSelectScene → GameplayScene → back to TitleScene.

Wire ModeSelectScene callbacks, enable Engine to start with BootScene, handle gameplay completion transitions.

---

## Step 1: Add GameplayScene lifecycle hooks and completion callback

**Files**:
- `src/openitup/scene/minimal_gameplay_scene.h` (modify: inherit from Scene)
- `src/openitup/scene/minimal_gameplay_scene.cpp` (add Scene interface methods)

**Changes**:
1. Change `MinimalGameplayScene` to inherit from `Scene` (not just standalone class)
2. Implement Scene interface:
   - `on_enter()`: empty (init already done in constructor)
   - `on_exit()`: empty (destructor handles cleanup)
   - `on_pause()`: empty (no pause support yet)
   - `on_resume()`: empty
   - `handle_input(const InputSnapshot&)`: empty (input polled in update)
   - Keep existing `update(double dt)` and `render()` but ensure they match Scene signature
3. Add optional `SceneStack*` member to constructor (nullable, non-owning)
4. In `update()`, after song completes, if scene_stack is non-null:
   - Replace self with TitleScene (or pop, depending on desired flow)

**Tests**:
- Existing MinimalGameplayScene tests continue working
- Build passes

**Commit message**:
```
refactor(scene): make MinimalGameplayScene inherit from Scene

- Implement Scene lifecycle interface (on_enter/on_exit/etc.)
- Add optional SceneStack* for transitions after completion
- Prepare for scene stack integration in Engine::run()

Relates to: US-SCN-007b
```

---

## Step 2: Wire ModeSelectScene to push GameplayScene

**Files**:
- `src/openitup/scene/mode_select_scene.cpp` (modify confirm handler)
- `src/openitup/scene/mode_select_scene.h` (add Engine* dependency)

**Changes**:
1. Add `Engine*` member to `ModeSelectScene` (non-owning, passed in constructor)
2. In `handle_input()`, when confirm is pressed and cursor is on Single or Double:
   - Construct a `MinimalGameplayScene` with:
     - Chart path: for now, hardcoded test chart or load from config/data directory
     - Data directory: engine config or hardcoded
     - Engine subsystems: audio, input, renderer from Engine
   - Call `scene_stack_->replace(std::move(gameplay_scene))`
3. For Co-op/Battle modes, log "Not implemented" and do nothing

**Tests**:
- Build passes
- Manual test: Boot → Title → Mode Select → Single → gameplay starts

**Commit message**:
```
feat(scene): wire ModeSelectScene to launch GameplayScene

- Add Engine* dependency to ModeSelectScene
- On confirm: push MinimalGameplayScene for Single/Double
- Co-op/Battle modes remain disabled (log not implemented)
- Hardcoded test chart for Phase 2 (full song select in Phase 3)

Relates to: US-SCN-007b
```

---

## Step 3: Wire GameplayScene to return to TitleScene on completion

**Files**:
- `src/openitup/scene/minimal_gameplay_scene.cpp` (modify update completion logic)

**Changes**:
1. In `MinimalGameplayScene::update()`, in the completion path:
   - After marking `complete_ = true` and logging final results
   - If `scene_stack_` is non-null:
     - Create a new `TitleScene` (needs Renderer*, TextRenderer*, SceneStack*)
     - Call `scene_stack_->replace(std::move(title_scene))`
2. Ensure TextRenderer is accessible (may need to pass from Engine or create placeholder)

**Tests**:
- Manual test: play through a short chart, verify return to title

**Commit message**:
```
feat(scene): return to TitleScene after gameplay completion

- On song end, GameplayScene replaces itself with TitleScene
- Closes the gameplay → result → title loop for Phase 2
- Full result screen in Phase 3

Relates to: US-SCN-007b
```

---

## Step 4: Wire Engine to start with BootScene instead of run_gameplay

**Files**:
- `src/openitup/core/engine.cpp` (modify constructor and run)
- `src/main.cpp` (modify to use Engine::run() instead of run_gameplay if not in CLI mode)

**Changes**:
1. In `Engine::Engine()` constructor, after initializing scene_stack:
   - Create a `BootScene` (needs Renderer*, TextRenderer*, SceneStack*)
   - Push it onto scene_stack
2. In `main.cpp`, add logic:
   - If `--chart` CLI arg is provided: use `engine.run_gameplay()` (direct gameplay mode for testing)
   - Else: use `engine.run()` (starts with BootScene)
3. Ensure TextRenderer is available or create stub placeholder

**Tests**:
- Build passes
- `./openitup` (no args): Boot → Title → Mode Select flow
- `./openitup --chart test.ksf`: Direct gameplay mode (existing behavior)

**Commit message**:
```
feat(engine): start with BootScene by default, CLI mode for direct gameplay

- Engine::run() now starts with BootScene instead of empty stack
- main.cpp: --chart arg uses run_gameplay(), else uses run()
- Completes Phase 2 scene flow: Boot → Title → Mode → Gameplay → Title

Relates to: US-SCN-007b
Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

## Acceptance Criteria Mapping

From story US-SCN-007b:

| AC | Step | Verification |
|----|------|--------------|
| Scenario 1: BGA renders behind note field | Existing | Already implemented in MinimalGameplayScene |
| Scenario 2: Sprite-based notes replace placeholders | Existing | Already implemented (Phase 2 DONE) |
| Scenario 3: Combo displayed as sprite numbers | Existing | Already implemented (Phase 2 DONE) |
| Scenario 4: Scene transitions to result on completion | 3 | GameplayScene → TitleScene transition (result screen in Phase 3) |
| Scenario 5: Scene transitions to result on failure | 3 | Same as Scenario 4 (life gauge failure in Phase 3) |

**Note**: Full result screen is Phase 3 (US-SCN-008). Phase 2 transitions directly to title after completion.

---

## Dependencies

- Existing BootScene, TitleScene, ModeSelectScene (all DONE Phase 2)
- MinimalGameplayScene (DONE Phase 1)
- SceneStack infrastructure (DONE Phase 2)

---

## Risks

- TextRenderer may not be integrated yet — use placeholder or skip text rendering if needed
- Hardcoded chart path for Phase 2 (full song select in Phase 3)
- No result screen yet (acceptable for Phase 2)

---

**Estimated total story points**: 8  
**Implementation complexity**: Medium (scene flow wiring)
