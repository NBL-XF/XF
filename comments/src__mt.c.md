# Comments extracted from `src/mt.c`

Source: `src/mt.c`

## Comment 1

============================================================
Internal forward declarations
============================================================

## Comment 2

============================================================
1.  Globals readers-writer lock
============================================================

## Comment 3

============================================================
2.  Chase-Lev work-stealing deque

 Reference: "Dynamic Circular Work-Stealing Deque"
            Chase & Lev, SPAA 2005.

 Indices never wrap; capacity must be power-of-two so (idx & mask)
 gives the slot.  We don't grow the deque — if it fills, push returns
 false and the caller falls back to synchronous execution.
============================================================

## Comment 4

cap must be a power of two >= 2.

## Comment 5

Drain any remaining items (should be zero at shutdown).

## Comment 6

full

## Comment 7

Empty: undo the decrement.

## Comment 8

Last element — race with stealers.

## Comment 9

A stealer got it first.

## Comment 10

empty

## Comment 11

lost the race

## Comment 12

============================================================
3.  Work item allocation / free
============================================================

## Comment 13

Release retained fn and args.

## Comment 14

xf_fn_release is declared in value.h; include transitively
through vm.h.  The cast avoids a direct value.h include here.

## Comment 15

result is consumed by xf_pool_await; caller releases it.

## Comment 16

============================================================
4.  Work execution (called by a worker thread)
============================================================

## Comment 17

vm_feed_record is the single-threaded path; each worker VM is
isolated (private stack/frames/rec), so no lock is needed here.
Global accesses inside the bytecode go through vm_global_read /
vm_global_write which take the pool's rwlock.

## Comment 18

Drain any leftover stack values — same pattern as main.c.

## Comment 19

vm_call_function_chunk declared in vm.h, included via mt.h → value.h chain

## Comment 20

Store result into vm->tasks[task_slot] and into w->result.

## Comment 21

ownership transferred; caller releases

## Comment 22

Signal completion.

## Comment 23

============================================================
5.  Steal from any other worker (load balancing)
============================================================

## Comment 24

Update pending counts.

## Comment 25

============================================================
6.  Worker thread main loop
============================================================

## Comment 26

GC safe point — check before doing any work.

## Comment 27

Shutdown check.

## Comment 28

1. Try own deque first.

## Comment 29

2. Try stealing.

## Comment 30

3. Nothing found — park until woken by a push or shutdown.

## Comment 31

Double-check under lock to avoid lost wakeup.

## Comment 32

============================================================
7.  vm_clone / vm_clone_free
============================================================

## Comment 33

Zero the whole struct first.

## Comment 34

Share read-only state from coordinator.

## Comment 35

protected by pool->gl

## Comment 36

read-only after compile

## Comment 37

Rec context defaults — same as vm_init.

## Comment 38

Private stack and frames: already zero from calloc.

## Comment 39

Release private stack.

## Comment 40

Release private frames.

## Comment 41

Rec buffers are private — free them.

## Comment 42

Do NOT touch globals/rules/chunks — those belong to the coordinator.

## Comment 43

============================================================
8.  Pool create / destroy
============================================================

## Comment 44

Wire coordinator so global ops go through the lock.

## Comment 45

Clean up previously initialised workers.

## Comment 46

Drain all pending work first.

## Comment 47

Signal shutdown and wake everyone.

## Comment 48

============================================================
9.  Submit helpers
============================================================

## Comment 49

Load-balanced dispatch: find the worker with the fewest pending items.
Falls back to round-robin if all workers are equally loaded.

## Comment 50

Deque full — execute synchronously in the coordinator.

## Comment 51

Wake a parked worker if any.

## Comment 52

Retain fn.

## Comment 53

Deep-copy and retain args.

## Comment 54

Fallback: execute synchronously.

## Comment 55

Scan all workers' deques for the matching work item to get done_cv.

## Comment 56

In practice, for OP_JOIN we find the item quickly because it was the
most recently pushed item.  For a more robust solution, keep a side
table of task_slot→work_item pointers; omitted here for clarity.

## Comment 57

Simple poll with yield — adequate for the common case.
A production implementation would maintain a slot→work_t map.

## Comment 58

Yield so the worker can run.

## Comment 59

Busy-wait until all workers have no pending items.

## Comment 60

============================================================
10.  GC safe-point barrier
============================================================

## Comment 61

Signal workers to stop at their next safe point.

## Comment 62

Wake any parked workers so they can reach the safe point check.

## Comment 63

Wait until all running workers have parked at the GC point.

## Comment 64

All workers are stopped — safe to collect.

## Comment 65

Release workers.

## Comment 66

Reached a safe point — park here until GC is done.

## Comment 67

wake the GC coordinator

## Comment 68

============================================================
11.  vm.h integration: locked global accessors
============================================================

## Comment 69

Propagate new pointer to all worker clones.

## Comment 70

Sync count to workers.

## Comment 71

GC threshold (with pool-aware barrier).

## Comment 72

============================================================
12.  Parallel record loop
============================================================

## Comment 73

Single-threaded fallback.

## Comment 74

Wait for all records to finish before returning.

## Comment 75

if defined(__linux__) || defined(__CYGWIN__)
 define _GNU_SOURCE
endif

## Comment 76

include "../include/mt.h"
include "../include/vm.h"
include "../include/value.h"
include "../include/gc.h"
include "../include/simd.h"

## Comment 77

include <stdio.h>
include <stdlib.h>
include <string.h>
include <errno.h>
