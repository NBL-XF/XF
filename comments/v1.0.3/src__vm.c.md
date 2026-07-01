# Comments extracted from `src/vm.c`

Version: `v1.0.3`

Source: `src/vm.c`

## Comment 1

============================================================
Chunk
============================================================

## Comment 2

============================================================
Disasm
============================================================

## Comment 3

============================================================
VM init/free
============================================================

## Comment 4

OPTIONAL: bind name "ARGS" → slot
If you want name lookup, you’ll hook this into compiler global map later

## Comment 5

release any live call frames

## Comment 6

release any live task results

## Comment 7

release anything still on the VM stack

## Comment 8

release globals

## Comment 9

release record buffers

## Comment 10

release BEGIN chunk

## Comment 11

release END chunk

## Comment 12

release rule chunks

## Comment 13

release compiled rule patterns

## Comment 14

flush redirects / files

## Comment 15

release globals

## Comment 16

============================================================
Stack / globals
============================================================

## Comment 17

xf_Value vm_pop(VM *vm) {
if (vm->stack_top == 0) {
vm_error(vm, "stack underflow");
return xf_val_nav(XF_TYPE_VOID);
}

xf_Value v = vm->stack[--vm->stack_top];
vm->stack[vm->stack_top] = xf_val_null();
return v;
}

## Comment 18

Important:
Do NOT release v here.
The caller now owns the popped stack reference.

Clear stale slot without releasing it.

## Comment 19

mt.c: write-locked, GC-aware

## Comment 20

mt.c: read-locked, retained

## Comment 21

mt.c: write-locked

## Comment 22

============================================================
Errors
============================================================

## Comment 23

============================================================
Redirect stubs
============================================================

## Comment 24

============================================================
Record splitting
============================================================

## Comment 25

trim trailing newline / carriage return

## Comment 26

preserve full raw record for $0

## Comment 27

separate mutable copy for field splitting

## Comment 28

============================================================
Small value helpers
============================================================

## Comment 29

============================================================
Execution loop
============================================================

## Comment 30

--------------------------------------------------------
Task slot allocator — scans vm->tasks[0..255] for a free slot.
Slots are freed when OP_JOIN retrieves the result.
--------------------------------------------------------

## Comment 31

all 256 slots in use

## Comment 32

vm_pop() removes the stack-owned value and transfers ownership
to this block.

## Comment 33

Replace the frame's current return value.
Do NOT retain+release here; just move ownership into the frame.

## Comment 34

items are on the stack with the first one deepest; copy in order

## Comment 35

xf_arr_push retains

## Comment 36

drop n source items

## Comment 37

retains a → rc=2

## Comment 38

rc=1 (only the value owns)

## Comment 39

stack retains → rc=2

## Comment 40

rc=1, stack owns

## Comment 41

retains each item

## Comment 42

set_add → map_set retains

## Comment 43

n is number of *pairs*, so 2n stack items

## Comment 44

map keys are strings

## Comment 45

retains v internally

## Comment 46

case OP_LOAD_GLOBAL: {
uint32_t idx =
((uint32_t)frame->chunk->code[frame->ip] << 24) |
((uint32_t)frame->chunk->code[frame->ip + 1] << 16) |
((uint32_t)frame->chunk->code[frame->ip + 2] << 8) |
(uint32_t)frame->chunk->code[frame->ip + 3];
frame->ip += 4;

xf_Value gv = (idx < vm->global_count)
? xf_value_retain(vm->globals[idx])
: xf_val_undef(XF_TYPE_VOID);

vm_push(vm, gv);
xf_value_release(gv);
break;
}

## Comment 47

vm_global_read takes the pool read-lock when multithreaded,
returns a retained value.  Single-threaded: no lock, same cost.

## Comment 48

push retained its own ref

## Comment 49

Mark this binding as no longer claimed.
Store replacement first, then release old.

## Comment 50

vm_global_write takes the pool write-lock, retains v, releases old.

## Comment 51

frame->ip is already past OP_CALL + argc byte because READ_U8()
consumed the argc operand. So the opcode itself is ip - 2.

## Comment 52

Defensive: if a native returned one of its args without retaining
(a contract violation), `ret` aliases an argv2[i] that we're about
to release. Retain so `ret` has independent ownership.

## Comment 53

Handle both native and compiled function errors here.

## Comment 54

vm_push retains ret, so we release our local ret afterward.

## Comment 55

Fallback: run synchronously so the slot is always filled.

## Comment 56

Push an integer task handle so OP_JOIN can identify the slot.

## Comment 57

blocks until done

## Comment 58

Synchronous fallback: task was already completed inline.

## Comment 59

Free the slot for reuse.

## Comment 60

============================================================
Begin/rule/end
============================================================

## Comment 61

sweep stack+frames after BEGIN

## Comment 62

sweep stack+frames after END

## Comment 63

============================================================
Record snapshot stubs
============================================================

## Comment 64

if defined(__linux__) || defined(__CYGWIN__)
 define _GNU_SOURCE
endif

## Comment 65

include "../include/vm.h"
include "../include/gc.h"
include "../include/mt.h"
include "../include/simd.h"
include <stdio.h>
include <stdlib.h>
include <string.h>
include <math.h>
include <stdarg.h>
include <regex.h>

## Comment 66

printf("%04zu  ", off);
 if (off > 0 && c->lines[off] == c->lines[off - 1]) printf("   | ");
   else printf("%4u ", c->lines[off]);

## Comment 67

printf("%-16s", opcode_name(op));

## Comment 68

printf("  %u", read_u16(c, off));

## Comment 69

printf("  <f64>");

## Comment 70

printf("  %u", read_u32(c, off));

## Comment 71

printf("  %u", c->code[off++]);

## Comment 72

printf("  %+d", delta);

## Comment 73

define READ_U8()  (frame->chunk->code[frame->ip++])
define READ_U16() (frame->ip += 2, (uint16_t)((frame->chunk->code[frame->ip-2] << 8) | frame->chunk->code[frame->ip-1]))
define READ_U32() (frame->ip += 4, \
