#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/core.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/symTable.h"
#include "../include/ast.h"
#include "../include/vm.h"
#include "../include/value.h"
#include "../include/interp.h"
#include "../include/repl.h"
static int xf_run_program(Program *prog, int argc, char **argv);
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "xf: cannot open '%s'\n", path);
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        fprintf(stderr, "xf: fseek failed for '%s'\n", path);
        return NULL;
    }

    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        fprintf(stderr, "xf: ftell failed for '%s'\n", path);
        return NULL;
    }

    rewind(f);

    char *buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        fprintf(stderr, "xf: out of memory reading '%s'\n", path);
        return NULL;
    }

    size_t nread = fread(buf, 1, (size_t)size, f);
    fclose(f);

    if (nread != (size_t)size) {
        free(buf);
        fprintf(stderr, "xf: failed to read '%s'\n", path);
        return NULL;
    }

    buf[size] = '\0';
    return buf;
}
static int xf_run_source(const char *source_name,
                         const char *source,
                         SrcMode mode,
                         int argc,
                         char **argv) {
    int rc = 1;

    Lexer lex = {0};
    SymTable syms = {0};
    Program *prog = NULL;

    if (!source) return 1;

    xf_lex_init(&lex, source, strlen(source), mode, source_name ? source_name : "<inline>");
    if (lex.had_error) {
        fprintf(stderr, "xf: lexer init error: %s\n", lex.err_msg);
        goto done;
    }

    xf_tokenize(&lex);

    if (lex.had_error) {
        fprintf(stderr, "[%s:%u:%u] lexer error: %s\n",
                lex.source_name ? lex.source_name : "<unknown>",
                lex.line,
                lex.col,
                lex.err_msg);
        goto done;
    }

    sym_init(&syms);
    if (syms.had_error) {
        fprintf(stderr, "xf: symtable init error: %s\n", syms.err_msg);
        goto done;
    }

    prog = xf_parse_program(&lex, &syms);
    if (!prog) {
        fprintf(stderr, "parser error\n");
        goto done;
    }

    rc = xf_run_program(prog, argc, argv);

done:
    ast_program_free(prog);
    sym_free(&syms);
    xf_lex_free(&lex);
    return rc;
}

static void usage(const char *argv0) {
    const char *name = argv0 ? argv0 : "xf";
    fprintf(stderr,
            "usage:\n"
            "  %s                 # repl\n"
            "  %s -r <file.xf>    # run file\n"
            "  %s -e \"code\"      # execute inline source\n",
            name, name, name);
}
static void bind_runtime_specials(Interp *it) {
    if (!it || !it->vm) return;

    struct {
        const char *name;
        xf_Value value;
    } specs[5];

specs[0].name = "file";
xf_Str *file_s = xf_str_from_cstr("");
specs[0].value = file_s ? xf_val_ok_str(file_s) : xf_val_null();
xf_str_release(file_s);
    specs[1].name = "match";
    specs[1].value = xf_val_null();

    xf_arr_t *caps = xf_arr_new();
    specs[2].name = "captures";
    specs[2].value = caps ? xf_val_ok_arr(caps) : xf_val_null();
    if (caps) xf_arr_release(caps);

    specs[3].name = "err";
    specs[3].value = xf_val_null();

    xf_Str *ofmt_s = xf_str_from_cstr("%.6g");
    specs[4].name = "OFMT";
    specs[4].value = ofmt_s ? xf_val_ok_str(ofmt_s) : xf_val_null();
    xf_str_release(ofmt_s);

    for (size_t i = 0; i < 5; i++) {
        uint32_t slot = interp_bind_global_cstr(it, specs[i].name);
        if (slot != UINT32_MAX) {
            vm_set_global(it->vm, slot, specs[i].value);
        }
        xf_value_release(specs[i].value);
    }
}

static void inject_args(Interp *it, int argc, char **argv) {
    xf_arr_t *arr = xf_arr_new();
    if (!arr) return;

    for (int i = 0; i < argc; i++) {
        xf_Str *s = xf_str_new(argv[i], strlen(argv[i]));
        if (!s) continue;

        xf_Value v = xf_val_ok_str(s);
        xf_arr_push(arr, v);

        xf_value_release(v);
        xf_str_release(s);
    }
xf_Value arrv = xf_val_ok_arr(arr);
uint32_t slot = interp_bind_global_cstr(it, "ARGS");
if (slot == UINT32_MAX) {
    xf_value_release(arrv);
    xf_arr_release(arr);
    return;
}

vm_set_global(it->vm, slot, arrv);

xf_value_release(arrv);
xf_arr_release(arr);
}

/* Drain any leftover values the VM left on the stack. */
static void xf_drain_stack(VM *vm) {
    while (vm->stack_top > 0) {
        xf_value_release(vm_pop(vm));
    }
}
static int xf_run_program(Program *prog, int argc, char **argv) {
    VM vm;
    vm_init(&vm, 1);

    SymTable syms;
    sym_init(&syms);
    if (syms.had_error) {
        fprintf(stderr, "xf: symtable init error: %s\n", syms.err_msg);
        vm_free(&vm);
        sym_free(&syms);
        return 1;
    }

    sym_register_builtins(&syms);
    core_register(&syms);

    Interp it = {0};
    it.vm   = &vm;
    it.syms = &syms;

    core_set_fn_caller(&vm, &syms, interp_exec_xf_fn_bridge);
    bind_runtime_specials(&it);

    if (!interp_compile_program(&it, prog)) {
        fprintf(stderr, "xf: compile failed\n");

        if (it.vm && it.vm->had_error) {
            fprintf(stderr, "xf: vm error: %s\n", it.vm->err_msg);
        }

        if (it.syms && it.syms->had_error) {
            fprintf(stderr, "xf: sym error: %s\n", it.syms->err_msg);
        }

        interp_reset_global_bindings(&it);
        vm_free(&vm);
        sym_free(&syms);
        return 1;
    }

    inject_args(&it, argc, argv);

    VMResult begin_rc = vm_run_begin(&vm);
    xf_drain_stack(&vm);

    if (begin_rc != VM_OK && !vm.should_exit) {
        fprintf(stderr, "BEGIN failed\n");
        interp_reset_global_bindings(&it);
        vm_free(&vm);
        sym_free(&syms);
        return 1;
    }

    if (!vm.should_exit && !isatty(fileno(stdin))) {
        char buf[4096];

        while (!vm.should_exit && fgets(buf, sizeof(buf), stdin)) {
            size_t len = strlen(buf);

            if (vm_feed_record(&vm, buf, len) != VM_OK) {
                fprintf(stderr, "runtime error\n");
                interp_reset_global_bindings(&it);
                vm_free(&vm);
                sym_free(&syms);
                return 1;
            }

            xf_drain_stack(&vm);
        }
    }

    if (!vm.should_exit) {
        VMResult end_rc = vm_run_end(&vm);
        xf_drain_stack(&vm);

        if (end_rc != VM_OK && !vm.should_exit) {
            fprintf(stderr, "END failed\n");
            interp_reset_global_bindings(&it);
            vm_free(&vm);
            sym_free(&syms);
            return 1;
        }
    }

    interp_reset_global_bindings(&it);
    vm_free(&vm);
    sym_free(&syms);
    return 0;
}
int main(int argc, char **argv) {
    if (argc == 1) {
        return xf_run_repl();
    }
if (argc >= 3 && strcmp(argv[1], "-r") == 0) {
    char *source = read_file(argv[2]);
    if (!source) {
        return 1;
    }

    int rc = xf_run_source(argv[2], source, XF_SRC_FILE, argc - 2, &argv[2]);
    free(source);
    return rc;
}
if (argc >= 3 && strcmp(argv[1], "-e") == 0) {
    return xf_run_source("<inline>", argv[2], XF_SRC_INLINE, argc - 2, &argv[2]);
}
    usage(argv[0]);
    return 1;
}