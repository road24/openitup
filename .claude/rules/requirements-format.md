---
description: "Business requirements formatting standards"
globs:
  - "docs/requirements/**"
---

# Business Requirements Format Standards

When creating or modifying files in docs/requirements/:

## Requirement ID Format
- Use `REQ-XXX-NNN` where XXX is a subsystem prefix (e.g., ENG, INP, AUD, CHT, JDG, REN, SCN, LUA, DAT, NET, AST) and NNN is a zero-padded sequential number.
- Never reuse an ID, even if the original requirement was deleted.
- Continue numbering from the last ID in the target file.

## Required Fields
Every requirement must have:
- **Status**: `[DONE]` | `[PLANNED Phase N]` | `[FUTURE]`
- **Priority**: `Must Have` | `Should Have` | `Could Have` (MoSCoW)
- **Description**: Clear business-level statement, not technical implementation. 1-3 sentences.
- **Acceptance Criteria**: Bullet list of measurable conditions.
- **Dependencies**: `REQ-XXX-NNN` references or `None`
- **Source**: Roadmap section, CLAUDE.md, or PO conversation reference

## Formatting Rules
- Separate requirements with `---` horizontal rules
- Use H2 (`##`) for requirement headings: `## REQ-XXX-NNN: [Title]`
- Bold field labels: `**Status**:`, `**Priority**:`, etc.
- Acceptance criteria as bullet points (not Gherkin — save Given/When/Then for stories)
- No implementation details — requirements describe WHAT, not HOW

## Index Maintenance
When adding requirements to a subsystem file, update `docs/requirements/README.md`:
- Increment the Total count for the affected subsystem
- Update Done/Planned/Future counts as appropriate
- Update the grand total row
