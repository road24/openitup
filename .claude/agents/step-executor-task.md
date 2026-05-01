# Step Executor Task

Execute IP-AST-018 step 1.

Read the implementation plan at /home/jose/Documents/development/openitup/docs/implementation-plans/IP-AST-018.md.

Read /home/jose/Documents/development/openitup/CLAUDE.md for build and test commands.

Implement step 1: Add LRU tracking to TextureCache.

You MUST:
1. Read the existing files mentioned in the IP step
2. Make the code changes specified
3. Write or update tests as specified
4. Run the build: `cmake --build build -j$(nproc)`
5. Fix any compilation errors
6. Run the tests: `cd build && ctest --output-on-failure`
7. Fix any test failures
8. Create a git commit with the exact message from the IP step

You MUST create a git commit before you return. If you cannot complete the step successfully, report the blocker and stop.
