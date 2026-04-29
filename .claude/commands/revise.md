---
description: "Feed discoveries from any stage back upstream to update artifacts, then cascade changes forward. Use when coding, testing, design, or planning reveals that an upstream artifact (requirement, story, design, plan) needs to change. Provide the target: 'requirement <REQ-id>', 'story <US-id>', 'design <TD-id>', or 'plan <IP-id>', plus a description of what was learned."
---

# Revise

**Target**: $ARGUMENTS

This is the **feedback loop** of the SDLC pipeline. At any stage, you may discover that an upstream artifact is incomplete, incorrect, or missing something. This skill updates that artifact with the new information, then cascades the changes forward through all downstream stages.

## Process

### 1. Capture the Discovery

Document what was learned and where:

```
## Discovery

**Found during**: [Stage where the issue was discovered — coding, testing, design, planning, review]
**Affects**: [Which upstream artifact needs to change]
**What was learned**: [Specific insight — what's missing, wrong, or incomplete]
**Evidence**: [Code, test failure, design conflict, or review finding that revealed this]
```

### 2. Assess Impact

Determine the scope of the change:

```
## Impact Assessment

**Upstream change needed**:
- [Artifact ID] — [What specifically needs to change]

**Downstream artifacts affected**:
- [ ] Stories need update? [Yes/No — which ones]
- [ ] Estimates change? [Yes/No — point increase/decrease]
- [ ] Technical Design needs update? [Yes/No — which sections]
- [ ] Implementation Plan needs update? [Yes/No — which steps]
- [ ] Code already written needs update? [Yes/No — which files]

**Scope change?**: [Does this add new requirements, stories, or increase points?]
```

### 3. PO Decision Point

Present the impact and ask for direction:

**If scope increases** (new requirements, new stories, more points):
> **"This discovery changes the project scope: [describe]. Current work can continue as-is, or we can update the upstream artifacts. What do you prefer?"**

Options:
1. **Update upstream + cascade** — revise the artifact and regenerate downstream
2. **Note for later** — log the discovery as a future story/requirement, continue current work
3. **Adjust scope** — update upstream but trade off something else to keep total scope stable

**If scope stays the same** (clarification or correction):
> **"This is a clarification to [artifact]. No scope change. Should I update it and cascade the changes?"**

### 4. Update the Upstream Artifact

Based on the target:

#### `requirement <REQ-id>`:
- Use **business-requirements-analyst** to update the specific requirement
- Preserve existing wording where possible — surgical edit, not rewrite
- Add the new acceptance criteria, constraints, or clarifications discovered
- Then use **business-requirements-verifier** to re-verify

#### `story <US-id>`:
- Use **agile-story-architect** to update the specific story
- Add missing acceptance criteria, adjust scope, or split if needed
- Then use **story-estimator** to re-estimate if scope changed
- Then use **agile-requirements-auditor** to re-audit

#### `design <TD-id>`:
- Use **technical-architect** to update the specific TD section
- Add missing interfaces, adjust class designs, update file plans
- The architect reads the current code state to inform the revision

#### `plan <IP-id>`:
- Use **technical-lead** to update the specific IP steps
- Add missing steps, reorder, or adjust existing steps
- Preserve completed steps — only revise steps not yet committed

Each update is committed separately with a clear message:
`fix(docs): revise [artifact ID] — [what changed and why]`

### 5. Cascade Forward

After the upstream artifact is updated, regenerate only the affected downstream artifacts:

| If you revised... | Then regenerate... |
|---|---|
| Requirement | → re-verify → update stories → re-estimate → re-audit → update TD → update IP |
| Story | → re-estimate → re-audit → update TD → update IP |
| Design (TD) | → update IP |
| Plan (IP) | → (nothing — just resume the dev loop) |

**Only regenerate what changed.** If a story update doesn't affect the technical design, skip the TD update. The cascade is smart, not mechanical.

Each regenerated artifact is committed separately.

### 6. Resume Work

After the cascade completes:

```
## Revision Complete

**What was revised**: [Artifact ID — what changed]
**Downstream updates**: [List of regenerated artifacts]
**Current code state**: [Stashed / Still valid / Needs update]

**Next step**: [Where to resume — e.g., "/code IP-XXX-NNN step N" or "/compile"]
```

If code was already written for the affected IP step:
- If the revision doesn't affect the code: resume from where you were
- If the revision changes the code needed: recommend `/code` to update

## Rules

- **Preserve work.** Stash code changes — they're evidence of what was learned and may still be useful.
- **Surgical updates.** Modify only the specific parts of upstream artifacts that need to change. Don't rewrite from scratch.
- **PO approval for scope changes.** Adding requirements, stories, or points requires explicit PO approval.
- **Commit each update separately.** The PO can revert any individual revision without losing the rest.
- **Smart cascade.** Only regenerate downstream artifacts that are actually affected by the change.
- **This is normal.** Feedback loops are how good software gets built. Every revision is a sign the team is learning.
