# Comments extracted from `src/repl.c`

Version: `v1.0.3`

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
include <limits.h>
include <sys/types.h>

## Comment 5

if defined(XF_ENABLE_READLINE) && defined(__has_include)
 if __has_include(<readline/readline.h>) && __has_include(<readline/history.h>)
   include <readline/readline.h>
   include <readline/history.h>
 else
   undef XF_ENABLE_READLINE
 endif
endif

## Comment 6

if !defined(XF_ENABLE_READLINE)

## Comment 7

endif

## Comment 8

include "../include/core.h"
include "../include/lexer.h"
include "../include/parser.h"
include "../include/symTable.h"
include "../include/ast.h"
include "../include/vm.h"
include "../include/value.h"
include "../include/interp.h"
define VERSION "1.0.3"

## Comment 9

ifndef PATH_MAX
define PATH_MAX 4096
endif
