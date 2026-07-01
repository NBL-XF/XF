#include "../include/value.h"

#include "../include/err.h"
#include "../include/symTable.h"

#include <math.h>
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

static int render_value(StrBuf *buf, NameStack *env, const Value *value, int parent_prec);
static int sb_append(StrBuf *buf, const char *text);
static int sb_append_n(StrBuf *buf, const char *text, size_t length);

static int value_is_scott_nil_shape(const Value *value) {
	return value != NULL &&
	       value->kind == VALUE_LAM &&
	       value->as.lam.body != NULL &&
	       value->as.lam.body->kind == VALUE_LAM &&
	       value->as.lam.body->as.lam.body != NULL &&
	       value->as.lam.body->as.lam.body->kind == VALUE_BOUND_VAR &&
	       value->as.lam.body->as.lam.body->as.bound_var.index == 0;
}

static int value_is_scott_cons_shape(const Value *value, const Value **out_head, const Value **out_tail) {
	const Value *body;
	const Value *apply_c;

	if (value == NULL ||
	    value->kind != VALUE_LAM ||
	    value->as.lam.body == NULL ||
	    value->as.lam.body->kind != VALUE_LAM ||
	    value->as.lam.body->as.lam.body == NULL) {
		return 0;
	}

	body = value->as.lam.body->as.lam.body;
	if (body->kind != VALUE_APP ||
	    body->as.app.fn == NULL ||
	    body->as.app.fn->kind != VALUE_APP ||
	    body->as.app.fn->as.app.fn == NULL ||
	    body->as.app.fn->as.app.fn->kind != VALUE_BOUND_VAR ||
	    body->as.app.fn->as.app.fn->as.bound_var.index != 1) {
		return 0;
	}

	apply_c = body->as.app.fn;
	if (out_head != NULL) {
		*out_head = apply_c->as.app.arg;
	}
	if (out_tail != NULL) {
		*out_tail = body->as.app.arg;
	}
	return 1;
}

static int render_scott_list(StrBuf *buf, NameStack *env, const Value *value) {
	const Value *head = NULL;
	const Value *tail = NULL;

	if (value_is_scott_nil_shape(value)) {
		return sb_append(buf, "nil");
	}

	if (!value_is_scott_cons_shape(value, &head, &tail)) {
		return 0;
	}

	if (!sb_append(buf, "cons ")) {
		return 0;
	}
	if (!render_value(buf, env, head, 3)) {
		return 0;
	}
	if (!sb_append(buf, " ")) {
		return 0;
	}
	if (!sb_append(buf, "(")) {
		return 0;
	}
	if (value_is_scott_nil_shape(tail) || value_is_scott_cons_shape(tail, NULL, NULL)) {
		if (!render_scott_list(buf, env, tail)) {
			return 0;
		}
	} else {
		if (!render_value(buf, env, tail, 0)) {
			return 0;
		}
	}
	return sb_append(buf, ")");
}

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

static int sb_append_double(StrBuf *buf, double value) {
	char tmp[128];
	int written;

	if (isinf(value)) {
		return sb_append(buf, value < 0.0 ? "-∞" : "∞");
	}

	if (isnan(value)) {
		return sb_append(buf, "nan");
	}

	written = snprintf(tmp, sizeof(tmp), "%.15g", value);
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

static char *name_for_depth(size_t depth) {
	char tmp[64];
	int written = snprintf(tmp, sizeof(tmp), "x%zu", depth);
	if (written < 0 || (size_t)written >= sizeof(tmp)) {
		return NULL;
	}
	return value_strdup(tmp);
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

Value *value_number_new(double number) {
	Value *value = (Value *)calloc(1, sizeof(*value));
	if (value == NULL) {
		return NULL;
	}

	value->kind = VALUE_NUMBER;
	value->as.number.value = number;
	return value;
}

Value *value_clone(const Value *value) {
	Value *body;
	Value *fn;
	Value *arg;

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

		return value_app_new(fn, arg);

	case VALUE_NUMBER:
		return value_number_new(value->as.number.value);
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
	case VALUE_NUMBER:
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
	case VALUE_NUMBER:
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


static int utf8_decode_one(const unsigned char *s, size_t len, unsigned int *cp, size_t *used) {
	if (len == 0) {
		return 0;
	}

	if (s[0] < 0x80) {
		*cp = s[0];
		*used = 1;
		return 1;
	}

	if ((s[0] & 0xE0) == 0xC0 && len >= 2) {
		*cp = ((unsigned int)(s[0] & 0x1F) << 6) |
		      (unsigned int)(s[1] & 0x3F);
		*used = 2;
		return 1;
	}

	if ((s[0] & 0xF0) == 0xE0 && len >= 3) {
		*cp = ((unsigned int)(s[0] & 0x0F) << 12) |
		      ((unsigned int)(s[1] & 0x3F) << 6) |
		      (unsigned int)(s[2] & 0x3F);
		*used = 3;
		return 1;
	}

	if ((s[0] & 0xF8) == 0xF0 && len >= 4) {
		*cp = ((unsigned int)(s[0] & 0x07) << 18) |
		      ((unsigned int)(s[1] & 0x3F) << 12) |
		      ((unsigned int)(s[2] & 0x3F) << 6) |
		      (unsigned int)(s[3] & 0x3F);
		*used = 4;
		return 1;
	}

	return 0;
}

static int decode_subscript_codepoint(unsigned int cp, char *out) {
	switch (cp) {
	case 0x2080: *out = '0'; return 1;
	case 0x2081: *out = '1'; return 1;
	case 0x2082: *out = '2'; return 1;
	case 0x2083: *out = '3'; return 1;
	case 0x2084: *out = '4'; return 1;
	case 0x2085: *out = '5'; return 1;
	case 0x2086: *out = '6'; return 1;
	case 0x2087: *out = '7'; return 1;
	case 0x2088: *out = '8'; return 1;
	case 0x2089: *out = '9'; return 1;
	case 0x2090: *out = 'a'; return 1; /* ₐ */
	case 0x2091: *out = 'e'; return 1; /* ₑ */
	case 0x2095: *out = 'h'; return 1; /* ₕ */
	case 0x1D62: *out = 'i'; return 1; /* ᵢ */
	case 0x2C7C: *out = 'j'; return 1; /* ⱼ */
	case 0x2096: *out = 'k'; return 1; /* ₖ */
	case 0x2097: *out = 'l'; return 1; /* ₗ */
	case 0x2098: *out = 'm'; return 1; /* ₘ */
	case 0x2099: *out = 'n'; return 1; /* ₙ */
	case 0x2092: *out = 'o'; return 1; /* ₒ */
	case 0x209A: *out = 'p'; return 1; /* ₚ */
	case 0x1D63: *out = 'r'; return 1; /* ᵣ */
	case 0x209B: *out = 's'; return 1; /* ₛ */
	case 0x209C: *out = 't'; return 1; /* ₜ */
	case 0x1D64: *out = 'u'; return 1; /* ᵤ */
	case 0x1D65: *out = 'v'; return 1; /* ᵥ */
	case 0x2093: *out = 'x'; return 1; /* ₓ */
	default:
		return 0;
	}
}

static int decode_subscript_string(const char *name, char *out, size_t out_cap) {
	const unsigned char *p = (const unsigned char *)name;
	size_t len = strlen(name);
	size_t pos = 0;
	size_t out_len = 0;

	while (pos < len) {
		unsigned int cp;
		size_t used;
		if (!utf8_decode_one(p + pos, len - pos, &cp, &used)) {
			return 0;
		}
		if (out_len + 1 >= out_cap || !decode_subscript_codepoint(cp, &out[out_len])) {
			return 0;
		}
		out_len++;
		pos += used;
	}

	if (out_len == 0) {
		return 0;
	}

	out[out_len] = '\0';
	return 1;
}

static int name_has_unicode_subscript_suffix(const char *name, size_t *base_len, char *decoded, size_t decoded_cap) {
	const unsigned char *p = (const unsigned char *)name;
	size_t len = strlen(name);
	size_t pos = 0;
	size_t first_sub = len;

	while (pos < len) {
		unsigned int cp;
		size_t used;
		char ch;
		if (!utf8_decode_one(p + pos, len - pos, &cp, &used)) {
			return 0;
		}
		if (decode_subscript_codepoint(cp, &ch)) {
			first_sub = pos;
			break;
		}
		pos += used;
	}

	if (first_sub == len || first_sub == 0) {
		return 0;
	}

	if (!decode_subscript_string(name + first_sub, decoded, decoded_cap)) {
		return 0;
	}

	*base_len = first_sub;
	return 1;
}

static int is_ascii_digits_text(const char *text) {
	size_t i;
	if (text == NULL || text[0] == '\0') {
		return 0;
	}
	for (i = 0; text[i] != '\0'; i++) {
		if (!(text[i] >= '0' && text[i] <= '9')) {
			return 0;
		}
	}
	return 1;
}

static int is_simple_ascii_sub_index(const char *suffix) {
	size_t len;
	if (suffix == NULL || suffix[0] == '\0') {
		return 0;
	}
	len = strlen(suffix);
	if (is_ascii_digits_text(suffix)) {
		return 1;
	}
	return len == 1 && ((suffix[0] >= 'A' && suffix[0] <= 'Z') ||
	                    (suffix[0] >= 'a' && suffix[0] <= 'z'));
}

static Value *value_app_take(Value *fn, Value *arg) {
	Value *app = value_app_new(fn, arg);
	if (app == NULL) {
		value_free(fn);
		value_free(arg);
	}
	return app;
}

static Value *lower_name_reference(const char *name, SymTable *env, const DefValue *defs);

static Value *build_sub_reference(const char *base_name, const char *index_name, SymTable *env, const DefValue *defs) {
	Value *sub_ref = value_free_var_new("sub");
	Value *base_ref;
	Value *index_ref;
	Value *partial;

	if (sub_ref == NULL) {
		err_set("out of memory");
		return NULL;
	}

	base_ref = lower_name_reference(base_name, env, defs);
	if (base_ref == NULL) {
		value_free(sub_ref);
		return NULL;
	}

	partial = value_app_take(sub_ref, base_ref);
	if (partial == NULL) {
		err_set("out of memory");
		return NULL;
	}

	if (is_ascii_digits_text(index_name)) {
		index_ref = value_number_new(strtod(index_name, NULL));
	} else {
		index_ref = lower_name_reference(index_name, env, defs);
	}
	if (index_ref == NULL) {
		value_free(partial);
		return NULL;
	}

	partial = value_app_take(partial, index_ref);
	if (partial == NULL) {
		err_set("out of memory");
		return NULL;
	}

	return partial;
}

static Value *try_lower_subscript_name(const char *name, SymTable *env, const DefValue *defs) {
	char decoded[128];
	size_t base_len;
	char *base = NULL;
	char *suffix = NULL;
	const char *underscore;

	if (name == NULL || name[0] == '\0') {
		return NULL;
	}

	if (decode_subscript_string(name, decoded, sizeof(decoded))) {
		if (is_ascii_digits_text(decoded)) {
			return value_number_new(strtod(decoded, NULL));
		}
		return lower_name_reference(decoded, env, defs);
	}

	if (name_has_unicode_subscript_suffix(name, &base_len, decoded, sizeof(decoded))) {
		base = (char *)malloc(base_len + 1);
		if (base == NULL) {
			err_set("out of memory");
			return NULL;
		}
		memcpy(base, name, base_len);
		base[base_len] = '\0';
		Value *result = build_sub_reference(base, decoded, env, defs);
		free(base);
		return result;
	}

	underscore = strrchr(name, '_');
	if (underscore != NULL && underscore != name && underscore[1] != '\0' && strchr(underscore + 1, '_') == NULL) {
		suffix = value_strdup(underscore + 1);
		if (suffix == NULL) {
			err_set("out of memory");
			return NULL;
		}
		if (is_simple_ascii_sub_index(suffix)) {
			base = (char *)malloc((size_t)(underscore - name) + 1);
			if (base == NULL) {
				free(suffix);
				err_set("out of memory");
				return NULL;
			}
			memcpy(base, name, (size_t)(underscore - name));
			base[underscore - name] = '\0';
			Value *result = build_sub_reference(base, suffix, env, defs);
			free(base);
			free(suffix);
			return result;
		}
		free(suffix);
	}

	return NULL;
}

static Value *lower_name_reference(const char *name, SymTable *env, const DefValue *defs) {
	size_t index;
	Value *resolved;
	Value *special;

	if (sym_table_lookup(env, name, &index)) {
		return value_bound_var_new(index);
	}

	resolved = def_values_lookup(defs, name);
	if (resolved != NULL) {
		return value_clone(resolved);
	}

	special = try_lower_subscript_name(name, env, defs);
	if (special != NULL) {
		return special;
	}

	return value_free_var_new(name);
}


static Value *lower_ast_with_defs(const Ast *ast, SymTable *env, const DefValue *defs) {
	Value *body;
	Value *fn;
	Value *arg;

	switch (ast->kind) {
	case AST_VAR:
		return lower_name_reference(ast->as.var.name, env, defs);

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

		return value_app_new(fn, arg);

	case AST_NUMBER:
		return value_number_new(ast->as.number.value);
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

Value *value_from_program(const Program *program) {
	SymTable env;
	DefValue *defs = NULL;
	const AstDef *def;
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

	unresolved_name = find_unresolved_def_ref(result, program);
	if (unresolved_name != NULL) {
		err_set("unresolved forward or mutual definition reference '%s'", unresolved_name);
		value_free(result);
		result = NULL;
	}

done:
	sym_table_free(&env);
	def_values_free(defs);

	if (result == NULL && !err_has()) {
		err_set("failed to lower program");
	}

	return result;
}

static int render_value(StrBuf *buf, NameStack *env, const Value *value, int parent_prec) {
	int need_parens;
	size_t env_index;

	if (value_is_scott_cons_shape(value, NULL, NULL)) {
		return render_scott_list(buf, env, value);
	}

	switch (value->kind) {
	case VALUE_BOUND_VAR:
		if (value->as.bound_var.index >= env->count) {
			return sb_appendf(buf, "v%d", (int)value->as.bound_var.index);
		}

		env_index = env->count - 1 - value->as.bound_var.index;
		return sb_append(buf, env->items[env_index]);

	case VALUE_FREE_VAR:
		return sb_append(buf, value->as.free_var.name);

	case VALUE_NUMBER:
		return sb_append_double(buf, value->as.number.value);

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
			char *name = name_for_depth(env->count);

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
