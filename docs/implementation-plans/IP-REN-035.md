# Implementation Plan: IP-REN-035

**Story**: US-REN-035 — Note field rendering performance  
**Story Points**: 8  
**Technical Design**: Optimization of existing NoteRenderer  
**Total Steps**: 3

---

## Overview

Optimize `NoteRenderer::render()` to maintain 60 FPS with 200+ visible notes by:
1. Tightening visible range calculation (currently generous)
2. Adding judged note culling
3. Caching sprite lookups per frame
4. Performance test with stress chart

---

## Step 1: Tighten visible range calculation and add judged note culling

**Files**:
- `src/openitup/render/note_renderer.cpp` (modify)
- `src/openitup/render/note_renderer.h` (modify signature if needed)

**Changes**:
1. In `NoteRenderer::render()`, replace the visible range calculation with a tighter window:
   - Currently calculates beat range with generous margins
   - New: visible_min = current_beat - (receptor_y + note_height) / pixels_per_beat
   - New: visible_max = current_beat + (480.0 - receptor_y) / pixels_per_beat
2. Add optional judged note callback/mask parameter to skip already-judged notes:
   - Modify `render()` signature: add optional `const std::vector<bool>* judged_notes` parameter (default nullptr)
   - In note loop, if judged_notes is non-null and note is judged, skip rendering

**Tests**:
- Existing tests in test_note_renderer.cpp continue passing
- Manual verification: run gameplay, note count should reduce as notes pass

**Commit message**:
```
perf(render): tighten note culling and add judged note filtering

- Reduce visible range margins to match actual viewport bounds
- Add optional judged_notes mask to NoteRenderer::render()
- Skip rendering notes that have already been judged
- Reduces draw calls during gameplay as song progresses

Relates to: US-REN-035
```

---

## Step 2: Cache sprite lookups per frame

**Files**:
- `src/openitup/render/note_renderer.cpp` (modify)

**Changes**:
1. At the top of `render()`, if skin is non-null:
   - Declare `std::array<const Sprite*, 10> sprite_cache`
   - Populate: `sprite_cache[col] = skin_->note(col)`
2. In the note rendering loop, replace `skin_->note(note.column)` calls with `sprite_cache[note.column]`
3. Do the same for receptor rendering in `render_receptors()` if needed

**Tests**:
- All existing NoteRenderer tests pass
- No visual regression in gameplay

**Commit message**:
```
perf(render): cache sprite lookups per frame in NoteRenderer

- Pre-fetch all column sprites at frame start instead of per-note
- Reduces virtual function calls during hot loop
- No visual change, purely optimization

Relates to: US-REN-035
```

---

## Step 3: Add performance stress test

**Files**:
- `test/test_note_renderer_perf.cpp` (new file)
- `CMakeLists.txt` (add new test executable)

**Changes**:
1. Create a stress test that generates a chart with 200+ notes visible simultaneously
2. Render the scene 1000 times, measure average time per render call
3. Assert: average time < 1.0ms on typical hardware (this is a sanity check, not a hard requirement)
4. Test with and without sprite rendering enabled

**Tests**:
- New test executable compiles
- Test passes on dev machine (warning if slow, not failure)

**Commit message**:
```
test(render): add performance stress test for NoteRenderer

- Generate chart with 200+ visible notes
- Measure render time over 1000 iterations
- Log average/min/max render times
- Validates US-REN-035 acceptance criteria

Relates to: US-REN-035
```

---

## Acceptance Criteria Mapping

From story US-REN-035:

| AC | Step | Verification |
|----|------|--------------|
| Scenario 1: No frame drops with 100 notes | 1, 2 | Tighter culling + caching reduces overhead |
| Scenario 2: Sprite batching for efficiency | 2 | Sprite cache reduces call overhead (batching is SDL-level, out of scope) |
| Scenario 3: CPU usage under 20% | 1, 2, 3 | Perf test logs timing, manual profiling validates |
| Scenario 4: Under 10 draw calls per frame | N/A | SDL batching is external, cache improves but doesn't directly control this |

**Note**: Full sprite batching (Scenario 4) requires SDL-level changes beyond this story scope. The optimizations here reduce per-note overhead, which is the actionable improvement within the current architecture.

---

## Dependencies

- Existing NoteRenderer implementation
- NoteSkin API for sprite access

---

## Risks

- Performance gains may vary by hardware
- Sprite batching (Scenario 4) not fully achievable without deeper SDL integration
- Accept as "best effort" optimization within current architecture

---

**Estimated total story points**: 8  
**Implementation complexity**: Medium (optimizations in hot path)
