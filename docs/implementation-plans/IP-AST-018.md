# Implementation Plan: IP-AST-018

**Story**: US-AST-018 - Texture cache LRU eviction  
**Technical Design**: Self-contained enhancement to existing TextureCache  
**Estimated Effort**: 5 story points  
**Created**: 2026-04-29

---

## Overview

Enhance TextureCache with LRU tracking and automatic eviction when GPU memory usage exceeds a configurable threshold (default 200 MB).

---

## Implementation Steps

### Step 1: Add LRU tracking to TextureCache

**Files Modified**:
- `src/openitup/gfx/texture_cache.h`
- `src/openitup/gfx/texture_cache.cpp`

**Changes**:
- Add `uint64_t last_access_tick` to `Entry` struct
- Add `uint64_t current_tick_` member initialized to 0
- Add `size_t memory_threshold_bytes_` member (default 200 * 1024 * 1024)
- Add `size_t current_memory_usage_` member initialized to 0
- In `load()`, after cache hit: update `entries_[idx].last_access_tick = current_tick_++`
- In `load()`, after texture creation: set `entry.last_access_tick = current_tick_++`
- In `load()`, track memory: query texture properties via SDL_QueryTexture to estimate bytes per pixel, accumulate into `current_memory_usage_`
- Add constructor parameter `size_t memory_threshold_mb = 200` with default

**Tests**:
- Unit test: Load 3 textures, verify last_access_tick increments (0, 1, 2)
- Unit test: Access cached texture, verify tick updates

**Commit Message**:
```
feat(gfx): add LRU tracking to TextureCache

Track last access tick for each cached texture to support eviction.
Memory usage estimation added based on texture dimensions and format.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

### Step 2: Implement eviction logic

**Files Modified**:
- `src/openitup/gfx/texture_cache.cpp`
- `src/openitup/gfx/texture_cache.h`

**Changes**:
- Add private method: `void evict_lru_until_below_threshold(size_t bytes_needed)`
- In `load()`, before creating new texture: call `evict_lru_until_below_threshold(estimated_texture_size)`
- Eviction algorithm:
  1. Collect all unpinned entries sorted by last_access_tick (ascending)
  2. Remove oldest entries until `current_memory_usage_ + bytes_needed <= memory_threshold_bytes_`
  3. For each evicted entry: destroy SDL_Texture, remove from path_to_index_, decrement current_memory_usage_
  4. Compact entries_ vector and update path_to_index_ indices
- Log each eviction at INFO level

**Tests**:
- Integration test: Set threshold to 10 MB, load textures totaling 15 MB, verify LRU textures evicted
- Integration test: Pin a texture, trigger eviction, verify pinned texture not evicted
- Integration test: Reload evicted texture, verify cache miss and new handle

**Commit Message**:
```
feat(gfx): implement LRU eviction in TextureCache

Automatically evict least-recently-used textures when memory threshold
exceeded. Pinned textures are never evicted. Default threshold 200 MB.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

### Step 3: Add tests and memory usage API

**Files Modified**:
- `test/test_texture_cache.cpp`
- `src/openitup/gfx/texture_cache.h`
- `src/openitup/gfx/texture_cache.cpp`

**Changes**:
- Add public method: `size_t get_memory_usage_bytes() const { return current_memory_usage_; }`
- Add public method: `size_t get_memory_threshold_bytes() const { return memory_threshold_bytes_; }`
- Write comprehensive unit tests:
  - Test eviction triggered by threshold
  - Test pinned textures not evicted
  - Test evicted texture reloads as cache miss
  - Test memory usage tracking accuracy
  - Test that actively used textures (recently accessed) are not evicted

**Tests**:
- All scenarios from AC mapped to test cases
- Memory usage API returns correct values

**Commit Message**:
```
test(gfx): add LRU eviction tests for TextureCache

Verify eviction behavior, pinned texture protection, and memory tracking
accuracy against US-AST-018 acceptance criteria.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

## Acceptance Criteria Mapping

| Scenario | Validation |
|----------|------------|
| Eviction triggered by memory threshold | Test loads 50 textures exceeding threshold, verifies eviction |
| Evicted texture reloads on next use | Test evicts texture, reloads it, verifies new handle |
| Actively rendered textures not evicted | Test marks texture as recently used, verifies not evicted |

---

## Dependencies

- US-AST-001 (texture cache) — already implemented
- SDL3 texture memory APIs for size estimation

---

## Risks & Mitigations

**Risk**: SDL_QueryTexture doesn't provide exact memory usage  
**Mitigation**: Use conservative estimate (width * height * 4 bytes for RGBA). Overestimate is safe.

**Risk**: Compacting entries_ vector invalidates handles  
**Mitigation**: Use stable handle mapping. Keep entries_ index stable by marking deleted slots instead of compacting, or use handle indirection table.

---

## Out of Scope

- Configurable threshold via settings.json (future: Phase 3)
- Per-texture memory profiling UI (future: Phase 5)
- Explicit eviction hints from renderer (future enhancement)
