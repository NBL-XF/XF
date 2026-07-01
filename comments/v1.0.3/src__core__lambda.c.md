# Comments extracted from `src/core/lambda.c`

Version: `v1.0.3`

Source: `src/core/lambda.c`

## Comment 1

core.lambda.eval(source [, max_steps [, use_prelude]]) -> str

## Comment 2

core.lambda.file(path [, max_steps [, use_prelude]]) -> str

## Comment 3

core.lambda.run(source [, max_steps [, use_prelude]]) -> map

## Comment 4

core.lambda.run_file(path [, max_steps [, use_prelude]]) -> map

## Comment 5

core.lambda.trace(source [, max_steps [, use_prelude]]) -> map

## Comment 6

lambda.c - core.lambda bridge for LambdaScript

Build note:
  Add the LambdaScript include directory to your XF compile flags, e.g.
    -I/Volumes/Experiments/lambdaScript/include
  and link the LambdaScript implementation/library, e.g.
    /Volumes/Experiments/lambdaScript/libls.a
  or the LambdaScript object files.

## Comment 7

include "internal.h"
include "ls.h"
