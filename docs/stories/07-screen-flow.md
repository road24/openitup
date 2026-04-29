# Epic: Screen Flow and Scene Management

User stories for the screen system (subsystem 7) implementing the scene stack, lifecycle, and all game flow screens from boot through gameplay to results.

---

## Story ID: US-SCN-001 - Scene Stack Core Infrastructure

**Status**: PLANNED (Phase 2)
**Estimate**: 5 points

**Story Card:**
> **As a** Developer
> **I want** a scene stack with push, pop, and replace operations
> **So that** I can compose screens with overlays and navigate between game states without destroying previous contexts

### 📝 Description
Implement the foundational `SceneStack` class that manages a vector of scene pointers with push, pop, and replace operations. The stack renders all scenes bottom-to-top (painter's algorithm) but only updates and sends input to the topmost scene. This enables pause overlays, dialog boxes, and other modal screens without destroying the underlying state.

### ✅ Acceptance Criteria

*   **Scenario 1: Push scene creates overlay**
    *   **Given** a scene stack contains one active scene
    *   **When** a new scene is pushed onto the stack
    *   **Then** the stack contains 2 scenes, the bottom scene's `on_pause()` is called, the new scene's `on_enter()` is called, and the new scene receives input

*   **Scenario 2: Pop scene returns to previous**
    *   **Given** a scene stack contains 2 scenes
    *   **When** the top scene is popped
    *   **Then** the popped scene's `on_exit()` is called, the now-topmost scene's `on_resume()` is called, and that scene receives input

*   **Scenario 3: Replace scene transitions**
    *   **Given** a scene stack contains 1 scene
    *   **When** a replacement scene is provided
    *   **Then** the old scene's `on_exit()` is called, it is removed from the stack, the new scene's `on_enter()` is called, and it becomes the top scene

*   **Scenario 4: Rendering order is bottom-to-top**
    *   **Given** a scene stack contains 3 scenes
    *   **When** `render()` is called on the stack
    *   **Then** all 3 scenes' `render()` methods are called in order from index 0 to 2

*   **Scenario 5: Only topmost scene receives updates**
    *   **Given** a scene stack contains 2 scenes
    *   **When** `update(dt)` is called on the stack
    *   **Then** only the topmost scene's `update()` method is called

*   **Scenario 6: Empty stack is valid**
    *   **Given** a scene stack is empty
    *   **When** `update()` or `render()` is called
    *   **Then** no error occurs and the operations complete immediately

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium — core infrastructure, straightforward state machine
*   **Dependencies**: US-ENG-001 (engine loop must exist to own the stack)
*   **Implementation Notes**: Use `std::vector<std::unique_ptr<Scene>>` for ownership. Scene interface should be pure virtual base class.

---

## Story ID: US-SCN-002 - Scene Lifecycle Interface

**Status**: PLANNED (Phase 2)
**Estimate**: 3 points

**Story Card:**
> **As a** Developer
> **I want** a standardized Scene interface with lifecycle hooks
> **So that** each screen can initialize resources on entry, clean up on exit, and respond correctly to pause/resume events

### 📝 Description
Define the `Scene` abstract base class with pure virtual methods: `on_enter()`, `on_exit()`, `on_pause()`, `on_resume()`, `update(dt)`, `render()`, and `handle_input(InputSnapshot)`. This contract ensures all screens follow a consistent lifecycle and the scene stack can orchestrate them correctly.

### ✅ Acceptance Criteria

*   **Scenario 1: on_enter called when scene becomes active**
    *   **Given** a scene is pushed or replaced onto an empty stack
    *   **When** the operation completes
    *   **Then** the scene's `on_enter()` method is called exactly once

*   **Scenario 2: on_exit called when scene removed**
    *   **Given** a scene is the top scene
    *   **When** it is popped or replaced
    *   **Then** the scene's `on_exit()` method is called exactly once before destruction

*   **Scenario 3: on_pause called when overlayed**
    *   **Given** a scene is the top scene
    *   **When** another scene is pushed on top
    *   **Then** the first scene's `on_pause()` method is called

*   **Scenario 4: on_resume called when overlay removed**
    *   **Given** a scene is paused with another scene on top
    *   **When** the top scene is popped
    *   **Then** the paused scene's `on_resume()` method is called

*   **Scenario 5: update and render called each frame**
    *   **Given** a scene is the topmost scene
    *   **When** the engine runs one frame
    *   **Then** `update(dt)` is called once with the frame delta, followed by `render()` once

*   **Scenario 6: handle_input receives snapshot**
    *   **Given** a scene is the topmost scene and input events occurred
    *   **When** the engine's input system produces an InputSnapshot
    *   **Then** `handle_input(InputSnapshot)` is called with the snapshot before `update()`

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Small — interface definition and contract documentation
*   **Dependencies**: US-SCN-001 (scene stack must exist to call these methods), US-INP-001 (InputSnapshot type must exist)
*   **Implementation Notes**: Header-only base class in `src/engine/scene.h`. No default implementations — all methods pure virtual.

---

## Story ID: US-SCN-003 - Boot Scene with Logo Display

**Status**: PLANNED (Phase 2)
**Estimate**: 3 points

**Story Card:**
> **As a** Player
> **I want** a boot splash screen with a logo
> **So that** I see immediate visual feedback when I launch the engine and understand initialization is in progress

### 📝 Description
Implement `BootScene` that displays a static or animated logo (either the openitup logo or a PIU version logo loaded via BGA) while the engine performs asset scanning, database initialization, and other startup tasks. Automatically transitions to `TitleScene` when initialization completes.

### ✅ Acceptance Criteria

*   **Scenario 1: Logo displays immediately on entry**
    *   **Given** the engine has just started
    *   **When** `BootScene` is entered
    *   **Then** a logo image or BGA animation is rendered centered on screen

*   **Scenario 2: Initialization completes in under 5 seconds**
    *   **Given** a typical game data directory with 50 songs
    *   **When** `BootScene` runs on typical hardware (quad-core CPU, SSD)
    *   **Then** initialization completes and transitions to title screen within 5 seconds

*   **Scenario 3: Progress indicator visible during scan**
    *   **Given** asset scanning is in progress
    *   **When** the boot scene is rendering
    *   **Then** a text indicator or progress bar shows current status (e.g., "Scanning songs: 23/50")

*   **Scenario 4: Transition to title on completion**
    *   **Given** all initialization tasks have completed
    *   **When** the next frame is processed
    *   **Then** the scene stack replaces BootScene with TitleScene

*   **Scenario 5: Fatal error displayed if initialization fails**
    *   **Given** no valid game data directory is found
    *   **When** BootScene attempts initialization
    *   **Then** an error message "No game data found. Check configuration." is displayed and the engine waits for user input to exit

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Small — mostly UI composition, initialization logic happens in other systems
*   **Dependencies**: US-SCN-001 (scene stack), US-REN-004 (BGA renderer for logo animation), US-AST-001 (asset manager for directory scanning)
*   **Implementation Notes**: Run asset scanning on a background thread to avoid blocking the render loop. Display logo at 640×480 logical center.

---

## Story ID: US-SCN-004 - Title Scene with Attract Mode Loop

**Status**: PLANNED (Phase 2)
**Estimate**: 3 points

**Story Card:**
> **As a** Player
> **I want** a title screen with looping attract mode animation
> **So that** I see an engaging visual while deciding whether to start a game session

### 📝 Description
Implement `TitleScene` that loops a BGA animation continuously and listens for coin or start input. Transitions to `ModeSelectScene` on input. After 30 seconds of inactivity, returns to `BootScene` to restart the attract loop.

### ✅ Acceptance Criteria

*   **Scenario 1: BGA loops continuously**
    *   **Given** TitleScene is active
    *   **When** the BGA animation reaches its final tick
    *   **Then** the animation resets to tick 0 and continues playing

*   **Scenario 2: Start input proceeds to mode select**
    *   **Given** TitleScene is active and the BGA has looped at least once
    *   **When** the player presses the "start" or "coin" input
    *   **Then** the scene stack replaces TitleScene with ModeSelectScene

*   **Scenario 3: Inactivity timeout returns to boot**
    *   **Given** TitleScene is active and no input has been received
    *   **When** 30 seconds have elapsed since scene entry
    *   **Then** the scene stack replaces TitleScene with BootScene

*   **Scenario 4: Audio plays during attract mode**
    *   **Given** TitleScene is active and a title theme audio file exists
    *   **When** the scene enters
    *   **Then** the audio begins playing and loops continuously

*   **Scenario 5: Any input resets inactivity timer**
    *   **Given** TitleScene is active and 20 seconds have passed
    *   **When** the player presses any button
    *   **Then** the inactivity timer resets to 0 and the 30-second timeout begins again

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Small — mostly wiring existing BGA system with input handling
*   **Dependencies**: US-SCN-001 (scene stack), US-REN-004 (BGA renderer), US-INP-002 (input snapshot), US-AUD-001 (music playback, optional)
*   **Implementation Notes**: BGA path should be configurable per game version. Inactivity timer uses accumulated `dt` from `update()` calls.

---

## Story ID: US-SCN-005 - Mode Select Scene

**Status**: PLANNED (Phase 2)
**Estimate**: 5 points

**Story Card:**
> **As a** Player
> **I want** a mode selection screen showing Single, Double, Co-op, and Battle options
> **So that** I can choose how I want to play before selecting a song

### 📝 Description
Implement `ModeSelectScene` with a visual layout displaying all four play modes. Players navigate between modes using directional input and confirm selection with a button. The chosen mode is passed to `SongSelectScene` on transition. Battle mode is visually disabled if not yet implemented.

### ✅ Acceptance Criteria

*   **Scenario 1: All modes displayed on entry**
    *   **Given** ModeSelectScene has just been entered
    *   **When** the scene renders
    *   **Then** four mode options are visible: "Single", "Double", "Co-op", "Battle"

*   **Scenario 2: Navigation moves selection cursor**
    *   **Given** the selection cursor is on "Single"
    *   **When** the player presses right directional input
    *   **Then** the cursor moves to "Double" and a selection sound effect plays

*   **Scenario 3: Confirm transitions to song select**
    *   **Given** the cursor is on "Double"
    *   **When** the player presses the confirm button
    *   **Then** the scene stack replaces ModeSelectScene with SongSelectScene, passing PlayMode::DOUBLE

*   **Scenario 4: Battle mode disabled if unavailable**
    *   **Given** Battle mode has not been implemented
    *   **When** the scene renders
    *   **Then** the "Battle" option is displayed in a dimmed or grayed-out state and cannot be selected

*   **Scenario 5: Back input returns to title**
    *   **Given** ModeSelectScene is active
    *   **When** the player presses the back button
    *   **Then** the scene stack replaces ModeSelectScene with TitleScene

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium — requires layout logic, input navigation state machine, and transition wiring
*   **Dependencies**: US-SCN-001 (scene stack), US-INP-002 (input snapshot), US-AUD-003 (SFX for selection sounds)
*   **Implementation Notes**: Use BGA backgrounds and sprite-based text for mode labels. Store selected mode in scene constructor parameter for SongSelectScene.

---

## Story ID: US-SCN-006 - Song Select Scene with Music Wheel

**Status**: PLANNED (Phase 3)
**Estimate**: 8 points

**Story Card:**
> **As a** Player
> **I want** a song selection screen with a scrolling music wheel, banner display, and difficulty selector
> **So that** I can browse available songs and choose which chart to play

### 📝 Description
Implement `SongSelectScene` that displays a vertical scrolling music wheel listing all songs from the song database. The selected song's banner/jacket image displays prominently. Difficulty tabs show available difficulties and their ratings. Preview audio plays for the selected song. Confirming selection transitions to `GameplayScene`.

### ✅ Acceptance Criteria

*   **Scenario 1: Music wheel displays all songs**
    *   **Given** the song database contains 10 songs
    *   **When** SongSelectScene enters
    *   **Then** all 10 song titles are visible in the music wheel, with the first song highlighted

*   **Scenario 2: Vertical navigation scrolls wheel**
    *   **Given** the music wheel is displaying and song 3 is selected
    *   **When** the player presses down
    *   **Then** song 4 becomes selected, the wheel scrolls, and the banner updates to song 4's banner

*   **Scenario 3: Banner displays for selected song**
    *   **Given** song "Pump Me Amadeus" is selected
    *   **When** the scene renders
    *   **Then** the banner image for "Pump Me Amadeus" is displayed at 640×240 resolution in the banner region

*   **Scenario 4: Difficulty tabs show available charts**
    *   **Given** the selected song has Normal (level 5) and Hard (level 9) difficulties
    *   **When** the scene renders
    *   **Then** two difficulty tabs are shown with labels "Normal (5)" and "Hard (9)"

*   **Scenario 5: Horizontal navigation changes difficulty**
    *   **Given** Normal difficulty is selected
    *   **When** the player presses right
    *   **Then** Hard difficulty becomes selected and the rating updates to "9"

*   **Scenario 6: Preview audio plays on selection change**
    *   **Given** song A is selected and its preview is playing
    *   **When** the player navigates to song B
    *   **Then** song A's preview stops, song B's preview loads and begins playing from its preview start point

*   **Scenario 7: Confirm transitions to gameplay**
    *   **Given** "Pump Me Amadeus" Hard difficulty is selected
    *   **When** the player presses confirm
    *   **Then** the scene stack replaces SongSelectScene with GameplayScene, passing the selected chart and play mode

*   **Scenario 8: Back input returns to mode select**
    *   **Given** SongSelectScene is active
    *   **When** the player presses back
    *   **Then** the scene stack replaces SongSelectScene with ModeSelectScene

*   **Scenario 9: Missing banner shows placeholder**
    *   **Given** a song has no banner file
    *   **When** that song is selected
    *   **Then** a placeholder image or text "No Banner" is displayed in the banner region

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Large — complex UI state, asset loading, preview management
*   **Dependencies**: US-SCN-001 (scene stack), US-AST-003 (song database), US-CHT-001 (chart loading), US-AUD-001 (preview playback)
*   **Implementation Notes**: Limit loaded banners to 5 (current + 2 up + 2 down) to conserve texture memory. Preview audio should seek to a configured preview start time (default 30 seconds into track).

---

## Story ID: US-SCN-007a - Minimal Gameplay Scene

**Status**: PLANNED (Phase 1)
**Estimate**: 5 points

**Story Card:**
> **As a** Player
> **I want** a gameplay screen that loads a chart, plays audio, renders placeholder notes, accepts keyboard input, runs the judge, and shows text-based timing feedback
> **So that** I can play a song and see how I'm doing

### 📝 Description
Implement a minimal `GameplayScene` for Phase 1 that the engine runs directly (no scene stack). Loads a chart from a CLI-provided path, plays audio synchronized to the judge, renders placeholder rectangle notes, accepts keyboard input, and displays judgment text on screen. This is the Phase 1 first playable build.

### ✅ Acceptance Criteria

*   **Scenario 1: Chart loads from CLI path**
    *   **Given** the engine is launched with `--chart /path/to/song.ksf`
    *   **When** the gameplay scene initializes
    *   **Then** the chart loads successfully and audio path is resolved relative to the chart directory

*   **Scenario 2: Audio plays synchronized to judge**
    *   **Given** the chart is loaded and audio is playing
    *   **When** the judge processes notes each tick
    *   **Then** audio position is queried and used for timing calculations

*   **Scenario 3: Placeholder notes scroll on screen**
    *   **Given** notes are defined in the chart
    *   **When** the scene renders
    *   **Then** colored rectangles appear at positions calculated from beat-to-screen conversion

*   **Scenario 4: Keyboard input produces judgments**
    *   **Given** a note is within the judgment window
    *   **When** the player presses the corresponding key
    *   **Then** a judgment is issued and timing error is calculated

*   **Scenario 5: Judgment text visible on screen**
    *   **Given** a judgment has been issued
    *   **When** the scene renders
    *   **Then** the most recent judgment ("Perfect", "Great", etc.) appears as colored text or rectangle on screen

*   **Scenario 6: Song completes without transitions**
    *   **Given** the audio has reached the end of the song
    *   **When** the scene updates
    *   **Then** the scene remains active (no scene stack transitions in Phase 1)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 points
*   **Dependencies**: US-ENG-011 (engine runs this scene directly), US-INP-021 (keyboard input), US-AUD-092 (audio playback + position), US-CHT-005 (KSF parser), US-JDG-001 (judge), US-REN-020 (placeholder note renderer)
*   **Phase**: 1
*   **Implementation Notes**: No scene stack in Phase 1. Engine::run_gameplay_scene() constructs this directly. Render order: clear → notes → judgment text. Use audio position as authoritative clock.

---

## Story ID: US-SCN-007b - Full Gameplay Scene Orchestration

**Status**: PLANNED (Phase 2)
**Estimate**: 8 points

**Story Card:**
> **As a** Player
> **I want** full gameplay visuals with BGA behind the note field, sprite-based notes, combo/judgment sprites, and transitions to result
> **So that** gameplay has visual polish and complete flow

### 📝 Description
Upgrade the minimal gameplay scene to orchestrate BGA behind the note field, sprite-based notes (replacing placeholders), sprite-based combo/judgment display, and transition to `ResultScene` when the song ends or player fails. This version uses the scene stack.

### ✅ Acceptance Criteria

*   **Scenario 1: BGA renders behind note field**
    *   **Given** the song has a BGA file
    *   **When** gameplay renders
    *   **Then** BGA is drawn first (background layer), then notes composite on top

*   **Scenario 2: Sprite-based notes replace placeholders**
    *   **Given** a note skin is loaded
    *   **When** notes are rendered
    *   **Then** sprite-based arrows appear instead of colored rectangles

*   **Scenario 3: Combo displayed as sprite numbers**
    *   **Given** the player has a combo of 42
    *   **When** the scene renders
    *   **Then** combo "42" appears using sprite-based digit graphics

*   **Scenario 4: Scene transitions to result on completion**
    *   **Given** the song has finished playing
    *   **When** the scene updates
    *   **Then** the scene stack replaces GameplayScene with ResultScene

*   **Scenario 5: Scene transitions to result on failure**
    *   **Given** the life gauge has reached 0
    *   **When** the scene updates
    *   **Then** audio stops, "FAILED" displays for 2 seconds, then transitions to ResultScene

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 8 points
*   **Dependencies**: US-SCN-001 (scene stack), US-SCN-007a (minimal gameplay scene), US-REN-021 (sprite notes), US-REN-024 (combo display), US-REN-027 (BGA during gameplay)
*   **Phase**: 2
*   **Implementation Notes**: Render order: BGA → note field → combo/judgment/life gauge. Scene stack handles transitions.

---

## Story ID: US-SCN-008 - Result Scene with Grade and Breakdown

**Status**: PLANNED (Phase 3)
**Estimate**: 5 points

**Story Card:**
> **As a** Player
> **I want** a result screen showing my grade, score, combo, and timing breakdown
> **So that** I can see how well I performed and compare against my previous attempts

### 📝 Description
Implement `ResultScene` that receives the final `GameplayState` from `GameplayScene` and displays the grade (SSS through F), final score, max combo, and judgment counts (Perfect/Great/Good/Bad/Miss). If the score is a new high score, it is saved to the player's profile. The scene transitions to `SongSelectScene` or `ModeSelectScene` on input.

### ✅ Acceptance Criteria

*   **Scenario 1: Grade displayed based on score**
    *   **Given** the player achieved a score of 950,000 points
    *   **When** ResultScene renders
    *   **Then** the grade "SS" is displayed prominently at the top of the screen

*   **Scenario 2: Score and max combo shown**
    *   **Given** the final score is 875,432 and max combo is 184
    *   **When** ResultScene renders
    *   **Then** "Score: 875,432" and "Max Combo: 184" are displayed

*   **Scenario 3: Judgment counts displayed**
    *   **Given** the player achieved 142 Perfect, 38 Great, 5 Good, 2 Bad, 1 Miss
    *   **When** ResultScene renders
    *   **Then** all five judgment counts are displayed in a breakdown table

*   **Scenario 4: High score saved to profile**
    *   **Given** the score 900,000 exceeds the player's previous high score of 850,000
    *   **When** ResultScene enters
    *   **Then** the profile is updated with the new high score and persisted to disk

*   **Scenario 5: High score indicator shown**
    *   **Given** the current score is a new high score
    *   **When** ResultScene renders
    *   **Then** a "NEW RECORD!" message is displayed near the score

*   **Scenario 6: Return to song select on confirm**
    *   **Given** ResultScene is active
    *   **When** the player presses confirm or start
    *   **Then** the scene stack replaces ResultScene with SongSelectScene

*   **Scenario 7: Failed grade shows no rank**
    *   **Given** the player failed the song (life gauge reached 0)
    *   **When** ResultScene renders
    *   **Then** the grade "F" is displayed and no score is saved

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium — primarily display logic with profile integration
*   **Dependencies**: US-SCN-001 (scene stack), US-JDG-014 (final GameplayState), US-DAT-005 (profile save)
*   **Implementation Notes**: Grade thresholds should come from the judge profile used during gameplay. Use sprite-based grade images and numbers for visual consistency.

---

## Story ID: US-SCN-009 - Name Entry Scene for High Scores

**Status**: PLANNED (Phase 5)
**Estimate**: 5 points

**Story Card:**
> **As a** Player
> **I want** a name entry screen when I achieve a new high score
> **So that** my initials or name are associated with my achievement on the leaderboard

### 📝 Description
Implement `NameEntryScene` that appears when the player achieves a new high score. Displays the current score and rank, and allows 3-10 character name entry via input navigation through a character grid. The name is saved with the high score record. If not a new high score, this scene is skipped.

### ✅ Acceptance Criteria

*   **Scenario 1: Name entry triggered only for high scores**
    *   **Given** the player's score is not in the top 10
    *   **When** ResultScene completes
    *   **Then** NameEntryScene is skipped and the flow transitions directly to SongSelectScene

*   **Scenario 2: Current score and rank displayed**
    *   **Given** the player achieved rank 3 with score 900,000
    *   **When** NameEntryScene renders
    *   **Then** "Rank: 3" and "Score: 900,000" are displayed at the top

*   **Scenario 3: Character grid navigation**
    *   **Given** the cursor is on 'A' in the character grid
    *   **When** the player presses right
    *   **Then** the cursor moves to 'B'

*   **Scenario 4: Character selection appends to name**
    *   **Given** the current name is "AB" and the cursor is on 'C'
    *   **When** the player presses confirm
    *   **Then** the name becomes "ABC" and is displayed in the name field

*   **Scenario 5: Name length constraints enforced**
    *   **Given** the current name is "ABCDEFGHIJ" (10 characters)
    *   **When** the player attempts to select another character
    *   **Then** no character is added and a visual indicator shows the name is at maximum length

*   **Scenario 6: Backspace removes last character**
    *   **Given** the current name is "ABC"
    *   **When** the player presses the back button
    *   **Then** the name becomes "AB"

*   **Scenario 7: Name saved with high score**
    *   **Given** the player has entered "ROAD24" and presses confirm
    *   **When** the final confirmation occurs
    *   **Then** the high score record is updated with name "ROAD24" and persisted to the profile

*   **Scenario 8: Minimum 3 character requirement**
    *   **Given** the current name is "AB" (2 characters)
    *   **When** the player attempts to confirm
    *   **Then** the confirmation is rejected and a message "Minimum 3 characters" is shown

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium — input state machine, character grid layout, validation
*   **Dependencies**: US-SCN-001 (scene stack), US-DAT-005 (profile high score storage)
*   **Implementation Notes**: Character grid should include A-Z, 0-9, space, and punctuation. Default to 3-letter initials if the player wants minimal entry.

---

## Story ID: US-SCN-010 - Pause Overlay Scene

**Status**: PLANNED (Phase 5)
**Estimate**: 5 points

**Story Card:**
> **As a** Player
> **I want** the ability to pause during gameplay
> **So that** I can take a break or adjust settings without losing my progress

### 📝 Description
Implement `PauseOverlayScene` that is pushed on top of `GameplayScene` when the player presses the start button during gameplay. The gameplay scene remains rendered in the background but is frozen. Audio is paused. The overlay displays options: Resume, Restart, and Quit to Select. Resuming returns to the exact gameplay position.

### ✅ Acceptance Criteria

*   **Scenario 1: Start button triggers pause**
    *   **Given** GameplayScene is active and the song is at 10.5 seconds
    *   **When** the player presses the start button
    *   **Then** PauseOverlayScene is pushed onto the stack, GameplayScene's `on_pause()` is called, and audio pauses

*   **Scenario 2: Gameplay scene rendered but frozen**
    *   **Given** PauseOverlayScene is active
    *   **When** the scene stack renders
    *   **Then** GameplayScene renders first (showing the frozen note field), then PauseOverlayScene renders the pause menu on top with a semi-transparent background

*   **Scenario 3: Resume returns to exact position**
    *   **Given** gameplay was paused at 10.5 seconds
    *   **When** the player selects "Resume"
    *   **Then** PauseOverlayScene is popped, GameplayScene's `on_resume()` is called, audio resumes at 10.5 seconds, and gameplay continues

*   **Scenario 4: Restart option restarts song**
    *   **Given** the pause menu is visible
    *   **When** the player selects "Restart"
    *   **Then** the scene stack pops PauseOverlayScene, replaces GameplayScene with a new GameplayScene for the same chart, and the song starts from the beginning

*   **Scenario 5: Quit option returns to song select**
    *   **Given** the pause menu is visible
    *   **When** the player selects "Quit to Select"
    *   **Then** the scene stack pops PauseOverlayScene, pops GameplayScene, and the player returns to SongSelectScene

*   **Scenario 6: Navigation between pause options**
    *   **Given** "Resume" is highlighted
    *   **When** the player presses down
    *   **Then** "Restart" becomes highlighted

*   **Scenario 7: No pause during intro or outro grace period**
    *   **Given** the song has not yet reached 2 seconds or has passed the final note by 3 seconds
    *   **When** the player presses start
    *   **Then** the pause request is ignored

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium — requires careful audio pause/resume coordination and scene state preservation
*   **Dependencies**: US-SCN-001 (scene stack with pause/resume hooks), US-AUD-001 (audio pause/resume), US-SCN-007 (GameplayScene)
*   **Implementation Notes**: Audio pause must preserve exact position. Use a semi-transparent overlay (alpha 0.7) over the frozen gameplay scene.

---

## Story ID: US-SCN-011 - Scene Transitions with BGA Animations

**Status**: PLANNED (Phase 5)
**Estimate**: 5 points

**Story Card:**
> **As a** Player
> **I want** smooth animated transitions between screens
> **So that** the game feels polished and visually cohesive as I navigate between menus and gameplay

### 📝 Description
Implement `TransitionScene` that wraps outgoing and incoming scenes during scene changes. Plays a BGA animation (fade, wipe, or other effect) over 0.5 to 2 seconds. Both scenes can render during the transition for crossfade effects. Transitions can be skipped if configured. This applies to scene replacements, not overlays.

### ✅ Acceptance Criteria

*   **Scenario 1: Transition BGA plays during scene switch**
    *   **Given** the player confirmed a mode selection
    *   **When** ModeSelectScene transitions to SongSelectScene
    *   **Then** a TransitionScene is pushed with a fade BGA, the fade plays for 1 second, then completes the scene replacement

*   **Scenario 2: Outgoing scene renders during fade-out**
    *   **Given** a transition is in progress with fade type
    *   **When** the transition is at 0.3 seconds (30% complete)
    *   **Then** the outgoing scene renders normally and a fade overlay at 30% opacity is composited on top

*   **Scenario 3: Incoming scene renders during fade-in**
    *   **Given** a transition is in progress at 0.7 seconds (70% complete)
    *   **When** the transition renders
    *   **Then** the incoming scene renders and a fade overlay at 30% opacity (100% - 70%) is composited on top

*   **Scenario 4: Transition duration configurable**
    *   **Given** a transition is configured for 0.5 seconds
    *   **When** the transition begins
    *   **Then** the scene switch completes after 0.5 seconds

*   **Scenario 5: Skip transition if enabled**
    *   **Given** transition skipping is enabled in settings
    *   **When** any scene transition begins
    *   **Then** the transition completes immediately without playing the animation

*   **Scenario 6: No visual discontinuity during wipe**
    *   **Given** a wipe transition is playing from left to right
    *   **When** the wipe is at 50%
    *   **Then** the left half shows the incoming scene and the right half shows the outgoing scene with no gap or overlap at the boundary

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium — requires compositing two scenes and managing transition state
*   **Dependencies**: US-SCN-001 (scene stack), US-REN-004 (BGA for transition effects)
*   **Implementation Notes**: TransitionScene should have a duration, a BGA reference, and pointers to outgoing/incoming scenes. Render order: outgoing → incoming (with alpha or clip rect) → BGA overlay.

---

## Story ID: US-SCN-012 - Settings Scene for Configuration

**Status**: PLANNED  
**Estimate**: 8 points

**Story Card:**
> **As a** Player
> **I want** a settings screen for video, audio, input, and gameplay options
> **So that** I can configure the engine to match my hardware and preferences

### 📝 Description
Implement `SettingsScene` with tabbed or categorized sections for Video (resolution, fullscreen, vsync), Audio (music volume, SFX volume, global offset), Input (key/pad configuration), and Gameplay (judge profile, speed mod defaults). Changes are saved immediately to `settings.json`. Accessible from the title screen or song select screen.

### ✅ Acceptance Criteria

*   **Scenario 1: Video settings displayed**
    *   **Given** SettingsScene is active on the Video tab
    *   **When** the scene renders
    *   **Then** options for resolution (e.g., 1920x1080, 1280x720), fullscreen toggle, and vsync toggle are visible

*   **Scenario 2: Resolution change applied immediately**
    *   **Given** the current resolution is 1920x1080 and the player selects 1280x720
    *   **When** the change is confirmed
    *   **Then** the window resizes to 1280x720, the logical 640×480 viewport scales correctly, and settings.json is updated

*   **Scenario 3: Audio volume sliders functional**
    *   **Given** the Audio tab is active
    *   **When** the player adjusts the music volume slider from 80% to 50%
    *   **Then** the music volume changes immediately (audible if music is playing), and the new value is saved

*   **Scenario 4: Global audio offset calibration**
    *   **Given** the Audio tab is active and offset is currently 0ms
    *   **When** the player adjusts the offset to +30ms
    *   **Then** the value updates, a preview tone plays with the new offset, and settings.json is updated

*   **Scenario 5: Input mapping screen accessible**
    *   **Given** the Input tab is active
    *   **When** the player selects "Configure Keyboard"
    *   **Then** an input mapping overlay appears prompting "Press key for Down-Left panel"

*   **Scenario 6: Gameplay judge profile selection**
    *   **Given** the Gameplay tab is active
    *   **When** the player navigates through judge profiles (Exceed, NX, Phoenix)
    *   **Then** each profile name and description is displayed, and selecting one updates the default judge profile

*   **Scenario 7: Changes saved immediately**
    *   **Given** the player changed 3 settings across different tabs
    *   **When** each change is made
    *   **Then** settings.json is written after each change, not on exit

*   **Scenario 8: Back input returns to previous scene**
    *   **Given** SettingsScene is active
    *   **When** the player presses back
    *   **Then** the scene stack pops SettingsScene and returns to the scene that pushed it

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Large — multi-section UI, live preview of changes, settings persistence
*   **Dependencies**: US-SCN-001 (scene stack), US-DAT-002 (settings.json), US-AUD-001 (volume and offset), US-INP-001 (input configuration)
*   **Implementation Notes**: Use a tab or menu bar for section navigation. Settings should apply immediately, not on save/apply button press.

---

## Story ID: US-SCN-013 - Profile Selection Scene

**Status**: PLANNED  
**Estimate**: 5 points

**Story Card:**
> **As a** Player
> **I want** a profile selection screen
> **So that** I can choose my player profile or create a new one, maintaining separate high scores and preferences

### 📝 Description
Implement `ProfileSelectionScene` that lists all available player profiles from the profiles directory. Players can select an existing profile, create a new one, or delete a profile (with confirmation). The selected profile persists for the session. Profile stats (total songs played, total score) are shown for each profile.

### ✅ Acceptance Criteria

*   **Scenario 1: All profiles listed on entry**
    *   **Given** three profiles exist: "ROAD24", "PLAYER2", "GUEST"
    *   **When** ProfileSelectionScene enters
    *   **Then** all three profile names are displayed in a list

*   **Scenario 2: Profile stats displayed**
    *   **Given** profile "ROAD24" has played 42 songs and total score 5,230,000
    *   **When** "ROAD24" is highlighted
    *   **Then** the stats "Songs: 42" and "Total Score: 5,230,000" are displayed

*   **Scenario 3: Create new profile option**
    *   **Given** the profile list is displayed
    *   **When** the player selects "Create New Profile"
    *   **Then** a name entry screen appears (reusing NameEntryScene or similar logic)

*   **Scenario 4: New profile added to list**
    *   **Given** the player entered the name "NEWPLAYER"
    *   **When** the name is confirmed
    *   **Then** a new profile file is created, and "NEWPLAYER" appears in the profile list

*   **Scenario 5: Profile selection persists for session**
    *   **Given** the player selected profile "ROAD24"
    *   **When** the scene transitions to TitleScene
    *   **Then** all high scores, settings, and stats for the session use profile "ROAD24"

*   **Scenario 6: Delete profile with confirmation**
    *   **Given** profile "GUEST" is highlighted
    *   **When** the player presses the delete action
    *   **Then** a confirmation dialog "Delete profile GUEST? This cannot be undone." appears

*   **Scenario 7: Confirmed delete removes profile**
    *   **Given** the delete confirmation dialog is visible for "GUEST"
    *   **When** the player confirms deletion
    *   **Then** the profile file is deleted, "GUEST" is removed from the list, and the cursor moves to the next available profile

*   **Scenario 8: Cancel delete returns to list**
    *   **Given** the delete confirmation dialog is visible
    *   **When** the player presses back or cancel
    *   **Then** the dialog closes and no deletion occurs

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Medium — file I/O, list UI, confirmation dialog
*   **Dependencies**: US-SCN-001 (scene stack), US-DAT-001 (profile files), US-SCN-009 (name entry logic, can be refactored into shared component)
*   **Implementation Notes**: Profile directory should be scanned on scene entry. Profile stats should be read from JSON without loading full high score history.

---

# Non-Functional Requirements

## NFR-SCN-001: Scene Transition Latency

**Story Card:**
> **As a** Player
> **I want** instant response to screen navigation inputs
> **So that** the game feels responsive and does not lag when I select options

### 📝 Description
Scene transitions (replace or push operations) must complete within 16 milliseconds (1 frame at 60 Hz) excluding any intentional transition animation. Scene `on_enter()` and `on_exit()` methods must be lightweight and defer heavy work (asset loading, decompression) to background threads or subsequent update frames.

### ✅ Acceptance Criteria

*   **Scenario 1: Scene push completes within 1 frame**
    *   **Given** a scene push operation is initiated
    *   **When** measured with high-precision timer
    *   **Then** the operation completes in under 16 milliseconds

*   **Scenario 2: Heavy loading uses background threads**
    *   **Given** GameplayScene is about to load a 5MB BGA file
    *   **When** `on_enter()` is called
    *   **Then** the BGA load is initiated on a worker thread and `on_enter()` returns immediately

*   **Scenario 3: Transition animations do not block logic**
    *   **Given** a 1-second fade transition is playing
    *   **When** the scene stack is processing updates
    *   **Then** the engine continues running at 60 Hz and remains responsive to input

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Performance constraint, not a discrete story
*   **Dependencies**: US-SCN-001 (scene stack implementation must respect this constraint)

---

## NFR-SCN-002: Scene Stack Memory Safety

**Story Card:**
> **As a** Developer
> **I want** scene stack operations to be memory-safe with no dangling pointers
> **So that** the engine does not crash during scene transitions or overlay operations

### 📝 Description
All scene pointers in the stack must use smart pointers (`std::unique_ptr`). Scenes removed from the stack (via pop or replace) are immediately destroyed. No scene may hold raw pointers to other scenes. The scene stack must be safely destructible at any point (e.g., if the engine shuts down mid-transition).

### ✅ Acceptance Criteria

*   **Scenario 1: Popped scene is destroyed**
    *   **Given** a scene is popped from the stack
    *   **When** the pop operation completes
    *   **Then** the scene's destructor is called before any further updates

*   **Scenario 2: No dangling pointers after replace**
    *   **Given** a scene is replaced
    *   **When** the old scene is destroyed
    *   **Then** no other system holds a raw pointer to the destroyed scene (verified via sanitizer or static analysis)

*   **Scenario 3: Stack destruction is safe**
    *   **Given** the engine is shutting down with 3 scenes in the stack
    *   **When** the scene stack is destroyed
    *   **Then** all scenes' `on_exit()` and destructors are called in reverse order with no crashes

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Architecture constraint, enforced by code review and static analysis
*   **Dependencies**: US-SCN-001 (scene stack implementation)

---

## NFR-SCN-003: Scene Render Budget

**Story Card:**
> **As a** Developer
> **I want** each scene's render method to complete in under 8 milliseconds
> **So that** the engine maintains 60 FPS even when multiple scenes are in the stack

### 📝 Description
Each scene's `render()` method must complete in under 8 milliseconds on typical hardware (quad-core CPU, mid-range GPU). This allows for 2 scenes (base + overlay) to render comfortably within a 16ms frame budget (60 FPS). Rendering must use batched draw calls and minimize state changes.

### ✅ Acceptance Criteria

*   **Scenario 1: Single scene renders in under 8ms**
    *   **Given** GameplayScene is rendering a full note field, BGA, and UI
    *   **When** measured on typical hardware
    *   **Then** the `render()` call completes in under 8 milliseconds

*   **Scenario 2: Two scenes render in under 16ms**
    *   **Given** GameplayScene is paused and PauseOverlayScene is on top
    *   **When** both scenes render
    *   **Then** the total render time is under 16 milliseconds

*   **Scenario 3: Draw call batching reduces overhead**
    *   **Given** a scene renders 200 UI sprites
    *   **When** rendering is profiled
    *   **Then** all sprites are submitted in fewer than 10 draw calls (batched by texture)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: Performance target, verified via profiling in Phase 2+
*   **Dependencies**: US-REN-001 (renderer batching), all scene implementations

---

## Cross-File Dependencies Summary

The Screen Flow stories depend on components from other subsystems:

- **US-ENG-001**: Engine loop (must exist to own scene stack)
- **US-INP-001, US-INP-002**: Input system and InputSnapshot
- **US-REN-001, US-REN-004, US-REN-007**: Renderer, BGA rendering, note renderer
- **US-AUD-001, US-AUD-002, US-AUD-003**: Audio playback, position query, SFX
- **US-CHT-001, US-CHT-014**: Chart loading and difficulty metadata
- **US-JDG-001, US-JDG-014**: Judge and final GameplayState
- **US-AST-001, US-AST-003**: Asset manager and song database
- **US-DAT-001, US-DAT-002, US-DAT-005**: Profile files, settings files, high score storage

---

**Document Version**: 1.0  
**Last Updated**: 2026-04-26  
**Status**: All stories PLANNED (Phase 1-5 implementation pending)
