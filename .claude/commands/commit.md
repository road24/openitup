---
description: "Stage changes, present diff to PO, and commit on approval. Run after /review passes."
---

# Commit

## Process

### 1. Identify Changes

Run `git status` and `git diff --stat` to see all modified/created files.

### 2. Stage Files

Stage specific files related to the current IP step:
- Use `git add <file>` for each file (never `git add -A` or `git add .`)
- Only stage files that are part of this step's implementation
- Do NOT stage unrelated changes

### 3. Present to PO

Show:
```
## Ready to Commit

**IP Step**: IP-XXX-NNN Step N — [Title]

**Files staged**:
- `src/path/file.h` (new)
- `src/path/file.cpp` (new)
- `test/test_module.cpp` (modified)
- `CMakeLists.txt` (modified)

**Diff summary**: +N lines, -N lines across N files

**Commit message**:
`feat(module): add ClassName with core interface`

**Build**: ✅  **Tests**: ✅  **Review**: ✅
```

Ask: **"Ready to commit?"**

### 4. On PO Approval

1. Create the commit with the expected message from the IP step
2. Report success

### 5. Update Tracking

After successful commit:
1. Check if this was the last step in the IP
   - If yes: update story status in `docs/stories/STATUS.md` from PLANNED to DONE
   - Report: **"Story US-XXX-NNN complete. STATUS.md updated."**
2. Report progress: **"Step N of M committed. N steps remaining."**
3. Recommend: **"Run `/code IP-XXX-NNN step N+1` to continue."**

### 6. On PO Rejection

If the PO says no:
- Ask what needs to change
- Recommend: **"Run `/code` to make the changes, then `/compile` → `/test` → `/review` → `/commit` again."**
- Do NOT unstage or discard changes — the PO may want minor tweaks

## Rules

- **PO approval required.** Never commit without explicit confirmation.
- **Specific staging.** Only stage files from this step. No stray files.
- **Conventional Commits.** Use the commit message format from the IP step.
- **Track progress.** Always report step N of M and update STATUS.md on completion.
