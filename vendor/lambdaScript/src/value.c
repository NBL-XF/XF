#include "../include/value.h"

#include "../include/err.h"
#include "../include/symTable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char *data;
	size_t length;
	size_t capacity;
} StrBuf;

typedef struct {
	char **items;
	size_t count;
	size_t capacity;
} NameStack;

typedef struct DefValue {
	char *name;
	Value *value;
	struct DefValue *next;
} DefValue;

static char *value_strdup(const char *src) {
	size_t len;
	char *dst;

	if (src == NULL) {
		return NULL;
	}

	len = strlen(src);
	dst = (char *)malloc(len + 1);
	if (dst == NULL) {
		return NULL;
	}

	memcpy(dst, src, len + 1);
	return dst;
}

static int sb_reserve(StrBuf *buf, size_t needed) {
	char *new_data;
	size_t new_capacity = buf->capacity == 0 ? 64 : buf->capacity;

	while (new_capacity < needed) {
		new_capacity *= 2;
	}

	new_data = (char *)realloc(buf->data, new_capacity);
	if (new_data == NULL) {
		return 0;
	}

	buf->data = new_data;
	buf->capacity = new_capacity;
	return 1;
}

static int sb_append_n(StrBuf *buf, const char *text, size_t length) {
	if (!sb_reserve(buf, buf->length + length + 1)) {
		return 0;
	}

	memcpy(buf->data + buf->length, text, length);
	buf->length += length;
	buf->data[buf->length] = '\0';
	return 1;
}

static int sb_append(StrBuf *buf, const char *text) {
	return sb_append_n(buf, text, strlen(text));
}

static int sb_appendf(StrBuf *buf, const char *fmt, int n) {
	char tmp[64];
	int written = snprintf(tmp, sizeof(tmp), fmt, n);

	if (written < 0) {
		return 0;
	}

	return sb_append_n(buf, tmp, (size_t)written);
}

static void name_stack_init(NameStack *stack) {
	stack->items = NULL;
	stack->count = 0;
	stack->capacity = 0;
}

static void name_stack_free(NameStack *stack) {
	size_t i;

	for (i = 0; i < stack->count; i++) {
		free(stack->items[i]);
	}

	free(stack->items);
	stack->items = NULL;
	stack->count = 0;
	stack->capacity = 0;
}

static int name_stack_push_owned(NameStack *stack, char *name) {
	char **new_items;
	size_t new_capacity;

	if (stack->count == stack->capacity) {
		new_capacity = stack->capacity == 0 ? 8 : stack->capacity * 2;
		new_items = (char **)realloc(stack->items, new_capacity * sizeof(*new_items));
		if (new_items == NULL) {
			free(name);
			return 0;
		}
		stack->items = new_items;
		stack->capacity = new_capacity;
	}

	stack->items[stack->count++] = name;
	return 1;
}

static void name_stack_pop(NameStack *stack) {
	if (stack->count == 0) {
		return;
	}

	stack->count--;
	free(stack->items[stack->count]);
	stack->items[stack->count] = NULL;
}

static void def_values_free(DefValue *defs) {
	DefValue *next;

	while (defs != NULL) {
		next = defs->next;
		free(defs->name);
		value_free(defs->value);
		free(defs);
		defs = next;
	}
}

static Value *def_values_lookup(const DefValue *defs, const char *name) {
	while (defs != NULL) {
		if (strcmp(defs->name, name) == 0) {
			return defs->value;
		}
		defs = defs->next;
	}
	return NULL;
}

static int def_values_add(DefValue **defs, const char *name, Value *value) {
	DefValue *node = (DefValue *)calloc(1, sizeof(*node));
	if (node == NULL) {
		return 0;
	}

	node->name = value_strdup(name);
	node->value = value;
	node->next = *defs;

	if (node->name == NULL) {
		free(node);
		return 0;
	}

	*defs = node;
	return 1;
}

static int program_has_def_name(const Program *program, const char *name) {
	const AstDef *def;

	for (def = program->defs; def != NULL; def = def->next) {
		if (strcmp(def->name, name) == 0) {
			return 1;
		}
	}

	return 0;
}

static const char *find_unresolved_def_ref(const Value *value, const Program *program) {
	const char *name;

	if (value == NULL) {
		return NULL;
	}

	switch (value->kind) {
	case VALUE_BOUND_VAR:
		return NULL;

	case VALUE_FREE_VAR:
		return program_has_def_name(program, value->as.free_var.name) ? value->as.free_var.name : NULL;

	case VALUE_LAM:
		return find_unresolved_def_ref(value->as.lam.body, program);

	case VALUE_APP:
		name = find_unresolved_def_ref(value->as.app.fn, program);
		if (name != NULL) {
			return name;
		}
		return find_unresolved_def_ref(value->as.app.arg, program);
	}

	return NULL;
}

static char *name_for_depth(size_t depth) {
	char reversed[64];
	size_t length = 0;
	size_t value = depth;
	char *name;
	size_t i;

	do {
		if (length + 1 >= sizeof(reversed)) {
			return NULL;
		}
		reversed[length++] = (char)('a' + (value % 26));
		if (value < 26) {
			break;
		}
		value = (value / 26) - 1;
	} while (1);

	name = (char *)malloc(length + 1);
	if (name == NULL) {
		return NULL;
	}

	for (i = 0; i < length; i++) {
		name[i] = reversed[length - 1 - i];
	}
	name[length] = '\0';
	return name;
}

Value *value_bound_var_new(size_t index) {
	Value *value = (Value *)calloc(1, sizeof(*value));
	if (value == NULL) {
		return NULL;
	}

	value->kind = VALUE_BOUND_VAR;
	value->as.bound_var.index = index;
	return value;
}

Value *value_free_var_new(const char *name) {
	Value *value = (Value *)calloc(1, sizeof(*value));
	if (value == NULL) {
		return NULL;
	}

	value->kind = VALUE_FREE_VAR;
	value->as.free_var.name = value_strdup(name);
	if (value->as.free_var.name == NULL) {
		free(value);
		return NULL;
	}

	return value;
}

Value *value_lam_new(Value *body) {
	Value *value;

	if (body == NULL) {
		return NULL;
	}

	value = (Value *)calloc(1, sizeof(*value));
	if (value == NULL) {
		return NULL;
	}

	value->kind = VALUE_LAM;
	value->as.lam.body = body;
	return value;
}

Value *value_app_new(Value *fn, Value *arg) {
	Value *value;

	if (fn == NULL || arg == NULL) {
		return NULL;
	}

	value = (Value *)calloc(1, sizeof(*value));
	if (value == NULL) {
		return NULL;
	}

	value->kind = VALUE_APP;
	value->as.app.fn = fn;
	value->as.app.arg = arg;
	return value;
}

Value *value_clone(const Value *value) {
	Value *body;
	Value *fn;
	Value *arg;
	Value *app;

	if (value == NULL) {
		return NULL;
	}

	switch (value->kind) {
	case VALUE_BOUND_VAR:
		return value_bound_var_new(value->as.bound_var.index);

	case VALUE_FREE_VAR:
		return value_free_var_new(value->as.free_var.name);

	case VALUE_LAM:
		body = value_clone(value->as.lam.body);
		if (body == NULL) {
			return NULL;
		}
		return value_lam_new(body);

	case VALUE_APP:
		fn = value_clone(value->as.app.fn);
		if (fn == NULL) {
			return NULL;
		}

		arg = value_clone(value->as.app.arg);
		if (arg == NULL) {
			value_free(fn);
			return NULL;
		}

		app = value_app_new(fn, arg);
		if (app == NULL) {
			value_free(fn);
			value_free(arg);
			return NULL;
		}
		return app;
	}

	err_set("unknown value node");
	return NULL;
}

void value_free(Value *value) {
	if (value == NULL) {
		return;
	}

	switch (value->kind) {
	case VALUE_BOUND_VAR:
		break;
	case VALUE_FREE_VAR:
		free(value->as.free_var.name);
		break;
	case VALUE_LAM:
		value_free(value->as.lam.body);
		break;
	case VALUE_APP:
		value_free(value->as.app.fn);
		value_free(value->as.app.arg);
		break;
	}

	free(value);
}

static Value *lower_ast_with_defs(const Ast *ast, SymTable *env, const DefValue *defs) {
	size_t index;
	Value *resolved;
	Value *body;
	Value *fn;
	Value *arg;
	Value *app;

	switch (ast->kind) {
	case AST_VAR:
		if (sym_table_lookup(env, ast->as.var.name, &index)) {
			return value_bound_var_new(index);
		}

		resolved = def_values_lookup(defs, ast->as.var.name);
		if (resolved != NULL) {
			return value_clone(resolved);
		}

		return value_free_var_new(ast->as.var.name);

	case AST_LAM:
		if (!sym_table_push(env, ast->as.lam.param)) {
			err_set("out of memory");
			return NULL;
		}

		body = lower_ast_with_defs(ast->as.lam.body, env, defs);
		sym_table_pop(env);

		if (body == NULL) {
			return NULL;
		}

		return value_lam_new(body);

	case AST_APP:
		fn = lower_ast_with_defs(ast->as.app.fn, env, defs);
		if (fn == NULL) {
			return NULL;
		}

		arg = lower_ast_with_defs(ast->as.app.arg, env, defs);
		if (arg == NULL) {
			value_free(fn);
			return NULL;
		}

		app = value_app_new(fn, arg);
		if (app == NULL) {
			value_free(fn);
			value_free(arg);
			err_set("out of memory");
			return NULL;
		}
		return app;
	}

	err_set("unknown AST node");
	return NULL;
}

Value *value_from_ast(const Ast *ast) {
	SymTable env;
	Value *result;

	err_clear();
	sym_table_init(&env);
	result = lower_ast_with_defs(ast, &env, NULL);
	sym_table_free(&env);

	if (result == NULL && !err_has()) {
		err_set("failed to lower AST");
	}

	return result;
}

/* value.c */

static int program_has_definition_named(const Program *program, const char *name) {
	AstDef *def;

	if (program == NULL || name == NULL) {
		return 0;
	}

	for (def = program->defs; def != NULL; def = def->next) {
		if (strcmp(def->name, name) == 0) {
			return 1;
		}
	}

	return 0;
}

static const char *find_unresolved_definition_reference(const Value *value, const Program *program) {
	const char *name;

	if (value == NULL) {
		return NULL;
	}

	switch (value->kind) {
	case VALUE_BOUND_VAR:
		return NULL;

	case VALUE_FREE_VAR:
		if (program_has_definition_named(program, value->as.free_var.name)) {
			return value->as.free_var.name;
		}
		return NULL;

	case VALUE_LAM:
		return find_unresolved_definition_reference(value->as.lam.body, program);

	case VALUE_APP:
		name = find_unresolved_definition_reference(value->as.app.fn, program);
		if (name != NULL) {
			return name;
		}
		return find_unresolved_definition_reference(value->as.app.arg, program);
	}

	return NULL;
}

/* value.c */

Value *value_from_program(const Program *program) {
	SymTable env;
	DefValue *defs = NULL;
	AstDef *def;
	Value *result = NULL;
	const char *unresolved_name = NULL;

	if (program == NULL || program->expr == NULL) {
		err_set("invalid program");
		return NULL;
	}

	err_clear();
	sym_table_init(&env);

	for (def = program->defs; def != NULL; def = def->next) {
		Value *lowered = lower_ast_with_defs(def->expr, &env, defs);
		if (lowered == NULL) {
			goto done;
		}

		if (!def_values_add(&defs, def->name, lowered)) {
			value_free(lowered);
			err_set("out of memory");
			goto done;
		}
	}

	result = lower_ast_with_defs(program->expr, &env, defs);
	if (result == NULL) {
		goto done;
	}

	unresolved_name = find_unresolved_definition_reference(result, program);
	if (unresolved_name != NULL) {
		err_set("unresolved forward or mutual definition reference '%s'", unresolved_name);
		value_free(result);
		result = NULL;
		goto done;
	}

done:
	sym_table_free(&env);
	def_values_free(defs);

	if (result == NULL && !err_has()) {
		err_set("failed to lower program");
	}
	return result;
}
static char *render_name_for_depth(size_t depth) {
	char tmp[64];
	int written = snprintf(tmp, sizeof(tmp), "x%zu", depth);
	if (written < 0 || (size_t)written >= sizeof(tmp)) {
		return NULL;
	}
	return value_strdup(tmp);
}

static int render_value(StrBuf *buf, NameStack *env, const Value *value, int parent_prec) {
	int need_parens;
	size_t env_index;

	switch (value->kind) {
	case VALUE_BOUND_VAR:
		if (value->as.bound_var.index >= env->count) {
			return sb_appendf(buf, "v%d", (int)value->as.bound_var.index);
		}

		env_index = env->count - 1 - value->as.bound_var.index;
		return sb_append(buf, env->items[env_index]);

	case VALUE_FREE_VAR:
		return sb_append(buf, value->as.free_var.name);

	case VALUE_LAM: {
		const Value *current = value;
		size_t pushed = 0;

		need_parens = parent_prec > 1;
		if (need_parens && !sb_append(buf, "(")) {
			return 0;
		}

		if (!sb_append(buf, "\\")) {
			return 0;
		}

		while (current->kind == VALUE_LAM) {
			char *name = render_name_for_depth(env->count);

			if (name == NULL) {
				while (pushed > 0) {
					name_stack_pop(env);
					pushed--;
				}
				return 0;
			}

			if (pushed > 0 && !sb_append(buf, " ")) {
				free(name);
				while (pushed > 0) {
					name_stack_pop(env);
					pushed--;
				}
				return 0;
			}

			if (!sb_append(buf, name)) {
				free(name);
				while (pushed > 0) {
					name_stack_pop(env);
					pushed--;
				}
				return 0;
			}

			if (!name_stack_push_owned(env, name)) {
				free(name);
				while (pushed > 0) {
					name_stack_pop(env);
					pushed--;
				}
				return 0;
			}

			pushed++;
			current = current->as.lam.body;
		}

		if (!sb_append(buf, ".")) {
			while (pushed > 0) {
				name_stack_pop(env);
				pushed--;
			}
			return 0;
		}

		if (!render_value(buf, env, current, 1)) {
			while (pushed > 0) {
				name_stack_pop(env);
				pushed--;
			}
			return 0;
		}

		while (pushed > 0) {
			name_stack_pop(env);
			pushed--;
		}

		if (need_parens && !sb_append(buf, ")")) {
			return 0;
		}
		return 1;
	}

	case VALUE_APP:
		need_parens = parent_prec > 2;
		if (need_parens && !sb_append(buf, "(")) {
			return 0;
		}
		if (!render_value(buf, env, value->as.app.fn, 2)) {
			return 0;
		}
		if (!sb_append(buf, " ")) {
			return 0;
		}
		if (!render_value(buf, env, value->as.app.arg, 3)) {
			return 0;
		}
		if (need_parens && !sb_append(buf, ")")) {
			return 0;
		}
		return 1;
	}

	return 0;
}

char *value_to_string(const Value *value) {
	StrBuf buf = {0};
	NameStack env;

	name_stack_init(&env);

	if (!render_value(&buf, &env, value, 0)) {
		name_stack_free(&env);
		free(buf.data);
		err_set("out of memory");
		return NULL;
	}

	name_stack_free(&env);

	if (buf.data == NULL) {
		buf.data = value_strdup("");
		if (buf.data == NULL) {
			err_set("out of memory");
			return NULL;
		}
	}

	return buf.data;
}