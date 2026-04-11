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

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s <file.xf>\n", argv0 ? argv0 : "xf");
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
static int xf_run_program(Program *prog, int argc, char **argv) {
    VM vm;
    vm_init(&vm, 1);
        SymTable syms;
    sym_init(&syms);
    if (syms.had_error) {
        fprintf(stderr, "xf: symtable init error: %s\n", syms.err_msg);
        vm_free(&vm);
        return 1;
    }

    sym_register_builtins(&syms);
    core_register(&syms);

    Interp it = {0};
    it.vm   = &vm;
    it.syms = &syms;
core_set_fn_caller(&vm, &syms, interp_exec_xf_fn_bridge);
    if (!interp_compile_program(&it, prog)) {
        fprintf(stderr, "compile failed\n");
        sym_free(&syms);
        vm_free(&vm);
        return 1;
    }

    inject_args(&it, argc, argv);

    if (vm_run_begin(&vm) != VM_OK) {
        fprintf(stderr, "BEGIN failed\n");
        sym_free(&syms);
        vm_free(&vm);
        return 1;
    }

    char buf[4096];
        while (!vm.should_exit && fgets(buf, sizeof(buf), stdin)) {
        size_t len = strlen(buf);
        if (vm_feed_record(&vm, buf, len) != VM_OK) {
            fprintf(stderr, "runtime error\n");
            sym_free(&syms);
            vm_free(&vm);
            return 1;
        }
    }

    if (vm_run_end(&vm) != VM_OK) {
        fprintf(stderr, "END failed\n");
        sym_free(&syms);
        vm_free(&vm);
        return 1;
    }

    sym_free(&syms);
    vm_free(&vm);
    return 0;
}
int main(int argc, char **argv) {
    int rc = 1;
    const char *path = NULL;
    char *source = NULL;

    Lexer lex = {0};
    SymTable syms = {0};
    Program *prog = NULL;

    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    path = argv[1];

    source = read_file(path);
    if (!source) {
        goto done;
    }

    xf_lex_init_cstr(&lex, source, XF_SRC_FILE, path);
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

//    ast_program_print(prog);

    rc = xf_run_program(prog, argc - 1, argv + 1);

done:
    ast_program_free(prog);
    sym_free(&syms);
    xf_lex_free(&lex);
    free(source);
    return rc;
}