---
description: "Run acceptance testing against a story's Gherkin criteria. The formal gate before marking a story DONE. Run after /integrate passes."
---

# Acceptance Testing

## Process

### 1. Identify Stories

Determine which stories to accept (from context, PO input, or the just-completed IP).

### 2. Load Acceptance Criteria

For each story, read its acceptance criteria from `docs/stories/XX-subsystem.md`. These are the Gherkin Given/When/Then scenarios that define "done."

### 3. Map Criteria to Tests

Use the **qa-test-engineer** agent to:
1. Map each AC scenario to a specific test case in the test suite
2. Identify any AC scenarios that lack automated test coverage
3. For uncovered scenarios, determine if they can be verified manually or need new tests

### 4. Execute Verification

For each AC scenario:

**If automated test exists**:
- Run the specific test: `cd build && ctest --output-on-failure -R "TestName"`
- Record pass/fail

**If no automated test**:
- Describe how to manually verify (e.g., "run the BGA player with this fixture and check output")
- Ask PO to confirm manual verification, or recommend writing the missing test

### 5. Traceability Check

Cross-reference the story's parent requirement:
- Read `REQ-XXX-NNN` from the story's References field
- Verify that the requirement's acceptance criteria are also satisfied
- Flag any requirement AC not covered by this story's tests

### 6. Report

```
## Acceptance Test Report

**Story**: US-XXX-NNN — [Title]
**Requirement**: REQ-XXX-NNN

### Acceptance Criteria Results

| # | Scenario | Test | Result |
|---|----------|------|--------|
| 1 | [Happy path title] | test_module::TestCase | ✅ PASS |
| 2 | [Edge case title] | test_module::EdgeCase | ✅ PASS |
| 3 | [Negative path] | (manual) | ⏳ PO to verify |

### Traceability

| Requirement AC | Story AC | Test | Covered? |
|----------------|----------|------|----------|
| [REQ AC 1] | Scenario 1 | TestCase1 | ✅ |
| [REQ AC 2] | Scenario 2 | TestCase2 | ✅ |
| [REQ AC 3] | — | — | ❌ Gap |

### Verdict: PASS ✅ / FAIL ❌ / PARTIAL ⚠️

**Coverage**: N/N AC scenarios verified (N%)
**Gaps**: [List any uncovered scenarios]
```

### 7. Decision

**On PASS (100% coverage)**:
> **"All acceptance criteria verified. Mark US-XXX-NNN as DONE in STATUS.md?"**

If PO approves, update `docs/stories/STATUS.md` and commit.

**On PARTIAL (some gaps)**:
> **"N of M acceptance criteria verified. N gaps remain: [list]. Options:"**
> 1. Write missing tests → `/code` to add them → `/compile` → `/test` → `/accept` again
> 2. Accept with known gaps (PO acknowledges the risk)
> 3. `/revise story <US-id>` if the AC themselves need to change

**On FAIL**:
> **"N acceptance criteria failed: [list]. The implementation doesn't match the story. Run `/code` to fix, then cycle through `/compile` → `/test` → `/integrate` → `/accept` again."**

## Rules

- **Every AC scenario must be checked.** No silent skipping.
- **Trace to requirements.** Acceptance testing validates stories AND their parent requirements.
- **PO confirms manual checks.** Automated tests are preferred but some things need human eyes.
- **This is the final gate.** A story is not DONE until /accept passes.
