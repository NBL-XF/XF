# Comments extracted from `src/gc.c`

Version: `v1.0.3`

Source: `src/gc.c`

## Comment 1

============================================================
Trace flag
============================================================

## Comment 2

============================================================
Internal helpers
============================================================

## Comment 3

============================================================
Core sweep: stack + all frame locals

Called only between major events (after BEGIN/END, on
global-alloc threshold) — never while a chunk is executing.

Ownership rules:
  vm_pop() transfers the stack-slot's reference to the caller.
  Frame locals are owned by the frame; we release and zero them.
  We do NOT touch vm->globals — those have indefinite lifetime.
============================================================

## Comment 4

--- 1. Release orphaned stack values ---

## Comment 5

transfers ownership

## Comment 6

--- 2. Release locals in any lingering frames --- *

Under normal operation there should be no live frames here
(we only collect between top-level events). But defensive
cleanup prevents leaks if an error path left frames behind.

## Comment 7

============================================================
Auto-GC: threshold-based trigger

Called from vm_alloc_global() after each new global is
allocated. Fires when global_count crosses a multiple of
XF_GC_GLOBAL_THRESHOLD.

Safe to call only between chunk executions (same contract
as xf_gc_collect).
============================================================

## Comment 8

============================================================
VM teardown hook (called by vm_free, not a replacement)
============================================================

## Comment 9

Final sweep before vm_free does its own structured teardown.
Clears any stack/frame residue so vm_free only has to handle
globals, chunks, and record buffers — which it already does.

## Comment 10

include "../include/gc.h"
include "../include/value.h"
include "../include/vm.h"

## Comment 11

include <stdio.h>
include <string.h>
