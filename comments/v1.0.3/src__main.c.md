# Comments extracted from `src/main.c`

Version: `v1.0.3`

Source: `src/main.c`

## Comment 1

Drain any leftover values the VM left on the stack.

## Comment 2

---- Create thread pool after compile so rules/chunks are frozen ----

## Comment 3

Drain pool before END so all record results are settled.

## Comment 4

Tear down pool before vm_free — workers hold clone pointers into vm.

## Comment 5

Release VM values/chunks first.
Then clear compiler-global name bindings.
Then release symbol-table values.

## Comment 6

Parse optional -j <N> worker-count flag anywhere before the mode flag.

## Comment 7

Cap at hardware concurrency if detectable.

## Comment 8

include <unistd.h>
include <stdio.h>
include <stdlib.h>
include <string.h>
include "../include/core.h"
include "../include/lexer.h"
include "../include/parser.h"
include "../include/symTable.h"
include "../include/ast.h"
include "../include/vm.h"
include "../include/value.h"
include "../include/interp.h"
include "../include/repl.h"
include "../include/mt.h"
define XF_VERSION "1.0.3"

## Comment 9

ifdef _SC_NPROCESSORS_ONLN

## Comment 10

endif
