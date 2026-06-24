#include "../include/interp.h"

#include "../include/err.h"

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static Value *shift_walk(const Value *term, int delta, size_t cutoff) {
	long long shifted;
	Value *body;
	Value *fn;
	Value *arg;

	switch (term->kind) {
	case VALUE_BOUND_VAR:
		if (term->as.bound_var.index >= cutoff) {
			shifted = (long long)term->as.bound_var.index + delta;
			if (shifted < 0 || shifted > LLONG_MAX) {
				err_set("invalid de Bruijn shift");
				return NULL;
			}
			return value_bound_var_new((size_t)shifted);
		}
		return value_bound_var_new(term->as.bound_var.index);

	case VALUE_FREE_VAR:
		return value_free_var_new(term->as.free_var.name);

	case VALUE_LAM:
		body = shift_walk(term->as.lam.body, delta, cutoff + 1);
		if (body == NULL) {
			return NULL;
		}
		return value_lam_new(body);

	case VALUE_APP:
		fn = shift_walk(term->as.app.fn, delta, cutoff);
		if (fn == NULL) {
			return NULL;
		}

		arg = shift_walk(term->as.app.arg, delta, cutoff);
		if (arg == NULL) {
			value_free(fn);
			return NULL;
		}

		return value_app_new(fn, arg);

	case VALUE_NUMBER:
		return value_number_new(term->as.number.value);
	}

	err_set("unknown value node");
	return NULL;
}

static Value *value_shift(const Value *term, int delta) {
	return shift_walk(term, delta, 0);
}

static Value *subst_walk(const Value *term, size_t depth, size_t target, const Value *replacement) {
	Value *body;
	Value *fn;
	Value *arg;

	switch (term->kind) {
	case VALUE_BOUND_VAR:
		if (term->as.bound_var.index == target + depth) {
			return shift_walk(replacement, (int)depth, 0);
		}
		return value_bound_var_new(term->as.bound_var.index);

	case VALUE_FREE_VAR:
		return value_free_var_new(term->as.free_var.name);

	case VALUE_LAM:
		body = subst_walk(term->as.lam.body, depth + 1, target, replacement);
		if (body == NULL) {
			return NULL;
		}
		return value_lam_new(body);

	case VALUE_APP:
		fn = subst_walk(term->as.app.fn, depth, target, replacement);
		if (fn == NULL) {
			return NULL;
		}

		arg = subst_walk(term->as.app.arg, depth, target, replacement);
		if (arg == NULL) {
			value_free(fn);
			return NULL;
		}

		return value_app_new(fn, arg);

	case VALUE_NUMBER:
		return value_number_new(term->as.number.value);
	}

	err_set("unknown value node");
	return NULL;
}

static Value *value_subst(const Value *term, size_t target, const Value *replacement) {
	return subst_walk(term, 0, target, replacement);
}

static Value *beta_reduce(const Value *body, const Value *arg) {
	Value *arg_up;
	Value *substituted;
	Value *result;

	arg_up = value_shift(arg, 1);
	if (arg_up == NULL) {
		return NULL;
	}

	substituted = value_subst(body, 0, arg_up);
	value_free(arg_up);
	if (substituted == NULL) {
		return NULL;
	}

	result = value_shift(substituted, -1);
	value_free(substituted);
	return result;
}


static int value_structural_equal(const Value *left, const Value *right) {
	if (left == right) {
		return 1;
	}
	if (left == NULL || right == NULL) {
		return 0;
	}
	if (left->kind != right->kind) {
		return 0;
	}

	switch (left->kind) {
	case VALUE_BOUND_VAR:
		return left->as.bound_var.index == right->as.bound_var.index;

	case VALUE_FREE_VAR:
		return strcmp(left->as.free_var.name, right->as.free_var.name) == 0;

	case VALUE_LAM:
		return value_structural_equal(left->as.lam.body, right->as.lam.body);

	case VALUE_APP:
		return value_structural_equal(left->as.app.fn, right->as.app.fn) &&
		       value_structural_equal(left->as.app.arg, right->as.app.arg);

	case VALUE_NUMBER:
		if (isnan(left->as.number.value) && isnan(right->as.number.value)) {
			return 1;
		}
		return left->as.number.value == right->as.number.value;
	}

	return 0;
}

static Value *value_church_boolean(int truthy) {
	Value *branch;
	Value *inner;

	branch = value_bound_var_new(truthy ? 1 : 0);
	if (branch == NULL) {
		return NULL;
	}

	inner = value_lam_new(branch);
	if (inner == NULL) {
		value_free(branch);
		return NULL;
	}

	return value_lam_new(inner);
}

static int is_equiv_name(const char *name) {
	return strcmp(name, "EQUIV") == 0 ||
	       strcmp(name, "equiv") == 0 ||
	       strcmp(name, "EQ") == 0 ||
	       strcmp(name, "eq") == 0;
}

static Value *apply_binary_primitive(const char *name, double left, double right) {
	if (strcmp(name, "ADD") == 0) {
		return value_number_new(left + right);
	}
	if (strcmp(name, "SUB") == 0) {
		return value_number_new(left - right);
	}
	if (strcmp(name, "MUL") == 0) {
		return value_number_new(left * right);
	}
	if (strcmp(name, "DIV") == 0) {
		return value_number_new(left / right);
	}
	if (strcmp(name, "MOD") == 0) {
		return value_number_new(fmod(left, right));
	}
	if (strcmp(name, "POW") == 0) {
		return value_number_new(pow(left, right));
	}
	return NULL;
}

static Value *apply_unary_primitive(const char *name, double value) {
	if (strcmp(name, "NEG") == 0) {
		return value_number_new(-value);
	}
	if (strcmp(name, "SQRT") == 0) {
		return value_number_new(sqrt(value));
	}
	if (strcmp(name, "LN") == 0) {
		return value_number_new(log(value));
	}
	return NULL;
}

static Value *try_reduce_primitive(const Value *term) {
	const Value *head;
	const Value *left;
	const Value *right;

	if (term->kind != VALUE_APP) {
		return NULL;
	}

	head = term->as.app.fn;
	right = term->as.app.arg;

	if (head->kind == VALUE_APP && head->as.app.fn->kind == VALUE_FREE_VAR) {
		if (is_equiv_name(head->as.app.fn->as.free_var.name)) {
			return value_church_boolean(value_structural_equal(head->as.app.arg, right));
		}

		if (head->as.app.arg->kind == VALUE_NUMBER && right->kind == VALUE_NUMBER) {
			return apply_binary_primitive(
				head->as.app.fn->as.free_var.name,
				head->as.app.arg->as.number.value,
				right->as.number.value
			);
		}
	}

	if (head->kind == VALUE_FREE_VAR && right->kind == VALUE_NUMBER) {
		return apply_unary_primitive(head->as.free_var.name, right->as.number.value);
	}

	left = head;
	if (left->kind == VALUE_FREE_VAR && strcmp(left->as.free_var.name, "INF") == 0) {
		return value_number_new(INFINITY);
	}

	return NULL;
}

Value *interp_step_normal(const Value *term) {
	Value *primitive_step;
	Value *body_step;
	Value *fn_step;
	Value *arg_step;
	Value *fn_clone;

	switch (term->kind) {
	case VALUE_BOUND_VAR:
	case VALUE_FREE_VAR:
	case VALUE_NUMBER:
		return NULL;

	case VALUE_LAM:
		body_step = interp_step_normal(term->as.lam.body);
		if (body_step == NULL) {
			return NULL;
		}
		return value_lam_new(body_step);

	case VALUE_APP:
		if (term->as.app.fn->kind == VALUE_LAM) {
			return beta_reduce(term->as.app.fn->as.lam.body, term->as.app.arg);
		}

		primitive_step = try_reduce_primitive(term);
		if (primitive_step != NULL) {
			return primitive_step;
		}

		err_clear();
		fn_step = interp_step_normal(term->as.app.fn);
		if (fn_step != NULL) {
			Value *app = value_app_new(fn_step, value_clone(term->as.app.arg));
			if (app == NULL) {
				value_free(fn_step);
				err_set("out of memory");
				return NULL;
			}
			return app;
		}
		if (err_has()) {
			return NULL;
		}

		err_clear();
		arg_step = interp_step_normal(term->as.app.arg);
		if (arg_step != NULL) {
			Value *app;
			fn_clone = value_clone(term->as.app.fn);
			if (fn_clone == NULL) {
				value_free(arg_step);
				err_set("out of memory");
				return NULL;
			}

			app = value_app_new(fn_clone, arg_step);
			if (app == NULL) {
				value_free(fn_clone);
				value_free(arg_step);
				err_set("out of memory");
				return NULL;
			}
			return app;
		}
		return NULL;
	}

	err_set("unknown value node");
	return NULL;
}

Value *interp_reduce_normal(const Value *term, size_t max_steps, size_t *steps_taken, int *reached_limit) {
	Value *current;
	size_t steps = 0;
	bool hit_limit = false;

	if (steps_taken != NULL) {
		*steps_taken = 0;
	}
	if (reached_limit != NULL) {
		*reached_limit = 0;
	}

	err_clear();

	current = value_clone(term);
	if (current == NULL) {
		err_set("out of memory");
		return NULL;
	}

	while (steps < max_steps) {
		Value *next;

		err_clear();
		next = interp_step_normal(current);

		if (next == NULL) {
			if (err_has()) {
				value_free(current);
				return NULL;
			}
			break;
		}

		value_free(current);
		current = next;
		steps++;
	}

	if (steps == max_steps) {
		Value *probe;

		err_clear();
		probe = interp_step_normal(current);
		if (probe == NULL) {
			if (err_has()) {
				value_free(current);
				return NULL;
			}
		} else {
			hit_limit = true;
			value_free(probe);
		}
	}

	if (reached_limit != NULL) {
		*reached_limit = hit_limit ? 1 : 0;
	}
	if (steps_taken != NULL) {
		*steps_taken = steps;
	}

	return current;
}
