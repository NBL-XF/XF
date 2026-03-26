#if defined(__linux__) || defined(__CYGWIN__)
#define _GNU_SOURCE
#endif
#include "../include/interp.h"
#include "../include/parser.h"
#include "../include/core.h"
#include "../include/gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>
#include <strings.h>
#include <pthread.h>
/* ============================================================
 * Spawn thread infrastructure
 *
 * Each `spawn fn(args)` statement launches a pthread.
 * The pthread receives a SpawnCtx containing:
 *   - a pre-evaluated xf_Value fn value (owned ref)
 *   - pre-evaluated arg values (owned refs)
 *   - a fresh SymTable/Interp that shares the parent VM
 * A global mutex protects VM state mutations (record context,
 * globals, redir cache) during concurrent execution.
 * The join handle is a uint32 thread id mapping into g_spawn[].
 * ============================================================ */

#define XF_SPAWN_MAX 256

typedef struct {
    pthread_t   tid;
    uint32_t    id;          /* handle value returned to XF code */
    bool        started;     /* thread has actually been launched */
    bool        done;        /* thread finished */
    bool        joined;      /* join already consumed */
    xf_Value    result;
    xf_Value    fn_val;      /* owned ref to the fn being called */
    xf_Value    args[64];
    size_t      argc;
    VM         *vm;          /* shared VM — access guarded by g_spawn_mu */
    SymTable   *parent_syms; /* borrowed — fallback only */
    SymTable    snap_syms;   /* owned snapshot */
    bool        snap_ready;
} SpawnCtx;
static pthread_mutex_t g_spawn_mu    = PTHREAD_MUTEX_INITIALIZER;
static SpawnCtx        g_spawn[XF_SPAWN_MAX];
static size_t          g_spawn_live  = 0;   /* number of occupied slots */
static uint32_t        g_spawn_next  = 1;  /* 0 reserved for "no handle" */
static void copy_globals_from(SymTable *dst, SymTable *src);
static void *spawn_thread_fn(void *arg);
static xf_Value interp_exec_xf_fn(void *vm_ptr, void *syms_ptr,
                                  xf_fn_t *fn, xf_Value *args, size_t argc);
static xf_Value mat_mul(xf_Value a, xf_Value b);
static xf_Value arr_broadcast(xf_Value a, xf_Value b, int op);
static xf_Value val_concat(xf_Value a, xf_Value b);
static xf_Value eval_in(xf_Value needle, xf_Value haystack) {
    if (needle.state != XF_STATE_OK)   return xf_value_retain(needle);
    if (haystack.state != XF_STATE_OK) return xf_value_retain(haystack);

    /* substring in string */
    if (haystack.type == XF_TYPE_STR && haystack.data.str) {
        xf_Value ns = xf_coerce_str(needle);
        if (ns.state != XF_STATE_OK || !ns.data.str) {
            if (ns.state == XF_STATE_OK) xf_value_release(ns);
            return xf_val_false();
        }

        bool found = strstr(haystack.data.str->data, ns.data.str->data) != NULL;
        xf_value_release(ns);
        return found ? xf_val_true() : xf_val_false();
    }

    /* element in array (stringified comparison for now) */
    if (haystack.type == XF_TYPE_ARR && haystack.data.arr) {
        xf_Value ns = xf_coerce_str(needle);
        if (ns.state != XF_STATE_OK || !ns.data.str) {
            if (ns.state == XF_STATE_OK) xf_value_release(ns);
            return xf_val_false();
        }

        xf_arr_t *a = haystack.data.arr;
        for (size_t i = 0; i < a->len; i++) {
            xf_Value es = xf_coerce_str(a->items[i]);
            bool match = (es.state == XF_STATE_OK &&
                          es.data.str &&
                          strcmp(es.data.str->data, ns.data.str->data) == 0);
            xf_value_release(es);

            if (match) {
                xf_value_release(ns);
                return xf_val_true();
            }
        }

        xf_value_release(ns);
        return xf_val_false();
    }

    /* key in map/set */
    if ((haystack.type == XF_TYPE_MAP || haystack.type == XF_TYPE_SET) && haystack.data.map) {
        xf_Value ks = xf_coerce_str(needle);
        if (ks.state != XF_STATE_OK || !ks.data.str) {
            if (ks.state == XF_STATE_OK) xf_value_release(ks);
            return xf_val_false();
        }

        xf_Value got = xf_map_get(haystack.data.map, ks.data.str);
        xf_value_release(ks);
        return (got.state == XF_STATE_OK) ? xf_val_true() : xf_val_false();
    }

    return xf_val_false();
}
static inline void maybe_auto_gc(Interp *it) {
    if (!it) return;
    xf_gc_maybe_collect(it->vm, it->syms, it);
}
static SpawnCtx *find_spawn_ctx_by_id(uint32_t hid) {
    if (hid == 0) return NULL;
    for (size_t i = 0; i < XF_SPAWN_MAX; i++) {
        if (g_spawn[i].id == hid) return &g_spawn[i];
    }
    return NULL;
}

static SpawnCtx *alloc_spawn_ctx(void) {
    if (g_spawn_live >= XF_SPAWN_MAX) return NULL;
    for (size_t i = 0; i < XF_SPAWN_MAX; i++) {
        if (g_spawn[i].id == 0) {
            memset(&g_spawn[i], 0, sizeof(g_spawn[i]));
            g_spawn_live++;
            return &g_spawn[i];
        }
    }
    return NULL;
}

static void free_spawn_ctx_slot(SpawnCtx *ctx) {
    if (!ctx || ctx->id == 0) return;
    if (ctx->snap_ready) {
        sym_free(&ctx->snap_syms);
        ctx->snap_ready = false;
    }
    xf_value_release(ctx->result);
    xf_value_release(ctx->fn_val);
    for (size_t i = 0; i < ctx->argc && i < 64; i++) xf_value_release(ctx->args[i]);
    memset(ctx, 0, sizeof(*ctx));
    if (g_spawn_live > 0) g_spawn_live--;
}

static bool start_spawn_ctx(SpawnCtx *ctx) {
    if (!ctx) return false;
    if (ctx->started) return true;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    int rc = pthread_create(&ctx->tid, &attr, spawn_thread_fn, ctx);

    pthread_attr_destroy(&attr);

    if (rc != 0) {
        ctx->tid = (pthread_t)0;
        ctx->started = false;
        return false;
    }

    ctx->started = true;
    return true;
}
static void *spawn_thread_fn(void *arg) {
    SpawnCtx *ctx = (SpawnCtx *)arg;

    SymTable syms;
    sym_init(&syms);

    /* copy globals exactly once */
    if (ctx->snap_ready) {
        copy_globals_from(&syms, &ctx->snap_syms);
    } else if (ctx->parent_syms) {
        copy_globals_from(&syms, ctx->parent_syms);
    }

    Interp it;
    memset(&it, 0, sizeof(it));
    it.is_worker = true;
    it.syms      = &syms;
    it.vm        = ctx->vm;
    it.last_err  = xf_val_null();

    core_set_fn_caller(ctx->vm, &syms, interp_exec_xf_fn);

    vm_rec_snapshot(ctx->vm, &it.rec_snap);
    it.use_rec_snap = true;

    if (ctx->fn_val.state == XF_STATE_OK &&
        ctx->fn_val.type  == XF_TYPE_FN  &&
        ctx->fn_val.data.fn) {
        xf_Fn *fn = ctx->fn_val.data.fn;

        if (!fn->is_native) {
            Scope *fn_sc = sym_push(&syms, SCOPE_FN);
            fn_sc->fn_ret_type = fn->return_type;

            for (size_t i = 0; i < fn->param_count; i++) {
                xf_Value av = i < ctx->argc ? ctx->args[i]
                                            : xf_val_undef(fn->params[i].type);
                Symbol *ps = sym_declare(&syms,
                                         fn->params[i].name,
                                         SYM_PARAM,
                                         fn->params[i].type,
                                         (Loc){.source="<spawn>", .line=0, .col=0});
                if (ps) {
                    xf_value_release(ps->value);
                    ps->value = xf_value_retain(av);
                    ps->state = av.state;
                    ps->is_defined = true;
                }
            }

            it.returning = false;
            interp_eval_stmt(&it, (Stmt *)fn->body);
xf_Value ret0 = it.returning ? xf_value_retain(it.return_val) : xf_val_null();
ctx->result = ret0;
it.returning = false;

scope_free(sym_pop(&syms));
                } else if (fn->native_v) {
            ctx->result = fn->native_v(ctx->args, ctx->argc);
        }
}
    interp_free(&it);
    vm_rec_snapshot_free(&it.rec_snap);
    sym_free(&syms);

    pthread_mutex_lock(&g_spawn_mu);
    ctx->done = true;
    pthread_mutex_unlock(&g_spawn_mu);
    return NULL;
}
static inline xf_Value propagate_err(xf_Value v) { return v; }

/* ── copy_globals_from ───────────────────────────────────────────
 * Copy all symbols from src's global scope into dst's global scope,
 * retaining each value.  Called before executing an XF fn in a
 * worker thread so that core.*, user-defined globals, etc. are
 * visible without sharing the parent SymTable struct.
 * dst must have been freshly sym_init'd (empty global scope).
 * This is a shallow copy: values are ref-counted, no double-free risk.
 * Caller must hold no lock — this reads parent globals which are
 * stable (not mutated) during parallel worker execution.
 * ---------------------------------------------------------------- */
static void copy_globals_from(SymTable *dst, SymTable *src) {
    if (!src || !src->global) return;
    Scope *sg = src->global;
    for (size_t i = 0; i < sg->capacity; i++) {
        Symbol *s = &sg->entries[i];
        if (!s->name) continue;   /* empty slot */
        Symbol *ds = sym_declare(dst, s->name, s->kind, s->type, s->decl_loc);
if (ds) {
    xf_value_release(ds->value);
    ds->value      = xf_value_retain(s->value);
    ds->state      = s->state;
    ds->is_const   = s->is_const;
    ds->is_defined = s->is_defined;
}
    }
}
static xf_Value interp_call_xf_fn(Interp *it, xf_fn_t *fn, xf_Value *args, size_t argc, Loc loc) {
    Scope *fn_sc = sym_push(it->syms, SCOPE_FN);
    fn_sc->fn_ret_type = fn->return_type;

    for (size_t i = 0; i < fn->param_count; i++) {
        xf_Value av = (i < argc) ? args[i] : xf_val_undef(fn->params[i].type);
        Symbol *ps = sym_declare(it->syms, fn->params[i].name,
                                 SYM_PARAM, fn->params[i].type, loc);
        if (ps) {
            xf_value_release(ps->value);
            ps->value      = xf_value_retain(av);
            ps->state      = av.state;
            ps->is_defined = true;
        }
    }

    bool saved_returning = it->returning;
    xf_Value saved_ret   = it->return_val;

    it->returning = false;
    it->return_val = xf_val_null();

    interp_eval_stmt(it, (Stmt *)fn->body);

    xf_Value result = it->returning ? xf_value_retain(it->return_val) : xf_val_null();

    xf_value_release(it->return_val);
    it->return_val = saved_ret;
    it->returning  = saved_returning;

    scope_free(sym_pop(it->syms));
    return result;
}
/* ── interp_exec_xf_fn ───────────────────────────────────────────
 * Executes an XF-language (or native) fn in the calling thread.
 * Creates a private SymTable+Interp that shares only the VM pointer.
 * Parent globals are COPIED into the private table so that core.*,
 * user-defined functions and variables are visible to the worker.
 * All mutations (local vars, fn scope) stay private — no races.
 * ---------------------------------------------------------------- */
static xf_Value interp_exec_xf_fn(void *vm_ptr, void *syms_ptr,
                                   xf_fn_t *fn, xf_Value *args, size_t argc) {
    if (!fn) return xf_val_null();
    if (fn->is_native && fn->native_v) return fn->native_v(args, argc);
    if (!fn->body) return xf_val_null();

    VM       *vm         = (VM *)vm_ptr;
    SymTable *parent_st  = (SymTable *)syms_ptr;

    /* private symbol table — copy parent globals in, no shared state */
    SymTable syms;
    sym_init(&syms);
    if (parent_st) copy_globals_from(&syms, parent_st);

    /* Bind a thread-local callback context for this private interpreter.
     * core_set_fn_caller() now uses thread-local overrides, so this worker
     * can safely point core.* callback paths at its private SymTable
     * without clobbering sibling threads. */
    Interp it;
    memset(&it, 0, sizeof(it));
    it.syms     = &syms;
    it.vm       = vm;
    it.last_err = xf_val_null();

    core_set_fn_caller(vm, parent_st, interp_exec_xf_fn);

    /* Snapshot record context — worker sees the record at call time */
    vm_rec_snapshot(vm, &it.rec_snap);
    it.use_rec_snap = true;

    /* push fn scope and bind parameters — all private */
    Scope *fn_sc = sym_push(&syms, SCOPE_FN);
    fn_sc->fn_ret_type = fn->return_type;
    for (size_t i = 0; i < fn->param_count; i++) {
        xf_Value av = (i < argc) ? args[i]
                                 : xf_val_undef(fn->params[i].type);
        Symbol *ps = sym_declare(&syms, fn->params[i].name,
                                  SYM_PARAM, fn->params[i].type,
                                  (Loc){.source="<worker>",.line=0,.col=0});
                                  if (ps) {
    xf_value_release(ps->value);
    ps->value      = xf_value_retain(av);
    ps->state      = av.state;
    ps->is_defined = true;
}
    }

    it.returning = false;
    interp_eval_stmt(&it, (Stmt *)fn->body);
    xf_Value result = it.returning ? xf_value_retain(it.return_val) : xf_val_null();
it.returning = false;

scope_free(sym_pop(&syms));
    interp_free(&it);
    vm_rec_snapshot_free(&it.rec_snap);
    sym_free(&syms);
    return result;
}

static bool bind_loop_value(Interp *it, LoopBind *bind, xf_Value v, Loc loc);
static bool is_complex_like(xf_Value v) {
    return v.state == XF_STATE_OK &&
           (v.type == XF_TYPE_COMPLEX || v.type == XF_TYPE_NUM);
}

static xf_complex_t to_complex(xf_Value v) {
    xf_complex_t z = {0.0, 0.0};
    if (v.state != XF_STATE_OK) return z;
    if (v.type == XF_TYPE_COMPLEX) return v.data.complex;
    if (v.type == XF_TYPE_NUM) {
        z.re = v.data.num;
        z.im = 0.0;
    }
    return z;
}

static xf_Value apply_binary_op(Interp *it,BinOp op, xf_Value a, xf_Value b, Loc loc) {
    if (a.state == XF_STATE_UNDEF || b.state == XF_STATE_UNDEF) {
        interp_error(it, loc, "undefined value used in operation");
        xf_err_t *e = xf_err_new("undefined value used in operation",
                                 loc.source, loc.line, loc.col);
        xf_Value out = xf_val_err(e, XF_TYPE_VOID);
        xf_err_release(e);
        return out;
    }

    if (a.state == XF_STATE_UNDET || b.state == XF_STATE_UNDET) {
        interp_error(it, loc, "undetermined value used in operation");
        xf_err_t *e = xf_err_new("undetermined value used in operation",
                                 loc.source, loc.line, loc.col);
        xf_Value out = xf_val_err(e, XF_TYPE_VOID);
        xf_err_release(e);
        return out;
    }
    if (is_complex_like(a) && is_complex_like(b) &&
        (a.type == XF_TYPE_COMPLEX || b.type == XF_TYPE_COMPLEX)) {
        xf_complex_t x = to_complex(a);
        xf_complex_t y = to_complex(b);

        switch (op) {
            case BINOP_ADD:
                return xf_val_ok_complex(x.re + y.re, x.im + y.im);

            case BINOP_SUB:
                return xf_val_ok_complex(x.re - y.re, x.im - y.im);

            case BINOP_MUL:
                return xf_val_ok_complex(
                    x.re * y.re - x.im * y.im,
                    x.re * y.im + x.im * y.re
                );

            case BINOP_DIV: {
                double d = y.re * y.re + y.im * y.im;
                if (d == 0.0) {
                    return xf_val_err(
                        xf_err_new("complex division by zero",
                                   loc.source ? loc.source : "<unknown>",
                                   loc.line, loc.col),
                        XF_TYPE_COMPLEX
                    );
                }
                return xf_val_ok_complex(
                    (x.re * y.re + x.im * y.im) / d,
                    (x.im * y.re - x.re * y.im) / d
                );
            }

            default:
                break;
        }
    }

    switch (op) {
        case BINOP_ADD:
            if ((a.state == XF_STATE_OK && a.type == XF_TYPE_ARR) ||
                (b.state == XF_STATE_OK && b.type == XF_TYPE_ARR))
                return arr_broadcast(a, b, 0);
            {
                xf_Value na = xf_coerce_num(a);
                if (na.state != XF_STATE_OK) return na;

                xf_Value nb = xf_coerce_num(b);
                if (nb.state != XF_STATE_OK) {
                    xf_value_release(na);
                    return nb;
                }

                xf_Value out = xf_val_ok_num(na.data.num + nb.data.num);
                xf_value_release(na);
                xf_value_release(nb);
                return out;
            }

        case BINOP_SUB:
            if ((a.state == XF_STATE_OK && a.type == XF_TYPE_ARR) ||
                (b.state == XF_STATE_OK && b.type == XF_TYPE_ARR))
                return arr_broadcast(a, b, 1);
            {
                xf_Value na = xf_coerce_num(a);
                if (na.state != XF_STATE_OK) return na;

                xf_Value nb = xf_coerce_num(b);
                if (nb.state != XF_STATE_OK) {
                    xf_value_release(na);
                    return nb;
                }

                xf_Value out = xf_val_ok_num(na.data.num - nb.data.num);
                xf_value_release(na);
                xf_value_release(nb);
                return out;
            }

        case BINOP_MUL:
            if ((a.state == XF_STATE_OK && a.type == XF_TYPE_ARR) ||
                (b.state == XF_STATE_OK && b.type == XF_TYPE_ARR))
                return arr_broadcast(a, b, 2);
            {
                xf_Value na = xf_coerce_num(a);
                if (na.state != XF_STATE_OK) return na;

                xf_Value nb = xf_coerce_num(b);
                if (nb.state != XF_STATE_OK) {
                    xf_value_release(na);
                    return nb;
                }

                xf_Value out = xf_val_ok_num(na.data.num * nb.data.num);
                xf_value_release(na);
                xf_value_release(nb);
                return out;
            }

        case BINOP_DIV:
            if ((a.state == XF_STATE_OK && a.type == XF_TYPE_ARR) ||
                (b.state == XF_STATE_OK && b.type == XF_TYPE_ARR))
                return arr_broadcast(a, b, 3);
            {
                xf_Value na = xf_coerce_num(a);
                if (na.state != XF_STATE_OK) return na;

                xf_Value nb = xf_coerce_num(b);
                if (nb.state != XF_STATE_OK) {
                    xf_value_release(na);
                    return nb;
                }

                if (nb.data.num == 0.0) {
                    xf_value_release(na);
                    xf_value_release(nb);
                    return xf_val_err(
                        xf_err_new("division by zero",
                                   loc.source ? loc.source : "<unknown>",
                                   loc.line, loc.col),
                        XF_TYPE_NUM
                    );
                }

                xf_Value out = xf_val_ok_num(na.data.num / nb.data.num);
                xf_value_release(na);
                xf_value_release(nb);
                return out;
            }

        case BINOP_MOD:
            if ((a.state == XF_STATE_OK && a.type == XF_TYPE_ARR) ||
                (b.state == XF_STATE_OK && b.type == XF_TYPE_ARR))
                return arr_broadcast(a, b, 4);
            {
                xf_Value na = xf_coerce_num(a);
                if (na.state != XF_STATE_OK) return na;

                xf_Value nb = xf_coerce_num(b);
                if (nb.state != XF_STATE_OK) {
                    xf_value_release(na);
                    return nb;
                }

                if (nb.data.num == 0.0) {
                    xf_value_release(na);
                    xf_value_release(nb);
                    return xf_val_err(
                        xf_err_new("modulo by zero",
                                   loc.source ? loc.source : "<unknown>",
                                   loc.line, loc.col),
                        XF_TYPE_NUM
                    );
                }

                xf_Value out = xf_val_ok_num(fmod(na.data.num, nb.data.num));
                xf_value_release(na);
                xf_value_release(nb);
                return out;
            }

        case BINOP_CONCAT:
            return val_concat(a, b);

        case BINOP_MADD:
            return arr_broadcast(a, b, 0);

        case BINOP_MSUB:
            return arr_broadcast(a, b, 1);

        case BINOP_MMUL:
            return mat_mul(a, b);

        case BINOP_MDIV:
            return arr_broadcast(a, b, 3);

        default:
            return xf_val_nav(XF_TYPE_VOID);
    }
}

static bool bind_loop_tuple(Interp *it, LoopBind *bind, xf_Value v, Loc loc) {
    if (!bind) return true;
    if (bind->kind != LOOP_BIND_TUPLE) {
        interp_error(it, loc, "internal error: expected tuple loop binding");
        return false;
    }

    if (v.state != XF_STATE_OK || v.type != XF_TYPE_TUPLE || !v.data.tuple) {
        interp_error(it, loc, "cannot destructure non-tuple value");
        return false;
    }

    size_t need = bind->as.tuple.count;
    size_t have = xf_tuple_len(v.data.tuple);

    if (have != need) {
        interp_error(it, loc, "tuple destructuring arity mismatch: expected %zu values, got %zu",
                     need, have);
        return false;
    }

    for (size_t i = 0; i < need; i++) {
        xf_Value elem = xf_tuple_get(v.data.tuple, i);
        if (!bind_loop_value(it, bind->as.tuple.items[i], elem, loc))
            return false;
    }

    return true;
}

static bool bind_loop_value(Interp *it, LoopBind *bind, xf_Value v, Loc loc) {
    if (!bind) return true;

    switch (bind->kind) {
        case LOOP_BIND_NAME: {
            Symbol *sym = sym_declare(it->syms, bind->as.name,
                                      SYM_VAR, XF_TYPE_VOID, loc);
            if (!sym) {
                interp_error(it, loc, "failed to bind loop variable '%s'",
                             ((bind->as.name && bind->as.name->data) ? bind->as.name->data : "<null>"));
                return false;
            }

            xf_value_release(sym->value);
            sym->value = xf_value_retain(v);
            sym->state = v.state;
            sym->is_defined = true;
            return true;
        }

        case LOOP_BIND_TUPLE:
            return bind_loop_tuple(it, bind, v, loc);

        default:
            interp_error(it, loc, "invalid loop binding");
            return false;
    }
}

static bool bind_loop_index_value(Interp *it,
                                  LoopBind *key_bind,
                                  LoopBind *val_bind,
                                  xf_Value keyv,
                                  xf_Value valv,
                                  Loc loc) {
    if (key_bind) {
        if (!bind_loop_value(it, key_bind, keyv, loc)) return false;
    }
    if (val_bind) {
        if (!bind_loop_value(it, val_bind, valv, loc)) return false;
    }
    return true;
}
void interp_init(Interp *it, SymTable *syms, VM *vm) {
    memset(it, 0, sizeof(*it));
    it->syms     = syms;
    it->vm       = vm;
    it->last_err = xf_val_null();
    if (vm) core_set_fn_caller(vm, syms, interp_exec_xf_fn);
}
void interp_free(Interp *it) {
    for (size_t i = 0; i < it->imp_prog_count; i++)
        ast_program_free(it->imp_progs[i]);
    free(it->imp_progs);
    it->imp_progs      = NULL;
    it->imp_prog_count = 0;
    it->imp_prog_cap   = 0;
}

/* ──────────────────────────────────────────────────────────────────────
 * interp_call_core — delegate to a core.MODULE.FN native function.
 *
 * Looks up the `core` module in the symbol table, traverses to the
 * named sub-module and function, then calls fn->native_v directly.
 * Returns NAV(VOID) if the path cannot be resolved (module not loaded,
 * function not found, or not a native callable).
 * ────────────────────────────────────────────────────────────────────── */
static xf_Value interp_call_core(Interp *it,
                                 const char *module, const char *fn_name,
                                 xf_Value *args, size_t argc) {
    Symbol *core_sym = sym_lookup(it->syms, "core", 4);
    if (!core_sym || core_sym->value.type != XF_TYPE_MODULE ||
        !core_sym->value.data.mod) return xf_val_nav(XF_TYPE_VOID);

    xf_Value sub = xf_module_get(core_sym->value.data.mod, module);
    if (sub.state != XF_STATE_OK || sub.type != XF_TYPE_MODULE || !sub.data.mod) {
        xf_value_release(sub);
        return xf_val_nav(XF_TYPE_VOID);
    }

    xf_Value fv = xf_module_get(sub.data.mod, fn_name);
    if (fv.state != XF_STATE_OK || fv.type != XF_TYPE_FN || !fv.data.fn) {
        xf_value_release(fv);
        xf_value_release(sub);
        return xf_val_nav(XF_TYPE_VOID);
    }

    xf_Fn *fn = fv.data.fn;
    if (!fn->is_native || !fn->native_v) {
        xf_value_release(fv);
        xf_value_release(sub);
        return xf_val_nav(XF_TYPE_VOID);
    }

    xf_Value ret = fn->native_v(args, argc);
    xf_value_release(fv);
    xf_value_release(sub);
    return ret;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

static char *xf_read_line_from_file(const char *path, int target_line) {
    if (!path || target_line <= 0) return NULL;
    if (strcmp(path, "<repl>") == 0) return NULL;

    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;

    char buf[4096];
    int line_no = 0;
    char *out = NULL;

    while (fgets(buf, sizeof(buf), fp)) {
        line_no++;
        if (line_no == target_line) {
            size_t len = strlen(buf);

            while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
                buf[--len] = '\0';
            }

            out = malloc(len + 1);
            if (out) memcpy(out, buf, len + 1);
            break;
        }
    }

    fclose(fp);
    return out;
}

static void xf_print_caret_line(const char *src_line, int col) {
    fprintf(stdout, "   | ");

    if (!src_line || col <= 1) {
        fprintf(stdout, "^\n");
        return;
    }

    int visual = 0;
    for (int i = 0; src_line[i] && i < col - 1; i++) {
        if (src_line[i] == '\t') {
            fputc('\t', stdout);
        } else {
            fputc(' ', stdout);
        }
        visual++;
    }

    (void)visual;
    fprintf(stdout, "^\n");
}
void interp_error(Interp *it, Loc loc, const char *fmt, ...) {
    if (!it) return;

    const char *src = loc.source ? loc.source : "<unknown>";

    char msg[1024];
    va_list ap;
    fmt = fmt ? fmt : "<null fmt>";
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt ? fmt : "<error>", ap);
    va_end(ap);

    fprintf(stderr, "ERR ──────CRAPPLES!!──────────────────────\n");
    fprintf(stderr, "  %s\n", msg);
    fprintf(stderr, "  --> %s:%u:%u\n", src, loc.line, loc.col);
    fprintf(stderr, "──────────────────────────────────────────\n");

    it->had_error = true;
}
static void xf_print_value(FILE *out, xf_Value v) {
    if (v.state == XF_STATE_TRUE)  { fputs("true",  out); return; }
    if (v.state == XF_STATE_FALSE) { fputs("false", out); return; }
        switch (v.state) {
        case XF_STATE_OK:
            break;
        default:
            fputs(XF_STATE_NAMES[v.state], out);
            return;
    }

    switch (v.type) {
        case XF_TYPE_NUM:
            fprintf(out, "%.15g", v.data.num);
            return;

        case XF_TYPE_STR:
            fputs(v.data.str ? v.data.str->data : "", out);
            return;
case XF_TYPE_COMPLEX: {
    double re = v.data.complex.re;
    double im = v.data.complex.im;

    if (re == 0.0) {
        fprintf(out, "%.15gi", im);
    } else if (im < 0.0) {
        fprintf(out, "%.15g%.15gi", re, im);
    } else {
        fprintf(out, "%.15g+%.15gi", re, im);
    }
    return;
}
        case XF_TYPE_ARR: {
            fputc('[', out);
            if (v.data.arr) {
                for (size_t i = 0; i < v.data.arr->len; i++) {
                    if (i > 0) fputs(", ", out);
                    xf_print_value(out, v.data.arr->items[i]);
                }
            }
            fputc(']', out);
            return;
        }
case XF_TYPE_TUPLE: {
    fputc('(', out);
    if (v.data.tuple) {
        size_t n = xf_tuple_len(v.data.tuple);
        for (size_t i = 0; i < n; i++) {
            if (i > 0) fputs(", ", out);
            xf_print_value(out, xf_tuple_get(v.data.tuple, i));
        }
        if (n == 1) fputc(',', out);
    }
    fputc(')', out);
    return;
}
                case XF_TYPE_MAP: {
            fputc('{', out);
            if (v.data.map) {
                for (size_t i = 0; i < v.data.map->order_len; i++) {
                    if (i > 0) fputs(", ", out);
                    xf_Str *k = v.data.map->order[i];
                    fputs(k ? k->data : "", out);
                    fputc(':', out);
                    xf_Value mv = xf_map_get(v.data.map, k);
                    xf_print_value(out, mv);
                }
            }
            fputc('}', out);
            return;
        }

        case XF_TYPE_SET: {
            fputc('{', out);
            if (v.data.map) {
                for (size_t i = 0; i < v.data.map->order_len; i++) {
                    if (i > 0) fputs(", ", out);
                    xf_Str *k = v.data.map->order[i];
                    fputs(k ? k->data : "", out);
                }
            }
            fputc('}', out);
            return;
        }

        case XF_TYPE_FN:
            fputs("<fn>", out);
            return;

        case XF_TYPE_VOID:
            fputs("void", out);
            return;

        default: {
            xf_Value sv = xf_coerce_str(v);
            if (sv.state == XF_STATE_OK && sv.data.str)
                fputs(sv.data.str->data, out);
            else
                fputs(XF_TYPE_NAMES[v.type], out);
            xf_value_release(sv);
            return;
        }
        printf("\n\n\n");
    }
}
void interp_type_err(Interp *it, Loc loc,
                     const char *op, uint8_t got, uint8_t expected) {
    interp_error(it, loc, "type error in '%s': got %s, expected %s",
                 op, XF_TYPE_NAMES[got], XF_TYPE_NAMES[expected]);
}
static xf_Fn *build_fn(xf_Str *name, uint8_t ret,
                        Param *params, size_t pc, Stmt *body) {
    xf_Fn *fn = calloc(1, sizeof(xf_Fn));
    atomic_store(&fn->refcount, 1);   /* must be 1: xf_fn_release checks fetch_sub==1 */
    fn->name        = xf_str_retain(name);
    fn->return_type = ret;
    fn->param_count = pc;
    fn->body        = body;
    fn->is_native   = false;
    if (pc > 0) {
        fn->params = calloc(pc, sizeof(xf_param_t));
        for (size_t i = 0; i < pc; i++) {
            fn->params[i].name = xf_str_retain(params[i].name);
            fn->params[i].type = params[i].type;
        }
    }
    return fn;
}
static bool is_truthy(xf_Value v) {
    /* boolean states resolve without touching the data union */
    if (v.state == XF_STATE_TRUE)  return true;
    if (v.state == XF_STATE_FALSE) return false;
    if (v.state != XF_STATE_OK) return false;
    if (v.type == XF_TYPE_BOOL) return v.data.num != 0.0;
    if (v.type == XF_TYPE_NUM) return v.data.num != 0.0;
    if (v.type == XF_TYPE_STR) return v.data.str && v.data.str->len > 0;
    if (v.type == XF_TYPE_ARR) return v.data.arr && v.data.arr->len > 0;
    if (v.type == XF_TYPE_MAP || v.type == XF_TYPE_SET)
        return v.data.map && v.data.map->used > 0;
    /* fn, regex, module — non-null pointer is truthy */
    return true;
}
static xf_Value make_bool(bool b) {
    return b ? xf_val_true() : xf_val_false();
}
static int val_cmp(xf_Value a, xf_Value b) {
    if (a.state != XF_STATE_OK && b.state != XF_STATE_OK)
        return (int)a.state - (int)b.state;

    xf_Value na = xf_coerce_num(a), nb = xf_coerce_num(b);
    if (na.state == XF_STATE_OK && nb.state == XF_STATE_OK) {
        int out = 0;
        if (na.data.num < nb.data.num) out = -1;
        else if (na.data.num > nb.data.num) out = 1;
        xf_value_release(na);
        xf_value_release(nb);
        return out;
    }
    xf_value_release(na);
    xf_value_release(nb);

    xf_Value sa = xf_coerce_str(a), sb = xf_coerce_str(b);
    int cmp = 0;
    if (sa.state == XF_STATE_OK && sb.state == XF_STATE_OK && sa.data.str && sb.data.str)
        cmp = strcmp(sa.data.str->data, sb.data.str->data);
    xf_value_release(sa);
    xf_value_release(sb);
    return cmp;
}
static xf_Value val_concat(xf_Value a, xf_Value b) {
    xf_Value sa = xf_coerce_str(a), sb = xf_coerce_str(b);
    if (sa.state != XF_STATE_OK) { xf_value_release(sb); return sa; }
    if (sb.state != XF_STATE_OK) { xf_value_release(sa); return sb; }
    size_t la = sa.data.str ? sa.data.str->len : 0;
    size_t lb = sb.data.str ? sb.data.str->len : 0;
    char *buf = malloc(la + lb + 1);
    if (la) memcpy(buf, sa.data.str->data, la);
    if (lb) memcpy(buf + la, sb.data.str->data, lb);
    buf[la+lb] = '\0';
    xf_Str *s = xf_str_new(buf, la+lb);
    free(buf);
    xf_Value r = xf_val_ok_str(s);
    xf_str_release(s);
    /* release coerced copies — coerce_str retains for STR type, so always release */
    xf_value_release(sa);
    xf_value_release(sb);
    return r;
}
static double scalar_op(double a, double b, int op) {
    switch(op){case 0:return a+b;case 1:return a-b;case 2:return a*b;
    case 3:return b!=0?a/b:0;case 4:return b!=0?fmod(a,b):0;case 5:return pow(a,b);default:return 0;}
}
static xf_Value elem_op(xf_Value a, xf_Value b, int op);
static xf_Value arr_broadcast(xf_Value a, xf_Value b, int op) {
    bool aa=(a.state==XF_STATE_OK&&a.type==XF_TYPE_ARR&&a.data.arr);
    bool ba=(b.state==XF_STATE_OK&&b.type==XF_TYPE_ARR&&b.data.arr);
    if(aa&&ba){size_t len=a.data.arr->len>b.data.arr->len?a.data.arr->len:b.data.arr->len;xf_arr_t*out=xf_arr_new();for(size_t i=0;i<len;i++){xf_Value av=i<a.data.arr->len?a.data.arr->items[i]:xf_val_ok_num(0);xf_Value bv=i<b.data.arr->len?b.data.arr->items[i]:xf_val_ok_num(0);xf_arr_push(out,elem_op(av,bv,op));}xf_Value r=xf_val_ok_arr(out);xf_arr_release(out);return r;}
    if(aa){xf_arr_t*out=xf_arr_new();for(size_t i=0;i<a.data.arr->len;i++)xf_arr_push(out,elem_op(a.data.arr->items[i],b,op));xf_Value r=xf_val_ok_arr(out);xf_arr_release(out);return r;}
    if(ba){xf_arr_t*out=xf_arr_new();for(size_t i=0;i<b.data.arr->len;i++)xf_arr_push(out,elem_op(a,b.data.arr->items[i],op));xf_Value r=xf_val_ok_arr(out);xf_arr_release(out);return r;}
    return xf_val_nav(XF_TYPE_NUM);
}
static xf_Value elem_op(xf_Value a, xf_Value b, int op) {
    if ((a.state == XF_STATE_OK && a.type == XF_TYPE_ARR) ||
        (b.state == XF_STATE_OK && b.type == XF_TYPE_ARR))
        return arr_broadcast(a, b, op);

    xf_Value na = xf_coerce_num(a), nb = xf_coerce_num(b);
    xf_Value out = xf_val_ok_num(
        scalar_op((na.state == XF_STATE_OK) ? na.data.num : 0,
                  (nb.state == XF_STATE_OK) ? nb.data.num : 0, op)
    );
    xf_value_release(na);
    xf_value_release(nb);
    return out;
}
static xf_Value mat_mul(xf_Value a, xf_Value b) {
    if(!(a.state==XF_STATE_OK&&a.type==XF_TYPE_ARR&&a.data.arr)) return xf_val_nav(XF_TYPE_ARR);
    if(!(b.state==XF_STATE_OK&&b.type==XF_TYPE_ARR&&b.data.arr)) return xf_val_nav(XF_TYPE_ARR);
    size_t rows=a.data.arr->len;
    size_t k_max=(rows>0&&a.data.arr->items[0].type==XF_TYPE_ARR&&a.data.arr->items[0].data.arr)?a.data.arr->items[0].data.arr->len:1;
    size_t cols=(b.data.arr->len>0&&b.data.arr->items[0].state==XF_STATE_OK&&b.data.arr->items[0].type==XF_TYPE_ARR&&b.data.arr->items[0].data.arr)?b.data.arr->items[0].data.arr->len:1;
    xf_arr_t *result=xf_arr_new();
    for(size_t i=0;i<rows;i++){
        xf_arr_t *row=xf_arr_new();
        for(size_t j=0;j<cols;j++){
            double sum=0;
            for(size_t k=0;k<k_max;k++){
                double aik=0,bkj=0;
                xf_Value ar=a.data.arr->items[i];
                if (ar.state == XF_STATE_OK && ar.type == XF_TYPE_ARR && ar.data.arr && k < ar.data.arr->len) {
    xf_Value n = xf_coerce_num(ar.data.arr->items[k]);
    if (n.state == XF_STATE_OK) aik = n.data.num;
    xf_value_release(n);
}
                else if(ar.state==XF_STATE_OK&&ar.type==XF_TYPE_NUM&&k==0)aik=ar.data.num;
                if(k<b.data.arr->len){xf_Value br=b.data.arr->items[k];
if (br.state == XF_STATE_OK && br.type == XF_TYPE_ARR && br.data.arr && j < br.data.arr->len) {
    xf_Value n = xf_coerce_num(br.data.arr->items[j]);
    if (n.state == XF_STATE_OK) bkj = n.data.num;
    xf_value_release(n);
}
                                        else if(br.state==XF_STATE_OK&&br.type==XF_TYPE_NUM&&j==0)bkj=br.data.num;}
                sum+=aik*bkj;
            }
            xf_arr_push(row,xf_val_ok_num(sum));
        }
        xf_Value rv=xf_val_ok_arr(row);xf_arr_release(row);xf_arr_push(result,rv);
    }
    xf_Value r=xf_val_ok_arr(result);xf_arr_release(result);return r;
}
static xf_Value apply_assign_op(Interp *it,AssignOp op, xf_Value cur, xf_Value rhs, Loc loc) {

switch (op) {
    case ASSIGNOP_EQ:     return rhs;
    case ASSIGNOP_ADD:    return apply_binary_op(it, BINOP_ADD,    cur, rhs, loc);
    case ASSIGNOP_SUB:    return apply_binary_op(it, BINOP_SUB,    cur, rhs, loc);
    case ASSIGNOP_MUL:    return apply_binary_op(it, BINOP_MUL,    cur, rhs, loc);
    case ASSIGNOP_MADD:   return apply_binary_op(it, BINOP_MADD,   cur, rhs, loc);
    case ASSIGNOP_MSUB:   return apply_binary_op(it, BINOP_MSUB,   cur, rhs, loc);
    case ASSIGNOP_MMUL:   return apply_binary_op(it, BINOP_MMUL,   cur, rhs, loc);
    case ASSIGNOP_MDIV:   return apply_binary_op(it, BINOP_MDIV,   cur, rhs, loc);
    case ASSIGNOP_DIV:    return apply_binary_op(it, BINOP_DIV,    cur, rhs, loc);
    case ASSIGNOP_MOD:    return apply_binary_op(it, BINOP_MOD,    cur, rhs, loc);
    case ASSIGNOP_CONCAT: return apply_binary_op(it, BINOP_CONCAT, cur, rhs, loc);
    default:              return rhs;
}

}
static xf_Value lvalue_load(Interp *it, Expr *target);
static bool lvalue_store(Interp *it, Expr *target, xf_Value val) {
    if (target->kind == EXPR_IDENT) {
        return sym_assign(it->syms, target->as.ident.name, val);
    }
    if (target->kind == EXPR_IVAR) {
        xf_Value sv = xf_coerce_str(val);
        const char *s = (sv.state == XF_STATE_OK && sv.data.str)
                         ? sv.data.str->data : "";
        VM *vm = it->vm;
        bool _ivar_matched = false;
        switch (target->as.ivar.var) {
            case TK_VAR_FS:  strncpy(IT_REC(it)->fs,  s, sizeof(IT_REC(it)->fs)-1);  _ivar_matched = true; break;
            case TK_VAR_RS:  strncpy(IT_REC(it)->rs,  s, sizeof(IT_REC(it)->rs)-1);  _ivar_matched = true; break;
            case TK_VAR_OFS: strncpy(IT_REC(it)->ofs, s, sizeof(IT_REC(it)->ofs)-1); _ivar_matched = true; break;
            case TK_VAR_ORS: strncpy(IT_REC(it)->ors, s, sizeof(IT_REC(it)->ors)-1); _ivar_matched = true; break;
            default: break;
        }
        xf_value_release(sv);
        if (_ivar_matched) return true;
    }
    if (target->kind == EXPR_SUBSCRIPT) {
    Expr *obj_expr = target->as.subscript.obj;
    xf_Value key   = interp_eval_expr(it, target->as.subscript.key);
    if (key.state != XF_STATE_OK) {
        interp_error(it, target->loc, "subscript key is not OK");
        xf_value_release(key);
        return false;
    }

    xf_Value container = lvalue_load(it, obj_expr);

    if (container.state == XF_STATE_OK && container.type == XF_TYPE_TUPLE) {
        interp_error(it, target->loc, "cannot assign to tuple element");
        xf_value_release(key);
        xf_value_release(container);
        return false;
    }

    if (container.state != XF_STATE_OK ||
        (container.type != XF_TYPE_ARR && container.type != XF_TYPE_MAP)) {
        xf_Value num_key = xf_coerce_num(key);

        if (num_key.state == XF_STATE_OK) {
            xf_arr_t *a = xf_arr_new();
            xf_value_release(container);
            container = xf_val_ok_arr(a);
            xf_arr_release(a);
        } else {
            xf_map_t *m = xf_map_new();
            xf_value_release(container);
            container = xf_val_ok_map(m);
            xf_map_release(m);
        }

        xf_value_release(num_key);

        if (!lvalue_store(it, obj_expr, xf_value_retain(container))) {
            xf_value_release(key);
            xf_value_release(container);
            return false;
        }
    }

    if (container.type == XF_TYPE_ARR && container.data.arr) {
        xf_Value ni = xf_coerce_num(key);
        if (ni.state != XF_STATE_OK) {
            interp_error(it, target->loc, "array index must be numeric");
            xf_value_release(ni);
            xf_value_release(key);
            xf_value_release(container);
            return false;
        }

        xf_arr_set(container.data.arr, (size_t)ni.data.num, xf_value_retain(val));

        xf_value_release(ni);
        xf_value_release(key);
        xf_value_release(container);
        return true;
    }

    if (container.type == XF_TYPE_MAP && container.data.map) {
        xf_Value sk = xf_coerce_str(key);
        if (sk.state != XF_STATE_OK || !sk.data.str) {
            xf_value_release(sk);
            xf_value_release(key);
            xf_value_release(container);
            interp_error(it, target->loc, "map key must be a string");
            return false;
        }

xf_map_set(container.data.map, sk.data.str, val);
        xf_value_release(sk);
        xf_value_release(key);
        xf_value_release(container);
        return true;
    }

    xf_value_release(key);
    xf_value_release(container);
    interp_error(it, target->loc, "cannot index into type '%s'",
                 XF_TYPE_NAMES[container.type]);
    return false;
}
    /* ── tuple destructuring: (a, b) = expr ──────────────────────────
     * RHS is a tuple  → unpack element-by-element.
     * RHS is a scalar → broadcast the same value to every target.   */
    if (target->kind == EXPR_TUPLE_LIT) {
        size_t n = target->as.tuple_lit.count;
        bool is_tuple_rhs = (val.state == XF_STATE_OK &&
                             val.type  == XF_TYPE_TUPLE &&
                             val.data.tuple);
        size_t have = is_tuple_rhs ? xf_tuple_len(val.data.tuple) : 0;
        for (size_t i = 0; i < n; i++) {
            xf_Value elem;
            if (is_tuple_rhs) {
                elem = (i < have)
                    ? xf_value_retain(xf_tuple_get(val.data.tuple, i))
                    : xf_val_nav(XF_TYPE_VOID);
            } else {
                elem = xf_value_retain(val);   /* broadcast scalar */
            }
            bool ok = lvalue_store(it, target->as.tuple_lit.items[i], xf_value_retain(elem));
            xf_value_release(elem);
            if (!ok) return false;
        }
        return true;
    }

    interp_error(it, target->loc, "invalid assignment target");
    return false;
}
static xf_Value lvalue_load(Interp *it, Expr *target) {
    if (target->kind == EXPR_IDENT) {
        Symbol *s = sym_lookup_str(it->syms, target->as.ident.name);
        if (!s) {
            interp_error(it, target->loc, "undetermined variable '%s'",
                         ((target->as.ident.name && target->as.ident.name->data) ? target->as.ident.name->data : "<null-ident>"));
            return xf_val_nav(XF_TYPE_BOOL);
        }
        return xf_value_retain(s->value);
    }
    return interp_eval_expr(it, target);
}
static size_t xf_sprintf_impl(char *out, size_t cap,
                               const char *fmt,
                               xf_Value *args, size_t argc) {
    size_t wi = 0;
    size_t ai = 0;
#define PUTC(c) do { if (wi < cap-1) out[wi++] = (c); } while(0)
#define PUTS(s) do { const char *_s=(s); while(*_s && wi<cap-1) out[wi++]=*_s++; } while(0)
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') { PUTC(*p); continue; }
        p++;
        if (!*p) break;
        if (*p == '%') { PUTC('%'); continue; }
        char flags[8] = {0}; int nf = 0;
        while (*p == '-' || *p == '+' || *p == ' ' || *p == '0' || *p == '#')
            flags[nf++] = *p++;
        char wbuf[16] = {0}; int wn = 0;
        while (isdigit((unsigned char)*p)) wbuf[wn++] = *p++;
        char pbuf[16] = {0};
        if (*p == '.') { p++; int pn=0; while(isdigit((unsigned char)*p)) pbuf[pn++]=*p++; }
        char subfmt[64];
        snprintf(subfmt, sizeof(subfmt), "%%%s%s%s%c",
                 flags, wbuf, *pbuf ? "." : "", *p ? *p : 's');
        if (*pbuf) snprintf(subfmt, sizeof(subfmt), "%%%s%s.%s%c",
                             flags, wbuf, pbuf, *p ? *p : 's');
        xf_Value av = (ai < argc) ? args[ai++] : xf_val_ok_num(0);
        char tmp[512];
        switch (*p) {
            case 'd': case 'i': {
                xf_Value n = xf_coerce_num(av);
                snprintf(tmp, sizeof(tmp), subfmt, (long long)(n.state==XF_STATE_OK?n.data.num:0));
                PUTS(tmp); break;
            }
            case 'u': {
                xf_Value n = xf_coerce_num(av);
                snprintf(tmp, sizeof(tmp), subfmt, (unsigned long long)(n.state==XF_STATE_OK?n.data.num:0));
                PUTS(tmp); break;
            }
            case 'o': case 'x': case 'X': {
                xf_Value n = xf_coerce_num(av);
                snprintf(tmp, sizeof(tmp), subfmt, (unsigned long long)(n.state==XF_STATE_OK?n.data.num:0));
                PUTS(tmp); break;
            }
            case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': {
                xf_Value n = xf_coerce_num(av);
                snprintf(tmp, sizeof(tmp), subfmt, n.state==XF_STATE_OK?n.data.num:0.0);
                PUTS(tmp); break;
            }
            case 's': {
                xf_Value s = xf_coerce_str(av);
                snprintf(tmp, sizeof(tmp), subfmt,
                         (s.state==XF_STATE_OK&&s.data.str) ? s.data.str->data : "");
                xf_value_release(s);
                PUTS(tmp); break;
            }
            case 'c': {
                xf_Value n = xf_coerce_num(av);
                char c = (n.state==XF_STATE_OK) ? (char)(int)n.data.num : '?';
                PUTC(c); break;
            }
            default:
                PUTC('%'); PUTC(*p); break;
        }
    }
    out[wi] = '\0';
    return wi;
#undef PUTC
#undef PUTS
}
static void csv_quote(FILE *f, const char *s) {
    bool needs = (strchr(s, ',') || strchr(s, '"') || strchr(s, '\n'));
    if (!needs) { fputs(s, f); return; }
    fputc('"', f);
    for (; *s; s++) { if (*s == '"') fputc('"', f); fputc(*s, f); }
    fputc('"', f);
}
static void tsv_escape(FILE *f, const char *s) {
    for (; *s; s++) {
        if (*s == '\t') fputs("\\t", f);
        else            fputc(*s, f);
    }
}
static void json_str(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s; s++) {
        switch (*s) {
            case '"':  fputs("\\\"", f); break;
            case '\\': fputs("\\\\", f); break;
            case '\n': fputs("\\n",  f); break;
            case '\r': fputs("\\r",  f); break;
            case '\t': fputs("\\t",  f); break;
            default:   fputc(*s, f);     break;
        }
    }
    fputc('"', f);
}
static void print_structured(Interp *it, xf_Value *vals, size_t count,
                              uint8_t mode) {
    RecordCtx *_rc = IT_REC(it);
    switch (mode) {
        case XF_OUTFMT_CSV:
            for (size_t i = 0; i < count; i++) {
                if (i > 0) fputc(',', stdout);
                xf_Value sv = xf_coerce_str(vals[i]);
                csv_quote(stdout, (sv.state==XF_STATE_OK&&sv.data.str)
                                  ? sv.data.str->data : "");
                xf_value_release(sv);
            }
            fputs(_rc->ors, stdout);
            break;
        case XF_OUTFMT_TSV:
            for (size_t i = 0; i < count; i++) {
                if (i > 0) fputc('\t', stdout);
                xf_Value sv = xf_coerce_str(vals[i]);
                tsv_escape(stdout, (sv.state==XF_STATE_OK&&sv.data.str)
                                   ? sv.data.str->data : "");
                xf_value_release(sv);
            }
            fputs(_rc->ors, stdout);
            break;
        case XF_OUTFMT_JSON: {
            fputc('{', stdout);
            for (size_t i = 0; i < count; i++) {
                if (i > 0) fputc(',', stdout);
                if (i < _rc->header_count)
                    json_str(stdout, _rc->headers[i]);
                else {
                    char key[32]; snprintf(key, sizeof(key), "f%zu", i+1);
                    json_str(stdout, key);
                }
                fputc(':', stdout);
                xf_Value sv = xf_coerce_str(vals[i]);
                json_str(stdout, (sv.state==XF_STATE_OK&&sv.data.str)
                                 ? sv.data.str->data : "");
                xf_value_release(sv);
            }
            fputs("}", stdout);
            fputs(_rc->ors, stdout);
            break;
        }
        default:
            for (size_t i = 0; i < count; i++) {
                if (i > 0) fputs(_rc->ofs, stdout);
                xf_Value sv = xf_coerce_str(vals[i]);
                if (sv.state==XF_STATE_OK && sv.data.str)
                    fputs(sv.data.str->data, stdout);
                else
                    fputs(XF_STATE_NAMES[vals[i].state], stdout);
                xf_value_release(sv);
            }
            fputs(_rc->ors, stdout);
            break;
    }
}
/* ── builtin dispatch table ──────────────────────────────────────────
 * Maps bare function name → (module, fn_name) for core delegation.
 * Sorted by name so future binary search is possible; currently hashed
 * via FNV-1a for O(1) average dispatch without a giant strcmp chain.
 * ─────────────────────────────────────────────────────────────────── */
typedef struct { const char *name; const char *mod; const char *fn; } CoreRoute;

static const CoreRoute k_core_routes[] = {
    { "abs",     "math", "abs"         },
    { "column",  "str",  "column"      },
    { "cos",     "math", "cos"         },
    { "gsub",    "str",  "replace_all" },
    { "index",   "str",  "index"       },
    { "int",     "math", "int"         },
    { "len",     "str",  "len"         },
    { "lower",   "str",  "lower"       },
    { "match",   "regex",    "match"  },
    { "rand",    "math", "rand"        },
    { "sin",     "math", "sin"         },
    { "split",   "generics", "split"  },
    { "sprintf", "str",  "sprintf"     },
    { "sqrt",    "math", "sqrt"        },
    { "srand",   "math", "srand"       },
    { "sub",     "str",  "replace"     },
    { "substr",  "str",  "substr"      },
    { "time",    "os",       "time"   },
    { "tolower", "str",  "lower"       },
    { "toupper", "str",  "upper"       },
    { "trim",    "str",  "trim"        },
    { "upper",   "str",  "upper"       },
};
#define CORE_ROUTE_COUNT (sizeof(k_core_routes)/sizeof(k_core_routes[0]))

static int core_route_cmp(const void *key, const void *elem) {
    return strcmp((const char *)key, ((const CoreRoute *)elem)->name);
}

/* Lookup name in the sorted route table; returns NULL if not found. */
static const CoreRoute *find_core_route(const char *name) {
    return (const CoreRoute *)bsearch(name, k_core_routes, CORE_ROUTE_COUNT,
                                      sizeof(CoreRoute), core_route_cmp);
}
xf_Value interp_call_builtin(Interp *it, const char *name, xf_Value *args, size_t argc) {
    (void)it;

    if (strcmp(name, "print") == 0) {
        for (size_t i = 0; i < argc; i++) {
            xf_Value sv = xf_coerce_str(args[i]);
            if (sv.state == XF_STATE_OK && sv.data.str) {
                fputs(sv.data.str->data, stdout);
            } else {
                fputs(XF_STATE_NAMES[args[i].state], stdout);
            }
            xf_value_release(sv);
            if (i + 1 < argc) fputc(' ', stdout);
        }
        fputc('\n', stdout);
        return xf_val_null();
    }

    if (strcmp(name, "len") == 0 && argc == 1) {
        xf_Value v = args[0];
        if (v.state != XF_STATE_OK) return xf_value_retain(v);

        switch (v.type) {
            case XF_TYPE_STR:
                return xf_val_ok_num(v.data.str ? (double)v.data.str->len : 0.0);
            case XF_TYPE_ARR:
                return xf_val_ok_num(v.data.arr ? (double)v.data.arr->len : 0.0);
            case XF_TYPE_TUPLE:
                return xf_val_ok_num(v.data.tuple ? (double)xf_tuple_len(v.data.tuple) : 0.0);
            case XF_TYPE_MAP:
            case XF_TYPE_SET:
                return xf_val_ok_num(v.data.map ? (double)v.data.map->order_len : 0.0);
            default:
                return xf_val_nav(XF_TYPE_NUM);
        }
    }

    if (strcmp(name, "push") == 0 && argc == 2) {
    if (args[0].state == XF_STATE_OK &&
        args[0].type  == XF_TYPE_ARR &&
        args[0].data.arr) {
        xf_arr_push(args[0].data.arr, xf_value_retain(args[1]));
    } else if (args[0].state == XF_STATE_OK &&
               args[0].type  == XF_TYPE_SET &&
               args[0].data.map) {
        xf_Value ks = xf_coerce_str(args[1]);
        if (ks.state == XF_STATE_OK && ks.data.str) {
            xf_map_set(args[0].data.map, ks.data.str, xf_val_ok_num(1.0));
        }
        xf_value_release(ks);
    }
    return xf_value_retain(args[0]);
}
    if (strcmp(name, "pop") == 0 && argc == 1) {
        if (args[0].state == XF_STATE_OK && args[0].type == XF_TYPE_ARR && args[0].data.arr) {
            return xf_arr_pop(args[0].data.arr);
        }
        return xf_val_nav(XF_TYPE_VOID);
    }

    if (strcmp(name, "unshift") == 0 && argc == 2) {
    if (args[0].state == XF_STATE_OK &&
        args[0].type  == XF_TYPE_ARR &&
        args[0].data.arr) {
        xf_arr_unshift(args[0].data.arr, xf_value_retain(args[1]));
    }
    return xf_value_retain(args[0]);
}
    if (strcmp(name, "shift") == 0 && argc == 1) {
        if (args[0].state == XF_STATE_OK && args[0].type == XF_TYPE_ARR && args[0].data.arr) {
            return xf_arr_shift(args[0].data.arr);
        }
        return xf_val_nav(XF_TYPE_VOID);
    }

    if (strcmp(name, "remove") == 0 && argc == 2) {
        xf_Value coll = args[0];
        if (coll.state != XF_STATE_OK) return xf_value_retain(coll);

        if (coll.type == XF_TYPE_ARR && coll.data.arr) {
            xf_Value ni = xf_coerce_num(args[1]);
            if (ni.state == XF_STATE_OK) {
                xf_arr_delete(coll.data.arr, (size_t)ni.data.num);
            }
            xf_value_release(ni);
            return xf_value_retain(coll);
        }

        if ((coll.type == XF_TYPE_MAP || coll.type == XF_TYPE_SET) && coll.data.map) {
            xf_Value ks = xf_coerce_str(args[1]);
            if (ks.state == XF_STATE_OK && ks.data.str) {
                xf_map_delete(coll.data.map, ks.data.str);
            }
            xf_value_release(ks);
            return xf_value_retain(coll);
        }

        return xf_val_nav(XF_TYPE_VOID);
    }

    if (strcmp(name, "keys") == 0 && argc == 1) {
        if (args[0].state == XF_STATE_OK &&
            (args[0].type == XF_TYPE_MAP || args[0].type == XF_TYPE_SET) &&
            args[0].data.map) {
            xf_arr_t *out = xf_arr_new();
            if (!out) return xf_val_nav(XF_TYPE_ARR);

            xf_map_t *m = args[0].data.map;
            for (size_t i = 0; i < m->order_len; i++) {
                xf_arr_push(out, xf_val_ok_str(m->order[i]));
            }

            xf_Value v = xf_val_ok_arr(out);
            xf_arr_release(out);
            return v;
        }
        return xf_val_nav(XF_TYPE_ARR);
    }

    if (strcmp(name, "values") == 0 && argc == 1) {
        if (args[0].state == XF_STATE_OK && args[0].type == XF_TYPE_MAP && args[0].data.map) {
            xf_arr_t *out = xf_arr_new();
            if (!out) return xf_val_nav(XF_TYPE_ARR);

            xf_map_t *m = args[0].data.map;
            for (size_t i = 0; i < m->order_len; i++) {
                xf_Value v = xf_map_get(m, m->order[i]);
                xf_arr_push(out, xf_value_retain(v));
            }

            xf_Value rv = xf_val_ok_arr(out);
            xf_arr_release(out);
            return rv;
        }
        return xf_val_nav(XF_TYPE_ARR);
    }

    if (strcmp(name, "has") == 0 && argc == 2) {
        xf_Value coll = args[0], needle = args[1];
        if (coll.state != XF_STATE_OK) return xf_value_retain(coll);
        if (needle.state != XF_STATE_OK) return xf_value_retain(needle);

        if (coll.type == XF_TYPE_STR && coll.data.str) {
            xf_Value ns = xf_coerce_str(needle);
            if (ns.state != XF_STATE_OK || !ns.data.str) {
                if (ns.state == XF_STATE_OK) xf_value_release(ns);
                return xf_val_false();
            }

            bool found = strstr(coll.data.str->data, ns.data.str->data) != NULL;
            xf_value_release(ns);
            return found ? xf_val_true() : xf_val_false();
        }

        if (coll.type == XF_TYPE_ARR && coll.data.arr) {
            xf_Value ns = xf_coerce_str(needle);
            if (ns.state != XF_STATE_OK || !ns.data.str) {
                if (ns.state == XF_STATE_OK) xf_value_release(ns);
                return xf_val_false();
            }

            xf_arr_t *a = coll.data.arr;
            for (size_t i = 0; i < a->len; i++) {
                xf_Value es = xf_coerce_str(a->items[i]);
                bool match = (es.state == XF_STATE_OK &&
                              es.data.str &&
                              strcmp(es.data.str->data, ns.data.str->data) == 0);
                xf_value_release(es);
                if (match) {
                    xf_value_release(ns);
                    return xf_val_true();
                }
            }

            xf_value_release(ns);
            return xf_val_false();
        }

        if ((coll.type == XF_TYPE_MAP || coll.type == XF_TYPE_SET) && coll.data.map) {
            xf_Value ks = xf_coerce_str(needle);
            if (ks.state != XF_STATE_OK || !ks.data.str) {
                if (ks.state == XF_STATE_OK) xf_value_release(ks);
                return xf_val_false();
            }

            xf_Value got = xf_map_get(coll.data.map, ks.data.str);
            xf_value_release(ks);
            return got.state == XF_STATE_OK ? xf_val_true() : xf_val_false();
        }

        return xf_val_false();
    }

    if (strcmp(name, "type") == 0 && argc == 1) {
        xf_Value v = args[0];
        uint8_t t = XF_STATE_IS_BOOL(v.state) ? XF_TYPE_BOOL : v.type;
        if (t >= XF_TYPE_COUNT) t = XF_TYPE_VOID;

        xf_Str *s = xf_str_from_cstr(XF_TYPE_NAMES[t]);
        if (!s) return xf_val_nav(XF_TYPE_STR);

        xf_Value out = xf_val_ok_str(s);
        xf_str_release(s);
        return out;
    }

    if (strcmp(name, "state") == 0 && argc == 1) {
        xf_Value v = args[0];
        uint8_t st = (v.state < XF_STATE_COUNT) ? v.state : XF_STATE_OK;

        xf_Str *s = xf_str_from_cstr(XF_STATE_NAMES[st]);
        if (!s) return xf_val_nav(XF_TYPE_STR);

        xf_Value out = xf_val_ok_str(s);
        xf_str_release(s);
        return out;
    }

    return xf_val_nav(XF_TYPE_VOID);
}
xf_Value interp_eval_expr(Interp *it, Expr *e) {
    if (!e) return xf_val_null();
    if (it->had_error || it->returning || it->exiting || it->nexting)
        return xf_val_null();

    switch (e->kind) {
    case EXPR_NUM:
        return xf_val_ok_num(e->as.num);

    case EXPR_STR: {
        xf_Value v = xf_val_ok_str(e->as.str.value);
        return v;
    }

    case EXPR_REGEX: {
        xf_Regex *re = calloc(1, sizeof(xf_Regex));
        if (!re) return xf_val_nav(XF_TYPE_REGEX);
        atomic_init(&re->refcount, 1);
        re->pattern  = xf_str_retain(e->as.regex.pattern);
        re->flags    = e->as.regex.flags;
        re->compiled = NULL;
        return xf_val_ok_re(re);
    }

    case EXPR_TUPLE_LIT: {
        size_t n = e->as.tuple_lit.count;
        xf_Value *items = n ? malloc(sizeof(xf_Value) * n) : NULL;

        for (size_t i = 0; i < n; i++) {
            xf_Value v = interp_eval_expr(it, e->as.tuple_lit.items[i]);
            if (v.state != XF_STATE_OK) {
                for (size_t j = 0; j < i; j++) xf_value_release(items[j]);
                free(items);
                return v;
            }
            items[i] = v;
        }

        xf_tuple_t *t = xf_tuple_new(items, n);
        free(items);

        xf_Value out = xf_val_ok_tuple(t);
        xf_tuple_release(t);
        return out;
    }

    case EXPR_ARR_LIT: {
        xf_arr_t *a = xf_arr_new();
        for (size_t i = 0; i < e->as.arr_lit.count; i++) {
            Expr *item = e->as.arr_lit.items[i];
            if (item->kind == EXPR_SPREAD) {
                /* ..arr — expand each element into this array */
                xf_Value sv = interp_eval_expr(it, item->as.spread.operand);
                if (sv.state == XF_STATE_OK && sv.type == XF_TYPE_ARR && sv.data.arr) {
                    xf_arr_t *src = sv.data.arr;
                    for (size_t j = 0; j < src->len; j++)
                        xf_arr_push(a, xf_value_retain(src->items[j]));
                }
                xf_value_release(sv);
            } else {
                /* Include all values (OK, NAV, NULL, etc.) — don't propagate errors */
                xf_arr_push(a, interp_eval_expr(it, item));
            }
        }
        xf_Value out = xf_val_ok_arr(a);
        xf_arr_release(a);
        return out;
    }

    case EXPR_SPREAD: {
        /* spread outside array literal — evaluate the operand directly */
        return interp_eval_expr(it, e->as.spread.operand);
    }


case EXPR_MAP_LIT: {
    xf_map_t *m = xf_map_new();

    for (size_t i = 0; i < e->as.map_lit.count; i++) {
        xf_Value kv = interp_eval_expr(it, e->as.map_lit.keys[i]);
        if (kv.state != XF_STATE_OK) {
            xf_map_release(m);
            return kv;
        }

        xf_Value vv = interp_eval_expr(it, e->as.map_lit.vals[i]);

        xf_Value ks = xf_coerce_str(kv);
        if (ks.state != XF_STATE_OK) {
            xf_value_release(kv);
            xf_value_release(vv);
            xf_map_release(m);
            return ks;
        }

        xf_map_set(m, ks.data.str, vv);

        xf_value_release(kv);
        xf_value_release(ks);
        /* do NOT release vv here */
    }

    xf_Value out = xf_val_ok_map(m);
    xf_map_release(m);
    return out;
}
case EXPR_SET_LIT: {
    xf_map_t *m = xf_map_new();

    for (size_t i = 0; i < e->as.set_lit.count; i++) {
        xf_Value v = interp_eval_expr(it, e->as.set_lit.items[i]);
        if (v.state != XF_STATE_OK) {
            xf_map_release(m);
            return v;
        }

        if (v.type == XF_TYPE_SET || v.type == XF_TYPE_MAP || v.type == XF_TYPE_ARR) {
            char kb[32];
            snprintf(kb, sizeof(kb), "#%zu", i);
            xf_Str *ks = xf_str_from_cstr(kb);

            xf_map_set(m, ks, v);   /* map owns v now */
            xf_str_release(ks);
        } else {
            xf_Value ks = xf_coerce_str(v);
            if (ks.state != XF_STATE_OK) {
                xf_value_release(v);
                xf_map_release(m);
                return ks;
            }

            xf_map_set(m, ks.data.str, xf_val_ok_num(1.0));
            xf_value_release(ks);
            xf_value_release(v);
        }
    }

    xf_Value out = xf_val_ok_map(m);
    xf_map_release(m);
    out.type = XF_TYPE_SET;
    return out;
}
    case EXPR_IDENT: {
        Symbol *s = sym_lookup_str(it->syms, e->as.ident.name);
        if (!s) {
            interp_error(it, e->loc, "undetermined variable '%s'",
                         ((e->as.ident.name && e->as.ident.name->data) ? e->as.ident.name->data : "<null-ident>"));
            return xf_val_undet(XF_TYPE_BOOL);
        }
        return xf_value_retain(s->value);
    }

    case EXPR_FIELD: {
        int n = e->as.field.index;
        RecordCtx *_rc = IT_REC(it);

        if (n == 0) {
            if (!_rc->buf) return xf_val_ok_str(xf_str_from_cstr(""));
            xf_Str *s = xf_str_new(_rc->buf, _rc->buf_len);
            xf_Value v = xf_val_ok_str(s);
            xf_str_release(s);
            return v;
        }

        if (n > 0 && (size_t)n <= _rc->field_count) {
            xf_Str *s = xf_str_from_cstr(_rc->fields[n - 1]);
            xf_Value v = xf_val_ok_str(s);
            xf_str_release(s);
            return v;
        }

        return xf_val_ok_str(xf_str_from_cstr(""));
    }

    case EXPR_IVAR: {
        RecordCtx *_rc = IT_REC(it);
        switch (e->as.ivar.var) {
            case TK_VAR_NR:  return xf_val_ok_num((double)_rc->nr);
            case TK_VAR_NF:  return xf_val_ok_num((double)_rc->field_count);
            case TK_VAR_FNR: return xf_val_ok_num((double)_rc->fnr);
            case TK_VAR_FS:  { xf_Str *s = xf_str_from_cstr(_rc->fs);  xf_Value v = xf_val_ok_str(s); xf_str_release(s); return v; }
            case TK_VAR_RS:  { xf_Str *s = xf_str_from_cstr(_rc->rs);  xf_Value v = xf_val_ok_str(s); xf_str_release(s); return v; }
            case TK_VAR_OFS: { xf_Str *s = xf_str_from_cstr(_rc->ofs); xf_Value v = xf_val_ok_str(s); xf_str_release(s); return v; }
            case TK_VAR_ORS: { xf_Str *s = xf_str_from_cstr(_rc->ors); xf_Value v = xf_val_ok_str(s); xf_str_release(s); return v; }
            default: return xf_val_nav(XF_TYPE_VOID);
        }
    }

    case EXPR_SUBSCRIPT: {
        xf_Value obj = interp_eval_expr(it, e->as.subscript.obj);
        xf_Value key = interp_eval_expr(it, e->as.subscript.key);

        if (obj.state != XF_STATE_OK) {
            xf_value_release(key);
            return obj;
        }
        if (key.state != XF_STATE_OK) {
            xf_value_release(obj);
            return key;
        }

        xf_Value _sub_result = xf_val_nav(XF_TYPE_VOID);

        if (obj.type == XF_TYPE_ARR) {
            xf_Value ni = xf_coerce_num(key);
            if (ni.state == XF_STATE_OK && obj.data.arr) {
                if (ni.data.num < 0) {
                    interp_error(it, e->loc, "negative index");
                    _sub_result = xf_val_nav(XF_TYPE_VOID);
                } else {
                    size_t idx = (size_t)ni.data.num;
                    if (idx >= obj.data.arr->len) {
                        interp_error(it, e->loc, "array index out of range");
                        _sub_result = xf_val_nav(XF_TYPE_VOID);
                    } else {
                        _sub_result = xf_value_retain(xf_arr_get(obj.data.arr, idx));
                    }
                }
            }

        } else if (obj.type == XF_TYPE_TUPLE) {
            xf_Value ni = xf_coerce_num(key);
            if (ni.state == XF_STATE_OK) {
                if (ni.data.num < 0) {
                    interp_error(it, e->loc, "negative index");
                    _sub_result = xf_val_nav(XF_TYPE_VOID);
                } else {
                    size_t idx = (size_t)ni.data.num;
                    if (!obj.data.tuple || idx >= xf_tuple_len(obj.data.tuple)) {
                        interp_error(it, e->loc, "tuple index out of range");
                        _sub_result = xf_val_nav(XF_TYPE_VOID);
                    } else {
                        _sub_result = xf_value_retain(xf_tuple_get(obj.data.tuple, idx));
                    }
                }
            }

        } else if (obj.type == XF_TYPE_MAP) {
            xf_Value sk = xf_coerce_str(key);
            if (sk.state == XF_STATE_OK && sk.data.str && obj.data.map) {
                _sub_result = xf_value_retain(xf_map_get(obj.data.map, sk.data.str));
            }
            xf_value_release(sk);

        } else if (obj.type == XF_TYPE_STR && obj.data.str) {
            xf_Value ni = xf_coerce_num(key);
            if (ni.state == XF_STATE_OK) {
                if (ni.data.num < 0) {
                    interp_error(it, e->loc, "negative index");
                    _sub_result = xf_val_nav(XF_TYPE_STR);
                } else {
                    size_t idx = (size_t)ni.data.num;
                    if (idx < obj.data.str->len) {
                        char ch[2] = { obj.data.str->data[idx], '\0' };
                        xf_Str *cs = xf_str_new(ch, 1);
                        _sub_result = xf_val_ok_str(cs);
                        xf_str_release(cs);
                    } else {
                        _sub_result = xf_val_nav(XF_TYPE_STR);
                    }
                }
            } else {
                _sub_result = xf_val_nav(XF_TYPE_STR);
            }

        } else {
            interp_error(it, e->loc, "subscript on non-indexable type '%s'",
                         XF_TYPE_NAMES[obj.type]);
        }

        xf_value_release(obj);
        xf_value_release(key);
        return _sub_result;
    }

    case EXPR_SVAR: {
        RecordCtx *_rc = IT_REC(it);
        switch (e->as.svar.var) {
            case TK_VAR_FILE: {
                xf_Str *s = xf_str_from_cstr(_rc->current_file);
                xf_Value v = xf_val_ok_str(s);
                xf_str_release(s);
                return v;
            }
            case TK_VAR_MATCH:
                return xf_value_retain(_rc->last_match);
            case TK_VAR_CAPS:
                return xf_value_retain(_rc->last_captures);
            case TK_VAR_ERR:
                return xf_value_retain(it->last_err);
            default: {
                xf_Str *s = xf_str_from_cstr("");
                xf_Value v = xf_val_ok_str(s);
                xf_str_release(s);
                return v;
            }
        }
    }

    case EXPR_UNARY: {
        switch (e->as.unary.op) {
            case UNOP_NEG: {
                xf_Value v = xf_coerce_num(interp_eval_expr(it, e->as.unary.operand));
                if (v.state != XF_STATE_OK) return v;
                return xf_val_ok_num(-v.data.num);
            }

            case UNOP_NOT:
                return make_bool(!is_truthy(interp_eval_expr(it, e->as.unary.operand)));

            case UNOP_PRE_INC: {
                xf_Value v = xf_coerce_num(lvalue_load(it, e->as.unary.operand));
                if (v.state != XF_STATE_OK) return v;
                xf_Value nv = xf_val_ok_num(v.data.num + 1);
                lvalue_store(it, e->as.unary.operand, nv);
                return nv;
            }

            case UNOP_PRE_DEC: {
                xf_Value v = xf_coerce_num(lvalue_load(it, e->as.unary.operand));
                if (v.state != XF_STATE_OK) return v;
                xf_Value nv = xf_val_ok_num(v.data.num - 1);
                lvalue_store(it, e->as.unary.operand, nv);
                return nv;
            }

            case UNOP_POST_INC: {
                xf_Value old = xf_coerce_num(lvalue_load(it, e->as.unary.operand));
                if (old.state == XF_STATE_OK)
                    lvalue_store(it, e->as.unary.operand, xf_val_ok_num(old.data.num + 1));
                return old;
            }

            case UNOP_POST_DEC: {
                xf_Value old = xf_coerce_num(lvalue_load(it, e->as.unary.operand));
                if (old.state == XF_STATE_OK)
                    lvalue_store(it, e->as.unary.operand, xf_val_ok_num(old.data.num - 1));
                return old;
            }

            default:
                return xf_val_nav(XF_TYPE_VOID);
        }
    }

    case EXPR_BINARY: {
    if (e->as.binary.op == BINOP_AND) {
        xf_Value a = interp_eval_expr(it, e->as.binary.left);
        bool ta = is_truthy(a);
        xf_value_release(a);
        if (!ta) return make_bool(false);

        xf_Value b = interp_eval_expr(it, e->as.binary.right);
        bool tb = is_truthy(b);
        xf_value_release(b);
        return make_bool(tb);
    }

    if (e->as.binary.op == BINOP_OR) {
        xf_Value a = interp_eval_expr(it, e->as.binary.left);
        bool ta = is_truthy(a);
        xf_value_release(a);
        if (ta) return make_bool(true);

        xf_Value b = interp_eval_expr(it, e->as.binary.right);
        bool tb = is_truthy(b);
        xf_value_release(b);
        return make_bool(tb);
    }

    xf_Value a = interp_eval_expr(it, e->as.binary.left);
    xf_Value b = interp_eval_expr(it, e->as.binary.right);

    switch (e->as.binary.op) {
        case BINOP_ADD:
        case BINOP_SUB:
        case BINOP_MUL:
        case BINOP_DIV:
        case BINOP_MOD:
        case BINOP_CONCAT:
        case BINOP_MADD:
        case BINOP_MSUB:
        case BINOP_MMUL:
        case BINOP_MDIV: {
            xf_Value ret = apply_binary_op(it,e->as.binary.op, a, b, e->loc);
            xf_value_release(a);
            xf_value_release(b);
            return ret;
        }

        case BINOP_POW: {
            if ((a.state == XF_STATE_OK && a.type == XF_TYPE_ARR) ||
                (b.state == XF_STATE_OK && b.type == XF_TYPE_ARR)) {
                xf_Value ret = arr_broadcast(a, b, 5);
                xf_value_release(a);
                xf_value_release(b);
                return ret;
            }

            xf_Value na = xf_coerce_num(a);
            xf_Value nb = xf_coerce_num(b);
            xf_value_release(a);
            xf_value_release(b);

            if (na.state != XF_STATE_OK) return na;
            if (nb.state != XF_STATE_OK) {
                xf_value_release(na);
                return nb;
            }

            xf_Value ret = xf_val_ok_num(pow(na.data.num, nb.data.num));
            xf_value_release(na);
            xf_value_release(nb);
            return ret;
        }

        case BINOP_PIPE_CMD: {
            xf_Value cmd = xf_coerce_str(b);
            xf_value_release(b);
            if (cmd.state != XF_STATE_OK || !cmd.data.str) {
                xf_value_release(cmd);
                xf_value_release(a);
                return xf_val_nav(XF_TYPE_STR);
            }

            xf_Value sv = xf_coerce_str(a);
            xf_value_release(a);
            if (sv.state != XF_STATE_OK || !sv.data.str) {
                xf_value_release(cmd);
                xf_value_release(sv);
                return xf_val_nav(XF_TYPE_STR);
            }

            char tmpname[] = "/tmp/xf_pipe_XXXXXX";
            int fd = mkstemp(tmpname);
            if (fd < 0) {
                xf_value_release(cmd);
                xf_value_release(sv);
                return xf_val_nav(XF_TYPE_STR);
            }

            FILE *tf = fdopen(fd, "w");
            if (!tf) {
                close(fd);
                unlink(tmpname);
                xf_value_release(cmd);
                xf_value_release(sv);
                return xf_val_nav(XF_TYPE_STR);
            }

            fwrite(sv.data.str->data, 1, sv.data.str->len, tf);
            fclose(tf);

            char shellcmd[8192];
            snprintf(shellcmd, sizeof(shellcmd), "%s < '%s'", cmd.data.str->data, tmpname);

            FILE *fp = popen(shellcmd, "r");
            if (!fp) {
                unlink(tmpname);
                xf_value_release(cmd);
                xf_value_release(sv);
                return xf_val_nav(XF_TYPE_STR);
            }

            char buf[65536];
            size_t n = 0;
            int c;
            while (n < sizeof(buf) - 1 && (c = fgetc(fp)) != EOF)
                buf[n++] = (char)c;
            buf[n] = '\0';

            pclose(fp);
            unlink(tmpname);

            while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
                buf[--n] = '\0';

            xf_Str *r = xf_str_from_cstr(buf);
            xf_Value rv = xf_val_ok_str(r);
            xf_str_release(r);

            xf_value_release(cmd);
            xf_value_release(sv);
            return rv;
        }

        case BINOP_EQ: {
            bool ok = (val_cmp(a, b) == 0);
            xf_value_release(a);
            xf_value_release(b);
            return make_bool(ok);
        }

        case BINOP_NEQ: {
            bool ok = (val_cmp(a, b) != 0);
            xf_value_release(a);
            xf_value_release(b);
            return make_bool(ok);
        }

        case BINOP_LT: {
            bool ok = (val_cmp(a, b) < 0);
            xf_value_release(a);
            xf_value_release(b);
            return make_bool(ok);
        }

        case BINOP_GT: {
            bool ok = (val_cmp(a, b) > 0);
            xf_value_release(a);
            xf_value_release(b);
            return make_bool(ok);
        }

        case BINOP_LTE: {
            bool ok = (val_cmp(a, b) <= 0);
            xf_value_release(a);
            xf_value_release(b);
            return make_bool(ok);
        }

        case BINOP_GTE: {
            bool ok = (val_cmp(a, b) >= 0);
            xf_value_release(a);
            xf_value_release(b);
            return make_bool(ok);
        }

        case BINOP_SPACESHIP: {
            xf_Value ret;
            if (a.state != b.state) ret = xf_val_ok_num(0.0);
            else ret = xf_val_ok_num((double)val_cmp(a, b));
            xf_value_release(a);
            xf_value_release(b);
            return ret;
        }
        case BINOP_IN:
            return eval_in(a,b);

        case BINOP_MATCH:
        case BINOP_NMATCH: {
            bool is_nmatch = (e->as.binary.op == BINOP_NMATCH);

            xf_Value sa = xf_coerce_str(a);
            xf_value_release(a);
            if (sa.state != XF_STATE_OK || !sa.data.str) {
                xf_value_release(sa);
                xf_value_release(b);
                return make_bool(is_nmatch);
            }

            const char *subject = sa.data.str->data;
            const char *pattern = NULL;
            int cflags = REG_EXTENDED;

            xf_Value sb_coerced = xf_val_null();
            bool sb_allocated = false;
            regex_t *precompiled = NULL;

            if (b.state == XF_STATE_OK && b.type == XF_TYPE_REGEX && b.data.re) {
                if (b.data.re->compiled) {
                    precompiled = (regex_t *)b.data.re->compiled;
                } else {
                    pattern = b.data.re->pattern ? b.data.re->pattern->data : "";
                    if (b.data.re->flags & XF_RE_ICASE) cflags |= REG_ICASE;
                    if (b.data.re->flags & XF_RE_MULTILINE) cflags |= REG_NEWLINE;
                }
            } else {
                sb_coerced = xf_coerce_str(b);
                sb_allocated = true;
                if (sb_coerced.state != XF_STATE_OK || !sb_coerced.data.str) {
                    xf_value_release(sa);
                    xf_value_release(sb_coerced);
                    xf_value_release(b);
                    return make_bool(is_nmatch);
                }
                pattern = sb_coerced.data.str->data;
            }

            regex_t re_local;
            char errbuf[128];
            bool matched;
            regmatch_t pm[33] = {0};
            int ngroups = 33;

            if (precompiled) {
                matched = (regexec(precompiled, subject, ngroups, pm, 0) == 0);
            } else {
                int rcc = regcomp(&re_local, pattern ? pattern : "", cflags);
                if (rcc != 0) {
                    regerror(rcc, &re_local, errbuf, sizeof(errbuf));
                    interp_error(it, e->loc, "invalid regex '%s': %s",
                                 pattern ? pattern : "", errbuf);
                    matched = false;
                } else {
                    matched = (regexec(&re_local, subject, ngroups, pm, 0) == 0);
                    regfree(&re_local);
                }
            }

            if (matched) {
                regoff_t ms = pm[0].rm_so, me = pm[0].rm_eo;
                if (ms >= 0) {
                    xf_Str *ms_str = xf_str_new(subject + ms, (size_t)(me - ms));
                    xf_value_release(IT_REC(it)->last_match);
                    IT_REC(it)->last_match = xf_val_ok_str(ms_str);
                    xf_str_release(ms_str);
                }

                xf_arr_t *caps = xf_arr_new();
                for (int gi = 1; gi < ngroups && pm[gi].rm_so >= 0; gi++) {
                    regoff_t gs = pm[gi].rm_so, ge = pm[gi].rm_eo;
                    xf_Str *gs_str = xf_str_new(subject + gs, (size_t)(ge - gs));
                    xf_Value gv = xf_val_ok_str(gs_str);
                    xf_str_release(gs_str);
                    xf_arr_push(caps, gv);
                }

                xf_value_release(IT_REC(it)->last_captures);
                IT_REC(it)->last_captures = xf_val_ok_arr(caps);
                xf_arr_release(caps);
            }

            xf_value_release(sa);
            if (sb_allocated) xf_value_release(sb_coerced);
            xf_value_release(b);
            return make_bool(is_nmatch ? !matched : matched);
        }

        default:
            xf_value_release(a);
            xf_value_release(b);
            return xf_val_nav(XF_TYPE_VOID);
    }
}
    case EXPR_TERNARY: {
        xf_Value cond = interp_eval_expr(it, e->as.ternary.cond);
        bool _cond_truthy = is_truthy(cond);
        xf_value_release(cond);
        return _cond_truthy
               ? interp_eval_expr(it, e->as.ternary.then)
               : interp_eval_expr(it, e->as.ternary.els);
    }

    case EXPR_COALESCE: {
        xf_Value v = interp_eval_expr(it, e->as.coalesce.left);

        switch (v.state) {
            case XF_STATE_NULL:
            case XF_STATE_NAV:
            case XF_STATE_UNDEF:
                xf_value_release(v);
                return interp_eval_expr(it, e->as.coalesce.right);

            default:
                return v;
        }
    }

    case EXPR_ASSIGN: {
    xf_Value rhs = interp_eval_expr(it, e->as.assign.value);

    if (e->as.assign.op != ASSIGNOP_EQ) {
        xf_Value cur = lvalue_load(it, e->as.assign.target);
        xf_Value old_rhs = rhs;
        rhs = apply_assign_op(it,e->as.assign.op, cur, old_rhs, e->loc);
        xf_value_release(cur);
        xf_value_release(old_rhs);
    }

    if (!lvalue_store(it, e->as.assign.target, xf_value_retain(rhs))) {
        xf_value_release(rhs);
        return xf_val_nav(XF_TYPE_VOID);
    }

    return rhs;
}
case EXPR_WALRUS: {
    xf_Value rhs = interp_eval_expr(it, e->as.walrus.value);

    Symbol *s = sym_lookup_str(it->syms, e->as.walrus.name);
    if (s) {
        xf_value_release(s->value);
        s->value      = rhs;
        s->state      = rhs.state;
        s->is_defined = true;
    } else {
        s = sym_declare(it->syms, e->as.walrus.name,
                        SYM_VAR, e->as.walrus.type, e->loc);
        if (s) {
            xf_value_release(s->value);
            s->value      = rhs;
            s->state      = rhs.state;
            s->is_defined = true;
        }
    }

    return xf_value_retain(rhs);
}

case EXPR_CALL: {
    xf_Value args[64];
    size_t argc = e->as.call.argc < 64 ? e->as.call.argc : 64;

    for (size_t i = 0; i < argc; i++)
        args[i] = interp_eval_expr(it, e->as.call.args[i]);

    /* -----------------------------
     * MEMBER CALL: obj.method(...)
     * ----------------------------- */
    if (e->as.call.callee->kind == EXPR_MEMBER) {
        Expr *member = e->as.call.callee;

        xf_Value obj = interp_eval_expr(it, member->as.member.obj);

        const char *mname =
            (member->as.member.field && member->as.member.field->data)
            ? member->as.member.field->data
            : "<null-member>";

        /* -------- MODULE CALL -------- */
        if (obj.state == XF_STATE_OK &&
            obj.type == XF_TYPE_MODULE &&
            obj.data.mod) {

            xf_Value callee = xf_module_get(obj.data.mod, mname);

            if (callee.state == XF_STATE_OK &&
                callee.type == XF_TYPE_FN &&
                callee.data.fn) {

                xf_Fn *fn = callee.data.fn;

                /* ---- XF function ---- */
                if (!fn->is_native) {
                    Scope *fn_sc = sym_push(it->syms, SCOPE_FN);
                    fn_sc->fn_ret_type = fn->return_type;

                    for (size_t i = 0; i < fn->param_count; i++) {
                        xf_Value av = i < argc ? args[i]
                                               : xf_val_undef(fn->params[i].type);

                        Symbol *ps = sym_declare(it->syms,
                                                 fn->params[i].name,
                                                 SYM_PARAM,
                                                 fn->params[i].type,
                                                 e->loc);
                        if (ps) {
                            xf_value_release(ps->value);
                            ps->value      = xf_value_retain(av);
                            ps->state      = av.state;
                            ps->is_defined = true;
                        }
                    }

                    bool saved_returning = it->returning;
                    xf_Value saved_ret   = it->return_val;

                    it->returning  = false;
                    it->return_val = xf_val_null();

                    interp_eval_stmt(it, (Stmt *)fn->body);

                    xf_Value ret = it->returning
                        ? xf_value_retain(it->return_val)
                        : xf_val_null();

                    xf_value_release(it->return_val);
                    it->return_val = saved_ret;
                    it->returning  = saved_returning;

                    scope_free(sym_pop(it->syms));

                    if (fn->return_type != XF_TYPE_VOID &&
                        ret.state == XF_STATE_NULL)
                        ret = xf_val_nav(fn->return_type);

                    xf_value_release(callee);
                    xf_value_release(obj);
                    for (size_t i = 0; i < argc; i++)
                        xf_value_release(args[i]);

                    return ret;
                }

                /* ---- native fn ---- */
                if (fn->native_v) {
                    xf_Value ret0 = fn->native_v(args, argc);
                    xf_Value ret  = xf_value_retain(ret0);

                    xf_value_release(callee);
                    xf_value_release(obj);
                    for (size_t i = 0; i < argc; i++)
                        xf_value_release(args[i]);

                    return ret;
                }
            }

            const char *modname =
                (obj.data.mod && obj.data.mod->name)
                ? obj.data.mod->name
                : "<module>";

            xf_value_release(callee);
            xf_value_release(obj);
            for (size_t i = 0; i < argc; i++)
                xf_value_release(args[i]);

            interp_error(it, e->loc, "'%s' has no member '%s'", modname, mname);
            return xf_val_nav(XF_TYPE_VOID);
        }

        /* -------- NON-MODULE MEMBER -------- */
        xf_Value margs[65];
        margs[0] = obj;
        for (size_t i = 0; i < argc && i + 1 < 65; i++)
            margs[i + 1] = args[i];

        xf_Value mr = interp_call_builtin(it, mname, margs, argc + 1);

        if (mr.state != XF_STATE_NAV) {
            xf_Value ret = xf_value_retain(mr);

            xf_value_release(obj);
            for (size_t i = 0; i < argc; i++)
                xf_value_release(args[i]);

            return ret;
        }

        xf_value_release(obj);
    }

    /* -----------------------------
     * NORMAL CALL: f(...)
     * ----------------------------- */
    xf_Value callee = interp_eval_expr(it, e->as.call.callee);

    if (callee.state == XF_STATE_OK &&
        callee.type == XF_TYPE_FN &&
        callee.data.fn) {

        xf_Fn *fn = callee.data.fn;

        /* ---- XF function ---- */
        if (!fn->is_native) {
            Scope *fn_sc = sym_push(it->syms, SCOPE_FN);
            fn_sc->fn_ret_type = fn->return_type;

            for (size_t i = 0; i < fn->param_count; i++) {
                xf_Value av = i < argc ? args[i]
                                       : xf_val_undef(fn->params[i].type);

                Symbol *ps = sym_declare(it->syms,
                                         fn->params[i].name,
                                         SYM_PARAM,
                                         fn->params[i].type,
                                         e->loc);
                if (ps) {
                    xf_value_release(ps->value);
                    ps->value      = xf_value_retain(av);
                    ps->state      = av.state;
                    ps->is_defined = true;
                }
            }

            bool saved_returning = it->returning;
            xf_Value saved_ret   = it->return_val;

            it->returning  = false;
            it->return_val = xf_val_null();

            interp_eval_stmt(it, (Stmt *)fn->body);

            xf_Value ret = it->returning
                ? xf_value_retain(it->return_val)
                : xf_val_null();

            xf_value_release(it->return_val);
            it->return_val = saved_ret;
            it->returning  = saved_returning;

            scope_free(sym_pop(it->syms));

            if (fn->return_type != XF_TYPE_VOID &&
                ret.state == XF_STATE_NULL)
                ret = xf_val_nav(fn->return_type);

            xf_value_release(callee);
            for (size_t i = 0; i < argc; i++)
                xf_value_release(args[i]);

            return ret;
        }

        /* ---- native ---- */
        if (fn->native_v) {
            xf_Value ret0 = fn->native_v(args, argc);
            xf_Value ret  = xf_value_retain(ret0);

            xf_value_release(callee);
            for (size_t i = 0; i < argc; i++)
                xf_value_release(args[i]);

            return ret;
        }
    }

    /* ---- builtin fallback ---- */
    if (e->as.call.callee->kind == EXPR_IDENT) {
        const char *name =
            (e->as.call.callee->as.ident.name &&
             e->as.call.callee->as.ident.name->data)
            ? e->as.call.callee->as.ident.name->data
            : "<null-ident>";

        xf_Value r = interp_call_builtin(it, name, args, argc);

        if (r.state != XF_STATE_NAV) {
            xf_Value ret = xf_value_retain(r);

            xf_value_release(callee);
            for (size_t i = 0; i < argc; i++)
                xf_value_release(args[i]);

            return ret;
        }
    }

    xf_value_release(callee);
    for (size_t i = 0; i < argc; i++)
        xf_value_release(args[i]);

    interp_error(it, e->loc, "attempt to call non-function");
    return xf_val_nav(XF_TYPE_VOID);
}



                case EXPR_STATE: {
        xf_Value v = interp_eval_expr(it, e->as.introspect.operand);

        uint8_t st = (v.state < XF_STATE_COUNT) ? v.state : XF_STATE_OK;
        const char *name = XF_STATE_NAMES[st];

        xf_Str *s = xf_str_from_cstr(name);
        xf_Value out = xf_val_ok_str(s);
        xf_str_release(s);
        return out;
    }

    case EXPR_TYPE: {
        xf_Value v = interp_eval_expr(it, e->as.introspect.operand);

        uint8_t t = XF_STATE_IS_BOOL(v.state) ? XF_TYPE_BOOL : v.type;
        if (t >= XF_TYPE_COUNT) t = XF_TYPE_VOID;

        const char *name = XF_TYPE_NAMES[t];

        xf_Str *s = xf_str_from_cstr(name);
        xf_Value out = xf_val_ok_str(s);
        xf_str_release(s);
        return out;
    }

    case EXPR_LEN: {
        xf_Value v = interp_eval_expr(it, e->as.introspect.operand);
        if (v.state != XF_STATE_OK) return v;

        switch (v.type) {
            case XF_TYPE_STR:
                return xf_val_ok_num(v.data.str ? (double)v.data.str->len : 0.0);
            case XF_TYPE_ARR:
                return xf_val_ok_num(v.data.arr ? (double)v.data.arr->len : 0.0);
            case XF_TYPE_TUPLE:
                return xf_val_ok_num(v.data.tuple ? (double)xf_tuple_len(v.data.tuple) : 0.0);
            case XF_TYPE_MAP:
            case XF_TYPE_SET:
                return xf_val_ok_num(v.data.map ? (double)v.data.map->order_len : 0.0);
            case XF_TYPE_MODULE:
                if (v.data.mod) {
                    xf_Value r = xf_module_get(v.data.mod, "length");
                    if (r.state != XF_STATE_NAV) return r;
                }
                interp_error(it, e->loc, "module has no member 'length'");
                return xf_val_nav(XF_TYPE_NUM);
            default:
                interp_error(it, e->loc, ".len not defined for type '%s'",
                             XF_TYPE_NAMES[v.type]);
                return xf_val_nav(XF_TYPE_NUM);
        }
    }

    case EXPR_CAST: {
        xf_Value v = interp_eval_expr(it, e->as.cast.operand);
        if (v.state != XF_STATE_OK) return v;

        uint8_t target = e->as.cast.to_type;
        if (v.type == target) return v;

        switch (target) {
            case XF_TYPE_NUM:
                return xf_coerce_num(v);

            case XF_TYPE_STR:
                return xf_coerce_str(v);

            default:
                interp_error(it, e->loc, "cast to '%s' is not supported",
                             XF_TYPE_NAMES[target]);
                return xf_val_nav(target);
        }
    }

    case EXPR_MEMBER: {
        xf_Value obj = interp_eval_expr(it, e->as.member.obj);
        const char *field = (e->as.member.field && e->as.member.field->data) ? e->as.member.field->data : "<null-field>";

        if (obj.state == XF_STATE_OK &&
            obj.type == XF_TYPE_MODULE && obj.data.mod) {
            xf_Value r = xf_module_get(obj.data.mod, field);
            if (r.state != XF_STATE_NAV) return r;
            interp_error(it, e->loc, "'%s' has no member '%s'",
                         obj.data.mod->name, field);
            return xf_val_nav(XF_TYPE_VOID);
        }

        if (strcmp(field, "state") == 0) {
            const char *name = XF_STATE_NAMES[
                obj.state < XF_STATE_COUNT ? obj.state : 0
            ];
            xf_Str *s = xf_str_from_cstr(name);
            xf_Value out = xf_val_ok_str(s);
            xf_str_release(s);
            return out;
        }

        if (strcmp(field, "type") == 0) {
            uint8_t t = XF_STATE_IS_BOOL(obj.state) ? XF_TYPE_BOOL : obj.type;
            const char *name = XF_TYPE_NAMES[
                t < XF_TYPE_COUNT ? t : 0
            ];
            xf_Str *s = xf_str_from_cstr(name);
            xf_Value out = xf_val_ok_str(s);
            xf_str_release(s);
            return out;
        }

        if (strcmp(field, "len") == 0 || strcmp(field, "length") == 0) {
            if (obj.state != XF_STATE_OK) return obj;
            if (obj.type == XF_TYPE_STR)
                return xf_val_ok_num(obj.data.str ? (double)obj.data.str->len : 0.0);
            if (obj.type == XF_TYPE_ARR)
                return xf_val_ok_num(obj.data.arr ? (double)obj.data.arr->len : 0.0);
            if (obj.type == XF_TYPE_MAP || obj.type == XF_TYPE_SET)
                return xf_val_ok_num(obj.data.map ? (double)obj.data.map->used : 0.0);
            return xf_val_nav(XF_TYPE_NUM);
        }

        interp_error(it, e->loc, "unknown member '.%s'", field);
        return xf_val_nav(XF_TYPE_VOID);
    }

case EXPR_PIPE_FN: {
    xf_Value left = interp_eval_expr(it, e->as.pipe_fn.left);
    Expr *rhs = e->as.pipe_fn.right;

    /* ── a |> f(b, c)  →  f(left, b, c) ──────────────────────
     * Inject left as the first argument into the call.         */
    if (rhs->kind == EXPR_CALL) {
        size_t extra_argc = rhs->as.call.argc < 63 ? rhs->as.call.argc : 63;
        xf_Value args[64];
        args[0] = left;
        for (size_t i = 0; i < extra_argc; i++)
            args[i + 1] = interp_eval_expr(it, rhs->as.call.args[i]);
        size_t total_argc = extra_argc + 1;

        xf_Value result = xf_val_nav(XF_TYPE_VOID);

        /* try builtin by name first */
        if (rhs->as.call.callee->kind == EXPR_IDENT &&
            rhs->as.call.callee->as.ident.name) {
            const char *bname = rhs->as.call.callee->as.ident.name->data;
            xf_Value r = interp_call_builtin(it, bname, args, total_argc);
            if (r.state != XF_STATE_NAV) {
                result = xf_value_retain(r);
                for (size_t i = 0; i < total_argc; i++) xf_value_release(args[i]);
                return result;
            }
        }

        /* evaluate callee and dispatch */
        xf_Value callee_val = interp_eval_expr(it, rhs->as.call.callee);
        if (callee_val.state == XF_STATE_OK &&
            callee_val.type  == XF_TYPE_FN  &&
            callee_val.data.fn) {
            xf_Fn *fn = callee_val.data.fn;
            if (fn->is_native && fn->native_v) {
                xf_Value r0 = fn->native_v(args, total_argc);
                result = xf_value_retain(r0);
            } else {
                result = interp_exec_xf_fn(it->vm, it->syms, fn, args, total_argc);
            }
        } else {
            interp_error(it, e->loc, "pipe target is not callable");
        }
        xf_value_release(callee_val);
        for (size_t i = 0; i < total_argc; i++) xf_value_release(args[i]);
        return result;
    }

    /* ── a |> obj.method ────────────────────────────────────── */
    if (rhs->kind == EXPR_MEMBER) {
        xf_Value obj = interp_eval_expr(it, rhs->as.member.obj);
        const char *mn = (rhs->as.member.field && rhs->as.member.field->data)
                         ? rhs->as.member.field->data : "<null-member>";

        /* module member: look up fn in module, call with left */
        if (obj.state == XF_STATE_OK &&
            obj.type  == XF_TYPE_MODULE && obj.data.mod) {
            xf_Value fv = xf_module_get(obj.data.mod, mn);
            if (fv.state == XF_STATE_OK &&
                fv.type  == XF_TYPE_FN  && fv.data.fn) {
                xf_Fn *fn = fv.data.fn;
                xf_Value result;
                if (fn->is_native && fn->native_v) {
                    xf_Value r0 = fn->native_v(&left, 1);
                    result = xf_value_retain(r0);
                } else {
                    result = interp_exec_xf_fn(it->vm, it->syms, fn, &left, 1);
                }
                xf_value_release(fv);
                xf_value_release(obj);
                xf_value_release(left);
                return result;
            }
            xf_value_release(fv);
            interp_error(it, e->loc, "module has no callable member '%s'", mn);
            xf_value_release(obj);
            xf_value_release(left);
            return xf_val_nav(XF_TYPE_VOID);
        }

        /* non-module: try builtin with (obj, left) as before */
        xf_Value margs[2] = { obj, left };
        xf_Value mr = interp_call_builtin(it, mn, margs, 2);
        if (mr.state != XF_STATE_NAV) {
            xf_Value ret = xf_value_retain(mr);
            xf_value_release(obj);
            xf_value_release(left);
            return ret;
        }
        xf_value_release(obj);
    }

    /* ── a |> ident ─────────────────────────────────────────── */
    if (rhs->kind == EXPR_IDENT) {
        const char *name = (rhs->as.ident.name && rhs->as.ident.name->data)
                           ? rhs->as.ident.name->data : "<null-ident>";
        xf_Value r = interp_call_builtin(it, name, &left, 1);
        if (r.state != XF_STATE_NAV) {
            xf_Value ret = xf_value_retain(r);
            xf_value_release(left);
            return ret;
        }
    }

    /* ── fallback: evaluate rhs as a fn value and call it ───── */
    xf_Value callee = interp_eval_expr(it, rhs);
    if (callee.state == XF_STATE_OK &&
        callee.type  == XF_TYPE_FN  &&
        callee.data.fn) {
        xf_Fn *fn = callee.data.fn;
        if (fn->is_native && fn->native_v) {
            xf_Value r0 = fn->native_v(&left, 1);
            xf_Value ret = xf_value_retain(r0);
            xf_value_release(callee);
            xf_value_release(left);
            return ret;
        }
        xf_Value ret = interp_exec_xf_fn(it->vm, it->syms, fn, &left, 1);
        xf_value_release(callee);
        xf_value_release(left);
        return ret;
    }

    xf_value_release(callee);
    xf_value_release(left);
    interp_error(it, e->loc, "pipe target is not callable");
    return xf_val_nav(XF_TYPE_VOID);
}
    case EXPR_FN: {
        xf_Fn *fn = build_fn(NULL, e->as.fn.return_type,
                             e->as.fn.params, e->as.fn.param_count,
                             e->as.fn.body);
        return xf_val_ok_fn(fn);
    }

    case EXPR_SPAWN: {
        if (it->is_worker)
            return xf_val_ok_num(0.0);

        Expr *call_expr = e->as.spawn_expr.call;
        if (!call_expr || call_expr->kind != EXPR_CALL)
            return xf_val_ok_num(0.0);

        xf_Value callee = interp_eval_expr(it, call_expr->as.call.callee);
        size_t argc = call_expr->as.call.argc < 64 ? call_expr->as.call.argc : 64;
        xf_Value sargs[64];
        for (size_t i = 0; i < argc; i++)
            sargs[i] = interp_eval_expr(it, call_expr->as.call.args[i]);

        pthread_mutex_lock(&g_spawn_mu);
        if (g_spawn_live >= XF_SPAWN_MAX ||
            callee.state != XF_STATE_OK ||
            callee.type  != XF_TYPE_FN  ||
            !callee.data.fn) {
            pthread_mutex_unlock(&g_spawn_mu);
            xf_value_release(callee);
            for (size_t i = 0; i < argc; i++) xf_value_release(sargs[i]);
            return xf_val_ok_num(0.0);
        }

        SpawnCtx *ctx = alloc_spawn_ctx();
        if (!ctx) {
            pthread_mutex_unlock(&g_spawn_mu);
            xf_value_release(callee);
            for (size_t i = 0; i < argc; i++) xf_value_release(sargs[i]);
            return xf_val_ok_num(0.0);
        }
        ctx->id = g_spawn_next++;
        ctx->vm = it->vm;
        sym_init(&ctx->snap_syms);
        copy_globals_from(&ctx->snap_syms, it->syms);
        ctx->snap_ready = true;
        ctx->fn_val = callee;
        ctx->argc   = argc;
        for (size_t i = 0; i < argc; i++) ctx->args[i] = sargs[i];

        uint32_t handle_id = ctx->id;
        pthread_mutex_unlock(&g_spawn_mu);

        if (!start_spawn_ctx(ctx)) {
            pthread_mutex_lock(&g_spawn_mu);
            SpawnCtx *slot = find_spawn_ctx_by_id(handle_id);
            if (slot) free_spawn_ctx_slot(slot);
            pthread_mutex_unlock(&g_spawn_mu);
            return xf_val_ok_num(0.0);
        }

        return xf_val_ok_num((double)handle_id);
    }

    case EXPR_STATE_LIT:
        return (xf_Value){
            .state = e->as.state_lit.state,
            .type  = XF_STATE_IS_BOOL(e->as.state_lit.state)
                        ? XF_TYPE_BOOL : XF_TYPE_VOID
        };

    default:
        interp_error(it, e->loc, "unhandled expression kind %d", e->kind);
        return xf_val_nav(XF_TYPE_VOID);
    }
}
/* ── iter_collection ────────────────────────────────────────────────
 * Shared iteration kernel used by both STMT_FOR and STMT_FOR_SHORT.
 * Walks arr / tuple / map / set / str collections, binding key+val
 * loop variables and executing `body` for each element.
 * ------------------------------------------------------------------ */
static void iter_collection(Interp *it, xf_Value col,
                             LoopBind *iter_key, LoopBind *iter_val,
                             Stmt *body, Loc loc) {
    if (col.type == XF_TYPE_ARR && col.data.arr) {
        xf_arr_t *a = col.data.arr;
        for (size_t i = 0; i < a->len; i++) {
            sym_push(it->syms, SCOPE_LOOP);
            xf_Value keyv = xf_val_ok_num((double)i);
            bind_loop_index_value(it, iter_key, iter_val, keyv, a->items[i], loc);
            interp_eval_stmt(it, body);
            scope_free(sym_pop(it->syms));
            if (it->nexting)  { it->nexting  = false; continue; }
            if (it->breaking) { it->breaking = false; break; }
            if (it->returning || it->exiting || it->had_error) break;
        }
    } else if (col.type == XF_TYPE_TUPLE && col.data.tuple) {
        size_t n = xf_tuple_len(col.data.tuple);
        for (size_t i = 0; i < n; i++) {
            xf_Value tv = xf_tuple_get(col.data.tuple, i);
            sym_push(it->syms, SCOPE_LOOP);
            xf_Value keyv = xf_val_ok_num((double)i);
            bind_loop_index_value(it, iter_key, iter_val, keyv, tv, loc);
            interp_eval_stmt(it, body);
            scope_free(sym_pop(it->syms));
            if (it->nexting)  { it->nexting  = false; continue; }
            if (it->breaking) { it->breaking = false; break; }
            if (it->returning || it->exiting || it->had_error) break;
        }
    } else if ((col.type == XF_TYPE_MAP || col.type == XF_TYPE_SET) && col.data.map) {
        xf_map_t *m     = col.data.map;
        bool      is_set = (col.type == XF_TYPE_SET);

        for (size_t i = 0; i < m->order_len; i++) {
            xf_Str  *key = m->order[i];
            xf_Value kv  = xf_val_ok_str(key);
            xf_Value vv  = is_set ? xf_val_ok_str(key) : xf_map_get(m, key);

            sym_push(it->syms, SCOPE_LOOP);

            /* `for (k, v) in map` — parser encodes both bindings as a single
             * 2-element LOOP_BIND_TUPLE on iter_val with no iter_key.
             * Split it here so each element is bound to key and value
             * individually, rather than trying to destructure the map value
             * as a tuple (which always fails for non-tuple values).         */
            if (!iter_key &&
                iter_val && iter_val->kind == LOOP_BIND_TUPLE &&
                iter_val->as.tuple.count == 2) {
                bind_loop_index_value(it,
                    iter_val->as.tuple.items[0],
                    iter_val->as.tuple.items[1],
                    kv, vv, loc);
            } else {
                bind_loop_index_value(it, iter_key, iter_val, kv, vv, loc);
            }

            interp_eval_stmt(it, body);
            scope_free(sym_pop(it->syms));

            xf_value_release(kv);
            xf_value_release(vv);   /* unconditional — fixes map value leak */

            if (it->nexting)  { it->nexting  = false; continue; }
            if (it->breaking) { it->breaking = false; break; }
            if (it->returning || it->exiting || it->had_error) break;
        }
    } else {
        interp_error(it, loc, "cannot iterate over type '%s'",
                     XF_TYPE_NAMES[col.type]);
    }
}
xf_Value interp_eval_stmt(Interp *it, Stmt *s) {
    if (!s) return xf_val_null();
    if (it->had_error || it->returning || it->exiting || it->nexting || it->breaking)
        return xf_val_null();

    switch (s->kind) {
case STMT_BLOCK: {
    xf_Value last = xf_val_null();
    sym_push(it->syms, SCOPE_BLOCK);

    for (size_t i = 0; i < s->as.block.count; i++) {
        xf_Value cur = interp_eval_stmt(it, s->as.block.stmts[i]);
        xf_value_release(last);
        last = cur;
        if (it->returning || it->exiting || it->nexting ||
            it->breaking || it->had_error) break;
    }

    scope_free(sym_pop(it->syms));
    return last;
}
    case STMT_EXPR:
        return interp_eval_expr(it, s->as.expr.expr);

case STMT_VAR_DECL: {
    xf_Value init = s->as.var_decl.init
                     ? interp_eval_expr(it, s->as.var_decl.init)
                     : xf_val_undef(s->as.var_decl.type);

    if (init.state == XF_STATE_OK &&
        s->as.var_decl.type != XF_TYPE_VOID &&
        init.type != s->as.var_decl.type) {
        if (s->as.var_decl.type == XF_TYPE_NUM) {
            xf_Value old = init;
            init = xf_coerce_num(old);
            xf_value_release(old);
        } else if (s->as.var_decl.type == XF_TYPE_STR) {
            xf_Value old = init;
            init = xf_coerce_str(old);
            xf_value_release(old);
        }
    }

    Symbol *sym = sym_lookup_local(it->syms,
                      s->as.var_decl.name->data, s->as.var_decl.name->len);
    if (!sym)
        sym = sym_declare(it->syms, s->as.var_decl.name,
                          SYM_VAR, s->as.var_decl.type, s->loc);

    if (sym) {
        xf_value_release(sym->value);
        sym->value = init;
        sym->state = init.state;
        sym->is_defined = true;
    }

    return xf_value_retain(init);
}
case STMT_FN_DECL: {
    xf_Fn *fn = build_fn(s->as.fn_decl.name, s->as.fn_decl.return_type,
                         s->as.fn_decl.params, s->as.fn_decl.param_count,
                         s->as.fn_decl.body);
    xf_Value fv = xf_val_ok_fn(fn);

    Scope *saved = it->syms->current;
    it->syms->current = it->syms->global;

    Symbol *sym = sym_lookup_local(it->syms,
                      s->as.fn_decl.name->data, s->as.fn_decl.name->len);
    if (!sym)
        sym = sym_declare(it->syms, s->as.fn_decl.name,
                          SYM_FN, XF_TYPE_FN, s->loc);

    it->syms->current = saved;

    if (sym) {
        xf_value_release(sym->value);
        sym->value = fv;
        sym->state = XF_STATE_OK;
        sym->is_defined = true;
    }

    xf_fn_release(fn);
    return xf_val_null();
}
    case STMT_IF: {
        for (size_t i = 0; i < s->as.if_stmt.count; i++) {
            xf_Value cond = interp_eval_expr(it, s->as.if_stmt.branches[i].cond);
            if (is_truthy(cond)) {
                sym_push(it->syms, SCOPE_BLOCK);
                xf_Value r = interp_eval_stmt(it, s->as.if_stmt.branches[i].body);
                scope_free(sym_pop(it->syms));
                return r;
            }
        }
        if (s->as.if_stmt.els) {
            sym_push(it->syms, SCOPE_BLOCK);
            xf_Value r = interp_eval_stmt(it, s->as.if_stmt.els);
            scope_free(sym_pop(it->syms));
            return r;
        }
        return xf_val_null();
    }

    case STMT_WHILE: {
        sym_push(it->syms, SCOPE_LOOP);
        while (is_truthy(interp_eval_expr(it, s->as.while_stmt.cond))) {
            interp_eval_stmt(it, s->as.while_stmt.body);
            if (it->nexting)  { it->nexting = false; continue; }
            if (it->breaking) { it->breaking = false; break; }
            if (it->returning || it->exiting || it->had_error) break;
        }
        scope_free(sym_pop(it->syms));
        return xf_val_null();
    }

    case STMT_WHILE_SHORT: {
        sym_push(it->syms, SCOPE_LOOP);
        while (is_truthy(interp_eval_expr(it, s->as.while_short.cond))) {
            interp_eval_stmt(it, s->as.while_short.body);
            if (it->nexting)  { it->nexting = false; continue; }
            if (it->breaking) { it->breaking = false; break; }
            if (it->returning || it->exiting || it->had_error) break;
        }
        scope_free(sym_pop(it->syms));
        return xf_val_null();
    }
    case STMT_FOR: {
        xf_Value col = interp_eval_expr(it, s->as.for_stmt.collection);
        if (col.state != XF_STATE_OK) return col;
        iter_collection(it, col, s->as.for_stmt.iter_key,
                        s->as.for_stmt.iter_val, s->as.for_stmt.body, s->loc);
        xf_value_release(col);
        return xf_val_null();
    }
        case STMT_FOR_SHORT: {
        xf_Value col = interp_eval_expr(it, s->as.for_short.collection);
        if (col.state != XF_STATE_OK) return col;
        iter_collection(it, col, s->as.for_short.iter_key,
                        s->as.for_short.iter_val, s->as.for_short.body, s->loc);
        xf_value_release(col);
        return xf_val_null();
    }

case STMT_RETURN: {
    xf_Value rv = s->as.ret.value
        ? interp_eval_expr(it, s->as.ret.value)
        : xf_val_null();

    xf_value_release(it->return_val);
    it->return_val = xf_value_retain(rv);
    it->returning  = true;

    return xf_val_null();  // or NAV depending on your semantics
}
    case STMT_NEXT:
        it->nexting = true;
        return xf_val_null();

    case STMT_EXIT:
        it->exiting = true;
        return xf_val_null();

    case STMT_BREAK:
        it->breaking = true;
        return xf_val_null();

case STMT_PRINT: {
    xf_Value vals[64];
    size_t count = s->as.print.count < 64 ? s->as.print.count : 64;

    for (size_t i = 0; i < count; i++)
        vals[i] = interp_eval_expr(it, s->as.print.args[i]);

    FILE *_out = stdout;

    if (s->as.print.redirect && s->as.print.redirect_op != 0) {
        xf_Value rv = interp_eval_expr(it, s->as.print.redirect);
        xf_Value rs = xf_coerce_str(rv);
        xf_value_release(rv);

        if (rs.state == XF_STATE_OK && rs.data.str) {
            FILE *cached = vm_redir_open(it->vm, rs.data.str->data,
                                         s->as.print.redirect_op);
            if (cached) _out = cached;
        }
        xf_value_release(rs);
    }

    for (size_t i = 0; i < count; i++) {
        if (i > 0) fputs(" ", _out);
        xf_print_value(_out, vals[i]);
    }
    fputc('\n', _out);

    for (size_t i = 0; i < count; i++)
        xf_value_release(vals[i]);

    return xf_val_null();
}
        case STMT_PRINTF: {
        if (s->as.printf_stmt.count == 0) return xf_val_null();

        xf_Value fmtv = interp_eval_expr(it, s->as.printf_stmt.args[0]);
        xf_Value fmts = xf_coerce_str(fmtv);
        xf_value_release(fmtv);

        if (fmts.state != XF_STATE_OK || !fmts.data.str) {
            xf_value_release(fmts);
            return xf_val_null();
        }

        xf_Value args[64];
        size_t argc = s->as.printf_stmt.count < 64 ? s->as.printf_stmt.count : 64;
        for (size_t i = 1; i < argc; i++)
            args[i] = interp_eval_expr(it, s->as.printf_stmt.args[i]);

        char buf[8192];
        xf_sprintf_impl(buf, sizeof(buf), fmts.data.str->data, args + 1, argc - 1);

        FILE *_pf_out = stdout;
        bool _pf_close = false;

        if (s->as.printf_stmt.redirect && s->as.printf_stmt.redirect_op != 0) {
            xf_Value rv = interp_eval_expr(it, s->as.printf_stmt.redirect);
            xf_Value rs = xf_coerce_str(rv);
            xf_value_release(rv);
            if (rs.state == XF_STATE_OK && rs.data.str) {
                FILE *cached = vm_redir_open(it->vm, rs.data.str->data,
                                             s->as.printf_stmt.redirect_op);
                if (cached) _pf_out = cached;
            }
            xf_value_release(rs);
        }

        fputs(buf, _pf_out);

        if (_pf_close) {
            if (s->as.printf_stmt.redirect_op == 3) pclose(_pf_out);
            else fclose(_pf_out);
        }

        for (size_t i = 1; i < argc; i++) xf_value_release(args[i]);
        xf_value_release(fmts);
        return xf_val_null();
    }

    case STMT_OUTFMT:
        if (s->as.outfmt.mode == XF_OUTFMT_JSON &&
            IT_REC(it)->out_mode != XF_OUTFMT_JSON)
            IT_REC(it)->headers_set = false;
        IT_REC(it)->out_mode = s->as.outfmt.mode;
        return xf_val_null();

    case STMT_IMPORT: {
        if (!s->as.import.path)
            return xf_val_null();

        const char *imp_path = (s->as.import.path && s->as.import.path->data) ? s->as.import.path->data : "<null-import-path>";
        FILE *imp_fp = fopen(imp_path, "r");
        if (!imp_fp) {
            interp_error(it, s->loc, "import: cannot open '%s'", imp_path);
            return xf_val_null();
        }

        fseek(imp_fp, 0, SEEK_END);
        long imp_sz = ftell(imp_fp);
        rewind(imp_fp);

        char *imp_src = malloc((size_t)imp_sz + 1);
        fread(imp_src, 1, (size_t)imp_sz, imp_fp);
        fclose(imp_fp);
        imp_src[imp_sz] = '\0';

        Lexer imp_lex;
        xf_lex_init(&imp_lex, imp_src, (size_t)imp_sz, XF_SRC_FILE, imp_path);
        xf_tokenize(&imp_lex);

        Program *imp_prog = parse(&imp_lex, it->syms);

        if (imp_prog) {
            /* Declaration pass only — mirror interp_run_program's first pass.
             * Imported BEGIN/END blocks must NOT fire at import time, and
             * pattern-action rules have no record context to run against.
             * Only TOP_FN (registers fn in syms) and TOP_STMT (top-level
             * variable declarations / assignments) are evaluated here. */
for (size_t ii = 0; ii < imp_prog->count; ii++) {
    TopLevel *t = imp_prog->items[ii];
    if (t->kind == TOP_BEGIN || t->kind == TOP_END ||
        t->kind == TOP_RULE)
        continue;

    xf_Value tv = interp_eval_top(it, t);
    xf_value_release(tv);

    if (it->had_error) break;
}
            /* DO NOT free imp_prog here — build_fn() stores a raw pointer
             * into top->as.fn.body.  Freeing the AST now would leave every
             * imported function body dangling (→ garbage stmt kind / segfault).
             * Instead, keep the program alive and free it in interp_free(). */
            if (it->imp_prog_count >= it->imp_prog_cap) {
                size_t new_cap = it->imp_prog_cap ? it->imp_prog_cap * 2 : 8;
                it->imp_progs  = realloc(it->imp_progs,
                                         new_cap * sizeof(Program *));
                it->imp_prog_cap = new_cap;
            }
            it->imp_progs[it->imp_prog_count++] = imp_prog;
        } else {
            interp_error(it, s->loc, "import: parse error in '%s'", imp_path);
        }

        xf_lex_free(&imp_lex);
        free(imp_src);
        return xf_val_null();
    }

    case STMT_DELETE: {
        Expr *tgt = s->as.delete.target;
        if (tgt && tgt->kind == EXPR_SUBSCRIPT) {
            xf_Value obj = interp_eval_expr(it, tgt->as.subscript.obj);
            xf_Value key = interp_eval_expr(it, tgt->as.subscript.key);

            if (obj.state == XF_STATE_OK && obj.type == XF_TYPE_ARR && obj.data.arr) {
                xf_Value ni = xf_coerce_num(key);
                if (ni.state == XF_STATE_OK)
                    xf_arr_delete(obj.data.arr, (size_t)ni.data.num);
            } else if (obj.state == XF_STATE_OK &&
                       (obj.type == XF_TYPE_MAP || obj.type == XF_TYPE_SET) &&
                       obj.data.map) {
                xf_Value ks = xf_coerce_str(key);
                if (ks.state == XF_STATE_OK && ks.data.str)
                    xf_map_delete(obj.data.map, ks.data.str);
                xf_value_release(ks);
            }

            xf_value_release(obj);
            xf_value_release(key);
        }
        return xf_val_null();
    }
case STMT_SPAWN: {
    if (it->is_worker) {
        return xf_val_nav(XF_TYPE_NUM);
    }

    Expr *call_expr = s->as.spawn.call;
    if (!call_expr || call_expr->kind != EXPR_CALL) {
        return xf_val_ok_num(0.0);
    }

    xf_Value callee = interp_eval_expr(it, call_expr->as.call.callee);
    size_t argc = call_expr->as.call.argc < 64 ? call_expr->as.call.argc : 64;
    xf_Value sargs[64];

    for (size_t i = 0; i < argc; i++)
        sargs[i] = interp_eval_expr(it, call_expr->as.call.args[i]);

    pthread_mutex_lock(&g_spawn_mu);
    if (g_spawn_live >= XF_SPAWN_MAX ||
        callee.state != XF_STATE_OK ||
        callee.type != XF_TYPE_FN ||
        !callee.data.fn) {
        pthread_mutex_unlock(&g_spawn_mu);
        xf_value_release(callee);
        for (size_t i = 0; i < argc; i++) xf_value_release(sargs[i]);
        return xf_val_ok_num(0.0);
    }

    SpawnCtx *ctx = alloc_spawn_ctx();
    if (!ctx) {
        pthread_mutex_unlock(&g_spawn_mu);
        xf_value_release(callee);
        for (size_t i = 0; i < argc; i++) xf_value_release(sargs[i]);
        return xf_val_ok_num(0.0);
    }
    ctx->id = g_spawn_next++;
    ctx->vm = it->vm;
    sym_init(&ctx->snap_syms);
    copy_globals_from(&ctx->snap_syms, it->syms);
    ctx->snap_ready = true;
    ctx->fn_val = callee;
    ctx->argc = argc;
    for (size_t i = 0; i < argc; i++) ctx->args[i] = sargs[i];

    uint32_t handle_id = ctx->id;
    pthread_mutex_unlock(&g_spawn_mu);

    if (!start_spawn_ctx(ctx)) {
        pthread_mutex_lock(&g_spawn_mu);
        SpawnCtx *slot = find_spawn_ctx_by_id(handle_id);
        if (slot) free_spawn_ctx_slot(slot);
        pthread_mutex_unlock(&g_spawn_mu);
        return xf_val_ok_num(0.0);
    }

    return xf_val_ok_num((double)handle_id);
}
case STMT_JOIN: {
    xf_Value handle = interp_eval_expr(it, s->as.join.handle);
    xf_Value join_result = xf_val_null();

    if (handle.state == XF_STATE_OK && handle.type == XF_TYPE_NUM) {
        uint32_t hid = (uint32_t)handle.data.num;

        /* Phase 1: copy tid by value while holding the lock — never keep
         * a pointer into g_spawn[] across a lock release.                  */
        pthread_t tid = (pthread_t)0;
        bool need_join = false;

        pthread_mutex_lock(&g_spawn_mu);
        SpawnCtx *slot = find_spawn_ctx_by_id(hid);
        if (slot) {
            if (!slot->started)
                start_spawn_ctx(slot);
            tid = slot->tid;
            need_join = slot->started;
        }
        pthread_mutex_unlock(&g_spawn_mu);

        /* Phase 2: block outside the lock using the copied tid value.      */
        if (need_join) {
            pthread_join(tid, NULL);

            /* Phase 3: copy result first, then swap-remove.                */
            pthread_mutex_lock(&g_spawn_mu);
            SpawnCtx *slot = find_spawn_ctx_by_id(hid);
            if (slot) {
                join_result = xf_value_retain(slot->result);
                free_spawn_ctx_slot(slot);
            }
            pthread_mutex_unlock(&g_spawn_mu);
        }
    }

    xf_value_release(handle);
    return join_result;
}

    case STMT_SUBST:
    case STMT_TRANS:
        return xf_val_null();

    default:
        interp_error(it, s->loc, "unhandled statement kind %d", s->kind);
        return xf_val_null();
    }
}
xf_Value interp_eval_top(Interp *it, TopLevel *top) {
    if (!top) return xf_val_null();
    switch (top->kind) {
        case TOP_BEGIN:
        case TOP_END:
            return interp_eval_stmt(it, top->as.begin_end.body);
case TOP_RULE:
    if (top->as.rule.pattern) {
        xf_Value pat = interp_eval_expr(it, top->as.rule.pattern);
        bool truth = is_truthy(pat);
        xf_value_release(pat);
        if (!truth) return xf_val_null();
    }
    return interp_eval_stmt(it, top->as.rule.body);
        case TOP_FN: {
            xf_Fn *fn = build_fn(top->as.fn.name, top->as.fn.return_type,
                                  top->as.fn.params, top->as.fn.param_count,
                                  top->as.fn.body);
            /* body stays in AST — not stolen */
            xf_Value fv = xf_val_ok_fn(fn);
            Symbol *sym = sym_lookup_local(it->syms,
                              top->as.fn.name->data, top->as.fn.name->len);
            if (!sym)
                sym = sym_declare(it->syms, top->as.fn.name, SYM_FN, XF_TYPE_FN, top->loc);
            if (sym) { xf_value_release(sym->value); sym->value = fv; sym->state = XF_STATE_OK; sym->is_defined = true; }
            return xf_val_null();
        }
        case TOP_STMT:
            return interp_eval_stmt(it, top->as.stmt.stmt);
        default:
            return xf_val_null();
    }
}
int interp_run_program(Interp *it, Program *prog) {
    for (size_t i = 0; i < prog->count; i++) {
        TopLevel *t = prog->items[i];
        if (t->kind == TOP_BEGIN || t->kind == TOP_END ||
            t->kind == TOP_RULE  || t->kind == TOP_STMT) continue;

        xf_Value tv = interp_eval_top(it, t);
        xf_value_release(tv);

        if (it->exiting || it->had_error) return it->had_error ? 1 : 0;
    }

    for (size_t i = 0; i < prog->count; i++) {
        if (prog->items[i]->kind != TOP_BEGIN) continue;

        xf_Value tv = interp_eval_top(it, prog->items[i]);
        xf_value_release(tv);
        xf_gc_maybe_collect(it->vm, it->syms, it);
        if (it->had_error) return 1;
        if (it->exiting) break;
    }

    return it->had_error ? 1 : 0;
}
void interp_run_end(Interp *it, Program *prog) {
    if (it->had_error) return;
    for (size_t i = 0; i < prog->count; i++) {
        if (prog->items[i]->kind != TOP_END) continue;
        it->exiting = false;

        xf_Value tv = interp_eval_top(it, prog->items[i]);
        xf_value_release(tv);
        xf_gc_maybe_collect(it->vm, it->syms, it);
        if (it->had_error) return;
    }
    vm_redir_flush(it->vm);
}

/* Print all fields of the current record through the active output format.
 * Used for format-conversion passthrough (e.g. -f csv:json data.csv). */
void interp_print_record(Interp *it) {
    RecordCtx *_rc = IT_REC(it);
    size_t count = _rc->field_count;
    if (count == 0) return;

    xf_Value *vals = malloc(sizeof(xf_Value) * count);
    if (!vals) return;
    for (size_t i = 0; i < count; i++) {
        xf_Str *s = xf_str_from_cstr(_rc->fields[i]);
        vals[i] = xf_val_ok_str(s);
        xf_str_release(s);
    }
    print_structured(it, vals, count, _rc->out_mode);
    for (size_t i = 0; i < count; i++) xf_value_release(vals[i]);
    free(vals);
}
void interp_feed_record(Interp *it, Program *prog,
                        const char *rec, size_t len) {
    vm_split_record(it->vm, rec, len);

    if (IT_REC(it)->out_mode == XF_OUTFMT_JSON &&
        !IT_REC(it)->headers_set) {
        vm_capture_headers(it->vm);
        return;
    }

    for (size_t i = 0; i < prog->count; i++) {
        TopLevel *t = prog->items[i];

        if (t->kind == TOP_RULE) {
            if (t->as.rule.pattern) {
                xf_Value pat = interp_eval_expr(it, t->as.rule.pattern);
                bool ok = is_truthy(pat);
                xf_value_release(pat);
                if (!ok) continue;
            }

            it->nexting = false;
            xf_Value rv = interp_eval_stmt(it, t->as.rule.body);
            xf_value_release(rv);

            if (it->nexting) { it->nexting = false; break; }
            if (it->exiting || it->had_error) return;
        } else if (t->kind == TOP_STMT) {
            it->nexting = false;
            xf_Value rv = interp_eval_top(it, t);
            xf_value_release(rv);

            if (it->nexting) { it->nexting = false; break; }
            if (it->exiting || it->had_error) return;
        }
    }
}
bool interp_compile_program(Interp *it, Program *prog) {
    (void)it; (void)prog;
    return true;
}
Chunk *interp_compile_expr(Interp *it, Expr *e, const char *name) {
    (void)it; (void)e;
    Chunk *c = malloc(sizeof(Chunk));
    chunk_init(c, name);
    chunk_write(c, OP_HALT, 0);
    return c;
}
Chunk *interp_compile_stmt(Interp *it, Stmt *s, const char *name) {
    (void)it; (void)s;
    Chunk *c = malloc(sizeof(Chunk));
    chunk_init(c, name );
    chunk_write(c, OP_HALT, 0);
    return c;
}