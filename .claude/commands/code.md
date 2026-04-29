---
description: "Write code and tests for one implementation plan step. Does NOT compile, test, or commit. Provide an IP reference and step number (e.g., 'IP-ENG-001 step 1')."
---

# Code

**Target**: $ARGUMENTS

## Process

### 1. Load Context

1. Parse the target to identify the IP document and step number
2. Read the implementation plan from `docs/implementation-plans/IP-XXX-NNN.md`
3. Read the specific step: files to create/modify, what to implement, tests to write
4. Read the Technical Design Document (TD) referenced by the IP
5. Read the project's `CLAUDE.md` for coding conventions
6. Read any existing files listed in the step's "Modify" entries
7. Read existing test files if the step adds to them

### 2. Write Code

- Create new files listed under "Create"
- Modify existing files listed under "Modify"
- Follow the project's coding conventions exactly (read existing source for style)
- Reference the TD's interface sketches for signatures and ownership
- Stay within the step's scope — do NOT implement code from other steps

### 3. Write Tests

- Add test cases to the specified test file
- Follow the project's test patterns (read existing tests)
- Cover the acceptance criteria scenarios mapped to this step
- Tests must be deterministic and self-contained

### 4. Report

Present a summary of what was written:

```
## Code Written

**IP Step**: IP-XXX-NNN Step N — [Title]

**Files created**:
- `src/path/file.h` — [what it contains]
- `src/path/file.cpp` — [what it implements]

**Files modified**:
- `src/path/existing.h` — [what changed]
- `CMakeLists.txt` — [what was added]

**Tests written**:
- `test/test_module.cpp` — [N test cases: list names]
```

Then ask: **"Code written. Run `/compile` to build."**

### 5. Scope Discovery

If while reading the codebase you discover that:
- The IP step is missing something the TD requires → recommend: `/revise plan`
- The TD doesn't account for an existing component → recommend: `/revise design`
- The story's acceptance criteria don't cover a necessary behavior → recommend: `/revise story <US-id>`
- The feature needs a capability not in any requirement → recommend: `/revise requirement <REQ-id>`

**Always report scope discoveries to the PO before proceeding.** Don't silently expand scope.

## Rules

- **Code and tests only.** Do not compile, run tests, review, or commit.
- **One step at a time.** Never code multiple IP steps in one pass.
- **Read before writing.** Match existing style exactly.
- **No scope creep.** Flag discoveries, don't fix them.
