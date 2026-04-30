# Implementation Plan: IP-REN-024
## Story: US-REN-024 - Combo Display

### Overview
Add visual combo counter that displays current combo using sprite-based digits.

### References
- **Story**: US-REN-024 in docs/stories/06-visual-rendering.md
- **Dependencies**: GameplayState (has current_combo()), US-REN-021 (NoteSkin)

---

## Implementation Steps

### Step 1: Add ComboDisplay class
**Commit message**: `feat(render): add ComboDisplay for on-screen combo counter`

**Changes**:
- Create `src/openitup/render/combo_display.h`
  - Class `ComboDisplay` with sprite rendering for multi-digit numbers
  - Constructor: `ComboDisplay(const NoteSkin* skin, TextureCache* cache)`
  - `void render(SDL_Renderer* renderer, int combo_value, double current_time_ms)`
  - Render position: x=320 (center), y=100 (upper screen)
  - Digit rendering: extract digits, render horizontally with spacing

- Create `src/openitup/render/combo_display.cpp`
  - Extract digits from combo value (123 → "1", "2", "3")
  - For each digit, try to load sprite "combo-N.sprj" (N=0-9)
  - If sprite available, render; otherwise use fallback text/rectangle
  - Horizontal spacing: 40px between digit centers

**Tests**:
- Unit test: digit extraction (123 → [1,2,3], 5 → [5], 0 → [0])
- Integration test: verify rendering with null skin (fallback)

**Build verification**: Compile, tests pass.

---

### Step 2: Add combo digit sprites to NoteSkin
**Commit message**: `feat(render): add combo digit sprites to NoteSkin`

**Changes**:
- Modify `src/openitup/render/noteskin.h`
  - Add: `const Sprite* combo_digit(int digit) const` (digit 0-9)
  - Private: `std::array<std::unique_ptr<Sprite>, 10> combo_digits_`

- Modify `src/openitup/render/noteskin.cpp`
  - Implement getter with bounds check

- Modify `src/openitup/render/noteskin_loader.cpp`
  - Load: `combo-0.sprj` through `combo-9.sprj`

**Tests**:
- Unit test: NoteSkin::combo_digit() returns nullptr when not loaded
- No fixture needed (optional sprites)

**Build verification**: Compile, tests pass.

---

### Step 3: Integrate ComboDisplay into MinimalGameplayScene
**Commit message**: `feat(scene): integrate ComboDisplay into gameplay scene`

**Changes**:
- Modify `src/openitup/scene/minimal_gameplay_scene.h`
  - Add `#include <openitup/render/combo_display.h>`
  - Add member: `ComboDisplay combo_display_;`

- Modify `src/openitup/scene/minimal_gameplay_scene.cpp`
  - Constructor: initialize `combo_display_(nullptr, nullptr)` (fallback mode)
  - In `render()`: call `combo_display_.render(renderer, gameplay_state_.current_combo(), global_time_ms_)`

**Tests**:
- Manual: run gameplay, observe combo incrementing/resetting
- Verify AC: combo increments on Perfect/Great/Good, resets on Miss/Bad

**Build verification**: Compile, run scene, verify combo displayed.

---

## Acceptance Mapping

| Scenario | Verified By |
|----------|-------------|
| 1. Combo increments on hit | Step 3 integration (GameplayState already handles this) |
| 2. Combo resets on miss | Step 3 integration (GameplayState already handles this) |
| 3. Sprite-based number rendering | Step 1 digit extraction, Step 2 sprite loading |
| 4. Combo positioned at y=100 | Step 1 (COMBO_Y constant) |

---

## Risk Assessment

**LOW RISK**
- Builds on proven pattern (JudgmentDisplay)
- GameplayState combo logic already exists and tested
- Graceful degradation if sprites missing

---

## Estimated Effort

**3 story points** — matches story estimate. Simple feature.
