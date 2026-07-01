# Comments extracted from `src/core/internal.h`

Version: `v1.0.3`

Source: `src/core/internal.h`

## Comment 1

── common macros ──────────────────────────────────────────────

## Comment 2

── helpers (defined in helpers.c) ────────────────────────────

## Comment 3

── fn-caller context (defined in helpers.c) ───────────────────

## Comment 4

── regex shared helpers (defined in regex.c) ──────────────────

## Comment 5

── str shared helper (defined in str.c) ───────────────────────

## Comment 6

── build_* forward declarations ──────────────────────────────

## Comment 7

internal.h

## Comment 8

pragma once
ifdef __linux__
 define _GNU_SOURCE
endif

## Comment 9

include "../../include/core.h"
include "../../include/value.h"
include "../../include/symTable.h"
include <math.h>
include <stdio.h>
include <stdlib.h>
include <string.h>
include <ctype.h>
include <time.h>
include <pthread.h>
include <regex.h>
include <stdbool.h>

## Comment 10

define NEED(n) \

## Comment 11

define FN(name, ret, impl)                     \

## Comment 12

define MATH1(fn) do {                     \

## Comment 13

define MATH2(fn) do {                          \

## Comment 14

define CR_MAX_GROUPS 32
