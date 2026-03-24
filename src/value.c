#include "../include/value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <regex.h>
struct xf_tuple {
    _Atomic uint32_t refcount;
    size_t len;
    xf_Value *items;
};
/* ============================================================
 * String implementation
 * ============================================================ */

xf_str_t *xf_str_new(const char *data, size_t len) {
    xf_str_t *s = malloc(sizeof(xf_str_t) + len + 1);
    if (!s) return NULL;
    atomic_store(&s->refcount, 1);
    s->len  = len;
    s->hash = 0;
    memcpy(s->data, data, len);
    s->data[len] = '\0';
    return s;
}

xf_str_t *xf_str_from_cstr(const char *cstr) {
    return xf_str_new(cstr, strlen(cstr));
}

xf_str_t *xf_str_retain(xf_str_t *s) {
    if (s) atomic_fetch_add(&s->refcount, 1);
    return s;
}

void xf_str_release(xf_str_t *s) {
    if (!s) return;
    if (atomic_fetch_sub(&s->refcount, 1) == 1)
        free(s);
}

/* FNV-1a hash */
uint32_t xf_str_hash(xf_str_t *s) {
    if (s->hash) return s->hash;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < s->len; i++) {
        h ^= (unsigned char)s->data[i];
        h *= 16777619u;
    }
    s->hash = h ? h : 1; /* 0 reserved for "not computed" */
    return s->hash;
}

int xf_str_cmp(const xf_str_t *a, const xf_str_t *b) {
    if (a == b) return 0;
    size_t min = a->len < b->len ? a->len : b->len;
    int r = memcmp(a->data, b->data, min);
    if (r) return r;
    return (int)(a->len - b->len);
}


/* ============================================================
 * Error implementation
 * ============================================================ */

xf_err_t *xf_err_new(const char *msg, const char *src,
                     uint32_t line, uint32_t col) {
    xf_err_t *e = calloc(1, sizeof(xf_err_t));
    if (!e) return NULL;

    atomic_store(&e->refcount, 1);
    e->message = xf_str_from_cstr(msg ? msg : "unknown error");
    e->source  = xf_str_from_cstr(src ? src : "<unknown>");
    e->line    = line;
    e->col     = col;
    e->cause   = NULL;
    e->siblings = NULL;
    e->sibling_count = 0;

    return e;
}

xf_err_t *xf_err_retain(xf_err_t *e) {
    if (e) atomic_fetch_add(&e->refcount, 1);
    return e;
}

void xf_err_release(xf_err_t *e) {
    if (!e) return;
    if (atomic_fetch_sub(&e->refcount, 1) != 1) return;

    xf_str_release(e->message);
    xf_str_release(e->source);
    xf_err_release(e->cause);

    /* siblings is a shallow copied pointer list; do NOT free siblings[i] */
    free(e->siblings);

    free(e);
}

xf_err_t *xf_err_chain(xf_err_t *parent, xf_err_t *cause,
                       xf_value_t **siblings, size_t sibling_count) {
    if (!parent) return cause;

    /* replace existing cause safely */
    if (parent->cause) {
        xf_err_release(parent->cause);
        parent->cause = NULL;
    }

    parent->cause = xf_err_retain(cause);

    /* replace existing sibling pointer list safely */
    free(parent->siblings);
    parent->siblings = NULL;
    parent->sibling_count = 0;

    if (sibling_count && siblings) {
        parent->siblings = malloc(sizeof(xf_value_t *) * sibling_count);
        if (parent->siblings) {
            memcpy(parent->siblings, siblings,
                   sizeof(xf_value_t *) * sibling_count);
            parent->sibling_count = sibling_count;
        }
    }

    return parent;
}




/* ============================================================
 * Value constructors
 * ============================================================ */

xf_value_t xf_val_ok_num(double n) {
    return (xf_value_t){ .state = XF_STATE_OK,
                         .type  = XF_TYPE_NUM,
                         .data  = { .num = n } };
}

xf_value_t xf_val_ok_str(xf_str_t *s) {
    return (xf_value_t){ .state = XF_STATE_OK,
                         .type  = XF_TYPE_STR,
                         .data  = { .str = xf_str_retain(s) } };
}

xf_value_t xf_val_ok_map(xf_map_t *m) {
    return (xf_value_t){
        .state = XF_STATE_OK,
        .type  = XF_TYPE_MAP,
        .data  = { .map = xf_map_retain(m) }
    };
}

xf_value_t xf_val_ok_set(xf_set_t *s) {
    return (xf_value_t){
        .state = XF_STATE_OK,
        .type  = XF_TYPE_SET,
        .data  = { .set = (xf_set_t *)xf_map_retain((xf_map_t *)s) }
    };
}
size_t xf_tuple_len(const xf_tuple_t *t) {
    return t ? t->len : 0;
}
xf_tuple_t *xf_tuple_new(xf_Value *items, size_t len) {
    xf_tuple_t *t = calloc(1, sizeof(xf_tuple_t));
    if (!t) return NULL;

    atomic_store(&t->refcount, 1);
    t->len = len;
    t->items = len ? calloc(len, sizeof(xf_Value)) : NULL;

    if (len && !t->items) {
        free(t);
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        t->items[i] = items[i]; /* steals ownership from caller */
    }

    return t;
}
xf_tuple_t *xf_tuple_retain(xf_tuple_t *t) {
    if (t) atomic_fetch_add(&t->refcount, 1);
    return t;
}

void xf_tuple_release(xf_tuple_t *t) {
    if (!t) return;
    if (atomic_fetch_sub(&t->refcount, 1) != 1) return;

    for (size_t i = 0; i < t->len; i++) {
        xf_value_release(t->items[i]);
    }
    free(t->items);
    free(t);
}

xf_Value xf_tuple_get(const xf_tuple_t *t, size_t idx) {
    if (!t || idx >= t->len) return xf_val_nav(XF_TYPE_VOID);
    return t->items[idx];
}
xf_Value xf_val_ok_tuple(xf_tuple_t *t) {
    return (xf_Value){
        .state = XF_STATE_OK,
        .type  = XF_TYPE_TUPLE,
        .data  = { .tuple = xf_tuple_retain(t) }
    };
}
xf_value_t xf_val_ok_arr(xf_arr_t *a) {
    return (xf_value_t){
        .state = XF_STATE_OK,
        .type  = XF_TYPE_ARR,
        .data  = { .arr = xf_arr_retain(a) }
    };
}

xf_value_t xf_val_ok_module(xf_module_t *m) {
    return (xf_value_t){
        .state = XF_STATE_OK,
        .type  = XF_TYPE_MODULE,
        .data  = { .mod = xf_module_retain(m) }
    };
}
xf_value_t xf_val_ok_fn(xf_fn_t *f) {
    return (xf_value_t){
        .state = XF_STATE_OK,
        .type  = XF_TYPE_FN,
        .data  = { .fn = xf_fn_retain(f) }
    };
}
xf_value_t xf_val_native_fn(const char *name, uint8_t ret_type,
                             xf_value_t (*fn)(xf_value_t *args, size_t argc)) {
    xf_fn_t *f = calloc(1, sizeof(xf_fn_t));
    atomic_store(&f->refcount, 1);
    f->name        = name ? xf_str_from_cstr(name) : NULL;
    f->return_type = ret_type;
    f->is_native   = true;
    f->native_v    = fn;
    return xf_val_ok_fn(f);
}

/* ── fn retain / release ───────────────────────────────────── */
xf_fn_t *xf_fn_retain(xf_fn_t *f) {
    if (f) atomic_fetch_add(&f->refcount, 1);
    return f;
}
void xf_fn_release(xf_fn_t *f) {
    if (!f || atomic_fetch_sub(&f->refcount, 1) != 1) return;
    xf_str_release(f->name);
    /* params and body are owned by the AST, not by the fn value */
    free(f);
}

/* ── regex retain / release ─────────────────────────────────── */
xf_regex_t *xf_regex_retain(xf_regex_t *r) {
    if (r) atomic_fetch_add(&r->refcount, 1);
    return r;
}
void xf_regex_release(xf_regex_t *r) {
    if (!r || atomic_fetch_sub(&r->refcount, 1) != 1) return;
    xf_str_release(r->pattern);
    /* compiled is a POSIX regex_t allocated inline; regfree then free */
    if (r->compiled) {
        regfree((regex_t *)r->compiled);
        free(r->compiled);
    }
    free(r);
}
xf_Value xf_value_retain(xf_Value v) {
    if (v.state != XF_STATE_OK) return v;
    switch (v.type) {
        case XF_TYPE_STR:    xf_str_retain(v.data.str);       break;
        case XF_TYPE_ARR:    xf_arr_retain(v.data.arr);       break;
        case XF_TYPE_TUPLE:  xf_tuple_retain(v.data.tuple);   break;
        case XF_TYPE_MAP:
        case XF_TYPE_SET:    xf_map_retain(v.data.map);       break;
        case XF_TYPE_FN:     xf_fn_retain(v.data.fn);         break;
        case XF_TYPE_REGEX:  xf_regex_retain(v.data.re);      break;
        case XF_TYPE_MODULE: xf_module_retain(v.data.mod);    break;
        default: break;
    }
    return v;
}
void xf_value_release(xf_Value v) {
    if (v.state != XF_STATE_OK) return;
    switch (v.type) {
        case XF_TYPE_STR:    xf_str_release(v.data.str);      break;
        case XF_TYPE_ARR:    xf_arr_release(v.data.arr);      break;
        case XF_TYPE_TUPLE:  xf_tuple_release(v.data.tuple);  break;
        case XF_TYPE_MAP:
        case XF_TYPE_SET:    xf_map_release(v.data.map);      break;
        case XF_TYPE_FN:     xf_fn_release(v.data.fn);        break;
        case XF_TYPE_REGEX:  xf_regex_release(v.data.re);     break;
        case XF_TYPE_MODULE: xf_module_release(v.data.mod);   break;
        default: break;
    }
}

xf_value_t xf_val_ok_re(xf_regex_t *r) {
    return (xf_value_t){ .state = XF_STATE_OK,
                         .type  = XF_TYPE_REGEX,
                         .data  = { .re = r } };
}


/* ============================================================
 * xf_arr — dynamic array of xf_value_t
 * ============================================================ */

#define ARR_INIT_CAP 8

xf_arr_t *xf_arr_new(void) {
    xf_arr_t *a = calloc(1, sizeof(xf_arr_t));
    atomic_store(&a->refcount, 1);
    a->cap   = ARR_INIT_CAP;
    a->items = calloc(a->cap, sizeof(xf_value_t));
    return a;
}

xf_arr_t *xf_arr_retain(xf_arr_t *a) {
    if (a) atomic_fetch_add(&a->refcount, 1);
    return a;
}

void xf_arr_release(xf_arr_t *a) {
    if (!a) return;

    uint32_t old = atomic_fetch_sub(&a->refcount, 1);
    printf("xf_arr_release a=%p old_rc=%u new_rc=%u len=%zu\n",
           (void *)a, old, old - 1, a->len);

    if (old != 1) return;

    printf("FREE ARRAY a=%p len=%zu\n", (void *)a, a->len);

    for (size_t i = 0; i < a->len; i++) {
        printf("  arr=%p releasing item[%zu]\n", (void *)a, i);
        xf_value_release(a->items[i]);
    }

    free(a->items);
    free(a);
}
void xf_arr_push(xf_arr_t *a, xf_value_t v) {
    if (a->len >= a->cap) {
        a->cap   = a->cap ? a->cap * 2 : ARR_INIT_CAP;
        a->items = realloc(a->items, a->cap * sizeof(xf_value_t));
    }
    a->items[a->len++] = v;
}

xf_value_t xf_arr_get(const xf_arr_t *a, size_t idx) {
    if (!a || idx >= a->len) return xf_val_nav(XF_TYPE_VOID);
    return a->items[idx];
}

void xf_arr_set(xf_arr_t *a, size_t idx, xf_value_t v) {
    if (!a) return;
    if (idx >= a->cap) {
        size_t new_cap = a->cap ? a->cap : ARR_INIT_CAP;
        while (new_cap <= idx) new_cap *= 2;
        a->items = realloc(a->items, new_cap * sizeof(xf_value_t));
        for (size_t i = a->cap; i < new_cap; i++)
            a->items[i] = xf_val_nav(XF_TYPE_VOID);
        a->cap = new_cap;
    }
    if (idx < a->len) xf_value_release(a->items[idx]);  /* release old */
    a->items[idx] = v;   /* steal new */
    if (idx >= a->len) a->len = idx + 1;
}

xf_value_t xf_arr_pop(xf_arr_t *a) {
    if (!a || a->len == 0) return xf_val_nav(XF_TYPE_VOID);
    return a->items[--a->len];
}

void xf_arr_unshift(xf_arr_t *a, xf_value_t v) {
    if (!a) return;
    if (a->len >= a->cap) {
        a->cap = a->cap ? a->cap * 2 : ARR_INIT_CAP;
        a->items = realloc(a->items, a->cap * sizeof(xf_value_t));
    }
    memmove(a->items + 1, a->items, a->len * sizeof(xf_value_t));
    a->items[0] = v;
    a->len++;
}

xf_value_t xf_arr_shift(xf_arr_t *a) {
    if (!a || a->len == 0) return xf_val_nav(XF_TYPE_VOID);
    xf_value_t v = a->items[0];
    memmove(a->items, a->items + 1, (a->len - 1) * sizeof(xf_value_t));
    a->len--;
    return v;
}

void xf_arr_delete(xf_arr_t *a, size_t idx) {
    if (!a || idx >= a->len) return;
    xf_value_release(a->items[idx]);  /* release item being removed */
    memmove(a->items + idx, a->items + idx + 1, (a->len - idx - 1) * sizeof(xf_value_t));
    a->len--;
}


/* ============================================================
 * xf_map — open-addressing hash map  xf_str_t* → xf_value_t
 * ============================================================ */

#define MAP_INIT_CAP  16
#define MAP_LOAD_MAX  0.7   /* rehash when load > 70% */
xf_Value xf_val_ok_complex(double re, double im) {
    xf_Value v;
    memset(&v, 0, sizeof(v));
    v.state = XF_STATE_OK;
    v.type  = XF_TYPE_COMPLEX;
    v.data.complex.re = re;
    v.data.complex.im = im;
    return v;
}
static size_t map_find_slot(const xf_map_t *m, const xf_str_t *key) {
    uint32_t h    = xf_str_hash((xf_str_t *)key);
    size_t   mask = m->cap - 1;
    size_t   idx  = h & mask;
    for (;;) {
        xf_map_slot_t *sl = &m->slots[idx];
        if (!sl->key) return idx;               /* empty → insert here */
        if (xf_str_cmp(sl->key, key) == 0) return idx;  /* found */
        idx = (idx + 1) & mask;
    }
}

static void map_rehash(xf_map_t *m) {
    size_t new_cap = m->cap * 2;
    xf_map_slot_t *old = m->slots;
    size_t         old_cap = m->cap;
    m->slots = calloc(new_cap, sizeof(xf_map_slot_t));
    m->cap   = new_cap;
    m->used  = 0;
    for (size_t i = 0; i < old_cap; i++) {
        if (!old[i].key) continue;
        size_t idx = map_find_slot(m, old[i].key);
        m->slots[idx] = old[i];
        m->used++;
    }
    free(old);
    /* order array holds borrowed refs into slots — slots still live, order survives */
}

xf_map_t *xf_map_new(void) {
    xf_map_t *m = calloc(1, sizeof(xf_map_t));
    atomic_store(&m->refcount, 1);
    m->cap   = MAP_INIT_CAP;
    m->slots = calloc(m->cap, sizeof(xf_map_slot_t));
    return m;
}

xf_map_t *xf_map_retain(xf_map_t *m) {
    if (m) atomic_fetch_add(&m->refcount, 1);
    return m;
}

void xf_map_release(xf_map_t *m) {
    if (!m || atomic_fetch_sub(&m->refcount, 1) != 1) return;
    for (size_t i = 0; i < m->cap; i++) {
        if (m->slots[i].key) {
            xf_str_release(m->slots[i].key);
            xf_value_release(m->slots[i].val);  /* release val */
        }
    }
    free(m->slots);
    free(m->order);
    free(m);
}

xf_value_t xf_map_get(const xf_map_t *m, const xf_str_t *key) {
    if (!m || !key) return xf_val_nav(XF_TYPE_VOID);
    size_t idx = map_find_slot(m, key);
    if (!m->slots[idx].key) return xf_val_nav(XF_TYPE_VOID);
    return m->slots[idx].val;
}

void xf_map_set(xf_map_t *m, xf_str_t *key, xf_value_t val) {
    if (!m || !key) return;
    if ((double)m->used / (double)m->cap > MAP_LOAD_MAX) map_rehash(m);
    size_t idx = map_find_slot(m, key);
    bool is_new = !m->slots[idx].key;
    if (is_new) {
        m->slots[idx].key = xf_str_retain(key);
        m->used++;
        if (m->order_len == m->order_cap) {
            size_t nc = m->order_cap ? m->order_cap * 2 : 8;
            m->order = realloc(m->order, nc * sizeof(*m->order));
            m->order_cap = nc;
        }
        m->order[m->order_len++] = m->slots[idx].key;
    } else {
        xf_value_release(m->slots[idx].val);  /* release old val on overwrite */
    }
    m->slots[idx].val = val;  /* steal new val */
}

bool xf_map_delete(xf_map_t *m, const xf_str_t *key) {
    if (!m || !key) return false;
    size_t mask = m->cap - 1;
    uint32_t h  = xf_str_hash((xf_str_t *)key);
    size_t idx  = h & mask;
    for (;;) {
        if (!m->slots[idx].key) return false;
        if (xf_str_cmp(m->slots[idx].key, key) == 0) break;
        idx = (idx + 1) & mask;
    }
    xf_str_t *dead_key = m->slots[idx].key;
    xf_value_release(m->slots[idx].val);  /* release val before clearing slot */
    m->slots[idx].key = NULL;
    m->slots[idx].val = xf_val_nav(XF_TYPE_VOID);  /* clear val */
    m->used--;
    /* Robin Hood backshift */
    size_t j = (idx + 1) & mask;
    while (m->slots[j].key) {
        size_t natural = xf_str_hash(m->slots[j].key) & mask;
        if (natural != j) {
            m->slots[idx] = m->slots[j];
            m->slots[j].key = NULL;
            idx = j;
        }
        j = (j + 1) & mask;
    }
    /* remove from insertion-order list */
    for (size_t i = 0; i < m->order_len; i++) {
        if (m->order[i] == dead_key) {
            memmove(m->order + i, m->order + i + 1,
                    (m->order_len - i - 1) * sizeof(*m->order));
            m->order_len--;
            break;
        }
    }
    xf_str_release(dead_key);
    return true;
}

size_t xf_map_count(const xf_map_t *m) { return m ? m->used : 0; }

xf_value_t xf_val_err(xf_err_t *e, uint8_t type) {
    return (xf_value_t){ .state = XF_STATE_ERR,
                         .type  = type,
                         .err   = xf_err_retain(e) };
}

xf_value_t xf_val_nav(uint8_t expected_type) {
    return (xf_value_t){ .state = XF_STATE_NAV,
                         .type  = expected_type };
}

xf_value_t xf_val_null(void) {
    return (xf_value_t){ .state = XF_STATE_NULL,
                         .type  = XF_TYPE_VOID };
}

xf_value_t xf_val_void(xf_value_t inner) {
    /* wrap an unexpected return as VOID — preserves type */
    xf_value_t v  = inner;
    v.state        = XF_STATE_VOID;
    return v;
}

xf_value_t xf_val_undef(uint8_t type) {
    return (xf_value_t){ .state = XF_STATE_UNDEF, .type = type };
}

xf_value_t xf_val_true(void) {
    return (xf_value_t){ .state = XF_STATE_TRUE, .type = XF_TYPE_BOOL };
}

xf_value_t xf_val_false(void) {
    return (xf_value_t){ .state = XF_STATE_FALSE, .type = XF_TYPE_BOOL };
}

xf_value_t xf_val_ok_bool(bool b) {
    return b ? xf_val_true() : xf_val_false();
}



/* ============================================================
 * Atomic collapse
 * ============================================================ */

bool xf_collapse(xf_atomic_value_t *av, xf_value_t resolved) {
    uint8_t expected = XF_STATE_UNDEF;
    if (atomic_compare_exchange_strong(&av->state, &expected,
                                       resolved.state)) {
        av->type = resolved.type;
        memcpy(&av->data, &resolved.data, sizeof(av->data));
        av->err  = resolved.err;
        return true;
    }
    return false;
}

uint8_t xf_atomic_state(const xf_atomic_value_t *av) {
    return atomic_load(&av->state);
}

xf_value_t xf_snapshot(const xf_atomic_value_t *av) {
    xf_value_t v;
    v.state = atomic_load(&av->state);
    v.type  = av->type;
    memcpy(&v.data, &av->data, sizeof(v.data));
    v.err   = av->err;
    return v;
}


/* ============================================================
 * Type coercion
 * ============================================================ */

xf_value_t xf_coerce_num(xf_value_t v) {
    if (v.state == XF_STATE_TRUE)  return xf_val_ok_num(1.0);
    if (v.state == XF_STATE_FALSE) return xf_val_ok_num(0.0);
    if (v.state != XF_STATE_OK) return v;
    if (v.type == XF_TYPE_NUM)  return v;
    if (v.type == XF_TYPE_BOOL) return xf_val_ok_num(v.data.num != 0.0 ? 1.0 : 0.0);
    if (v.type == XF_TYPE_STR) {
        char *end;
        double n = strtod(v.data.str->data, &end);
        if (end != v.data.str->data && *end == '\0')
            return xf_val_ok_num(n);
    }
    return xf_val_nav(XF_TYPE_NUM);
}
xf_value_t xf_coerce_str(xf_value_t v) {
    if (v.state == XF_STATE_TRUE) {
        xf_str_t *s = xf_str_from_cstr("true");
        xf_value_t out = xf_val_ok_str(s);
        xf_str_release(s);
        return out;
    }
    if (v.state == XF_STATE_FALSE) {
        xf_str_t *s = xf_str_from_cstr("false");
        xf_value_t out = xf_val_ok_str(s);
        xf_str_release(s);
        return out;
    }
    if (v.state != XF_STATE_OK) return v;

    if (v.type == XF_TYPE_STR)
        return xf_value_retain(v);

    if (v.type == XF_TYPE_BOOL) {
        xf_str_t *s = xf_str_from_cstr(v.data.num != 0.0 ? "true" : "false");
        xf_value_t out = xf_val_ok_str(s);
        xf_str_release(s);
        return out;
    }

    char buf[64];
    if (v.type == XF_TYPE_NUM) {
        if (v.data.num == (long long)v.data.num)
            snprintf(buf, sizeof(buf), "%lld", (long long)v.data.num);
        else
            snprintf(buf, sizeof(buf), "%.14g", v.data.num);

        xf_str_t *s = xf_str_from_cstr(buf);
        xf_value_t out = xf_val_ok_str(s);
        xf_str_release(s);
        return out;
    }
if (v.type == XF_TYPE_COMPLEX) {
    char buf[96];
    double re = v.data.complex.re;
    double im = v.data.complex.im;

    if (re == 0.0) {
        snprintf(buf, sizeof(buf), "%.14gi", im);
    } else if (im < 0.0) {
        snprintf(buf, sizeof(buf), "%.14g%.14gi", re, im);
    } else {
        snprintf(buf, sizeof(buf), "%.14g+%.14gi", re, im);
    }

    xf_str_t *s = xf_str_from_cstr(buf);
    xf_value_t out = xf_val_ok_str(s);
    xf_str_release(s);
    return out;
}
    if (v.type == XF_TYPE_TUPLE) {
        xf_tuple_t *t = v.data.tuple;
        size_t cap = 64;
        size_t len = 0;
        char *outbuf = malloc(cap);
        if (!outbuf) return xf_val_nav(XF_TYPE_STR);

        outbuf[len++] = '(';

        if (t) {
            for (size_t i = 0; i < t->len; i++) {
                if (i > 0) {
                    const char *sep = ", ";
                    size_t slen = 2;
                    while (len + slen + 1 > cap) {
                        cap *= 2;
                        char *tmp = realloc(outbuf, cap);
                        if (!tmp) {
                            free(outbuf);
                            return xf_val_nav(XF_TYPE_STR);
                        }
                        outbuf = tmp;
                    }
                    memcpy(outbuf + len, sep, slen);
                    len += slen;
                }

                xf_value_t elem = xf_tuple_get(t, i);
                xf_value_t sv = xf_coerce_str(elem);
                const char *s = (sv.state == XF_STATE_OK && sv.data.str)
                              ? sv.data.str->data
                              : XF_TYPE_NAMES[elem.type];
                size_t slen = strlen(s);

                while (len + slen + 2 > cap) {
                    cap *= 2;
                    char *tmp = realloc(outbuf, cap);
                    if (!tmp) {
                        xf_value_release(sv);
                        free(outbuf);
                        return xf_val_nav(XF_TYPE_STR);
                    }
                    outbuf = tmp;
                }

                memcpy(outbuf + len, s, slen);
                len += slen;
                xf_value_release(sv);
            }

            if (t->len == 1) {
                while (len + 2 > cap) {
                    cap *= 2;
                    char *tmp = realloc(outbuf, cap);
                    if (!tmp) {
                        free(outbuf);
                        return xf_val_nav(XF_TYPE_STR);
                    }
                    outbuf = tmp;
                }
                outbuf[len++] = ',';
            }
        }

        outbuf[len++] = ')';
        outbuf[len] = '\0';

        xf_str_t *s = xf_str_new(outbuf, len);
        free(outbuf);
        if (!s) return xf_val_nav(XF_TYPE_STR);

        xf_value_t out = xf_val_ok_str(s);
        xf_str_release(s);
        return out;
    }

    xf_str_t *s = xf_str_from_cstr(XF_TYPE_NAMES[v.type]);
    xf_value_t out = xf_val_ok_str(s);
    xf_str_release(s);
    return out;
}
bool xf_can_coerce(xf_value_t v, uint8_t target_type) {
    if (v.type == target_type)  return true;
    if (target_type == XF_TYPE_STR) return true; /* everything stringifies */
    if (target_type == XF_TYPE_NUM && v.type == XF_TYPE_STR) {
        char *end;
        strtod(v.data.str->data, &end);
        return end != v.data.str->data && *end == '\0';
    }
    return false;
}


/* ============================================================
 * State propagation
 * ============================================================ */

uint8_t xf_dominant_state(xf_value_t a, xf_value_t b) {
    /* ERR > NAV > UNDEF > VOID > NULL > TRUE > FALSE > OK */
    static const uint8_t priority[XF_STATE_COUNT] = {
        [XF_STATE_OK]    = 0,
        [XF_STATE_FALSE] = 1,
        [XF_STATE_TRUE]  = 2,
        [XF_STATE_NULL]  = 3,
        [XF_STATE_VOID]  = 4,
        [XF_STATE_UNDEF] = 5,
        [XF_STATE_NAV]   = 6,
        [XF_STATE_ERR]   = 7,
    };
    return priority[a.state] >= priority[b.state] ? a.state : b.state;
}

xf_value_t xf_collect_err(xf_value_t *children, size_t n,
                           const char *src, uint32_t line) {
    /* build a single ERR value from n children, at least one of which is ERR */
    xf_err_t *root = xf_err_new("child error", src, line, 0);

    /* find first ERR child as cause */
    for (size_t i = 0; i < n; i++) {
        if (children[i].state == XF_STATE_ERR) {
            root->cause = xf_err_retain(children[i].err);
            break;
        }
    }

    /* attach all siblings */
    root->sibling_count = n;
    root->siblings = malloc(sizeof(xf_value_t *) * n);
    if (root->siblings) {
        for (size_t i = 0; i < n; i++) {
            root->siblings[i] = malloc(sizeof(xf_value_t));
            if (root->siblings[i])
                *root->siblings[i] = children[i];
        }
    }
    return xf_val_err(root, XF_TYPE_VOID);
}


/* ============================================================
 * Display
 * ============================================================ */

static void print_value_data(xf_value_t v) {
    /* boolean states print directly without needing data union */
    if (v.state == XF_STATE_TRUE)  { printf("true");  return; }
    if (v.state == XF_STATE_FALSE) { printf("false"); return; }
    if (v.state != XF_STATE_OK && v.state != XF_STATE_VOID) return;
    switch (v.type) {
    case XF_TYPE_COMPLEX: {
    double re = v.data.complex.re;
    double im = v.data.complex.im;

    if (re == 0.0) {
        printf("%.15gi", im);
    } else if (im < 0.0) {
        printf("%.15g%.15gi", re, im);
    } else {
        printf("%.15g+%.15gi", re, im);
    }
    break;
}
        case XF_TYPE_BOOL:
            printf(v.data.num != 0.0 ? "true" : "false");
            break;
        case XF_TYPE_NUM:
            if (v.data.num == (long long)v.data.num)
                printf("%lld", (long long)v.data.num);
            else
                printf("%.14g", v.data.num);
            break;
        case XF_TYPE_STR:
            printf("\"%s\"", v.data.str ? v.data.str->data : "");
            break;
        case XF_TYPE_FN:
            if (v.data.fn && v.data.fn->name)
                printf("<fn %s(%s)->%s>",
                       v.data.fn->name->data,
                       "",  /* param list — todo */
                       XF_TYPE_NAMES[v.data.fn->return_type]);
            else
                printf("<fn>");
            break;
        case XF_TYPE_REGEX:
            if (v.data.re && v.data.re->pattern)
                printf("/%s/", v.data.re->pattern->data);
            else
                printf("<regex>");
            break;
        case XF_TYPE_MAP: {
            xf_map_t *m = v.data.map;
            printf("{");
            if (m) {
                for (size_t i = 0; i < m->order_len; i++) {
                    if (i) printf(", ");
                    xf_str_t *k = m->order[i];
                    size_t slot = map_find_slot(m, k);
                    /* Print key: bare if it's a pure integer, quoted otherwise */
                    {
                        char *end;
                        strtod(k->data, &end);
                        bool is_num = (end != k->data && *end == '\0');
                        if (is_num) printf("%s: ", k->data);
                        else        printf("\"%s\": ", k->data);
                    }
                    print_value_data(m->slots[slot].val);
                }
            }
            printf("}");
            break;
        }
        case XF_TYPE_ARR: {
            xf_arr_t *a = v.data.arr;
            printf("[");
            if (a) {
                for (size_t i = 0; i < a->len; i++) {
                    if (i > 0) printf(", ");
                    print_value_data(a->items[i]);
                }
            }
            printf("]");
            break;
        }
        case XF_TYPE_SET: {
            xf_map_t *m = v.data.map;
            printf("{");
            if (m) {
                for (size_t i = 0; i < m->order_len; i++) {
                    if (i) printf(", ");
                    xf_str_t *k = m->order[i];
                    size_t slot = map_find_slot(m, k);
                    xf_value_t elem = m->slots[slot].val;
                    if (elem.state == XF_STATE_OK &&
                        (elem.type == XF_TYPE_SET || elem.type == XF_TYPE_MAP ||
                         elem.type == XF_TYPE_ARR)) {
                        print_value_data(elem);
                    } else {
                        printf("%s", k->data);
                    }
                }
            }
            printf("}");
            break;
        }
        case XF_TYPE_TUPLE: {
    xf_tuple_t *t = v.data.tuple;
    printf("(");
    if (t) {
        size_t n = xf_tuple_len(t);
        for (size_t i = 0; i < n; i++) {
            if (i > 0) printf(", ");
            print_value_data(xf_tuple_get(t, i));
        }
        if (n == 1) printf(",");
    }
    printf(")");
    break;
}
        case XF_TYPE_MODULE:
            if (v.data.mod && v.data.mod->name)
                printf("<module %s>", v.data.mod->name);
            else
                printf("<module>");
            break;
        default:            printf("<void>");  break;
    }
}

void xf_value_print(xf_value_t v) {
    print_value_data(v);
}

void xf_value_repl_print(xf_value_t v) {
    /* "=> true  [bool, TRUE]" */
    printf("=> ");
    print_value_data(v);
    uint8_t type = (XF_STATE_IS_BOOL(v.state)) ? XF_TYPE_BOOL : v.type;
    printf("  [%s, %s]\r\n",
           XF_TYPE_NAMES[type < XF_TYPE_COUNT ? type : 0],
           XF_STATE_NAMES[v.state < XF_STATE_COUNT ? v.state : 0]);
}

void xf_err_print(xf_err_t *e) {
    if (!e) return;
    printf("ERR ");
    printf("──────────────────────────────────────────────\r\n");
    if (e->message)
        printf("  reason:   %s\n", e->message->data);
    if (e->source)
        printf("  at:       %s:%u:%u\n", e->source->data, e->line, e->col);
    if (e->cause) {
        printf("  caused by:\r\n");
        if (e->cause->message)
            printf("    %s\n", e->cause->message->data);
    }
    if (e->sibling_count) {
        printf("  children:\r\n");
        for (size_t i = 0; i < e->sibling_count; i++) {
            xf_value_t *c = e->siblings[i];
            if (!c) continue;
            printf("    [%zu] ", i);
            print_value_data(*c);
            printf("  [%s, %s]\n",
                   XF_TYPE_NAMES[c->type],
                   XF_STATE_NAMES[c->state]);
        }
    }
    printf("──────────────────────────────────────────────\r\n");
}

/* ============================================================
 * Module implementation
 * ============================================================ */

#define MODULE_INIT_CAP 16

xf_module_t *xf_module_new(const char *name) {
    xf_module_t *m = calloc(1, sizeof(xf_module_t));
    atomic_store(&m->refcount, 1);
    m->name    = name;   /* static string — not owned */
    m->cap     = MODULE_INIT_CAP;
    m->entries = calloc(m->cap, sizeof(xf_module_entry_t));
    return m;
}

xf_module_t *xf_module_retain(xf_module_t *m) {
    if (m) atomic_fetch_add(&m->refcount, 1);
    return m;
}

void xf_module_release(xf_module_t *m) {
    if (!m) return;
    if (atomic_fetch_sub(&m->refcount, 1) != 1) return;
    for (size_t i = 0; i < m->count; i++)
        xf_value_release(m->entries[i].val);
    free(m->entries);
    free(m);
}

void xf_module_set(xf_module_t *m, const char *name, xf_value_t val) {
    for (size_t i = 0; i < m->count; i++) {
        if (strcmp(m->entries[i].name, name) == 0) {
            xf_value_release(m->entries[i].val);
            m->entries[i].val = val;
            return;
        }
    }

    if (m->count >= m->cap) {
        m->cap *= 2;
        m->entries = realloc(m->entries, m->cap * sizeof(xf_module_entry_t));
    }

    m->entries[m->count].name = name;
    m->entries[m->count].val  = val;
    m->count++;
}
xf_value_t xf_module_get(const xf_module_t *m, const char *name) {
    if (!m) return xf_val_nav(XF_TYPE_VOID);
    for (size_t i = 0; i < m->count; i++) {
        if (strcmp(m->entries[i].name, name) == 0)
            return xf_value_retain(m->entries[i].val);
    }
    return xf_val_nav(XF_TYPE_VOID);
}