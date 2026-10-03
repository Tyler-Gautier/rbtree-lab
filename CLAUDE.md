# rbtree-lab: project rules

## Commands
- Build & unit tests: ‘make test‘
- Sanitizers: ‘make asan‘ Valgrind: ‘make memcheck‘
- A change is DONE only when all three pass. Always run them; show output.

## Hard constraints
- NEVER modify include/rbtree.h. It is the graded contract.
- All heap allocation in src/ goes through rb_malloc/rb_free
(tests/fault_alloc.h). Direct malloc/free in src/ is a defect.
- Any allocation may fail. Every failure path must unwind completely:
no leaks, tree left exactly as before the call, documented error code.
- NEVER weaken, skip, or delete a test to make the suite pass. If a test
looks wrong, stop and explain why instead.

## Style
- C23. -Wall -Wextra -Werror must stay clean. No VLAs.
- Error handling: goto-cleanup pattern for multi-allocation functions.
- Prefer the smallest diff that passes. Do not refactor unrelated code.
- Every non-obvious loop gets a one-line invariant comment.

## Workflow
- For any multi-file or algorithmic change: propose a plan and wait for
approval before editing.
- Commit only from a green state; message format "M<n>: <what>".

## Code map
- include/rbtree.h fixed public API: Provided in Appendix A of CS370-HW2.PDF
- src/rbtree.c: My implementation of rbtree.h's definition of an rbtree
- src/pool.c: A slab pool for rb_create_pooled
- tests/fault_alloc.c: Fault-injector Allocator; Mutation 1
- tests/fuzz.c: Randomized stress test with a custom Sorted List for storing held key/value pairs
- tests/test_rbtree.c: Tests for rbtree.c

## Prompt Log
- After each user prompt, append a ## <date> entry to PROMPTLOG.md with the prompt and a brief summary of the response.