# Comments extracted from `src/core/helpers.c`

Source: `src/core/helpers.c`

## Comment 1

── arg coercion helpers ───────────────────────────────────────

## Comment 2

Fast path: already a string

## Comment 3

Slow path: coerce

## Comment 4

release previous value in slot

## Comment 5

store new value (takes ownership of c)

## Comment 6

── fn-caller context ──────────────────────────────────────────

## Comment 7

Active execution context must be thread-local.
Root/global symbols may be captured once and reused for global lookups.

## Comment 8

Capture root symtable once from the first real registration.

## Comment 9

include "internal.h"
