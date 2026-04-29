# Gameplay Judge User Stories

This document contains user stories for the Gameplay Judge subsystem (Subsystem 5). Stories are organized by implementation phase and cross-reference dependencies from other subsystems.

**Status Legend:**
- **DONE**: Implemented and tested
- **PLANNED**: Ready for implementation in the specified phase
- **FUTURE**: Post-Phase 9, or dependent on external decisions

**Story ID Convention:** US-JDG-NNN (Gameplay Judge)

---

## Phase 1: Core Judge Logic (Taps Only)

### Story ID: US-JDG-019 - Default Hardcoded Timing Profile

**Story Card:**
> **As a** Developer
> **I want** hardcoded timing windows for Phase 1
> **So that** the judge can classify hits without needing JSON profile loading

#### Description
Provide a default set of hardcoded timing windows (Perfect ±16ms, Great ±33ms, Good ±66ms, Bad ±100ms, Miss >100ms) for Phase 1 gameplay. JSON profile loading will be implemented in Phase 4.

#### Acceptance Criteria

*   **Scenario 1: Default windows are available**
    *   **Given** the judge is initialized without loading a profile
    *   **When** notes are judged
    *   **Then** Perfect window is ±16ms, Great is ±33ms, Good is ±66ms, Bad is ±100ms

*   **Scenario 2: Judgments use hardcoded windows**
    *   **Given** a note at time 1000ms
    *   **When** input arrives at 1015ms (15ms late)
    *   **Then** the judgment is "Perfect" (within ±16ms window)

*   **Scenario 3: Hardcoded values match Exceed era**
    *   **Given** the hardcoded timing windows
    *   **When** compared to documented Exceed timing
    *   **Then** the values match historically accurate Exceed timing

#### Technical Notes & Constraints
*   **Estimation Pointer**: 2 story points
*   **Dependencies**: US-JDG-001
*   **Phase**: 1

---

### Story ID: US-JDG-001 - Deterministic Pure Logic Judge

**Story Card:**
> **As a** Developer
> **I want** a judge module with no rendering or audio dependencies
> **So that** the testing infrastructure remains fast and can run judgment logic in isolation

#### Description
The judge must be a pure logic component that accepts timing data and input events, returns judgment results, and can run in automated test environments. No direct calls to SDL, OpenGL, or audio APIs are permitted within judge code.

#### Acceptance Criteria

*   **Scenario 1: Judge runs in unit test environment**
    *   **Given** a GoogleTest suite with no SDL_Init call
    *   **When** I instantiate a Judge object with timing data
    *   **Then** the test compiles, links, and runs without graphics context errors

*   **Scenario 2: Same inputs produce same outputs**
    *   **Given** a note at beat 4.0, input timestamp 1000ms, song position 1005ms
    *   **When** I call judge.evaluate() twice with identical parameters
    *   **Then** both calls return judgment type "Great" and timing error +5ms

*   **Scenario 3: No audio or rendering state in judge**
    *   **Given** the Judge class source code
    *   **When** I grep for "SDL_", "glBegin", "Mix_", or "AudioStream"
    *   **Then** zero matches are found in judge.h and judge.cpp

#### Technical Notes & Constraints
*   **Estimation Pointer**: 4 points
*   **Dependencies**: US-CHT-001 (TimingData beat-to-time conversion), US-INP-001 (InputSnapshot structure), US-AUD-001 (position query interface)

---

### Story ID: US-JDG-002 - Five-Tier Judgment Classification

**Story Card:**
> **As a** Player
> **I want** five distinct judgment types based on timing accuracy
> **So that** I can see how precisely I hit each note

#### Description
The judge must classify each note hit into one of five categories: Perfect, Great, Good, Bad, or Miss. Each judgment has an associated timing window measured in milliseconds from the note's beat position.

#### Acceptance Criteria

*   **Scenario 1: Perfect judgment for exact hit**
    *   **Given** a note at beat 4.0 (song time 1000ms) and Perfect window of ±16ms
    *   **When** input arrives at 1001ms (1ms early)
    *   **Then** judgment type is "Perfect" and timing error is -1ms

*   **Scenario 2: Great judgment outside Perfect window**
    *   **Given** Perfect window ±16ms, Great window ±40ms
    *   **When** input arrives at 1025ms (25ms late)
    *   **Then** judgment type is "Great" and timing error is +25ms

*   **Scenario 3: Miss judgment beyond all windows**
    *   **Given** Bad window ±135ms (outermost window)
    *   **When** input arrives at 1150ms (150ms late)
    *   **Then** judgment type is "Miss" and timing error is +150ms

*   **Scenario 4: Early vs late indicated by sign**
    *   **Given** a note at 1000ms
    *   **When** input arrives at 995ms
    *   **Then** timing error is -5ms (negative indicates early)

*   **Scenario 5: Boundary condition at exact window edge**
    *   **Given** Perfect window ±16ms, note at 1000ms
    *   **When** input arrives at exactly 1016ms (16ms late)
    *   **Then** judgment type is "Perfect" (boundary is inclusive)

#### Technical Notes & Constraints
*   **Estimation Pointer**: 4 points
*   **Dependencies**: US-JDG-001, US-CHT-001 (time_at_beat function)

---

### Story ID: US-JDG-003 - Automatic Miss Assignment

**Story Card:**
> **As a** Player
> **I want** notes automatically judged as Miss when I fail to hit them
> **So that** every note receives a judgment even if I ignore it

#### Description
Notes that pass beyond the latest timing window without receiving input must be automatically judged as Miss. This prevents unjudged notes from accumulating and ensures the judge state remains consistent.

#### Acceptance Criteria

*   **Scenario 1: Miss issued when note passes Bad window**
    *   **Given** a note at beat 4.0 (1000ms) and Bad window ±135ms
    *   **When** song position advances to 1150ms with no input for that column
    *   **Then** judgment type "Miss" is emitted with timing error +135ms

*   **Scenario 2: Judgment event emitted for auto-miss**
    *   **Given** a note that passed without input
    *   **When** the auto-miss is triggered
    *   **Then** a JudgmentEvent struct is emitted containing note ID, "Miss", error, and column

*   **Scenario 3: No notes left unjudged at song end**
    *   **Given** a chart with 100 notes
    *   **When** the song completes with only 90 manual inputs
    *   **Then** exactly 100 JudgmentEvent records exist (90 manual + 10 auto-miss)

*   **Scenario 4: Missed notes do not block future processing**
    *   **Given** note A at beat 4.0 (missed) and note B at beat 5.0
    *   **When** input arrives at beat 5.0
    *   **Then** note B is judged normally without requiring note A to be resolved first

#### Technical Notes & Constraints
*   **Estimation Pointer**: 2 points
*   **Dependencies**: US-JDG-002

---

### Story ID: US-JDG-004 - Judgment Event Emission

**Story Card:**
> **As a** Developer
> **I want** immutable judgment events emitted in beat order
> **So that** I can decouple score tracking from judgment logic

#### Description
Each tick, the judge scans notes within the judgable range, matches them against input events from the InputSnapshot, and emits JudgmentEvent structs. Events must be immutable after emission and ordered by note beat position.

#### Acceptance Criteria

*   **Scenario 1: One event per note judgment**
    *   **Given** three notes at beats 1.0, 2.0, 3.0
    *   **When** three inputs arrive within Perfect windows
    *   **Then** exactly three JudgmentEvent structs are emitted

*   **Scenario 2: Events contain complete data**
    *   **Given** a note with ID 42 in column 2 judged as "Great" at -10ms
    *   **When** the judgment event is emitted
    *   **Then** event.note_id == 42, event.judgment == "Great", event.timing_error == -10, event.column == 2

*   **Scenario 3: Events emitted in beat order**
    *   **Given** notes at beats 5.0, 3.0, 4.0 (unordered in chart file)
    *   **When** all three are judged in a single tick
    *   **Then** events are emitted in order: 3.0, 4.0, 5.0

*   **Scenario 4: Events are immutable**
    *   **Given** a JudgmentEvent struct emitted by the judge
    *   **When** a caller attempts to modify event.judgment
    *   **Then** compilation fails (all fields are const or private with no setters)

#### Technical Notes & Constraints
*   **Estimation Pointer**: 2 points
*   **Dependencies**: US-JDG-002, US-INP-001 (InputSnapshot structure)

---

### Story ID: US-JDG-011 - Judge Frame Independence

**Story Card:**
> **As a** Developer
> **I want** judgment timing independent of rendering frame rate
> **So that** players with 60Hz, 120Hz, or 144Hz displays receive identical judgment results

#### Description
The judge must operate on a fixed 60 Hz logic tick synchronized to audio position, not rendering frame rate. Rendering interpolates between logic steps for smooth visuals without affecting judgment accuracy.

**Note:** Formerly US-ENG-051, migrated from core engine subsystem.

#### Acceptance Criteria

*   **Scenario 1: Identical judgments at different refresh rates**
    *   **Given** the same note, input timestamp, and audio position
    *   **When** the game runs at 60Hz vs 144Hz display refresh
    *   **Then** both produce identical judgment type and timing error (within 1ms)

*   **Scenario 2: Fixed 60 Hz logic step**
    *   **Given** the game loop with time accumulator
    *   **When** a single rendering frame accumulates 30ms delta time
    *   **Then** the engine calls judge.update() exactly twice (2 * 16.67ms ticks)

*   **Scenario 3: Judge uses audio position, not wall clock**
    *   **Given** audio position at 1000ms and wall clock at 1050ms (audio lagged)
    *   **When** judging a note at beat 4.0 (1000ms)
    *   **Then** timing error is calculated from audio position 1000ms, not wall clock 1050ms

#### Technical Notes & Constraints
*   **Estimation Pointer**: 4 points
*   **Dependencies**: US-ENG-001 (fixed-step game loop with Clock utility), US-AUD-002 (get_position_ms function)

---

### Story ID: US-JDG-012 - No RNG in Judge

**Story Card:**
> **As a** Player
> **I want** judgment results determined purely by input timing
> **So that** replaying the same inputs produces the same score

#### Description
The judge must be deterministic with no random number generation. Same note timing, input timestamps, and audio position must always produce the same judgment. This is critical for replay verification and anti-cheat systems.

**Note:** Formerly US-ENG-052, migrated from core engine subsystem.

#### Acceptance Criteria

*   **Scenario 1: No RNG calls in judge code**
    *   **Given** the Judge class source code
    *   **When** I grep for "rand", "random", "RNG", or "std::mt19937"
    *   **Then** zero matches are found in judge.h and judge.cpp

*   **Scenario 2: Replay produces identical score**
    *   **Given** a recorded input sequence with timestamps
    *   **When** I replay the sequence twice with identical audio sync
    *   **Then** both runs produce identical judgment lists and final score

*   **Scenario 3: No floating-point non-determinism**
    *   **Given** a note at beat 4.0 and input at 1005ms
    *   **When** I evaluate the judgment on x86, ARM, and x64 architectures
    *   **Then** all three produce identical timing error values (within IEEE 754 rounding)

#### Technical Notes & Constraints
*   **Estimation Pointer**: 2 points
*   **Dependencies**: US-JDG-001

---

### Story ID: US-JDG-005 - GameplayState Separation

**Story Card:**
> **As a** Developer
> **I want** score and combo tracked separately from judgment logic
> **So that** the judge remains a pure timing oracle with no display concerns

#### Description
A separate GameplayState object subscribes to judgment events and maintains running combo, score, life gauge, and grade. The judge emits events but never reads score or combo state.

#### Acceptance Criteria

*   **Scenario 1: Judge has no score knowledge**
    *   **Given** the Judge class interface
    *   **When** I inspect public methods and member variables
    *   **Then** no method returns score, combo, or life gauge values

*   **Scenario 2: GameplayState can be reset independently**
    *   **Given** a Judge instance with 50 notes already processed
    *   **When** I destroy the GameplayState and create a new one
    *   **Then** the new GameplayState starts with combo 0 and score 0 without affecting the Judge

*   **Scenario 3: Multiple observers on same judge**
    *   **Given** one Judge instance
    *   **When** I attach two GameplayState instances to the same judgment event stream
    *   **Then** both accumulate identical combo and score values independently

*   **Scenario 4: GameplayState does not influence judgments**
    *   **Given** GameplayState with combo 50 and life gauge at 80
    *   **When** the next note is judged
    *   **Then** judgment type is determined solely by input timing, unaffected by combo or life state

#### Technical Notes & Constraints
*   **Estimation Pointer**: 4 points
*   **Dependencies**: US-JDG-004 (judgment event emission)

---

### Story ID: US-JDG-006 - Combo Tracking

**Story Card:**
> **As a** Player
> **I want** my combo count displayed during gameplay
> **So that** I can see how many consecutive notes I have hit without missing

#### Description
GameplayState tracks current combo (incremented on Perfect/Great/Good, reset on Bad/Miss), max combo for the session, and exposes these values for display rendering.

#### Acceptance Criteria

*   **Scenario 1: Combo increments on good judgments**
    *   **Given** current combo is 5
    *   **When** a "Perfect" judgment event is received
    *   **Then** current combo becomes 6

*   **Scenario 2: Combo resets on Bad judgment**
    *   **Given** current combo is 20
    *   **When** a "Bad" judgment event is received
    *   **Then** current combo becomes 0 and max combo remains 20

*   **Scenario 3: Max combo tracked throughout song**
    *   **Given** combo values: 10 → 0 (miss) → 15 → 0 (miss) → 8
    *   **When** the song completes
    *   **Then** max combo is 15

*   **Scenario 4: Combo value accessible for display**
    *   **Given** GameplayState with current combo 42
    *   **When** GameplayScene calls get_current_combo()
    *   **Then** the method returns 42 without side effects

#### Technical Notes & Constraints
*   **Estimation Pointer**: 2 points
*   **Dependencies**: US-JDG-005

---

## Phase 3: Holds, Life Gauge, Scoring

### Story ID: US-JDG-007 - Hold Note Head Judgment

**Story Card:**
> **As a** Player
> **I want** hold notes activated when I hit their head
> **So that** I can begin holding the panel to score the hold body

#### Description
Hold notes have a head and tail. The head is judged like a tap note. Once the head is judged (any judgment except Miss), the hold becomes active and scoring transitions to tracking panel hold state.

#### Acceptance Criteria

*   **Scenario 1: Hold activates after head judgment**
    *   **Given** a hold note with head at beat 4.0
    *   **When** input arrives within Great window
    *   **Then** judgment event "Great" is emitted and hold state transitions to "Active"

*   **Scenario 2: Hold does not activate on missed head**
    *   **Given** a hold note with head at beat 4.0
    *   **When** no input arrives and head auto-misses at beat 4.2
    *   **Then** judgment event "Miss" is emitted and hold state remains "Inactive"

*   **Scenario 3: Head timing error included in event**
    *   **Given** a hold note head at 1000ms
    *   **When** input arrives at 1012ms
    *   **Then** judgment event includes timing_error +12ms

*   **Scenario 4: Hold body not scored until head judged**
    *   **Given** a hold note with head at beat 4.0 and tail at beat 6.0
    *   **When** player holds panel continuously from beat 3.5 through 6.0
    *   **Then** hold body score begins accumulating only after head is judged at beat 4.0

#### Technical Notes & Constraints
*   **Estimation Pointer**: 4 points
*   **Dependencies**: US-JDG-002, US-CHT-004 (hold note structure in chart)

---

### Story ID: US-JDG-008 - Hold Body Continuous Scoring

**Story Card:**
> **As a** Player
> **I want** partial score for holding a note as long as I keep the panel pressed
> **So that** releasing early gives proportional credit rather than complete failure

#### Description
While a hold is active, the judge checks each tick whether the panel is held. For each tick held, partial score accumulates. The percentage held determines the final hold score.

#### Acceptance Criteria

*   **Scenario 1: Full hold score for continuous press**
    *   **Given** a hold from beat 4.0 to 6.0 (120 ticks at 60 Hz)
    *   **When** player holds panel for all 120 ticks
    *   **Then** hold score is 100 percent

*   **Scenario 2: Partial score on early release**
    *   **Given** a hold requiring 120 ticks
    *   **When** player releases at tick 60 (50 percent held)
    *   **Then** hold score is 50 percent

*   **Scenario 3: Hold continues while panel held**
    *   **Given** an active hold and player holding the panel
    *   **When** judge.update() is called each tick
    *   **Then** hold_ticks_scored increments by 1 each tick while InputSnapshot shows panel down

*   **Scenario 4: Hold ends at tail or release**
    *   **Given** a hold tail at beat 6.0
    *   **When** song position reaches beat 6.0
    *   **Then** hold state transitions to "Complete" regardless of panel state

#### Technical Notes & Constraints
*   **Estimation Pointer**: 4 points
*   **Dependencies**: US-JDG-007, US-INP-001 (InputSnapshot panel state)

---

### Story ID: US-JDG-009 - Hold Grace Window Recovery

**Story Card:**
> **As a** Player
> **I want** brief forgiveness when I accidentally lift during a hold
> **So that** I can recover by immediately re-pressing without full miss penalty

#### Description
Hold notes support a grace window (e.g., 150ms) where releasing and re-pressing allows the hold to continue. If the player re-presses within the grace window, scoring resumes. Grace window duration is defined in the judge profile.

#### Acceptance Criteria

*   **Scenario 1: Re-press within grace window continues hold**
    *   **Given** a hold in progress and grace window 150ms
    *   **When** player releases for 100ms then re-presses
    *   **Then** hold scoring resumes and ticks during release are not scored

*   **Scenario 2: Release beyond grace window breaks hold**
    *   **Given** grace window 150ms
    *   **When** player releases for 200ms
    *   **Then** hold transitions to "Broken" and no further ticks are scored

*   **Scenario 3: Grace window does not apply to head**
    *   **Given** a hold head at beat 4.0
    *   **When** player presses at beat 3.9, releases at 3.95, re-presses at 4.0
    *   **Then** head judgment uses timing from first press (3.9), grace window does not affect head timing

*   **Scenario 4: Grace window loaded from profile**
    *   **Given** a judge profile JSON with "hold_grace_ms": 200
    *   **When** the judge loads the profile
    *   **Then** hold grace window is 200ms

#### Technical Notes & Constraints
*   **Estimation Pointer**: 4 points
*   **Dependencies**: US-JDG-008, US-JDG-013 (judge profile loading)

---

### Story ID: US-JDG-010 - Life Gauge with HP Drain

**Story Card:**
> **As a** Player
> **I want** a life gauge that decreases on misses and increases on good hits
> **So that** I fail the song if I perform too poorly

#### Description
GameplayState maintains a life gauge starting at 50 percent. Perfect and Great judgments recover HP, Bad and Miss judgments drain HP. When life reaches 0, the song fails. Drain and recovery amounts are defined in the judge profile.

#### Acceptance Criteria

*   **Scenario 1: Life starts at initial value**
    *   **Given** a new GameplayState with judge profile specifying initial_life 50
    *   **When** gameplay begins
    *   **Then** get_life() returns 50

*   **Scenario 2: Perfect judgment recovers HP**
    *   **Given** life at 50 and profile specifying perfect_recovery 0.5
    *   **When** a "Perfect" judgment event is received
    *   **Then** life increases to 50.5

*   **Scenario 3: Miss judgment drains HP**
    *   **Given** life at 50 and profile specifying miss_drain 5.0
    *   **When** a "Miss" judgment event is received
    *   **Then** life decreases to 45

*   **Scenario 4: Fail triggered at zero life**
    *   **Given** life at 2.0 and miss_drain 5.0
    *   **When** a "Miss" judgment occurs
    *   **Then** life becomes 0 and GameplayState.is_failed() returns true

*   **Scenario 5: Life clamped to 0-100 range**
    *   **Given** life at 95 and perfect_recovery 10
    *   **When** a "Perfect" judgment occurs
    *   **Then** life becomes 100 (not 105)

#### Technical Notes & Constraints
*   **Estimation Pointer**: 2 points
*   **Dependencies**: US-JDG-005, US-JDG-013 (judge profile structure)

---

## Phase 4: Multi-Version Judge Profiles

### Story ID: US-JDG-013 - Data-Driven Timing Windows

**Story Card:**
> **As a** Developer
> **I want** timing windows loaded from JSON configuration
> **So that** I can add support for new PIU versions without modifying judge code

#### Description
Timing windows for Perfect, Great, Good, and Bad judgments are defined in JudgeProfile JSON files, not hardcoded constants. Each window is a positive millisecond value representing the symmetric range around the note.

#### Acceptance Criteria

*   **Scenario 1: Timing windows loaded from JSON**
    *   **Given** a file exceed.json with "perfect_window_ms": 16, "great_window_ms": 40
    *   **When** the judge loads the profile
    *   **Then** a note hit within 16ms is judged Perfect, hit at 20ms is judged Great

*   **Scenario 2: Each judgment has configurable window**
    *   **Given** a profile JSON with all four windows specified
    *   **When** the JSON is parsed
    *   **Then** JudgeProfile struct contains perfect_window, great_window, good_window, bad_window

*   **Scenario 3: Windows validated at load time**
    *   **Given** a profile with perfect_window 50 and great_window 40 (invalid: perfect > great)
    *   **When** the profile is loaded
    *   **Then** loading fails with error logged "Perfect window must be <= Great window"

*   **Scenario 4: Invalid profiles logged and rejected**
    *   **Given** a profile JSON with bad_window -10 (negative value)
    *   **When** the profile is loaded
    *   **Then** loading fails with error logged "Timing windows must be positive"

#### Technical Notes & Constraints
*   **Estimation Pointer**: 4 points
*   **Dependencies**: US-JDG-002

---

### Story ID: US-JDG-014 - Scoring Formula in Judge Profile

**Story Card:**
> **As a** Developer
> **I want** scoring formulas defined in judge profile JSON
> **So that** different PIU versions can have different point values and combo multipliers

#### Description
Judge profiles define points per judgment type and grade thresholds. The combo multiplier mechanism is implemented as a hardcoded function selected by a "combo_multiplier_type" field in the profile (e.g., "linear", "exponential", "exceed_era"). GameplayState reads these values to calculate score correctly for each version.

#### Acceptance Criteria

*   **Scenario 1: Points per judgment configurable**
    *   **Given** a profile with "perfect_points": 1000, "great_points": 500
    *   **When** GameplayState receives a "Perfect" judgment
    *   **Then** score increases by 1000 points

*   **Scenario 2: Combo multiplier type selects algorithm**
    *   **Given** a profile with "combo_multiplier_type": "linear"
    *   **When** a judgment occurs at combo 50
    *   **Then** the engine applies the hardcoded linear multiplier function (e.g., 1.0 + combo * 0.001)

*   **Scenario 3: Grade thresholds defined in profile**
    *   **Given** a profile with grade thresholds SSS: 99.5, SS: 99, S: 95, A: 90, B: 80, C: 70, D: 60
    *   **When** score percentage is 92
    *   **Then** grade is "A"

*   **Scenario 4: Max score calculation correct**
    *   **Given** a chart with 100 tap notes and profile with perfect_points 1000
    *   **When** GameplayState calculates max_possible_score
    *   **Then** the value is 100000 (100 * 1000)

*   **Scenario 5: Invalid combo multiplier type rejected**
    *   **Given** a profile with "combo_multiplier_type": "unknown_function"
    *   **When** the profile is loaded
    *   **Then** loading fails with error logged "Unknown combo multiplier type: unknown_function"

#### Technical Notes & Constraints
*   **Estimation Pointer**: 4 points
*   **Dependencies**: US-JDG-013, US-JDG-005

---

### Story ID: US-JDG-015 - Grade Calculation

**Story Card:**
> **As a** Player
> **I want** my current grade displayed during gameplay
> **So that** I can see whether I am on track for my target grade

#### Description
GameplayState calculates current grade (SSS, SS, S, A, B, C, D, F) based on accumulated score and grade thresholds from the judge profile. Grade updates in real-time as score changes.

#### Acceptance Criteria

*   **Scenario 1: Grade updates in real-time**
    *   **Given** score at 850000 out of 1000000 max (85 percent) and grade threshold S at 90 percent
    *   **When** get_current_grade() is called
    *   **Then** the method returns "A"

*   **Scenario 2: Grade thresholds from profile applied**
    *   **Given** a judge profile with SSS: 99.5 percent, SS: 99 percent, S: 95 percent
    *   **When** score reaches 99.2 percent
    *   **Then** current grade is "SS"

*   **Scenario 3: Final grade displayed on result screen**
    *   **Given** a completed song with final score 920000 out of 1000000
    *   **When** ResultScene queries GameplayState.get_final_grade()
    *   **Then** the method returns "S"

*   **Scenario 4: Grade considers percentage thresholds**
    *   **Given** max score 1000000 and SSS threshold 99.5 percent
    *   **When** score is 995000
    *   **Then** grade is "SSS"

#### Technical Notes & Constraints
*   **Estimation Pointer**: 2 points
*   **Dependencies**: US-JDG-005, US-JDG-014 (scoring formula)

---

### Story ID: US-JDG-016 - Judge Profile Per PIU Version

**Story Card:**
> **As a** Player
> **I want** judge profiles for each Pump It Up version
> **So that** I can play songs with the timing and scoring rules from my preferred era

#### Description
The engine ships with judge profiles for Exceed, Zero, NX, NX2, Fiesta, XX, and Phoenix. Each profile is a separate JSON file with historically accurate timing windows, life gauge rules, and scoring formulas.

#### Acceptance Criteria

*   **Scenario 1: Separate JSON file per version**
    *   **Given** the judge_profiles/ directory
    *   **When** I list files
    *   **Then** I see exceed.json, zero.json, nx.json, nx2.json, fiesta.json, xx.json, phoenix.json

*   **Scenario 2: Timing windows historically accurate**
    *   **Given** the exceed.json profile
    *   **When** I load the file
    *   **Then** perfect_window is 16ms and great_window is 40ms (matching Exceed era)

*   **Scenario 3: Profile includes life gauge drain rates**
    *   **Given** the phoenix.json profile
    *   **When** I parse the JSON
    *   **Then** fields perfect_recovery, great_recovery, miss_drain, bad_drain are present

*   **Scenario 4: Profile includes scoring formula**
    *   **Given** the fiesta.json profile
    *   **When** I parse the JSON
    *   **Then** fields perfect_points, great_points, combo_multiplier_type, grade_thresholds are present

#### Technical Notes & Constraints
*   **Estimation Pointer**: 4 points
*   **Dependencies**: US-JDG-013, US-JDG-014
*   **Research Note**: Requires community research to confirm historically accurate values for each version

---

## Phase 5: Co-op and Advanced Features

### Story ID: US-JDG-017 - Co-op Mode Dual Judge Instances

**Story Card:**
> **As a** Player
> **I want** to play co-op mode with a second player
> **So that** we can share a song and both contribute to the score

#### Description
For co-op mode, two independent Judge instances process notes from separate columns (P1: columns 0-4, P2: columns 5-9). Each judge receives its own InputSnapshot. A single shared GameplayState aggregates judgments from both judges.

#### Acceptance Criteria

*   **Scenario 1: Two independent judge instances**
    *   **Given** co-op mode enabled
    *   **When** GameplayScene initializes
    *   **Then** two Judge objects are created, each bound to 5 columns

*   **Scenario 2: Each judge receives own InputSnapshot**
    *   **Given** P1 presses down-left (column 0) and P2 presses up-right (column 8)
    *   **When** input system produces snapshots
    *   **Then** judge_p1 receives column 0 press, judge_p2 receives column 8 press

*   **Scenario 3: GameplayState aggregates judgments from both**
    *   **Given** judge_p1 emits "Perfect" and judge_p2 emits "Great"
    *   **When** GameplayState processes both events
    *   **Then** combo increases by 2 and score increases by perfect_points + great_points

*   **Scenario 4: Life gauge sharing configurable**
    *   **Given** co-op mode with "life_sharing": "separate" in profile
    *   **When** P1 misses a note
    *   **Then** only P1's life gauge decreases, P2's life remains unchanged

#### Technical Notes & Constraints
*   **Estimation Pointer**: 6 points
*   **Dependencies**: US-JDG-001, US-JDG-005, US-INP-006 (multi-player input snapshots)

---

### Story ID: US-JDG-018 - Shared vs Separate Life Gauge

**Story Card:**
> **As a** Player
> **I want** to choose whether co-op mode shares one life gauge or gives each player their own
> **So that** we can select the difficulty level that matches our skill

#### Description
Co-op mode supports two life gauge configurations: "shared" (both players drain/recover the same gauge) and "separate" (each player has their own gauge, both must survive). Configuration is specified in the judge profile.

#### Acceptance Criteria

*   **Scenario 1: Shared life gauge mode**
    *   **Given** profile with "coop_life_mode": "shared"
    *   **When** P1 gets a Miss (drain 5 HP) and P2 gets a Perfect (recover 0.5 HP)
    *   **Then** single life gauge decreases by 4.5 HP total

*   **Scenario 2: Separate life gauge mode**
    *   **Given** profile with "coop_life_mode": "separate"
    *   **When** P1 gets a Miss and P2 gets a Perfect
    *   **Then** P1 life decreases by 5 HP and P2 life increases by 0.5 HP independently

*   **Scenario 3: Both players must survive in separate mode**
    *   **Given** separate life mode with P1 life at 10 and P2 life at 60
    *   **When** P1 life reaches 0
    *   **Then** GameplayState.is_failed() returns true even though P2 life is positive

*   **Scenario 4: Failure requires both in shared mode**
    *   **Given** shared life mode with life at 5
    *   **When** one player gets a Miss draining life to 0
    *   **Then** both players fail together

#### Technical Notes & Constraints
*   **Estimation Pointer**: 2 points
*   **Dependencies**: US-JDG-017, US-JDG-010

---

## Non-Functional Requirements

### Story ID: US-JDG-NFR-001 - Judgment Accuracy

**Story Card:**
> **As a** Player
> **I want** judgment timing accurate within 1 millisecond
> **So that** I can trust the judge for competitive play

#### Description
Timing error values returned by the judge must be accurate within 1ms of the true time delta between input and note position. This requires audio position queries accurate to the sample level.

#### Acceptance Criteria

*   **Scenario 1: Timing error within 1ms tolerance**
    *   **Given** a note at 1000.0ms and input at 1005.2ms
    *   **When** the judge calculates timing error
    *   **Then** the returned value is between 5.0ms and 6.0ms

*   **Scenario 2: Sub-sample audio position accuracy**
    *   **Given** audio at 48kHz sample rate (20.8μs per sample)
    *   **When** get_position_ms() is called
    *   **Then** the value is accurate to within 1 sample (0.021ms)

#### Technical Notes & Constraints
*   **Estimation Pointer**: 2 points (Spike/Research)
*   **Dependencies**: US-AUD-002 (high-precision position query)

---

### Story ID: US-JDG-NFR-002 - Judge Performance Budget

**Story Card:**
> **As a** Developer
> **I want** judge.update() to complete within 100 microseconds per tick
> **So that** judgment does not consume significant frame budget

#### Description
The judge must process input and emit judgment events efficiently. On a mid-range CPU, judge.update() with 10 notes in the judgable window must complete in under 100μs, leaving frame budget for rendering and audio.

#### Acceptance Criteria

*   **Scenario 1: Update completes within 100μs**
    *   **Given** a chart with 10 notes within judgable range
    *   **When** judge.update() is profiled on a 2GHz CPU
    *   **Then** execution time is less than 100 microseconds

*   **Scenario 2: Performance scales linearly with active notes**
    *   **Given** 10 notes taking 80μs and 20 notes taking 160μs
    *   **When** 5 notes are in range
    *   **Then** execution time is approximately 40μs

#### Technical Notes & Constraints
*   **Estimation Pointer**: 1 point (Spike)
*   **Dependencies**: US-JDG-001

---

### Story ID: US-JDG-NFR-003 - Judge Profile Validation

**Story Card:**
> **As a** Developer
> **I want** detailed error messages when judge profiles fail to load
> **So that** I can quickly fix JSON syntax or validation errors

#### Description
When a judge profile JSON file is malformed or contains invalid values, the engine logs a detailed error message with file path, line number, and specific validation failure, then falls back to a default profile.

#### Acceptance Criteria

*   **Scenario 1: JSON parse error with line number**
    *   **Given** a profile JSON with missing comma on line 5
    *   **When** the profile is loaded
    *   **Then** error log includes "exceed.json:5: Expected comma after 'perfect_window'"

*   **Scenario 2: Validation error with field name**
    *   **Given** a profile with "perfect_window": -10
    *   **When** validation runs
    *   **Then** error log includes "exceed.json: 'perfect_window' must be positive"

*   **Scenario 3: Fallback to default profile on error**
    *   **Given** exceed.json fails to load
    *   **When** GameplayScene initializes
    *   **Then** judge loads built-in default profile and logs "Using default timing profile"

#### Technical Notes & Constraints
*   **Estimation Pointer**: 2 points
*   **Dependencies**: US-JDG-013

---

## Cross-Subsystem Dependencies

The following stories from other subsystems are referenced in judge stories:

**Chart System (Subsystem 4):**
- US-CHT-001: TimingData with time_at_beat() and beat_at_time()
- US-CHT-003: Note structure with tap, hold head, hold tail types
- US-CHT-004: Hold note representation in chart data
- US-CHT-NNN: Chart content hash for score identity (Phase 4)

**Input System (Subsystem 2):**
- US-INP-001: InputSnapshot structure with panel state and edge events
- US-INP-002: Input polling at fixed 60 Hz tick
- US-INP-006: Multi-player InputSnapshot generation for co-op mode

**Audio System (Subsystem 3):**
- US-AUD-001: Music playback with play, pause, seek
- US-AUD-002: get_position_ms() high-precision query
- US-AUD-NNN: Global audio offset calibration (Phase 5)

**Engine (Subsystem 1):**
- US-ENG-001: Fixed 60 Hz game loop with time accumulator
- US-ENG-NNN: Clock utility for delta time and elapsed time

---

## Story Summary by Phase

**Phase 1 (9 stories):** US-JDG-001 through US-JDG-006, US-JDG-011, US-JDG-012, plus 1 NFR validation story

**Phase 3 (4 stories):** US-JDG-007 through US-JDG-010 (holds, life)

**Phase 4 (4 stories):** US-JDG-013 through US-JDG-016 (data-driven profiles, scoring, grade)

**Phase 5 (2 stories):** US-JDG-017 through US-JDG-018 (co-op mode)

**NFR (3 stories):** US-JDG-NFR-001 through US-JDG-NFR-003 (accuracy, performance, validation)

**Total:** 22 stories
