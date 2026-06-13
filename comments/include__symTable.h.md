# Comments extracted from `include/symTable.h`

Source: `include/symTable.h`

## Comment 1

============================================================
xf symbol table

Scoped chain of hash tables.
Each scope is a flat array of Symbol entries using open
addressing.

Scopes form a linked chain:
  current -> parent -> ... -> global

Symbols carry:
  - declared type information
  - runtime value

Ownership:
  - Symbol.name is retained by the symbol
  - Symbol.value owns one retained reference to any heap
    payload it carries
============================================================

## Comment 2

------------------------------------------------------------
Symbol kinds
------------------------------------------------------------

## Comment 3

------------------------------------------------------------
Symbol
------------------------------------------------------------

## Comment 4

retained

## Comment 5

declared XF_TYPE_*

## Comment 6

declared but not yet assigned if false

## Comment 7

runtime value; owns one retained ref

## Comment 8

------------------------------------------------------------
Scope
------------------------------------------------------------

## Comment 9

open-addressed symbol table

## Comment 10

active entry count

## Comment 11

NULL only for global

## Comment 12

valid when kind == SCOPE_FN

## Comment 13

------------------------------------------------------------
SymTable
------------------------------------------------------------

## Comment 14

------------------------------------------------------------
Lifecycle
------------------------------------------------------------

## Comment 15

Pop and free the current scope.
The global scope is never popped by callers.

## Comment 16

------------------------------------------------------------
Symbol operations
------------------------------------------------------------

## Comment 17

------------------------------------------------------------
Scope queries
------------------------------------------------------------

## Comment 18

------------------------------------------------------------
Builtins
------------------------------------------------------------

## Comment 19

------------------------------------------------------------
Debug
------------------------------------------------------------

## Comment 20

XF_SYMTABLE_H

## Comment 21

ifndef XF_SYMTABLE_H
define XF_SYMTABLE_H

## Comment 22

include <stdbool.h>
include <stddef.h>

## Comment 23

include "value.h"
include "lexer.h"

## Comment 24

define SCOPE_INIT_CAP 16

## Comment 25

endif /* XF_SYMTABLE_H */
