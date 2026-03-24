#include "../include/symTable.h"
#include "../include/vm.h"
#include "../include/interp.h"
#include "../include/value.h"
#include "../include/gc.h"
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>

/* ------------------------------------------------------------
 * External GC registry
 * ------------------------------------------------------------ */

typedef struct xf_GcNode {
    void               *ptr;
    xf_GcObjKind        kind;
    bool                marked;
    struct xf_GcNode   *next;
} xf_GcNode;

static xf_GcNode *g_gc_head = NULL;
static pthread_mutex_t g_gc_mu = PTHREAD_MUTEX_INITIALIZER;

static _Atomic size_t g_gc_tracked           = 0;
static _Atomic size_t g_gc_marked_count      = 0;
static _Atomic size_t g_gc_swept_last        = 0;
static _Atomic size_t g_gc_allocs_since_last = 0;
static _Atomic size_t g_gc_threshold         = 4096;
static _Atomic bool   g_gc_auto_enabled      = true;

/* ------------------------------------------------------------
 * registry helpers
 * ------------------------------------------------------------ */

static xf_GcNode *gc_find_locked(void *ptr) {
    for (xf_GcNode *n = g_gc_head; n; n = n->next) {
        if (n->ptr == ptr) return n;
    }
    return NULL;
}

void xf_gc_register_obj(void *ptr, xf_GcObjKind kind) {
    if (!ptr) return;

    pthread_mutex_lock(&g_gc_mu);

    if (!gc_find_locked(ptr)) {
        xf_GcNode *n = (xf_GcNode *)calloc(1, sizeof(*n));
        if (n) {
            n->ptr    = ptr;
            n->kind   = kind;
            n->marked = false;
            n->next   = g_gc_head;
            g_gc_head = n;
            atomic_fetch_add(&g_gc_tracked, 1);
        }
    }

    pthread_mutex_unlock(&g_gc_mu);
}

void xf_gc_unregister_obj(void *ptr) {
    if (!ptr) return;

    pthread_mutex_lock(&g_gc_mu);

    xf_GcNode **cur = &g_gc_head;
    while (*cur) {
        if ((*cur)->ptr == ptr) {
            xf_GcNode *dead = *cur;
            *cur = dead->next;
            free(dead);
            atomic_fetch_sub(&g_gc_tracked, 1);
            break;
        }
        cur = &(*cur)->next;
    }

    pthread_mutex_unlock(&g_gc_mu);
}

void xf_gc_note_alloc(void) {
    atomic_fetch_add(&g_gc_allocs_since_last, 1);
}

/* ------------------------------------------------------------
 * mark helpers
 * ------------------------------------------------------------ */

static void gc_mark_ptr(void *ptr) {
    if (!ptr) return;

    pthread_mutex_lock(&g_gc_mu);
    xf_GcNode *n = gc_find_locked(ptr);
    if (n && !n->marked) {
        n->marked = true;
        atomic_fetch_add(&g_gc_marked_count, 1);
    }
    pthread_mutex_unlock(&g_gc_mu);
}

static bool gc_is_marked_ptr(void *ptr) {
    bool out = false;
    if (!ptr) return false;

    pthread_mutex_lock(&g_gc_mu);
    xf_GcNode *n = gc_find_locked(ptr);
    out = (n && n->marked);
    pthread_mutex_unlock(&g_gc_mu);

    return out;
}

void xf_gc_mark_value(xf_Value v) {
    if (v.state != XF_STATE_OK) return;

    switch (v.type) {
        case XF_TYPE_STR:
            if (!v.data.str || gc_is_marked_ptr(v.data.str)) {
                if (v.data.str) gc_mark_ptr(v.data.str);
                return;
            }
            gc_mark_ptr(v.data.str);
            return;

        case XF_TYPE_ARR: {
            xf_arr_t *a = v.data.arr;
            if (!a || gc_is_marked_ptr(a)) {
                if (a) gc_mark_ptr(a);
                return;
            }
            gc_mark_ptr(a);
            for (size_t i = 0; i < a->len; i++)
                xf_gc_mark_value(a->items[i]);
            return;
        }

        case XF_TYPE_TUPLE: {
            xf_tuple_t *t = v.data.tuple;
            if (!t || gc_is_marked_ptr(t)) {
                if (t) gc_mark_ptr(t);
                return;
            }
            gc_mark_ptr(t);
            size_t n = xf_tuple_len(t);
            for (size_t i = 0; i < n; i++)
                xf_gc_mark_value(xf_tuple_get(t, i));
            return;
        }

        case XF_TYPE_MAP:
        case XF_TYPE_SET: {
            xf_map_t *m = v.data.map;
            if (!m || gc_is_marked_ptr(m)) {
                if (m) gc_mark_ptr(m);
                return;
            }
            gc_mark_ptr(m);
            for (size_t i = 0; i < m->order_len; i++) {
                xf_str_t *k = m->order[i];
                if (k) gc_mark_ptr(k);
                xf_gc_mark_value(xf_map_get(m, k));
            }
            return;
        }

        case XF_TYPE_FN: {
            xf_fn_t *f = v.data.fn;
            if (!f || gc_is_marked_ptr(f)) {
                if (f) gc_mark_ptr(f);
                return;
            }
            gc_mark_ptr(f);
            if (f->name) gc_mark_ptr(f->name);
            if (f->params) {
                for (size_t i = 0; i < f->param_count; i++) {
                    if (f->params[i].name) gc_mark_ptr(f->params[i].name);
                    if (f->params[i].has_default && f->params[i].default_val)
                        xf_gc_mark_value(*f->params[i].default_val);
                }
            }
            return;
        }

        case XF_TYPE_REGEX: {
            xf_regex_t *r = v.data.re;
            if (!r || gc_is_marked_ptr(r)) {
                if (r) gc_mark_ptr(r);
                return;
            }
            gc_mark_ptr(r);
            if (r->pattern) gc_mark_ptr(r->pattern);
            return;
        }

        case XF_TYPE_MODULE: {
            xf_module_t *m = v.data.mod;
            if (!m || gc_is_marked_ptr(m)) {
                if (m) gc_mark_ptr(m);
                return;
            }
            gc_mark_ptr(m);
            for (size_t i = 0; i < m->count; i++)
                xf_gc_mark_value(m->entries[i].val);
            return;
        }

        default:
            return;
    }
}

void xf_gc_mark_syms(SymTable *st) {
    if (!st) return;

    for (Scope *sc = st->current; sc; sc = sc->parent) {
        for (size_t i = 0; i < sc->capacity; i++) {
            Symbol *sym = &sc->entries[i];
            if (!sym->name) continue;
            gc_mark_ptr(sym->name);
            xf_gc_mark_value(sym->value);
        }
    }
}

void xf_gc_mark_vm(VM *vm) {
    if (!vm) return;

    for (size_t i = 0; i < vm->stack_top; i++)
        xf_gc_mark_value(vm->stack[i]);

    for (size_t i = 0; i < vm->frame_count; i++) {
        CallFrame *fr = &vm->frames[i];

        for (size_t j = 0; j < fr->local_count; j++)
            xf_gc_mark_value(fr->locals[j]);

        xf_gc_mark_value(fr->return_val);
    }

    for (size_t i = 0; i < vm->global_count; i++)
        xf_gc_mark_value(vm->globals[i]);

    xf_gc_mark_value(vm->rec.last_match);
    xf_gc_mark_value(vm->rec.last_captures);
    xf_gc_mark_value(vm->rec.last_err);
}

void xf_gc_mark_interp(Interp *it) {
    if (!it) return;

    xf_gc_mark_value(it->return_val);
    xf_gc_mark_value(it->last_err);

    if (it->use_rec_snap) {
        xf_gc_mark_value(it->rec_snap.last_match);
        xf_gc_mark_value(it->rec_snap.last_captures);
        xf_gc_mark_value(it->rec_snap.last_err);
    }
}

/* ------------------------------------------------------------
 * sweep helpers
 * ------------------------------------------------------------ */

typedef struct {
    void        *ptr;
    xf_GcObjKind kind;
} xf_GcDoomed;

static void gc_force_release(void *ptr, xf_GcObjKind kind) {
    if (!ptr) return;

    switch (kind) {
        case XF_GC_OBJ_STR:
            atomic_store(&((xf_str_t *)ptr)->refcount, 1);
            xf_str_release((xf_str_t *)ptr);
            break;

        case XF_GC_OBJ_ARR:
            atomic_store(&((xf_arr_t *)ptr)->refcount, 1);
            xf_arr_release((xf_arr_t *)ptr);
            break;

        case XF_GC_OBJ_TUPLE:
    /* tuple internals not visible here — skip forced release for now */
    xf_tuple_release((xf_tuple_t *)ptr);
    break;
        case XF_GC_OBJ_MAP:
            atomic_store(&((xf_map_t *)ptr)->refcount, 1);
            xf_map_release((xf_map_t *)ptr);
            break;

        case XF_GC_OBJ_FN:
            atomic_store(&((xf_fn_t *)ptr)->refcount, 1);
            xf_fn_release((xf_fn_t *)ptr);
            break;

        case XF_GC_OBJ_REGEX:
            atomic_store(&((xf_regex_t *)ptr)->refcount, 1);
            xf_regex_release((xf_regex_t *)ptr);
            break;

        case XF_GC_OBJ_MODULE:
            atomic_store(&((xf_module_t *)ptr)->refcount, 1);
            xf_module_release((xf_module_t *)ptr);
            break;

        default:
            break;
    }
}

static size_t gc_collect_doomed(xf_GcDoomed **out) {
    *out = NULL;

    pthread_mutex_lock(&g_gc_mu);

    size_t doomed_count = 0;
    for (xf_GcNode *n = g_gc_head; n; n = n->next) {
        if (!n->marked) doomed_count++;
    }

    if (doomed_count == 0) {
        pthread_mutex_unlock(&g_gc_mu);
        return 0;
    }

    xf_GcDoomed *doomed = (xf_GcDoomed *)calloc(doomed_count, sizeof(*doomed));
    if (!doomed) {
        pthread_mutex_unlock(&g_gc_mu);
        return 0;
    }

    size_t idx = 0;
    xf_GcNode **cur = &g_gc_head;
    while (*cur) {
        if (!(*cur)->marked) {
            xf_GcNode *dead = *cur;
            *cur = dead->next;

            doomed[idx].ptr  = dead->ptr;
            doomed[idx].kind = dead->kind;
            idx++;

            free(dead);
            atomic_fetch_sub(&g_gc_tracked, 1);
        } else {
            (*cur)->marked = false; /* clear mark for next cycle */
            cur = &(*cur)->next;
        }
    }

    pthread_mutex_unlock(&g_gc_mu);

    *out = doomed;
    return idx;
}

static void gc_clear_marks_only(void) {
    pthread_mutex_lock(&g_gc_mu);
    for (xf_GcNode *n = g_gc_head; n; n = n->next)
        n->marked = false;
    pthread_mutex_unlock(&g_gc_mu);
}

/* ------------------------------------------------------------
 * public collect
 * ------------------------------------------------------------ */

void xf_gc_collect(VM *vm, SymTable *syms, Interp *it) {
    atomic_store(&g_gc_marked_count, 0);
    atomic_store(&g_gc_swept_last, 0);

    gc_clear_marks_only();

    xf_gc_mark_syms(syms);
    xf_gc_mark_vm(vm);
    xf_gc_mark_interp(it);

    xf_GcDoomed *doomed = NULL;
    size_t n = gc_collect_doomed(&doomed);

    for (size_t i = 0; i < n; i++)
        gc_force_release(doomed[i].ptr, doomed[i].kind);

    free(doomed);

    atomic_store(&g_gc_swept_last, n);
    atomic_store(&g_gc_allocs_since_last, 0);
}

void xf_gc_maybe_collect(VM *vm, SymTable *syms, Interp *it) {
    if (!atomic_load(&g_gc_auto_enabled)) return;
    if (atomic_load(&g_gc_allocs_since_last) < atomic_load(&g_gc_threshold)) return;
    xf_gc_collect(vm, syms, it);
}

/* ------------------------------------------------------------
 * tuning / stats
 * ------------------------------------------------------------ */

void xf_gc_set_threshold(size_t threshold) {
    if (threshold == 0) threshold = 1;
    atomic_store(&g_gc_threshold, threshold);
}

size_t xf_gc_get_threshold(void) {
    return atomic_load(&g_gc_threshold);
}

void xf_gc_set_auto_enabled(bool enabled) {
    atomic_store(&g_gc_auto_enabled, enabled);
}

bool xf_gc_auto_enabled(void) {
    return atomic_load(&g_gc_auto_enabled);
}

xf_GcStats xf_gc_stats(void) {
    xf_GcStats s;
    s.tracked           = atomic_load(&g_gc_tracked);
    s.marked            = atomic_load(&g_gc_marked_count);
    s.swept             = atomic_load(&g_gc_swept_last);
    s.allocs_since_last = atomic_load(&g_gc_allocs_since_last);
    s.threshold         = atomic_load(&g_gc_threshold);
    s.auto_enabled      = atomic_load(&g_gc_auto_enabled);
    return s;
}