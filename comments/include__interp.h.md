# Comments extracted from `include/interp.h`

Source: `include/interp.h`

## Comment 1

global binding / compiler

## Comment 2

compiler entry

## Comment 3

XF_INTERP_H

## Comment 4

ifndef XF_INTERP_H
define XF_INTERP_H

## Comment 5

include <stdbool.h>
include <stdint.h>
include <stdlib.h>
include <string.h>
include "value.h"
include "ast.h"
include "vm.h"
include "symTable.h"

## Comment 6

define XF_MAX_FN_LOCALS        256
define XF_MAX_LOOP_DEPTH       64
define XF_MAX_BREAK_PATCHES    512
define XF_MAX_CONTINUE_DEPTH   64
define XF_MAX_CONTINUE_PATCHES 512

## Comment 7

endif /* XF_INTERP_H */
