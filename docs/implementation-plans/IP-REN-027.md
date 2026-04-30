# Implementation Plan: IP-REN-027
## Story: US-REN-027 - BGA Background During Gameplay

### Overview
Integrate BGA animation rendering into MinimalGameplayScene, driven by audio position.

### References
- **Story**: US-REN-027 in docs/stories/06-visual-rendering.md
- **Dependencies**: US-REN-010 (BGA system), audio position tracking

---

## Implementation Steps

### Step 1: Load BGA file in MinimalGameplayScene constructor
**Commit message**: `feat(scene): load BGA animation in gameplay scene`

**Changes**:
- Modify `src/openitup/scene/minimal_gameplay_scene.h`
  - Add `#include <openitup/bga/animation.h>`
  - Add optional member: `std::unique_ptr<BgaAnimation> bga_;`

- Modify `src/openitup/scene/minimal_gameplay_scene.cpp`
  - In constructor (file-based), after loading audio:
    - Probe for BGA files: `song.bga`, `Song.bga`, `SONG.BGA`, `song.bgaj`, etc.
    - If found, load with `BgaLoader` (from SystemAssetManager or direct)
    - On failure, log warning and leave bga_ = nullptr
    - Graceful degradation per US-AST-032

**Tests**:
- Integration test: MinimalGameplayScene with BGA file loads successfully
- Integration test: MinimalGameplayScene without BGA starts normally

**Build verification**: Compile, tests pass.

---

### Step 2: Render BGA in gameplay scene
**Commit message**: `feat(scene): render BGA behind note field in gameplay`

**Changes**:
- Modify `src/openitup/scene/minimal_gameplay_scene.cpp`
  - In `render()`, before rendering note field:
    - If `bga_` is not null:
      - Convert audio position (ms) to BGA tick: `tick = song_ms / (1000.0 / 60.0)`
      - Call `bga_->render(renderer, tick, texture_cache)`
    - This ensures BGA is drawn first (z-order below notes per AC scenario 1)

**Tests**:
- Manual test: gameplay with BGA shows animation behind notes
- Verify AC scenario 2: BGA synchronized to audio position

**Build verification**: Compile, run gameplay with BGA, verify sync.

---

## Acceptance Mapping

| Scenario | Verified By |
|----------|-------------|
| 1. BGA renders below notes | Step 2 (render order: BGA first, then notes) |
| 2. BGA synchronized to audio | Step 2 (tick = song_ms / 16.67) |
| 3. Missing BGA does not block | Step 1 (graceful nullptr check) |
| 4. BGA during pause (TBD) | DEFERRED — no pause scene yet |

---

## Risk Assessment

**LOW RISK**
- BGA system already tested and working
- Minimal integration surface
- Graceful degradation on missing file

---

## Estimated Effort

**3 story points** — matches story estimate.
