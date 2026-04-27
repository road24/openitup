# Visual Rendering Requirements

## REQ-REN-001: SDL3 Renderer with Logical Resolution
**Status**: [DONE]  
**Priority**: Must Have

The engine must provide a `Renderer` class owning SDL_Window and SDL_Renderer, configured for 640x480 logical resolution with letterboxing.

**Acceptance Criteria**:
- 640x480 virtual coordinate space maintained at all display resolutions
- Aspect ratio preserved with black bars on sides or top/bottom
- Smooth scaling (linear filtering) applied
- Works on displays from 720p to 4K

**Dependencies**: None  
**Source**: CLAUDE.md, Roadmap current state

---

## REQ-REN-002: Frame Render Loop
**Status**: [DONE]  
**Priority**: Must Have

The renderer must provide `begin_frame()` and `end_frame()` methods for the render loop, handling clear and present operations.

**Acceptance Criteria**:
- begin_frame() clears to black
- end_frame() presents to display with vsync or uncapped
- Frame timing independent of update rate
- No tearing artifacts

**Dependencies**: REQ-REN-001  
**Source**: CLAUDE.md, Roadmap current state

---

## REQ-REN-003: Sprite Loading and Rendering
**Status**: [DONE]  
**Priority**: Must Have

The engine must load and render sprites from SPRJ (JSON) and legacy SPR/SP2 formats with three rendering modes: TILE, ANI, PATTERN.

**Acceptance Criteria**:
- SPRJ JSON format loads correctly
- SPR and SP2 binary formats load correctly (with converters)
- TILE mode draws all pictures back-to-front
- ANI mode selects single frame based on normalized time parameter
- PATTERN mode tiles pictures into grid

**Dependencies**: REQ-REN-001, REQ-AST-001  
**Source**: CLAUDE.md, Roadmap current state

---

## REQ-REN-004: BGA Animation System
**Status**: [DONE]  
**Priority**: Must Have

The engine must render BGA animations from BGAJ (JSON) and binary BGA formats with full keyframe interpolation.

**Acceptance Criteria**:
- Loads BGAJ JSON and binary BGA formats
- Interpolates translate, scale, rotate, color, alpha at 60 ticks/second
- Non-interpolated properties (pivot, effect, display) snap at keyframes
- Up to 50 layers composited via painter's algorithm
- Five blend modes: normal, screen, multiply, dodge, difference

**Dependencies**: REQ-REN-003  
**Source**: CLAUDE.md, Roadmap current state

---

## REQ-REN-005: Texture Cache with Case-Insensitive Probing
**Status**: [DONE]  
**Priority**: Must Have

The engine must cache loaded textures and probe for .tga, .png, .dds formats in case-insensitive manner.

**Acceptance Criteria**:
- Textures loaded once, reused across sprites
- Case-insensitive file lookup (linux compatibility)
- Probes .tga, .png, .dds in order
- LRU eviction when cache exceeds limit (optional)

**Dependencies**: REQ-REN-001  
**Source**: CLAUDE.md, Roadmap current state

---

## REQ-REN-006: Visual Regression Test Suite
**Status**: [DONE]  
**Priority**: Should Have

The engine must include regression tests rendering BGA frames and comparing pixel-by-pixel against reference images.

**Acceptance Criteria**:
- 17 regression tests covering 30 reference images
- Tolerance <= 2 per RGB channel
- Tests cover interpolated and non-interpolated properties
- Tests cover visibility windows and blend modes

**Dependencies**: REQ-REN-004  
**Source**: CLAUDE.md, Roadmap current state

---

## REQ-REN-007: Note Renderer Beat-Space Scrolling
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

Notes must scroll in beat-space, not time-space. The renderer converts note beat position to screen position using current scroll speed and BPM.

**Acceptance Criteria**:
- Notes correctly handle BPM changes (no visual discontinuities)
- Stops in timing data freeze note scroll
- Speed modifiers applied at render time, not stored in chart
- Receptor line position configurable

**Dependencies**: REQ-CHT-003  
**Source**: Roadmap subsystem 6

---

## REQ-REN-008: Placeholder Rectangle Note Rendering
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

Phase 1 note renderer must use simple colored rectangles to prove scroll math works before sprite integration.

**Acceptance Criteria**:
- Each of 5 columns has distinct color
- Note size and spacing visually reasonable
- Scroll speed formula verified correct
- Can be replaced by sprite renderer without changing judge

**Dependencies**: REQ-REN-007  
**Source**: Roadmap subsystem 6, Phase 1

---

## REQ-REN-009: Sprite-Based Note Skins
**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

The note renderer must support sprite-based note skins loaded from directories with manifests.

**Acceptance Criteria**:
- Note skin directory contains manifest JSON
- Sprites for each column direction (5 for single, 10 for double)
- Receptor sprites for panel targets
- Hold body and cap sprites
- Judgment and combo number sprites

**Dependencies**: REQ-REN-003, REQ-REN-008  
**Source**: Roadmap subsystem 6, Phase 2

---

## REQ-REN-010: Single and Double Mode Layouts
**Status**: [PLANNED Phase 5]  
**Priority**: Must Have

The note field must support single mode (5 columns centered) and double mode (10 columns full width) layouts.

**Acceptance Criteria**:
- Single mode: 5 columns centered in 640x480 space
- Double mode: 10 columns spanning full width
- Column spacing configurable per skin
- Receptor line position configurable per mode

**Dependencies**: REQ-REN-009  
**Source**: Roadmap subsystem 6, Phase 5

---

## REQ-REN-011: Hold Note Rendering
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

The note renderer must draw hold note bodies (connecting head to tail) and caps.

**Acceptance Criteria**:
- Hold body sprites tile or stretch from head to tail
- Head and tail caps use distinct sprites
- Active holds visually distinct from inactive
- Holds correctly handle stops and BPM changes

**Dependencies**: REQ-REN-009, REQ-JDG-006  
**Source**: Roadmap subsystem 6, Phase 3

---

## REQ-REN-012: Speed Modifier Rendering
**Status**: [PLANNED Phase 5]  
**Priority**: Should Have

The renderer must support C-mod (constant scroll speed) and M-mod (multiplied scroll speed) modifiers applied at render time.

**Acceptance Criteria**:
- C-mod: notes scroll at fixed pixels/second regardless of BPM
- M-mod: base scroll speed multiplied by factor (1x-8x typical)
- Modifiers do not affect chart data or judge timing
- Speed change does not affect audio synchronization

**Dependencies**: REQ-REN-007  
**Source**: Roadmap subsystem 6, Phase 5

---

## REQ-REN-013: BGA Background During Gameplay
**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

BGA animations must play behind the note field during gameplay, synchronized to audio position.

**Acceptance Criteria**:
- BGA tick counter driven by audio position
- BGA renders before note field (correct z-order)
- BGA continues playing during pause (or freezes based on design)
- Missing BGA does not prevent gameplay

**Dependencies**: REQ-REN-004, REQ-AUD-002  
**Source**: Roadmap subsystem 6, Phase 2

---

## REQ-REN-014: Judgment and Combo Display
**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

The renderer must display judgment text/sprites and current combo using sprite-based numbers and text.

**Acceptance Criteria**:
- Judgment appears on hit (Perfect, Great, Good, Bad, Miss)
- Combo number updates and displays prominently
- Timing error (early/late ms) shown optionally
- Judgment and combo positioning configurable

**Dependencies**: REQ-REN-009, REQ-JDG-002  
**Source**: Roadmap subsystem 6, Phase 2

---

## REQ-REN-015: Life Gauge Rendering
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

The renderer must display the life gauge as a visual bar reflecting current HP percentage.

**Acceptance Criteria**:
- Bar fills 0-100% based on GameplayState life value
- Visual feedback on drain (red flash) and recovery (green flash)
- Fail state visually indicated when life reaches 0
- Position and style configurable per skin

**Dependencies**: REQ-JDG-013  
**Source**: Roadmap Phase 3

---

## REQ-REN-016: Hit Effects and Feedback
**Status**: [PLANNED Phase 3]  
**Priority**: Should Have

The renderer must show hit effects when notes are successfully judged (particle effects, receptor flash).

**Acceptance Criteria**:
- Receptor lights up or flashes on hit
- Optional particle effects on Perfect judgment
- Effects duration and intensity configurable
- Effects do not obscure incoming notes

**Dependencies**: REQ-REN-009, REQ-JDG-009  
**Source**: Roadmap Phase 3

---

## REQ-REN-017: High Refresh Rate Rendering
**Status**: [PLANNED Phase 1]  
**Priority**: Should Have

The renderer must support high refresh rate displays (120Hz, 144Hz) with smooth interpolation between logic ticks.

**Acceptance Criteria**:
- Rendering runs at display refresh rate
- Note positions interpolated between 60Hz logic ticks
- No stuttering on high refresh rate displays
- Interpolation does not introduce input lag

**Dependencies**: REQ-ENG-001, REQ-REN-002  
**Source**: Roadmap architecture decisions

---

## REQ-REN-018: Note Field Rendering Performance
**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

The note renderer must maintain 60 FPS with 100+ notes visible on screen simultaneously.

**Acceptance Criteria**:
- No frame drops with 100 notes visible
- Sprite batching for efficient rendering
- Draw calls minimized via texture atlases
- CPU usage under 20% on target hardware during rendering

**Dependencies**: REQ-REN-009  
**Source**: Roadmap performance targets (implied)
