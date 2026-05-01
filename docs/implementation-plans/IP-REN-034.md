# Implementation Plan: IP-REN-034

**Story**: US-REN-034 — High Refresh Rate Rendering
**Estimate**: 5 points
**Technical Design**: Inline (no separate TD required)

## Overview

Wire render interpolation alpha from Engine → MinimalGameplayScene → NoteRenderer to enable smooth note scrolling at display refresh rates above 60Hz without introducing input lag.

**Current State**:
- Engine computes `render_alpha` in fixed-step loop
- `MinimalGameplayScene::render(double alpha)` receives but ignores alpha
- `NoteRenderer::render()` uses discrete song_position_ms (60Hz step)

**Target State**:
- NoteRenderer interpolates note Y positions using render_alpha
- Smooth scrolling at 144Hz+ displays

## Implementation Steps

### Step 1: Add render_alpha to NoteRenderer

**Files to modify**:
- `src/openitup/render/note_renderer.h`
- `src/openitup/render/note_renderer.cpp`

**Changes**:
1. Add optional `render_alpha` parameter to `NoteRenderer::render()` method (default 0.0)
2. Apply alpha interpolation to note Y positions: `y_interpolated = y_current + (y_next - y_current) * alpha`
3. Implementation: compute beat position for current frame, add fractional beat advance based on scroll speed and alpha

**Tests**:
- Add unit test: note Y position increases smoothly with alpha from 0.0 to 1.0
- Verify alpha=0.0 matches current behavior (no regression)

**Commit message**:
```
feat(render): add render_alpha interpolation to NoteRenderer

NoteRenderer::render() now accepts optional render_alpha parameter for
sub-tick note position smoothing on high refresh displays.

When alpha > 0, note Y positions are interpolated between the current
60Hz tick and the next tick, enabling smooth 144Hz+ rendering without
affecting input timing.

Related: US-REN-034
```

### Step 2: Wire alpha through MinimalGameplayScene

**Files to modify**:
- `src/openitup/scene/minimal_gameplay_scene.cpp`

**Changes**:
1. Pass `alpha` parameter to `note_renderer_.render()` call in `MinimalGameplayScene::render()`
2. Remove `/*alpha*/` comment marker

**Tests**:
- No new tests (integration verified by existing scene tests)

**Commit message**:
```
feat(scene): wire render_alpha to NoteRenderer in MinimalGameplayScene

Pass render_alpha from scene to note renderer for sub-tick note position
interpolation on high refresh displays.

Completes: US-REN-034
```

## Testing Strategy

**Unit Tests** (Step 1):
- `test/test_note_renderer.cpp`:
  - `InterpolationAtAlphaZero`: alpha=0.0 produces same Y as discrete render
  - `InterpolationAtAlphaHalf`: alpha=0.5 produces Y midway between ticks
  - `InterpolationAtAlphaOne`: alpha=1.0 produces Y matching next tick

**Integration** (verified by existing tests):
- Engine loop already computes correct alpha
- No visual regression expected

**Acceptance Criteria Coverage**:
- Scenario 1 (144Hz rendering): verified by engine loop test
- Scenario 2 (position interpolation): unit tests
- Scenario 3 (no stuttering): manual verification
- Scenario 4 (no input lag): judge uses discrete ticks, unaffected by render alpha
