---
description: "C++ coding standards for source files"
globs:
  - "src/**/*.cpp"
  - "src/**/*.h"
  - "src/**/*.hpp"
---

# C++ Source Code Standards

When creating or modifying C++ source files:

## File Organization
- Headers (.h) contain declarations, minimal includes, and inline definitions
- Source (.cpp) contains implementations; first include is the corresponding .h
- Use `#pragma once` for include guards
- Include order: project headers, then third-party, then standard library (separated by blank lines)

## Naming Conventions
- Classes/structs: `PascalCase`
- Methods/functions: `snake_case`
- Member variables: `snake_case` with trailing underscore (e.g., `renderer_`)
- Constants/enums: `UPPER_SNAKE_CASE`
- Namespaces: `lowercase`
- File names: `snake_case.h` / `snake_case.cpp`

## Memory and Ownership
- `std::unique_ptr` for single ownership
- `std::shared_ptr` only when shared ownership is genuinely needed
- No raw `new`/`delete` — use RAII throughout
- Prefer value semantics where practical
- Use `const&` for read-only parameters, move semantics for transfers

## Error Handling
- Use spdlog for logging (project dependency)
- Critical errors: log at error level and throw or return error code
- Non-critical: log warning and continue (graceful degradation)

## Testing Requirement
- Every new public function or behavior must have corresponding tests
- Follow the three-tier structure: unit tests for pure logic, integration tests for SDL-dependent code, regression tests for visual output
- New test files go in `test/` with naming pattern `test_<module>.cpp`

## Build System
- New source files must be added to `CMakeLists.txt`
- New dependencies use CMake FetchContent
