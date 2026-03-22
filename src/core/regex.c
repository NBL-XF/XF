#include "internal.h"

/* ── shared helpers (used by str.c, generics.c, edit.c) ──────── */

int cr_parse_flags(xf_Value *args, size_t argc, size_t flag_idx) {
    int cflags = REG_EXTENDED;
    if (flag_idx < argc && args[flag_idx].state == XF_STATE_OK &&
        args[flag_idx].type == XF_TYPE_STR && args[flag_idx].data.str) {
        const char *fs = args[flag_idx].data.str->data;
        for (; *fs; fs++) {
            if (*fs == 'i' || *fs == 'I') cflags |= REG_ICASE;
            if (*fs == 'm' || *fs == 'M') cflags |= REG_NEWLINE;
        }
    }
    return cflags;
}

bool cr_compile(const char *pat, int cflags,
                regex_t *out, char *errmsg, size_t errmsg_len) {
    int rc = regcomp(out, pat, cflags);
    if (rc != 0) {
        regerror(rc, out, errmsg, errmsg_len);
        regfree(out);
        return false;
    }
    return true;
}

xf_Str *cr_apply_replacement(const char *subject, const regmatch_t *pm,
                               size_t ngroups, const char *repl) {
    char buf[8192];
    size_t out = 0;
    for (const char *r = repl; *r && out < sizeof(buf) - 1; r++) {
        if (*r == '\\' && *(r+1) >= '1' && *(r+1) <= '9') {
            size_t g = (size_t)(*(r+1) - '0');
            r++;
            if (g < ngroups && pm[g].rm_so >= 0) {
                size_t glen = (size_t)(pm[g].rm_eo - pm[g].rm_so);
                if (out + glen < sizeof(buf) - 1) {
                    memcpy(buf + out, subject + pm[g].rm_so, glen);
                    out += glen;
                }
            }
        } else {
            buf[out++] = *r;
        }
    }
    buf[out] = '\0';
    return xf_str_new(buf, out);
}

/* ── internal result builder ─────────────────────────────────── */

static xf_Value cr_build_match_map(const char *subject, regmatch_t *pm,
                                    size_t ngroups) {
    xf_map_t *m = xf_map_new();

    xf_Str *mkey   = xf_str_from_cstr("match");
    xf_Str *mval_s = xf_str_new(subject + pm[0].rm_so,
                                  (size_t)(pm[0].rm_eo - pm[0].rm_so));
    xf_map_set(m, mkey, xf_val_ok_str(mval_s));
    xf_str_release(mkey);
    xf_str_release(mval_s);

    xf_Str *ikey = xf_str_from_cstr("index");
    xf_map_set(m, ikey, xf_val_ok_num((double)pm[0].rm_so));
    xf_str_release(ikey);

    xf_arr_t *grp_arr = xf_arr_new();
    for (size_t g = 1; g < ngroups; g++) {
        if (pm[g].rm_so >= 0) {
            xf_Str *gs = xf_str_new(subject + pm[g].rm_so,
                                     (size_t)(pm[g].rm_eo - pm[g].rm_so));
            xf_arr_push(grp_arr, xf_val_ok_str(gs));
            xf_str_release(gs);
        } else {
            xf_Str *empty = xf_str_from_cstr("");
            xf_arr_push(grp_arr, xf_val_ok_str(empty));
            xf_str_release(empty);
        }
    }
    xf_Str *gkey = xf_str_from_cstr("groups");
    xf_map_set(m, gkey, xf_val_ok_arr(grp_arr));
    xf_str_release(gkey);
    xf_arr_release(grp_arr);

    xf_Value tmp = xf_val_ok_map(m);
    xf_map_release(m);
    return tmp;
}

/* ── cr_match ────────────────────────────────────────────────── */

static xf_Value cr_match(xf_Value *args, size_t argc) {
    NEED(2);
    xf_Value sv = xf_coerce_str(args[0]);
    xf_Value pv = xf_coerce_str(args[1]);
    if (sv.state != XF_STATE_OK || pv.state != XF_STATE_OK)
        return xf_val_nav(XF_TYPE_MAP);

    int cflags = cr_parse_flags(args, argc, 2);
    regex_t re; char errmsg[256];
    if (!cr_compile(pv.data.str->data, cflags, &re, errmsg, sizeof(errmsg)))
        return xf_val_nav(XF_TYPE_MAP);

    size_t ngroups = re.re_nsub + 1;
    if (ngroups > CR_MAX_GROUPS) ngroups = CR_MAX_GROUPS;
    regmatch_t pm[CR_MAX_GROUPS];

    const char *subject = sv.data.str->data;
    int rc = regexec(&re, subject, ngroups, pm, 0);
    regfree(&re);

    if (rc != 0) return xf_val_nav(XF_TYPE_MAP);
    return cr_build_match_map(subject, pm, ngroups);
}

/* ── cr_search ───────────────────────────────────────────────── */

static xf_Value cr_search(xf_Value *args, size_t argc) {
    NEED(2);
    xf_Value sv = xf_coerce_str(args[0]);
    xf_Value pv = xf_coerce_str(args[1]);
    if (sv.state != XF_STATE_OK || pv.state != XF_STATE_OK)
        return xf_val_nav(XF_TYPE_ARR);

    int cflags = cr_parse_flags(args, argc, 2);
    regex_t re; char errmsg[256];
    if (!cr_compile(pv.data.str->data, cflags, &re, errmsg, sizeof(errmsg)))
        return xf_val_nav(XF_TYPE_ARR);

    size_t ngroups = re.re_nsub + 1;
    if (ngroups > CR_MAX_GROUPS) ngroups = CR_MAX_GROUPS;
    regmatch_t pm[CR_MAX_GROUPS];

    xf_arr_t   *results     = xf_arr_new();
    const char *cursor      = sv.data.str->data;
    size_t      base_offset = 0;

    while (*cursor) {
        int rc = regexec(&re, cursor, ngroups, pm, base_offset ? REG_NOTBOL : 0);
        if (rc != 0) break;

        regmatch_t abs_pm[CR_MAX_GROUPS];
        for (size_t g = 0; g < ngroups; g++) {
            abs_pm[g] = pm[g];
            if (pm[g].rm_so >= 0) {
                abs_pm[g].rm_so += (regoff_t)base_offset;
                abs_pm[g].rm_eo += (regoff_t)base_offset;
            }
        }

        xf_Value mv = cr_build_match_map(sv.data.str->data, abs_pm, ngroups);
        xf_arr_push(results, mv);

        size_t adv = (pm[0].rm_eo > pm[0].rm_so) ? (size_t)pm[0].rm_eo : 1;
        base_offset += adv;
        cursor      += adv;
    }

    regfree(&re);
    xf_Value rv = xf_val_ok_arr(results);
    xf_arr_release(results);
    return rv;
}

/* ── cr_replace_impl ─────────────────────────────────────────── */

static xf_Value cr_replace_impl(xf_Value *args, size_t argc, bool global) {
    NEED(3);
    xf_Value sv   = xf_coerce_str(args[0]);
    xf_Value pv   = xf_coerce_str(args[1]);
    xf_Value repv = xf_coerce_str(args[2]);
    if (sv.state != XF_STATE_OK || pv.state != XF_STATE_OK || repv.state != XF_STATE_OK)
        return xf_val_nav(XF_TYPE_STR);

    int cflags = cr_parse_flags(args, argc, 3);
    regex_t re; char errmsg[256];
    if (!cr_compile(pv.data.str->data, cflags, &re, errmsg, sizeof(errmsg)))
        return xf_val_nav(XF_TYPE_STR);

    size_t ngroups = re.re_nsub + 1;
    if (ngroups > CR_MAX_GROUPS) ngroups = CR_MAX_GROUPS;
    regmatch_t pm[CR_MAX_GROUPS];

    const char *subject = sv.data.str->data;
    const char *repl    = repv.data.str->data;

    size_t cap = strlen(subject) * 2 + 256;
    char  *out = malloc(cap);
    size_t used = 0;

#define ENSURE(n) \
    do { if (used + (n) + 1 >= cap) { cap = cap*2+(n)+1; out = realloc(out, cap); } } while(0)

    const char *cursor = subject;
    int eflags = 0;
    while (*cursor) {
        int rc = regexec(&re, cursor, ngroups, pm, eflags);
        if (rc != 0) break;

        size_t pre = (size_t)pm[0].rm_so;
        ENSURE(pre);
        memcpy(out + used, cursor, pre);
        used += pre;

        xf_Str *rep_str = cr_apply_replacement(cursor, pm, ngroups, repl);
        ENSURE(rep_str->len);
        memcpy(out + used, rep_str->data, rep_str->len);
        used += rep_str->len;
        xf_str_release(rep_str);

        size_t adv = (pm[0].rm_eo > pm[0].rm_so) ? (size_t)pm[0].rm_eo : 1;
        if (adv == 0) {
            ENSURE(1);
            out[used++] = *cursor;
            adv = 1;
        }
        cursor += adv;
        eflags  = REG_NOTBOL;
        if (!global) break;
    }
#undef ENSURE

    size_t tail = strlen(cursor);
    if (used + tail + 1 >= cap) { cap = used + tail + 2; out = realloc(out, cap); }
    memcpy(out + used, cursor, tail);
    used += tail;
    out[used] = '\0';

    regfree(&re);
    xf_Str *result = xf_str_new(out, used);
    free(out);
    xf_Value rv = xf_val_ok_str(result);
    xf_str_release(result);
    return rv;
}

static xf_Value cr_replace(xf_Value *args, size_t argc) {
    return cr_replace_impl(args, argc, false);
}
static xf_Value cr_replace_all(xf_Value *args, size_t argc) {
    return cr_replace_impl(args, argc, true);
}

/* ── cr_groups ───────────────────────────────────────────────── */

static xf_Value cr_groups(xf_Value *args, size_t argc) {
    NEED(2);
    xf_Value sv = xf_coerce_str(args[0]);
    xf_Value pv = xf_coerce_str(args[1]);
    if (sv.state != XF_STATE_OK || pv.state != XF_STATE_OK)
        return xf_val_nav(XF_TYPE_ARR);

    int cflags = cr_parse_flags(args, argc, 2);
    regex_t re; char errmsg[256];
    if (!cr_compile(pv.data.str->data, cflags, &re, errmsg, sizeof(errmsg)))
        return xf_val_nav(XF_TYPE_ARR);

    size_t ngroups = re.re_nsub + 1;
    if (ngroups > CR_MAX_GROUPS) ngroups = CR_MAX_GROUPS;
    regmatch_t pm[CR_MAX_GROUPS];

    const char *subject = sv.data.str->data;
    int rc = regexec(&re, subject, ngroups, pm, 0);
    regfree(&re);
    if (rc != 0) return xf_val_nav(XF_TYPE_ARR);

    xf_arr_t *arr = xf_arr_new();
    for (size_t g = 1; g < ngroups; g++) {
        if (pm[g].rm_so >= 0) {
            xf_Str *gs = xf_str_new(subject + pm[g].rm_so,
                                     (size_t)(pm[g].rm_eo - pm[g].rm_so));
            xf_arr_push(arr, xf_val_ok_str(gs));
            xf_str_release(gs);
        } else {
            xf_Str *empty = xf_str_from_cstr("");
            xf_arr_push(arr, xf_val_ok_str(empty));
            xf_str_release(empty);
        }
    }
    xf_Value rv = xf_val_ok_arr(arr);
    xf_arr_release(arr);
    return rv;
}

/* ── cr_test ─────────────────────────────────────────────────── */

static xf_Value cr_test(xf_Value *args, size_t argc) {
    NEED(2);
    xf_Value sv = xf_coerce_str(args[0]);
    xf_Value pv = xf_coerce_str(args[1]);
    if (sv.state != XF_STATE_OK || pv.state != XF_STATE_OK)
        return xf_val_ok_num(0);

    int cflags = cr_parse_flags(args, argc, 2);
    regex_t re; char errmsg[256];
    if (!cr_compile(pv.data.str->data, cflags, &re, errmsg, sizeof(errmsg)))
        return xf_val_ok_num(0);

    int rc = regexec(&re, sv.data.str->data, 0, NULL, 0);
    regfree(&re);
    return xf_val_ok_num(rc == 0 ? 1.0 : 0.0);
}

/* ── cr_split ────────────────────────────────────────────────── */

static xf_Value cr_split(xf_Value *args, size_t argc) {
    NEED(2);
    xf_Value sv = xf_coerce_str(args[0]);
    xf_Value pv = xf_coerce_str(args[1]);
    if (sv.state != XF_STATE_OK || pv.state != XF_STATE_OK)
        return xf_val_nav(XF_TYPE_ARR);

    int cflags = cr_parse_flags(args, argc, 2);
    regex_t re; char errmsg[256];
    if (!cr_compile(pv.data.str->data, cflags, &re, errmsg, sizeof(errmsg)))
        return xf_val_nav(XF_TYPE_ARR);

    regmatch_t  pm[1];
    xf_arr_t   *arr    = xf_arr_new();
    const char *cursor = sv.data.str->data;

    while (*cursor) {
        int rc = regexec(&re, cursor, 1, pm, 0);
        if (rc != 0) break;
        xf_Str *seg = xf_str_new(cursor, (size_t)pm[0].rm_so);
        xf_arr_push(arr, xf_val_ok_str(seg));
        xf_str_release(seg);
        size_t adv = (pm[0].rm_eo > pm[0].rm_so) ? (size_t)pm[0].rm_eo : 1;
        cursor += adv;
    }
    xf_Str *tail = xf_str_from_cstr(cursor);
    xf_arr_push(arr, xf_val_ok_str(tail));
    xf_str_release(tail);

    regfree(&re);
    xf_Value rv = xf_val_ok_arr(arr);
    xf_arr_release(arr);
    return rv;
}

/* ── build ───────────────────────────────────────────────────── */

xf_module_t *build_regex(void) {
    xf_module_t *m = xf_module_new("core.regex");
    FN("match",       XF_TYPE_MAP, cr_match);
    FN("search",      XF_TYPE_ARR, cr_search);
    FN("replace",     XF_TYPE_STR, cr_replace);
    FN("replace_all", XF_TYPE_STR, cr_replace_all);
    FN("groups",      XF_TYPE_ARR, cr_groups);
    FN("test",        XF_TYPE_NUM, cr_test);
    FN("split",       XF_TYPE_ARR, cr_split);
    return m;
}