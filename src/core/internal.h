#pragma once
#ifdef __linux__
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
#include <stdbool.h>

/* ── common macros ────────────────────────────────────────────── */

#define NEED(n) \
    do { if (argc < (n)) return xf_val_nav(XF_TYPE_VOID); } while(0)

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

#define CR_MAX_GROUPS 32

/* ── helpers (defined in helpers.c) ──────────────────────────── */

extern bool     arg_num(xf_Value *args, size_t argc, size_t i, double *out);
extern bool     arg_str(xf_Value *args, size_t argc, size_t i,
                        const char **out, size_t *outlen);
extern xf_Value propagate(xf_Value *args, size_t argc);
extern xf_Value make_str_val(const char *data, size_t len);

/* ── fn-caller context (defined in helpers.c) ─────────────────── */

extern void            core_set_fn_caller(void *vm, void *syms, xf_fn_caller_t caller);
extern xf_fn_caller_t  core_get_fn_caller(void);
extern void           *core_get_fn_caller_vm(void);
extern void           *core_get_fn_caller_syms(void);
extern void           *core_get_root_syms(void);

/* ── regex shared helpers (defined in regex.c) ────────────────── */

extern bool     cr_compile(const char *pat, int cflags,
                            regex_t *out, char *errmsg, size_t errmsg_len);
extern int      cr_parse_flags(xf_Value *args, size_t argc, size_t flag_idx);
extern xf_Str  *cr_apply_replacement(const char *subject, const regmatch_t *pm,
                                      size_t ngroups, const char *repl);

/* ── str shared helper (defined in str.c) ─────────────────────── */

extern bool cs_arg_pat(xf_Value *args, size_t argc, size_t pat_idx,
                       const char **pat_out, int *cflags_out, bool *is_regex);

/* ── build_* forward declarations ────────────────────────────── */
/* internal.h */
xf_module_t *build_img(void);
xf_module_t *build_math(void);
xf_module_t *build_str(void);
xf_module_t *build_os(void);
xf_module_t *build_generics(void);
xf_module_t *build_regex(void);
xf_module_t *build_format(void);
xf_module_t *build_ds(void);
xf_module_t *build_edit(void);
xf_module_t *build_process(void);