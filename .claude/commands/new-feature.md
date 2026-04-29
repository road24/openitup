---
description: "Run the full SDLC pipeline for a new feature idea. Takes a PO description through all 7 stages: requirements → verification → stories → estimation+audit → design → plan → build."
---

# New Feature Pipeline

The Product Owner has described a feature: **$ARGUMENTS**

Execute the following pipeline. At each stage, produce a committable artifact and pause for PO approval before proceeding.

## Stage 1: Business Requirements

Use the **business-requirements-analyst** agent to:
1. Read the project's CLAUDE.md and existing requirements in docs/requirements/
2. Transform the PO's description into formal REQ-XXX-NNN requirements
3. Present the requirements to the PO

After presenting, ask: **"Do these requirements capture what you want? Should I proceed to verification?"**

If approved, the requirements should be saved to the appropriate docs/requirements/ file.

## Stage 2: Verification

Use the **business-requirements-verifier** agent to:
1. Validate the new requirements against SMART criteria
2. Check for conflicts with existing requirements
3. Identify any coverage gaps

Present the verification report. If score < 70%, recommend revisions before proceeding.

Ask: **"Verification complete. Ready to create stories?"**

## Stage 3: User Stories

Use the **agile-story-architect** agent to:
1. Decompose the verified requirements into US-XXX-NNN stories
2. Write Gherkin acceptance criteria for each
3. Generate STATUS.md rows

Present the stories. Ask: **"Do these stories look right? Ready for estimation and quality audit?"**

## Stage 4: Estimation + Quality Audit

Use the **story-estimator** agent to size the stories, then the **agile-requirements-auditor** agent to check quality. Present both reports together.

If any stories need revision (auditor score < 70%), flag them.
If any stories need decomposition (estimator flags > 8 points), flag them.

Ask: **"Estimates and audit complete. Ready for technical design?"**

## Stage 5: Technical Design

Use the **technical-architect** agent to:
1. Read the project's source code and existing architecture
2. Produce a Technical Design Document (TD-XXX-NNN)
3. Define classes, interfaces, file plans, and architectural decisions

Present the design. Ask: **"Does this design look right? Want to revise before planning implementation?"**

## Stage 6: Implementation Plan

Use the **technical-lead** agent to:
1. Break the technical design into ordered implementation steps
2. Each step = one commit (compiles, passes tests)
3. Include test requirements and commit messages per step

Present the plan. Ask: **"Does this plan look right? Ready to start building?"**

## Stage 7: Dev Loop

If the PO approves the plan, begin the atomic dev loop for each IP step:

1. **`/code IP-XXX-NNN step 1`** — Write code + tests. Report what was written.
2. **`/compile`** — Build. On fail → `/code` to fix → `/compile` again.
3. **`/test`** — Run tests. On fail → `/code` to fix → `/compile` → `/test` again.
4. **`/review`** — Code review. On critical issues → `/code` to fix → cycle again.
5. **`/commit`** — Present diff, PO approves, commit.
6. Ask: **"Step N committed. Continue to step N+1?"**
7. Repeat for each step until done.

At any point, if implementation reveals a scope issue:
- Use **`/revise`** to escalate to the appropriate upstream stage
- PO must approve scope changes before they take effect

## Stage 8: Integration Verification

After all IP steps are committed:

1. **`/integrate`** — Run full build + complete test suite to verify cross-component behavior.
2. On fail → `/code` to fix → cycle through compile/test again → `/integrate`.

Ask: **"Integration verified. Ready for acceptance testing?"**

## Stage 9: Acceptance Testing

1. **`/accept`** — Verify each Gherkin AC scenario has a passing test. Check traceability to parent requirements.
2. On pass → update STATUS.md to DONE.
3. On partial → PO decides: write missing tests, accept with gaps, or `/revise`.

Say: **"Story US-XXX-NNN accepted and marked DONE."**

## Rules

- Each stage output should be committed separately
- If the PO says "stop" or "back" at any stage, halt and address feedback
- If a stage reveals issues with a previous stage's output, use `/revise`
- Never skip stages — the pipeline is sequential for a reason
- The PO can exit at any stage and resume later with individual skills
- Every dev loop action is atomic: code, compile, test, review, commit — each is its own step with PO visibility
