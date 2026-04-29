# Business Requirements Index

This directory contains the formal business requirements for the openitup Pump It Up engine reimplementation. Requirements are extracted from the engine roadmap and organized by subsystem.

## Requirements Summary

| Category | File | Total | Done | Planned | Future |
|----------|------|-------|------|---------|--------|
| Core Engine | [01-core-engine.md](01-core-engine.md) | 8 | 3 | 5 | 0 |
| Input System | [02-input-system.md](02-input-system.md) | 12 | 1 | 10 | 1 |
| Audio System | [03-audio-system.md](03-audio-system.md) | 11 | 0 | 11 | 0 |
| Chart System | [04-chart-system.md](04-chart-system.md) | 15 | 1 | 13 | 1 |
| Gameplay Judge | [05-gameplay-judge.md](05-gameplay-judge.md) | 15 | 0 | 15 | 0 |
| Visual Rendering | [06-visual-rendering.md](06-visual-rendering.md) | 19 | 7 | 12 | 0 |
| Screen Flow | [07-screen-flow.md](07-screen-flow.md) | 14 | 0 | 14 | 0 |
| Scripting System | [08-scripting-system.md](08-scripting-system.md) | 10 | 0 | 10 | 0 |
| Data Management | [09-data-management.md](09-data-management.md) | 13 | 0 | 13 | 0 |
| Network System | [10-network-system.md](10-network-system.md) | 11 | 0 | 0 | 11 |
| Asset Management | [11-asset-management.md](11-asset-management.md) | 11 | 3 | 8 | 0 |
| **TOTAL** | | **139** | **15** | **111** | **13** |

## Status Definitions

- **[DONE]**: Implemented and tested in current codebase
- **[PLANNED Phase N]**: Scheduled for implementation in roadmap phase N
- **[FUTURE]**: Identified but not scheduled for any current phase

## Requirements Format

Each requirement follows this structure:

```
REQ-XXX-NNN: [Title]
Status: [DONE] | [PLANNED Phase N] | [FUTURE]
Priority: Must Have | Should Have | Could Have

Description: Clear statement of the business need.

Acceptance Criteria:
- Measurable condition 1
- Measurable condition 2

Dependencies: Other requirement IDs
Source: Roadmap section reference
```

## Coverage

These requirements cover:
- All 11 subsystems defined in the roadmap
- All 9 implementation phases
- Current state (BGA subsystem complete with 82 tests)
- Architecture decisions and their rationale
- Implicit requirements derived from technical constraints

## Notes

- Requirements are traceable to specific roadmap sections
- Priorities follow MoSCoW method
- Phase assignments match the roadmap implementation plan
- Some requirements span multiple phases (marked with earliest phase)
