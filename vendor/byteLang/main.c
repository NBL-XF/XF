#include "bl.h"

#include <stdio.h>
#include <string.h>

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
