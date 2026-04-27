# Gameplay Judge Requirements

## REQ-JDG-001: Pure Logic Judge Module
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The judge must be a pure logic module with no rendering or audio dependencies, returning judgments given note beat position, input timestamp, and current song position.

**Acceptance Criteria**:
- No direct SDL, OpenGL, or audio API calls in judge code
- Judgment calculation is deterministic (same inputs -> same outputs)
- Judge can run in unit tests without graphics context
- Judge operates on timing data only

**Dependencies**: REQ-CHT-003, REQ-INP-002, REQ-AUD-002  
**Source**: Roadmap subsystem 5, architecture decisions

---

## REQ-JDG-002: Five-Tier Judgment System
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

The judge must return judgments in five categories: Perfect, Great, Good, Bad, Miss.

**Acceptance Criteria**:
- Each judgment has associated timing window
- Signed timing error returned in milliseconds
- Early vs late indicated in timing error sign
- Judgment accuracy within 1ms

**Dependencies**: REQ-JDG-001  
**Source**: Roadmap subsystem 5

---

## REQ-JDG-003: Data-Driven Timing Windows
**Status**: [PLANNED Phase 4]  
**Priority**: Must Have

Timing windows must be defined in `JudgeProfile` JSON files, not hardcoded, allowing different PIU versions to have different timing.

**Acceptance Criteria**:
- Timing windows loaded from JSON file
- Each judgment type has configurable window (ms)
- Windows validated at load time (Great <= Good, etc)
- Invalid profiles logged and rejected

**Dependencies**: REQ-JDG-002  
**Source**: Roadmap subsystem 5

---

## REQ-JDG-004: Judge Profile Per PIU Version
**Status**: [PLANNED Phase 4]  
**Priority**: Must Have

The engine must provide judge profiles for each Pump It Up version: Exceed, Zero, NX, NX2, Fiesta, XX, Phoenix.

**Acceptance Criteria**:
- Separate JSON file per version
- Timing windows historically accurate per version
- Profile includes life gauge drain rates per version
- Profile includes scoring formula per version

**Dependencies**: REQ-JDG-003  
**Source**: Roadmap subsystem 5, Phase 4

---

## REQ-JDG-005: Scoring Formula in Judge Profile
**Status**: [PLANNED Phase 4]  
**Priority**: Must Have

Judge profiles must define scoring formulas including points per judgment, combo bonuses, and grade thresholds.

**Acceptance Criteria**:
- Points per judgment type configurable
- Combo multiplier formula defined
- Grade thresholds (SSS, SS, S, A, B, C, D, F) defined
- Max score calculation correct

**Dependencies**: REQ-JDG-003  
**Source**: Roadmap subsystem 5, Phase 4

---

## REQ-JDG-006: Hold Note Processing
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

The judge must process hold notes correctly: active once head is judged, held while panel held, partial score on early release.

**Acceptance Criteria**:
- Hold becomes active after head judgment
- Hold continues while panel held continuously
- Early release gives partial score based on hold percentage
- Hold tail judgment triggers on release or at tail beat

**Dependencies**: REQ-JDG-001, REQ-CHT-004  
**Source**: Roadmap subsystem 5, Phase 3

---

## REQ-JDG-007: Hold Grace Window
**Status**: [PLANNED Phase 3]  
**Priority**: Should Have

Hold notes must support a grace window where releasing and re-pressing can recover the hold without full miss.

**Acceptance Criteria**:
- Grace window duration configurable per judge profile
- Re-press within grace window continues hold scoring
- Grace window does not apply to initial head judgment
- Grace window behavior matches selected PIU version

**Dependencies**: REQ-JDG-006, REQ-JDG-003  
**Source**: Roadmap subsystem 5, Phase 3

---

## REQ-JDG-008: Automatic Miss Assignment
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

Notes that pass beyond the latest timing window without being hit must be automatically judged as Miss.

**Acceptance Criteria**:
- Miss issued when note is later than Bad window + song position
- Miss judgment event emitted same as manual judgments
- No notes left unjudged at song end
- Missed notes do not block future note processing

**Dependencies**: REQ-JDG-002  
**Source**: Roadmap subsystem 5

---

## REQ-JDG-009: Judgment Event Emission
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

Each tick the judge must scan notes within judgable range, match against input events, and emit `JudgmentEvent` structs.

**Acceptance Criteria**:
- One judgment event per note judgment
- Events include note ID, judgment type, timing error, column
- Events emitted in beat order
- Events immutable after emission

**Dependencies**: REQ-JDG-001, REQ-INP-002  
**Source**: Roadmap subsystem 5

---

## REQ-JDG-010: Co-op Mode Judging
**Status**: [PLANNED Phase 5]  
**Priority**: Should Have

For co-op mode, two judge instances (one per player) must share a single `GameplayState` with shared or separate life gauge.

**Acceptance Criteria**:
- Two independent judge instances
- Each judge receives own InputSnapshot
- GameplayState aggregates judgments from both judges
- Life gauge sharing configurable (shared or separate)

**Dependencies**: REQ-JDG-001, REQ-INP-006  
**Source**: Roadmap subsystem 5, Phase 5

---

## REQ-JDG-011: GameplayState Separation
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

A separate `GameplayState` object must subscribe to judgment events and maintain running combo, score, life gauge, and grade independently of the judge.

**Acceptance Criteria**:
- GameplayState does not influence judge decisions
- Judge has no knowledge of score or combo
- GameplayState can be reset without recreating judge
- Multiple GameplayState instances can observe same judge

**Dependencies**: REQ-JDG-009  
**Source**: Roadmap subsystem 5

---

## REQ-JDG-012: Combo Tracking
**Status**: [PLANNED Phase 1]  
**Priority**: Must Have

GameplayState must track current combo, max combo, and break combo on Bad/Miss judgments.

**Acceptance Criteria**:
- Combo increments on Perfect, Great, Good
- Combo resets to zero on Bad, Miss
- Max combo tracked throughout song
- Combo value accessible for display

**Dependencies**: REQ-JDG-011  
**Source**: Roadmap subsystem 5 (implied)

---

## REQ-JDG-013: Life Gauge with HP Drain
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

GameplayState must maintain a life gauge with HP drain on misses and recovery on good judgments, with fail detection.

**Acceptance Criteria**:
- Life starts at configured initial value (50% typical)
- Perfect/Great recover HP, Bad/Miss drain HP
- Drain/recovery amounts defined in judge profile
- Fail triggered when life reaches 0
- Life gauge value 0-100 accessible for rendering

**Dependencies**: REQ-JDG-011, REQ-JDG-003  
**Source**: Roadmap subsystem 5, Phase 3

---

## REQ-JDG-014: Grade Calculation
**Status**: [PLANNED Phase 3]  
**Priority**: Must Have

GameplayState must calculate current grade (SSS, SS, S, A, B, C, D, F) based on accumulated score and grade thresholds from judge profile.

**Acceptance Criteria**:
- Grade updates in real-time as score changes
- Grade thresholds from judge profile applied correctly
- Final grade displayed on result screen
- Grade considers both percentage and combo thresholds where applicable

**Dependencies**: REQ-JDG-011, REQ-JDG-005  
**Source**: Roadmap subsystem 5, Phase 3
