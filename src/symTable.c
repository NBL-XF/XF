#include "../include/symTable.h"
#include "../include/value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Scope alloc / free
 * ============================================================ */

static Scope *scope_new(ScopeKind kind, Scope *parent, uint8_t fn_ret_type) {
    Scope *sc = calloc(1, sizeof(Scope));
    if (!sc) return NULL;

    sc->kind        = kind;
    sc->capacity    = SCOPE_INIT_CAP;
    sc->parent      = parent;
    sc->fn_ret_type = fn_ret_type;
    sc->entries     = calloc(sc->capacity, sizeof(Symbol));

    if (!sc->entries) {
        free(sc);
        return NULL;
    }

    return sc;
}

static void scope_free(Scope *sc) {
    if (!sc) return;

    for (size_t i = 0; i < sc->capacity; i++) {
        Symbol *s = &sc->entries[i];
        if (!s->name) continue;

        xf_str_release(s->name);
        xf_value_release(s->value);
        s->name = NULL;
    }

    free(sc->entries);
    free(sc);
}


/* ============================================================
 * SymTable init / free
 * ============================================================ */

void sym_init(SymTable *st) {
    memset(st, 0, sizeof(*st));

    st->global = scope_new(SCOPE_GLOBAL, NULL, XF_TYPE_VOID);
    if (!st->global) {
        st->had_error = true;
        snprintf(st->err_msg, sizeof(st->err_msg),
                 "failed to allocate global scope");
        return;
    }

    st->current = st->global;
    st->depth   = 0;
}

void sym_free(SymTable *st) {
    Scope *sc = st->current;
    while (sc) {
        Scope *parent = sc->parent;
        scope_free(sc);
        sc = parent;
    }

    st->current = NULL;
    st->global  = NULL;
    st->depth   = 0;
}


/* ============================================================
 * Scope push / pop
 * ============================================================ */

Scope *sym_push(SymTable *st, ScopeKind kind) {
    if (!st || !st->current) return NULL;

    uint8_t fn_ret_type =
        (kind == SCOPE_FN) ? XF_TYPE_VOID : st->current->fn_ret_type;

    Scope *sc = scope_new(kind, st->current, fn_ret_type);
    if (!sc) {
        st->had_error = true;
        snprintf(st->err_msg, sizeof(st->err_msg),
                 "failed to allocate scope");
        return NULL;
    }

    st->current = sc;
    st->depth++;
    return sc;
}

bool sym_pop(SymTable *st) {
    if (!st || !st->current) return false;
    if (st->current == st->global) return false;

    Scope *popped = st->current;
    st->current = popped->parent;
    st->depth--;

    scope_free(popped);
    return true;
}


/* ============================================================
 * Internal hash lookup within a single scope
 * ============================================================ */

static uint32_t hash_str_raw(const char *s, size_t len) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned char)s[i];
        h *= 16777619u;
    }
    return h;
}

static Symbol *scope_find(Scope *sc, const char *name, size_t len) {
    if (!sc || !name || sc->count == 0) return NULL;

    uint32_t h   = hash_str_raw(name, len);
    size_t   idx = h & (sc->capacity - 1);

    for (size_t probe = 0; probe < sc->capacity; probe++) {
        Symbol *s = &sc->entries[idx];

        if (!s->name) return NULL;

        if (s->name->len == len &&
            memcmp(s->name->data, name, len) == 0) {
            return s;
        }

        idx = (idx + 1) & (sc->capacity - 1);
    }

    return NULL;
}

static bool scope_grow(Scope *sc) {
    size_t new_cap = sc->capacity * 2;
    Symbol *new_entries = calloc(new_cap, sizeof(Symbol));
    if (!new_entries) return false;

    for (size_t i = 0; i < sc->capacity; i++) {
        Symbol *old = &sc->entries[i];
        if (!old->name) continue;

        uint32_t h   = xf_str_hash(old->name);
        size_t   idx = h & (new_cap - 1);

        while (new_entries[idx].name) {
            idx = (idx + 1) & (new_cap - 1);
        }

        new_entries[idx] = *old;
    }

    free(sc->entries);
    sc->entries  = new_entries;
    sc->capacity = new_cap;
    return true;
}

static Symbol *scope_insert(Scope *sc, xf_Str *name) {
    if (!sc || !name) return NULL;

    if (sc->count * 4 >= sc->capacity * 3) {
        if (!scope_grow(sc)) return NULL;
    }

    uint32_t h   = xf_str_hash(name);
    size_t   idx = h & (sc->capacity - 1);

    while (sc->entries[idx].name) {
        idx = (idx + 1) & (sc->capacity - 1);
    }

    sc->entries[idx].name = xf_str_retain(name);
    sc->count++;
    return &sc->entries[idx];
}


/* ============================================================
 * Symbol operations
 * ============================================================ */

Symbol *sym_declare(SymTable *st, xf_Str *name, SymKind kind,
                    uint8_t type, Loc loc) {
    if (!st || !st->current || !name) return NULL;

    Symbol *existing = scope_find(st->current, name->data, name->len);
    if (existing) {
        st->had_error = true;
        st->err_loc   = loc;
        snprintf(st->err_msg, sizeof(st->err_msg),
                 "'%s' already declared in this scope (previous at %s:%u:%u)",
                 name->data,
                 existing->decl_loc.source ? existing->decl_loc.source : "<unknown>",
                 existing->decl_loc.line,
                 existing->decl_loc.col);
        return NULL;
    }

    Symbol *sym = scope_insert(st->current, name);
    if (!sym) {
        st->had_error = true;
        st->err_loc   = loc;
        snprintf(st->err_msg, sizeof(st->err_msg),
                 "failed to insert symbol '%s'", name->data);
        return NULL;
    }

    sym->kind       = kind;
    sym->type       = type;
    sym->is_const   = false;
    sym->is_defined = false;
    sym->decl_loc   = loc;
    sym->value      = xf_val_undef(type);

    return sym;
}

Symbol *sym_lookup(SymTable *st, const char *name, size_t len) {
    if (!st || !name) return NULL;

    Scope *sc = st->current;
    while (sc) {
        Symbol *s = scope_find(sc, name, len);
        if (s) return s;
        sc = sc->parent;
    }

    return NULL;
}

Symbol *sym_lookup_str(SymTable *st, xf_Str *name) {
    if (!name) return NULL;
    return sym_lookup(st, name->data, name->len);
}

Symbol *sym_lookup_local(SymTable *st, const char *name, size_t len) {
    if (!st || !st->current) return NULL;
    return scope_find(st->current, name, len);
}

Symbol *sym_lookup_local_str(SymTable *st, xf_Str *name) {
    if (!st || !st->current || !name) return NULL;
    return scope_find(st->current, name->data, name->len);
}

bool sym_assign(SymTable *st, xf_Str *name, xf_Value val) {
    if (!st || !name) return false;

    Symbol *sym = sym_lookup_str(st, name);
    if (!sym) {
        st->had_error = true;
        snprintf(st->err_msg, sizeof(st->err_msg),
                 "assignment to undetermined variable '%s'", name->data);
        return false;
    }

    if (sym->is_const && sym->is_defined) {
        st->had_error = true;
        snprintf(st->err_msg, sizeof(st->err_msg),
                 "'%s' is immutable", name->data);
        return false;
    }

    xf_value_release(sym->value);
    sym->value      = xf_value_retain(val);
    sym->is_defined = true;

    return true;
}

Symbol *sym_define_builtin(SymTable *st, const char *name,
                           uint8_t type, xf_Value val) {
    if (!st || !st->global || !name) return NULL;

    xf_Str *s = xf_str_from_cstr(name);
    if (!s) return NULL;

    Symbol *sym = scope_insert(st->global, s);
    xf_str_release(s);

    if (!sym) {
        st->had_error = true;
        snprintf(st->err_msg, sizeof(st->err_msg),
                 "failed to define builtin '%s'", name);
        return NULL;
    }

    sym->kind       = SYM_BUILTIN;
    sym->type       = type;
    sym->is_const   = true;
    sym->is_defined = true;
    sym->value      = xf_value_retain(val);

    return sym;
}


/* ============================================================
 * Scope query helpers
 * ============================================================ */

bool sym_in_fn(const SymTable *st) {
    if (!st) return false;

    Scope *sc = st->current;
    while (sc) {
        if (sc->kind == SCOPE_FN) return true;
        sc = sc->parent;
    }
    return false;
}

Scope *sym_fn_scope(SymTable *st) {
    if (!st) return NULL;

    Scope *sc = st->current;
    while (sc) {
        if (sc->kind == SCOPE_FN) return sc;
        sc = sc->parent;
    }
    return NULL;
}

bool sym_in_loop(const SymTable *st) {
    if (!st) return false;

    Scope *sc = st->current;
    while (sc && sc->kind != SCOPE_FN) {
        if (sc->kind == SCOPE_LOOP) return true;
        sc = sc->parent;
    }
    return false;
}

uint8_t sym_fn_return_type(const SymTable *st) {
    Scope *sc = sym_fn_scope((SymTable *)st);
    return sc ? sc->fn_ret_type : XF_TYPE_VOID;
}


/* ============================================================
 * Built-in registration
 * ============================================================ */

static const char *const k_builtins[] = {
    "sin", "cos", "sqrt", "abs", "int",
    "len", "split", "sub", "gsub", "match", "substr", "index",
    "toupper", "tolower", "trim", "sprintf", "column",
    "getline", "close", "flush",
    "push", "pop", "shift", "unshift", "remove", "has", "keys", "values",
    "read", "lines", "write", "append",
    "system", "time", "rand", "srand", "exit",
};

#define BUILTIN_COUNT (sizeof(k_builtins) / sizeof(k_builtins[0]))

void sym_register_builtins(SymTable *st) {
    xf_Value fn_undef = xf_val_undef(XF_TYPE_FN);

    for (size_t i = 0; i < BUILTIN_COUNT; i++) {
        sym_define_builtin(st, k_builtins[i], XF_TYPE_FN, fn_undef);
    }
}


/* ============================================================
 * Debug
 * ============================================================ */

static const char *scope_kind_name(ScopeKind k) {
    switch (k) {
        case SCOPE_GLOBAL:  return "global";
        case SCOPE_FN:      return "fn";
        case SCOPE_BLOCK:   return "block";
        case SCOPE_LOOP:    return "loop";
        case SCOPE_PATTERN: return "pattern";
        default:            return "?";
    }
}

void sym_print_scope(const Scope *sc) {
    if (!sc) return;

    printf("scope[%s] (%zu entries)\n", scope_kind_name(sc->kind), sc->count);
    for (size_t i = 0; i < sc->capacity; i++) {
        const Symbol *s = &sc->entries[i];
        if (!s->name) continue;

        printf("  %-20s  type=%-8s state=%-10s %s\n",
               s->name->data,
               XF_TYPE_NAMES[s->type < XF_TYPE_COUNT ? s->type : 0],
               XF_STATE_NAMES[s->value.state < XF_STATE_COUNT ? s->value.state : 0],
               s->is_const ? "const" : "");
    }
}

void sym_print_all(const SymTable *st) {
    if (!st) return;

    Scope *sc = st->current;
    int depth = (int)st->depth;

    while (sc) {
        printf("[depth %d] ", depth--);
        sym_print_scope(sc);
        sc = sc->parent;
    }
}