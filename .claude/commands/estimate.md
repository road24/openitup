---
description: "Estimate story points for one or more user stories. Provide a story ID (US-XXX-NNN), a range (US-AUD-001 to US-AUD-005), or a subsystem name."
---

# Estimate Stories

**Target**: $ARGUMENTS

## Process

1. **Locate the stories**: Read the specified story/stories from `docs/stories/`. If a subsystem name is given, find the matching file.

2. **Calibrate**: Read `docs/stories/STATUS.md` to find completed stories and their point values for calibration.

3. **Run estimation**: Use the **story-estimator** agent to:
   - Assign Fibonacci story points to each story
   - Compare against similar completed stories
   - Flag stories > 8 points for decomposition
   - Flag low-confidence estimates needing spikes

4. **Present the report**: Show the estimation table with points, confidence, and rationale.

5. **Offer to update**: Ask the PO: **"Should I update the story files and STATUS.md with these estimates?"**
