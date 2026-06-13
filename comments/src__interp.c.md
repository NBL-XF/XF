# Comments extracted from `src/interp.c`

Source: `src/interp.c`

## Comment 1

If build_compiled_fn succeeds, fn_chunk ownership moved into out->body.
If it fails, build_compiled_fn already frees fn_chunk.

## Comment 2

── loop break/continue patch lists ────────────────────────────
Each nested loop pushes an entry.  STMT_BREAK appends to the
top entry; after the loop's back-jump we patch them all to the
instruction right after the loop.
──────────────────────────────────────────────────────────────

## Comment 3

placeholder

## Comment 4

if (top_level_compile && !g_interp_preserve_bindings) {
interp_reset_global_bindings(it);
}

## Comment 5

only allocate these once per top-level compile pass

## Comment 6

append imported rules after existing ones if imports contain rules

## Comment 7

zero/init appended region

## Comment 8

statement-form spawn discards returned handle

## Comment 9

statement-form join discards joined result

## Comment 10

text

## Comment 11

csv

## Comment 12

tsv

## Comment 13

keep assignment semantics consistent with FS/OFS/ORS handling

## Comment 14

unreachable but silences compiler warning

## Comment 15

v now owns the retained error; drop our constructor reference once

## Comment 16

stack: [left]

## Comment 17

[left, left]

## Comment 18

[left, state]

## Comment 19

[left, state, "OK"]

## Comment 20

[left, bool]

## Comment 21

OP_JUMP_IF pops bool, so false path stack is just [left]

## Comment 22

discard bad left

## Comment 23

true path already has [left] on stack; do nothing

## Comment 24

No dedicated VM cast opcode yet.
For tonight, compile the operand and let runtime coercions happen
through the existing arithmetic/string/member/call paths.

## Comment 25

++x / --x / x++ / x--

## Comment 26

save original for post-inc/post-dec

## Comment 27

save updated value into the target

## Comment 28

stack is [old, new]; swap then pop => leave old

## Comment 29

patch any breaks even on failure to avoid leak

## Comment 30

patch all break jumps to here (after the loop)

## Comment 31

hidden globals used by the lowered loop

## Comment 32

iterable view

## Comment 33

original map/set

## Comment 34

numeric loop index

## Comment 35

user-visible loop vars

## Comment 36

Setup phase:
  - arrays/tuples/strings: coll_slot = collection
  - maps/sets: src_slot = original collection, coll_slot = keys/elements array

## Comment 37

preserve original map/set

## Comment 38

iterable view = keys array (map) or element array (set)

## Comment 39

idx = 0

## Comment 40

condition: idx < len(coll)

## Comment 41

coll_slot = keys(src)
key = coll[idx]
val = src[key]

## Comment 42

single-bind map iteration yields keys

## Comment 43

set iteration yields elements directly

## Comment 44

array / tuple / string iteration

## Comment 45

idx = idx + 1

## Comment 46

include "../include/interp.h"
include "../include/ast.h"
include "../include/symTable.h"
include "../include/vm.h"
include "../include/parser.h"

## Comment 47

define MAX_LOOP_DEPTH       64
define MAX_BREAK_PATCHES   512
define MAX_CONTINUE_DEPTH    64
define MAX_CONTINUE_PATCHES 512
