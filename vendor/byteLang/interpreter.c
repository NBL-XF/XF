#include "interpreter.h"

#include "parser.h"
#include "value.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *bl_strdup_local(const char *text) {
    size_t length = strlen(text);
    char *copy = (char *)calloc(length + 1, 1);
    if (!copy) {
        perror("calloc");
        exit(1);
    }
    memcpy(copy, text, length + 1);
    return copy;
}

static char *bl_read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char *buffer = (char *)calloc((size_t)size + 1, 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    if (size > 0 && fread(buffer, 1, (size_t)size, file) != (size_t)size) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return buffer;
}

static void bl_set_error(BLInterpreter *interp, const char *message) {
    if (!interp) return;
    free(interp->last_error);
    interp->last_error = NULL;
    if (!message) return;
    interp->last_error = bl_strdup_local(message);
}

static BLStatus bl_parse_only(BLInterpreter *interp, const char *virtual_path, const char *source) {
    BLParser parser;
    bl_parser_init(&parser, virtual_path, source);
    BLNode *program = bl_parse_program(&parser);
    if (!program) {
        bl_set_error(interp, parser.error ? parser.error : "parse error");
        bl_parser_dispose(&parser);
        return BL_STATUS_ERROR;
    }

    bl_parser_dispose(&parser);
    bl_set_error(interp, NULL);
    return BL_STATUS_OK;
}

static void bl_fill_view(BLValue value, BLValueView *view) {
    if (!view) return;
    memset(view, 0, sizeof(*view));

    switch (value.tag) {
        case BLV_NULL:
            view->kind = BL_VALUE_NULL;
            break;
        case BLV_INT:
            view->kind = BL_VALUE_INT;
            view->as.int_value = value.as.i;
            break;
        case BLV_OBJECT:
            if (bl_value_is_string(value)) {
                BLString *string = bl_value_as_string(value);
                view->kind = BL_VALUE_STRING;
                view->as.string_value = string->data;
            } else if (bl_value_is_buffer(value)) {
                BLBuffer *buffer = bl_value_as_buffer(value);
                view->kind = BL_VALUE_BUFFER;
                view->as.ptr_value.data = buffer->data;
                view->as.ptr_value.size = buffer->size;
                view->as.ptr_value.owned = buffer->owned;
            } else if (bl_value_is_array(value)) {
                BLArray *array = bl_value_as_array(value);
                view->kind = BL_VALUE_ARRAY;
                view->as.array_value.size = array->size;
                view->as.array_value.owned = array->owned;
            }
            break;
    }
}

BLInterpreter *bl_interpreter_create(void) {
    return (BLInterpreter *)calloc(1, sizeof(BLInterpreter));
}

void bl_interpreter_destroy(BLInterpreter *interp) {
    if (!interp) return;
    free(interp->last_error);
    free(interp);
}

BLStatus bl_interpreter_check_source(BLInterpreter *interp, const char *virtual_path, const char *source) {
    return bl_parse_only(interp, virtual_path, source);
}

BLStatus bl_interpreter_check_file(BLInterpreter *interp, const char *path) {
    char *source = bl_read_file(path);
    if (!source) {
        bl_set_error(interp, "failed to read file");
        return BL_STATUS_ERROR;
    }
    BLStatus status = bl_interpreter_check_source(interp, path, source);
    free(source);
    return status;
}

BLStatus bl_interpreter_run_source(BLInterpreter *interp, const char *virtual_path, const char *source, BLValueView *out_value) {
    BLParser parser;
    bl_parser_init(&parser, virtual_path, source);
    BLNode *program = bl_parse_program(&parser);
    if (!program) {
        bl_set_error(interp, parser.error ? parser.error : "parse error");
        bl_parser_dispose(&parser);
        return BL_STATUS_ERROR;
    }

    BLVM vm;
    bl_vm_init(&vm, virtual_path);
    BLValue result = bl_value_null();
    int ok = bl_vm_execute(&vm, program, &result);
    if (!ok) {
        bl_set_error(interp, bl_vm_last_error(&vm));
        bl_value_release(result);
        bl_vm_dispose(&vm);
        bl_parser_dispose(&parser);
        return BL_STATUS_ERROR;
    }

    bl_fill_view(result, out_value);
    bl_value_release(result);
    bl_vm_dispose(&vm);
    bl_parser_dispose(&parser);
    bl_set_error(interp, NULL);
    return BL_STATUS_OK;
}

BLStatus bl_interpreter_run_file(BLInterpreter *interp, const char *path, BLValueView *out_value) {
    char *source = bl_read_file(path);
    if (!source) {
        bl_set_error(interp, "failed to read file");
        return BL_STATUS_ERROR;
    }
    BLStatus status = bl_interpreter_run_source(interp, path, source, out_value);
    free(source);
    return status;
}

BLStatus bl_check_file(BLInterpreter *interp, const char *path) {
    return bl_interpreter_check_file(interp, path);
}

BLStatus bl_check_source(BLInterpreter *interp, const char *virtual_path, const char *source) {
    return bl_interpreter_check_source(interp, virtual_path, source);
}

BLStatus bl_run_file(BLInterpreter *interp, const char *path, BLValueView *out_value) {
    return bl_interpreter_run_file(interp, path, out_value);
}

BLStatus bl_run_source(BLInterpreter *interp, const char *virtual_path, const char *source, BLValueView *out_value) {
    return bl_interpreter_run_source(interp, virtual_path, source, out_value);
}

const char *bl_last_error(const BLInterpreter *interp) {
    return (interp && interp->last_error) ? interp->last_error : "";
}
