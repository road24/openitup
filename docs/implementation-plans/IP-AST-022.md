# Implementation Plan: IP-AST-022

**Story**: US-AST-022 - Font loading for text rendering  
**Technical Design**: Use SDL3's built-in SDL_RenderDebugText for Phase 2 practicality  
**Estimated Effort**: 5 story points  
**Created**: 2026-04-29

---

## Overview

For Phase 2, implement basic text rendering using SDL3's built-in `SDL_RenderDebugText` function. This provides immediate text rendering capability without adding external dependencies. Full TTF font loading can be deferred to Phase 3 when more sophisticated UI is needed.

---

## Implementation Steps

### Step 1: Add TextRenderer class with SDL_RenderDebugText

**Files Created**:
- `src/openitup/render/text_renderer.h`
- `src/openitup/render/text_renderer.cpp`

**Changes**:
- Create `TextRenderer` class with methods:
  - `draw_text(const std::string& text, int x, int y, SDL_Color color)`
  - Simple wrapper around `SDL_RenderDebugText`
- Add to CMakeLists.txt

**Tests**:
- Integration test: Render text to offscreen target, verify pixels are non-black
- Unit test: Verify text bounds calculation (if needed)

**Commit Message**:
```
feat(render): add TextRenderer using SDL_RenderDebugText

Simple text rendering for Phase 2 using SDL3's built-in debug text.
Sufficient for combo/score display and basic UI. TTF fonts deferred
to Phase 3.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

### Step 2: Integrate TextRenderer into SystemAssetManager

**Files Modified**:
- `src/openitup/core/system_asset_manager.h`
- `src/openitup/core/system_asset_manager.cpp`

**Changes**:
- Add `std::unique_ptr<TextRenderer> text_renderer_` member
- Initialize in constructor
- Add getter: `TextRenderer* get_text_renderer()`

**Tests**:
- Unit test: SystemAssetManager provides non-null TextRenderer

**Commit Message**:
```
feat(core): integrate TextRenderer into SystemAssetManager

Make text rendering available to all scenes via system asset manager.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

### Step 3: Add tests and documentation

**Files Modified**:
- `test/test_text_renderer.cpp` (new)
- `CLAUDE.md` (document text rendering approach)

**Changes**:
- Write integration tests for text rendering
- Document Phase 2 vs Phase 3 text rendering strategy in CLAUDE.md

**Tests**:
- Test basic text rendering
- Test multi-line text
- Test empty string handling

**Commit Message**:
```
test(render): add TextRenderer integration tests

Verify basic text rendering functionality against US-AST-022
acceptance criteria using SDL_RenderDebugText.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

## Acceptance Criteria Mapping

| Scenario | Validation | Phase 2 Note |
|----------|------------|--------------|
| Default font loaded | SystemAssetManager provides TextRenderer | SDL_RenderDebugText uses built-in font |
| Text rendered correctly | Integration test renders text to screen | Debug font has limited glyph coverage, acceptable for Phase 2 |
| Missing font fallback | N/A | SDL_RenderDebugText always available |

---

## Dependencies

- SDL3 (already in use)
- US-AST-021 (system asset directory structure) — already implemented

---

## Risks & Mitigations

**Risk**: SDL_RenderDebugText has limited glyph coverage (ASCII only)  
**Mitigation**: Acceptable for Phase 2 (combo/score display). Full Unicode support deferred to Phase 3 with SDL_ttf.

**Risk**: Debug text quality may be lower than production fonts  
**Mitigation**: Sufficient for Phase 2. Upgrade to TTF in Phase 3 when UI polish becomes priority.

---

## Out of Scope

- TrueType font loading (deferred to Phase 3)
- Font size/style selection (deferred to Phase 3)
- Advanced text layout (word wrap, alignment) (deferred to Phase 3)
- Unicode/international character support (deferred to Phase 3)

---

## Phase 3 Migration Path

When Phase 3 adds SDL_ttf:
1. Add SDL_ttf to CMake FetchContent
2. Extend TextRenderer to load TTF fonts from `data/system/fonts/`
3. Keep SDL_RenderDebugText as fallback
4. Update tests for full glyph coverage
