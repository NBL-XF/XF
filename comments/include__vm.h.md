# Comments extracted from `include/vm.h`

Source: `include/vm.h`

## Comment 1

chunk

## Comment 2

vm

## Comment 3

optional runtime helpers

## Comment 4

ifndef XF_VM_H
define XF_VM_H

## Comment 5

include <stddef.h>
include <stdint.h>
include <stdbool.h>
include <pthread.h>
include <stdio.h>
include "value.h"

## Comment 6

define VM_STACK_MAX   1024
define VM_FRAMES_MAX  64
define VM_REDIR_MAX   32
define FIELD_MAX      256
define XF_OUTFMT_TEXT  0
define XF_OUTFMT_CSV   1
define XF_OUTFMT_TSV   2
define XF_OUTFMT_JSON  3

## Comment 7

endif
