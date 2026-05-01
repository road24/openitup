# Lua Game Package Structure Specification

This document defines the directory structure and manifest format for Lua-based game packages in openitup.

## Overview

A game package is a self-contained directory containing Lua screen scripts, assets, and configuration. Game packages allow different PIU eras (Exceed, NX, Phoenix, etc.) to coexist with version-specific behavior without engine recompilation.

## Directory Structure

```
data/games/<game_name>/
├── manifest.lua          # Game metadata and configuration
├── screens/              # Lua screen scripts
│   ├── boot.lua
│   ├── title.lua
│   ├── mode_select.lua
│   ├── song_select.lua
│   ├── result.lua
│   └── name_entry.lua
├── assets/               # Game-specific assets
│   ├── sprites/          # UI sprites (.sprj files)
│   ├── bgas/             # Background animations (.bgaj files)
│   ├── textures/         # Texture files (.png, .tga, .dds)
│   ├── fonts/            # Font files (.ttf)
│   └── sounds/           # Sound effects (.ogg, .mp3)
└── judge_profile.json    # Timing windows and scoring rules
```

## Manifest Format

The `manifest.lua` file must define a global `game` table with the following fields:

```lua
game = {
    name = "Exceed",                    -- Display name
    version = "1.0",                    -- Package version
    judge_profile = "judge_profile.json", -- Relative path to judge profile
    asset_dir = "assets/",              -- Relative path to asset directory
    initial_scene = "boot"              -- First screen to load
}
```

### Required Fields

- **name** (string): Human-readable name displayed in version selection UI
- **version** (string): Semantic version string for compatibility tracking
- **judge_profile** (string): Path to JSON file with timing windows and scoring rules, relative to game directory

### Optional Fields

- **asset_dir** (string): Directory for assets, relative to game directory (default: "assets/")
- **initial_scene** (string): First screen to load (default: "boot")
- **author** (string): Package creator credit
- **description** (string): Brief description of the game version
- **base_year** (int): Original release year of this PIU version (e.g., 2003 for Exceed)

## Screen Script Format

Each Lua file in `screens/` defines a scene with lifecycle functions:

```lua
-- screens/title.lua example

-- State persists across frames
local timer = 0.0
local attract_mode = false

-- Called when scene becomes active
function on_enter(params)
    timer = 0.0
    attract_mode = false
    audio.play_music("title.ogg", true)  -- loop=true
end

-- Called every logic tick (16.67ms at 60 TPS)
function update(dt)
    timer = timer + dt
    
    if timer > 30.0 then
        attract_mode = true
    end
    
    if input.is_pressed(PadInput.START) then
        scene.push("mode_select")
    end
end

-- Called every frame (uncapped render rate)
function render()
    renderer.draw_bga("title_screen.bgaj", 0.0, 0.0, timer)
    
    if attract_mode then
        renderer.draw_text("Press START", 320, 400)
    end
end

-- Called when scene is popped or replaced
function on_exit()
    audio.stop_music()
end
```

### Required Lifecycle Functions

- **on_enter(params)**: Initialization when scene becomes active. `params` is a table passed from the previous scene.
- **update(dt)**: Logic update called at fixed 60 TPS. `dt` is elapsed time in seconds since last tick.
- **render()**: Rendering called at display refresh rate. Should be stateless (use update() to change state).
- **on_exit()**: Cleanup when scene is removed from stack.

## Asset Path Resolution

Asset paths in Lua scripts are relative to the game's `asset_dir`. For example:

```lua
-- In games/exceed/screens/title.lua:
renderer.draw_sprite("ui/button.sprj", 100, 200)
-- Resolves to: data/games/exceed/assets/ui/button.sprj
```

The engine prepends the game directory and `asset_dir` automatically.

## Judge Profile Format

The `judge_profile.json` file defines timing windows and scoring rules:

```json
{
    "name": "Exceed",
    "windows_ms": {
        "perfect": 21,
        "great": 43,
        "good": 102,
        "bad": 135,
        "miss": 180
    },
    "scoring": {
        "perfect": 1000,
        "great": 500,
        "good": 100,
        "bad": 0,
        "miss": 0
    },
    "life": {
        "starting_hp": 40,
        "perfect_hp": 1,
        "great_hp": 0,
        "good_hp": -1,
        "bad_hp": -3,
        "miss_hp": -5
    }
}
```

See `docs/judge-profile-spec.md` for complete format specification.

## Validation

The engine validates game packages at load time:

1. **Manifest exists**: `manifest.lua` must be present
2. **Required fields**: `name`, `version`, `judge_profile` must be defined
3. **Judge profile exists**: Referenced judge profile JSON must exist and be valid
4. **Initial scene exists**: If `initial_scene` is specified, the corresponding `.lua` file must exist

Missing or invalid packages are logged at WARN level and excluded from version selection.

## Example: Minimal Game Package

```
data/games/simple/
├── manifest.lua
├── screens/
│   └── title.lua
└── judge_profile.json
```

**manifest.lua**:
```lua
game = {
    name = "Simple",
    version = "0.1",
    judge_profile = "judge_profile.json"
}
```

**screens/title.lua**:
```lua
function on_enter(params) end
function update(dt) end
function render()
    renderer.draw_text("Simple Game", 320, 240)
end
function on_exit() end
```

**judge_profile.json**:
```json
{
    "name": "Simple",
    "windows_ms": {"perfect": 21, "great": 43, "good": 102, "bad": 135, "miss": 180},
    "scoring": {"perfect": 1000, "great": 500, "good": 100, "bad": 0, "miss": 0},
    "life": {"starting_hp": 40, "perfect_hp": 1, "great_hp": 0, "good_hp": -1, "bad_hp": -3, "miss_hp": -5}
}
```

This minimal package is sufficient for the engine to recognize and load the game.

## Loading a Game Package

Game packages are loaded by name:

```cpp
// C++ engine code
game_manager.load_game("exceed");  // Loads data/games/exceed/
```

Only one game package is active at a time. Switching games reloads the manifest, judge profile, and screen scripts.

## Multiple Game Packages

Multiple game packages can coexist in `data/games/`:

```
data/games/
├── exceed/
├── nx/
└── phoenix/
```

The player selects the active game at startup or from settings. The engine persists the selection in the user profile.

## See Also

- `docs/judge-profile-spec.md` - Judge profile JSON format
- `docs/lua-api.md` - Complete Lua API reference
- `src/openitup/lua/lua_bindings.h` - C++ API binding implementation
