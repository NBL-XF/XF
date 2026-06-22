#include "value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BLObject *bl_object_alloc(size_t size, BLObjectKind kind) {
    BLObject *object = (BLObject *)calloc(1, size);
    if (!object) {
        perror("calloc");
        exit(1);
    }
    object->kind = kind;
    object->ref_count = 1;
    return object;
}

BLValue bl_value_null(void) {
    BLValue value;
    value.tag = BLV_NULL;
    value.as.i = 0;
    return value;
}

BLValue bl_value_int(long long number) {
    BLValue value;
    value.tag = BLV_INT;
    value.as.i = number;
    return value;
}

BLValue bl_value_string_n(const char *text, size_t length) {
    BLString *string = (BLString *)bl_object_alloc(sizeof(BLString), BLO_STRING);
    string->data = (char *)calloc(length + 1, 1);
    if (!string->data) {
        perror("calloc");
        exit(1);
    }
    memcpy(string->data, text, length);
    string->size = length;

    BLValue value;
    value.tag = BLV_OBJECT;
    value.as.obj = &string->base;
    return value;
}

BLValue bl_value_string_copy(const char *text) {
    return bl_value_string_n(text ? text : "", text ? strlen(text) : 0);
}

BLValue bl_value_buffer(size_t size) {
    BLBuffer *buffer = (BLBuffer *)bl_object_alloc(sizeof(BLBuffer), BLO_BUFFER);
    buffer->data = (unsigned char *)calloc(size ? size : 1, 1);
    if (!buffer->data) {
        perror("calloc");
        exit(1);
    }
    buffer->size = size;
    buffer->owned = 1;

    BLValue value;
    value.tag = BLV_OBJECT;
    value.as.obj = &buffer->base;
    return value;
}

BLValue bl_value_array(size_t size) {
    BLArray *array = (BLArray *)bl_object_alloc(sizeof(BLArray), BLO_ARRAY);
    array->items = (BLValue *)calloc(size ? size : 1, sizeof(BLValue));
    if (!array->items) {
        perror("calloc");
        exit(1);
    }
    for (size_t i = 0; i < size; ++i) {
        array->items[i] = bl_value_null();
    }
    array->size = size;
    array->owned = 1;

    BLValue value;
    value.tag = BLV_OBJECT;
    value.as.obj = &array->base;
    return value;
}

int bl_value_is_truthy(BLValue value) {
    switch (value.tag) {
        case BLV_NULL:
            return 0;
        case BLV_INT:
            return value.as.i != 0;
        case BLV_OBJECT:
            return 1;
    }
    return 0;
}

void bl_value_retain(BLValue value) {
    if (value.tag == BLV_OBJECT && value.as.obj) {
        value.as.obj->ref_count += 1;
    }
}

static void bl_object_free(BLObject *object) {
    if (!object) return;
    switch (object->kind) {
        case BLO_STRING: {
            BLString *string = (BLString *)object;
            free(string->data);
            free(string);
            break;
        }
        case BLO_BUFFER: {
            BLBuffer *buffer = (BLBuffer *)object;
            free(buffer->data);
            free(buffer);
            break;
        }
        case BLO_ARRAY: {
            BLArray *array = (BLArray *)object;
            if (array->items) {
                for (size_t i = 0; i < array->size; ++i) {
                    bl_value_release(array->items[i]);
                }
            }
            free(array->items);
            free(array);
            break;
        }
    }
}

void bl_value_release(BLValue value) {
    if (value.tag != BLV_OBJECT || !value.as.obj) return;
    value.as.obj->ref_count -= 1;
    if (value.as.obj->ref_count == 0) {
        bl_object_free(value.as.obj);
    }
}

BLValue bl_value_clone(BLValue value) {
    bl_value_retain(value);
    return value;
}

int bl_value_is_string(BLValue value) {
    return value.tag == BLV_OBJECT && value.as.obj && value.as.obj->kind == BLO_STRING;
}

int bl_value_is_buffer(BLValue value) {
    return value.tag == BLV_OBJECT && value.as.obj && value.as.obj->kind == BLO_BUFFER;
}

int bl_value_is_array(BLValue value) {
    return value.tag == BLV_OBJECT && value.as.obj && value.as.obj->kind == BLO_ARRAY;
}

BLString *bl_value_as_string(BLValue value) {
    return bl_value_is_string(value) ? (BLString *)value.as.obj : NULL;
}

BLBuffer *bl_value_as_buffer(BLValue value) {
    return bl_value_is_buffer(value) ? (BLBuffer *)value.as.obj : NULL;
}

BLArray *bl_value_as_array(BLValue value) {
    return bl_value_is_array(value) ? (BLArray *)value.as.obj : NULL;
}

char *bl_value_to_cstring(BLValue value) {
    if (bl_value_is_string(value)) {
        BLString *string = bl_value_as_string(value);
        char *copy = (char *)calloc(string->size + 1, 1);
        if (!copy) {
            perror("calloc");
            exit(1);
        }
        memcpy(copy, string->data, string->size);
        return copy;
    }

    if (value.tag == BLV_INT) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%lld", value.as.i);
        size_t length = strlen(buffer);
        char *copy = (char *)calloc(length + 1, 1);
        if (!copy) {
            perror("calloc");
            exit(1);
        }
        memcpy(copy, buffer, length + 1);
        return copy;
    }

    char *copy = (char *)calloc(5, 1);
    if (!copy) {
        perror("calloc");
        exit(1);
    }
    memcpy(copy, "null", 5);
    return copy;
}

long long bl_value_to_int(BLValue value, int *ok) {
    if (value.tag == BLV_INT) {
        if (ok) *ok = 1;
        return value.as.i;
    }

    if (bl_value_is_string(value)) {
        BLString *string = bl_value_as_string(value);
        char *end = NULL;
        long long result = strtoll(string->data, &end, 10);
        if (ok) *ok = (end && *end == '\0');
        return result;
    }

    if (ok) *ok = 0;
    return 0;
}

int bl_value_compare_eq(BLValue left, BLValue right) {
    if (left.tag != right.tag) return 0;
    if (left.tag == BLV_NULL) return 1;
    if (left.tag == BLV_INT) return left.as.i == right.as.i;
    if (left.as.obj == right.as.obj) return 1;

    if (bl_value_is_string(left) && bl_value_is_string(right)) {
        BLString *a = bl_value_as_string(left);
        BLString *b = bl_value_as_string(right);
        if (a->size != b->size) return 0;
        return memcmp(a->data, b->data, a->size) == 0;
    }

    return 0;
}
