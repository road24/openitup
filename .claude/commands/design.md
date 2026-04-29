---
description: "Produce a technical design for one or more stories. Stage 5: transforms sprint-ready stories into a Technical Design Document (TD) with classes, interfaces, file plans, and ADRs. Provide a story ID (US-XXX-NNN) or a set of related story IDs."
---

# Technical Design

**Target**: $ARGUMENTS

## Process

1. **Locate stories**: Read the specified story/stories from `docs/stories/`. Quick quality check — if a story lacks acceptance criteria or looks incomplete, recommend running `/audit` or `/refine` first.

2. **Read the codebase**: Use the **technical-architect** agent to:
   - Read the project's `CLAUDE.md`, source tree (`src/`), and build system (`CMakeLists.txt`)
   - Read existing technical designs in `docs/technical-designs/` for patterns and conventions
   - Understand what already exists so the design integrates cleanly

3. **Produce TD**: Generate a Technical Design Document (TD-XXX-NNN) covering:
   - Architecture overview (how it fits the existing system)
   - New types with interface sketches (public API, ownership, dependencies)
   - Modified types (what changes to existing code)
   - File plan (every new/modified file listed)
   - Data flow for key scenarios
   - Architectural Decision Records for non-obvious choices
   - Risk assessment
   - Testing strategy

4. **Present for review**: Show the TD and ask: **"Does this technical design look right? Want to revise anything before we plan the implementation?"**

5. **Save artifact**: Create `docs/technical-designs/` directory if needed. Save as `docs/technical-designs/TD-XXX-NNN.md`.

6. **Handoff**: After PO approval, recommend:
   > "Design complete. Run `/plan TD-XXX-NNN` to break this into implementation steps, or iterate on this design first."
