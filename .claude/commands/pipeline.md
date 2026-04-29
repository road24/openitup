---
description: "Drive an artifact forward through all remaining SDLC stages until it's ready to build. Provide a starting point: a requirement (REQ-XXX-NNN), a story (US-XXX-NNN), a subsystem name, or 'phase N'. The pipeline detects where the artifact is and runs every stage from there."
---

# Pipeline to Ready

**Target**: $ARGUMENTS

## Process

### Step 1: Detect Current State

Parse the target and determine where it sits in the pipeline:

1. **If REQ-XXX-NNN**: Starts at Stage 2 (verify requirement)
2. **If US-XXX-NNN**: Check if it has estimates and passed audit:
   - No estimates or audit → starts at Stage 4 (estimate + audit)
   - Has estimates, passed audit → starts at Stage 5 (technical design)
   - Has TD document → starts at Stage 6 (plan)
   - Has IP document → ready for dev loop. Report: "Ready to code. Run `/code IP-XXX-NNN step 1` to start."
3. **If subsystem name or "phase N"**: Gather all PLANNED stories in scope, determine the earliest stage needed per story, and process them in batches

### Step 2: Run Remaining Stages

Execute each remaining stage sequentially, with PO approval gates between stages.

#### From Stage 2: Verify Requirements
- Use **business-requirements-verifier** on the requirement(s)
- If score ≥ 70%: proceed to Stage 3
- If score < 70%: present issues and ask PO to approve fixes
  - If approved: use **business-requirements-analyst** to rewrite, then re-verify
  - Loop until passing or PO decides to skip

Commit verification results.

#### Stage 3: Create Stories
- Use **agile-story-architect** to decompose requirements into stories
- Present stories to PO for approval
- Commit stories and STATUS.md updates

#### Stage 4: Estimate + Audit
- Use **story-estimator** to size stories
- Use **agile-requirements-auditor** to audit quality
- Stories scoring ≥ 70%: proceed
- Stories scoring < 70%: present issues and ask PO to approve revisions
  - If approved: revise and re-audit
  - Loop until passing or PO decides to skip
- Stories > 8 points: recommend decomposition, send back to Stage 3 if PO agrees

Commit estimate and audit updates.

#### Stage 5: Technical Design
- Use **technical-architect** to produce TD-XXX-NNN
- Present design to PO for approval
- If PO wants revisions: iterate on the design before proceeding
- Commit TD document

#### Stage 6: Implementation Plan
- Use **technical-lead** to produce IP-XXX-NNN from the TD
- Present plan to PO for approval
- If PO wants revisions: iterate on the plan before proceeding
- Commit IP document

### Step 3: Report Completion

```
## Pipeline Complete

**Started from**: [Stage N — what triggered it]
**Artifacts produced**:
- [x] Requirements verified (score: N%)
- [x] Stories created: US-XXX-NNN, US-XXX-NNN (N total)
- [x] Estimates: N points total
- [x] Audit: N% average quality score
- [x] Technical Design: TD-XXX-NNN
- [x] Implementation Plan: IP-XXX-NNN (N steps)

**Ready to code**: Run `/code IP-XXX-NNN step 1` to start the dev loop.
```

### Batch Mode

When processing a subsystem or phase with multiple stories:

1. Group stories by their current pipeline position
2. Process the furthest-behind group first (requirements before stories before estimates)
3. Present batch results at each stage
4. PO can approve the whole batch or cherry-pick individual stories to advance
5. Stories that fail a quality gate are held back — the rest proceed

```
## Batch Pipeline Status

| Story ID | Stage 2 | Stage 3 | Stage 4 | Stage 5 | Stage 6 | Status |
|----------|---------|---------|---------|---------|---------|--------|
| US-XXX-NNN | ✅ | ✅ | ✅ | ✅ | ✅ | Ready to build |
| US-YYY-NNN | ✅ | ✅ | ⚠️ 65% | — | — | Held: audit score |
| US-ZZZ-NNN | ✅ | ✅ | ✅ | ✅ | — | In progress: IP |
```

## Rules

- **PO approval at every stage.** Never auto-advance without confirmation.
- **Each stage = separate commit.** The pipeline produces N commits, one per stage per batch.
- **Quality gates are enforced.** Stories below 70% audit score don't advance unless PO explicitly overrides.
- **No skipping stages.** A requirement must be verified before stories are created. Stories must be estimated and audited before technical design.
- **Batch efficiency.** When processing multiple items, run agents in appropriate batches rather than one-by-one. Present consolidated reports.
