---
description: "User story formatting standards"
globs:
  - "docs/stories/**"
---

# User Story Format Standards

When creating or modifying files in docs/stories/:

## Story ID Format
- Use `US-XXX-NNN` where XXX is the subsystem prefix and NNN is a sequential number.
- The XXX prefix must match the parent requirement's subsystem prefix.
- Never reuse an ID. Continue from the last ID in the target file.

## Required Structure
Every story must have:
1. **H3 heading**: `### Story ID: US-XXX-NNN - [Goal-Oriented Title]`
2. **Story Card**: As a / I want to / So that (in blockquote)
3. **References**: Link to parent `REQ-XXX-NNN`
4. **Status**: `DONE` | `PLANNED` | `IN PROGRESS`
5. **Description**: 1-2 sentences of context
6. **Acceptance Criteria**: Gherkin format (Given/When/Then)
7. **Technical Notes**: Estimation Pointer (story points), Dependencies (US-XXX-NNN refs), Phase

## Acceptance Criteria Rules
- Minimum 2 scenarios per story (1 happy path + 1 edge case)
- Strict Given/When/Then format
- No implementation details in scenarios
- Each scenario must be independently testable

## STATUS.md Maintenance
When adding or updating stories, update `docs/stories/STATUS.md`:
- Add new rows to the appropriate phase table
- Update phase summary point totals
- Update status when stories change state (PLANNED → IN PROGRESS → DONE)
- Update the DONE count in the phase summary
