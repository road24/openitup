# Screen Flow Requirements

## REQ-SCN-001: Scene Stack Architecture
**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

The engine must implement a scene stack with push, pop, and replace operations supporting overlays.

**Acceptance Criteria**:
- Scenes can be pushed onto stack (overlay)
- Scenes can be popped from stack (return to previous)
- Scenes can replace top scene (transition)
- Stack renders bottom-to-top (overlays visible on top)
- Only topmost scene receives input and updates

**Dependencies**: REQ-ENG-002  
**Source**: Roadmap subsystem 7, Phase 2

---

## REQ-SCN-002: Scene Lifecycle Interface
**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

Each scene must implement lifecycle methods: `on_enter()`, `on_exit()`, `on_pause()`, `on_resume()`, `update(dt)`, `render()`, `handle_input(InputSnapshot)`.

**Acceptance Criteria**:
- on_enter() called when scene becomes active
- on_exit() called when scene removed from stack
- on_pause() called when another scene pushed on top
- on_resume() called when overlaying scene popped
- update() and render() called each frame for active scene
- handle_input() receives input snapshot

**Dependencies**: REQ-SCN-001  
**Source**: Roadmap subsystem 7

---

## REQ-SCN-003: Boot Scene
**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

The engine must display a boot/splash screen showing logo and performing initialization.

**Acceptance Criteria**:
- Displays openitup logo or PIU version logo
- Performs asset scanning during boot
- Transitions to title screen when complete
- Boot duration < 5 seconds on typical hardware

**Dependencies**: REQ-SCN-001, REQ-REN-004  
**Source**: Roadmap subsystem 7, Phase 2

---

## REQ-SCN-004: Title Scene
**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

The engine must provide a title screen with attract mode loop using BGA animation.

**Acceptance Criteria**:
- Loops BGA animation continuously
- Accepts coin/start input to proceed
- Timeout returns to boot/attract after inactivity
- Audio plays during attract (optional)

**Dependencies**: REQ-SCN-001, REQ-REN-004, REQ-INP-002  
**Source**: Roadmap subsystem 7, Phase 2

---

## REQ-SCN-005: Mode Select Scene
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

The engine must provide mode selection screen for Single, Double, Co-op, Battle modes.

**Acceptance Criteria**:
- Visual layout shows all mode options
- Input navigates between modes
- Selection transitions to song select with chosen mode
- Battle mode disabled if not yet implemented

**Dependencies**: REQ-SCN-001, REQ-INP-002  
**Source**: Roadmap subsystem 7, Phase 3

---

## REQ-SCN-006: Song Select Scene
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

The engine must provide song selection screen with music wheel, banner display, and difficulty selector.

**Acceptance Criteria**:
- Music wheel lists all available songs
- Banner/jacket image displays for selected song
- Difficulty tabs show available difficulties and ratings
- Preview audio plays for selected song
- Selection transitions to gameplay scene

**Dependencies**: REQ-SCN-001, REQ-AST-003, REQ-CHT-014  
**Source**: Roadmap subsystem 7, Phase 3

---

## REQ-SCN-007a: Minimal Gameplay Scene
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The engine must provide a minimal gameplay scene that loads chart from KSF parser, plays audio, renders placeholder notes as rectangles, accepts keyboard input, runs judge, and shows text-based timing feedback.

**Acceptance Criteria**:
- Loads chart from KSF parser
- Plays audio and queries position each frame
- Renders notes as colored rectangles scrolling in beat-space
- Accepts keyboard input via InputSnapshot
- Calls judge.update() with input snapshot
- Shows judgment word rendered as SDL text or colored rectangle flash
- No dependency on scene stack (Phase 1 runs GameplayScene directly)

**Dependencies**: REQ-ENG-001, REQ-INP-001, REQ-AUD-001, REQ-CHT-001, REQ-JDG-001, REQ-REN-007, REQ-REN-019  
**Source**: Roadmap subsystem 7, Phase 1

---

## REQ-SCN-007b: Full Gameplay Scene Orchestration
**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

The engine must provide full gameplay orchestration including BGA playback, sprite-based note rendering, combo/judgment display, life gauge, and result scene transition.

**Acceptance Criteria**:
- Advances BGA tick counter synchronized to audio
- Renders sprite-based notes using note skin
- Displays sprite-based judgments and combo
- Displays life gauge with HP drain
- Transitions to result scene on song completion
- Integrates with scene stack for pause/overlay support

**Dependencies**: REQ-SCN-001, REQ-SCN-007a, REQ-REN-008, REQ-JDG-011  
**Source**: Roadmap subsystem 7, Phase 2

---

## REQ-SCN-008: Result Scene
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

The engine must display result screen showing grade, score, max combo, and timing breakdown after song completion.

**Acceptance Criteria**:
- Displays final grade (SSS-F)
- Displays final score and max combo
- Shows judgment counts (Perfect/Great/Good/Bad/Miss)
- Shows timing graph or breakdown
- Saves high score to profile if new record
- Returns to song select or mode select on input

**Dependencies**: REQ-SCN-001, REQ-JDG-014, REQ-DAT-005  
**Source**: Roadmap subsystem 7, Phase 3

---

## REQ-SCN-009: Name Entry Scene
**Status**: [PLANNED Phase 3]  
**Priority**: Should Have

The engine must provide name entry screen for high score records.

**Acceptance Criteria**:
- Displays current score and rank
- Allows 3-10 character name entry
- Character selection via input navigation
- Name saved with high score record
- Skipped if not a new high score

**Dependencies**: REQ-SCN-001, REQ-DAT-005  
**Source**: Roadmap subsystem 7

---

## REQ-SCN-010: Pause Overlay Scene
**Status**: [PLANNED Phase 5]  
**Priority**: Should Have

The engine must support pausing gameplay by pushing a pause overlay without destroying gameplay state.

**Acceptance Criteria**:
- Start button pauses gameplay during song
- Pause overlay pushed on top of gameplay scene
- Gameplay scene rendered but frozen in background
- Audio paused
- Resume returns to exact position
- Options: resume, restart, quit to select

**Dependencies**: REQ-SCN-001, REQ-AUD-001  
**Source**: Roadmap subsystem 7, Phase 5

---

## REQ-SCN-011: Scene Transitions with BGA Animations
**Status**: [PLANNED Phase 5]  
**Priority**: Should Have

The engine must support smooth transitions between scenes using BGA animations for fades and wipes.

**Acceptance Criteria**:
- TransitionScene wraps outgoing and incoming scenes
- Transition BGA plays during scene switch
- Transition duration 0.5-2 seconds typical
- No visual pop or discontinuity
- Transitions skip-able if enabled

**Dependencies**: REQ-SCN-001, REQ-REN-004  
**Source**: Roadmap subsystem 7, Phase 5

---

## REQ-SCN-012: Settings/Options Scene
**Status**: [PLANNED Phase 3]  
**Priority**: Should Have

The engine must provide a settings screen for configuring video, audio, input, and gameplay options.

**Acceptance Criteria**:
- Video: resolution, fullscreen, vsync
- Audio: music volume, SFX volume, offset
- Input: key/pad configuration
- Gameplay: judge profile, speed mod defaults
- Changes saved immediately to settings file

**Dependencies**: REQ-SCN-001, REQ-DAT-002  
**Source**: Roadmap Phase 3, subsystem 9

---

## REQ-SCN-013: Profile Selection Scene
**Status**: [PLANNED Phase 5]  
**Priority**: Should Have

The engine must provide profile selection screen for choosing or creating player profiles.

**Acceptance Criteria**:
- Lists all available profiles
- Create new profile option
- Delete profile option (with confirmation)
- Profile selection persists for session
- Shows profile stats (songs played, total score)

**Dependencies**: REQ-SCN-001, REQ-DAT-001  
**Source**: Roadmap subsystem 9, Phase 5
