# Implementation Plan: IP-AST-021

**Story**: US-AST-021 - System asset directory structure  
**Technical Design**: TD-AST-021  
**Estimated Steps**: 5  
**Author**: delivery-coordinator  
**Date**: 2026-04-29

---

## Step 1: Add TextureCache pinning support

**Description**: Extend TextureCache to support pinning textures so they are never evicted by LRU.

**Files**:
- Modify: `src/openitup/gfx/texture_cache.h`
- Modify: `src/openitup/gfx/texture_cache.cpp`
- Create: `test/test_texture_cache_pinning.cpp`

**Changes**:
1. Add `bool pinned` field to `TextureEntry` struct, default `false`
2. Add `void pin_texture(const std::string& path)` method
3. Write unit tests verifying pinned textures exist and unpinned can be set

**Tests**:
- Pin existing texture, verify flag is set
- Pin non-existent texture, verify logs warning
- All existing TextureCache tests still pass

**Build**: Must compile.

**Commit Message**:
```
feat(gfx): add texture pinning support to TextureCache

Add pinned flag to TextureEntry to support system assets that should never
be evicted by LRU. Pinning is a precursor to US-AST-018 (LRU eviction).

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

## Step 2: Implement system directory discovery

**Description**: Create system path resolution with CLI override, env var, and search heuristic.

**Files**:
- Create: `src/openitup/core/system_paths.h`
- Create: `src/openitup/core/system_paths.cpp`
- Create: `test/test_system_paths.cpp`
- Modify: `CMakeLists.txt`

**Changes**:
1. Implement `find_system_dir()` with 5-step search heuristic
2. Use `std::filesystem::canonical()` to resolve symlinks
3. Write unit tests with temp directories simulating each search path

**Tests**:
- CLI override takes precedence
- Environment variable fallback
- Relative CWD search
- Relative binary path search
- Returns nullopt when not found

**Build**: Must compile and pass tests.

**Commit Message**:
```
feat(core): implement system asset directory discovery

Add find_system_dir() with search order: CLI flag, env var, CWD relative,
binary relative, system install path. Supports development and deployment.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

## Step 3: Implement SystemAssetManager

**Description**: Create SystemAssetManager to load and cache system sprites and animations.

**Files**:
- Create: `src/openitup/core/system_asset_manager.h`
- Create: `src/openitup/core/system_asset_manager.cpp`
- Modify: `CMakeLists.txt`

**Changes**:
1. Implement `SystemAssetManager::create()` using `find_system_dir()`
2. Implement `get_sprite()` with lazy loading and caching
3. Implement `get_animation()` with lazy loading and caching
4. Pin all loaded textures via TextureCache

**Tests**: Deferred to Step 4 (integration tests).

**Build**: Must compile.

**Commit Message**:
```
feat(core): add SystemAssetManager for engine assets

SystemAssetManager loads UI sprites, fonts, and animations from
data/system/. All textures are pinned to prevent LRU eviction.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

## Step 4: Add SystemAssetManager integration tests

**Description**: Write integration tests for SystemAssetManager with fixture assets.

**Files**:
- Create: `test/test_system_asset_manager.cpp`
- Create: `test/fixtures/data/system/sprites/test_sprite.sprj`
- Create: `test/fixtures/data/system/sprites/textures/test_texture.png`
- Modify: `CMakeLists.txt`

**Changes**:
1. Create minimal test fixtures in `test/fixtures/data/system/`
2. Write tests for successful sprite loading
3. Write tests for missing system directory (returns nullptr)
4. Write tests for missing required asset (logs ERROR)
5. Verify textures are pinned after loading

**Tests**: All integration tests pass.

**Build**: Must compile and pass all tests.

**Commit Message**:
```
test(core): add SystemAssetManager integration tests

Verify system directory discovery, sprite loading, texture pinning, and
error handling with test fixtures.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

## Step 5: Integrate SystemAssetManager into Engine

**Description**: Wire SystemAssetManager into Engine initialization and expose to scenes.

**Files**:
- Modify: `src/openitup/core/engine.h`
- Modify: `src/openitup/core/engine.cpp`
- Modify: `test/test_engine.cpp` (if exists)

**Changes**:
1. Add `std::unique_ptr<SystemAssetManager> system_asset_mgr_` to Engine
2. Add `SystemAssetManager* system_assets()` accessor
3. Initialize SystemAssetManager in `Engine::initialize()`
4. Log error and fail initialization if SystemAssetManager creation fails
5. Add test verifying Engine initialization fails without system assets

**Tests**:
- Engine initializes successfully with valid system directory
- Engine fails initialization when system directory is missing

**Build**: Must compile and pass all tests.

**Commit Message**:
```
feat(core): integrate SystemAssetManager into Engine

Engine now initializes SystemAssetManager at startup and exposes it to
scenes via system_assets(). Fatal error if system assets not found.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

## Acceptance Validation

After all steps, verify story US-AST-021 acceptance criteria:

1. **Scenario 1**: System directory loaded at startup
   - ✅ Step 5 integration test

2. **Scenario 2**: Missing system assets are fatal
   - ✅ Step 4 integration test, Step 5 engine test

3. **Scenario 3**: System assets cached separately
   - ✅ Step 1 pinning, Step 4 verification

---

## Dependencies

- TextureCache (US-AST-001) ✅ DONE
- Engine (US-ENG-011) ✅ DONE
- Sprite loading (US-REN-003) ✅ DONE
- BGA loading (US-REN-010) ✅ DONE

---

## Risks

- **Missing system assets in test environment**: Mitigated by creating test fixtures in Step 4
- **Path traversal security**: Mitigated by using `std::filesystem::canonical()`
