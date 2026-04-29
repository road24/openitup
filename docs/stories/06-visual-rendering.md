# Visual Rendering User Stories

This document decomposes the visual rendering requirements from REQ-REN-001 through REQ-REN-018 into developer-ready user stories following the INVEST principles.

---

## Epic: BGA Animation Stack (DONE)

The foundational 2D compositing animation stack providing sprite and BGA rendering capabilities.

---

### Story ID: US-REN-001 - SDL3 Renderer with Logical Resolution

**Story Card:**
> **As a** Player
> **I want** the game window to display at my native monitor resolution
> **So that** the game fills my screen without distortion or stretching

**Status**: DONE

### 📝 Description
The Renderer class wraps SDL_Window and SDL_Renderer, configuring a 640×480 logical coordinate space that scales to any physical display resolution while preserving aspect ratio through letterboxing.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: 640×480 virtual space maintained**
    *   **Given** a display running at 1920×1080 resolution
    *   **When** the renderer initializes
    *   **Then** all draw coordinates use 640×480 space and scale correctly to the physical display

*   **Scenario 2: Aspect ratio preserved with letterboxing**
    *   **Given** a 16:9 widescreen display
    *   **When** rendering a 4:3 game frame
    *   **Then** black bars appear on the left and right sides to maintain 4:3 aspect ratio

*   **Scenario 3: Smooth scaling applied**
    *   **Given** logical resolution of 640×480 scaled to 2560×1920
    *   **When** textures are rendered
    *   **Then** linear filtering is applied (no pixelation or aliasing)

*   **Scenario 4: Multi-resolution support**
    *   **Given** displays ranging from 1280×720 to 3840×2160
    *   **When** the renderer initializes on each
    *   **Then** the 640×480 logical space renders correctly on all resolutions

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1 (already implemented)
*   **Dependencies**: None
*   **Implementation**: `src/renderer.cpp`, SDL_RenderSetLogicalPresentation
*   **Tests**: Integration tests in `test_integration.cpp` verify coordinate mapping

---

### Story ID: US-REN-002 - Frame Render Loop

**Story Card:**
> **As a** Developer
> **I want** the renderer to provide begin_frame and end_frame methods
> **So that** I can structure the render loop with clear initialization and presentation boundaries

**Status**: DONE

### 📝 Description
The renderer exposes begin_frame (clears the backbuffer) and end_frame (presents to display) methods to wrap each frame's draw operations, managing vsync and frame timing.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Clear to black at frame start**
    *   **Given** a frame is about to render
    *   **When** begin_frame is called
    *   **Then** the backbuffer is cleared to RGB(0, 0, 0)

*   **Scenario 2: Present with vsync**
    *   **Given** a frame has been drawn
    *   **When** end_frame is called
    *   **Then** the backbuffer is presented to the display with vsync enabled

*   **Scenario 3: Frame timing independence**
    *   **Given** a 60Hz logic update rate and 144Hz display
    *   **When** frames are rendered
    *   **Then** rendering occurs at 144Hz without affecting logic timing

*   **Scenario 4: No tearing artifacts**
    *   **Given** vsync is enabled
    *   **When** fast horizontal motion is rendered
    *   **Then** no screen tearing is visible

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 1 (already implemented)
*   **Dependencies**: US-REN-001
*   **Implementation**: `src/renderer.cpp`, begin_frame/end_frame methods
*   **Tests**: `test_integration.cpp` validates clear and present calls

---

### Story ID: US-REN-003 - Sprite Loading from JSON Format

**Story Card:**
> **As a** Content Creator
> **I want** to load sprites from SPRJ JSON files
> **So that** I can use human-editable sprite definitions with normalized UV coordinates

**Status**: DONE

### 📝 Description
The sprite loader parses SPRJ JSON files containing texture references, picture definitions, and rendering mode configuration. UV coordinates are normalized to 0.0-1.0 range.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Valid SPRJ loads successfully**
    *   **Given** a SPRJ file with 5 pictures referencing "tile.png"
    *   **When** the sprite is loaded
    *   **Then** all 5 picture definitions are parsed and the texture is cached

*   **Scenario 2: Normalized UV coordinates**
    *   **Given** a picture definition with src_uv [0.0, 0.0, 0.5, 0.5]
    *   **When** the picture is rendered
    *   **Then** the top-left quadrant of the texture is drawn

*   **Scenario 3: Missing texture file**
    *   **Given** a SPRJ file referencing "missing.tga"
    *   **When** the sprite is loaded
    *   **Then** an error is logged and the load fails with a clear message

*   **Scenario 4: Three rendering modes configured**
    *   **Given** SPRJ files with mode "TILE", "ANI", or "PATTERN"
    *   **When** each sprite is loaded
    *   **Then** the correct rendering mode is stored

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 (already implemented)
*   **Dependencies**: US-REN-001, US-REN-005
*   **Implementation**: `src/sprite.cpp`, nlohmann/json parser
*   **Tests**: 8 unit tests in `test_sprite.cpp`, integration tests with fixtures

---

### Story ID: US-REN-004 - Legacy SPR Format Loading

**Story Card:**
> **As a** Content Creator
> **I want** to load sprites from legacy SPR binary files
> **So that** I can use original Pump It Up game assets without manual conversion

**Status**: DONE

### 📝 Description
The sprite loader supports binary SPR format using absolute pixel coordinates. UV normalization requires actual texture dimensions, so the converter fails if referenced textures are not found.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: SPR converts to SPRJ**
    *   **Given** an SPR file with 3 pictures and "tile.tga" at 256×256
    *   **When** spr2sprj converter runs
    *   **Then** a valid SPRJ file is produced with normalized UVs

*   **Scenario 2: UV normalization with actual dimensions**
    *   **Given** an SPR picture with pixel coords [0, 0, 128, 128] and texture 256×256
    *   **When** conversion occurs
    *   **Then** the SPRJ src_uv is [0.0, 0.0, 0.5, 0.5]

*   **Scenario 3: Missing texture fails conversion**
    *   **Given** an SPR file referencing "notfound.tga"
    *   **When** spr2sprj runs without that texture present
    *   **Then** conversion fails with error message indicating missing file

*   **Scenario 4: Custom asset directory**
    *   **Given** textures in /custom/path/
    *   **When** spr2sprj runs with --asset-dir /custom/path/
    *   **Then** textures are found and conversion succeeds

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 (already implemented)
*   **Dependencies**: US-REN-003
*   **Implementation**: `tools/spr2sprj.cpp`, binary parsing
*   **Tests**: Integration test with SPR fixture conversion

---

### Story ID: US-REN-005 - Texture Cache with Case-Insensitive Probing

**Story Card:**
> **As a** Player on Linux
> **I want** the engine to load textures regardless of file extension casing
> **So that** I can use game assets from Windows without renaming files

**Status**: DONE

### 📝 Description
The texture cache implements case-insensitive file lookup and probes for .tga, .png, and .dds formats in order. Textures are loaded once and reused across all sprites.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Case-insensitive lookup**
    *   **Given** a sprite references "Tile.TGA" but the file is "tile.tga"
    *   **When** the texture is loaded
    *   **Then** the file is found and loaded successfully

*   **Scenario 2: Format probing order**
    *   **Given** a sprite references "bg" and files "bg.png" and "bg.tga" exist
    *   **When** the texture is loaded
    *   **Then** "bg.tga" is tried first, then "bg.png" if TGA is missing

*   **Scenario 3: Texture reuse across sprites**
    *   **Given** 10 sprites all reference "common.png"
    *   **When** all sprites are loaded
    *   **Then** the texture is loaded exactly once and shared

*   **Scenario 4: All formats fail**
    *   **Given** a sprite references "missing" with no .tga, .png, or .dds present
    *   **When** the texture is loaded
    *   **Then** an error is logged listing all probed paths

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 (already implemented)
*   **Dependencies**: US-REN-001
*   **Implementation**: `src/texture_cache.cpp`, case-insensitive FS wrapper
*   **Tests**: 6 unit tests in `test_texture_cache.cpp`

---

### Story ID: US-REN-006 - Legacy SP2 Format Loading

**Story Card:**
> **As a** Content Creator
> **I want** to load sprites from SP2 format files
> **So that** I can use later-era Pump It Up assets with named pictures

**Status**: DONE

### 📝 Description
SP2 format uses position+size offsets divided by fixed 256 for UV coordinates. Conversion to SPRJ does not require actual texture files since the divisor is constant.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: SP2 converts to SPRJ without textures**
    *   **Given** an SP2 file with pictures using 256-grid coordinates
    *   **When** spr2sprj converter runs
    *   **Then** a valid SPRJ is produced without requiring texture files

*   **Scenario 2: Fixed 256 divisor normalization**
    *   **Given** an SP2 picture with coords [128, 0, 128, 128]
    *   **When** conversion occurs
    *   **Then** SPRJ src_uv is [0.5, 0.0, 1.0, 0.5]

*   **Scenario 3: Named pictures preserved**
    *   **Given** an SP2 with picture names "frame_01", "frame_02"
    *   **When** converted to SPRJ
    *   **Then** picture names are preserved in JSON

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 (already implemented)
*   **Dependencies**: US-REN-003, US-REN-004
*   **Implementation**: `tools/spr2sprj.cpp`, SP2 parser branch
*   **Tests**: Integration test with SP2 fixture

---

### Story ID: US-REN-007 - Sprite TILE Rendering Mode

**Story Card:**
> **As a** Content Creator
> **I want** sprites in TILE mode to draw all pictures back-to-front
> **So that** I can create layered static compositions

**Status**: DONE

### 📝 Description
TILE mode renders all pictures in reverse order (last picture drawn first, first appears on top). The time parameter `t` is ignored.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: All pictures drawn**
    *   **Given** a TILE sprite with 5 pictures
    *   **When** the sprite is rendered at t=0.0
    *   **Then** all 5 pictures appear on screen

*   **Scenario 2: Painter's algorithm order**
    *   **Given** pictures [A, B, C] in a TILE sprite
    *   **When** rendered
    *   **Then** C is drawn first, A appears on top (reverse iteration)

*   **Scenario 3: Time parameter ignored**
    *   **Given** a TILE sprite rendered at t=0.0 and t=1.0
    *   **When** both renders occur
    *   **Then** the output is identical

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 (already implemented)
*   **Dependencies**: US-REN-003
*   **Implementation**: `src/sprite.cpp`, TILE branch in render method
*   **Tests**: 3 unit tests in `test_sprite.cpp`

---

### Story ID: US-REN-008 - Sprite ANI Rendering Mode

**Story Card:**
> **As a** Content Creator
> **I want** sprites in ANI mode to select a single frame based on time
> **So that** I can create frame-by-frame animations

**Status**: DONE

### 📝 Description
ANI mode selects one picture using `frame = floor(num_pictures * t)` where t is normalized [0.0, 1.0). Only the selected frame is drawn.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Frame selection at t=0.0**
    *   **Given** an ANI sprite with 4 pictures
    *   **When** rendered at t=0.0
    *   **Then** picture 0 is drawn (floor(4 * 0.0) = 0)

*   **Scenario 2: Frame selection at t=0.5**
    *   **Given** an ANI sprite with 4 pictures
    *   **When** rendered at t=0.5
    *   **Then** picture 2 is drawn (floor(4 * 0.5) = 2)

*   **Scenario 3: Frame clamping at t=1.0**
    *   **Given** an ANI sprite with 4 pictures
    *   **When** rendered at t=1.0
    *   **Then** picture 3 is drawn (clamped to last frame)

*   **Scenario 4: Single picture drawn**
    *   **Given** an ANI sprite with 10 pictures rendered at t=0.3
    *   **When** the frame is rendered
    *   **Then** only picture 3 appears (others are not drawn)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 (already implemented)
*   **Dependencies**: US-REN-003
*   **Implementation**: `src/sprite.cpp`, ANI branch with frame calculation
*   **Tests**: 5 unit tests in `test_sprite.cpp` covering edge cases

---

### Story ID: US-REN-009 - Sprite PATTERN Rendering Mode

**Story Card:**
> **As a** Content Creator
> **I want** sprites in PATTERN mode to tile pictures into a grid
> **So that** I can create scrolling backgrounds and tiled patterns

**Status**: DONE

### 📝 Description
PATTERN mode tiles pictures into a grid_y × grid_x grid. The time parameter shifts which picture fills each cell according to a direction (horizontal or vertical).

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Grid tiling with 3×3 layout**
    *   **Given** a PATTERN sprite with 9 pictures and grid 3×3
    *   **When** rendered at t=0.0
    *   **Then** 9 pictures fill the screen in a 3×3 grid

*   **Scenario 2: Horizontal direction shift**
    *   **Given** a PATTERN sprite with direction HORIZONTAL
    *   **When** rendered at increasing t values
    *   **Then** pictures shift from right to left

*   **Scenario 3: Vertical direction shift**
    *   **Given** a PATTERN sprite with direction VERTICAL
    *   **When** rendered at increasing t values
    *   **Then** pictures shift from bottom to top

*   **Scenario 4: Wrapping behavior**
    *   **Given** a PATTERN with 4 pictures rendered at t causing picture 5
    *   **When** the pattern is rendered
    *   **Then** picture index wraps modulo 4

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 (already implemented)
*   **Dependencies**: US-REN-003
*   **Implementation**: `src/sprite.cpp`, PATTERN branch with grid math
*   **Tests**: 4 unit tests in `test_sprite.cpp`

---

### Story ID: US-REN-010 - BGA JSON Format Loading

**Story Card:**
> **As a** Content Creator
> **I want** to load BGA animations from BGAJ JSON files
> **So that** I can author and edit animations in a human-readable format

**Status**: DONE

### 📝 Description
The BGA loader parses BGAJ JSON files defining up to 50 layers, each containing a sprite reference and keyframe event list. Each keyframe defines tick, position, scale, rotation, pivot, color, alpha, effect, and display.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Valid BGAJ loads successfully**
    *   **Given** a BGAJ file with 3 layers and 10 keyframes per layer
    *   **When** the BGA is loaded
    *   **Then** all layers and keyframes are parsed correctly

*   **Scenario 2: Keyframe property parsing**
    *   **Given** a keyframe with tick=120, translate=[100.0, 200.0], scale=[2.0, 2.0], rotate=45.0
    *   **When** loaded
    *   **Then** all numeric values match exactly

*   **Scenario 3: Blend mode enumeration**
    *   **Given** keyframes with effect "screen", "multiply", "dodge", "difference"
    *   **When** loaded
    *   **Then** each maps to the correct internal enum value

*   **Scenario 4: Invalid JSON structure**
    *   **Given** a BGAJ with missing required "layers" field
    *   **When** loading is attempted
    *   **Then** an error is logged with the specific missing field

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 (already implemented)
*   **Dependencies**: US-REN-003 (sprites must load first)
*   **Implementation**: `src/bga.cpp`, JSON schema validation
*   **Tests**: 12 unit tests in `test_bga.cpp`

---

### Story ID: US-REN-011 - BGA Binary Format Loading

**Story Card:**
> **As a** Content Creator
> **I want** to load BGA animations from binary .bga files
> **So that** I can use original Pump It Up game assets

**Status**: DONE

### 📝 Description
Binary BGA format has 16-byte header with "BGA2" magic, exactly 50 layers (inactive layers have empty/space-prefixed sprite names), little-endian, 64-byte keyframe records with 14 bytes reserved padding.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Binary BGA converts to BGAJ**
    *   **Given** a binary .bga file with 10 active layers
    *   **When** bga2bgaj converter runs
    *   **Then** a valid BGAJ JSON file is produced

*   **Scenario 2: Magic number validation**
    *   **Given** a file with incorrect magic number (not "BGA2")
    *   **When** loading is attempted
    *   **Then** the load fails with error "Invalid BGA header magic"

*   **Scenario 3: Inactive layer filtering**
    *   **Given** a BGA with 50 layers where 40 have empty sprite names
    *   **When** converted to BGAJ
    *   **Then** only the 10 active layers appear in the JSON

*   **Scenario 4: Little-endian integer parsing**
    *   **Given** a keyframe tick stored as bytes [0x78, 0x00, 0x00, 0x00]
    *   **When** parsed
    *   **Then** the tick value is 120

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 (already implemented)
*   **Dependencies**: US-REN-010
*   **Implementation**: `tools/bga2bgaj.cpp`, binary parsing
*   **Tests**: Integration test with binary BGA fixture

---

### Story ID: US-REN-012 - Keyframe Interpolated Properties

**Story Card:**
> **As a** Player
> **I want** BGA animations to smoothly transition between keyframes
> **So that** movements and effects appear fluid at 60 ticks per second

**Status**: DONE

### 📝 Description
The BGA renderer interpolates translate, scale, rotate, color RGB, and alpha linearly between consecutive keyframes based on the current tick position. Sub-tick precision supports high refresh rate displays.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Position interpolation at mid-keyframe**
    *   **Given** keyframes at tick 0 (x=0) and tick 60 (x=600)
    *   **When** rendering at tick 30
    *   **Then** the layer is drawn at x=300

*   **Scenario 2: Rotation interpolation**
    *   **Given** keyframes at tick 0 (rotate=0°) and tick 120 (rotate=360°)
    *   **When** rendering at tick 60
    *   **Then** rotation is 180°

*   **Scenario 3: Color interpolation**
    *   **Given** keyframes at tick 0 (color=[255,0,0]) and tick 60 (color=[0,255,0])
    *   **When** rendering at tick 30
    *   **Then** color is approximately [127, 127, 0] (±2 per channel)

*   **Scenario 4: Alpha transparency interpolation**
    *   **Given** keyframes at tick 0 (alpha=255) and tick 60 (alpha=0)
    *   **When** rendering at tick 45
    *   **Then** alpha is 63 (75% fade)

*   **Scenario 5: Sub-tick precision**
    *   **Given** tick 30.5 on a 144Hz display
    *   **When** interpolation occurs
    *   **Then** the position is calculated with fractional tick precision

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 (already implemented)
*   **Dependencies**: US-REN-010
*   **Implementation**: `src/keyframe.cpp`, lerp utilities
*   **Tests**: 15 regression tests verify smooth transitions

---

### Story ID: US-REN-013 - Keyframe Non-Interpolated Properties

**Story Card:**
> **As a** Content Creator
> **I want** pivot, blend mode, and visibility to change instantly at keyframes
> **So that** I can create precise effect timing and sudden transitions

**Status**: DONE

### 📝 Description
The properties pivot, effect (blend mode), and display (visibility) are taken from the start keyframe of each interval and snap instantly when a new keyframe is reached. They do not interpolate.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Pivot snaps at keyframe boundary**
    *   **Given** keyframes at tick 60 (pivot=[0,0]) and tick 61 (pivot=[320,240])
    *   **When** rendering at tick 60.9
    *   **Then** pivot is [0, 0] (start keyframe value)

*   **Scenario 2: Blend mode change**
    *   **Given** keyframes at tick 0 (effect="normal") and tick 60 (effect="screen")
    *   **When** rendering at tick 30
    *   **Then** effect is "normal" until tick 60 is reached

*   **Scenario 3: Visibility toggle**
    *   **Given** keyframes at tick 0 (display=false) and tick 60 (display=true)
    *   **When** rendering at tick 59
    *   **Then** the layer is invisible

*   **Scenario 4: Instant transition at exact tick**
    *   **Given** keyframes at tick 60 (display=false) and tick 60 (display=true)
    *   **When** rendering at tick 60.0
    *   **Then** the layer becomes visible immediately

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 (already implemented)
*   **Dependencies**: US-REN-012
*   **Implementation**: `src/keyframe.cpp`, property branching logic
*   **Tests**: 5 regression tests cover snap boundaries

---

### Story ID: US-REN-014 - Layer Visibility Window

**Story Card:**
> **As a** Content Creator
> **I want** layers to be invisible before their first keyframe and at/past their last keyframe
> **So that** I can precisely control when elements appear and disappear

**Status**: DONE

### 📝 Description
A layer is invisible before its first keyframe tick AND at/past its last keyframe tick. To keep a layer visible through tick N, the last keyframe must be at tick N+1 or later.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Invisible before first keyframe**
    *   **Given** a layer with first keyframe at tick 60
    *   **When** rendering at tick 59
    *   **Then** the layer does not appear

*   **Scenario 2: Visible at first keyframe**
    *   **Given** a layer with first keyframe at tick 60
    *   **When** rendering at tick 60
    *   **Then** the layer is drawn

*   **Scenario 3: Invisible at last keyframe**
    *   **Given** a layer with last keyframe at tick 120
    *   **When** rendering at tick 120
    *   **Then** the layer does not appear

*   **Scenario 4: Hold visible through tick N**
    *   **Given** a layer that should be visible through tick 100
    *   **When** keyframes are authored
    *   **Then** the last keyframe must be at tick 101 or later

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 (already implemented)
*   **Dependencies**: US-REN-010
*   **Implementation**: `src/bga.cpp`, visibility range check
*   **Tests**: 4 regression tests verify boundary conditions

---

### Story ID: US-REN-015 - BGA Layer Compositing

**Story Card:**
> **As a** Content Creator
> **I want** up to 50 layers composited in index order
> **So that** I can build complex visual scenes with layered elements

**Status**: DONE

### 📝 Description
BGA animations support up to 50 layers, rendered in index order using the painter's algorithm (layer 0 drawn first, layer 49 appears on top).

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Layer ordering preserved**
    *   **Given** layers [background, midground, foreground] at indices [0, 1, 2]
    *   **When** the BGA is rendered
    *   **Then** foreground appears on top of midground appears on top of background

*   **Scenario 2: Maximum 50 layers supported**
    *   **Given** a BGA with 50 active layers
    *   **When** rendered
    *   **Then** all 50 layers are drawn correctly

*   **Scenario 3: Inactive layer skipped**
    *   **Given** layer 5 has no sprite reference
    *   **When** the BGA is rendered
    *   **Then** layer 5 is skipped without error

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2 (already implemented)
*   **Dependencies**: US-REN-010, US-REN-007
*   **Implementation**: `src/bga.cpp`, layer iteration
*   **Tests**: Regression tests with multi-layer compositions

---

### Story ID: US-REN-016 - BGA Blend Modes

**Story Card:**
> **As a** Content Creator
> **I want** five blend modes for layer effects
> **So that** I can create lighting, shadows, and color effects

**Status**: DONE

### 📝 Description
The BGA renderer supports five blend modes: normal (SDL_BLENDMODE_BLEND), screen, multiply, dodge, and difference. The last two are approximations due to SDL3 limitations.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Normal blend (alpha compositing)**
    *   **Given** a layer with effect="normal" and alpha=128
    *   **When** rendered over a background
    *   **Then** 50% transparency is applied

*   **Scenario 2: Screen blend (additive lighting)**
    *   **Given** a white layer with effect="screen" over gray background
    *   **When** rendered
    *   **Then** the result is brighter (lightening effect)

*   **Scenario 3: Multiply blend (shadows)**
    *   **Given** a gray layer with effect="multiply" over white background
    *   **When** rendered
    *   **Then** the result is darker

*   **Scenario 4: Dodge and difference approximations**
    *   **Given** layers with effect="dodge" or "difference"
    *   **When** rendered
    *   **Then** SDL3 approximations are applied (documented limitation)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 (already implemented)
*   **Dependencies**: US-REN-015
*   **Implementation**: `src/bga.cpp`, SDL_SetTextureBlendMode mapping
*   **Tests**: Regression tests compare against reference images

---

### Story ID: US-REN-017 - BGA Player Tool

**Story Card:**
> **As a** Developer
> **I want** a BGA player tool for interactive playback and snapshots
> **So that** I can preview animations and generate test reference images

**Status**: DONE

### 📝 Description
The bga_player tool supports interactive playback with keyboard controls and automated snapshot export to PNG at specified ticks.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Interactive playback**
    *   **Given** the command `./bga_player title_screen.bgaj`
    *   **When** the tool runs
    *   **Then** the BGA plays in a window with 60 ticks per second

*   **Scenario 2: Snapshot export**
    *   **Given** the command `./bga_player title.bgaj --snapshot 60 output.png`
    *   **When** the tool runs
    *   **Then** frame at tick 60 is saved to output.png

*   **Scenario 3: Playback controls**
    *   **Given** the player is running
    *   **When** the spacebar is pressed
    *   **Then** playback pauses/resumes

*   **Scenario 4: Automated test usage**
    *   **Given** 30 reference PNGs needed for regression tests
    *   **When** snapshot commands are scripted
    *   **Then** all references are generated non-interactively

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3 (already implemented)
*   **Dependencies**: US-REN-010, US-REN-015
*   **Implementation**: `tools/bga_player.cpp`, CLI11 arg parsing
*   **Tests**: Used to generate fixtures for `test_regression.cpp`

---

### Story ID: US-REN-018 - Visual Regression Test Suite

**Story Card:**
> **As a** Developer
> **I want** regression tests comparing rendered frames to reference images
> **So that** I can detect unintended visual changes in the rendering pipeline

**Status**: DONE

### 📝 Description
The regression suite loads BGAJ/SPRJ fixtures, renders frames at specific ticks, and compares pixel-by-pixel against 30 committed reference PNGs with tolerance ≤2 per RGB channel.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: All interpolated properties covered**
    *   **Given** regression tests for translate, scale, rotate, color, alpha
    *   **When** tests run
    *   **Then** smooth transitions are verified across multiple frames

*   **Scenario 2: Non-interpolated properties covered**
    *   **Given** regression tests for pivot, effect, display snapping
    *   **When** tests run
    *   **Then** instant changes at keyframe boundaries are verified

*   **Scenario 3: Pixel tolerance**
    *   **Given** a rendered frame differing by 1 in red channel per pixel
    *   **When** compared to reference
    *   **Then** the test passes (within tolerance)

*   **Scenario 4: Pixel tolerance exceeded**
    *   **Given** a rendered frame differing by 5 in any channel
    *   **When** compared to reference
    *   **Then** the test fails with a clear diff report

*   **Scenario 5: Coverage of 30 reference images**
    *   **Given** 17 test cases
    *   **When** all tests run
    *   **Then** 30 distinct reference images are validated

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5 (already implemented)
*   **Dependencies**: US-REN-012, US-REN-013, US-REN-014, US-REN-017
*   **Implementation**: `test/test_regression.cpp`, 17 test cases
*   **Tests**: Self-testing (these ARE the tests)

---

## Epic: Note Renderer

The note scrolling and rendering system for gameplay, converting beat-space to screen-space.

---

### Story ID: US-REN-019 - Beat-Space to Screen-Space Conversion

**Story Card:**
> **As a** Developer
> **I want** notes to scroll in beat-space rather than time-space
> **So that** BPM changes and stops do not cause visual discontinuities

**Status**: PLANNED (Phase 1)

### 📝 Description
The note renderer converts each note's beat position to vertical screen position using the current scroll speed, BPM from timing data, and configurable receptor line position.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Basic scroll calculation**
    *   **Given** a note at beat 4.0, current beat 0.0, scroll speed 1.0x, BPM 120
    *   **When** the note position is calculated
    *   **Then** the note is positioned 320 pixels above the receptor line (4 beats × 80 pixels/beat)

*   **Scenario 2: BPM change handled smoothly**
    *   **Given** BPM changes from 120 to 180 at beat 8.0
    *   **When** notes before and after the change are rendered
    *   **Then** no visual jump occurs at the BPM boundary

*   **Scenario 3: Stop freezes note scroll**
    *   **Given** a 2-second stop at beat 16.0
    *   **When** the stop is active
    *   **Then** all notes remain stationary for 2 seconds

*   **Scenario 4: Configurable receptor position**
    *   **Given** receptor_y is set to 400 instead of default 360
    *   **When** notes are rendered
    *   **Then** the target line is drawn at y=400

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5
*   **Dependencies**: REQ-CHT-003 (timing data from chart system)
*   **Related Stories**: US-REN-020 (placeholder rendering), US-REN-026 (speed mods)
*   **NFR**: Must not cause stuttering or frame drops during BPM changes

---

### Story ID: US-REN-020 - Placeholder Rectangle Note Rendering

**Story Card:**
> **As a** Developer
> **I want** notes rendered as colored rectangles in Phase 1
> **So that** I can verify the scroll math without dependency on sprite assets

**Status**: PLANNED (Phase 1)

### 📝 Description
Phase 1 note renderer draws simple colored rectangles (one color per column) at calculated screen positions to validate the beat-to-screen conversion logic.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Five distinct column colors**
    *   **Given** single mode (5 columns)
    *   **When** one note per column is rendered
    *   **Then** each note has a distinct color (red, blue, green, yellow, magenta)

*   **Scenario 2: Rectangle size consistency**
    *   **Given** notes at various beat positions
    *   **When** rendered
    *   **Then** all rectangles are 48×48 pixels

*   **Scenario 3: Visually reasonable spacing**
    *   **Given** 4 consecutive quarter notes
    *   **When** rendered
    *   **Then** spacing between notes is proportional to beat distance

*   **Scenario 4: Replaceable without changing judge**
    *   **Given** the placeholder renderer is replaced with sprite renderer
    *   **When** the judge is tested
    *   **Then** no judge logic changes are required

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-REN-019
*   **Implementation Note**: Intentionally minimal to unblock Phase 1 gameplay testing
*   **Related Stories**: US-REN-021 (sprite-based replacement)

---

### Story ID: US-REN-021 - Sprite-Based Note Skins

**Story Card:**
> **As a** Player
> **I want** notes to use sprite graphics from a note skin
> **So that** the game has visual polish matching the original Pump It Up

**Status**: PLANNED (Phase 2)

### 📝 Description
The note renderer loads sprite-based note skins from directories containing a manifest JSON and sprite files for each column direction, receptors, hold bodies/caps, and judgment/combo numbers.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Note skin manifest loads**
    *   **Given** a note skin directory with "manifest.json" defining sprite paths
    *   **When** the skin is loaded
    *   **Then** all sprite references are resolved and cached

*   **Scenario 2: Column sprites for single mode**
    *   **Given** a skin with 5 sprites (down-left, up-left, center, up-right, down-right)
    *   **When** a single mode chart is rendered
    *   **Then** each column uses its corresponding sprite

*   **Scenario 3: Column sprites for double mode**
    *   **Given** a skin with 10 sprites (5 per player side)
    *   **When** a double mode chart is rendered
    *   **Then** all 10 column sprites are used

*   **Scenario 4: Missing skin fallback**
    *   **Given** no note skin is configured
    *   **When** the renderer initializes
    *   **Then** the placeholder rectangle renderer is used with a warning logged

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5
*   **Dependencies**: US-REN-003 (sprite loading), US-REN-020 (replaces placeholder)
*   **Related Stories**: US-REN-022 (receptors), US-REN-024 (holds)
*   **NFR**: Skin loading should complete in under 500ms

---

### Story ID: US-REN-022 - Receptor Rendering

**Story Card:**
> **As a** Player
> **I want** receptor sprites at the judgment line
> **So that** I know where to time my steps

**Status**: PLANNED (Phase 2)

### 📝 Description
Receptors are static sprites drawn at the configured receptor line position for each column. They represent the target timing zone.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Receptors at judgment line**
    *   **Given** receptor_y is 360
    *   **When** the note field is rendered
    *   **Then** receptor sprites are drawn at y=360 for each column

*   **Scenario 2: Receptor count matches mode**
    *   **Given** single mode is active
    *   **When** the note field is rendered
    *   **Then** exactly 5 receptors are drawn

*   **Scenario 3: Double mode receptor layout**
    *   **Given** double mode is active
    *   **When** the note field is rendered
    *   **Then** 10 receptors span the full width of the 640px space

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-REN-021
*   **Related Stories**: US-REN-028 (hit effects on receptors)

---

### Story ID: US-REN-023 - Judgment Display

**Story Card:**
> **As a** Player
> **I want** to see judgment text when I hit notes
> **So that** I get immediate feedback on my timing accuracy

**Status**: PLANNED (Phase 2)

### 📝 Description
Judgment sprites (Perfect, Great, Good, Bad, Miss) appear on screen for 0.5 seconds after each note judgment, positioned above the receptor line.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Perfect judgment displayed**
    *   **Given** a note is hit with timing error within ±15ms
    *   **When** the judgment occurs
    *   **Then** "Perfect" sprite appears for 0.5 seconds

*   **Scenario 2: Miss judgment displayed**
    *   **Given** a note passes the 135ms late window without being hit
    *   **When** the auto-miss triggers
    *   **Then** "Miss" sprite appears for 0.5 seconds

*   **Scenario 3: Judgment positioning**
    *   **Given** receptor line at y=360
    *   **When** a judgment sprite is displayed
    *   **Then** it is centered horizontally at y=280

*   **Scenario 4: Timing error display (optional)**
    *   **Given** timing display is enabled in settings
    *   **When** a note is hit 5ms early
    *   **Then** "-5ms" appears below the judgment sprite

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-REN-021, REQ-JDG-002 (judgment system)
*   **Related Stories**: US-REN-024 (combo display)

---

### Story ID: US-REN-024 - Combo Display

**Story Card:**
> **As a** Player
> **I want** to see my current combo count
> **So that** I can track my performance during gameplay

**Status**: PLANNED (Phase 2)

### 📝 Description
The combo counter displays the current number of consecutive hits (Perfect/Great/Good) using sprite-based number graphics. Updates in real-time and resets to 0 on Bad/Miss.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Combo increments on hit**
    *   **Given** current combo is 15
    *   **When** a note is judged Perfect
    *   **Then** the display updates to "16" within one frame

*   **Scenario 2: Combo resets on miss**
    *   **Given** current combo is 42
    *   **When** a note is judged Miss
    *   **Then** the display updates to "0"

*   **Scenario 3: Sprite-based number rendering**
    *   **Given** combo is 123
    *   **When** rendered
    *   **Then** three sprite digits "1", "2", "3" are drawn horizontally

*   **Scenario 4: Combo positioning**
    *   **Given** default UI layout
    *   **When** the combo is displayed
    *   **Then** it appears in the upper center at y=100

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-REN-021, REQ-JDG-002
*   **Related Stories**: US-REN-023 (judgment display)

---

### Story ID: US-REN-025 - Hold Note Body Rendering

**Story Card:**
> **As a** Player
> **I want** hold notes to show a connecting body between head and tail
> **So that** I know when to keep holding the panel

**Status**: PLANNED (Phase 3)

### 📝 Description
Hold notes render with a visible body sprite connecting the head and tail notes. The body tiles or stretches vertically and is visually distinct when active vs inactive.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Body connects head to tail**
    *   **Given** a hold from beat 4.0 to beat 8.0 (4-beat duration)
    *   **When** the hold is rendered
    *   **Then** a vertical body sprite spans from head to tail

*   **Scenario 2: Active hold visual state**
    *   **Given** a hold's head has been judged and panel is held
    *   **When** the hold is rendered
    *   **Then** the body uses the "active" sprite variant (brighter/highlighted)

*   **Scenario 3: Inactive hold visual state**
    *   **Given** a hold's head has not been hit
    *   **When** the hold is rendered
    *   **Then** the body uses the "inactive" sprite variant

*   **Scenario 4: Holds through BPM changes**
    *   **Given** a hold spanning a BPM change from 120 to 180
    *   **When** the hold is rendered
    *   **Then** the body length adjusts smoothly without gaps

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5
*   **Dependencies**: US-REN-021, REQ-JDG-006 (hold note judge logic)
*   **Related Stories**: US-REN-026 (hold caps)

---

### Story ID: US-REN-026 - Hold Note Cap Rendering

**Story Card:**
> **As a** Player
> **I want** hold note heads and tails to have distinct cap sprites
> **So that** I can visually distinguish hold endpoints from tap notes

**Status**: PLANNED (Phase 3)

### 📝 Description
Hold head and tail use separate cap sprites distinct from tap note sprites, making holds immediately recognizable.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Head cap differs from tap**
    *   **Given** a hold note head and a tap note in the same column
    *   **When** both are rendered
    *   **Then** the hold head uses a distinct sprite (e.g., brighter or with arrow)

*   **Scenario 2: Tail cap visually distinct**
    *   **Given** a hold note tail
    *   **When** rendered
    *   **Then** it uses a tail-specific sprite (e.g., downward arrow or flat cap)

*   **Scenario 3: Caps align with body**
    *   **Given** a hold with body and caps
    *   **When** rendered
    *   **Then** the caps seamlessly connect to the body edges (no gaps)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-REN-025
*   **Implementation Note**: Sprite manifest must define head_cap and tail_cap per column

---

### Story ID: US-REN-027 - BGA Background During Gameplay

**Story Card:**
> **As a** Player
> **I want** BGA animations to play behind the note field
> **So that** gameplay has dynamic backgrounds synchronized to the music

**Status**: PLANNED (Phase 2)

### 📝 Description
The BGA animation system (existing) is integrated into GameplayScene, rendering behind the note field. BGA tick counter is driven by audio position from the audio system.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: BGA renders below notes**
    *   **Given** a song with BGA and note field
    *   **When** gameplay is active
    *   **Then** BGA is drawn first (z-order below notes)

*   **Scenario 2: BGA synchronized to audio**
    *   **Given** audio playback position is 2.5 seconds (tick 150)
    *   **When** the BGA is rendered
    *   **Then** the frame at tick 150 is displayed

*   **Scenario 3: Missing BGA does not block gameplay**
    *   **Given** a song with no BGA file
    *   **When** gameplay starts
    *   **Then** the note field renders normally with a black background

*   **Scenario 4: BGA during pause (design decision TBD)**
    *   **Given** gameplay is paused
    *   **When** the pause screen is active
    *   **Then** BGA either freezes or continues (configurable)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-REN-010 (BGA system exists), REQ-AUD-002 (audio position tracking)
*   **Related Stories**: US-REN-019 (note field rendering)
*   **NFR**: BGA rendering must not impact note field frame rate

---

### Story ID: US-REN-028 - Hit Effects and Receptor Flash

**Story Card:**
> **As a** Player
> **I want** receptors to flash when I hit notes
> **So that** I get visual confirmation of successful inputs

**Status**: PLANNED (Phase 3)

### 📝 Description
When a note is successfully judged, the corresponding receptor lights up or flashes for 0.1 seconds. Optional particle effects appear on Perfect judgments.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Receptor flash on hit**
    *   **Given** a note is judged in column 2
    *   **When** the judgment occurs
    *   **Then** receptor 2 switches to "lit" sprite for 0.1 seconds

*   **Scenario 2: No flash on miss**
    *   **Given** a note is judged Miss
    *   **When** the judgment occurs
    *   **Then** no receptor flash occurs

*   **Scenario 3: Perfect particle effect (optional)**
    *   **Given** perfect_effects is enabled in settings
    *   **When** a note is judged Perfect
    *   **Then** a particle effect sprite plays at the receptor position

*   **Scenario 4: Effects do not obscure notes**
    *   **Given** a hit effect is playing
    *   **When** incoming notes are approaching
    *   **Then** notes remain clearly visible (effects are translucent or brief)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-REN-021, REQ-JDG-009 (judgment events)
*   **Related Stories**: US-REN-022 (receptor rendering)

---

### Story ID: US-REN-029 - Life Gauge Visual Rendering

**Story Card:**
> **As a** Player
> **I want** to see my current life gauge as a visual bar
> **So that** I know when I'm at risk of failing

**Status**: PLANNED (Phase 3)

### 📝 Description
The life gauge is rendered as a horizontal bar filling 0-100% based on the current HP value from GameplayState. Visual feedback (red flash on drain, green on recovery) provides additional cues.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Bar fills proportionally to HP**
    *   **Given** current HP is 75 out of 100
    *   **When** the life gauge is rendered
    *   **Then** the bar is 75% filled

*   **Scenario 2: Red flash on drain**
    *   **Given** HP decreases from 60 to 40 due to a Miss
    *   **When** the gauge updates
    *   **Then** the bar flashes red for 0.2 seconds

*   **Scenario 3: Green flash on recovery**
    *   **Given** HP increases from 40 to 50 due to Perfect hits
    *   **When** the gauge updates
    *   **Then** the bar flashes green for 0.2 seconds

*   **Scenario 4: Fail state indicated**
    *   **Given** HP reaches 0
    *   **When** the gauge is rendered
    *   **Then** the bar turns red/empty and displays "FAIL" text

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: REQ-JDG-013 (life gauge logic)
*   **Related Stories**: US-REN-023 (judgment display)
*   **NFR**: Gauge updates must be smooth (interpolate bar fill, not snap)

---

### Story ID: US-REN-030 - Single Mode Note Field Layout

**Story Card:**
> **As a** Player
> **I want** single mode to display 5 columns centered in the play area
> **So that** the note field matches the classic Pump It Up layout

**Status**: PLANNED (Phase 5)

### 📝 Description
Single mode renders 5 columns (down-left, up-left, center, up-right, down-right) centered horizontally in the 640×480 virtual space with configurable column spacing.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Five columns centered**
    *   **Given** single mode is active
    *   **When** the note field is rendered
    *   **Then** columns are centered at x=320 with equal spacing

*   **Scenario 2: Configurable column spacing**
    *   **Given** column_spacing is set to 60 pixels
    *   **When** the note field is rendered
    *   **Then** adjacent columns are 60 pixels apart

*   **Scenario 3: Receptor line position**
    *   **Given** receptor_y is 360
    *   **When** the note field is rendered
    *   **Then** all receptors are at y=360

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-REN-021
*   **Related Stories**: US-REN-031 (double mode)

---

### Story ID: US-REN-031 - Double Mode Note Field Layout

**Story Card:**
> **As a** Player
> **I want** double mode to display 10 columns spanning the full width
> **So that** I can play double charts with both sides visible

**Status**: PLANNED (Phase 5)

### 📝 Description
Double mode renders 10 columns (5 per player) spanning the full 640px width with separate spacing per side.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Ten columns spanning full width**
    *   **Given** double mode is active
    *   **When** the note field is rendered
    *   **Then** 5 left columns start at x=0 and 5 right columns end at x=640

*   **Scenario 2: Visual separation between sides**
    *   **Given** double mode is active
    *   **When** the note field is rendered
    *   **Then** a gap or divider appears between the two 5-column sets

*   **Scenario 3: Per-side column spacing**
    *   **Given** column_spacing is 55 pixels
    *   **When** double mode is rendered
    *   **Then** each side's 5 columns are spaced 55 pixels apart

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-REN-030
*   **Related Stories**: US-REN-021 (must support 10 column sprites)

---

### Story ID: US-REN-032 - C-Mod Speed Modifier

**Story Card:**
> **As a** Player
> **I want** to use constant scroll speed (C-mod)
> **So that** notes scroll at a fixed rate regardless of BPM changes

**Status**: PLANNED (Phase 5)

### 📝 Description
C-mod (constant modifier) makes notes scroll at a fixed pixels-per-second rate. BPM changes do not affect visual scroll speed, only the spacing between notes.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Fixed scroll rate at 120 BPM**
    *   **Given** C-mod is set to 400 pixels/second and BPM is 120
    *   **When** notes scroll
    *   **Then** they move at 400 pixels/second

*   **Scenario 2: Fixed scroll rate at 180 BPM**
    *   **Given** C-mod is 400 pixels/second and BPM changes to 180
    *   **When** notes scroll
    *   **Then** they still move at 400 pixels/second (spacing adjusts)

*   **Scenario 3: Chart data unaffected**
    *   **Given** C-mod is active
    *   **When** the chart is loaded and judged
    *   **Then** no chart data or timing is modified

*   **Scenario 4: Speed change does not affect audio sync**
    *   **Given** C-mod is changed mid-song
    *   **When** notes continue scrolling
    *   **Then** audio position remains the authoritative timing source

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5
*   **Dependencies**: US-REN-019
*   **Related Stories**: US-REN-033 (M-mod)
*   **Implementation Note**: Modifies pixels-per-beat calculation in renderer only

---

### Story ID: US-REN-033 - M-Mod Speed Modifier

**Story Card:**
> **As a** Player
> **I want** to use multiplied scroll speed (M-mod)
> **So that** I can adjust note visibility to my reading preference

**Status**: PLANNED (Phase 5)

### 📝 Description
M-mod multiplies the base scroll speed by a factor (typically 1x-8x). Visual scroll speed scales with BPM, maintaining the original game's scroll behavior.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: 2x multiplier doubles scroll speed**
    *   **Given** base scroll speed is 200 pixels/second and M-mod is 2.0x
    *   **When** notes scroll
    *   **Then** they move at 400 pixels/second

*   **Scenario 2: Scroll scales with BPM**
    *   **Given** M-mod is 2.0x and BPM changes from 120 to 180
    *   **When** notes scroll
    *   **Then** scroll speed increases proportionally (1.5x faster)

*   **Scenario 3: Typical range 1x-8x**
    *   **Given** M-mod is set to 0.5x, 1.0x, 4.0x, or 8.0x
    *   **When** the renderer initializes
    *   **Then** all values are accepted and applied correctly

*   **Scenario 4: Chart data unaffected**
    *   **Given** M-mod is active
    *   **When** the chart is loaded and judged
    *   **Then** no chart data or timing is modified

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 3
*   **Dependencies**: US-REN-019
*   **Related Stories**: US-REN-032 (C-mod)

---

### Story ID: US-REN-034 - High Refresh Rate Rendering

**Story Card:**
> **As a** Player with a 144Hz monitor
> **I want** smooth rendering at my display's refresh rate
> **So that** note scrolling is fluid without stuttering

**Status**: PLANNED (Phase 2)

### 📝 Description
The renderer runs at the display's refresh rate (120Hz, 144Hz, etc.) while game logic runs at 60Hz. Note positions are interpolated between logic ticks for smooth visuals without introducing input lag.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Rendering at 144Hz**
    *   **Given** a 144Hz display and 60Hz logic tick rate
    *   **When** gameplay runs
    *   **Then** rendering occurs at 144fps (verified via frame timer)

*   **Scenario 2: Position interpolation between ticks**
    *   **Given** a note moves from y=400 to y=380 over one logic tick (16.67ms)
    *   **When** rendered at 144Hz (6.94ms per frame)
    *   **Then** the note position is interpolated smoothly across 2.4 render frames

*   **Scenario 3: No stuttering on high refresh displays**
    *   **Given** a 144Hz display
    *   **When** notes scroll continuously
    *   **Then** no visible stuttering or judder occurs

*   **Scenario 4: No input lag introduced**
    *   **Given** interpolation is active
    *   **When** a player presses an input
    *   **Then** the input is processed in the next logic tick (not delayed)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 5
*   **Dependencies**: REQ-ENG-001 (fixed-step loop), US-REN-002, US-REN-021 (needs sprite state for meaningful interpolation)
*   **Phase**: 2 (Phase 1 runs at vsync refresh only)
*   **Related Stories**: US-REN-019 (note rendering)
*   **NFR**: Frame time must be stable within ±1ms on target hardware

---

### Story ID: US-REN-036 - Minimal Timing Feedback Display

**Story Card:**
> **As a** Player
> **I want** to see my most recent judgment on screen
> **So that** I get immediate feedback on my timing accuracy

**Status**: PLANNED (Phase 1)

### 📝 Description
Render the most recent judgment as colored text or a colored rectangle on screen during Phase 1 gameplay. This provides minimal timing feedback before sprite-based judgment display is implemented in Phase 2.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Judgment text rendered on screen**
    *   **Given** the judge issues a "Perfect" judgment
    *   **When** the gameplay scene renders
    *   **Then** the text "Perfect" appears in green at the center-top of the screen

*   **Scenario 2: Judgment updates each hit**
    *   **Given** the player hits three notes in sequence (Perfect, Great, Miss)
    *   **When** each judgment is issued
    *   **Then** the displayed judgment updates to show the most recent result

*   **Scenario 3: Distinct colors per judgment type**
    *   **Given** judgments for Perfect, Great, Good, Bad, Miss
    *   **When** each is displayed
    *   **Then** each uses a distinct color (e.g., green, blue, yellow, orange, red)

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 2
*   **Dependencies**: US-JDG-002 (judgment types)
*   **Phase**: 1
*   **Implementation Note**: SDL_Renderer can draw colored rectangles or use simple text rendering. Replaced by US-REN-023 (sprite-based) in Phase 2.

---

### Story ID: US-REN-035 - Note Field Rendering Performance

**Story Card:**
> **As a** Developer
> **I want** the note renderer to maintain 60 FPS with 100+ notes visible
> **So that** complex charts do not cause frame drops

**Status**: PLANNED (Phase 2)

### 📝 Description
The note renderer uses sprite batching and texture atlases to minimize draw calls, maintaining 60 FPS (or higher on capable displays) with 100+ notes on screen.

### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: No frame drops with 100 notes**
    *   **Given** a chart section with 100 notes simultaneously visible
    *   **When** that section is rendered
    *   **Then** frame rate remains at 60 FPS or higher

*   **Scenario 2: Sprite batching for efficiency**
    *   **Given** 50 notes using the same texture
    *   **When** rendered
    *   **Then** all 50 are drawn in a single batched draw call

*   **Scenario 3: CPU usage under 20% for rendering**
    *   **Given** gameplay is active with 80 notes visible
    *   **When** CPU usage is measured
    *   **Then** the rendering subsystem uses under 20% of one core

*   **Scenario 4: Texture atlas reduces draw calls**
    *   **Given** note skin sprites are packed into a single texture atlas
    *   **When** the note field is rendered
    *   **Then** total draw calls are under 10 per frame

### 📊 Technical Notes & Constraints
*   **Estimation Pointer**: 8
*   **Dependencies**: US-REN-021
*   **Implementation Note**: Requires SDL3 texture atlas or render batching
*   **NFR**: Target hardware: Intel i5 or equivalent, integrated graphics

---

## Non-Functional Requirements

### NFR-REN-001: Visual Quality
The rendering system must maintain visual fidelity comparable to original PIU Exceed era graphics at modern resolutions.

**Acceptance Criteria:**
*   Letterboxing preserves 4:3 aspect ratio on all display sizes
*   Linear filtering applied to scaled textures (no pixelation)
*   Blend modes approximate original game's lighting effects within SDL3 limitations
*   Color accuracy within ±5 per channel compared to reference images

**Estimation Pointer**: N/A (covered by existing stories)

---

### NFR-REN-002: Frame Timing Stability
Frame rendering must be stable and predictable to avoid disrupting player timing.

**Acceptance Criteria:**
*   Frame time variance under ±2ms at 60 FPS
*   No stuttering during BPM changes or stops
*   High refresh rate displays (120Hz+) render smoothly without judder
*   vsync or adaptive sync supported

**Estimation Pointer**: N/A (architectural requirement)

---

### NFR-REN-003: Asset Loading Performance
Visual assets must load quickly to minimize wait times.

**Acceptance Criteria:**
*   Note skin loading completes in under 500ms
*   BGA loading completes in under 1 second for files up to 5MB
*   Texture cache preloading during song select (asynchronous)
*   No frame drops during lazy texture loading

**Estimation Pointer**: N/A (optimization work)

---

### NFR-REN-004: Memory Efficiency
The rendering system must manage memory efficiently for long gameplay sessions.

**Acceptance Criteria:**
*   Texture cache evicts unused textures after 2 minutes
*   Total texture memory usage under 512MB for typical song
*   BGA animations do not leak memory over 1-hour session
*   Sprite data is shared across instances (no duplication)

**Estimation Pointer**: N/A (profiling and optimization)

---

## Story Dependencies (Cross-File)

### From Chart System (04-chart-system.md)
*   **REQ-CHT-003**: TimingData for beat-to-time conversion (used by US-REN-019, US-REN-027, US-REN-032)

### From Judge System (05-judge-system.md)
*   **REQ-JDG-002**: Judgment events for feedback (used by US-REN-023, US-REN-024, US-REN-028)
*   **REQ-JDG-006**: Hold note logic for active state (used by US-REN-025)
*   **REQ-JDG-009**: Hit events for receptor flash (used by US-REN-028)
*   **REQ-JDG-013**: Life gauge state (used by US-REN-029)

### From Audio System (03-audio-system.md)
*   **REQ-AUD-002**: Audio position tracking (used by US-REN-027)

### From Engine Loop (01-engine-loop.md)
*   **REQ-ENG-001**: Fixed-step game loop (used by US-REN-034)

### To Asset Management (11-asset-management.md)
*   **US-REN-021**: Note skin loading requires asset directory scanning
*   **US-REN-027**: BGA file discovery requires asset management

---

## Story Point Summary

### DONE Stories (Epic: BGA Animation Stack)
*   US-REN-001: 1 (SDL3 renderer)
*   US-REN-002: 1 (frame loop)
*   US-REN-003: 2 (SPRJ loading)
*   US-REN-004: 3 (SPR binary loading)
*   US-REN-005: 3 (texture cache)
*   US-REN-006: 2 (SP2 loading)
*   US-REN-007: 2 (TILE mode)
*   US-REN-008: 2 (ANI mode)
*   US-REN-009: 3 (PATTERN mode)
*   US-REN-010: 3 (BGAJ loading)
*   US-REN-011: 3 (BGA binary loading)
*   US-REN-012: 5 (interpolated properties)
*   US-REN-013: 3 (non-interpolated properties)
*   US-REN-014: 2 (visibility window)
*   US-REN-015: 2 (layer compositing)
*   US-REN-016: 3 (blend modes)
*   US-REN-017: 3 (BGA player tool)
*   US-REN-018: 5 (regression tests)
*   **DONE Total: 48 points**

### PLANNED Stories (Epic: Note Renderer)
*   US-REN-019: 5 (beat-space conversion)
*   US-REN-020: 2 (placeholder rectangles)
*   US-REN-021: 5 (sprite-based skins)
*   US-REN-022: 2 (receptors)
*   US-REN-023: 3 (judgment display)
*   US-REN-024: 3 (combo display)
*   US-REN-025: 5 (hold bodies)
*   US-REN-026: 2 (hold caps)
*   US-REN-027: 3 (BGA during gameplay)
*   US-REN-028: 3 (hit effects)
*   US-REN-029: 3 (life gauge rendering)
*   US-REN-030: 2 (single mode layout)
*   US-REN-031: 3 (double mode layout)
*   US-REN-032: 5 (C-mod)
*   US-REN-033: 3 (M-mod)
*   US-REN-034: 5 (high refresh rate)
*   US-REN-035: 8 (performance)
*   **PLANNED Total: 62 points**

---

## Personas Defined

*   **Player**: End user playing songs, expects smooth visuals and clear feedback
*   **Developer**: Engine contributor implementing and testing rendering features
*   **Content Creator**: Modder or asset creator authoring sprites, BGAs, and note skins

---

## Notes for Implementation

1. **DONE vs PLANNED Marking**: All BGA animation stack stories (US-REN-001 through US-REN-018) are marked DONE with references to existing tests/code in CLAUDE.md.

2. **Testable Acceptance Criteria**: Every AC uses concrete values (e.g., "360 pixels", "0.5 seconds", "±2 per channel tolerance") to ensure they are measurable.

3. **No Separate Verification Stories**: Testing is embedded in ACs. Regression tests (US-REN-018) serve as the verification mechanism for the entire BGA stack.

4. **Grammar Consistency**: All story cards follow "I want [noun/infinitive verb phrase]" format.

5. **Story Sizing**: Largest story is US-REN-035 at 8 points (performance optimization). Most are 2-5 points (small, estimable).

6. **Cross-File Dependencies**: Clearly documented in dedicated section linking to chart, judge, audio, and engine systems.
