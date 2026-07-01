# Comments extracted from `src/core/str.c`

Version: `v1.0.3`

Source: `src/core/str.c`

## Comment 1

── cs_arg_pat (exported) ──────────────────────────────────────

## Comment 2

── internal regex replace helpers ────────────────────────────

## Comment 3

── cs_* functions ─────────────────────────────────────────────

## Comment 4

Scan fmt for the first conversion specifier and dispatch on its type

## Comment 5

No specifier — just return the format string

## Comment 6

Collect the specifier: %[flags][width][.prec]type

## Comment 7

skip %

## Comment 8

flags

## Comment 9

width

## Comment 10

precision

## Comment 11

type char

## Comment 12

Build suffix (text after the specifier) into a second buffer

## Comment 13

Replace conv with lld for safety

## Comment 14

remove conv

## Comment 15

include "internal.h"
