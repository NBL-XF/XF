#include "internal.h"

/* ── file handle table ────────────────────────────────────────── */

#define COS_MAX_HANDLES 64

typedef struct {
    FILE  *fp;
    bool   open;
    size_t lines_read;
} CosHandle;

static CosHandle       cos_handles[COS_MAX_HANDLES];
static pthread_mutex_t cos_handle_mu   = PTHREAD_MUTEX_INITIALIZER;
static bool            cos_handles_inited = false;

static void cos_init_handles(void) {
    if (cos_handles_inited) return;
    memset(cos_handles, 0, sizeof(cos_handles));
    cos_handles_inited = true;
}

static xf_Value csy_open(xf_Value *args, size_t argc) {
    NEED(1);
    const char *path; size_t plen;
    if (!arg_str(args, argc, 0, &path, &plen)) return propagate(args, argc);

    pthread_mutex_lock(&cos_handle_mu);
    cos_init_handles();
    int slot = -1;
    for (int i = 0; i < COS_MAX_HANDLES; i++)
        if (!cos_handles[i].open) { slot = i; break; }
    if (slot < 0) { pthread_mutex_unlock(&cos_handle_mu); return xf_val_nav(XF_TYPE_NUM); }
    FILE *fp = fopen(path, "r");
    if (!fp)  { pthread_mutex_unlock(&cos_handle_mu); return xf_val_nav(XF_TYPE_NUM); }
    cos_handles[slot].fp         = fp;
    cos_handles[slot].open       = true;
    cos_handles[slot].lines_read = 0;
    pthread_mutex_unlock(&cos_handle_mu);
    return xf_val_ok_num((double)slot);
}

static xf_Value csy_chunk(xf_Value *args, size_t argc) {
    NEED(2);
    double dh, dn;
    if (!arg_num(args, argc, 0, &dh)) return propagate(args, argc);
    if (!arg_num(args, argc, 1, &dn)) return propagate(args, argc);
    int slot = (int)dh;
    if (slot < 0 || slot >= COS_MAX_HANDLES) return xf_val_nav(XF_TYPE_ARR);

    pthread_mutex_lock(&cos_handle_mu);
    if (!cos_handles[slot].open || !cos_handles[slot].fp) {
        pthread_mutex_unlock(&cos_handle_mu);
        xf_arr_t *empty = xf_arr_new();
        xf_Value v = xf_val_ok_arr(empty); xf_arr_release(empty); return v;
    }
    FILE *fp = cos_handles[slot].fp;
    pthread_mutex_unlock(&cos_handle_mu);

    size_t n       = (size_t)(dn < 1 ? 1 : dn);
    size_t cap     = 65536;
    char  *line_buf = malloc(cap);
    xf_arr_t *out  = xf_arr_new();
    size_t count   = 0;

    while (count < n) {
        size_t pos = 0; int c; bool got = false;
        while ((c = fgetc(fp)) != EOF) {
            got = true;
            if (pos + 2 >= cap) { cap *= 2; line_buf = realloc(line_buf, cap); }
            if (c == '\n') break;
            if (c != '\r') line_buf[pos++] = (char)c;
        }
        if (!got) break;
        line_buf[pos] = '\0';
        xf_Str *ls = xf_str_new(line_buf, pos);
        xf_arr_push(out, xf_val_ok_str(ls));
        xf_str_release(ls);
        count++;
    }
    free(line_buf);

    pthread_mutex_lock(&cos_handle_mu);
    cos_handles[slot].lines_read += count;
    pthread_mutex_unlock(&cos_handle_mu);

    xf_Value v = xf_val_ok_arr(out); xf_arr_release(out); return v;
}

static xf_Value csy_tell(xf_Value *args, size_t argc) {
    NEED(1);
    double dh;
    if (!arg_num(args, argc, 0, &dh)) return propagate(args, argc);
    int slot = (int)dh;
    if (slot < 0 || slot >= COS_MAX_HANDLES) return xf_val_nav(XF_TYPE_NUM);
    pthread_mutex_lock(&cos_handle_mu);
    double nr = cos_handles[slot].open ? (double)cos_handles[slot].lines_read : -1.0;
    pthread_mutex_unlock(&cos_handle_mu);
    return xf_val_ok_num(nr);
}

static xf_Value csy_close(xf_Value *args, size_t argc) {
    NEED(1);
    double dh;
    if (!arg_num(args, argc, 0, &dh)) return propagate(args, argc);
    int slot = (int)dh;
    if (slot < 0 || slot >= COS_MAX_HANDLES) return xf_val_null();
    pthread_mutex_lock(&cos_handle_mu);
    if (cos_handles[slot].open && cos_handles[slot].fp) {
        fclose(cos_handles[slot].fp);
        cos_handles[slot].fp   = NULL;
        cos_handles[slot].open = false;
    }
    pthread_mutex_unlock(&cos_handle_mu);
    return xf_val_null();
}

/* ── file helpers ─────────────────────────────────────────────── */

static xf_Value csy_read(xf_Value *args, size_t argc) {
    NEED(1);
    const char *path; size_t plen;
    if (!arg_str(args, argc, 0, &path, &plen)) return propagate(args, argc);
    FILE *fp = fopen(path, "r");
    if (!fp) return xf_val_nav(XF_TYPE_STR);
    char buf[65536]; size_t n = 0; int c;
    while (n < sizeof(buf)-1 && (c = fgetc(fp)) != EOF) buf[n++] = (char)c;
    buf[n] = '\0'; fclose(fp);
    return make_str_val(buf, n);
}

static xf_Value csy_write(xf_Value *args, size_t argc) {
    NEED(2);
    const char *path; size_t plen; const char *data; size_t dlen;
    if (!arg_str(args, argc, 0, &path, &plen)) return propagate(args, argc);
    if (!arg_str(args, argc, 1, &data, &dlen)) return propagate(args, argc);
    FILE *fp = fopen(path, "w");
    if (!fp) return xf_val_ok_num(0);
    fwrite(data, 1, dlen, fp); fclose(fp);
    return xf_val_ok_num(1);
}

static xf_Value csy_append(xf_Value *args, size_t argc) {
    NEED(2);
    const char *path; size_t plen; const char *data; size_t dlen;
    if (!arg_str(args, argc, 0, &path, &plen)) return propagate(args, argc);
    if (!arg_str(args, argc, 1, &data, &dlen)) return propagate(args, argc);
    FILE *fp = fopen(path, "a");
    if (!fp) return xf_val_ok_num(0);
    fwrite(data, 1, dlen, fp); fclose(fp);
    return xf_val_ok_num(1);
}

static xf_Value csy_lines(xf_Value *args, size_t argc) {
    NEED(1);
    const char *path; size_t plen;
    if (!arg_str(args, argc, 0, &path, &plen)) return propagate(args, argc);
    FILE *fp = fopen(path, "r");
    if (!fp) return xf_val_nav(XF_TYPE_ARR);
    xf_arr_t *a = xf_arr_new();
    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        size_t ln = strlen(line);
        while (ln > 0 && (line[ln-1] == '\n' || line[ln-1] == '\r')) line[--ln] = '\0';
        xf_Str *ls = xf_str_new(line, ln);
        xf_arr_push(a, xf_val_ok_str(ls)); xf_str_release(ls);
    }
    fclose(fp);
    xf_Value r = xf_val_ok_arr(a); xf_arr_release(a); return r;
}

/* ── shell helpers ────────────────────────────────────────────── */

static xf_Value csy_execute(xf_Value *args, size_t argc) {
    NEED(1);
    const char *cmd; size_t cmdlen;
    if (!arg_str(args, argc, 0, &cmd, &cmdlen)) return propagate(args, argc);
    return xf_val_ok_num((double)system(cmd));
}

static xf_Value csy_run(xf_Value *args, size_t argc) {
    NEED(1);
    const char *cmd; size_t cmdlen;
    if (!arg_str(args, argc, 0, &cmd, &cmdlen)) return propagate(args, argc);
    FILE *fp = popen(cmd, "r");
    if (!fp) return xf_val_nav(XF_TYPE_STR);
    char buf[65536]; size_t n = 0; int c;
    while (n < sizeof(buf)-1 && (c = fgetc(fp)) != EOF) buf[n++] = (char)c;
    buf[n] = '\0'; pclose(fp);
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';
    return make_str_val(buf, n);
}

static xf_Value csy_run_lines(xf_Value *args, size_t argc) {
    NEED(1);
    const char *cmd; size_t cmdlen;
    if (!arg_str(args, argc, 0, &cmd, &cmdlen)) return propagate(args, argc);
    FILE *fp = popen(cmd, "r");
    if (!fp) return xf_val_nav(XF_TYPE_ARR);
    xf_arr_t *a = xf_arr_new(); char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        size_t ln = strlen(line);
        while (ln > 0 && (line[ln-1] == '\n' || line[ln-1] == '\r')) line[--ln] = '\0';
        xf_Str *ls = xf_str_new(line, ln);
        xf_arr_push(a, xf_val_ok_str(ls)); xf_str_release(ls);
    }
    pclose(fp);
    xf_Value r = xf_val_ok_arr(a); xf_arr_release(a); return r;
}

static xf_Value csy_exit(xf_Value *args, size_t argc) {
    double code = 0; arg_num(args, argc, 0, &code);
    exit((int)code);
}

static xf_Value csy_time(xf_Value *args, size_t argc) {
    (void)args; (void)argc;
    return xf_val_ok_num((double)time(NULL));
}

static xf_Value csy_env(xf_Value *args, size_t argc) {
    NEED(1);
    const char *name; size_t namelen;
    if (!arg_str(args, argc, 0, &name, &namelen)) return propagate(args, argc);
    const char *val = getenv(name);
    if (!val) return xf_val_nav(XF_TYPE_STR);
    return make_str_val(val, strlen(val));
}

static xf_Value csy_free(xf_Value *args, size_t argc) {
    (void)args; (void)argc;
    return xf_val_null();
}

xf_module_t *build_os(void) {
    xf_module_t *m = xf_module_new("core.os");
    FN("execute",   XF_TYPE_NUM,  csy_execute);
    FN("exec",      XF_TYPE_NUM,  csy_execute);
    FN("exit",      XF_TYPE_VOID, csy_exit);
    FN("time",      XF_TYPE_NUM,  csy_time);
    FN("env",       XF_TYPE_STR,  csy_env);
    FN("read",      XF_TYPE_STR,  csy_read);
    FN("write",     XF_TYPE_NUM,  csy_write);
    FN("open",      XF_TYPE_NUM,  csy_open);
    FN("chunk",     XF_TYPE_ARR,  csy_chunk);
    FN("tell",      XF_TYPE_NUM,  csy_tell);
    FN("close",     XF_TYPE_VOID, csy_close);
    FN("append",    XF_TYPE_NUM,  csy_append);
    FN("lines",     XF_TYPE_ARR,  csy_lines);
    FN("run",       XF_TYPE_STR,  csy_run);
    FN("run_lines", XF_TYPE_ARR,  csy_run_lines);
    FN("free",      XF_TYPE_VOID, csy_free);
    return m;
}