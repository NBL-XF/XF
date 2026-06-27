#ifndef LS_H
#define LS_H

#include <stddef.h>

typedef struct ls_State ls_State;


/*
 * Built-in logical/set operators recognized by the evaluator:
 *
 *   elem x S      membership on Scott lists, or fallback to predicate-set application S x
 *   contains S x  alias for elem x S
 *   forall S p    universal quantifier over a finite Scott list domain
 *   exists S p    existential quantifier over a finite Scott list domain
 *
 * Parser sugar lowers unbounded quantifiers to BOOLS, so:
 *
 *   forall x . expr   == forall BOOLS (\x.expr)
 *   exists x . expr   == exists BOOLS (\x.expr)
 */

typedef struct {
	size_t max_steps;
	int trace;
	int quiet;
	int use_prelude;

	/*
	 * Script arguments exposed to LambdaScript as builtins:
	 *
	 *   ARG0  = source_name, or <eval>/<stdin> when appropriate
	 *   ARG1  = first script argument
	 *   ARG2  = second script argument
	 *   ...
	 *   ARGC  = Church numeral count of script arguments, excluding ARG0
	 *   ARGS  = Church list of ARG1..ARGN
	 *
	 * Because LambdaScript does not have string literals yet, each argument is
	 * currently injected as a symbolic free variable. Valid identifiers are used
	 * directly. Other strings are encoded as ARGVAL_<hex-bytes>.
	 */
	const char *source_name;
	size_t argc;
	const char *const *argv;

	/*
	 * Global official-library directory used by:
	 *
	 *   import {math}
	 *
	 * When NULL, LambdaScript falls back to:
	 *   1. $LAMBDASCRIPT_LIB_DIR
	 *   2. $XDG_DATA_HOME/lambdascript/lib
	 *   3. $HOME/.lambdascript/lib
	 *   4. ./.lambdascript/lib
	 */
	const char *library_dir;
} ls_Options;

typedef struct {
	char *output;
	char *trace;
	size_t steps;
	int reached_step_limit;
} ls_Result;

ls_State *ls_newstate(void);
void ls_close(ls_State *L);

void ls_options_init(ls_Options *options);
void ls_result_init(ls_Result *result);
void ls_result_free(ls_Result *result);

int ls_eval_string(ls_State *L, const char *source, const ls_Options *options, ls_Result *result);
int ls_eval_file(ls_State *L, const char *path, const ls_Options *options, ls_Result *result);

const char *ls_errmsg(const ls_State *L);

#endif