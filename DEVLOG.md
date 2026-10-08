# DEVLOG

## 2026-10-07 (week 1, evening 2)
STATE: M0 committed. fault_alloc.h/.c still placeholders; rb_malloc/rb_free
 still static in src/rbtree.c.
DID: Planning Mutation 1. Settling the fault injector's contract before
 designing the sweep harness.
DECIDED: TODO(me): fill in each decision below with what I chose and why.
 - Q1 one-shot vs sticky failure: TODO
 - Q2 does the failed allocation count toward fault_alloc_total(): TODO
 - Q3 does arm/disarm reset the total, or is it cumulative: TODO
 - Q4 how the sweep detects the fault fired / when it stops: TODO
LEARNED:
NEXT FIRST STEP:
OPEN: harness design: fixed scenario contents, rb_create inside or outside
 the sweep, how to snapshot "before" for the unchanged check.
