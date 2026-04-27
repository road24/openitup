# Epic: Lua Scripting Integration

This epic delivers Lua scripting as the primary mechanism for defining game-specific screen logic, enabling multiple Pump It Up versions (Exceed, NX, Phoenix) to coexist as selectable game packages without engine recompilation.

---

## Story ID: US-LUA-001 - Integrate sol2 Lua Binding Library

**Story Card:**
> **As a** Developer
> **I want** sol2 header-only library integration into the build system
> **So that** the engine can expose C++ APIs to Lua without manual C API calls

### Description
Add sol2 to CMake FetchContent and initialize a Lua state at engine startup. The Lua VM must remain sandboxed and catch errors without crashing.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Sol2 dependency resolution**
    *   **Given** a clean build environment
    *   **When** CMake configures the project
    *   **Then** sol2 is fetched from its repository and available to include
    *   **And** the build completes without linking errors

*   **Scenario 2: Lua state initialization at startup**
    *   **Given** the engine starts
    *   **When** the Engine constructor runs
    *   **Then** a sol::state is created and stored as a member
    *   **And** standard Lua libraries are loaded (math, string, table)

*   **Scenario 3: Lua error does not crash engine**
    *   **Given** the engine is running with Lua state initialized
    *   **When** a Lua script with a syntax error is executed (e.g., `sol::state::script("invalid lua!")`)
    *   **Then** the error is caught by sol2's exception mechanism
    *   **And** the error message is logged via spdlog at ERROR level
    *   **And** the engine continues running

*   **Scenario 4: No manual Lua C API calls remain**
    *   **Given** the codebase after sol2 integration
    *   **When** searching for `lua_` prefixed function calls (e.g., `lua_pushstring`, `luaL_loadfile`)
    *   **Then** zero matches are found in engine source files (src/ directory)

### Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: REQ-ENG-002 (Engine loop must exist to own the Lua state)
*   **Implementation Notes**: Use FetchContent with `GIT_TAG v3.3.0` or later. Set `SOL2_LUA_VERSION=5.4.0`. As Definition of Done, ensure no judge timing functions are exposed to Lua (no bindings for Judge::calculate_judgment(), timing window overrides, or note timestamp modification).

**Status**: PLANNED

---

## Story ID: US-LUA-002 - Expose Input Query API to Lua

**Story Card:**
> **As a** Content Creator
> **I want** input state queries accessible from Lua
> **So that** screen scripts can react to player panel presses and menu actions

### Description
Bind the InputSnapshot structure to Lua, allowing scripts to check if a panel is currently held, was pressed this frame, or was released this frame.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Check if panel is currently held**
    *   **Given** a Lua script running during an update tick
    *   **When** the script calls `input.is_held(PadInput.P1_CENTER)`
    *   **And** Player 1 is standing on the center panel
    *   **Then** the function returns `true`

*   **Scenario 2: Check if panel was pressed this frame**
    *   **Given** a Lua script running during an update tick
    *   **When** Player 1 presses the UP_LEFT panel this frame (edge event)
    *   **And** the script calls `input.is_pressed(PadInput.P1_UP_LEFT)`
    *   **Then** the function returns `true`
    *   **And** calling `is_pressed` again on the next frame returns `false` (edge cleared)

*   **Scenario 3: Query menu actions**
    *   **Given** a title screen Lua script
    *   **When** the player presses the START button
    *   **And** the script calls `input.is_pressed(PadInput.START)`
    *   **Then** the function returns `true`
    *   **And** the script can transition to the next scene

*   **Scenario 4: Enum constants accessible in Lua**
    *   **Given** a Lua script namespace
    *   **When** the script references `PadInput.P1_DOWN_LEFT`, `PadInput.P1_UP_RIGHT`, `PadInput.BACK`
    *   **Then** each constant maps to the correct integer value from the C++ `PadInput` enum

### Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-LUA-001, REQ-ENG-001 (Input system must produce InputSnapshot)
*   **Implementation Notes**: Use `sol::state::new_usertype<InputSnapshot>()` to bind methods. Expose `is_held(int)`, `is_pressed(int)`, `is_released(int)`. Register enum via `sol::state::new_enum`.

**Status**: PLANNED

---

## Story ID: US-LUA-003 - Expose Audio Control API to Lua

**Story Card:**
> **As a** Content Creator
> **I want** audio playback control from Lua
> **So that** screens can play music, SFX, and synchronize visuals to song position

### Description
Bind audio system functions to Lua for playing, pausing, seeking music, playing one-shot SFX, and querying playback position.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Play music from Lua**
    *   **Given** a song OGG file at a known path
    *   **When** a Lua script calls `audio.play_music("path/to/song.ogg")`
    *   **Then** the audio system loads the file and begins playback
    *   **And** audio is audible through the system speakers

*   **Scenario 2: Query current playback position**
    *   **Given** music is playing for 2.5 seconds
    *   **When** a Lua script calls `audio.get_position_ms()`
    *   **Then** the function returns a value between 2400 and 2600 (allowing 100ms tolerance for frame timing)

*   **Scenario 3: Pause and seek**
    *   **Given** music is playing
    *   **When** a Lua script calls `audio.pause_music()`
    *   **Then** playback stops
    *   **When** the script calls `audio.seek_music(5000)` (seek to 5 seconds)
    *   **And** then calls `audio.play_music()` (resume)
    *   **Then** playback resumes from approximately the 5-second mark (within 100ms)

*   **Scenario 4: Play SFX without interrupting music**
    *   **Given** music is playing
    *   **When** a Lua script calls `audio.play_sfx("sounds/menu_beep.wav")`
    *   **Then** the sound effect plays simultaneously with the music
    *   **And** music playback is unaffected

*   **Scenario 5: Missing file does not crash**
    *   **Given** a Lua script attempts to play a nonexistent file
    *   **When** the script calls `audio.play_music("does_not_exist.ogg")`
    *   **Then** the function logs an error message via spdlog
    *   **And** returns `false` or nil
    *   **And** the engine continues running

### Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: US-LUA-001, REQ-AUD-001 (Audio system must exist)
*   **Implementation Notes**: Bind AudioSystem methods via `sol::state::set_function()`. Wrap file loading in try-catch to prevent exceptions from propagating to Lua.

**Status**: PLANNED

---

## Story ID: US-LUA-004 - Expose Sprite and BGA Rendering API to Lua

**Story Card:**
> **As a** Content Creator
> **I want** sprite and BGA drawing commands from Lua
> **So that** screens can compose visual elements during their render pass

### Description
Bind rendering functions to Lua for drawing sprites, playing BGA animations, and rendering text.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Draw a sprite**
    *   **Given** a SPRJ sprite loaded by the asset system
    *   **When** a Lua script calls `renderer.draw_sprite("ui/button.sprj", x, y)`
    *   **Then** the sprite is rendered at the specified position in 640×480 virtual space

*   **Scenario 2: Play a BGA animation synchronized to time**
    *   **Given** a BGAJ animation file loaded
    *   **When** a Lua script calls `renderer.play_bga("backgrounds/title.bgaj", current_tick)`
    *   **Then** the BGA renders at the specified tick (60 ticks per second timeline)
    *   **And** all layers composite in painter's algorithm order

*   **Scenario 3: Draw text with color**
    *   **Given** a loaded font
    *   **When** a Lua script calls `renderer.draw_text(text, x, y, {r=255, g=255, b=255, a=255})`
    *   **Then** the text appears in the specified color

*   **Scenario 4: Invalid asset path logs error without crashing**
    *   **Given** a Lua script attempts to draw a nonexistent sprite
    *   **When** the script calls `renderer.draw_sprite("missing.sprj", 0, 0)`
    *   **Then** an error is logged at ERROR level
    *   **And** the render call is skipped
    *   **And** the frame completes rendering

### Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: US-LUA-001, existing Renderer class, SPRJ/BGAJ loaders
*   **Implementation Notes**: Bind Renderer methods. Use sol::table for color RGBA. Cache loaded sprites/BGAs to avoid redundant file I/O per frame.

**Status**: PLANNED

---

## Story ID: US-LUA-005 - Expose Scene Stack Navigation API to Lua

**Story Card:**
> **As a** Content Creator
> **I want** scene push, pop, and replace operations from Lua
> **So that** scripts can control screen transitions and overlays

### Description
Bind scene stack operations to Lua, allowing scripts to navigate between screens and push overlays without returning to C++.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Push a new scene onto the stack**
    *   **Given** a Lua script running in the TitleScene
    *   **When** the player presses START and the script calls `scene.push("song_select", {mode="single"})`
    *   **Then** the SongSelectScene is instantiated with parameters `{mode="single"}`
    *   **And** the SongSelectScene becomes the active scene
    *   **And** TitleScene remains in the stack below it

*   **Scenario 2: Pop the current scene**
    *   **Given** a Lua script running in a PauseOverlay scene
    *   **When** the player presses BACK and the script calls `scene.pop()`
    *   **Then** the PauseOverlay is removed from the stack
    *   **And** the scene below (GameplayScene) resumes as the active scene

*   **Scenario 3: Replace the current scene**
    *   **Given** a Lua script running in BootScene
    *   **When** initialization completes and the script calls `scene.replace("title")`
    *   **Then** BootScene is removed from the stack
    *   **And** TitleScene becomes the new active scene
    *   **And** the scene stack contains only TitleScene

*   **Scenario 4: Pass parameters between scenes**
    *   **Given** a song is selected in SongSelectScene
    *   **When** the script calls `scene.push("gameplay", {chart_path="/path/to/chart.ksf", difficulty="hard"})`
    *   **Then** GameplayScene can access `params.chart_path` and `params.difficulty` in its `on_enter()` method

### Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: US-LUA-001, REQ-SCN-002 (Scene stack must exist)
*   **Implementation Notes**: Bind SceneStack methods via global functions. Parameters passed as `sol::table`. Scene names map to registered Lua scripts.

**Status**: PLANNED

---

## Story ID: US-LUA-006 - Expose Profile and Score Access API to Lua

**Story Card:**
> **As a** Content Creator
> **I want** profile data queries from Lua
> **So that** screens can display high scores, settings, and player statistics

### Description
Bind profile system methods to Lua for reading high scores, player settings, and saved data.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Retrieve high score for a chart**
    *   **Given** a player has completed a chart with a score of 985000
    *   **When** a Lua script calls `profile.get_high_score(chart_hash)`
    *   **Then** the function returns `985000`

*   **Scenario 2: Query player's preferred speed mod**
    *   **Given** the player's profile has a saved speed mod setting of `"C550"`
    *   **When** a Lua script calls `profile.get_setting("speed_mod")`
    *   **Then** the function returns `"C550"`

*   **Scenario 3: Save custom data to profile**
    *   **Given** a screen needs to persist UI state
    *   **When** the script calls `profile.set_data("last_selected_song", "Beethoven Virus")`
    *   **And** the engine exits and restarts
    *   **Then** calling `profile.get_data("last_selected_song")` returns `"Beethoven Virus"`

*   **Scenario 4: Query play statistics**
    *   **Given** a player has completed 47 songs
    *   **When** a Lua script calls `profile.get_stat("total_songs_played")`
    *   **Then** the function returns `47`

### Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-LUA-001, REQ-PRF-001 (Profile system must exist)
*   **Implementation Notes**: Bind ProfileSystem methods as read-only queries. Write operations only via explicit save functions to prevent accidental data loss.

**Status**: PLANNED

---

## Story ID: US-LUA-007 - Expose Timer Utilities to Lua

**Story Card:**
> **As a** Content Creator
> **I want** time query and delay timer functions in Lua
> **So that** screens can schedule animations, timeouts, and timed transitions

### Description
Bind clock and timer utilities to Lua for querying elapsed time and creating delayed callbacks.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Query current engine time**
    *   **Given** the engine has been running for 5.3 seconds
    *   **When** a Lua script calls `time.now()`
    *   **Then** the function returns a value between 5.2 and 5.4 (allowing frame timing tolerance)

*   **Scenario 2: Create a one-shot delay timer**
    *   **Given** a Lua script in TitleScene's `on_enter()` method
    *   **When** the script calls `time.delay(2.0, function() scene.replace("attract") end)`
    *   **Then** after 2.0 seconds (±0.05s), the callback executes
    *   **And** the scene transitions to attract mode

*   **Scenario 3: Cancel a pending timer**
    *   **Given** a timer was created with `timer_id = time.delay(5.0, callback_function)`
    *   **When** the script calls `time.cancel(timer_id)` before 5 seconds elapse
    *   **Then** the callback never executes

*   **Scenario 4: Query delta time**
    *   **Given** a Lua script in a scene's `update(dt)` method
    *   **When** the script accesses the `dt` parameter
    *   **Then** the value represents seconds since the last update (typically 0.016666 for 60 Hz)

### Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-LUA-001, REQ-ENG-001 (Clock utility must exist)
*   **Implementation Notes**: Bind Clock class methods. Timers stored in a priority queue managed by the Engine. Return timer IDs as handles for cancellation.

**Status**: PLANNED

---

## Story ID: US-LUA-008 - Enforce Per-Frame Lua Execution Budget

**Story Card:**
> **As a** Player
> **I want** Lua scripts constrained to a per-frame time budget
> **So that** poorly written scripts do not cause frame drops or input lag

### Description
Implement a timeout mechanism that kills Lua update() and render() calls exceeding 5ms per frame, logging an error and reverting to a safe fallback state.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Normal script completes within budget**
    *   **Given** a Lua script's `update()` method takes 2ms to execute
    *   **When** the engine calls the script each frame
    *   **Then** no timeout error is logged
    *   **And** the script continues running normally

*   **Scenario 2: Timeout kills long-running update()**
    *   **Given** a Lua script's `update()` method enters an infinite loop
    *   **When** execution exceeds 5ms
    *   **Then** the Lua VM is interrupted
    *   **And** an error is logged: "Lua update() exceeded 5ms budget, aborting"
    *   **And** the engine continues running without the script

*   **Scenario 3: Timeout kills long-running render()**
    *   **Given** a Lua script's `render()` method performs heavy computation
    *   **When** execution exceeds 5ms
    *   **Then** the Lua VM is interrupted
    *   **And** an error is logged: "Lua render() exceeded 5ms budget, aborting"
    *   **And** the frame completes rendering

*   **Scenario 4: Engine maintains 60 FPS despite script timeout**
    *   **Given** a Lua script times out
    *   **When** measuring frame time over 100 frames after the timeout
    *   **Then** average frame time remains below 16.67ms (60 FPS)

### Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-LUA-001, REQ-SCR-007
*   **Implementation Notes**: Use `lua_sethook()` with `LUA_MASKCOUNT` to check elapsed time every N instructions. Store start time before script call, compare in hook callback. Throw C++ exception to unwind on timeout.

**Status**: PLANNED

---

## Story ID: US-LUA-009 - Log Lua Errors with Stack Traces

**Story Card:**
> **As a** Developer
> **I want** Lua runtime errors captured with stack traces
> **So that** I can diagnose script issues without guessing where failures occur

### Description
Wrap all Lua script execution in error handlers that capture stack traces, log them via spdlog, and allow the engine to continue running.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Syntax error logged with file and line number**
    *   **Given** a Lua script `title.lua` has a syntax error on line 42 (`local x = `)
    *   **When** the engine loads the script
    *   **Then** an error is logged: "[ERROR] Lua syntax error in title.lua:42: unexpected symbol near '='"

*   **Scenario 2: Runtime error logged with call stack**
    *   **Given** a Lua script calls a nonexistent function `foo()` inside `on_enter()`
    *   **When** the scene's `on_enter()` is invoked
    *   **Then** an error is logged with a stack trace showing:
        *   `attempt to call a nil value (global 'foo')`
        *   `on_enter (title.lua:15)`
        *   `[C++] Scene::enter()`

*   **Scenario 3: Engine continues after Lua error**
    *   **Given** a Lua script throws a runtime error during `update()`
    *   **When** the error is logged
    *   **Then** the engine does not crash
    *   **And** the next frame continues processing

*   **Scenario 4: Error messages logged to file with timestamps**
    *   **Given** multiple Lua errors occur during a session
    *   **When** the session ends
    *   **Then** the log file contains timestamped entries for each error
    *   **And** each entry includes the full stack trace

### Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-LUA-001, REQ-SCR-009
*   **Implementation Notes**: Use `sol::protected_function` for all script calls. Check `sol::protected_function_result::valid()` after execution. Extract stack trace via `sol::error::what()`.

**Status**: PLANNED

---

## Story ID: US-LUA-010 - Sandbox Lua Filesystem and OS Access

**Story Card:**
> **As a** Player
> **I want** Lua scripts prevented from arbitrary filesystem or OS access
> **So that** malicious or buggy scripts cannot damage my system

### Description
Disable or sandbox Lua's `io` and `os` libraries, restricting file and network access to engine-provided APIs only.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Lua io.open() is unavailable**
    *   **Given** a Lua script attempts to open a file directly
    *   **When** the script calls `io.open("/etc/passwd", "r")`
    *   **Then** the function is nil or throws an error
    *   **And** no file handle is returned

*   **Scenario 2: Lua os.execute() is unavailable**
    *   **Given** a Lua script attempts to execute a shell command
    *   **When** the script calls `os.execute("rm -rf /")`
    *   **Then** the function is nil or throws an error
    *   **And** no command is executed

*   **Scenario 3: Engine-provided file API works**
    *   **Given** the engine provides a whitelisted file reading API
    *   **When** a Lua script calls `engine.read_file("data/settings.json")`
    *   **Then** the engine validates the path is within allowed directories
    *   **And** returns file contents if permitted

*   **Scenario 4: Network access unavailable unless explicitly provided**
    *   **Given** a Lua script attempts to use `socket` or `http` libraries
    *   **When** the script calls `http.request("http://example.com")`
    *   **Then** the library is unavailable
    *   **And** the call fails

### Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-LUA-001, REQ-SCR-010
*   **Implementation Notes**: After `sol::state::open_libraries()`, set `io` and `os` to nil: `lua["io"] = sol::nil; lua["os"] = sol::nil;`. Provide alternative `engine.*` functions with path validation.

**Status**: PLANNED

---

## Story ID: US-LUA-011 - Define Lua Game Directory Structure

**Story Card:**
> **As a** Content Creator
> **I want** a standardized directory structure for game definitions
> **So that** I can package screens, assets, and configuration as a coherent game version

### Description
Specify the directory layout and manifest format for a Lua game package, including separate files per screen, asset references, and judge profile selection.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Valid game directory structure**
    *   **Given** a directory `games/exceed/`
    *   **When** the directory contains:
        *   `manifest.lua` (metadata: name, version, judge profile path)
        *   `screens/boot.lua`
        *   `screens/title.lua`
        *   `screens/select.lua`
        *   `assets/sprites/` (sprite files)
        *   `assets/bgas/` (BGA files)
        *   `judge_profile.json` (timing windows and rules)
    *   **Then** the engine recognizes this as a valid game package

*   **Scenario 2: Manifest contains required metadata**
    *   **Given** a `manifest.lua` file
    *   **When** the file is loaded
    *   **Then** it provides:
        *   `game.name = "Exceed"`
        *   `game.version = "1.0"`
        *   `game.judge_profile = "judge_profile.json"`
        *   `game.asset_dir = "assets/"`

*   **Scenario 3: Each screen is a separate Lua file**
    *   **Given** the `screens/` directory
    *   **When** the engine loads a scene named `"title"`
    *   **Then** it executes `screens/title.lua`
    *   **And** the script defines lifecycle functions: `on_enter()`, `update(dt)`, `render()`, `on_exit()`

*   **Scenario 4: Missing manifest logs error**
    *   **Given** a directory `games/incomplete/` without a `manifest.lua`
    *   **When** the engine attempts to load the game
    *   **Then** an error is logged: "Game directory 'incomplete' missing manifest.lua"
    *   **And** the game is not loaded

### Technical Notes & Constraints
*   **Story Points**: 1
*   **Dependencies**: US-LUA-001, REQ-SCR-004
*   **Implementation Notes**: Document specification in `docs/lua-game-structure.md`. Parser validates directory structure before loading scripts.

**Status**: PLANNED

---

## Story ID: US-LUA-012 - Implement Boot Screen in Lua

**Story Card:**
> **As a** Developer
> **I want** the boot screen implemented as a Lua script
> **So that** I can validate the Lua scene lifecycle and API bindings work end-to-end

### Description
Convert the boot screen to a Lua script as a proof-of-concept for Lua-driven scenes. The boot screen displays a logo BGA, waits 2 seconds, then transitions to the title screen.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Boot screen loads and displays BGA**
    *   **Given** the engine starts
    *   **When** the boot scene is the initial scene
    *   **Then** the boot logo BGA animation plays to completion
    *   **And** the logo animation is visible on screen

*   **Scenario 2: Boot screen transitions after delay**
    *   **Given** the boot scene has been active for 2.0 seconds
    *   **When** the timer callback executes
    *   **Then** the script calls `scene.replace("title")`
    *   **And** the title screen becomes the active scene

*   **Scenario 3: Boot screen can be skipped**
    *   **Given** the boot scene is displaying
    *   **When** the player presses START within the 2-second window
    *   **Then** the script detects the input via `input.is_pressed(PadInput.START)`
    *   **And** immediately calls `scene.replace("title")`

*   **Scenario 4: Lua error in boot screen does not crash**
    *   **Given** the `boot.lua` script has a runtime error
    *   **When** the engine attempts to execute the script
    *   **Then** the error is logged
    *   **And** the engine falls back to a hardcoded title screen

### Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-LUA-002, US-LUA-004, US-LUA-005, US-LUA-007
*   **Implementation Notes**: Create `games/default/screens/boot.lua`. Use this as the reference implementation for Lua scene structure.

**Status**: PLANNED

---

## Story ID: US-LUA-013 - Implement Title Screen in Lua

**Story Card:**
> **As a** Content Creator
> **I want** the title screen implemented as a Lua script
> **So that** different game versions can have distinct title screen behaviors without engine changes

### Description
Convert the title screen to Lua, including attract mode BGA playback, input handling for START button, and transition to mode select.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Title screen plays attract BGA loop**
    *   **Given** the title scene is active
    *   **When** 3 seconds elapse without input
    *   **Then** the attract mode BGA animation plays in a repeating loop
    *   **And** the animation restarts when it reaches the final tick

*   **Scenario 2: Player presses START to continue**
    *   **Given** the title scene is active
    *   **When** the player presses START
    *   **Then** the script detects `input.is_pressed(PadInput.START)`
    *   **And** plays a confirmation SFX
    *   **And** calls `scene.push("mode_select")`

*   **Scenario 3: Attract mode timeout transitions to demo**
    *   **Given** the title scene is active for 30 seconds without input
    *   **When** the timeout timer expires
    *   **Then** the script calls `scene.push("demo_gameplay", {demo_mode=true})`

*   **Scenario 4: Coin button displays credit count**
    *   **Given** the title scene is active
    *   **When** the player presses COIN
    *   **Then** the script increments a credit counter
    *   **And** renders the updated credit count on screen

### Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-LUA-012 (boot must work first), US-LUA-002, US-LUA-003, US-LUA-004, US-LUA-007
*   **Implementation Notes**: Create `games/default/screens/title.lua`. State machine for attract timer and input handling.

**Status**: PLANNED

---

## Story ID: US-LUA-014 - Implement Mode Select Screen in Lua

**Story Card:**
> **As a** Content Creator
> **I want** mode selection screen logic in Lua
> **So that** different game versions can offer version-specific modes without engine recompilation

### Description
Convert the mode select screen to Lua, allowing players to choose Single, Double, Co-op, or Battle modes via panel input.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Display mode options**
    *   **Given** the mode select scene is active
    *   **When** the scene renders
    *   **Then** four mode option buttons are visible (Single, Double, Co-op, Battle)

*   **Scenario 2: Navigate between modes with arrow panels**
    *   **Given** Single mode is currently highlighted
    *   **When** the player presses the RIGHT arrow panel
    *   **Then** Double mode becomes highlighted
    *   **And** a navigation SFX plays

*   **Scenario 3: Select a mode with CENTER panel**
    *   **Given** Double mode is highlighted
    *   **When** the player presses the CENTER panel
    *   **Then** the script calls `scene.push("song_select", {mode="double"})`
    *   **And** a confirmation SFX plays

*   **Scenario 4: Return to title with BACK button**
    *   **Given** the mode select scene is active
    *   **When** the player presses BACK
    *   **Then** the script calls `scene.pop()`
    *   **And** the title screen becomes active again

### Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-LUA-013 (title must push to mode_select), US-LUA-002, US-LUA-003, US-LUA-005
*   **Implementation Notes**: Create `games/default/screens/mode_select.lua`. Grid layout for button positions. Selection state persists across frames in Lua global table.

**Status**: PLANNED

---

## Story ID: US-LUA-015 - Implement Song Select Screen in Lua

**Story Card:**
> **As a** Content Creator
> **I want** song selection logic in Lua
> **So that** different game versions can customize song wheel behavior and UI layout

### Description
Convert the song select screen to Lua, including music wheel scrolling, banner display, difficulty selection, and chart preview.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Display song list in a scrollable wheel**
    *   **Given** 10 songs are available in the song database
    *   **When** the song select scene is active
    *   **Then** the script queries `songs.get_list()` and receives an array of song metadata
    *   **And** song titles are visible in a scrollable format

*   **Scenario 2: Scroll through songs with UP/DOWN panels**
    *   **Given** Song A is currently selected
    *   **When** the player presses the DOWN panel
    *   **Then** Song B becomes selected
    *   **And** the wheel scrolls smoothly (animated position update over 0.2 seconds)
    *   **And** a scroll SFX plays

*   **Scenario 3: Display banner for selected song**
    *   **Given** Song C is selected
    *   **When** the song has a banner image at `assets/banners/songc.png`
    *   **Then** the banner image is visible on screen

*   **Scenario 4: Cycle through difficulties with LEFT/RIGHT panels**
    *   **Given** a song with difficulties [Easy, Hard, Double]
    *   **When** the player presses RIGHT
    *   **Then** the difficulty selector advances from Easy to Hard
    *   **And** the difficulty name and level number are rendered

*   **Scenario 5: Confirm selection with CENTER panel**
    *   **Given** Song D is selected with difficulty Hard
    *   **When** the player presses CENTER
    *   **Then** the script calls `scene.push("gameplay", {chart_path="/path/to/chart.ksf", difficulty="hard"})`

*   **Scenario 6: Preview song audio**
    *   **Given** Song E is selected
    *   **When** the song remains selected for 1.5 seconds
    *   **Then** the script plays the song audio starting at the preview point (30-second mark)

### Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-LUA-014 (mode_select must push to song_select), US-LUA-002, US-LUA-003, US-LUA-004, US-LUA-005, US-LUA-006, REQ-AST-001 (song database)
*   **Implementation Notes**: Create `games/default/screens/song_select.lua`. Tween library for scroll animation. Preview timer cancels and restarts when selection changes.

**Status**: PLANNED

---

## Story ID: US-LUA-016 - Implement Result Screen in Lua

**Story Card:**
> **As a** Content Creator
> **I want** result screen layout and animations in Lua
> **So that** different game versions can present scores with version-specific visual styles

### Description
Convert the result screen to Lua, displaying final grade, score, combo, timing breakdown, and high score comparison.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Display final score and grade**
    *   **Given** gameplay completed with score 987500 and grade S
    *   **When** the result scene's `on_enter(params)` receives `{score=987500, grade="S"}`
    *   **Then** the grade letter and score value are visible on screen

*   **Scenario 2: Display combo statistics**
    *   **Given** gameplay ended with max combo 342
    *   **When** the result scene receives `{max_combo=342}`
    *   **Then** the max combo value is visible on screen

*   **Scenario 3: Display timing breakdown table**
    *   **Given** gameplay judgment counts are `{perfect=200, great=30, good=5, bad=2, miss=1}`
    *   **When** the result scene receives these counts
    *   **Then** all judgment counts are visible in a table format

*   **Scenario 4: Compare against previous high score**
    *   **Given** the chart's previous high score was 950000
    *   **When** the result scene queries `profile.get_high_score(chart_hash)`
    *   **And** the current score is 987500
    *   **Then** a new high score message is visible
    *   **And** plays a congratulations SFX

*   **Scenario 5: Transition to name entry if high score**
    *   **Given** the player achieved a new high score
    *   **When** 5 seconds elapse or the player presses START
    *   **Then** the script calls `scene.push("name_entry", {score=987500, chart_hash="abc123"})`

*   **Scenario 6: Return to song select if not high score**
    *   **Given** the player did not achieve a high score
    *   **When** 8 seconds elapse or the player presses START
    *   **Then** the script calls `scene.pop()` (returns to song select)

### Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: US-LUA-015 (song_select pushes to gameplay, which pushes to result), US-LUA-002, US-LUA-003, US-LUA-004, US-LUA-006
*   **Implementation Notes**: Create `games/default/screens/result.lua`. Layout positions defined in Lua, not C++. Grade thresholds read from judge profile for version consistency.

**Status**: PLANNED

---

## Story ID: US-LUA-017 - Implement Name Entry Screen in Lua

**Story Card:**
> **As a** Content Creator
> **I want** name entry logic in Lua
> **So that** different game versions can customize high score name input UI

### Description
Convert the name entry screen to Lua, allowing player name input using arcade-style letter selection. Name length constraints are enforced per US-SCN-009 (3-10 characters).

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Display character selection grid**
    *   **Given** the name entry scene is active
    *   **When** the scene renders
    *   **Then** a 26-letter alphabet grid is displayed (A-Z)
    *   **And** the currently selected letter is highlighted

*   **Scenario 2: Navigate letters with arrow panels**
    *   **Given** letter A is selected
    *   **When** the player presses RIGHT arrow panel
    *   **Then** letter B becomes selected
    *   **And** a navigation SFX plays

*   **Scenario 3: Confirm letter with CENTER panel**
    *   **Given** letter K is selected and name input is empty
    *   **When** the player presses CENTER
    *   **Then** K is appended to the name buffer
    *   **And** the updated name is visible on screen

*   **Scenario 4: Enforce minimum name length**
    *   **Given** the name buffer contains fewer than 3 characters
    *   **When** the player attempts to confirm the name
    *   **Then** the confirmation is rejected
    *   **And** an error message is displayed

*   **Scenario 5: Enforce maximum name length**
    *   **Given** the name buffer contains 10 characters (maximum per US-SCN-009)
    *   **When** the player attempts to add another letter
    *   **Then** the input is rejected
    *   **And** the name remains at 10 characters

*   **Scenario 6: Complete name entry after valid input**
    *   **Given** the name buffer contains a valid name between 3-10 characters
    *   **When** the player presses CENTER to confirm
    *   **Then** the script calls `profile.save_high_score(chart_hash, score, name)`
    *   **And** calls `scene.pop()` to return to result or song select

*   **Scenario 7: Cancel name entry with BACK button**
    *   **Given** the name entry scene is active with partial name AB
    *   **When** the player presses BACK
    *   **Then** the last character is removed
    *   **And** a delete SFX plays

*   **Scenario 8: Timeout saves default name**
    *   **Given** the name entry scene is active
    *   **When** 30 seconds elapse without completing the name
    *   **Then** the script saves the current partial name padded to minimum length
    *   **And** calls `scene.pop()`

### Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: US-LUA-016 (result pushes to name_entry), US-LUA-002, US-LUA-003, US-LUA-004, US-LUA-006, US-SCN-009 (name length constraints)
*   **Implementation Notes**: Create `games/default/screens/name_entry.lua`. Grid layout and navigation logic in Lua. Timeout timer cancels on input. Validate name length against engine-defined constraints.

**Status**: PLANNED

---

## Story ID: US-LUA-018 - Create Complete Exceed-Style Game Package

**Story Card:**
> **As a** Content Creator
> **I want** a complete Exceed-era game definition in Lua
> **So that** I can demonstrate the full screen flow with version-authentic behavior

### Description
Package all Lua screens into a complete `games/exceed/` directory with Exceed-specific judge profile, assets, and screen flow.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Directory structure is complete**
    *   **Given** the `games/exceed/` directory
    *   **When** the directory is inspected
    *   **Then** it contains:
        *   `manifest.lua` (name="Exceed", version="2003", judge_profile="exceed_judge.json")
        *   `screens/boot.lua`
        *   `screens/title.lua`
        *   `screens/mode_select.lua`
        *   `screens/song_select.lua`
        *   `screens/result.lua`
        *   `screens/name_entry.lua`
        *   `exceed_judge.json` (Exceed-era timing windows)

*   **Scenario 2: Full screen flow completes**
    *   **Given** the engine loads the `exceed` game package
    *   **When** the player progresses through boot → title → mode select → song select → gameplay → result → name entry
    *   **Then** each transition succeeds without errors
    *   **And** all screens use Exceed-specific BGAs and UI layouts

*   **Scenario 3: Judge profile matches Exceed timing**
    *   **Given** the `exceed_judge.json` file specifies Perfect window of ±21ms
    *   **When** gameplay uses this profile
    *   **Then** input timing within ±21ms of a note's time is judged as Perfect

*   **Scenario 4: Asset references resolve correctly**
    *   **Given** the manifest specifies `asset_dir = "assets/"`
    *   **When** a screen loads a sprite via `renderer.draw_sprite("ui/button.sprj", x, y)`
    *   **Then** the engine resolves the path to `games/exceed/assets/ui/button.sprj`

### Technical Notes & Constraints
*   **Story Points**: 1
*   **Dependencies**: US-LUA-012 through US-LUA-017 (all screens implemented)
*   **Implementation Notes**: This story validates the full architecture. Exceed judge profile based on documented Exceed timing data.

**Status**: PLANNED

---

## Story ID: US-LUA-019 - Create Complete NX-Style Game Package

**Story Card:**
> **As a** Content Creator
> **I want** a second complete game definition for NX-era PIU
> **So that** I can validate the multi-version architecture works independently

### Description
Create a `games/nx/` directory with NX-specific screens, judge profile, and asset references, demonstrating that multiple game versions coexist without interference.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: NX directory is independent**
    *   **Given** both `games/exceed/` and `games/nx/` exist
    *   **When** the `games/nx/` directory is inspected
    *   **Then** it contains:
        *   `manifest.lua` (name="NX", version="2006", judge_profile="nx_judge.json")
        *   All required screen Lua files
        *   `nx_judge.json` (NX-era timing windows, different from Exceed)

*   **Scenario 2: NX title screen has distinct behavior**
    *   **Given** the engine loads the `nx` game package
    *   **When** the title screen is active
    *   **Then** the NX-specific attract mode BGA plays
    *   **And** the attract timeout is 45 seconds (different from Exceed's 30 seconds)

*   **Scenario 3: NX judge profile has different timing**
    *   **Given** the `nx_judge.json` file specifies Perfect window of ±25ms (looser than Exceed)
    *   **When** gameplay uses this profile
    *   **Then** input timing within ±25ms is judged as Perfect

*   **Scenario 4: Switch between game versions at startup**
    *   **Given** the engine command-line accepts `--game <name>`
    *   **When** launched with `--game exceed`
    *   **Then** the Exceed package is loaded
    *   **When** relaunched with `--game nx`
    *   **Then** the NX package is loaded instead
    *   **And** no Exceed assets or scripts are active

### Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-LUA-018 (Exceed package must exist first)
*   **Implementation Notes**: NX judge profile based on documented NX timing data. Validate that changing `--game` flag fully reloads Lua state.

**Status**: PLANNED

---

## Story ID: US-LUA-020 - Implement Game Version Switching at Runtime

**Story Card:**
> **As a** Player
> **I want** game version selection from an in-game menu
> **So that** I can switch between PIU versions without restarting the engine

### Description
Add a settings menu (accessible from title screen) that lists available game packages and reloads the selected package, clearing Lua state and reloading scripts.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Access game switcher from title screen**
    *   **Given** the title screen is active
    *   **When** the player presses the SELECT button
    *   **Then** the script calls `scene.push("game_switcher")`

*   **Scenario 2: Display available game packages**
    *   **Given** the `games/` directory contains `exceed/` and `nx/` subdirectories
    *   **When** the game switcher scene is active
    *   **Then** the script queries `engine.get_game_list()` and receives `["exceed", "nx"]`
    *   **And** renders both options in a menu

*   **Scenario 3: Select a different game**
    *   **Given** the current game is "exceed"
    *   **When** the player selects "nx" and presses CENTER
    *   **Then** the script calls `engine.load_game("nx")`
    *   **And** the engine clears the current Lua state
    *   **And** loads the `games/nx/manifest.lua` and screen scripts
    *   **And** transitions to the NX title screen

*   **Scenario 4: Game switch preserves profile data**
    *   **Given** the player has high scores saved under the "exceed" game
    *   **When** the player switches to "nx" and then back to "exceed"
    *   **Then** the high scores remain intact
    *   **And** are queryable via `profile.get_high_score(chart_hash)`

### Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-LUA-018, US-LUA-019
*   **Implementation Notes**: Create `games/default/screens/game_switcher.lua`. Engine owns game switching logic. Lua state cleared via `sol::state` destructor and recreation.

**Status**: PLANNED

---

## Story ID: US-LUA-021 - Implement Lua Script Hot Reloading

**Story Card:**
> **As a** Developer
> **I want** Lua scripts reloaded automatically when files change
> **So that** I can iterate on screen logic without restarting the engine

### Description
Add a file watcher that detects changes to Lua scripts and reloads them, preserving or reinitializing scene state as appropriate.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Detect script file modification**
    *   **Given** the engine is running with hot reload enabled
    *   **When** `games/exceed/screens/title.lua` is saved with changes
    *   **Then** the file watcher detects the modification within 1 second

*   **Scenario 2: Reload script and reinitialize scene**
    *   **Given** the title screen is active and `title.lua` is modified
    *   **When** the file watcher triggers a reload
    *   **Then** the engine calls the current scene's `on_exit()`
    *   **And** reloads `title.lua` via `sol::state::script_file()`
    *   **And** calls the new scene's `on_enter()`
    *   **And** the screen updates to reflect the new code

*   **Scenario 3: Syntax error does not crash during reload**
    *   **Given** the engine is running
    *   **When** a Lua file is saved with a syntax error
    *   **Then** the reload attempt logs an error: "Hot reload failed for title.lua: [syntax error details]"
    *   **And** the previous valid version of the script remains active
    *   **And** the engine continues running

*   **Scenario 4: Hot reload disabled in production builds**
    *   **Given** the engine is compiled with `CMAKE_BUILD_TYPE=Release`
    *   **When** a Lua file is modified
    *   **Then** no reload occurs
    *   **And** the file watcher is not running

### Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-LUA-018 (needs a working Lua game to test against)
*   **Implementation Notes**: Use `inotify` (Linux) or `kqueue` (BSD/macOS) for file watching. Guarded by `#ifdef DEBUG` or runtime flag. Track file modification timestamps to debounce rapid saves.

**Status**: PLANNED

---

## Story ID: US-LUA-022 - Expose Primitive Shape Drawing to Lua

**Story Card:**
> **As a** Content Creator
> **I want** primitive shape drawing functions in Lua
> **So that** screens can render debug overlays, UI elements, and simple graphics without requiring pre-rendered sprites

### Description
Bind primitive rendering functions to Lua for drawing filled/outlined rectangles, circles, and lines with specified colors. These primitives support rapid prototyping and debugging of screen layouts.

### Acceptance Criteria (Confirmation)

*   **Scenario 1: Draw a filled rectangle**
    *   **Given** a Lua script during a render pass
    *   **When** the script calls `renderer.draw_rect(x, y, width, height, {r=255, g=0, b=0, a=255}, filled=true)`
    *   **Then** a red filled rectangle is rendered at the specified position and size

*   **Scenario 2: Draw an outlined rectangle**
    *   **Given** a Lua script during a render pass
    *   **When** the script calls `renderer.draw_rect(x, y, width, height, {r=0, g=255, b=0, a=255}, filled=false)`
    *   **Then** a green outlined rectangle is rendered

*   **Scenario 3: Draw a filled circle**
    *   **Given** a Lua script during a render pass
    *   **When** the script calls `renderer.draw_circle(center_x, center_y, radius, {r=0, g=0, b=255, a=255}, filled=true)`
    *   **Then** a blue filled circle is rendered at the specified center and radius

*   **Scenario 4: Draw a line**
    *   **Given** a Lua script during a render pass
    *   **When** the script calls `renderer.draw_line(x1, y1, x2, y2, {r=255, g=255, b=255, a=255})`
    *   **Then** a white line is rendered between the two points

*   **Scenario 5: Invalid color values are clamped**
    *   **Given** a Lua script calls a draw function with color values outside 0-255
    *   **When** the script calls `renderer.draw_rect(0, 0, 100, 100, {r=300, g=-50, b=128, a=128})`
    *   **Then** the color values are clamped to valid ranges (r=255, g=0, b=128, a=128)
    *   **And** the rectangle renders without error

### Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: US-LUA-001, US-LUA-004, REQ-SCR-002
*   **Implementation Notes**: Bind SDL3 primitive drawing functions via sol2. Use `sol::table` for color RGBA. Support both filled and outlined shapes via boolean parameter. Primitives render immediately during the current render pass.

**Status**: PLANNED

---

# Non-Functional Requirements

## NFR-LUA-001: Lua Execution Performance Budget

**Requirement**: Lua update() and render() calls must each complete in under 5ms per frame on reference hardware (Intel i5-8400, 6 cores, 2.8 GHz base).

**Acceptance Criteria**:
*   **Given** a Lua screen script running on reference hardware
*   **When** profiling 1000 consecutive frames
*   **Then** 99th percentile frame time for Lua execution is below 5ms
*   **And** average frame time is below 3ms

**Status**: PLANNED

---

## NFR-LUA-002: Script Load Time

**Requirement**: Loading a complete game package (all screen scripts, manifest, and judge profile) must complete in under 500ms.

**Acceptance Criteria**:
*   **Given** a game package with 7 screen scripts and 1 judge profile
*   **When** the engine calls `engine.load_game("exceed")`
*   **Then** the operation completes in under 500ms (measured on reference hardware)
*   **And** the title screen is ready to accept input

**Status**: PLANNED

---

## NFR-LUA-003: Memory Usage Per Game Package

**Requirement**: A loaded game package must consume less than 50MB of RAM (excluding texture and audio assets, which are shared across versions).

**Acceptance Criteria**:
*   **Given** a game package is loaded
*   **When** measuring heap allocation via valgrind massif
*   **Then** Lua state, scripts, and game-specific data consume less than 50MB
*   **And** the difference between unloaded and loaded state is under 50MB

**Status**: PLANNED

---

## NFR-LUA-004: Error Recovery Time

**Requirement**: The engine must recover from a Lua runtime error within one frame (16.67ms at 60 FPS).

**Acceptance Criteria**:
*   **Given** a Lua script throws a runtime error during update()
*   **When** the error is caught
*   **Then** the next frame begins processing within 16.67ms
*   **And** the framerate does not drop below 60 FPS for more than 2 consecutive frames

**Status**: PLANNED

---

## NFR-LUA-005: Hot Reload Latency (Development Mode Only)

**Requirement**: Hot reloading a modified Lua script must complete within 2 seconds of file save.

**Acceptance Criteria**:
*   **Given** a Lua script file is saved with modifications
*   **When** the file watcher detects the change
*   **Then** the script is reloaded and the scene reinitialized within 2 seconds
*   **And** visual feedback (console log or on-screen indicator) confirms the reload

**Status**: PLANNED

---

# Story Dependencies

Cross-file dependencies for this epic:

*   **US-LUA-001** depends on REQ-ENG-002 (Engine class must exist to own Lua state)
*   **US-LUA-002** depends on REQ-ENG-001 (Input system must produce InputSnapshot)
*   **US-LUA-003** depends on REQ-AUD-001 (Audio system must exist)
*   **US-LUA-005** depends on REQ-SCN-002 (Scene stack must exist)
*   **US-LUA-006** depends on REQ-PRF-001 (Profile system must exist)
*   **US-LUA-015** depends on REQ-AST-001 (Song database must exist)

Within this epic:
*   All API binding stories (US-LUA-002 through US-LUA-007, US-LUA-022) depend on US-LUA-001
*   All screen implementation stories (US-LUA-012 through US-LUA-017) depend on their respective API bindings
*   US-LUA-018 (Exceed package) depends on US-LUA-012 through US-LUA-017
*   US-LUA-019 (NX package) depends on US-LUA-018
*   US-LUA-020 (runtime switching) depends on US-LUA-018 and US-LUA-019
*   US-LUA-021 (hot reload) depends on US-LUA-018

---

# Estimated Delivery by Phase

This epic spans two roadmap phases:

**Phase 5** (Lua Bindings Proof-of-Concept):
- US-LUA-001 through US-LUA-010: Core Lua integration, API bindings, safety features
- US-LUA-012: Boot screen as Lua PoC
- US-LUA-022: Primitive shape drawing
- Total: 13 stories (2+2+3+3+3+2+2+5+2+2+2+3 = 31 story points)

**Phase 7** (Full Lua Game Definitions):
- US-LUA-011, US-LUA-013 through US-LUA-021: Complete screen implementations, game packages, hot reload
- Total: 11 stories (1+2+2+5+3+3+1+2+5+5 = 29 story points)

**Total Epic Effort**: 24 stories, 60 story points across 2 phases.
