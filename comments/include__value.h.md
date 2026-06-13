# Comments extracted from `include/value.h`

Source: `include/value.h`

## Comment 1

spinlock: guards items/len/cap mutations

## Comment 2

spinlock: guards slots/order mutations

## Comment 3

internal backing store

## Comment 4

--------------------------------------------------------
Embedded spinlock helpers (used by xf_arr / xf_map internals).
Based on atomic_flag — zero-overhead when uncontended.
--------------------------------------------------------

## Comment 5

ifndef XF_VALUE_H
define XF_VALUE_H

## Comment 6

include <stdatomic.h>

## Comment 7

include <stdbool.h>

## Comment 8

include <stddef.h>

## Comment 9

include <stdint.h>

## Comment 10

define XF_STATE_IS_TERMINAL(s)\

## Comment 11

define XF_STATE_IS_ERROR(s)\

## Comment 12

define XF_STATE_IS_BOOL(s)\

## Comment 13

define XF_STATE_IS_PENDING(s)\

## Comment 14

define XF_RE_GLOBAL    (1u << 0)
define XF_RE_ICASE     (1u << 1)
define XF_RE_MULTILINE (1u << 2)
define XF_RE_EXTENDED  (1u << 3)

## Comment 15

define XF_NULL  (xf_val_null())
define XF_UNDEF (xf_val_undef(XF_TYPE_VOID))
define XF_UNDET (xf_val_undet(XF_TYPE_BOOL))

## Comment 16

define XF_PROPAGATE(v, expr)                                   \

## Comment 17

if defined(__x86_64__) || defined(__i386__)

## Comment 18

else

## Comment 19

endif

## Comment 20

endif
