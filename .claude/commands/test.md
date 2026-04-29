---
description: "Run the project test suite. Reports pass/fail per test with diagnostics. Run after /compile."
---

# Test

## Process

### 1. Read Test Commands

Read the project's `CLAUDE.md` for the test commands.

### 2. Run Tests

```bash
cd build && ctest --output-on-failure
```

### 3. Report Results

**On all pass**:
```
## Tests: PASS ✅

All N tests passed.

| Suite | Tests | Status |
|-------|-------|--------|
| [SuiteName] | N | ✅ |
```
Then recommend: **"Tests pass. Run `/review` for code review."**

**On failure**:
```
## Tests: FAIL ❌

**N/M tests passed.** N failures:

| Test | Suite | Error |
|------|-------|-------|
| [TestName] | [Suite] | [failure message] |

**Diagnosis**:
[Use the **build-engineer** agent to analyze test failures and suggest fixes]

**Is this a regression?** [Yes — pre-existing test broke / No — new test failing]

**Recommended fix**: [concrete suggestion]
```
Then recommend:
- If regression: **"A pre-existing test broke. Run `/code` to fix the regression, then `/compile` → `/test` again."**
- If new test failing: **"New test failing. Run `/code` to fix the implementation or the test, then `/compile` → `/test` again."**

## Rules

- **Report only.** Do not modify source files or tests.
- **Diagnose failures.** Use the build-engineer agent to interpret test output.
- **Distinguish regressions from new failures.** This affects the fix strategy.
- **Run the full suite.** Not just new tests — catch regressions.
