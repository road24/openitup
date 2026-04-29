---
description: "Create an implementation plan from a technical design. Stage 6: breaks a TD into ordered steps where each step is one commit that compiles and passes tests. Provide a TD reference (TD-XXX-NNN)."
---

# Implementation Plan

**Target**: $ARGUMENTS

## Process

1. **Locate design**: Read the Technical Design Document from `docs/technical-designs/TD-XXX-NNN.md`. If the TD doesn't exist, recommend running `/design` first.

2. **Read build context**: Use the **technical-lead** agent to:
   - Read the project's `CLAUDE.md` for build/test commands
   - Read `CMakeLists.txt` for build structure
   - Read existing test files for patterns and conventions
   - Read source files listed in the TD's "Modified Types" section

3. **Produce IP**: Generate an Implementation Plan (IP-XXX-NNN) with:
   - Ordered steps, each specifying:
     - Files to create or modify
     - What to implement (referencing TD interface sketches)
     - Tests to write
     - Definition of done (build + test commands)
     - Expected commit message
   - PR strategy (single vs multiple PRs)
   - Build verification commands
   - Acceptance verification (which tests prove which stories)

4. **Present for review**: Show the IP and ask: **"Does this build plan look right? Want to adjust step order or granularity?"**

5. **Save artifact**: Create `docs/implementation-plans/` directory if needed. Save as `docs/implementation-plans/IP-XXX-NNN.md`.

6. **Handoff**: After PO approval, recommend:
   > "Plan ready. Run `/build IP-XXX-NNN step 1` to start coding."
