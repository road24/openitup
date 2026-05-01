# Story Tracking Matrix

Generated: 2026-04-30

---

## Phase Summary

| Phase | Stories | Total Points |
|-------|---------|--------------|
| DONE (no phase) | 32 | 70+ (8 stories unpointed) |
| DONE (Phase 1) | 48 | 134 |
| DONE (Phase 2) | 17 | 76 |
| DONE (Phase 3) | 42 | 121 |
| DONE (Phase 4) | 17 | 89 |
| Phase 1 | 0 | 0 |
| Phase 2 | 0 | 0 |
| Phase 3 | 0 | 0 |
| Phase 4 | 0 | 0 |
| Phase 5 | 52 | 154 |
| Phase 6 | 9 | 31 |
| Phase 7 | 18 | 43 |
| Phase 8 | 15 | 54 |
| Phase 9 | 17 | 67 |
| FUTURE (unphased) | 2 | 5 |
| NFR | 30 | 5 (most validated via testing) |
| **TOTAL** | **298** | **841+** |

---

## DONE Stories

| Story ID | Title | Status | Phase | Points | Dependencies |
|----------|-------|--------|-------|--------|--------------|
| US-ENG-001 | Time accumulator for fixed logic step | DONE | 1 | 5 | US-ENG-021 |
| US-ENG-003 | Uncapped/variable refresh rate render | DONE | 1 | 2 | US-ENG-001, US-ENG-011 |
| US-ENG-004 | Render state interpolation | DONE | 1 | 3 | US-ENG-001, US-ENG-003 |
| US-ENG-011 | Engine class as subsystem owner | DONE | 1 | 3 | None |
| US-ENG-012 | Integrate renderer into engine | DONE | 1 | 2 | US-ENG-011 |
| US-ENG-021 | Clock utility wrapping SDL perf ctr | DONE | 1 | 3 | None |
| US-ENG-031 | Verify Linux cross-distro compat | DONE | N/A | 5 | None |
| US-ENG-032 | Verify Windows platform compat | DONE | N/A | 5 | None |
| US-ENG-041 | CMake FetchContent for all deps | DONE | N/A | 5 | None |
| US-ENG-042 | Optimize incremental build perf | DONE | N/A | 2 | US-ENG-041 |
| US-ENG-061 | Log asset loading errors w/ context | DONE | N/A | 2 | None |
| US-ENG-062 | Graceful degrade missing non-crit | DONE | N/A | 3 | None |
| US-ENG-063a | Catch/log engine loop exceptions | DONE | 1 | 2 | US-ENG-011 |
| US-ENG-063b | Graceful startup failure | DONE | 1 | 1 | None |
| US-INP-001 | Define PadInput enum for all controls | DONE | 1 | 1 | None |
| US-INP-002 | Create InputSnapshot structure | DONE | 1 | 2 | US-INP-001 |
| US-INP-003 | Define InputDriver interface | DONE | 1 | 2 | US-INP-002 |
| US-INP-011 | Capture input snapshot once per tick | DONE | 1 | 3 | US-INP-003, US-ENG-001 |
| US-INP-012 | Detect press/release edge events | DONE | 1 | 3 | US-INP-002, US-INP-011 |
| US-INP-013 | Verify no input events lost | DONE | 1 | 3 | US-INP-012 |
| US-INP-021 | KeyboardDriver w/ configurable keymap | DONE | 1 | 5 | US-INP-003, US-INP-012 |
| US-INP-022 | Default QWEASDZXC keymap | DONE | 1 | 1 | US-INP-021 |
| US-INP-024 | Support 10+ key rollover | DONE | 1 | 2 | US-INP-021 |
| US-REN-001 | SDL3 renderer with logical resolution | DONE | N/A | 1 | None |
| US-REN-002 | Frame render loop | DONE | N/A | 1 | US-REN-001 |
| US-REN-003 | Sprite loading from JSON (SPRJ) | DONE | N/A | 2 | US-REN-001, US-REN-005 |
| US-REN-004 | Legacy SPR format loading | DONE | N/A | 3 | US-REN-003 |
| US-REN-005 | Texture cache w/ case-insensitive | DONE | N/A | 3 | US-REN-001 |
| US-REN-006 | Legacy SP2 format loading | DONE | N/A | 2 | US-REN-003, US-REN-004 |
| US-REN-007 | Sprite TILE rendering mode | DONE | N/A | 2 | US-REN-003 |
| US-REN-008 | Sprite ANI rendering mode | DONE | N/A | 2 | US-REN-003 |
| US-REN-009 | Sprite PATTERN rendering mode | DONE | N/A | 3 | US-REN-003 |
| US-REN-010 | BGA JSON format loading (BGAJ) | DONE | N/A | 3 | US-REN-003 |
| US-REN-011 | BGA binary format loading | DONE | N/A | 3 | US-REN-010 |
| US-REN-012 | Keyframe interpolated properties | DONE | N/A | 5 | US-REN-010 |
| US-REN-013 | Keyframe non-interpolated properties | DONE | N/A | 3 | US-REN-012 |
| US-REN-014 | Layer visibility window | DONE | N/A | 2 | US-REN-010 |
| US-REN-015 | BGA layer compositing | DONE | N/A | 2 | US-REN-010, US-REN-007 |
| US-REN-016 | BGA blend modes | DONE | N/A | 3 | US-REN-015 |
| US-REN-017 | BGA player tool | DONE | N/A | 3 | US-REN-010, US-REN-015 |
| US-REN-018 | Visual regression test suite | DONE | N/A | 5 | US-REN-012, US-REN-013, US-REN-014, US-REN-017 |
| US-AST-001 | Texture cache for reuse | DONE | N/A | ? | None |
| US-AST-002 | Case-insensitive texture file lookup | DONE | N/A | ? | US-AST-001 |
| US-AST-003 | Format probing for texture ext | DONE | N/A | ? | US-AST-002 |
| US-AST-004 | Texture cache memory release | DONE | N/A | ? | US-AST-001 |
| US-AST-005 | Relative path resolution from data | DONE | N/A | ? | US-AST-001 |
| US-AST-006 | SPR to SPRJ converter | DONE | N/A | ? | US-AST-001, US-AST-002, US-AST-003 |
| US-AST-007 | SP2 to SPRJ converter | DONE | N/A | ? | None |
| US-AST-008 | BGA binary to JSON converter | DONE | N/A | ? | None |
| US-REN-021 | Sprite-based note skins | DONE | 2 | 5 | US-REN-003, US-REN-020 |
| US-REN-022 | Receptor rendering | DONE | 2 | 2 | US-REN-021 |
| US-SCN-001 | Scene stack core infrastructure | DONE | 2 | 5 | US-ENG-001 |
| US-SCN-002 | Scene lifecycle interface | DONE | 2 | 3 | US-SCN-001, US-INP-001 |
| US-AST-021 | System asset directory structure | DONE | 2 | 2 | US-AST-001, US-AST-009 |
| US-AST-018 | Texture cache LRU eviction | DONE | 2 | 5 | US-AST-001 |
| US-AST-022 | Font loading for text rendering | DONE | 2 | 5 | US-AST-021 |
| US-REN-023 | Judgment display | DONE | 2 | 3 | US-REN-021 |
| US-REN-024 | Combo display | DONE | 2 | 3 | US-REN-021 |
| US-REN-027 | BGA background during gameplay | DONE | 2 | 3 | US-REN-010 |
| US-AUD-001 | Load OGG Vorbis music files | DONE | 1 | 3 | None |
| US-AUD-002 | Load MP3 music files | DONE | 1 | 3 | US-AUD-001 |
| US-AUD-003 | Music playback controls | DONE | 1 | 2 | US-AUD-001, US-AUD-002 |
| US-AUD-004 | Seek to millisecond position | DONE | 1 | 3 | US-AUD-003 |
| US-AUD-011 | Report HW sample-accurate position | DONE | 1 | 5 | US-AUD-003 |
| US-AUD-021 | Provide audio position to judge | DONE | 1 | 2 | US-AUD-011 |
| US-AUD-081 | Initialize SDL3 audio subsystem | DONE | 1 | 3 | US-ENG-011 |
| US-AUD-091 | Define AudioSystem interface | DONE | 1 | 2 | None |
| US-AUD-092 | Implement SDL3AudioSystem backend | DONE | 1 | 5 | US-AUD-091, US-AUD-081 |
| US-CHT-001 | Define internal chart structure | DONE | 1 | 3 | None |
| US-CHT-002 | Implement chart metadata model | DONE | 1 | 2 | US-CHT-001 |
| US-CHT-003 | Implement timing data model | DONE | 1 | 5 | US-CHT-001 |
| US-CHT-004 | Implement note data model | DONE | 1 | 3 | US-CHT-001 |
| US-CHT-005 | Parse KSF chart format | DONE | 1 | 5 | US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004 |
| US-JDG-001 | Deterministic pure logic judge | DONE | 1 | 4 | US-CHT-001, US-INP-001, US-AUD-001 |
| US-JDG-002 | Five-tier judgment classification | DONE | 1 | 4 | US-JDG-001 |
| US-JDG-003 | Automatic miss assignment | DONE | 1 | 2 | US-JDG-002 |
| US-JDG-004 | Judgment event emission | DONE | 1 | 2 | US-JDG-002, US-INP-001 |
| US-JDG-005 | GameplayState separation | DONE | 1 | 4 | US-JDG-004 |
| US-JDG-006 | Combo tracking | DONE | 1 | 2 | US-JDG-005 |
| US-JDG-011 | Judge frame independence | DONE | 1 | 4 | US-ENG-001, US-AUD-002 |
| US-JDG-012 | No RNG in judge | DONE | 1 | 2 | US-JDG-001 |
| US-JDG-019 | Default hardcoded timing profile | DONE | 1 | 2 | US-JDG-001 |
| US-REN-019 | Beat-space to screen-space conversion | DONE | 1 | 5 | None |
| US-REN-020 | Placeholder rectangle note rendering | DONE | 1 | 2 | US-REN-019 |
| US-REN-036 | Minimal timing feedback display | DONE | 1 | 2 | US-JDG-002 |
| US-SCN-007a | Minimal gameplay scene | DONE | 1 | 5 | US-ENG-011, US-INP-021, US-AUD-092, US-CHT-005, US-JDG-001, US-REN-020 |
| US-AST-009 | Command-line data directory argument | DONE | 1 | 2 | None |
| US-AST-010 | Env variable for data directory | DONE | 1 | 1 | US-AST-009 |
| US-AST-032 | Missing BGA allows gameplay w/o bg | DONE | 1 | 1 | None |
| US-AST-033 | All missing assets logged | DONE | 1 | 1 | US-AST-001 |
| US-SCN-007b | Full gameplay scene orchestration | DONE | 2 | 8 | US-SCN-001, US-SCN-007a, US-REN-021, US-REN-024, US-REN-027 |

---

## Phase 2

| Story ID | Title | Status | Phase | Points | Dependencies |
|----------|-------|--------|-------|--------|--------------|
| US-REN-034 | High refresh rate rendering | DONE | 2 | 5 | US-REN-002 |
| US-REN-035 | Note field rendering performance | DONE | 2 | 8 | US-REN-021 |
| US-SCN-003 | Boot scene with logo display | DONE | 2 | 3 | US-SCN-001 |
| US-SCN-004 | Title scene with attract mode loop | DONE | 2 | 3 | US-SCN-001 |
| US-SCN-005 | Mode select scene | DONE | 2 | 5 | US-SCN-001 |
| US-SCN-007b | Full gameplay scene orchestration | DONE | 2 | 8 | US-SCN-001, US-SCN-007a, US-REN-021, US-REN-024, US-REN-027 |

---

## Phase 3

| Story ID | Title | Status | Phase | Points | Dependencies |
|----------|-------|--------|-------|--------|--------------|
| US-AST-017 | Chart loaded on song selection | DONE | 3 | 2 | US-AST-013 |
| US-AST-019 | Audio loaded at gameplay start | DONE | 3 | 2 | US-AST-017 |
| US-AST-020 | BGA loaded on song selection | DONE | 3 | 2 | US-AST-016 |
| US-INP-023 | Persist keymap to settings file | DONE | 3 | 3 | US-INP-022 |
| US-AUD-031 | Load short audio samples to memory | DONE | 3 | 3 | US-AUD-001, US-AUD-002 |
| US-AUD-032 | Play sound effects with low latency | DONE | 3 | 5 | US-AUD-031 |
| US-AUD-033 | Independent SFX volume control | DONE | 3 | 2 | US-AUD-032 |
| US-AUD-041 | Trigger key sounds on panel press | DONE | 3 | 2 | US-AUD-032 |
| US-AUD-042 | Support per-column key sounds | DONE | 3 | 2 | US-AUD-041 |
| US-AUD-051 | Play judgment sounds based on timing | DONE | 3 | 2 | US-AUD-032 |
| US-AUD-052 | Configurable judgment sound volume | DONE | 3 | 2 | US-AUD-051 |
| US-AUD-093 | Select audio backend at compile time | DONE | 3 | 2 | US-AUD-092 |
| US-CHT-018 | Store preview audio information | DONE | 3 | 2 | US-CHT-002 |
| US-JDG-007 | Hold note head judgment | DONE | 3 | 4 | US-JDG-002 |
| US-JDG-008 | Hold body continuous scoring | DONE | 3 | 4 | US-JDG-007 |
| US-JDG-009 | Hold grace window recovery | DONE | 3 | 4 | US-JDG-008, US-JDG-013 |
| US-JDG-010 | Life gauge with HP drain | DONE | 3 | 2 | US-JDG-005, US-JDG-013 |
| US-REN-025 | Hold note body rendering | DONE | 3 | 5 | US-REN-021 |
| US-REN-026 | Hold note cap rendering | DONE | 3 | 2 | US-REN-025 |
| US-REN-028 | Hit effects and receptor flash | DONE | 3 | 3 | US-REN-021 |
| US-REN-029 | Life gauge visual rendering | DONE | 3 | 3 | None |
| US-SCN-006 | Song select scene with music wheel | DONE | 3 | 8 | US-SCN-001 |
| US-SCN-008 | Result scene w/ grade & breakdown | DONE | 3 | 5 | US-SCN-001 |
| US-DAT-001 | Platform-appropriate user data dir | DONE | 3 | 2 | None |
| US-DAT-002 | Automatic user data dir creation | DONE | 3 | 2 | US-DAT-001 |
| US-DAT-003 | Settings JSON file structure | DONE | 3 | 3 | US-DAT-002 |
| US-DAT-004 | Settings load at startup | DONE | 3 | 2 | US-DAT-003 |
| US-DAT-005 | Settings save on change | DONE | 3 | 2 | US-DAT-003, US-DAT-004 |
| US-DAT-006 | Atomic file write for settings | DONE | 3 | 2 | US-DAT-005 |
| US-DAT-007 | Settings value validation | DONE | 3 | 3 | US-DAT-004 |
| US-DAT-031 | Chart metadata cache file structure | DONE | 3 | 5 | US-DAT-002 |
| US-DAT-032 | Cache invalidation on dir modify | DONE | 3 | 2 | US-DAT-031 |
| US-DAT-033 | Fast cache loading performance | DONE | 3 | 2 | US-DAT-031 |
| US-AST-011 | Config file for multiple data dirs | DONE | 3 | 2 | US-AST-009, US-AST-012 |
| US-AST-012 | Recursive directory scan for songs | DONE | 3 | 5 | US-AST-011 |
| US-AST-013 | Song metadata extraction during scan | DONE | 3 | 5 | US-AST-012 |
| US-AST-014 | Cached song DB for startup perf | DONE | 3 | 5 | US-AST-013 |
| US-AST-015 | Banner and audio file discovery | DONE | 3 | 2 | US-AST-012 |
| US-AST-016 | BGA file discovery per song | DONE | 3 | 2 | US-AST-012 |
| US-AST-023 | Judgment and menu sound effects | DONE | 3 | 2 | US-AST-021 |
| US-AST-030 | Missing chart/audio excludes song | DONE | 3 | 1 | US-AST-012 |
| US-AST-031 | Missing banner shows placeholder | DONE | 3 | 1 | US-AST-015 |

---

## Phase 4

| Story ID | Title | Status | Phase | Points | Dependencies |
|----------|-------|--------|-------|--------|--------------|
| US-CHT-006 | Parse SSC chart format | PLANNED | 4 | 8 | US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004 |
| US-CHT-007 | Parse SMA chart format | PLANNED | 4 | 5 | US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004 |
| US-CHT-008 | Parse STX chart format | PLANNED | 4 | 8 | US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004 |
| US-CHT-009 | Parse SEE chart format | PLANNED | 4 | 8 | US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004 |
| US-CHT-010 | Parse NX chart format | PLANNED | 4 | 13 | US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004 |
| US-CHT-011 | Specify OSF JSON chart format | PLANNED | 4 | 5 | US-CHT-001, US-CHT-002, US-CHT-003, US-CHT-004 |
| US-CHT-012 | Parse OSF JSON chart format | PLANNED | 4 | 5 | US-CHT-011 |
| US-CHT-013 | Write charts to OSF format | PLANNED | 4 | 3 | US-CHT-011, US-CHT-012 |
| US-CHT-014 | Compute chart content hash | PLANNED | 4 | 5 | US-CHT-001, US-CHT-003, US-CHT-004, US-CHT-015 |
| US-CHT-015 | Canonical binary repr for hashing | PLANNED | 4 | 2 | US-CHT-001, US-CHT-003, US-CHT-004 |
| US-CHT-016 | Validate charts for common errors | PLANNED | 4 | 5 | US-CHT-001, US-CHT-003, US-CHT-004 |
| US-CHT-017 | Classify chart difficulty and mode | PLANNED | 4 | 3 | US-CHT-002, US-CHT-004 |
| US-CHT-019 | Handle multi-chart files | PLANNED | 4 | 5 | US-CHT-006, US-CHT-017 |
| US-JDG-013 | Data-driven timing windows | PLANNED | 4 | 4 | US-JDG-002 |
| US-JDG-014 | Scoring formula in judge profile | PLANNED | 4 | 4 | US-JDG-013, US-JDG-005 |
| US-JDG-015 | Grade calculation | PLANNED | 4 | 2 | US-JDG-005, US-JDG-014 |
| US-JDG-016 | Judge profile per PIU version | PLANNED | 4 | 4 | US-JDG-013, US-JDG-014 |

---

## Phase 5

| Story ID | Title | Status | Phase | Points | Dependencies |
|----------|-------|--------|-------|--------|--------------|
| US-INP-061 | Separate input snapshots per player | PLANNED | 5 | 3 | US-INP-002, US-INP-011 |
| US-INP-062 | Keyboard driver player assignment | PLANNED | 5 | 2 | US-INP-021, US-INP-061 |
| US-AUD-012 | Position accuracy across seek ops | PLANNED | 5 | 3 | US-AUD-004, US-AUD-011 |
| US-AUD-061 | Global audio offset to judge timing | PLANNED | 5 | 2 | US-AUD-021 |
| US-AUD-062 | Persist audio offset in user profile | PLANNED | 5 | 1 | US-AUD-061 |
| US-AUD-071 | Calibration screen with metronome | PLANNED | 5 | 5 | US-AUD-061 |
| US-AUD-072 | Save calibration & return to prev | PLANNED | 5 | 1 | US-AUD-071, US-AUD-062 |
| US-AUD-082 | Stable operation over extended sess | PLANNED | 5 | 3 | US-AUD-081 |
| US-JDG-017 | Co-op mode dual judge instances | PLANNED | 5 | 6 | US-JDG-001, US-JDG-005 |
| US-JDG-018 | Shared vs separate life gauge | PLANNED | 5 | 2 | US-JDG-017, US-JDG-010 |
| US-REN-030 | Single mode note field layout | PLANNED | 5 | 2 | US-REN-021 |
| US-REN-031 | Double mode note field layout | PLANNED | 5 | 3 | US-REN-030 |
| US-REN-032 | C-Mod speed modifier | PLANNED | 5 | 5 | US-REN-019 |
| US-REN-033 | M-Mod speed modifier | PLANNED | 5 | 3 | US-REN-019 |
| US-SCN-009 | Name entry scene for high scores | PLANNED | 5 | 5 | US-SCN-001 |
| US-SCN-010 | Pause overlay scene | PLANNED | 5 | 5 | US-SCN-001, US-AUD-001 |
| US-SCN-011 | Scene transitions w/ BGA animations | PLANNED | 5 | 5 | US-SCN-001 |
| US-SCN-012 | Settings scene for configuration | PLANNED | 5 | 8 | US-SCN-001 |
| US-SCN-013 | Profile selection scene | PLANNED | 5 | 5 | US-SCN-001 |
| US-LUA-001 | Integrate sol2 Lua binding library | PLANNED | 5 | 2 | None |
| US-LUA-002 | Expose input query API to Lua | PLANNED | 5 | 2 | US-LUA-001 |
| US-LUA-003 | Expose audio control API to Lua | PLANNED | 5 | 3 | US-LUA-001 |
| US-LUA-004 | Expose sprite/BGA rendering to Lua | PLANNED | 5 | 3 | US-LUA-001 |
| US-LUA-005 | Expose scene stack nav API to Lua | PLANNED | 5 | 3 | US-LUA-001 |
| US-LUA-006 | Expose profile/score access to Lua | PLANNED | 5 | 2 | US-LUA-001 |
| US-LUA-007 | Expose timer utilities to Lua | PLANNED | 5 | 2 | US-LUA-001 |
| US-LUA-008 | Per-frame Lua execution budget | PLANNED | 5 | 5 | US-LUA-001 |
| US-LUA-009 | Log Lua errors with stack traces | PLANNED | 5 | 2 | US-LUA-001 |
| US-LUA-010 | Sandbox Lua filesystem/OS access | PLANNED | 5 | 2 | US-LUA-001 |
| US-LUA-012 | Implement boot screen in Lua | PLANNED | 5 | 2 | US-LUA-002, US-LUA-004, US-LUA-005, US-LUA-007 |
| US-LUA-022 | Expose primitive shape drawing | PLANNED | 5 | 3 | US-LUA-001, US-LUA-004 |
| US-DAT-008 | Profile JSON file structure | PLANNED | 5 | 3 | US-DAT-002 |
| US-DAT-009 | Default profile creation | PLANNED | 5 | 2 | US-DAT-008 |
| US-DAT-010 | Profile load and activation | PLANNED | 5 | 2 | US-DAT-008, US-DAT-009 |
| US-DAT-011 | Profile save after gameplay | PLANNED | 5 | 2 | US-DAT-010 |
| US-DAT-012 | Atomic file write for profiles | PLANNED | 5 | 2 | US-DAT-011 |
| US-DAT-013 | Profile service object API | PLANNED | 5 | 5 | US-DAT-010, US-DAT-011 |
| US-DAT-014 | Chart content hash computation | PLANNED | 5 | 5 | None |
| US-DAT-015 | Record high score after gameplay | PLANNED | 5 | 2 | US-DAT-013, US-DAT-014 |
| US-DAT-016 | High score entry metadata | PLANNED | 5 | 1 | US-DAT-015 |
| US-DAT-017 | Query top high scores for chart | PLANNED | 5 | 1 | US-DAT-015 |
| US-DAT-023 | Profile selection at startup | PLANNED | 5 | 5 | US-DAT-010 |
| US-DAT-024 | Create new profile | PLANNED | 5 | 3 | US-DAT-023 |
| US-DAT-025 | Delete profile with confirmation | PLANNED | 5 | 2 | US-DAT-023 |
| US-DAT-026 | Switch active profile w/o data loss | PLANNED | 5 | 2 | US-DAT-023, US-DAT-011 |
| US-DAT-027 | Persist active profile across sess | PLANNED | 5 | 2 | US-DAT-026, US-DAT-004 |
| US-DAT-028 | Schema version number in JSON files | PLANNED | 5 | 1 | US-DAT-008, US-DAT-003 |
| US-DAT-029 | Migration function schema v0 to v1 | PLANNED | 5 | 3 | US-DAT-028 |
| US-DAT-030 | Preserve unknown JSON fields | PLANNED | 5 | 2 | US-DAT-008 |
| US-AST-024 | Note skin directory structure | PLANNED | 5 | 5 | US-AST-021 |
| US-AST-025 | Multiple note skins installed | PLANNED | 5 | 2 | US-AST-024 |
| US-AST-026 | Selected note skin in profile | PLANNED | 5 | 2 | US-AST-025 |

---

## Phase 6

| Story ID | Title | Status | Phase | Points | Dependencies |
|----------|-------|--------|-------|--------|--------------|
| US-INP-031 | HidPadDriver using SDL3 gamepad API | PLANNED | 6 | 5 | US-INP-003 |
| US-INP-032 | Button-to-panel mapping per device | PLANNED | 6 | 3 | US-INP-031 |
| US-INP-033 | Axis threshold for analog sensors | PLANNED | 6 | 3 | US-INP-032 |
| US-INP-063 | HID driver device-to-player binding | PLANNED | 6 | 2 | US-INP-031, US-INP-061 |
| US-INP-071 | Input mapping configuration screen | PLANNED | 6 | 5 | US-INP-031, US-INP-032 |
| US-INP-072 | Per-device input calibration offsets | PLANNED | 6 | 3 | US-INP-031 |
| US-INP-073 | Calibration feedback screen | PLANNED | 6 | 5 | US-INP-072 |
| US-INP-081 | Merge input from multiple drivers | PLANNED | 6 | 3 | US-INP-003, US-INP-021, US-INP-031 |
| US-INP-082 | Configure driver priority | PLANNED | 6 | 2 | US-INP-081 |

---

## Phase 7

| Story ID | Title | Status | Phase | Points | Dependencies |
|----------|-------|--------|-------|--------|--------------|
| US-LUA-011 | Lua game directory structure | PLANNED | 7 | 1 | US-LUA-001 |
| US-LUA-013 | Implement title screen in Lua | PLANNED | 7 | 2 | US-LUA-012 |
| US-LUA-014 | Implement mode select screen in Lua | PLANNED | 7 | 2 | US-LUA-013 |
| US-LUA-015 | Implement song select screen in Lua | PLANNED | 7 | 5 | US-LUA-014 |
| US-LUA-016 | Implement result screen in Lua | PLANNED | 7 | 3 | US-LUA-015 |
| US-LUA-017 | Implement name entry screen in Lua | PLANNED | 7 | 3 | US-LUA-016 |
| US-LUA-018 | Complete Exceed-style game package | PLANNED | 7 | 1 | US-LUA-012 through US-LUA-017 |
| US-LUA-019 | Complete NX-style game package | PLANNED | 7 | 2 | US-LUA-018 |
| US-LUA-020 | Game version switching at runtime | PLANNED | 7 | 5 | US-LUA-018, US-LUA-019 |
| US-LUA-021 | Lua script hot reloading | PLANNED | 7 | 5 | US-LUA-018 |
| US-DAT-018 | Track total songs played | PLANNED | 7 | 1 | US-DAT-008, US-DAT-011 |
| US-DAT-019 | Track total play time | PLANNED | 7 | 1 | US-DAT-008, US-DAT-011 |
| US-DAT-020 | Track total score accumulated | PLANNED | 7 | 1 | US-DAT-008, US-DAT-011 |
| US-DAT-021 | Track judgment distribution | PLANNED | 7 | 1 | US-DAT-008, US-DAT-011 |
| US-DAT-022 | Calculate average accuracy | PLANNED | 7 | 1 | US-DAT-021 |
| US-AST-027 | Exceed-era detection via .see files | PLANNED | 7 | 2 | US-AST-012 |
| US-AST-028 | NX-era detection via .nx files | PLANNED | 7 | 2 | US-AST-027 |
| US-AST-029 | Multi-version data coexistence | PLANNED | 7 | 5 | US-AST-028 |

---

## Phase 8

| Story ID | Title | Status | Phase | Points | Dependencies |
|----------|-------|--------|-------|--------|--------------|
| US-INP-041 | ArcadeIODriver for PIUIO boards | PLANNED | 8 | 8 | US-INP-003 |
| US-INP-042 | Support PIUIO v1 and v2 hardware | PLANNED | 8 | 5 | US-INP-041 |
| US-INP-051 | Control panel lamps during gameplay | PLANNED | 8 | 5 | US-INP-041 |
| US-INP-064 | Arcade I/O native player separation | PLANNED | 8 | 2 | US-INP-041, US-INP-061 |
| US-NET-001 | HTTP client integration | FUTURE | 8 | 3 | None |
| US-NET-002 | Background thread request executor | FUTURE | 8 | 5 | US-NET-001 |
| US-NET-003 | Default request timeout | FUTURE | 8 | 2 | US-NET-002 |
| US-NET-020 | Post score after gameplay | FUTURE | 8 | 3 | US-NET-011 |
| US-NET-021 | Score submission payload | FUTURE | 8 | 2 | US-NET-020 |
| US-NET-022 | Failed submission queue | FUTURE | 8 | 5 | US-NET-020 |
| US-NET-023 | Capture RNG seed with score | FUTURE | 8 | 2 | US-NET-021 |
| US-NET-050 | API documentation for client valid | FUTURE | 8 | 3 | None |
| US-NET-051 | Persistent score storage | FUTURE | 8 | 5 | US-NET-050 |
| US-NET-100 | Offline mode is fully functional | FUTURE | 8 | 2 | US-NET-002 |
| US-NET-101 | Network ops never block rendering | FUTURE | 8 | 2 | US-NET-002 |

---

## Phase 9

| Story ID | Title | Status | Phase | Points | Dependencies |
|----------|-------|--------|-------|--------|--------------|
| US-NET-010 | Account registration | FUTURE | 9 | 5 | US-NET-002 |
| US-NET-011 | Account login | FUTURE | 9 | 3 | US-NET-010 |
| US-NET-012 | Token refresh mechanism | FUTURE | 9 | 5 | US-NET-011 |
| US-NET-030 | Query leaderboard by chart hash | FUTURE | 9 | 2 | US-NET-001, US-NET-020 |
| US-NET-031 | Display leaderboard in song select | FUTURE | 9 | 5 | US-NET-030 |
| US-NET-032 | Leaderboard pagination | FUTURE | 9 | 2 | US-NET-031 |
| US-NET-040 | Capture input snapshot each tick | FUTURE | 9 | 5 | None |
| US-NET-041 | Include replay hash in submission | FUTURE | 9 | 2 | US-NET-040, US-NET-021 |
| US-NET-043 | Replay playback for verification | FUTURE | 9 | 5 | US-NET-040 |
| US-NET-052 | Rate limiting per account and IP | FUTURE | 9 | 3 | US-NET-051 |
| US-NET-053 | Score validation for physical poss | FUTURE | 9 | 5 | US-NET-051 |
| US-NET-054 | Suspicious score flagging w/ review | FUTURE | 9 | 8 | US-NET-053 |
| US-NET-060 | Web leaderboard browser | FUTURE | 9 | 5 | US-NET-030 |
| US-NET-061 | Player profile page | FUTURE | 9 | 5 | US-NET-060 |
| US-NET-062 | Player search | FUTURE | 9 | 3 | US-NET-061 |
| US-NET-102 | Progress indication for long ops | FUTURE | 9 | 2 | US-NET-002 |
| US-NET-103 | SQL injection and XSS protection | FUTURE | 9 | 2 | US-NET-051 |

---

## FUTURE (no phase assigned)

| Story ID | Title | Status | Phase | Points | Dependencies |
|----------|-------|--------|-------|--------|--------------|
| US-DAT-034 | Export profile to single file | FUTURE | 7 | 2 | US-DAT-008, US-DAT-023 |
| US-DAT-035 | Import profile from single file | FUTURE | 7 | 3 | US-DAT-034 |

---

## Non-Functional Requirements

NFRs are tracked separately. Some have story points (spike/research tasks); most are validated through integration testing and do not carry separate implementation effort.

| Story ID | Title | Status | Phase | Points | Dependencies |
|----------|-------|--------|-------|--------|--------------|
| NFR-ENG-PERF | Performance targets (60 FPS) | PLANNED | 3 | N/A | None |
| NFR-ENG-MEM | Memory constraints (<512 MB) | PLANNED | 3 | N/A | None |
| NFR-INP-LAT | Input latency minimization (<10ms) | PLANNED | 8+ | N/A | None |
| NFR-AUD-POS | Audio position accuracy (<2ms) | PLANNED | 1 | N/A | US-AUD-011 |
| NFR-AUD-SFX | Sound effect latency (<5ms) | PLANNED | 3 | N/A | US-AUD-032 |
| NFR-CHT-001 | Parser performance (500 charts/2s) | PLANNED | N/A | N/A | US-CHT-005, US-CHT-006 |
| NFR-CHT-002 | Timing accuracy (<0.1ms) | PLANNED | N/A | N/A | US-CHT-003 |
| NFR-CHT-003 | Chart memory efficiency (<50KB) | PLANNED | N/A | N/A | US-CHT-001 |
| US-JDG-NFR-001 | Judgment accuracy (<1ms) | PLANNED | N/A | 2 | US-AUD-011 |
| US-JDG-NFR-002 | Judge performance budget (<100us) | PLANNED | N/A | 1 | US-JDG-001 |
| US-JDG-NFR-003 | Judge profile validation | PLANNED | N/A | 2 | US-JDG-013 |
| NFR-REN-001 | Visual quality | PLANNED | N/A | N/A | None |
| NFR-REN-002 | Frame timing stability | PLANNED | N/A | N/A | None |
| NFR-REN-003 | Asset loading performance | PLANNED | N/A | N/A | None |
| NFR-REN-004 | Memory efficiency (textures) | PLANNED | N/A | N/A | None |
| NFR-SCN-001 | Scene transition latency (<16ms) | PLANNED | N/A | N/A | US-SCN-001 |
| NFR-SCN-002 | Scene stack memory safety | PLANNED | N/A | N/A | US-SCN-001 |
| NFR-SCN-003 | Scene render budget (<8ms) | PLANNED | N/A | N/A | None |
| NFR-LUA-001 | Lua execution perf (<5ms/frame) | PLANNED | N/A | N/A | US-LUA-008 |
| NFR-LUA-002 | Script load time (<500ms) | PLANNED | N/A | N/A | None |
| NFR-LUA-003 | Memory per game package (<50MB) | PLANNED | N/A | N/A | None |
| NFR-LUA-004 | Error recovery time (<16.67ms) | PLANNED | N/A | N/A | US-LUA-009 |
| NFR-LUA-005 | Hot reload latency (<2s, dev only) | PLANNED | N/A | N/A | US-LUA-021 |
| US-DAT-NFR-001 | Data corruption prevention | PLANNED | 3-5 | N/A | US-DAT-006, US-DAT-012 |
| US-DAT-NFR-002 | Thread safety for profile service | PLANNED | 8 | 2 | US-DAT-013 |
| NFR-AST-001 | Startup performance (<5s warm) | PLANNED | N/A | N/A | US-AST-014 |
| NFR-AST-002 | Memory efficiency (<100KB/song) | PLANNED | N/A | N/A | US-AST-013 |
| NFR-AST-003 | Case-insensitive scan perf (<10s) | PLANNED | N/A | N/A | US-AST-012 |
| NFR-AST-004 | Texture probe latency (<5ms) | PLANNED | N/A | N/A | US-AST-002, US-AST-003 |
| NFR-AST-005 | Format converter correctness | PLANNED | N/A | N/A | US-AST-006, US-AST-007, US-AST-008 |

---

## Legend

| Status | Meaning |
|--------|---------|
| **DONE** | Implemented, tested, and merged into the codebase |
| **PLANNED** | Defined with acceptance criteria and assigned to a specific phase |
| **FUTURE** | Defined with acceptance criteria but not yet scheduled, or in Phase 8-9 |

| Field | Meaning |
|-------|---------|
| **Story ID** | Unique identifier (US-{subsystem}-{number}) |
| **Phase** | Roadmap phase (1-9) or N/A for completed/unphased work |
| **Points** | Story point estimate; "?" means not specified in source file; "N/A" means NFR validated via testing |
| **Dependencies** | Other story IDs that must be completed first |

---

## Source Files

| File | Subsystem | Stories | DONE | PLANNED | FUTURE |
|------|-----------|---------|------|---------|--------|
| 01-core-engine.md | Engine (ENG) | 14 | 6 | 8 | 0 |
| 02-input-system.md | Input (INP) | 26 | 0 | 26 | 0 |
| 03-audio-system.md | Audio (AUD) | 23 | 9 | 14 | 0 |
| 04-chart-system.md | Chart (CHT) | 19 | 5 | 14 | 0 |
| 05-gameplay-judge.md | Judge (JDG) | 23 | 9 | 14 | 0 |
| 06-visual-rendering.md | Rendering (REN) | 36 | 23 | 13 | 0 |
| 07-screen-flow.md | Scenes (SCN) | 14 | 3 | 11 | 0 |
| 08-scripting-system.md | Lua (LUA) | 22 | 0 | 22 | 0 |
| 09-data-management.md | Data (DAT) | 37 | 0 | 33 | 2 |
| 10-network-system.md | Network (NET) | 27 | 0 | 0 | 27 |
| 11-asset-management.md | Assets (AST) | 33 | 12 | 21 | 0 |
