---
description: "Verify a business requirement or set of requirements. Provide a requirement ID (REQ-XXX-NNN), a range, or a subsystem filename."
---

# Refine Requirements

**Target**: $ARGUMENTS

## Process

1. **Locate the requirements**: Read the specified requirement(s) from `docs/requirements/`. If a subsystem name is given (e.g., "audio" or "input"), find the matching file.

2. **Run verification**: Use the **business-requirements-verifier** agent to validate the requirements against SMART criteria, check for cross-subsystem conflicts, and identify coverage gaps.

3. **Present the report**: Show the SMART scorecard, issues found, and recommended fixes.

4. **Offer to fix**: If issues were found, ask the PO: **"Should I apply these suggested fixes to the requirements?"**

5. **If fixes are approved**: Use the **business-requirements-analyst** agent to rewrite the flagged requirements incorporating the verifier's feedback. Then re-verify to confirm the fixes resolved the issues.
