# Implementation Plan: IP-REN-022

**Story**: US-REN-022 - Receptor Rendering
**Dependencies**: US-REN-021 (sprite infrastructure complete)
**Estimated Effort**: 2 story points

## Overview

Complete receptor rendering by replacing the placeholder gray rectangles with sprite-based receptor backgrounds from the noteskin. The 3-layer compositing (background → press → judge) is already implemented; Layer 1 currently uses a fallback rectangle and needs sprite rendering.

## Implementation Steps

### Step 1: Create receptor sprite fixtures for testing

**Files**:
- `test/fixtures/ARROW_RECEPTOR_SINGLE.sprj`
- `test/fixtures/receptor_single_*.png` (test texture)

**Changes**:
- Generate a simple 64×64 test texture with a receptor-like pattern (outline + center dot)
- Create SPRJ referencing this texture
- This enables testing without requiring full noteskin assets

**Tests**: None (fixture generation)

**Commit Message**:
```
test(render): add receptor sprite test fixtures

Add ARROW_RECEPTOR_SINGLE.sprj fixture for testing receptor sprite
rendering in NoteRenderer.render_receptors(). Use a simple 64x64 test
pattern (white outline + center dot).

Part of US-REN-022.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

### Step 2: Replace placeholder rectangle with sprite rendering

**Files**:
- `src/openitup/render/note_renderer.cpp`

**Changes**:
- In `render_receptors()`, replace lines 146-154 (placeholder rectangle) with sprite rendering
- Use `skin_->receptor(PlayMode::SINGLE)` for single mode
- Fall back to the current placeholder only if the sprite is missing
- Use the same `noteskin_loop_t` animation as the note sprites

**Logic**:
```cpp
// Layer 1: Receptor background
if (skin_ && cache_) {
    const Sprite* receptor_sprite = skin_->receptor(PlayMode::SINGLE);
    if (receptor_sprite) {
        float t = noteskin_loop_t(global_time_ms);
        receptor_sprite->draw(renderer, *cache_, t, xform, ColorMod{}, SDL_BLENDMODE_BLEND);
    }
}
// Fallback to placeholder if sprite not available
if (!skin_ || !skin_->receptor(PlayMode::SINGLE)) {
    // ... existing placeholder code ...
}
```

**Tests**:
- Add integration test in `test_noteskin.cpp`:
  - Create a NoteSkin with a receptor sprite
  - Call `renderer.render_receptors()` with the skin
  - Verify SDL_RenderTexture is called (via mock or integration snapshot)
- Verify existing tests still pass (fallback behavior preserved)

**Commit Message**:
```
feat(render): render receptor sprites in NoteRenderer

Replace placeholder gray rectangles with sprite-based receptor rendering
in NoteRenderer.render_receptors(). Load receptor sprite from noteskin
using skin_->receptor(PlayMode::SINGLE). Preserve fallback rectangle
if sprite is missing.

Completes 3-layer receptor compositing:
- Layer 1: Receptor background sprite (this commit)
- Layer 2: Press overlay sprite (already implemented)
- Layer 3: Judge overlay sprite (already implemented)

Closes US-REN-022.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

## Acceptance Criteria Mapping

- **Scenario 1: Receptors at judgment line** → Already passing (receptor_y positioning is unchanged)
- **Scenario 2: Receptor count matches mode** → Already passing (loop over num_columns)
- **Scenario 3: Double mode receptor layout** → Deferred to US-REN-031 (Phase 5); current impl uses SINGLE mode for all

## Post-Implementation

- Update `docs/stories/STATUS.md`: mark US-REN-022 as DONE
- Run full test suite to verify no regressions
- Story meets Phase 2 goal: "looks like a game" — receptors now have proper visual polish
