# Comments extracted from `src/core/os.c`

Source: `src/core/os.c`

## Comment 1

── file handle table ──────────────────────────────────────────

## Comment 2

── file helpers ───────────────────────────────────────────────

## Comment 3

── shell helpers ──────────────────────────────────────────────

## Comment 4

Thread-local state for the nftw callback

## Comment 5

0 = unlimited

## Comment 6

file

## Comment 7

line number

## Comment 8

matched text

## Comment 9

match start/end

## Comment 10

stop

## Comment 11

Portable fallback: manual opendir recursion

## Comment 12

recurse

## Comment 13

core.os.grep(pattern, path [, flags_str [, max_results]]) -> arr<map>

## Comment 14

include "internal.h"

## Comment 15

define COS_MAX_HANDLES 64

## Comment 16

include <dirent.h>
include <sys/stat.h>

## Comment 17

ifdef __linux__
include <ftw.h>

## Comment 18

endif

## Comment 19

ifdef __linux__

## Comment 20

else

## Comment 21

endif
