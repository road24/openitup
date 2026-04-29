---
description: "Run full integration verification after all commits for a story are done. Validates cross-component behavior before marking a story complete. Run after the last /commit for an IP."
---

# Integration Verification

## Process

### 1. Identify Scope

1. Determine which IP just completed (from context or PO input)
2. Read the IP to identify all stories it covers
3. Read those stories' acceptance criteria for integration-level scenarios

### 2. Full Build

Run a clean build to ensure everything compiles together:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug [project-specific flags from CLAUDE.md]
cmake --build build -j$(nproc)
```

If the build fails, use the **build-engineer** agent to diagnose. Recommend `/code` to fix.

### 3. Full Test Suite

Run the complete test suite — all tiers:
```bash
cd build && ctest --output-on-failure
```

This is not just the tests from the current story — it's every test in the project. The goal is to catch cross-component regressions that per-step testing might miss.

### 4. Integration-Specific Checks

Beyond the test suite, verify:
- **Cross-subsystem interfaces**: Do the new components integrate correctly with existing ones?
- **Data flow**: Does data pass correctly through the full pipeline described in the TD?
- **Resource management**: No leaks, no dangling references across component boundaries?
- **Build system**: Are all new files properly linked? No orphaned sources?

Use the **build-engineer** agent to run any additional verification commands from CLAUDE.md (e.g., specific integration test targets, sanitizers, static analysis).

### 5. Report

```
## Integration Verification

**Stories**: US-XXX-NNN, US-XXX-NNN
**IP**: IP-XXX-NNN (all N steps committed)

**Build**: ✅ / ❌
**Test Suite**: N/N passed ✅ / ❌ (N failures)
**Cross-component**: [Observations]

**Verdict**: PASS ✅ / FAIL ❌
```

**On pass**: Recommend **"Integration verified. Run `/accept` to validate against acceptance criteria."**

**On fail**: Report failures, diagnose with build-engineer, recommend `/code` to fix → `/compile` → `/test` → `/integrate` again.

## Rules

- **Run after all IP steps are committed.** Not after each step — after the full story implementation.
- **Full suite, not partial.** The whole test suite runs. Regressions are blockers.
- **Don't mark stories DONE yet.** That happens after `/accept`.
