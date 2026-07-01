# Comments extracted from `src/core/ds.c`

Version: `v1.0.3`

Source: `src/core/ds.c`

## Comment 1

── helpers ────────────────────────────────────────────────────

## Comment 2

returns a retained value via xf_map_get

## Comment 3

── column / row ───────────────────────────────────────────────

## Comment 4

retained

## Comment 5

arr retains

## Comment 6

drop local retain

## Comment 7

── sort ───────────────────────────────────────────────────────

## Comment 8

── agg ────────────────────────────────────────────────────────

## Comment 9

retained

## Comment 10

retained/converted

## Comment 11

retained

## Comment 12

Release the old lookup result before overwriting bucket.
This matters if the key exists but contains a non-array value.

## Comment 13

xf_val_ok_arr() takes/retains value-level ownership.
Drop the raw container ownership from xf_arr_new().

## Comment 14

retained

## Comment 15

── merge ──────────────────────────────────────────────────────

## Comment 16

retained

## Comment 17

retained

## Comment 18

── index / keys / values / filter ────────────────────────────

## Comment 19

retained

## Comment 20

retained/converted

## Comment 21

retained

## Comment 22

map retains

## Comment 23

retained

## Comment 24

retained

## Comment 25

arr retains

## Comment 26

drop local retain

## Comment 27

Function predicate: filter(ds, fn(row) -> bool)

## Comment 28

Column/value filter: filter(ds, col) or filter(ds, col, val)

## Comment 29

retained

## Comment 30

retained/converted

## Comment 31

── transpose / expand ─────────────────────────────────────────

## Comment 32

retained

## Comment 33

retained

## Comment 34

local value

## Comment 35

arr retains

## Comment 36

drop local

## Comment 37

retained

## Comment 38

retained

## Comment 39

── flatten ────────────────────────────────────────────────────

## Comment 40

arr of arr-of-maps (chunked dataset)

## Comment 41

retained

## Comment 42

retained

## Comment 43

retained

## Comment 44

retained

## Comment 45

retained

## Comment 46

retained

## Comment 47

arr retains

## Comment 48

drop local retain

## Comment 49

── agg_parallel ───────────────────────────────────────────────

## Comment 50

borrowed

## Comment 51

owned by xformed

## Comment 52

retained

## Comment 53

retained/converted

## Comment 54

retained

## Comment 55

retained

## Comment 56

retained

## Comment 57

retained

## Comment 58

arr retains

## Comment 59

drop local retain

## Comment 60

retained

## Comment 61

retained

## Comment 62

retained

## Comment 63

── stream ─────────────────────────────────────────────────────

## Comment 64

include "internal.h"

## Comment 65

define CD_PAGG_MAX 64

## Comment 66

define CD_STREAM_MAX 256
