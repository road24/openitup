---
description: "Testing patterns and conventions"
globs:
  - "test/**/*.cpp"
  - "test/fixtures/**"
---

# Testing Standards

When creating or modifying test files:

## Framework
- Google Test (GTest) via CMake FetchContent
- Test files: `test/test_<module>.cpp`
- Fixtures: `test/fixtures/`

## Three-Tier Structure
1. **Unit tests**: Pure logic, no SDL. Test math, parsing, data structures, algorithms.
2. **Integration tests** (`test_integration.cpp`): Generate temp fixtures at runtime, render to offscreen targets, verify pixel values or output state.
3. **Regression tests** (`test_regression.cpp`): Load committed fixtures from `test/fixtures/`, render at specific states, compare pixel-by-pixel against reference PNGs.

## Naming
- Test suites: `PascalCase` matching the module (e.g., `KeyframeInterpolation`, `TextureCache`)
- Test cases: descriptive names (e.g., `LerpReturnsStartAtZero`, `HandlesEmptyInput`)

## Fixture Rules
- Committed fixtures go in `test/fixtures/` (tracked by git)
- Reference PNGs for regression tests go in `test/fixtures/reference/`
- Pixel comparison tolerance: ≤ 2 per channel
- Temporary fixtures generated at runtime go in `/tmp` — never committed

## Coverage Expectations
- Every acceptance criterion maps to at least one test case
- Edge cases from story AC Scenario 2+ must have explicit tests
- No story marked DONE unless its tests pass across all applicable tiers
