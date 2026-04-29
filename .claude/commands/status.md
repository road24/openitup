---
description: "Quick project status check. Shows completion percentage, phase progress, and next priorities."
---

# Project Status

## Process

1. Read `docs/stories/STATUS.md` for story tracking data.
2. Read `docs/requirements/README.md` for requirement coverage totals.

3. Calculate and display:

```
## Project Health

**Stories**: X/N done (Y%)
**Story Points**: ~X of ~N completed
**Requirements**: X/N implemented

**Current Phase**: Phase N
  - X/Y stories done (Z%)
  - Key blockers: [if any]

**Next Ready Stories** (dependencies satisfied):
1. US-XXX-NNN — [Title] (N pts)
2. US-XXX-NNN — [Title] (N pts)
3. US-XXX-NNN — [Title] (N pts)
```

4. Keep the output to a single screen. This is a quick glance, not a full report.
