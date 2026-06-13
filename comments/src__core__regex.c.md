# Comments extracted from `src/core/regex.c`

Source: `src/core/regex.c`

## Comment 1

── shared helpers (exported via internal.h) ───────────────────

## Comment 2

Convert Perl-style shorthands to POSIX ERE equivalents.
\d → [0-9]   \D → [^0-9]
\w → [a-zA-Z0-9_]  \W → [^a-zA-Z0-9_]
\s → [[:space:]]   \S → [^[:space:]]

## Comment 3

── local helper: extract pattern + cflags from str or regex arg ─

## Comment 4

For plain string patterns, honour explicit flags arg

## Comment 5

── cr_build_match_map ──────────────────────────────────────────

## Comment 6

── cr_match ───────────────────────────────────────────────────

## Comment 7

── cr_search ──────────────────────────────────────────────────

## Comment 8

── cr_split ───────────────────────────────────────────────────

## Comment 9

── cr_replace_impl ────────────────────────────────────────────

## Comment 10

── cr_groups ──────────────────────────────────────────────────

## Comment 11

── cr_test ────────────────────────────────────────────────────

## Comment 12

include "internal.h"

## Comment 13

define ENSURE(n) \

## Comment 14

undef ENSURE
