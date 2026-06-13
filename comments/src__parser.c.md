# Comments extracted from `src/parser.c`

Source: `src/parser.c`

## Comment 1

============================================================
Init
============================================================

## Comment 2

============================================================
Token navigation
============================================================

## Comment 3

============================================================
Matching
============================================================

## Comment 4

============================================================
Error handling
============================================================

## Comment 5

============================================================
Panic recovery
============================================================

## Comment 6

============================================================
Primary expressions
============================================================

## Comment 7

TK_DIAMOND

## Comment 8

() => empty tuple

## Comment 9

(expr) => grouped expression

## Comment 10

(a, b, c) => tuple literal

## Comment 11

trailing comma

## Comment 12

trailing comma

## Comment 13

(

## Comment 14

)

## Comment 15

trailing comma

## Comment 16

function call: expr(...)

## Comment 17

subscript: expr[expr]

## Comment 18

introspection props: expr.len / expr.type / expr.state

## Comment 19

postfix ++

## Comment 20

postfix --

## Comment 21

right-associative

## Comment 22

walrus :=  (only valid on an identifier target)

## Comment 23

right-associative

## Comment 24

regular assignment operators

## Comment 25

right-associative

## Comment 26

map literal

## Comment 27

set literal

## Comment 28

TK_GT

## Comment 29

---- expression / shorthand zone ----

## Comment 30

Only treat '>' as shorthand-for if the left side is actually
collection[binding]. Otherwise rewind and parse as a normal expr stmt.

## Comment 31

consume '>'

## Comment 32

Not shorthand.
Rewind and parse a full expression statement so normal assignments,
arithmetic, coalesce, pipes, comparisons, etc. all work again.

## Comment 33

bare action: { ... }

## Comment 34

pattern { ... }

## Comment 35

otherwise treat as top-level stmt expression

## Comment 36

BEGIN { ... }

## Comment 37

END { ... }

## Comment 38

IMPORTANT:
A top-level '{ ... }' is a patternless rule,
not a plain statement block.

## Comment 39

top-level typed fn decl vs top-level typed stmt

## Comment 40

consume type just to look ahead

## Comment 41

Try rule parsing first for expression-headed top-level forms,
then fall back to stmt parsing.

## Comment 42

ignore if your compiler wants p->had_error here

## Comment 43

fn_tok here is the 'fn' token; return type should already be tracked by caller

## Comment 44

include "../include/lexer.h"
include "../include/parser.h"

## Comment 45

include <stdio.h>
include <stdlib.h>
include <string.h>
