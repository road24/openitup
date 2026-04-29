---
description: "Run code review on current changes. Reports findings and pass/fail. Run after /test."
---

# Review

## Process

### 1. Gather Context

1. Read the current changes: `git diff` for unstaged, `git diff --cached` for staged
2. Identify which IP step and TD these changes relate to (from recent `/code` invocation or commit history)
3. Read the TD for design alignment checking

### 2. Run Review

Use the **code-reviewer** agent to review all changes:
- Check correctness, style, test coverage, design alignment
- Reference the project's conventions from `CLAUDE.md` and existing source

### 3. Report Results

**On pass (no Critical issues)**:
```
## Review: PASS ✅

**Verdict**: Approve

**Findings**:
- [Important/Minor notes if any]

**Positive notes**:
- [What's done well]
```
Then recommend: **"Review passes. Run `/commit` to finalize."**

**On fail (Critical issues found)**:
```
## Review: NEEDS CHANGES ⚠️

**Critical issues** (must fix):
- `file.cpp:NN` — [issue] → [fix]

**Important suggestions**:
- `file.cpp:NN` — [suggestion]

**Design alignment**:
- [Any deviations from the TD]
```
Then recommend based on findings:
- Code quality issues: **"Run `/code` to fix the critical issues, then `/compile` → `/test` → `/review` again."**
- Design deviations: **"The implementation deviates from the TD. Either run `/code` to align, or `/revise design` if the TD needs updating."**
- Scope creep detected: **"This change goes beyond the IP step scope. Consider `/revise plan` to adjust the implementation plan."**

## Rules

- **Report only.** Do not modify source files.
- **Be specific.** File, line, issue, fix.
- **Flag design deviations.** The TD is the contract. Deviations need explicit approval.
- **Flag scope creep.** If the change does more than the IP step specifies, call it out.
