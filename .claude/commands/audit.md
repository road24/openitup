---
description: "Audit existing project artifacts — requirements, stories, estimates, and tracking. Provide a scope: 'all', a subsystem name (e.g., 'audio', 'input'), a phase (e.g., 'phase 1'), or a specific file path."
---

# Project Audit

**Scope**: $ARGUMENTS

## Process

This skill validates existing project artifacts that may have been created before the studio framework was in place. It checks quality, consistency, and identifies gaps.

### Step 1: Determine Scope

Parse the target:
- **"all"**: Audit every subsystem's requirements and stories
- **Subsystem name** (e.g., "audio", "input", "core-engine"): Audit that subsystem's requirements + stories
- **"phase N"**: Audit all stories assigned to that phase
- **"requirements"**: Audit only requirements across all subsystems
- **"stories"**: Audit only stories across all subsystems
- **File path**: Audit the specific file

### Step 2: Requirements Audit

For each requirements file in scope:

1. Use the **business-requirements-verifier** agent to:
   - Run SMART analysis on every requirement
   - Check cross-subsystem consistency
   - Identify coverage gaps against the roadmap (`docs/engine-roadmap.md` if it exists)
   - Flag conflicts and duplicates

2. Present a summary table:

```
## Requirements Audit Summary

| Subsystem | File | Total | Score | Critical Issues |
|-----------|------|-------|-------|-----------------|
| [name] | [file] | N reqs | N% | N issues |
```

3. List all Critical and Important issues with suggested fixes.

### Step 3: Stories Audit

For each stories file in scope:

1. Use the **agile-requirements-auditor** agent in batch mode to:
   - Run INVEST scorecard on every story
   - Check 3Cs compliance
   - Audit acceptance criteria health
   - Produce Quality Scores

2. Present a summary table:

```
## Stories Audit Summary

| Subsystem | File | Total | Avg Score | Ready | Needs Revision | Blocked |
|-----------|------|-------|-----------|-------|----------------|---------|
| [name] | [file] | N | N% | N | N | N |
```

3. List stories scoring below 70% with specific issues and fixes.

### Step 4: Estimation Audit

For stories in scope that have point estimates:

1. Use the **story-estimator** agent to:
   - Validate existing estimates against completed work calibration
   - Flag stories where the existing estimate seems off (too high or too low)
   - Identify unestimated stories (marked with "?" in STATUS.md)
   - Check for stories > 8 points that should be decomposed

2. Present:

```
## Estimation Audit

- **Estimated**: N stories (N points total)
- **Unestimated**: N stories
- **Overestimated**: N stories (suggest lower)
- **Underestimated**: N stories (suggest higher)
- **Needs decomposition**: N stories (> 8 points)
```

### Step 5: Tracking Consistency

Read `docs/stories/STATUS.md` and cross-reference against actual story files:

1. Check that every story in the story files appears in STATUS.md
2. Check that STATUS.md point totals match individual story estimates
3. Check that dependency references are valid (no dangling US-XXX-NNN refs)
4. Check that phase assignments are consistent between stories and STATUS.md
5. Check that DONE stories in STATUS.md are actually implemented in the codebase (search for related source files or tests)

Present:

```
## Tracking Consistency

- **Stories in files but missing from STATUS.md**: [list]
- **Stories in STATUS.md but missing from files**: [list]
- **Point mismatches**: [list]
- **Invalid dependencies**: [list]
- **Phase mismatches**: [list]
```

### Step 6: Traceability Coverage (V-Model Right Side)

If technical designs exist (`docs/technical-designs/`), check their traceability matrices:

1. For each requirement in scope, check: is there at least one test case that verifies it?
2. For each test case, check: does it trace back to a requirement?
3. For each story's acceptance criterion, check: is there a corresponding test?

Present:

```
## Traceability Coverage

**Requirements with tests**: N/N (N%)
**Requirements WITHOUT tests**: [list REQ-XXX-NNN]
**AC scenarios with tests**: N/N (N%)
**AC scenarios WITHOUT tests**: [list US-XXX-NNN Scenario N]
**Orphan tests** (no requirement link): [list test names]
```

If no TDs exist yet, skip this step and note: "Traceability audit requires technical designs. Run `/design` first."

### Step 7: Overall Report

Produce a final summary:

```
## Audit Report

**Overall Health**: Good / Needs Attention / Critical

### Requirements
- Score: N% average across N requirements
- Critical issues: N
- Coverage gaps: N

### Stories
- Score: N% average across N stories
- Sprint-ready: N stories
- Needs revision: N stories
- Blocked: N stories

### Estimates
- Estimated: N / N stories
- Total points: ~N
- Decomposition needed: N stories

### Tracking
- Consistency issues: N

### Recommended Actions
1. [Highest priority fix — e.g., "Fix 5 Critical requirement issues in audio system"]
2. [Next priority — e.g., "Revise 12 stories scoring below 70%"]
3. [Next — e.g., "Estimate 8 unpointed stories"]
4. [Next — e.g., "Decompose 3 stories over 8 points"]
```

### Step 8: Offer Fixes

Ask the PO: **"Want me to fix the issues found? I can:"**
- **Fix requirements**: Rewrite flagged requirements using the business-requirements-analyst
- **Fix stories**: Revise low-scoring stories using the agile-story-architect
- **Update estimates**: Apply corrected estimates using the story-estimator
- **Fix tracking**: Update STATUS.md to resolve consistency issues

Each fix category produces a separate commit.

### Step 9: Offer Pipeline Handoff

After fixes are applied, ask: **"Want me to drive the fixed artifacts forward through the remaining pipeline stages? This will run `/pipeline` on the audited scope to take everything from its current state to implementation-ready."**

If approved, invoke `/pipeline` with the same scope, which will:
- Verify fixed requirements (Stage 2)
- Create stories from any requirements that don't have them yet (Stage 3)
- Estimate and audit stories (Stage 4)
- Produce technical designs (Stage 5)
- Produce implementation plans (Stage 6)

Each stage has its own PO approval gate and produces separate commits.

## Rules

- **Read-only first.** The audit itself does not modify any files. Changes only happen if the PO approves fixes in Step 7.
- **Subsystem by subsystem.** When auditing "all", process one subsystem at a time to keep output manageable. Present results incrementally.
- **Calibrate to existing work.** The project may have conventions that predate this framework. Flag deviations but don't assume they're wrong — ask the PO.
- **Be constructive.** This isn't a blame exercise. The goal is to bring existing artifacts up to quality standards for the pipeline to work smoothly.
