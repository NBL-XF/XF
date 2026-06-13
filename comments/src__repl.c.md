# Comments extracted from `src/repl.c`

Source: `src/repl.c`

## Comment 1

strip trailing whitespace

## Comment 2

add non-duplicate entries to history

## Comment 3

handle Ctrl+D cleanly

## Comment 4

include <stdio.h>
include <stdlib.h>
include <string.h>
include <readline/readline.h>
include <readline/history.h>

## Comment 5

include "../include/core.h"
include "../include/lexer.h"
include "../include/parser.h"
include "../include/symTable.h"
include "../include/ast.h"
include "../include/vm.h"
include "../include/value.h"
include "../include/interp.h"
define VERSION "1.0.1"
