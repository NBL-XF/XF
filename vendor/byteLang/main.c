#include "bl.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    const char *path;
    int check_only;
    int print_result;
} BLCLIOptions;

static void bl_usage(const char *argv0) {
    fprintf(stderr,
            "usage:\n"
            "  %s [--check] [--print-result] file.bl\n"
            "  %s --help\n",
            argv0, argv0);
}

static int bl_parse_cli(int argc, char **argv, BLCLIOptions *options) {
    memset(options, 0, sizeof(*options));

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "--help") == 0) {
            bl_usage(argv[0]);
            return 0;
        }
        if (strcmp(arg, "--check") == 0) {
            options->check_only = 1;
            continue;
        }
        if (strcmp(arg, "--print-result") == 0) {
            options->print_result = 1;
            continue;
        }
        if (!options->path) {
            options->path = arg;
            continue;
        }

        fprintf(stderr, "unexpected argument: %s\n", arg);
        bl_usage(argv[0]);
        return 2;
    }

    if (!options->path) {
        bl_usage(argv[0]);
        return 2;
    }

    return 1;
}

static int bl_is_directory(const char *path) {
    struct stat st;
    if (!path || !path[0] || stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

static int bl_join_path(char *out, size_t out_size, const char *left, const char *right) {
    if (!out || out_size == 0 || !left || !right) return 0;
    return snprintf(out, out_size, "%s%s%s",
                    left,
                    (left[0] && left[strlen(left) - 1] == '/') ? "" : "/",
                    right) < (int)out_size;
}

static int bl_mkdir_if_missing(const char *path) {
    if (mkdir(path, 0777) == 0) return 1;
    return errno == EEXIST;
}

static int bl_mkdirs(const char *path) {
    char buffer[PATH_MAX];
    size_t len;

    if (!path || !path[0]) return 0;
    len = strlen(path);
    if (len >= sizeof(buffer)) return 0;

    memcpy(buffer, path, len + 1);

    for (char *p = buffer + 1; *p; ++p) {
        if (*p != '/') continue;
        *p = '\0';
        if (buffer[0] && !bl_mkdir_if_missing(buffer)) return 0;
        *p = '/';
    }

    return bl_mkdir_if_missing(buffer);
}

static int bl_copy_file(const char *src, const char *dst) {
    FILE *in = NULL;
    FILE *out = NULL;
    char buffer[8192];
    size_t read_count;
    int ok = 0;

    in = fopen(src, "rb");
    if (!in) goto cleanup;

    out = fopen(dst, "wb");
    if (!out) goto cleanup;

    while ((read_count = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        if (fwrite(buffer, 1, read_count, out) != read_count) goto cleanup;
    }

    if (ferror(in)) goto cleanup;
    ok = 1;

cleanup:
    if (out) fclose(out);
    if (in) fclose(in);
    return ok;
}

static int bl_copy_tree(const char *src_dir, const char *dst_dir) {
    DIR *dir = NULL;
    struct dirent *entry;
    int ok = 0;

    if (!bl_is_directory(src_dir)) return 1;
    if (!bl_mkdirs(dst_dir)) return 0;

    dir = opendir(src_dir);
    if (!dir) return 0;

    ok = 1;
    while ((entry = readdir(dir)) != NULL) {
        char src_path[PATH_MAX];
        char dst_path[PATH_MAX];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (!bl_join_path(src_path, sizeof(src_path), src_dir, entry->d_name) ||
            !bl_join_path(dst_path, sizeof(dst_path), dst_dir, entry->d_name)) {
            ok = 0;
            break;
        }

        if (bl_is_directory(src_path)) {
            if (!bl_copy_tree(src_path, dst_path)) {
                ok = 0;
                break;
            }
            continue;
        }

        if (!bl_copy_file(src_path, dst_path)) {
            ok = 0;
            break;
        }
    }

    closedir(dir);
    return ok;
}

static int bl_get_global_lib_dir(char *out, size_t out_size) {
    const char *home = getenv("HOME");

    if (home && home[0]) {
        return snprintf(out, out_size, "%s/.bytelang/lib", home) < (int)out_size;
    }

    return snprintf(out, out_size, "./.bytelang/lib") < (int)out_size;
}

static int bl_ensure_global_lib_dir(void) {
    char lib_dir[PATH_MAX];

    if (!bl_get_global_lib_dir(lib_dir, sizeof(lib_dir))) {
        return 0;
    }

    if (bl_is_directory(lib_dir)) {
        return 1;
    }

    if (!bl_mkdirs(lib_dir)) {
        return 0;
    }

    return bl_copy_tree("./Libraries", lib_dir);
}

static void bl_print_result(const BLValueView *result) {
    if (!result) return;

    switch (result->kind) {
        case BL_VALUE_NULL:
            break;
        case BL_VALUE_INT:
            printf("%lld\n", result->as.int_value);
            break;
        case BL_VALUE_STRING:
            printf("%s\n", result->as.string_value ? result->as.string_value : "");
            break;
        case BL_VALUE_BUFFER:
            printf("<buffer size=%zu owned=%d>\n",
                   result->as.ptr_value.size,
                   result->as.ptr_value.owned);
            break;
        case BL_VALUE_ARRAY:
            printf("<array size=%zu owned=%d>\n",
                   result->as.array_value.size,
                   result->as.array_value.owned);
            break;
        default:
            break;
    }
}

int main(int argc, char **argv) {
    BLCLIOptions options;
    int parse_rc = bl_parse_cli(argc, argv, &options);
    if (parse_rc <= 0) return 0;
    if (parse_rc > 1) return parse_rc;

    if (!bl_ensure_global_lib_dir()) {
        fprintf(stderr, "failed to initialize ~/.bytelang/lib\n");
        return 1;
    }

    BLInterpreter *interp = bl_interpreter_create();
    if (!interp) {
        fprintf(stderr, "failed to create interpreter\n");
        return 1;
    }

    BLStatus status;
    BLValueView result;
    memset(&result, 0, sizeof(result));

    if (options.check_only) {
        status = bl_check_file(interp, options.path);
    } else {
        status = bl_run_file(interp, options.path, &result);
    }

    if (status != BL_STATUS_OK) {
        fprintf(stderr, "%s\n", bl_last_error(interp));
        bl_interpreter_destroy(interp);
        return 1;
    }

    if (options.print_result && !options.check_only) {
        bl_print_result(&result);
    }

    bl_interpreter_destroy(interp);
    return 0;
}
