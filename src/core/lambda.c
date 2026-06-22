// lambda.c - core.lambda bridge for LambdaScript
//
// Build note:
//   Add the LambdaScript include directory to your XF compile flags, e.g.
//     -I/Volumes/Experiments/lambdaScript/include
//   and link the LambdaScript implementation/library, e.g.
//     /Volumes/Experiments/lambdaScript/libls.a
//   or the LambdaScript object files.

#include "internal.h"
#include "ls.h"

static pthread_mutex_t cl_eval_mu = PTHREAD_MUTEX_INITIALIZER;

static xf_Value cl_error(const char *msg, uint8_t type) {
    xf_err_t *e = xf_err_new(msg ? msg : "lambda error", "<core.lambda>", 0, 0);
    if (!e) return xf_val_nav(type);

    xf_Value out = xf_val_err(e, type);
    xf_err_release(e);
    return out;
}

static bool cl_arg_bool(xf_Value *args, size_t argc, size_t i, bool fallback) {
    if (i >= argc) return fallback;

    xf_Value v = args[i];

    if (v.type == XF_TYPE_BOOL) {
        if (v.state == XF_STATE_TRUE) return true;
        if (v.state == XF_STATE_FALSE) return false;
    }

    if (v.state == XF_STATE_TRUE) return true;
    if (v.state == XF_STATE_FALSE) return false;

    double n = 0.0;
    if (arg_num(args, argc, i, &n)) return n != 0.0;

    const char *s = NULL;
    size_t len = 0;
    if (arg_str(args, argc, i, &s, &len)) {
        if (len == 4 && strncmp(s, "true", 4) == 0) return true;
        if (len == 5 && strncmp(s, "false", 5) == 0) return false;
        if (len == 1 && s[0] == '1') return true;
        if (len == 1 && s[0] == '0') return false;
    }

    return fallback;
}

static void cl_options_from_args(xf_Value *args,
                                 size_t argc,
                                 size_t first_opt,
                                 bool trace,
                                 ls_Options *options) {
    ls_options_init(options);
    options->trace = trace ? 1 : 0;
    options->quiet = 1;
    options->use_prelude = 1;

    if (argc > first_opt) {
        double steps = 0.0;
        if (arg_num(args, argc, first_opt, &steps) && steps > 0.0) {
            options->max_steps = (size_t)steps;
        }
    }

    if (argc > first_opt + 1) {
        options->use_prelude = cl_arg_bool(args, argc, first_opt + 1, true) ? 1 : 0;
    }
}

static void cl_map_set(xf_map_t *m, const char *key, xf_Value value) {
    xf_Str *ks = xf_str_from_cstr(key);
    if (ks) {
        xf_map_set(m, ks, value);
        xf_str_release(ks);
    }
    xf_value_release(value);
}

static xf_Value cl_result_map(int ok, const ls_Result *result, const char *error_msg) {
    xf_map_t *m = xf_map_new();
    if (!m) return xf_val_nav(XF_TYPE_MAP);

    cl_map_set(m, "ok", xf_val_ok_bool(ok != 0));
    cl_map_set(m, "output", make_str_val((ok && result && result->output) ? result->output : "",
                                          (ok && result && result->output) ? strlen(result->output) : 0));
    cl_map_set(m, "trace", make_str_val((ok && result && result->trace) ? result->trace : "",
                                         (ok && result && result->trace) ? strlen(result->trace) : 0));
    cl_map_set(m, "steps", xf_val_ok_num((ok && result) ? (double)result->steps : 0.0));
    cl_map_set(m, "reached_limit", xf_val_ok_bool((ok && result && result->reached_step_limit) != 0));
    cl_map_set(m, "error", make_str_val((!ok && error_msg) ? error_msg : "",
                                         (!ok && error_msg) ? strlen(error_msg) : 0));

    xf_Value out = xf_val_ok_map(m);
    xf_map_release(m);
    return out;
}

static int cl_eval_string_locked(const char *source,
                                 const ls_Options *options,
                                 ls_Result *result,
                                 char *errbuf,
                                 size_t errcap) {
    int ok = 0;
    ls_State *L = NULL;

    if (errbuf && errcap > 0) errbuf[0] = '\0';

    pthread_mutex_lock(&cl_eval_mu);

    L = ls_newstate();
    if (!L) {
        if (errbuf && errcap > 0) snprintf(errbuf, errcap, "out of memory");
        goto done;
    }

    ok = ls_eval_string(L, source, options, result);
    if (!ok && errbuf && errcap > 0) {
        snprintf(errbuf, errcap, "%s", ls_errmsg(L));
    }

done:
    ls_close(L);
    pthread_mutex_unlock(&cl_eval_mu);
    return ok;
}

static int cl_eval_file_locked(const char *path,
                               const ls_Options *options,
                               ls_Result *result,
                               char *errbuf,
                               size_t errcap) {
    int ok = 0;
    ls_State *L = NULL;

    if (errbuf && errcap > 0) errbuf[0] = '\0';

    pthread_mutex_lock(&cl_eval_mu);

    L = ls_newstate();
    if (!L) {
        if (errbuf && errcap > 0) snprintf(errbuf, errcap, "out of memory");
        goto done;
    }

    ok = ls_eval_file(L, path, options, result);
    if (!ok && errbuf && errcap > 0) {
        snprintf(errbuf, errcap, "%s", ls_errmsg(L));
    }

done:
    ls_close(L);
    pthread_mutex_unlock(&cl_eval_mu);
    return ok;
}

/* core.lambda.eval(source [, max_steps [, use_prelude]]) -> str */
static xf_Value cl_eval(xf_Value *args, size_t argc) {
    NEED(1);

    const char *source = NULL;
    size_t source_len = 0;
    if (!arg_str(args, argc, 0, &source, &source_len)) return propagate(args, argc);

    ls_Options options;
    ls_Result result;
    char errbuf[512];

    cl_options_from_args(args, argc, 1, false, &options);
    ls_result_init(&result);

    if (!cl_eval_string_locked(source, &options, &result, errbuf, sizeof(errbuf))) {
        ls_result_free(&result);
        return cl_error(errbuf, XF_TYPE_STR);
    }

    xf_Value out = make_str_val(result.output ? result.output : "",
                                result.output ? strlen(result.output) : 0);
    ls_result_free(&result);
    return out;
}

/* core.lambda.file(path [, max_steps [, use_prelude]]) -> str */
static xf_Value cl_file(xf_Value *args, size_t argc) {
    NEED(1);

    const char *path = NULL;
    size_t path_len = 0;
    if (!arg_str(args, argc, 0, &path, &path_len)) return propagate(args, argc);

    ls_Options options;
    ls_Result result;
    char errbuf[512];

    cl_options_from_args(args, argc, 1, false, &options);
    ls_result_init(&result);

    if (!cl_eval_file_locked(path, &options, &result, errbuf, sizeof(errbuf))) {
        ls_result_free(&result);
        return cl_error(errbuf, XF_TYPE_STR);
    }

    xf_Value out = make_str_val(result.output ? result.output : "",
                                result.output ? strlen(result.output) : 0);
    ls_result_free(&result);
    return out;
}

/* core.lambda.run(source [, max_steps [, use_prelude]]) -> map */
static xf_Value cl_run(xf_Value *args, size_t argc) {
    NEED(1);

    const char *source = NULL;
    size_t source_len = 0;
    if (!arg_str(args, argc, 0, &source, &source_len)) return propagate(args, argc);

    ls_Options options;
    ls_Result result;
    char errbuf[512];

    cl_options_from_args(args, argc, 1, false, &options);
    ls_result_init(&result);

    if (!cl_eval_string_locked(source, &options, &result, errbuf, sizeof(errbuf))) {
        xf_Value out = cl_result_map(0, NULL, errbuf);
        ls_result_free(&result);
        return out;
    }

    xf_Value out = cl_result_map(1, &result, NULL);
    ls_result_free(&result);
    return out;
}

/* core.lambda.run_file(path [, max_steps [, use_prelude]]) -> map */
static xf_Value cl_run_file(xf_Value *args, size_t argc) {
    NEED(1);

    const char *path = NULL;
    size_t path_len = 0;
    if (!arg_str(args, argc, 0, &path, &path_len)) return propagate(args, argc);

    ls_Options options;
    ls_Result result;
    char errbuf[512];

    cl_options_from_args(args, argc, 1, false, &options);
    ls_result_init(&result);

    if (!cl_eval_file_locked(path, &options, &result, errbuf, sizeof(errbuf))) {
        xf_Value out = cl_result_map(0, NULL, errbuf);
        ls_result_free(&result);
        return out;
    }

    xf_Value out = cl_result_map(1, &result, NULL);
    ls_result_free(&result);
    return out;
}

/* core.lambda.trace(source [, max_steps [, use_prelude]]) -> map */
static xf_Value cl_trace(xf_Value *args, size_t argc) {
    NEED(1);

    const char *source = NULL;
    size_t source_len = 0;
    if (!arg_str(args, argc, 0, &source, &source_len)) return propagate(args, argc);

    ls_Options options;
    ls_Result result;
    char errbuf[512];

    cl_options_from_args(args, argc, 1, true, &options);
    ls_result_init(&result);

    if (!cl_eval_string_locked(source, &options, &result, errbuf, sizeof(errbuf))) {
        xf_Value out = cl_result_map(0, NULL, errbuf);
        ls_result_free(&result);
        return out;
    }

    xf_Value out = cl_result_map(1, &result, NULL);
    ls_result_free(&result);
    return out;
}

xf_module_t *build_lambda(void) {
    xf_module_t *m = xf_module_new("core.lambda");
    FN("eval",     XF_TYPE_STR, cl_eval);
    FN("file",     XF_TYPE_STR, cl_file);
    FN("run",      XF_TYPE_MAP, cl_run);
    FN("run_file", XF_TYPE_MAP, cl_run_file);
    FN("trace",    XF_TYPE_MAP, cl_trace);
    return m;
}
