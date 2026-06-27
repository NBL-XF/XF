#ifndef LS_VALUE_H
#define LS_VALUE_H

#include <stddef.h>

#include "../include/ast.h"

typedef enum {
	VALUE_BOUND_VAR,
	VALUE_FREE_VAR,
	VALUE_LAM,
	VALUE_APP,
	VALUE_NUMBER
} ValueKind;

typedef struct Value Value;

struct Value {
	ValueKind kind;
	union {
		struct {
			size_t index;
		} bound_var;
		struct {
			char *name;
		} free_var;
		struct {
			Value *body;
		} lam;
		struct {
			Value *fn;
			Value *arg;
		} app;
		struct {
			double value;
		} number;
	} as;
};

Value *value_bound_var_new(size_t index);
Value *value_free_var_new(const char *name);
Value *value_lam_new(Value *body);
Value *value_app_new(Value *fn, Value *arg);
Value *value_number_new(double value);
Value *value_church_boolean_new(int truthy);
int value_is_church_boolean(const Value *value, int *out_truth);
Value *value_scott_nil_new(void);
Value *value_scott_cons_take(Value *head, Value *tail);
int value_match_scott_list(const Value *value, int *is_nil_out, const Value **head_out, const Value **tail_out);
Value *value_clone(const Value *value);
void value_free(Value *value);

Value *value_from_ast(const Ast *ast);
Value *value_from_program(const Program *program);
char *value_to_string(const Value *value);

#endif
