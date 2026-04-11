#ifndef XF_INTERP_H
#define XF_INTERP_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"
#include "ast.h"
#include "vm.h"
#include "symTable.h"

typedef struct Interp {
    VM       *vm;
    SymTable *syms;
    bool      had_error;

    bool      returning;
    bool      breaking;
    bool      continuing;

    xf_Value  return_val;
    xf_Value  last_err;
} Interp;
/* global binding / compiler */
uint32_t interp_bind_global_name(Interp *it, xf_Str *name);
uint32_t interp_bind_global_cstr(Interp *it, const char *name);
void     interp_reset_global_bindings(void);

/* compiler entry */
bool     interp_compile_program(Interp *it, Program *prog);
void     interp_init(Interp *it, SymTable *syms, VM *vm);
void     interp_free(Interp *it);
xf_Value interp_eval_expr(Interp *it, Expr *e);
xf_Value interp_eval_stmt(Interp *it, Stmt *s);
bool     interp_run_program(Interp *it, Program *prog);
xf_Value interp_exec_xf_fn_bridge(void *vm_ptr, void *syms_ptr,
                                  xf_fn_t *fn, xf_Value *args, size_t argc);
#endif /* XF_INTERP_H */