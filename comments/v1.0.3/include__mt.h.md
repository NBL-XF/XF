# Comments extracted from `include/mt.h`

Version: `v1.0.3`

Source: `include/mt.h`

## Comment 1

mt.h — multithreading for the xf interpreter

Three orthogonal systems, all defined here:

  1. xf_pool_t       — work-stealing threadpool backing OP_SPAWN/OP_JOIN
                       and parallel record dispatch (vm_feed_record_mt).

  2. xf_globals_lock — readers-writer lock protecting vm->globals[].
                       All workers share one globals array; reads are
                       lock-free relative to each other, writes serialize.

  3. xf_obj_lock_t   — per-object spinlock embedded in xf_arr_t / xf_map_t
                       for safe concurrent mutation. The refcount is already
                       atomic; this only guards structural mutation
                       (push/pop/set/delete) that touches the backing array.

GC integration:
  xf_pool_gc_barrier() parks all workers at a safe point and then calls
  xf_gc_collect() on the coordinator VM before releasing them.
  Workers check pool->gc_requested at the top of their dispatch loops.

Load balancing:
  Each worker owns a Chase-Lev deque. The pool's dispatcher assigns
  incoming records to the least-loaded worker (min pending tasks).
  Workers steal from the tail of neighbours when their own deque is empty.

## Comment 2

Pull in value.h for the canonical type definitions.
mt.h must be included AFTER value.h and vm.h in translation units,
or just let this include handle it.

## Comment 3

============================================================
Forward declaration for VM (defined in vm.h)
============================================================

## Comment 4

============================================================
Tunables
============================================================

## Comment 5

Maximum number of worker threads the pool will ever spawn.

## Comment 6

Capacity of each worker's Chase-Lev deque (must be power of two).

## Comment 7

Number of steal attempts before a worker parks on the condition variable.

## Comment 8

============================================================
1.  Per-object spinlock  (embed one in xf_arr_t and xf_map_t)
============================================================

## Comment 9

ATOMIC_FLAG_INIT = unlocked

## Comment 10

Initialize before first use (or use XF_OBJ_LOCK_INIT as a struct literal).

## Comment 11

Spin-wait; back off with a compiler barrier to avoid hoisting.

## Comment 12

__builtin_ia32_pause() on x86/x86-64, otherwise a no-op fence.

## Comment 13

RAII-style helpers for C11 _cleanup_ (optional, GCC/Clang only).

## Comment 14

============================================================
2.  Globals readers-writer lock

 Wraps pthread_rwlock_t.  All OP_LOAD_GLOBAL callers take a
 read lock; OP_STORE_GLOBAL and vm_alloc_global take a write lock.
 The write lock is also required before realloc-ing vm->globals.
============================================================

## Comment 15

Read-lock: multiple readers allowed concurrently.

## Comment 16

Write-lock: exclusive; also used for realloc.

## Comment 17

============================================================
3.  Work item

 A single unit of work submitted to the pool.  Covers both
 record-level parallelism and OP_SPAWN tasks.
============================================================

## Comment 18

process one stdin record through all rules

## Comment 19

execute a user function (OP_SPAWN)

## Comment 20

Maximum bytes for an inline record copy (avoids malloc for short lines).

## Comment 21

---- XF_WORK_RECORD fields ----

## Comment 22

inline copy when short

## Comment 23

heap copy when long

## Comment 24

---- XF_WORK_SPAWN fields ----

## Comment 25

retained; released after execution

## Comment 26

heap-allocated retained copy

## Comment 27

index into vm->tasks[]

## Comment 28

---- completion ----

## Comment 29

0=pending 1=running 2=done

## Comment 30

written once by worker; read by JOIN

## Comment 31

============================================================
4.  Chase-Lev work-stealing deque (per worker)

 The owner pushes/pops from the bottom (index bottom).
 Thieves steal from the top (index top).
 All indices are monotonically increasing; capacity is a power of two.
============================================================

## Comment 32

ring buffer, size = cap

## Comment 33

must be power of two

## Comment 34

steal from here (shared)

## Comment 35

push/pop here (owner only)

## Comment 36

Owner-side push.  Returns false if the deque is full.

## Comment 37

Owner-side pop (LIFO, avoids contention with stealers).

## Comment 38

Thief-side steal (FIFO from top).

## Comment 39

============================================================
5.  Worker state
============================================================

## Comment 40

per-worker VM clone (private stack/frames/rec)

## Comment 41

back-pointer to the pool

## Comment 42

Load-balancing statistics (updated by owner, read by dispatcher).

## Comment 43

items currently in this worker's deque

## Comment 44

total work items completed

## Comment 45

============================================================
6.  Pool
============================================================

## Comment 46

Coordinator VM — owns globals[], chunks, rules, stdout.
Workers read globals under gl; never touch chunks outside their rec.

## Comment 47

Globals lock — shared by all workers and the coordinator.

## Comment 48

Park/wake workers when there is nothing to steal.

## Comment 49

number of currently parked workers

## Comment 50

GC safe-point barrier.

## Comment 51

workers that reached a safe point

## Comment 52

Shutdown flag.

## Comment 53

Round-robin / load-balanced dispatch cursor.

## Comment 54

============================================================
7.  Public API
============================================================

## Comment 55

xf_pool_create — allocate and start a thread pool.

  coord_vm   The primary VM.  Its globals[], rules[], and chunks are
             shared read-only with workers.  Must outlive the pool.
  nworkers   Number of worker threads to spawn (1..XF_MT_MAX_WORKERS).

Returns NULL on failure.

## Comment 56

xf_pool_destroy — drain all pending work, join all threads, free pool.

 Blocks until every queued item has been processed.

## Comment 57

xf_pool_submit_record — dispatch one stdin record to the least-loaded worker.

 Makes an internal copy of rec[0..len]; the caller may free/reuse the buffer
 immediately after this returns.

 Returns false if the pool is shutting down or out of memory.

## Comment 58

xf_pool_submit_spawn — submit a user function for OP_SPAWN.

 fn and args are retained by the pool; the caller releases its own refs.
 task_slot is written into the work item and into vm->tasks[].

 Returns false on failure.

## Comment 59

xf_pool_await — block until the task in the given slot is done.

 Returns the retained result value.  Caller must release it.
 If task_slot is out of range or never submitted, returns xf_val_null().

## Comment 60

xf_pool_drain — wait for all pending work to finish (no shutdown).

 Used between BEGIN and the record loop, and again before END.

## Comment 61

xf_pool_gc_barrier — coordinate a stop-the-world GC sweep.

 Parks all workers at their next safe point, calls xf_gc_collect on
 the coordinator VM, then releases all workers.

 Called by xf_gc_check_threshold when the pool is active.

## Comment 62

xf_pool_worker_check_gc — called inside each worker's dispatch loop.

 If gc_requested is set, the worker increments gc_parked, waits on
 gc_cv, then decrements gc_parked when released.

## Comment 63

============================================================
8.  vm.h integration helpers

 These are thin wrappers used by OP_LOAD_GLOBAL, OP_STORE_GLOBAL,
 vm_alloc_global, and vm_feed_record when a pool is present.
 When pool == NULL they fall back to the original unguarded paths.
============================================================

## Comment 64

vm_global_read — retained read of vm->globals[idx] under read-lock.

 Caller must release the returned value.

## Comment 65

vm_global_write — write to vm->globals[idx] under write-lock.

 Retains v, releases the old value.  Returns false if idx out of range.

## Comment 66

vm_global_alloc — allocate a new global slot under write-lock.

 May realloc vm->globals; write-lock prevents races during growth.
 Returns the new slot index, or UINT32_MAX on failure.

## Comment 67

============================================================
9.  vm_clone / vm_clone_free

 Each worker needs its own stack, frame array, and rec context,
 but shares globals[], rules[], and chunks with the coordinator.

 vm_clone() allocates a shallow copy:
   - Zeroed stack, frame array, rec buffers.
   - globals, global_count/cap: pointer shared (protected by gl).
   - rules, rule_count, patterns: pointer shared (read-only after compile).
   - begin_chunk, end_chunk: shared (read-only).
   - has_pool pointer set to pool.

 vm_clone_free() releases stack/frame/rec only; never touches globals
 or chunks (those belong to the coordinator).
============================================================

## Comment 68

============================================================
10.  Parallel record loop replacement

 Drop-in for the fgets loop in main.c / xf_run_program.
 Reads stdin and dispatches each record to the pool.
 Blocks until all records have been processed.
============================================================

## Comment 69

XF_MT_H

## Comment 70

pragma once
ifndef XF_MT_H
define XF_MT_H

## Comment 71

if defined(__linux__) || defined(__CYGWIN__)
 define _GNU_SOURCE
endif

## Comment 72

include <stddef.h>
include <stdint.h>
include <stdbool.h>
include <stdatomic.h>
include <stdio.h>
include <pthread.h>

## Comment 73

include "value.h"

## Comment 74

ifndef XF_MT_MAX_WORKERS
 define XF_MT_MAX_WORKERS  64
endif

## Comment 75

ifndef XF_MT_DEQUE_CAP
 define XF_MT_DEQUE_CAP    256
endif

## Comment 76

ifndef XF_MT_STEAL_SPINS
 define XF_MT_STEAL_SPINS  32
endif

## Comment 77

define XF_OBJ_LOCK_INIT  { ATOMIC_FLAG_INIT }

## Comment 78

if defined(__x86_64__) || defined(__i386__)

## Comment 79

else

## Comment 80

endif

## Comment 81

define XF_OBJ_SCOPED_LOCK(lk) \

## Comment 82

define XF_WORK_REC_INLINE  512

## Comment 83

define XF_WORK_PENDING  0
define XF_WORK_RUNNING  1
define XF_WORK_DONE     2

## Comment 84

endif /* XF_MT_H */
