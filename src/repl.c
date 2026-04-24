#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "../include/core.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/symTable.h"
#include "../include/ast.h"
#include "../include/vm.h"
#include "../include/value.h"
#include "../include/interp.h"
#define VERSION "1.0.0"

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

static void xf_repl_print_result(VM *vm) {
    if (!vm || vm->stack_top == 0) return;

    xf_Value v = vm_peek(vm, 0);
    xf_value_repl_print(v);

    while (vm->stack_top > 0) {
        xf_value_release(vm_pop(vm));
    }
}

static void xf_repl_clear_eval_artifacts(VM *vm) {
    if (!vm) return;

    if (vm->begin_chunk) {
        chunk_free(vm->begin_chunk);
        free(vm->begin_chunk);
        vm->begin_chunk = NULL;
    }

    if (vm->end_chunk) {
        chunk_free(vm->end_chunk);
        free(vm->end_chunk);
        vm->end_chunk = NULL;
    }

    if (vm->rules) {
        for (size_t i = 0; i < vm->rule_count; i++) {
            if (vm->rules[i]) {
                chunk_free(vm->rules[i]);
                free(vm->rules[i]);
                vm->rules[i] = NULL;
            }
        }
        free(vm->rules);
        vm->rules = NULL;
    }

    if (vm->patterns) {
        for (size_t i = 0; i < vm->rule_count; i++) {
            xf_value_release(vm->patterns[i]);
        }
        free(vm->patterns);
        vm->patterns = NULL;
    }

    vm->rule_count = 0;
}

static int xf_repl_eval_line(VM *vm, SymTable *syms, const char *line) {
    if (!vm || !syms || !line) return 1;

    Lexer lex = {0};
    Program *prog = NULL;

    xf_repl_clear_eval_artifacts(vm);

    xf_lex_init_cstr(&lex, line, XF_SRC_REPL, "<repl>");
    if (lex.had_error) {
        fprintf(stderr, "repl lexer init error: %s\n", lex.err_msg);
        xf_lex_free(&lex);
        return 1;
    }

    xf_tokenize(&lex);
    if (lex.had_error) {
        fprintf(stderr, "[%s:%u:%u] lexer error: %s\n",
                lex.source_name ? lex.source_name : "<repl>",
                lex.line,
                lex.col,
                lex.err_msg);
        xf_lex_free(&lex);
        return 1;
    }

    prog = xf_parse_program(&lex, syms);
    if (!prog) {
        fprintf(stderr, "repl parse failed\n");
        xf_lex_free(&lex);
        return 1;
    }

    Interp it = {0};
    it.vm   = vm;
    it.syms = syms;
core_set_fn_caller(vm, syms, interp_exec_xf_fn_bridge);
if (prog->count == 1 &&
    prog->items[0] &&
    prog->items[0]->kind == TOP_STMT &&
    prog->items[0]->as.stmt.stmt &&
    prog->items[0]->as.stmt.stmt->kind == STMT_EXPR) {

    Stmt *expr_stmt = prog->items[0]->as.stmt.stmt;

    vm->begin_chunk = calloc(1, sizeof(Chunk));
    if (!vm->begin_chunk) {
        fprintf(stderr, "repl compile failed\n");
        ast_program_free(prog);
        xf_lex_free(&lex);
        xf_repl_clear_eval_artifacts(vm);
        return 1;
    }

    chunk_init(vm->begin_chunk, "<repl-expr>");

    if (!interp_compile_expr_repl(&it,
                                  vm->begin_chunk,
                                  expr_stmt->as.expr.expr)) {
        fprintf(stderr, "repl compile failed\n");
        ast_program_free(prog);
        xf_lex_free(&lex);
        xf_repl_clear_eval_artifacts(vm);
        return 1;
    }

    chunk_write(vm->begin_chunk, OP_INSPECT, expr_stmt->loc.line);
    chunk_write(vm->begin_chunk, OP_HALT, 0);
}
 else {
    if (!interp_compile_program_repl(&it, prog)) {
        fprintf(stderr, "repl compile failed\n");
        ast_program_free(prog);
        xf_lex_free(&lex);
        xf_repl_clear_eval_artifacts(vm);
        return 1;
    }
}
    if (vm_run_begin(vm) != VM_OK) {
        fprintf(stderr, "repl BEGIN failed\n");
        ast_program_free(prog);
        xf_lex_free(&lex);
        xf_repl_clear_eval_artifacts(vm);
        return 1;
    }

    xf_repl_print_result(vm);

    ast_program_free(prog);
    xf_lex_free(&lex);
    xf_repl_clear_eval_artifacts(vm);
    return 0;
}

int xf_run_repl(void) {
    VM vm;
    SymTable syms;

    vm_init(&vm, 1);
    sym_init(&syms);

    if (syms.had_error) {
        fprintf(stderr, "xf: symtable init error: %s\n", syms.err_msg);
        sym_free(&syms);
        vm_free(&vm);
        return 1;
    }

    sym_register_builtins(&syms);
    core_register(&syms);
    core_set_fn_caller(&vm, &syms, interp_exec_xf_fn_bridge);

    Interp it0 = {0};
    it0.vm = &vm;
    it0.syms = &syms;
    bind_runtime_specials(&it0);

    puts("              *****:          -**********              ");
    puts("               ******:       -************             ");
    puts("                 ******     **************             ");
    puts("                  ******   *******-::::::.             ");
    puts("                   +****** *****                       ");
    puts("                    ******* ***                        ");
    puts("                     ******=  .*****                   ");
    puts("                      ******= ******                   ");
    puts("                       ****** =******                  ");
    puts("                    *   ******                         ");
    puts("                  *****  *******                       ");
    puts("                ********   *******                     ");
    puts("              ********       ******                    ");
    puts("              *******         *******                  ");
    puts("              ******           *****:                  ");
    printf("xf repl v%s  (:quit to exit)\n", VERSION);

    using_history();

    char *line;
    while ((line = readline(">> ")) != NULL) {
        /* strip trailing whitespace */
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' '))
            line[--len] = '\0';

        if (len == 0) {
            free(line);
            continue;
        }

        if (strcmp(line, ":quit") == 0 || strcmp(line, ":q") == 0) {
            free(line);
            break;
        }

        if (strcmp(line, ":stack") == 0) {
            vm_dump_stack(&vm);
            free(line);
            continue;
        }

        /* add non-duplicate entries to history */
        HIST_ENTRY *prev = history_length > 0 ? history_get(history_base + history_length - 1) : NULL;
        if (!prev || strcmp(prev->line, line) != 0)
            add_history(line);

        vm.had_error = false;
        memset(vm.err_msg, 0, sizeof(vm.err_msg));

        xf_repl_eval_line(&vm, &syms, line);
        free(line);
    }

    if (!line) printf("\n"); /* handle Ctrl+D cleanly */

    xf_repl_clear_eval_artifacts(&vm);
    sym_free(&syms);
    vm_free(&vm);
    return 0;
}