# Comments extracted from `include/simd.h`

Source: `include/simd.h`

## Comment 1

simd.h — SIMD acceleration for the xf interpreter

Five domains:

  1. CPU feature detection  — runtime dispatch table populated once at
                              startup via xf_simd_init().

  2. String ops             — search, compare, FNV hash.

  3. Field / record split   — whitespace-split and single-char-FS split
                              using 16-byte vector scans.

  4. Bulk numeric ops       — sum, min, max, scale over double arrays
                              extracted from xf_arr_t.

  5. Worker dispatch hint   — xf_simd_memcpy_nt() for non-temporal stores
                              when handing large record buffers to workers.

Every function has a scalar fallback compiled unconditionally.
The SIMD paths are selected at runtime through function pointers set by
xf_simd_init().  Code that calls these functions needs no #ifdefs.

Compile-time guards:
  XF_SIMD_DISABLE   — define to force scalar-only (useful for sanitizers)

## Comment 2

============================================================
1.  CPU feature flags
============================================================

## Comment 3

ARM Advanced SIMD

## Comment 4

bitmask of XF_CPU_*

## Comment 5

e.g. "Apple M3" or "Intel Core i9-13900K"

## Comment 6

xf_simd_init — detect CPU features, set dispatch table.
Must be called once before any other xf_simd_* function.
Safe to call from main() before threads start.

## Comment 7

Query detected features after init.

## Comment 8

============================================================
2.  String operations
============================================================

## Comment 9

xf_simd_str_find_char — find first occurrence of byte c in buf[0..len).
Returns pointer to found byte, or NULL.
Equivalent to memchr but vectorised for long haystacks.

## Comment 10

xf_simd_str_find_sep_ws — find first whitespace byte (space or tab) in
buf[0..len).  Used by the whitespace field splitter.
Returns pointer or NULL.

## Comment 11

xf_simd_str_skip_ws — advance past leading space/tab bytes.
Returns pointer to first non-whitespace byte, or buf+len if all ws.

## Comment 12

xf_simd_memcmp — fast memcmp; returns 0 if equal.
Drop-in for the inner loop of xf_str_cmp.

## Comment 13

xf_simd_str_hash — FNV-1a with SIMD unrolling for long strings.
Same output as the scalar implementation in value.c.

## Comment 14

============================================================
3.  Record / field splitting
============================================================

## Comment 15

xf_simd_split_ws — whitespace field split.

  buf        mutable NUL-terminated record buffer (will be modified in place)
  len        length of buf (excluding NUL)
  fields     output array of char* pointers, one per field
  max_fields capacity of fields[]

Returns number of fields found.
Replaces the inner whitespace-split loop in split_record().

## Comment 16

xf_simd_split_sep — single-character separator split.

Same contract as xf_simd_split_ws but splits on sep instead of whitespace.
Replaces the single-char FS loop in split_record().

## Comment 17

xf_simd_trim_newline — strip trailing \n and \r from buf[0..len).
Returns new length.  Replaces the trim loop at the top of split_record().

## Comment 18

============================================================
4.  Bulk numeric operations over double arrays

These operate on a plain double* extracted from xf_arr_t items
(caller must verify all elements are XF_TYPE_NUM / XF_STATE_OK
before calling).  The _arr variants do the type-check loop for you.
============================================================

## Comment 19

Raw double-array ops — no bounds or type checking.

## Comment 20

xs[i] *= factor

## Comment 21

xs[i] += v

## Comment 22

============================================================
5.  Worker / memory helpers
============================================================

## Comment 23

xf_simd_memcpy_nt — non-temporal (streaming) memcpy for large buffers.
Avoids polluting L1/L2 when copying record data to worker VM buffers.
Falls back to memcpy for small sizes or when NT stores aren't available.

dst must be 16-byte aligned for the SIMD path (unaligned is safe but
falls back automatically).

## Comment 24

XF_SIMD_NT_THRESHOLD — minimum size to use NT stores.
Below this memcpy is faster.

## Comment 25

256 KB

## Comment 26

XF_SIMD_H

## Comment 27

pragma once
ifndef XF_SIMD_H
define XF_SIMD_H

## Comment 28

include <stddef.h>
include <stdint.h>
include <stdbool.h>

## Comment 29

define XF_CPU_SSE2      (1u <<  0)
define XF_CPU_SSE4_2    (1u <<  1)
define XF_CPU_AVX2      (1u <<  2)
define XF_CPU_POPCNT    (1u <<  3)
define XF_CPU_NEON      (1u <<  4)   /* ARM Advanced SIMD */

## Comment 30

define XF_SIMD_NT_THRESHOLD  (256 * 1024)   /* 256 KB */

## Comment 31

endif /* XF_SIMD_H */
