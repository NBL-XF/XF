/* src/core/byte.c */

#include "internal.h"
#include "bl.h"

static pthread_mutex_t cb_eval_mu = PTHREAD_MUTEX_INITIALIZER;

static xf_Value cb_error(const char *msg, uint8_t type) {
    xf_err_t *e = xf_err_new(msg ? msg : "byte error", "<core.byte>", 0, 0);
    if (!e) return xf_val_nav(type);

    xf_Value out = xf_val_err(e, type);
    xf_err_release(e);
    return out;
}

static void cb_map_set(xf_map_t *m, const char *key, xf_Value value) {
    xf_Str *ks = xf_str_from_cstr(key);
    if (ks) {
        xf_map_set(m, ks, value);
        xf_str_release(ks);
    }
    xf_value_release(value);
}

static const char *cb_kind_name(BLValueKind kind) {
    switch (kind) {
        case BL_VALUE_NULL:   return "null";
        case BL_VALUE_INT:    return "int";
        case BL_VALUE_STRING: return "string";
        case BL_VALUE_BUFFER: return "buffer";
        case BL_VALUE_ARRAY:  return "array";
        default:              return "unknown";
    }
}

static xf_Value cb_result_map(int ok, const BLValueView *result, const char *error_msg) {
    xf_map_t *m = xf_map_new();
    if (!m) return xf_val_nav(XF_TYPE_MAP);

    cb_map_set(m, "ok", xf_val_ok_bool(ok != 0));
    cb_map_set(m, "kind", make_str_val(
        (ok && result) ? cb_kind_name(result->kind) : "",
        (ok && result) ? strlen(cb_kind_name(result->kind)) : 0
    ));

    cb_map_set(m, "int", xf_val_ok_num(
        (ok && result && result->kind == BL_VALUE_INT) ? (double)result->as.int_value : 0.0
    ));

    cb_map_set(m, "string", make_str_val(
        (ok && result && result->kind == BL_VALUE_STRING && result->as.string_value)
            ? result->as.string_value
            : "",
        (ok && result && result->kind == BL_VALUE_STRING && result->as.string_value)
            ? strlen(result->as.string_value)
            : 0
    ));

    cb_map_set(m, "size", xf_val_ok_num(
        (ok && result && result->kind == BL_VALUE_BUFFER)
            ? (double)result->as.ptr_value.size
            : (ok && result && result->kind == BL_VALUE_ARRAY)
                ? (double)result->as.array_value.size
                : 0.0
    ));

    cb_map_set(m, "owned", xf_val_ok_bool(
        (ok && result && result->kind == BL_VALUE_BUFFER)
            ? (result->as.ptr_value.owned != 0)
            : (ok && result && result->kind == BL_VALUE_ARRAY)
                ? (result->as.array_value.owned != 0)
                : false
    ));

    cb_map_set(m, "error", make_str_val(
        (!ok && error_msg) ? error_msg : "",
        (!ok && error_msg) ? strlen(error_msg) : 0
    ));

    xf_Value out = xf_val_ok_map(m);
    xf_map_release(m);
    return out;
}

static int cb_check_source_locked(const char *virtual_path,
                                  const char *source,
                                  char *errbuf,
                                  size_t errcap) {
    int ok = 0;
    BLInterpreter *interp = NULL;

    if (errbuf && errcap > 0) errbuf[0] = '\0';

    pthread_mutex_lock(&cb_eval_mu);

    interp = bl_interpreter_create();
    if (!interp) {
        if (errbuf && errcap > 0) snprintf(errbuf, errcap, "out of memory");
        goto done;
    }

    ok = (bl_check_source(interp, virtual_path, source) == BL_STATUS_OK);
    if (!ok && errbuf && errcap > 0) {
        snprintf(errbuf, errcap, "%s", bl_last_error(interp));
    }

done:
    bl_interpreter_destroy(interp);
    pthread_mutex_unlock(&cb_eval_mu);
    return ok;
}

static int cb_check_file_locked(const char *path, char *errbuf, size_t errcap) {
    int ok = 0;
    BLInterpreter *interp = NULL;

    if (errbuf && errcap > 0) errbuf[0] = '\0';

    pthread_mutex_lock(&cb_eval_mu);

    interp = bl_interpreter_create();
    if (!interp) {
        if (errbuf && errcap > 0) snprintf(errbuf, errcap, "out of memory");
        goto done;
    }

    ok = (bl_check_file(interp, path) == BL_STATUS_OK);
    if (!ok && errbuf && errcap > 0) {
        snprintf(errbuf, errcap, "%s", bl_last_error(interp));
    }

done:
    bl_interpreter_destroy(interp);
    pthread_mutex_unlock(&cb_eval_mu);
    return ok;
}

static int cb_run_source_locked(const char *virtual_path,
                                const char *source,
                                BLValueView *result,
                                char *errbuf,
                                size_t errcap) {
    int ok = 0;
    BLInterpreter *interp = NULL;

    if (errbuf && errcap > 0) errbuf[0] = '\0';
    if (result) memset(result, 0, sizeof(*result));

    pthread_mutex_lock(&cb_eval_mu);

    interp = bl_interpreter_create();
    if (!interp) {
        if (errbuf && errcap > 0) snprintf(errbuf, errcap, "out of memory");
        goto done;
    }

    ok = (bl_run_source(interp, virtual_path, source, result) == BL_STATUS_OK);
    if (!ok && errbuf && errcap > 0) {
        snprintf(errbuf, errcap, "%s", bl_last_error(interp));
    }

done:
    bl_interpreter_destroy(interp);
    pthread_mutex_unlock(&cb_eval_mu);
    return ok;
}

static int cb_run_file_locked(const char *path,
                              BLValueView *result,
                              char *errbuf,
                              size_t errcap) {
    int ok = 0;
    BLInterpreter *interp = NULL;

    if (errbuf && errcap > 0) errbuf[0] = '\0';
    if (result) memset(result, 0, sizeof(*result));

    pthread_mutex_lock(&cb_eval_mu);

    interp = bl_interpreter_create();
    if (!interp) {
        if (errbuf && errcap > 0) snprintf(errbuf, errcap, "out of memory");
        goto done;
    }

    ok = (bl_run_file(interp, path, result) == BL_STATUS_OK);
    if (!ok && errbuf && errcap > 0) {
        snprintf(errbuf, errcap, "%s", bl_last_error(interp));
    }

done:
    bl_interpreter_destroy(interp);
    pthread_mutex_unlock(&cb_eval_mu);
    return ok;
}

/* core.byte.check(source) -> bool */
static xf_Value cb_check(xf_Value *args, size_t argc) {
    NEED(1);

    const char *source = NULL;
    size_t source_len = 0;
    char errbuf[512];

    if (!arg_str(args, argc, 0, &source, &source_len)) return propagate(args, argc);
    (void)source_len;

    if (!cb_check_source_locked("<core.byte.eval>", source, errbuf, sizeof(errbuf))) {
        return cb_error(errbuf, XF_TYPE_BOOL);
    }

    return xf_val_ok_bool(true);
}

/* core.byte.check_file(path) -> bool */
static xf_Value cb_check_file(xf_Value *args, size_t argc) {
    NEED(1);

    const char *path = NULL;
    size_t path_len = 0;
    char errbuf[512];

    if (!arg_str(args, argc, 0, &path, &path_len)) return propagate(args, argc);
    (void)path_len;

    if (!cb_check_file_locked(path, errbuf, sizeof(errbuf))) {
        return cb_error(errbuf, XF_TYPE_BOOL);
    }

    return xf_val_ok_bool(true);
}

/* core.byte.eval(source) -> map */
static xf_Value cb_eval(xf_Value *args, size_t argc) {
    NEED(1);

    const char *source = NULL;
    size_t source_len = 0;
    BLValueView result;
    char errbuf[512];

    if (!arg_str(args, argc, 0, &source, &source_len)) return propagate(args, argc);
    (void)source_len;

    if (!cb_run_source_locked("<core.byte.eval>", source, &result, errbuf, sizeof(errbuf))) {
        return cb_result_map(0, NULL, errbuf);
    }

    return cb_result_map(1, &result, NULL);
}

/* core.byte.file(path) -> map */
static xf_Value cb_file(xf_Value *args, size_t argc) {
    NEED(1);

    const char *path = NULL;
    size_t path_len = 0;
    BLValueView result;
    char errbuf[512];

    if (!arg_str(args, argc, 0, &path, &path_len)) return propagate(args, argc);
    (void)path_len;

    if (!cb_run_file_locked(path, &result, errbuf, sizeof(errbuf))) {
        return cb_result_map(0, NULL, errbuf);
    }

    return cb_result_map(1, &result, NULL);
}

/* core.byte.run(source) -> map */
static xf_Value cb_run(xf_Value *args, size_t argc) {
    return cb_eval(args, argc);
}

/* core.byte.run_file(path) -> map */
static xf_Value cb_run_file(xf_Value *args, size_t argc) {
    return cb_file(args, argc);
}

xf_module_t *build_byte(void) {
    xf_module_t *m = xf_module_new("core.byte");
    FN("check", XF_TYPE_BOOL, cb_check);
    FN("check_file", XF_TYPE_BOOL, cb_check_file);
    FN("eval", XF_TYPE_MAP, cb_eval);
    FN("file", XF_TYPE_MAP, cb_file);
    FN("run", XF_TYPE_MAP, cb_run);
    FN("run_file", XF_TYPE_MAP, cb_run_file);
    return m;
}


/* src/core/internal.h */
/* add this forward declaration near build_lambda() */

xf_module_t *build_byte(void);

