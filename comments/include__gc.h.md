# Comments extracted from `include/gc.h`

Source: `include/gc.h`

## Comment 1

How many globals between automatic threshold sweeps.

## Comment 2

XF automatic garbage collection / runtime cleanup layer.

v1 design rule:
  - never mutate refcounts directly
  - never force-free values
  - only release VM-owned transient roots through xf_value_release()
  - globals/user variables stay alive until overwritten, VM reset, or rip

## Comment 3

Collect transient VM-owned values after an execution boundary.

Safe places to call:
  - after each REPL command
  - after file execution finishes
  - after runtime error cleanup

Unsafe idea:
  - calling this in the middle of opcode execution unless the VM stack/root
    model is explicitly designed for it.

## Comment 4

Same as xf_gc_collect(), but returns simple counters for debugging.
You can leave this unimplemented at first or make collect() call collect_stats().

## Comment 5

Full VM teardown helper.

This is for interpreter shutdown / reset, not normal automatic collection.
It may release globals, rules, patterns, chunks, and runtime-owned state.

## Comment 6

XF_GC_H

## Comment 7

ifndef XF_GC_H
define XF_GC_H

## Comment 8

include "vm.h"
include <stddef.h>
include <stdint.h>
include <stdbool.h>

## Comment 9

define XF_GC_GLOBAL_THRESHOLD 64

## Comment 10

endif /* XF_GC_H */
