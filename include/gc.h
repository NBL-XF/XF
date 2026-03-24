#ifndef XF_GC_H
#define XF_GC_H

#include <stddef.h>
#include <stdbool.h>
#include "value.h"
#include "symTable.h"
#include "vm.h"
#include "interp.h"

typedef enum {
    XF_GC_OBJ_STR = 1,
    XF_GC_OBJ_ARR,
    XF_GC_OBJ_TUPLE,
    XF_GC_OBJ_MAP,
    XF_GC_OBJ_FN,
    XF_GC_OBJ_REGEX,
    XF_GC_OBJ_MODULE
} xf_GcObjKind;

typedef struct {
    size_t tracked;
    size_t marked;
    size_t swept;
    size_t allocs_since_last;
    size_t threshold;
    bool   auto_enabled;
} xf_GcStats;

void xf_gc_register_obj(void *ptr, xf_GcObjKind kind);
void xf_gc_unregister_obj(void *ptr);
void xf_gc_note_alloc(void);

void xf_gc_mark_value(xf_Value v);
void xf_gc_mark_syms(SymTable *st);
void xf_gc_mark_vm(VM *vm);
void xf_gc_mark_interp(Interp *it);

void xf_gc_collect(VM *vm, SymTable *syms, Interp *it);
void xf_gc_maybe_collect(VM *vm, SymTable *syms, Interp *it);

void   xf_gc_set_threshold(size_t threshold);
size_t xf_gc_get_threshold(void);
void   xf_gc_set_auto_enabled(bool enabled);
bool   xf_gc_auto_enabled(void);
xf_GcStats xf_gc_stats(void);

#endif /* XF_GC_H */