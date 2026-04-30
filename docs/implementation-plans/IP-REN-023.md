# Implementation Plan: IP-REN-023
## Story: US-REN-023 - Judgment Display

### Overview
Add visual judgment feedback (Perfect/Great/Good/Bad/Miss sprites) that displays for 0.5 seconds after each judgment event.

### References
- **Story**: US-REN-023 in docs/stories/06-visual-rendering.md
- **Dependencies**: US-REN-021 (NoteSkin), judge system (JudgmentEvent, GameplayState)

---

## Implementation Steps

### Step 1: Add judgment sprite support to NoteSkin
**Commit message**: `feat(render): add judgment tier sprites to NoteSkin`

**Changes**:
- Modify `src/openitup/render/noteskin.h`
  - Add judgment tier sprite accessors (5 sprites: perfect, great, good, bad, miss)
  - `const Sprite* judgment_perfect() const;`
  - `const Sprite* judgment_great() const;`
  - `const Sprite* judgment_good() const;`
  - `const Sprite* judgment_bad() const;`
  - `const Sprite* judgment_miss() const;`
  - Private: `std::unique_ptr<Sprite> judgment_perfect_;` (etc.)

- Modify `src/openitup/render/noteskin_loader.cpp`
  - Load sprites from: `judge-perfect.sprj`, `judge-great.sprj`, etc.
  - Graceful failure if sprites missing (log warning, leave nullptr)

**Tests**:
- Unit test: NoteSkinLoader loads judgment sprites when present
- Fixture: add minimal judgment sprite fixtures to test/fixtures/noteskins/

**Build verification**: Compile, all existing tests pass.

---

### Step 2: Upgrade JudgmentDisplay to use sprites
**Commit message**: `feat(render): upgrade JudgmentDisplay to render judgment sprites`

**Changes**:
- Modify `src/openitup/render/judgment_display.h`
  - Add constructor parameters: `const NoteSkin* skin, TextureCache* cache`
  - Store pointers as private members
  - Keep existing API: `on_judgment()`, `render()`, `is_visible()`

- Modify `src/openitup/render/judgment_display.cpp`
  - Constructor: store skin and cache pointers
  - `render()`: if skin non-null and appropriate sprite exists, render sprite
  - Otherwise fall back to colored rectangle (backward compatibility)
  - Sprite center position: x=320, y=240 (centered)
  - Use TILE mode (t=0) for static display

**Tests**:
- Unit test: JudgmentDisplay with null skin still works (rectangles)
- Unit test: JudgmentDisplay with skin renders sprites
- Update existing tests to pass nullptr for backward compat

**Build verification**: Compile, all existing tests pass.

---

### Step 3: Integrate sprite-based JudgmentDisplay in MinimalGameplayScene
**Commit message**: `feat(scene): wire NoteSkin into JudgmentDisplay in gameplay scene`

**Changes**:
- Modify `src/openitup/scene/minimal_gameplay_scene.h`
  - Add `NoteSkin* note_skin_` member (nullable, non-owning)
  - Add `TextureCache* texture_cache_` member (nullable, non-owning)
  - Update JudgmentDisplay constructor call to pass skin and cache

- Modify `src/openitup/scene/minimal_gameplay_scene.cpp`
  - Constructor: initialize note_skin_ and texture_cache_ from SystemAssetManager (if available)
  - Pass to JudgmentDisplay constructor
  - Keep existing on_judgment() and render() calls (API unchanged)

**Tests**:
- Manual test: load noteskin with judgment sprites, observe sprite display
- Verify fallback to rectangles if sprites missing

**Build verification**: Compile, run gameplay scene, observe sprites or rectangles.

---

## Acceptance Mapping

| Scenario | Verified By |
|----------|-------------|
| 1. Perfect judgment displayed for 0.5s | Step 1 unit test, Step 2 integration |
| 2. Miss judgment displayed | Step 2 integration (auto-miss events) |
| 3. Judgment positioned at y=receptor-80 | Step 2 (y calculation) |
| 4. Timing error display (optional) | DEFERRED — story says optional, skip for now |

---

## Risk Assessment

**LOW RISK**
- Small surface area: one new class, minimal integration
- Dependent systems (judge, noteskin) are stable and tested
- Graceful degradation if sprites missing

---

## Estimated Effort

**3 story points** — matches story estimate. Simple feature, clear acceptance criteria.
