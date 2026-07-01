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

static int church_boolean_value(const Value *value, int *out_truth) {
	if (value == NULL || out_truth == NULL) {
		return 0;
	}

	if (value->kind != VALUE_LAM ||
	    value->as.lam.body == NULL ||
	    value->as.lam.body->kind != VALUE_LAM ||
	    value->as.lam.body->as.lam.body == NULL ||
	    value->as.lam.body->as.lam.body->kind != VALUE_BOUND_VAR) {
		return 0;
	}

	if (value->as.lam.body->as.lam.body->as.bound_var.index == 1) {
		*out_truth = 1;
		return 1;
	}

	if (value->as.lam.body->as.lam.body->as.bound_var.index == 0) {
		*out_truth = 0;
		return 1;
	}

	return 0;
}

static int value_has_normal_step(const Value *value, int *out_has_step) {
	Value *next;

	if (out_has_step == NULL) {
		err_set("invalid step probe");
		return 0;
	}

	err_clear();
	next = interp_step_normal(value);
	if (next != NULL) {
		value_free(next);
		*out_has_step = 1;
		return 1;
	}

	if (err_has()) {
		return 0;
	}

	*out_has_step = 0;
	return 1;
}



static int reduce_clone_normal(const Value *value, Value **out_value) {
	size_t steps = 0;
	int reached_limit = 0;

	if (out_value == NULL) {
		err_set("invalid reduce target");
		return 0;
	}

	*out_value = interp_reduce_normal(value, 4096, &steps, &reached_limit);
	if (*out_value == NULL) {
		return 0;
	}
	if (reached_limit) {
		value_free(*out_value);
		*out_value = NULL;
		err_set("reduction step limit reached");
		return 0;
	}
	return 1;
}

static const Value *value_head(const Value *value) {
	while (value != NULL && value->kind == VALUE_APP) {
		value = value->as.app.fn;
	}
	return value;
}

static int marker_boolean_value(const Value *value, int *out_truth) {
	const Value *head;

	if (church_boolean_value(value, out_truth)) {
		return 1;
	}

	head = value_head(value);
	if (head == NULL || head->kind != VALUE_FREE_VAR) {
		return 0;
	}

	if (strcmp(head->as.free_var.name, "True") == 0 ||
	    strcmp(head->as.free_var.name, "TRUE") == 0) {
		*out_truth = 1;
		return 1;
	}

	if (strcmp(head->as.free_var.name, "False") == 0 ||
	    strcmp(head->as.free_var.name, "FALSE") == 0 ||
	    strcmp(head->as.free_var.name, "nil") == 0 ||
	    strcmp(head->as.free_var.name, "NIL") == 0) {
		*out_truth = 0;
		return 1;
	}

	return 0;
}

static int normalized_boolean_value(const Value *value, int *out_truth) {
	Value *reduced;
	int ok;

	if (!reduce_clone_normal(value, &reduced)) {
		return 0;
	}

	ok = marker_boolean_value(reduced, out_truth);
	value_free(reduced);
	return ok;
}

static int is_boolean_binary_name(const char *name) {
	return strcmp(name, "AND") == 0 ||
	       strcmp(name, "OR") == 0 ||
	       strcmp(name, "IMP") == 0 ||
	       strcmp(name, "IFF") == 0;
}

static Value *apply_boolean_binary_primitive(const char *name, int left, int right) {
	if (strcmp(name, "AND") == 0) {
		return value_church_boolean(left && right);
	}
	if (strcmp(name, "OR") == 0) {
		return value_church_boolean(left || right);
	}
	if (strcmp(name, "IMP") == 0) {
		return value_church_boolean((!left) || right);
	}
	if (strcmp(name, "IFF") == 0) {
		return value_church_boolean(left == right);
	}
	return NULL;
}

static int list_value_is_nil(const Value *value) {
	return value != NULL &&
	       value->kind == VALUE_LAM &&
	       value->as.lam.body != NULL &&
	       value->as.lam.body->kind == VALUE_LAM &&
	       value->as.lam.body->as.lam.body != NULL &&
	       value->as.lam.body->as.lam.body->kind == VALUE_BOUND_VAR &&
	       value->as.lam.body->as.lam.body->as.bound_var.index == 0;
}

static int list_value_unpack_cons(const Value *value, Value **out_head, Value **out_tail) {
	const Value *body;
	const Value *step1;
	const Value *head_term;
	const Value *tail_term;

	if (out_head == NULL || out_tail == NULL) {
		err_set("invalid list unpack target");
		return 0;
	}

	*out_head = NULL;
	*out_tail = NULL;

	if (value == NULL ||
	    value->kind != VALUE_LAM ||
	    value->as.lam.body == NULL ||
	    value->as.lam.body->kind != VALUE_LAM) {
		return 0;
	}

	body = value->as.lam.body->as.lam.body;
	if (body == NULL ||
	    body->kind != VALUE_APP ||
	    body->as.app.fn == NULL ||
	    body->as.app.fn->kind != VALUE_APP ||
	    body->as.app.fn->as.app.fn == NULL ||
	    body->as.app.fn->as.app.fn->kind != VALUE_BOUND_VAR ||
	    body->as.app.fn->as.app.fn->as.bound_var.index != 1) {
		return 0;
	}

	step1 = body->as.app.fn;
	head_term = step1->as.app.arg;
	tail_term = body->as.app.arg;

	*out_head = value_shift(head_term, -2);
	if (*out_head == NULL) {
		return 0;
	}

	*out_tail = value_shift(tail_term, -2);
	if (*out_tail == NULL) {
		value_free(*out_head);
		*out_head = NULL;
		return 0;
	}

	return 1;
}

static Value *apply_value_pair(const Value *fn, const Value *arg) {
	return value_app_new(value_clone(fn), value_clone(arg));
}

static Value *reduce_applied_once(const Value *fn, const Value *arg) {
	Value *app;
	Value *reduced;

	app = apply_value_pair(fn, arg);
	if (app == NULL) {
		err_set("out of memory");
		return NULL;
	}

	reduced = interp_reduce_normal(app, 4096, NULL, NULL);
	value_free(app);
	return reduced;
}

static Value *reduce_elem_primitive(const Value *needle, const Value *set) {
	Value *predicate_result = NULL;
	int truth = 0;
	Value *set_normal = NULL;
	Value *needle_normal = NULL;
	Value *current = NULL;

	predicate_result = reduce_applied_once(set, needle);
	if (predicate_result != NULL) {
		if (marker_boolean_value(predicate_result, &truth)) {
			value_free(predicate_result);
			return value_church_boolean(truth);
		}
		value_free(predicate_result);
	}

	if (!reduce_clone_normal(set, &set_normal)) {
		err_clear();
		return NULL;
	}
	if (!reduce_clone_normal(needle, &needle_normal)) {
		value_free(set_normal);
		err_clear();
		return NULL;
	}

	current = set_normal;
	set_normal = NULL;

	for (;;) {
		Value *head_item = NULL;
		Value *tail_item = NULL;
		Value *head_normal = NULL;

		if (list_value_is_nil(current)) {
			value_free(current);
			value_free(needle_normal);
			return value_church_boolean(0);
		}

		if (!list_value_unpack_cons(current, &head_item, &tail_item)) {
			value_free(current);
			value_free(needle_normal);
			return NULL;
		}

		if (!reduce_clone_normal(head_item, &head_normal)) {
			value_free(head_item);
			value_free(tail_item);
			value_free(current);
			value_free(needle_normal);
			return NULL;
		}

		if (value_structural_equal(needle_normal, head_normal)) {
			value_free(head_normal);
			value_free(head_item);
			value_free(tail_item);
			value_free(current);
			value_free(needle_normal);
			return value_church_boolean(1);
		}

		value_free(head_normal);
		value_free(head_item);
		value_free(current);
		current = NULL;

		if (!reduce_clone_normal(tail_item, &current)) {
			value_free(tail_item);
			value_free(needle_normal);
			return NULL;
		}
		value_free(tail_item);
	}
}

static Value *reduce_contains_primitive(const Value *set, const Value *needle) {
	return reduce_elem_primitive(needle, set);
}

static Value *reduce_quantifier_primitive(const char *name, const Value *domain, const Value *predicate) {
	Value *current = NULL;
	int want_forall = strcmp(name, "forall") == 0 || strcmp(name, "FORALL") == 0;

	if (!reduce_clone_normal(domain, &current)) {
		return NULL;
	}

	for (;;) {
		Value *head_item = NULL;
		Value *tail_item = NULL;
		Value *pred_result = NULL;
		int truth = 0;

		if (list_value_is_nil(current)) {
			value_free(current);
			return value_church_boolean(want_forall ? 1 : 0);
		}

		if (!list_value_unpack_cons(current, &head_item, &tail_item)) {
			value_free(current);
			return NULL;
		}

		pred_result = reduce_applied_once(predicate, head_item);
		value_free(head_item);
		if (pred_result == NULL) {
			value_free(tail_item);
			value_free(current);
			return NULL;
		}

		if (!marker_boolean_value(pred_result, &truth)) {
			value_free(pred_result);
			value_free(tail_item);
			value_free(current);
			return NULL;
		}
		value_free(pred_result);

		if (want_forall && !truth) {
			value_free(tail_item);
			value_free(current);
			return value_church_boolean(0);
		}
		if (!want_forall && truth) {
			value_free(tail_item);
			value_free(current);
			return value_church_boolean(1);
		}

		value_free(current);
		current = NULL;

		if (!reduce_clone_normal(tail_item, &current)) {
			value_free(tail_item);
			return NULL;
		}
		value_free(tail_item);
	}
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
		const char *name = head->as.app.fn->as.free_var.name;
		const Value *left_arg = head->as.app.arg;

		if (strcmp(name, "True") == 0 || strcmp(name, "TRUE") == 0) {
			return value_clone(left_arg);
		}
		if (strcmp(name, "False") == 0 || strcmp(name, "FALSE") == 0 ||
		    strcmp(name, "nil") == 0 || strcmp(name, "NIL") == 0) {
			return value_clone(right);
		}

		if (is_equiv_name(name)) {
			int left_truth = 0;
			int right_truth = 0;
			int left_has_step = 0;
			int right_has_step = 0;

			if (!value_has_normal_step(left_arg, &left_has_step)) {
				return NULL;
			}
			if (left_has_step) {
				return NULL;
			}
			if (!value_has_normal_step(right, &right_has_step)) {
				return NULL;
			}
			if (right_has_step) {
				return NULL;
			}

			if (marker_boolean_value(left_arg, &left_truth) &&
			    marker_boolean_value(right, &right_truth)) {
				return value_church_boolean(left_truth == right_truth);
			}

			return value_church_boolean(value_structural_equal(left_arg, right));
		}

		if (left_arg->kind == VALUE_NUMBER && right->kind == VALUE_NUMBER) {
			return apply_binary_primitive(name, left_arg->as.number.value, right->as.number.value);
		}

		if (is_boolean_binary_name(name)) {
			int left_truth = 0;
			int right_truth = 0;
			if (normalized_boolean_value(left_arg, &left_truth) &&
			    normalized_boolean_value(right, &right_truth)) {
				return apply_boolean_binary_primitive(name, left_truth, right_truth);
			}
		}

		if (strcmp(name, "elem") == 0) {
			return reduce_elem_primitive(left_arg, right);
		}
		if (strcmp(name, "contains") == 0) {
			return reduce_contains_primitive(left_arg, right);
		}
		if (strcmp(name, "forall") == 0 || strcmp(name, "exists") == 0 ||
		    strcmp(name, "FORALL") == 0 || strcmp(name, "EXISTS") == 0) {
			return reduce_quantifier_primitive(name, left_arg, right);
		}
	}

	if (head->kind == VALUE_FREE_VAR) {
		if (right->kind == VALUE_NUMBER) {
			Value *numeric = apply_unary_primitive(head->as.free_var.name, right->as.number.value);
			if (numeric != NULL) {
				return numeric;
			}
		}

		if (strcmp(head->as.free_var.name, "NOT") == 0) {
			int truth = 0;
			if (normalized_boolean_value(right, &truth)) {
				return value_church_boolean(!truth);
			}
		}
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
