---
description: "Generate a sprint review summary showing progress, velocity, and next priorities."
---

# Sprint Review

## Process

1. **Gather data**: Read `docs/stories/STATUS.md` and `docs/requirements/README.md` for current project state.

2. **Run review**: Use the **sprint-manager** agent to produce:
   - Summary of recently completed stories
   - Stories currently in progress
   - Velocity trend (points completed per sprint, if enough data)
   - Blockers or dependency chains preventing progress
   - Recommended focus for next sprint

3. **Present the review** in a concise format suitable for a quick standup.

4. **Offer planning**: Ask: **"Want me to plan the next sprint?"** If yes, use the sprint-manager to produce a sprint backlog.
