# Gameplay Design Requirements

This subsystem defines specification requirements for gameplay mechanics. These are design documents that must exist before gameplay polishing can begin. Phase 1 implemented hardcoded Exceed-era values; these specifications formalize the exact mechanics that must be supported.

**Subsystem Prefix**: GPD

---

## REQ-GPD-001: Timing Windows Specification

**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

**Description**: A formal specification document defining exact timing windows (in milliseconds) for each judgment tier (Perfect/Great/Good/Bad/Miss) across all supported difficulty levels and game versions. Timing windows determine how precisely a player must hit a note to achieve each judgment.

**Acceptance Criteria**:
- Document specifies timing windows for all judgment tiers
- Values provided for each difficulty level (Easy, Normal, Hard, Crazy, Freestyle, Nightmare)
- Values provided for each supported game version (at minimum: Exceed era)
- All values expressed in milliseconds with ±X ms notation
- Document explains how timing windows interact with input polling rate
- Specification includes visual diagrams showing window overlaps/boundaries

**Dependencies**: None  
**Source**: PO request for gameplay mechanics specification (2026-04-29)

---

## REQ-GPD-002: Scoring Formula Specification

**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

**Description**: A formal specification document defining the exact formula for score calculation, including base points per judgment tier, combo multiplier progression, and any bonus scoring mechanics. This specification enables consistent scoring behavior across all charts and difficulties.

**Acceptance Criteria**:
- Document specifies base point values for each judgment tier
- Combo multiplier formula is defined with exact breakpoints
- Maximum achievable score formula is documented
- Edge cases documented (combo breaks, holds, mines if applicable)
- Examples provided showing score calculation for sample sequences
- Specification explains how scoring interacts with modifiers

**Dependencies**: REQ-GPD-001  
**Source**: PO request for gameplay mechanics specification (2026-04-29)

---

## REQ-GPD-003: Grade Thresholds Specification

**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

**Description**: A formal specification document defining the grade system (e.g., SS/S/A/B/C/D/F) and the exact thresholds for achieving each grade. Specifies whether grades are based on score percentage, accuracy percentage, combo, or other metrics.

**Acceptance Criteria**:
- All grade tiers are defined with labels
- Exact threshold values or percentage ranges for each grade
- Document specifies the metric used (score %, accuracy %, miss count, etc.)
- Edge cases documented (perfect full combo requirements, specific miss limits)
- Visual grade display rules specified (colors, labels, icons)
- Specification explains grade calculation timing (end of song vs real-time)

**Dependencies**: REQ-GPD-002  
**Source**: PO request for gameplay mechanics specification (2026-04-29)

---

## REQ-GPD-004: Life/Health Bar Specification

**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

**Description**: A formal specification document defining how the life/health bar system works, including initial state, fill/drain rates per judgment tier, fail conditions, and difficulty-specific scaling. This specification governs player survival and stage failure conditions.

**Acceptance Criteria**:
- Initial life bar value/percentage defined for all difficulty levels
- Exact fill amounts specified for positive judgments (Perfect/Great/Good)
- Exact drain amounts specified for negative judgments (Bad/Miss)
- Stage fail condition clearly defined (empty bar, below threshold, etc.)
- Recovery mechanics documented (if any)
- Specification explains life bar behavior during holds and long notes
- Visual representation rules defined (bar color, thresholds, danger states)

**Dependencies**: REQ-GPD-001  
**Source**: PO request for gameplay mechanics specification (2026-04-29)

---

## REQ-GPD-005: Hold Note Scoring Specification

**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

**Description**: A formal specification document defining how hold notes (long notes) are judged and scored, including head judgment, tick/sustain judgment during the hold, tail judgment, and partial credit rules. Hold notes have complex scoring interactions that differ from tap notes.

**Acceptance Criteria**:
- Hold head judgment timing windows defined
- Tick frequency and judgment during hold defined (if applicable)
- Hold tail judgment timing windows defined
- Partial credit rules specified (early release, missed head but held tail, etc.)
- Score contribution calculation for holds documented
- Life bar interaction specified for each hold phase
- Combo behavior during holds specified (per-tick vs head+tail only)
- Visual feedback rules for hold states defined

**Dependencies**: REQ-GPD-001, REQ-GPD-002, REQ-GPD-004  
**Source**: PO request for gameplay mechanics specification (2026-04-29)

---

## REQ-GPD-006: Miss Penalties Specification

**Status**: [PLANNED Phase 2]  
**Priority**: Must Have

**Description**: A formal specification document defining the exact behavior and consequences when a note is missed, including combo breaks, life drain, score impact, and judgment display. Clarifies edge cases like overlapping notes and simultaneous misses.

**Acceptance Criteria**:
- Combo break behavior on miss is specified
- Life bar drain amount for miss is defined
- Score penalty (if any beyond zero points) is specified
- Visual feedback for miss judgments defined
- Miss detection timing specified (when does a note become a miss)
- Multi-note miss behavior specified (simultaneous arrows)
- Miss grace period rules specified (if any)

**Dependencies**: REQ-GPD-001, REQ-GPD-002, REQ-GPD-004  
**Source**: PO request for gameplay mechanics specification (2026-04-29)

---

## REQ-GPD-007: Scrolling Mechanism Specification

**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

**Description**: A formal specification document defining how notes scroll on screen, including scroll speed calculation, BPM-locked vs constant-speed modes, receptor position, scroll direction options (upscroll/downscroll), and speed modifier interactions. This specification governs the visual timing presentation.

**Acceptance Criteria**:
- Scroll speed calculation formula defined (relation to BPM and speed mod)
- Receptor position on screen specified (y-coordinate, lane x-coordinates)
- Scroll direction options documented (upscroll, downscroll, centered)
- Speed modifier system explained (1x, 2x, C-mods, M-mods, etc.)
- Note spawn timing and culling rules specified
- Visual perspective/scaling rules defined (perspective warp, constant size, etc.)
- Specification explains scroll behavior during BPM changes and stops

**Dependencies**: REQ-CHT-001 (Chart Format)  
**Source**: PO request for gameplay mechanics specification (2026-04-29)

---

## REQ-GPD-008: Autosync Mechanism Specification

**Status**: [PLANNED Phase 3]  
**Priority**: Should Have

**Description**: A formal specification document defining the automatic audio synchronization system that detects and corrects timing offset drift between audio playback and chart timing. Autosync improves timing accuracy without manual calibration.

**Acceptance Criteria**:
- Detection algorithm specified (moving average, trend analysis, etc.)
- Adjustment threshold and rate defined (when to adjust, how much)
- Adjustment application method specified (immediate, gradual, per-song vs global)
- Minimum sample size for reliable detection specified
- Edge case handling documented (player deliberately off-timing, chart sync issues)
- User visibility and control options specified (enable/disable, manual override)
- Specification explains interaction with manual global offset setting

**Dependencies**: REQ-GPD-001, REQ-JDG-001 (Timing Engine)  
**Source**: PO request for gameplay mechanics specification (2026-04-29)

---

## REQ-GPD-009: Code Detection Specification

**Status**: [PLANNED Phase 3]  
**Priority**: Should Have

**Description**: A formal specification document defining the arcade-style code entry system where players input sequences on the dance pad to activate modifiers, unlock features, or trigger easter eggs. Common in arcade rhythm games as a traditional input method.

**Acceptance Criteria**:
- Code input detection method specified (which game states allow code entry)
- Input buffer size and timing constraints defined
- Valid code format specified (arrow sequences, button combinations)
- Known codes documented with their effects (speed mods, visual mods, unlocks)
- Feedback mechanism specified (audio/visual confirmation, rejection indication)
- Code persistence rules defined (per-session, per-song, global unlock)
- Specification explains collision handling with normal gameplay input

**Dependencies**: REQ-INP-001 (Input System)  
**Source**: PO request for gameplay mechanics specification (2026-04-29)

---

## REQ-GPD-010: Modifiers System Specification

**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

**Description**: A formal specification document defining all available gameplay modifiers and their exact effects, including speed modifiers (XMod, CMod, MMod), visual modifiers (Vanish, Sudden, Hidden, etc.), and pattern modifiers (Mirror, Shuffle, Random). Modifiers alter gameplay difficulty and visual presentation.

**Acceptance Criteria**:
- All modifier categories defined with complete modifier lists
- Exact behavior specified for each modifier (algorithmic changes, visual effects)
- Modifier incompatibility rules documented (mutually exclusive combinations)
- Modifier activation methods specified (code entry, menu selection, profile defaults)
- Modifier display and UI presentation rules defined
- Score/grade eligibility rules per modifier specified (some may disable ranking)
- Specification explains modifier interaction with chart features (stops, BPM changes, holds)
- Performance impact considerations documented

**Dependencies**: REQ-GPD-007, REQ-GPD-009  
**Source**: PO request for gameplay mechanics specification (2026-04-29)

---

## REQ-GPD-011: Gameplay Polishing Gate

**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

**Description**: A gate requirement establishing that gameplay polishing and refinement work cannot begin until all gameplay design specification documents (REQ-GPD-001 through REQ-GPD-010) are complete and reviewed. This ensures implementation is guided by clear, documented design rather than ad-hoc decisions.

**Acceptance Criteria**:
- All specification documents REQ-GPD-001 through REQ-GPD-010 marked [DONE]
- Each specification has been reviewed and approved by PO
- Specifications are internally consistent (no contradictions between documents)
- Any stories tagged as "gameplay polishing" or "refinement" are blocked until this gate passes
- Implementation work for hardcoded Phase 1 values must reference these specifications when refining
- Stories implementing these specifications are not blocked (this gate applies to polishing, not initial implementation)

**Dependencies**: REQ-GPD-001, REQ-GPD-002, REQ-GPD-003, REQ-GPD-004, REQ-GPD-005, REQ-GPD-006, REQ-GPD-007, REQ-GPD-008, REQ-GPD-009, REQ-GPD-010, REQ-CHT-023 (Unified Step Format)  
**Source**: PO request for gameplay mechanics specification (2026-04-29)

---
