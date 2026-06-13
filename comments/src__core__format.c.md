# Comments extracted from `src/core/format.c`

Source: `src/core/format.c`

## Comment 1

── string padding/wrap helpers ────────────────────────────────

## Comment 2

── format() ───────────────────────────────────────────────────

## Comment 3

── number formatters ──────────────────────────────────────────

## Comment 4

── JSON serializer ────────────────────────────────────────────

## Comment 5

── JSON parser ────────────────────────────────────────────────

## Comment 6

── csv_row / tsv_row ──────────────────────────────────────────

## Comment 7

── table ──────────────────────────────────────────────────────

## Comment 8

header widths

## Comment 9

cell widths

## Comment 10

top separator

## Comment 11

header row

## Comment 12

middle separator

## Comment 13

data rows

## Comment 14

bottom separator

## Comment 15

include "internal.h"

## Comment 16

define CF_ENSURE(n) \

## Comment 17

undef CF_ENSURE

## Comment 18

define CSV_ENSURE(n) do{if(pos+(n)+4>=cap){cap=cap*2+(n)+4;buf=realloc(buf,cap);}}while(0)

## Comment 19

undef CSV_ENSURE

## Comment 20

define TB_ENSURE(n) \

## Comment 21

define TB_CHAR(ch) \

## Comment 22

define TB_STR(s,l) \

## Comment 23

define TB_PAD(n) \

## Comment 24

define TB_SEP() \

## Comment 25

undef TB_ENSURE
undef TB_CHAR
undef TB_STR
undef TB_PAD
undef TB_SEP
