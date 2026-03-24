#include "internal.h"

/* ── arg coercion helpers ─────────────────────────────────────── */

bool arg_num(xf_Value *args, size_t argc, size_t i, double *out) {
    if (i >= argc) return false;
    xf_Value v = args[i];
    if (v.state != XF_STATE_OK) return false;
    if (v.type == XF_TYPE_NUM) { *out = v.data.num; return true; }

    xf_Value c = xf_coerce_num(v);
    if (c.state == XF_STATE_OK) {
        *out = c.data.num;
        xf_value_release(c);
        return true;
    }
    xf_value_release(c);
    return false;
}
bool arg_str(xf_Value *args, size_t argc, size_t i,
             const char **out, size_t *outlen) {
    enum { ARG_STR_SLOTS = 16 };
    static _Thread_local xf_Value slots[ARG_STR_SLOTS];
    static _Thread_local bool     inited    = false;
    static _Thread_local size_t   next_slot = 0;
    if (!inited) {
        for (size_t k = 0; k < ARG_STR_SLOTS; k++) slots[k] = xf_val_null();
        inited = true;
    }
    if (out)    *out    = "";
    if (outlen) *outlen = 0;
    if (i >= argc) return false;
    xf_Value v = args[i];
    if (v.state != XF_STATE_OK) return false;
    xf_Value c = xf_coerce_str(v);
    if (c.state != XF_STATE_OK) return false;
    size_t slot = next_slot;
    next_slot = (next_slot + 1u) % ARG_STR_SLOTS;
    xf_value_release(slots[slot]);
    slots[slot] = c;
    if (slots[slot].data.str) {
        if (out)    *out    = slots[slot].data.str->data;
        if (outlen) *outlen = slots[slot].data.str->len;
    }
    return true;
}

xf_Value propagate(xf_Value *args, size_t argc) {
    for (size_t i = 0; i < argc; i++)
        if (args[i].state != XF_STATE_OK) return args[i];
    return xf_val_nav(XF_TYPE_VOID);
}

xf_Value make_str_val(const char *data, size_t len) {
    xf_Str *s = xf_str_new(data, len);
    xf_Value v = xf_val_ok_str(s);
    xf_str_release(s);
    return v;
}

/* ── fn-caller context ────────────────────────────────────────── */

static xf_fn_caller_t  g_fn_caller      = NULL;
static void           *g_fn_caller_vm   = NULL;
static void           *g_fn_caller_syms = NULL;
/*
 * g_root_syms: captured once on the very first core_set_fn_caller call
 * that provides a real callback.  At that moment the VM is registering
 * the interpreter at startup with the global SymTable, so this pointer
 * stays valid for the lifetime of the process and always contains the
 * root scope (where 'core' lives).
 *
 * g_fn_caller_syms by contrast is overwritten on every XF function
 * invocation with whatever local frame is currently active, so it must
 * not be used when the callee needs access to globals.
 */
static void           *g_root_syms      = NULL;

static _Thread_local xf_fn_caller_t  tl_fn_caller      = NULL;
static _Thread_local void           *tl_fn_caller_vm   = NULL;
static _Thread_local void           *tl_fn_caller_syms = NULL;

void core_set_fn_caller(void *vm, void *syms, xf_fn_caller_t caller) {
    tl_fn_caller_vm   = vm;
    tl_fn_caller_syms = syms;
    if (caller) tl_fn_caller = caller;
    if (caller) {
        g_fn_caller      = caller;
        g_fn_caller_vm   = vm;
        g_fn_caller_syms = syms;
        /* Capture root symtable on first real registration only */
        if (!g_root_syms && syms) g_root_syms = syms;
    } else {
        if (vm)   g_fn_caller_vm   = vm;
        if (syms) g_fn_caller_syms = syms;
    }
}

xf_fn_caller_t core_get_fn_caller(void)      { return tl_fn_caller      ? tl_fn_caller      : g_fn_caller;      }
void          *core_get_fn_caller_vm(void)   { return tl_fn_caller_vm   ? tl_fn_caller_vm   : g_fn_caller_vm;   }
void          *core_get_fn_caller_syms(void) { return tl_fn_caller_syms ? tl_fn_caller_syms : g_fn_caller_syms; }
void          *core_get_root_syms(void)      { return g_root_syms ? g_root_syms : g_fn_caller_syms; }