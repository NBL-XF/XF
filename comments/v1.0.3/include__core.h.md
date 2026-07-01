# Comments extracted from `include/core.h`

Version: `v1.0.3`

Source: `include/core.h`

## Comment 1

============================================================
xf core module library

Registers the following namespace hierarchy as a global:

  core                  (module)
  core.math             sin cos tan asin acos atan atan2
                        sqrt pow exp log log2 log10
                        abs floor ceil round int
                        min max clamp
                        rand srand
                        PI  E  INF
  core.string           len upper lower trim ltrim rtrim
                        substr index contains
                        starts_with ends_with
                        replace replace_all
                        repeat reverse
                        sprintf
  core.system           exec exit time env

Usage:
  core_register(syms);   // called once at startup

Then in xf scripts:
  num r = core.math.sqrt(9)            # 3
  str s = core.string.upper("hello")   # HELLO
  num t = core.system.time()           # unix timestamp
============================================================

## Comment 2

── XF-function execution callback ─────────────────────────────
core.process.run and core.ds.stream need to call XF-language
(non-native) functions from pthreads.  The interpreter registers
a callback here so core.c can execute XF fns without a hard
dependency on interp.h.

vm     — the shared VM pointer (void* to avoid pulling in vm.h)
syms   — the caller's SymTable pointer; worker thread copies its
          global scope so core.*, user globals etc. are visible
fn     — the XF or native function to call
args   — argument values (owned by caller)
argc   — argument count
Returns an owned xf_Value result.
----------------------------------------------------------------

## Comment 3

XF_CORE_H

## Comment 4

ifndef XF_CORE_H
define XF_CORE_H

## Comment 5

include "value.h"
include "symTable.h"

## Comment 6

endif /* XF_CORE_H */
