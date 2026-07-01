# Comments extracted from `src/simd.c`

Version: `v1.0.3`

Source: `src/simd.c`

## Comment 1

simd.c — SIMD acceleration for the xf interpreter

Build notes:
  The scalar fallbacks compile on any C11 target.
  SIMD paths are guarded by __has_include / compiler-defined macros and
  are only compiled when the host toolchain supports them.
  Runtime dispatch means a binary built on a modern machine still runs
  correctly on older hardware.

  Recommended flags:
    x86-64:  -O3 -msse4.2   (AVX2 paths need -mavx2 but are checked at runtime)
    ARM:     -O3             (NEON is baseline on AArch64)

## Comment 2

============================================================
Architecture detection
============================================================

## Comment 3

Pull in intrinsics only when available and not disabled.

## Comment 4

XF_SIMD_DISABLE

## Comment 5

============================================================
1.  CPU feature detection
============================================================

## Comment 6

Brand string

## Comment 7

============================================================
2.  String operations — scalar fallbacks
============================================================

## Comment 8

============================================================
2a. String operations — SSE4.2 paths
============================================================

## Comment 9

_mm_cmpistri with RANGES mode: match any byte in the range set.

## Comment 10

For single-char search, fall through to optimised memchr which the
libc already vectorises well. Use SSE4.2 for the whitespace case.

## Comment 11

Search for ' ' or '\t' using _SIDD_CMP_RANGES.

## Comment 12

tail

## Comment 13

Find first non-whitespace: negate polarity.

## Comment 14

XF_HAS_SSE4_2

## Comment 15

============================================================
2b. String hash — SSE2 unrolled FNV-1a
============================================================

## Comment 16

Process 8 bytes per iteration unrolled — no gather needed.

## Comment 17

============================================================
2c. NEON string ops
============================================================

## Comment 18

Reduce: if any lane is 0xFF, there's a match.

## Comment 19

non-whitespace = NOT (space OR tab)

## Comment 20

Same unrolled FNV — NEON gather would need shuffle tables; just unroll.

## Comment 21

XF_HAS_NEON

## Comment 22

============================================================
2d. Runtime dispatch table — string
============================================================

## Comment 23

Public string API — thin wrappers through dispatch table.

## Comment 24

libc already vectorises this

## Comment 25

============================================================
3.  Record / field splitting
============================================================

## Comment 26

skip whitespace

## Comment 27

find end of token

## Comment 28

last field — no trailing separator

## Comment 29

NUL-terminate and advance

## Comment 30

============================================================
4a. Bulk numeric — scalar fallbacks
============================================================

## Comment 31

============================================================
4b. Bulk numeric — AVX2 paths
============================================================

## Comment 32

horizontal add

## Comment 33

XF_HAS_AVX2

## Comment 34

============================================================
4c. Bulk numeric — NEON paths
============================================================

## Comment 35

XF_HAS_NEON

## Comment 36

============================================================
4d. Runtime dispatch table — numerics
============================================================

## Comment 37

============================================================
5.  Worker / memory helpers
============================================================

## Comment 38

============================================================
Final dispatch initialisation — called at end of xf_simd_init
============================================================

## Comment 39

Re-open xf_simd_init to wire dispatch after feature detection.

## Comment 40

We use a constructor attribute so init happens before main on GCC/Clang.

## Comment 41

if defined(__linux__) || defined(__CYGWIN__)
 define _GNU_SOURCE
endif

## Comment 42

include "../include/simd.h"

## Comment 43

include <string.h>
include <stdlib.h>
include <float.h>
include <math.h>

## Comment 44

if defined(__x86_64__) || defined(__i386__)
 define XF_ARCH_X86 1
endif

## Comment 45

if defined(__aarch64__) || defined(__arm__)
 define XF_ARCH_ARM 1
endif

## Comment 46

ifndef XF_SIMD_DISABLE
 ifdef XF_ARCH_X86
   include <cpuid.h>
   ifdef __SSE2__
     include <emmintrin.h>
     define XF_HAS_SSE2 1
   endif
   ifdef __SSE4_2__
     include <nmmintrin.h>
     define XF_HAS_SSE4_2 1
   endif
   ifdef __AVX2__
     include <immintrin.h>
     define XF_HAS_AVX2 1
   endif
 endif
 ifdef XF_ARCH_ARM
   if defined(__ARM_NEON) || defined(__ARM_NEON__)
     include <arm_neon.h>
     define XF_HAS_NEON 1
   endif
 endif
endif /* XF_SIMD_DISABLE */

## Comment 47

ifdef XF_ARCH_X86

## Comment 48

endif

## Comment 49

ifdef XF_ARCH_ARM

## Comment 50

ifdef XF_HAS_NEON

## Comment 51

endif

## Comment 52

endif

## Comment 53

ifdef XF_ARCH_X86

## Comment 54

elif defined(XF_ARCH_ARM)

## Comment 55

else

## Comment 56

endif

## Comment 57

ifdef XF_HAS_SSE4_2

## Comment 58

endif /* XF_HAS_SSE4_2 */

## Comment 59

ifdef XF_HAS_SSE2

## Comment 60

endif

## Comment 61

ifdef XF_HAS_NEON

## Comment 62

endif /* XF_HAS_NEON */

## Comment 63

ifdef XF_HAS_NEON

## Comment 64

endif
ifdef XF_HAS_SSE4_2

## Comment 65

ifdef XF_HAS_SSE2

## Comment 66

endif

## Comment 67

endif
ifdef XF_HAS_SSE2

## Comment 68

endif

## Comment 69

ifdef XF_HAS_AVX2

## Comment 70

endif /* XF_HAS_AVX2 */

## Comment 71

ifdef XF_HAS_NEON

## Comment 72

endif /* XF_HAS_NEON */

## Comment 73

ifdef XF_HAS_NEON

## Comment 74

endif
ifdef XF_HAS_AVX2

## Comment 75

endif

## Comment 76

ifdef XF_HAS_AVX2

## Comment 77

endif

## Comment 78

ifdef XF_HAS_SSE2

## Comment 79

endif
