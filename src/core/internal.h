#pragma once

#if defined(__linux__) || defined(__CYGWIN__)
#  define _GNU_SOURCE
#endif

#include "../../include/core.h"
#include "../../include/value.h"
#include "../../include/symTable.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <regex.h>

/* ── convenience macros ───────────────────────────────────────── */

#define NEED(n) \
    do { if (argc < (n)) return xf_val_nav(XF_TYPE_VOID); } while(0)

/* used inside build_* functions where local var `m` is the module */
#define FN(name, ret, impl) \
    xf_module_set(m, name, xf_val_native_fn(name, ret, impl))

#define MATH1(c_fn) \
    do { double x; \
         if (!arg_num(args, argc, 0, &x)) return propagate(args, argc); \
         return xf_val_ok_num(c_fn(x)); } while(0)

#define MATH2(c_fn) \
    do { double x, y; \
         if (!arg_num(args, argc, 0, &x)) return propagate(args, argc); \
         if (!arg_num(args, argc, 1, &y)) return propagate(args, argc); \
         return xf_val_ok_num(c_fn(x, y)); } while(0)

/* ── helpers (core/helpers.c) ─────────────────────────────────── */

bool     arg_num(xf_Value *args, size_t argc, size_t i, double *out);
bool     arg_str(xf_Value *args, size_t argc, size_t i,
                 const char **out, size_t *outlen);
xf_Value propagate(xf_Value *args, size_t argc);
xf_Value make_str_val(const char *data, size_t len);

/* ── fn-caller context (core/helpers.c) ──────────────────────── */

void            core_set_fn_caller(void *vm, void *syms, xf_fn_caller_t caller);
xf_fn_caller_t  core_get_fn_caller(void);
void           *core_get_fn_caller_vm(void);
void           *core_get_fn_caller_syms(void);

/* ── regex shared helpers (core/regex.c) ─────────────────────── */

#define CR_MAX_GROUPS 32

bool    cr_compile(const char *pat, int cflags,
                   regex_t *out, char *errmsg, size_t errmsg_len);
int     cr_parse_flags(xf_Value *args, size_t argc, size_t flag_idx);
xf_Str *cr_apply_replacement(const char *subject, const regmatch_t *pm,
                              size_t ngroups, const char *repl);

/* cs_arg_pat: pattern-arg extractor shared by str.c and generics.c
 * Defined in core/str.c. */
bool cs_arg_pat(xf_Value *args, size_t argc, size_t pat_idx,
                const char **pat_out, int *cflags_out, bool *is_regex);

/* ── build_* forward declarations ────────────────────────────── */

xf_module_t *build_math(void);
xf_module_t *build_str(void);
xf_module_t *build_os(void);
xf_module_t *build_generics(void);
xf_module_t *build_ds(void);
xf_module_t *build_edit(void);
xf_module_t *build_format(void);
xf_module_t *build_regex(void);
xf_module_t *build_process(void);