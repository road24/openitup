# Technical Design: TD-AST-021

**Story**: US-AST-021 - System asset directory structure  
**Author**: delivery-coordinator  
**Date**: 2026-04-29  
**Status**: APPROVED

---

## Overview

This design establishes the system asset directory (`data/system/`) for engine-owned assets (UI sprites, fonts, judgment sounds, menu BGAs) separate from user-provided song content.

---

## Architecture Decisions

### ADR-001: Compile-Time vs Runtime System Asset Path

**Decision**: System asset path is determined at runtime via search heuristic, with `--system-dir` override.

**Rationale**:
- Supports development (build tree) and installation (system paths)
- Allows portable game packages without recompilation
- Consistent with `--data-dir` user content pattern

**Alternatives Rejected**:
- Hardcode relative to binary: breaks when binary is symlinked
- CMake install manifest: requires installation, complicates development

**Search Order**:
1. `--system-dir` CLI argument (if provided)
2. `OPENITUP_SYSTEM_DIR` environment variable (if set)
3. `./data/system/` relative to CWD
4. `../data/system/` relative to binary location (handles build tree)
5. `/usr/share/openitup/data/system/` (Linux install path)

---

### ADR-002: System Asset Cache Pinning

**Decision**: System textures are marked as pinned in TextureCache and never evicted by LRU.

**Rationale**:
- System textures (UI elements, fonts, judgment sprites) are used continuously
- Evicting them would cause immediate reload → thrashing
- Small memory cost (~10 MB) for guaranteed availability

**Implementation**: Add `pinned` flag to TextureCache entries. LRU eviction skips pinned entries.

---

### ADR-003: System Asset Directory Structure

```
data/system/
├── fonts/
│   └── default.ttf           # Default UI font (required)
├── sprites/
│   ├── menu.sprj             # Menu UI elements
│   ├── judgment.sprj         # Judgment display sprites (PERFECT/GREAT/GOOD/BAD/MISS)
│   ├── receptor.sprj         # Default receptor arrows
│   ├── combo.sprj            # Combo display digits
│   └── textures/             # Texture files referenced by sprites
│       ├── menu_bg.png
│       ├── judgment.png
│       └── ...
├── sfx/                      # Phase 3: Sound effects
│   ├── key.wav
│   ├── perfect.wav
│   ├── miss.wav
│   └── cursor.wav
└── animations/               # Phase 2+: Menu/title BGAs
    ├── title.bgaj
    └── boot_logo.bgaj
```

---

## Component Design

### 1. SystemAssetManager Class

**Responsibility**: Locate, load, and cache system assets.

**Interface**:
```cpp
class SystemAssetManager {
public:
    // Initialization
    static std::unique_ptr<SystemAssetManager> create(
        const std::filesystem::path& system_dir_override = {});
    
    // Asset retrieval
    std::shared_ptr<Sprite> get_sprite(const std::string& name);
    std::shared_ptr<BgaAnimation> get_animation(const std::string& name);
    // Font loading in US-AST-022
    
    // Query
    std::filesystem::path system_dir() const;
    bool has_sprite(const std::string& name) const;
    
private:
    std::filesystem::path system_dir_;
    std::map<std::string, std::shared_ptr<Sprite>> sprite_cache_;
    std::map<std::string, std::shared_ptr<BgaAnimation>> animation_cache_;
    TextureCache* texture_cache_;  // borrowed reference
};
```

**Load Behavior**:
- `get_sprite("judgment")` → loads `data/system/sprites/judgment.sprj`
- All textures loaded via `texture_cache_` are marked as pinned
- Missing optional assets (animations) log WARN but don't fail
- Missing required assets (default font) log ERROR and return nullptr

---

### 2. TextureCache Pinning Extension

**Changes to TextureCache**:
```cpp
struct TextureEntry {
    SDL_Texture* texture;
    int width;
    int height;
    size_t memory_bytes;
    bool pinned;  // NEW: never evict if true
    std::chrono::steady_clock::time_point last_access;
};

// New method
void TextureCache::pin_texture(const std::string& path);
```

**LRU Eviction Update** (US-AST-018):
- When evicting, skip entries where `pinned == true`
- If all textures are pinned and memory exceeds threshold, log ERROR but don't evict

---

### 3. System Directory Discovery

**Implementation** (`src/openitup/core/system_paths.h`):
```cpp
namespace openitup::core {

std::optional<std::filesystem::path> find_system_dir(
    const std::filesystem::path& cli_override,
    const std::filesystem::path& binary_path);

}  // namespace openitup::core
```

**Logic**:
1. If `cli_override` is non-empty and exists → return it
2. Check `OPENITUP_SYSTEM_DIR` env var
3. Check `./data/system/`
4. Check `<binary_dir>/../data/system/`
5. Check `/usr/share/openitup/data/system/` (Linux)
6. Return nullopt if none exist

---

## Integration Points

### Engine Initialization

```cpp
// In Engine::initialize()
auto system_dir_override = /* from CLI args */;
system_asset_mgr_ = SystemAssetManager::create(system_dir_override);
if (!system_asset_mgr_) {
    spdlog::error("Failed to locate system assets");
    return false;
}
```

---

### Scene Asset Access

Scenes access system assets via Engine:

```cpp
class MinimalGameplayScene : public Scene {
    void render() override {
        auto judgment_sprite = engine_->system_assets()->get_sprite("judgment");
        // Render judgment sprite
    }
};
```

---

## File Plan

### New Files

| File | Purpose |
|------|---------|
| `src/openitup/core/system_paths.h` | System directory discovery |
| `src/openitup/core/system_paths.cpp` | Search heuristic implementation |
| `src/openitup/core/system_asset_manager.h` | SystemAssetManager interface |
| `src/openitup/core/system_asset_manager.cpp` | SystemAssetManager implementation |
| `test/test_system_paths.cpp` | Unit tests for path discovery |
| `test/test_system_asset_manager.cpp` | Integration tests for asset loading |

### Modified Files

| File | Change |
|------|--------|
| `src/openitup/gfx/texture_cache.h` | Add `pinned` field to TextureEntry |
| `src/openitup/gfx/texture_cache.cpp` | Add `pin_texture()` method |
| `src/openitup/core/engine.h` | Add `SystemAssetManager* system_assets()` |
| `src/openitup/core/engine.cpp` | Initialize SystemAssetManager in `initialize()` |
| `CMakeLists.txt` | Add new source files to build |

---

## Testing Strategy

### Unit Tests

1. **System path discovery**:
   - CLI override takes precedence
   - Environment variable fallback
   - Relative CWD search
   - Relative binary path search
   - Returns nullopt when not found

2. **TextureCache pinning**:
   - Pinned textures are not evicted
   - LRU evicts unpinned textures first
   - Pinning non-existent texture logs warning

### Integration Tests

1. **SystemAssetManager initialization**:
   - Locates valid system directory
   - Logs error when system directory missing
   - Logs error when required assets missing

2. **Asset loading**:
   - Load system sprite successfully
   - Load system animation successfully
   - Textures from system assets are pinned
   - Missing optional asset logs WARN
   - Missing required asset logs ERROR

---

## Security Considerations

- **Path Traversal**: Validate that `--system-dir` does not escape into sensitive directories
- **Symlink Attacks**: Use `std::filesystem::canonical()` to resolve symlinks before loading

---

## Performance Considerations

- System asset loading happens once at startup → no runtime impact
- Pinned textures consume ~10 MB (estimated) → acceptable overhead
- Directory probing uses `std::filesystem::exists()` → <1 ms per check

---

## Acceptance Mapping

Story US-AST-021 acceptance criteria:

| Scenario | Test Coverage |
|----------|---------------|
| System directory loaded at startup | Integration test: successful init |
| Missing system assets are fatal | Integration test: missing required asset |
| System assets cached separately | Unit test: pinning prevents eviction |

---

## Future Extensions (Out of Scope)

- **US-AST-022**: Font loading via SystemAssetManager
- **US-AST-023**: Sound effect loading
- **Per-game-package system assets** (Phase 7): Multiple system directories per Lua game package
