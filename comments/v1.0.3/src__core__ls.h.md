# Comments extracted from `src/core/ls.h`

Version: `v1.0.3`

Source: `src/core/ls.h`

## Comment 1

Built-in logical/set operators recognized by the evaluator:

  elem x S      membership on Scott lists, or fallback to predicate-set application S x
  contains S x  alias for elem x S
  forall S p    universal quantifier over a finite Scott list domain
  exists S p    existential quantifier over a finite Scott list domain

Parser sugar lowers unbounded quantifiers to BOOLS, so:

  forall x . expr   == forall BOOLS (\x.expr)
  exists x . expr   == exists BOOLS (\x.expr)

## Comment 2

Script arguments exposed to LambdaScript as builtins:

  ARG0  = source_name, or <eval>/<stdin> when appropriate
  ARG1  = first script argument
  ARG2  = second script argument
  ...
  ARGC  = Church numeral count of script arguments, excluding ARG0
  ARGS  = Church list of ARG1..ARGN

Because LambdaScript does not have string literals yet, each argument is
currently injected as a symbolic free variable. Valid identifiers are used
directly. Other strings are encoded as ARGVAL_<hex-bytes>.

## Comment 3

Global official-library directory used by:

  import {math}

When NULL, LambdaScript falls back to:
  1. $LAMBDASCRIPT_LIB_DIR
  2. $XDG_DATA_HOME/lambdascript/lib
  3. $HOME/.lambdascript/lib
  4. ./.lambdascript/lib

## Comment 4

ifndef LS_H
define LS_H

## Comment 5

include <stddef.h>

## Comment 6

endif
