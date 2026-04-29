---
description: "Run the project build. Reports pass/fail with diagnostics. Run after /code."
---

# Compile

## Process

### 1. Read Build Commands

Read the project's `CLAUDE.md` for the build commands (e.g., cmake flags, build directory).

### 2. Build

Run the project's build:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug [project-specific flags from CLAUDE.md]
cmake --build build -j$(nproc)
```

### 3. Report Results

**On success**:
```
## Build: PASS ✅

Compiled successfully. No errors, no warnings.
```
Then recommend: **"Build passes. Run `/test` to verify."**

**On failure**:
```
## Build: FAIL ❌

**Errors**:
- `file.cpp:NN` — [error message]
- `file.h:NN` — [error message]

**Diagnosis**:
[Use the **build-engineer** agent to analyze the errors and suggest specific fixes]

**Recommended fix**: [concrete suggestion]
```
Then recommend: **"Build failed. Run `/code` to apply the fix, then `/compile` again."**

## Rules

- **Report only.** Do not modify source files. Do not fix the code.
- **Diagnose failures.** Use the build-engineer agent to interpret errors.
- **Include the full error.** Don't summarize — the PO needs to see exactly what failed.
