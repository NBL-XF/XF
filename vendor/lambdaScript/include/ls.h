#ifndef LS_H
#define LS_H

#include <stddef.h>

typedef struct ls_State ls_State;

typedef struct {
	size_t max_steps;
	int trace;
	int quiet;
	int use_prelude;
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