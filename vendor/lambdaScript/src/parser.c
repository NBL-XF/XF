#include "../include/parser.h"

#include "../include/err.h"
#include "../include/lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	Lexer lexer;
} Parser;

static char *dup_range(const char *start, size_t length) {
	char *s = (char *)malloc(length + 1);
	if (s == NULL) {
		return NULL;
	}

	memcpy(s, start, length);
	s[length] = '\0';
	return s;
}

static char *trim_in_place(char *s) {
	char *end;

	while (*s != '\0' && isspace((unsigned char)*s)) {
		s++;
	}

	end = s + strlen(s);
	while (end > s && isspace((unsigned char)end[-1])) {
		end--;
	}
	*end = '\0';

	return s;
}

static int is_ident_start_byte(unsigned char c) {
	return isalpha(c) || c == '_' || c >= 0x80;
}

static int is_ident_part_byte(unsigned char c) {
	return isalnum(c) || c == '_' || c >= 0x80;
}

static int is_ident_text(const char *s) {
	size_t i;

	if (s == NULL || s[0] == '\0') {
		return 0;
	}
	if (!is_ident_start_byte((unsigned char)s[0])) {
		return 0;
	}

	for (i = 1; s[i] != '\0'; i++) {
		if (!is_ident_part_byte((unsigned char)s[i])) {
			return 0;
		}
	}

	return 1;
}

static int is_ident_span(const char *start, size_t length) {
	size_t i;

	if (start == NULL || length == 0) {
		return 0;
	}
	if (!is_ident_start_byte((unsigned char)start[0])) {
		return 0;
	}

	for (i = 1; i < length; i++) {
		if (!is_ident_part_byte((unsigned char)start[i])) {
			return 0;
		}
	}

	return 1;
}

typedef enum {
	ASSIGN_NONE = 0,
	ASSIGN_PLAIN,
	ASSIGN_ADD,
	ASSIGN_SUB,
	ASSIGN_MUL,
	ASSIGN_DIV,
	ASSIGN_MOD
} AssignKind;

typedef struct {
	char *eq;
	char *op_start;
	size_t op_byte;
	AssignKind kind;
	int lhs_is_bare_ident;
} AssignmentSplit;

static AssignmentSplit find_assignment_split(char *line) {
	AssignmentSplit split;
	char *eq = strchr(line, '=');
	char *lhs_start;
	char *lhs_end;

	memset(&split, 0, sizeof(split));
	while (eq != NULL) {
		if (!(eq > line && eq[-1] == '<' && eq[1] == '>')) {
			break;
		}
		eq = strchr(eq + 1, '=');
	}

	if (eq == NULL) {
		return split;
	}

	split.eq = eq;
	split.op_start = eq;
	split.op_byte = (size_t)(eq - line);
	split.kind = ASSIGN_PLAIN;

	if (eq > line) {
		switch (eq[-1]) {
		case '+':
			split.op_start = eq - 1;
			split.op_byte = (size_t)(split.op_start - line);
			split.kind = ASSIGN_ADD;
			break;
		case '-':
			split.op_start = eq - 1;
			split.op_byte = (size_t)(split.op_start - line);
			split.kind = ASSIGN_SUB;
			break;
		case '*':
			split.op_start = eq - 1;
			split.op_byte = (size_t)(split.op_start - line);
			split.kind = ASSIGN_MUL;
			break;
		case '/':
			split.op_start = eq - 1;
			split.op_byte = (size_t)(split.op_start - line);
			split.kind = ASSIGN_DIV;
			break;
		case '%':
			split.op_start = eq - 1;
			split.op_byte = (size_t)(split.op_start - line);
			split.kind = ASSIGN_MOD;
			break;
		default:
			break;
		}
	}

	lhs_start = line;
	while (*lhs_start != '\0' && isspace((unsigned char)*lhs_start)) {
		lhs_start++;
	}

	lhs_end = split.op_start;
	while (lhs_end > lhs_start && isspace((unsigned char)lhs_end[-1])) {
		lhs_end--;
	}

	split.lhs_is_bare_ident = is_ident_span(lhs_start, (size_t)(lhs_end - lhs_start));
	return split;
}

static int parser_is_atom_start(TokenKind kind) {
	return kind == TOK_IDENT ||
	       kind == TOK_NUMBER ||
	       kind == TOK_LPAREN ||
	       kind == TOK_LAMBDA ||
	       kind == TOK_SIGMA ||
	       kind == TOK_INTEGRAL ||
	       kind == TOK_LIMIT;
}

static void parser_advance(Parser *parser) {
	lexer_next(&parser->lexer);
}


static int token_is_ident_text_value(const Token *token, const char *text) {
	return token->kind == TOK_IDENT &&
	       strlen(text) == token->length &&
	       memcmp(token->start, text, token->length) == 0;
}

static int parser_expect(Parser *parser, TokenKind kind, const char *message) {
	if (parser->lexer.current.kind != kind) {
		err_set("%s at byte %zu", message, parser->lexer.current.pos);
		return 0;
	}

	parser_advance(parser);
	return 1;
}



static int parser_expect_ident_keyword(Parser *parser, const char *keyword, const char *message) {
	if (!token_is_ident_text_value(&parser->lexer.current, keyword) &&
	    !token_is_ident_text_value(&parser->lexer.current, "TO")) {
		err_set("%s at byte %zu", message, parser->lexer.current.pos);
		return 0;
	}

	parser_advance(parser);
	return 1;
}

static int line_is_math_binder_expr(const char *line) {
	Lexer lexer;

	lexer_init(&lexer, line);
	if (lexer.current.kind != TOK_SIGMA && lexer.current.kind != TOK_INTEGRAL) {
		return 0;
	}

	lexer_next(&lexer);
	if (lexer.current.kind != TOK_IDENT) {
		return 0;
	}

	lexer_next(&lexer);
	return lexer.current.kind == TOK_EQUAL;
}

static Ast *parse_term(Parser *parser);

static Ast *parse_abstraction(Parser *parser) {
	char **params = NULL;
	size_t count = 0;
	size_t capacity = 0;
	Ast *body = NULL;
	size_t i;

	if (!parser_expect(parser, TOK_LAMBDA, "expected lambda")) {
		return NULL;
	}

	if (parser->lexer.current.kind != TOK_IDENT) {
		err_set("expected identifier after lambda at byte %zu", parser->lexer.current.pos);
		goto fail;
	}

	while (parser->lexer.current.kind == TOK_IDENT) {
		char *name;
		char **new_params;
		size_t new_capacity;

		if (count == capacity) {
			new_capacity = capacity == 0 ? 4 : capacity * 2;
			new_params = (char **)realloc(params, new_capacity * sizeof(*new_params));
			if (new_params == NULL) {
				err_set("out of memory");
				goto fail;
			}
			params = new_params;
			capacity = new_capacity;
		}

		name = dup_range(parser->lexer.current.start, parser->lexer.current.length);
		if (name == NULL) {
			err_set("out of memory");
			goto fail;
		}

		params[count++] = name;
		parser_advance(parser);
	}

	if (!parser_expect(parser, TOK_DOT, "expected '.' after lambda parameters")) {
		goto fail;
	}

	body = parse_term(parser);
	if (body == NULL) {
		goto fail;
	}

	for (i = count; i > 0; i--) {
		body = ast_lam_new(params[i - 1], body);
		if (body == NULL) {
			err_set("out of memory");
			goto fail;
		}
	}

	for (i = 0; i < count; i++) {
		free(params[i]);
	}
	free(params);

	return body;

fail:
	if (body != NULL) {
		ast_free(body);
	}
	for (i = 0; i < count; i++) {
		free(params[i]);
	}
	free(params);
	return NULL;
}

static Ast *make_unary_call(const char *name, Ast *expr) {
	Ast *op;
	Ast *app;

	op = ast_var_new(name);
	if (op == NULL) {
		ast_free(expr);
		err_set("out of memory");
		return NULL;
	}

	app = ast_app_new(op, expr);
	if (app == NULL) {
		ast_free(op);
		ast_free(expr);
		err_set("out of memory");
		return NULL;
	}

	return app;
}

static Ast *make_binary_call(const char *name, Ast *left, Ast *right) {
	Ast *op;
	Ast *partial;
	Ast *app;

	op = ast_var_new(name);
	if (op == NULL) {
		ast_free(left);
		ast_free(right);
		err_set("out of memory");
		return NULL;
	}

	partial = ast_app_new(op, left);
	if (partial == NULL) {
		ast_free(op);
		ast_free(left);
		ast_free(right);
		err_set("out of memory");
		return NULL;
	}

	app = ast_app_new(partial, right);
	if (app == NULL) {
		ast_free(partial);
		ast_free(right);
		err_set("out of memory");
		return NULL;
	}

	return app;
}

static Ast *make_ternary_call(const char *name, Ast *first, Ast *second, Ast *third) {
	Ast *op;
	Ast *partial1;
	Ast *partial2;
	Ast *app;

	op = ast_var_new(name);
	if (op == NULL) {
		ast_free(first);
		ast_free(second);
		ast_free(third);
		err_set("out of memory");
		return NULL;
	}

	partial1 = ast_app_new(op, first);
	if (partial1 == NULL) {
		ast_free(op);
		ast_free(second);
		ast_free(third);
		err_set("out of memory");
		return NULL;
	}

	partial2 = ast_app_new(partial1, second);
	if (partial2 == NULL) {
		ast_free(partial1);
		ast_free(third);
		err_set("out of memory");
		return NULL;
	}

	app = ast_app_new(partial2, third);
	if (app == NULL) {
		ast_free(partial2);
		ast_free(third);
		err_set("out of memory");
		return NULL;
	}

	return app;
}

static Ast *parse_symbolic_binder(Parser *parser, TokenKind opener, const char *name) {
	char *param_name = NULL;
	Ast *lower = NULL;
	Ast *upper = NULL;
	Ast *body = NULL;
	Ast *lambda = NULL;
	Ast *result = NULL;

	parser_advance(parser);

	if (parser->lexer.current.kind != TOK_IDENT) {
		err_set("expected identifier after %s at byte %zu",
		        opener == TOK_SIGMA ? "Σ" : (opener == TOK_INTEGRAL ? "∫" : "lim"),
		        parser->lexer.current.pos);
		goto fail;
	}

	param_name = dup_range(parser->lexer.current.start, parser->lexer.current.length);
	if (param_name == NULL) {
		err_set("out of memory");
		goto fail;
	}
	parser_advance(parser);

	if (opener == TOK_LIMIT) {
		if (!parser_expect(parser, TOK_IMPLIES, "expected '->' after limit variable")) {
			goto fail;
		}
		lower = parse_term(parser);
		if (lower == NULL) {
			goto fail;
		}
		if (!parser_expect(parser, TOK_DOT, "expected '.' after limit point")) {
			goto fail;
		}
		body = parse_term(parser);
		if (body == NULL) {
			goto fail;
		}
		lambda = ast_lam_new(param_name, body);
		body = NULL;
		if (lambda == NULL) {
			err_set("out of memory");
			goto fail;
		}
		result = make_binary_call(name, lower, lambda);
		lower = NULL;
		lambda = NULL;
		return result;
	}

	if (!parser_expect(parser, TOK_EQUAL, "expected '=' after binder variable")) {
		goto fail;
	}

	lower = parse_term(parser);
	if (lower == NULL) {
		goto fail;
	}

	if (!(parser->lexer.current.kind == TOK_TO || token_is_ident_text_value(&parser->lexer.current, "to") || token_is_ident_text_value(&parser->lexer.current, "TO"))) {
		err_set("expected 'to' in bounded form at byte %zu", parser->lexer.current.pos);
		goto fail;
	}
	parser_advance(parser);

	upper = parse_term(parser);
	if (upper == NULL) {
		goto fail;
	}

	if (!parser_expect(parser, TOK_DOT, "expected '.' before binder body")) {
		goto fail;
	}

	body = parse_term(parser);
	if (body == NULL) {
		goto fail;
	}

	lambda = ast_lam_new(param_name, body);
	body = NULL;
	if (lambda == NULL) {
		err_set("out of memory");
		goto fail;
	}

	result = make_ternary_call(name, lower, upper, lambda);
	lower = NULL;
	upper = NULL;
	lambda = NULL;
	return result;

fail:
	free(param_name);
	ast_free(lower);
	ast_free(upper);
	ast_free(body);
	ast_free(lambda);
	return NULL;
}


static int ident_ends_with_underscore(const char *name) {
	size_t len;

	if (name == NULL) {
		return 0;
	}

	len = strlen(name);
	return len > 0 && name[len - 1] == '_';
}

static int lexer_paren_has_top_level_comma(Lexer lexer) {
	int depth = 0;

	if (lexer.current.kind != TOK_LPAREN) {
		return 0;
	}

	do {
		if (lexer.current.kind == TOK_LPAREN) {
			depth++;
		} else if (lexer.current.kind == TOK_RPAREN) {
			depth--;
			if (depth == 0) {
				return 0;
			}
		} else if (lexer.current.kind == TOK_COMMA && depth == 1) {
			return 1;
		} else if (lexer.current.kind == TOK_EOF || lexer.current.kind == TOK_INVALID) {
			return 0;
		}

		lexer_next(&lexer);
	} while (depth > 0);

	return 0;
}

static Ast *make_indexed_call_from_base(Ast *base, Ast **items, size_t count) {
	Ast *current;
	size_t i;

	if (base == NULL || count == 0) {
		ast_free(base);
		return NULL;
	}

	current = make_binary_call("sub", base, items[0]);
	if (current == NULL) {
		for (i = 1; i < count; i++) {
			ast_free(items[i]);
		}
		return NULL;
	}

	for (i = 1; i < count; i++) {
		Ast *next = ast_app_new(current, items[i]);
		if (next == NULL) {
			ast_free(current);
			for (; i < count; i++) {
				ast_free(items[i]);
			}
			err_set("out of memory");
			return NULL;
		}
		current = next;
	}

	return current;
}

static Ast *parse_indexed_args(Parser *parser, Ast *base) {
	Ast **items = NULL;
	size_t count = 0;
	size_t capacity = 0;
	Ast *result = NULL;

	if (!parser_expect(parser, TOK_LPAREN, "expected '('")) {
		ast_free(base);
		return NULL;
	}

	for (;;) {
		Ast *item;
		Ast **new_items;

		item = parse_term(parser);
		if (item == NULL) {
			goto fail;
		}

		if (count == capacity) {
			size_t new_capacity = capacity == 0 ? 4 : capacity * 2;
			new_items = (Ast **)realloc(items, new_capacity * sizeof(*new_items));
			if (new_items == NULL) {
				ast_free(item);
				err_set("out of memory");
				goto fail;
			}
			items = new_items;
			capacity = new_capacity;
		}

		items[count++] = item;

		if (parser->lexer.current.kind == TOK_COMMA) {
			parser_advance(parser);
			continue;
		}
		break;
	}

	if (!parser_expect(parser, TOK_RPAREN, "expected ')' after index list")) {
		goto fail;
	}

	result = make_indexed_call_from_base(base, items, count);
	free(items);
	return result;

fail:
	ast_free(base);
	for (size_t i = 0; i < count; i++) {
		ast_free(items[i]);
	}
	free(items);
	return NULL;
}

static Ast *parse_atom(Parser *parser) {
	Ast *node;
	char *name;

	switch (parser->lexer.current.kind) {
	case TOK_IDENT: {
		Lexer lookahead;

		name = dup_range(parser->lexer.current.start, parser->lexer.current.length);
		if (name == NULL) {
			err_set("out of memory");
			return NULL;
		}

		parser_advance(parser);
		node = ast_var_new(name);
		if (node == NULL) {
			free(name);
			err_set("out of memory");
			return NULL;
		}

		lookahead = parser->lexer;
		if (parser->lexer.current.kind == TOK_LPAREN &&
		    (ident_ends_with_underscore(name) || lexer_paren_has_top_level_comma(lookahead))) {
			Ast *base = node;
			if (ident_ends_with_underscore(name)) {
				char *trimmed = dup_range(name, strlen(name) - 1);
				if (trimmed == NULL) {
					ast_free(base);
					free(name);
					err_set("out of memory");
					return NULL;
				}
				ast_free(base);
				base = ast_var_new(trimmed);
				free(trimmed);
				if (base == NULL) {
					free(name);
					err_set("out of memory");
					return NULL;
				}
			}
			free(name);
			return parse_indexed_args(parser, base);
		}

		free(name);
		return node;
	}

	case TOK_NUMBER:
		node = ast_number_new(parser->lexer.current.number);
		if (node == NULL) {
			err_set("out of memory");
			return NULL;
		}
		parser_advance(parser);
		return node;

	case TOK_LPAREN:
		parser_advance(parser);
		node = parse_term(parser);
		if (node == NULL) {
			return NULL;
		}
		if (!parser_expect(parser, TOK_RPAREN, "expected ')'")) {
			ast_free(node);
			return NULL;
		}
		return node;

	case TOK_LAMBDA:
		return parse_abstraction(parser);

	case TOK_SIGMA:
		return parse_symbolic_binder(parser, TOK_SIGMA, "sigma");

	case TOK_INTEGRAL:
		return parse_symbolic_binder(parser, TOK_INTEGRAL, "integral");

	case TOK_LIMIT:
		return parse_symbolic_binder(parser, TOK_LIMIT, "limit");

	default:
		err_set("unexpected token at byte %zu", parser->lexer.current.pos);
		return NULL;
	}
}

static Ast *parse_application(Parser *parser) {
	Ast *left = parse_atom(parser);

	if (left == NULL) {
		return NULL;
	}

	while (parser_is_atom_start(parser->lexer.current.kind)) {
		Ast *right = parse_atom(parser);
		Ast *app;

		if (right == NULL) {
			ast_free(left);
			return NULL;
		}

		app = ast_app_new(left, right);
		if (app == NULL) {
			ast_free(left);
			ast_free(right);
			err_set("out of memory");
			return NULL;
		}

		left = app;
	}

	return left;
}

static Ast *parse_unary(Parser *parser) {
	Ast *expr;

	switch (parser->lexer.current.kind) {
	case TOK_MINUS:
		parser_advance(parser);
		expr = parse_unary(parser);
		if (expr == NULL) {
			return NULL;
		}
		return make_unary_call("NEG", expr);

	case TOK_SQRT:
		parser_advance(parser);
		expr = parse_unary(parser);
		if (expr == NULL) {
			return NULL;
		}
		return make_unary_call("SQRT", expr);

	case TOK_LN:
		parser_advance(parser);
		expr = parse_unary(parser);
		if (expr == NULL) {
			return NULL;
		}
		return make_unary_call("LN", expr);

	default:
		return parse_application(parser);
	}
}

static Ast *parse_power(Parser *parser) {
	Ast *left = parse_unary(parser);

	if (left == NULL) {
		return NULL;
	}

	if (parser->lexer.current.kind == TOK_POWER) {
		Ast *right;

		parser_advance(parser);
		right = parse_power(parser);
		if (right == NULL) {
			ast_free(left);
			return NULL;
		}

		return make_binary_call("POW", left, right);
	}

	return left;
}

static Ast *parse_mul(Parser *parser) {
	Ast *left = parse_power(parser);

	if (left == NULL) {
		return NULL;
	}

	while (parser->lexer.current.kind == TOK_STAR ||
	       parser->lexer.current.kind == TOK_SLASH ||
	       parser->lexer.current.kind == TOK_PERCENT) {
		TokenKind op = parser->lexer.current.kind;
		Ast *right;
		const char *name;

		parser_advance(parser);
		right = parse_power(parser);
		if (right == NULL) {
			ast_free(left);
			return NULL;
		}

		name = op == TOK_STAR ? "MUL" : (op == TOK_SLASH ? "DIV" : "MOD");
		left = make_binary_call(name, left, right);
		if (left == NULL) {
			return NULL;
		}
	}

	return left;
}

static Ast *parse_add(Parser *parser) {
	Ast *left = parse_mul(parser);

	if (left == NULL) {
		return NULL;
	}

	while (parser->lexer.current.kind == TOK_PLUS ||
	       parser->lexer.current.kind == TOK_MINUS) {
		TokenKind op = parser->lexer.current.kind;
		Ast *right;

		parser_advance(parser);
		right = parse_mul(parser);
		if (right == NULL) {
			ast_free(left);
			return NULL;
		}

		left = make_binary_call(op == TOK_PLUS ? "ADD" : "SUB", left, right);
		if (left == NULL) {
			return NULL;
		}
	}

	return left;
}

static Ast *parse_membership(Parser *parser) {
	Ast *left = parse_add(parser);

	if (left == NULL) {
		return NULL;
	}

	while (parser->lexer.current.kind == TOK_IN ||
	       parser->lexer.current.kind == TOK_CONTAINS ||
	       token_is_ident_text_value(&parser->lexer.current, "in") ||
	       token_is_ident_text_value(&parser->lexer.current, "contains") ||
	       token_is_ident_text_value(&parser->lexer.current, "CONTAINS")) {
		int is_in = parser->lexer.current.kind == TOK_IN || token_is_ident_text_value(&parser->lexer.current, "in");
		Ast *right;

		parser_advance(parser);
		right = parse_add(parser);
		if (right == NULL) {
			ast_free(left);
			return NULL;
		}

		left = make_binary_call(is_in ? "elem" : "contains", left, right);
		if (left == NULL) {
			return NULL;
		}
	}

	return left;
}

static Ast *parse_not(Parser *parser) {
	Ast *expr;

	if (parser->lexer.current.kind == TOK_NOT) {
		parser_advance(parser);
		expr = parse_not(parser);
		if (expr == NULL) {
			return NULL;
		}
		return make_unary_call("NOT", expr);
	}

	return parse_membership(parser);
}

static Ast *parse_and(Parser *parser) {
	Ast *left = parse_not(parser);

	if (left == NULL) {
		return NULL;
	}

	while (parser->lexer.current.kind == TOK_AND) {
		Ast *right;

		parser_advance(parser);
		right = parse_not(parser);
		if (right == NULL) {
			ast_free(left);
			return NULL;
		}

		left = make_binary_call("AND", left, right);
		if (left == NULL) {
			return NULL;
		}
	}

	return left;
}

static Ast *parse_or(Parser *parser) {
	Ast *left = parse_and(parser);

	if (left == NULL) {
		return NULL;
	}

	while (parser->lexer.current.kind == TOK_OR) {
		Ast *right;

		parser_advance(parser);
		right = parse_and(parser);
		if (right == NULL) {
			ast_free(left);
			return NULL;
		}

		left = make_binary_call("OR", left, right);
		if (left == NULL) {
			return NULL;
		}
	}

	return left;
}

static Ast *parse_implication(Parser *parser) {
	Ast *left = parse_or(parser);

	if (left == NULL) {
		return NULL;
	}

	if (parser->lexer.current.kind == TOK_IMPLIES) {
		Ast *right;

		parser_advance(parser);
		right = parse_implication(parser);
		if (right == NULL) {
			ast_free(left);
			return NULL;
		}

		return make_binary_call("IMP", left, right);
	}

	return left;
}

static Ast *parse_iff(Parser *parser) {
	Ast *left = parse_implication(parser);

	if (left == NULL) {
		return NULL;
	}

	while (parser->lexer.current.kind == TOK_IFF) {
		Ast *right;

		parser_advance(parser);
		right = parse_implication(parser);
		if (right == NULL) {
			ast_free(left);
			return NULL;
		}

		left = make_binary_call("IFF", left, right);
		if (left == NULL) {
			return NULL;
		}
	}

	return left;
}

static Ast *parse_term(Parser *parser) {
	if (parser->lexer.current.kind == TOK_LAMBDA) {
		return parse_abstraction(parser);
	}

	return parse_iff(parser);
}

Ast *parser_parse_expr(const char *source) {
	Parser parser;
	Ast *root;

	err_clear();
	lexer_init(&parser.lexer, source);

	if (parser.lexer.current.kind == TOK_INVALID) {
		err_set("invalid token at byte %zu", parser.lexer.current.pos);
		return NULL;
	}

	root = parse_term(&parser);
	if (root == NULL) {
		return NULL;
	}

	if (parser.lexer.current.kind == TOK_INVALID) {
		ast_free(root);
		err_set("invalid token at byte %zu", parser.lexer.current.pos);
		return NULL;
	}

	if (parser.lexer.current.kind != TOK_EOF) {
		ast_free(root);
		err_set("unexpected trailing input at byte %zu", parser.lexer.current.pos);
		return NULL;
	}

	return root;
}

Program *parser_parse_program(const char *source) {
	const char *line_start = source;
	const char *cursor = source;
	size_t line_no = 1;
	Program *program = program_new();

	if (program == NULL) {
		err_set("out of memory");
		return NULL;
	}

	while (1) {
		char *raw_line;
		char *trimmed;
		AssignmentSplit split;

		if (*cursor != '\n' && *cursor != '\0') {
			cursor++;
			continue;
		}

		raw_line = dup_range(line_start, (size_t)(cursor - line_start));
		if (raw_line == NULL) {
			err_set("out of memory");
			goto fail;
		}

		trimmed = trim_in_place(raw_line);
		{
			size_t len = strlen(trimmed);
			if (len > 0 && trimmed[len - 1] == '\r') {
				trimmed[len - 1] = '\0';
				trimmed = trim_in_place(trimmed);
			}
		}

		if (trimmed[0] != '\0') {
			if (!(trimmed[0] == ';' || (trimmed[0] == '-' && trimmed[1] == '-'))) {
				split = find_assignment_split(trimmed);

				if (split.eq != NULL && !split.lhs_is_bare_ident && !line_is_math_binder_expr(trimmed)) {
					err_set("line %zu: invalid token at byte %zu", line_no, split.op_byte);
					free(raw_line);
					goto fail;
				}

				if (split.eq != NULL && split.lhs_is_bare_ident) {
					Ast *expr;
					char saved[512];
					char *lhs;
					char *rhs;

					*split.op_start = '\0';
					lhs = trim_in_place(trimmed);
					rhs = trim_in_place(split.eq + 1);

					if (!is_ident_text(lhs)) {
						err_set("line %zu: invalid definition name", line_no);
						free(raw_line);
						goto fail;
					}
					if (rhs[0] == '\0') {
						err_set("line %zu: missing definition body", line_no);
						free(raw_line);
						goto fail;
					}

					expr = parser_parse_expr(rhs);
					if (expr == NULL) {
						snprintf(saved, sizeof(saved), "%s", err_get());
						err_set("line %zu: %s", line_no, saved);
						free(raw_line);
						goto fail;
					}

					if (split.kind != ASSIGN_PLAIN) {
						Ast *lhs_ref = ast_var_new(lhs);
						const char *op_name =
							split.kind == ASSIGN_ADD ? "ADD" :
							split.kind == ASSIGN_SUB ? "SUB" :
							split.kind == ASSIGN_MUL ? "MUL" :
							split.kind == ASSIGN_DIV ? "DIV" : "MOD";

						if (lhs_ref == NULL) {
							ast_free(expr);
							err_set("out of memory");
							free(raw_line);
							goto fail;
						}

						expr = make_binary_call(op_name, lhs_ref, expr);
						if (expr == NULL) {
							free(raw_line);
							goto fail;
						}
					}

					if (!program_add_def(program, lhs, expr)) {
						err_set("out of memory");
						free(raw_line);
						goto fail;
					}
				} else {
					Ast *expr;
					char saved[512];

					if (program->expr != NULL) {
						err_set("line %zu: multiple top-level expressions", line_no);
						free(raw_line);
						goto fail;
					}

					expr = parser_parse_expr(trimmed);
					if (expr == NULL) {
						snprintf(saved, sizeof(saved), "%s", err_get());
						err_set("line %zu: %s", line_no, saved);
						free(raw_line);
						goto fail;
					}

					program->expr = expr;
				}
			}
		}

		free(raw_line);

		if (*cursor == '\0') {
			break;
		}

		cursor++;
		line_start = cursor;
		line_no++;
	}

	if (program->expr == NULL) {
		err_set("program must end with an expression");
		goto fail;
	}

	return program;

fail:
	program_free(program);
	return NULL;
}
