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

static int is_ident_text(const char *s) {
	size_t i;

	if (s == NULL || s[0] == '\0') {
		return 0;
	}
	if (!(isalpha((unsigned char)s[0]) || s[0] == '_')) {
		return 0;
	}

	for (i = 1; s[i] != '\0'; i++) {
		if (!(isalnum((unsigned char)s[i]) || s[i] == '_')) {
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
	if (!(isalpha((unsigned char)start[0]) || start[0] == '_')) {
		return 0;
	}

	for (i = 1; i < length; i++) {
		if (!(isalnum((unsigned char)start[i]) || start[i] == '_')) {
			return 0;
		}
	}

	return 1;
}

static char *find_definition_equals(char *line) {
	char *eq = strchr(line, '=');
	char *lhs_start;
	char *lhs_end;

	if (eq == NULL) {
		return NULL;
	}

	lhs_start = line;
	while (lhs_start < eq && isspace((unsigned char)*lhs_start)) {
		lhs_start++;
	}

	lhs_end = eq;
	while (lhs_end > lhs_start && isspace((unsigned char)lhs_end[-1])) {
		lhs_end--;
	}

	if (!is_ident_span(lhs_start, (size_t)(lhs_end - lhs_start))) {
		return NULL;
	}

	return eq;
}

static int parser_is_atom_start(TokenKind kind) {
	return kind == TOK_IDENT || kind == TOK_LPAREN || kind == TOK_LAMBDA;
}

static void parser_advance(Parser *parser) {
	lexer_next(&parser->lexer);
}

static int parser_expect(Parser *parser, TokenKind kind, const char *message) {
	if (parser->lexer.current.kind != kind) {
		err_set("%s at byte %zu", message, parser->lexer.current.pos);
		return 0;
	}

	parser_advance(parser);
	return 1;
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

static Ast *parse_atom(Parser *parser) {
	Ast *node;
	char *name;

	switch (parser->lexer.current.kind) {
	case TOK_IDENT:
		name = dup_range(parser->lexer.current.start, parser->lexer.current.length);
		if (name == NULL) {
			err_set("out of memory");
			return NULL;
		}

		parser_advance(parser);
		node = ast_var_new(name);
		free(name);

		if (node == NULL) {
			err_set("out of memory");
			return NULL;
		}
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

	return parse_application(parser);
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
		char *eq;

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
				eq = find_definition_equals(trimmed);

				if (eq != NULL) {
					Ast *expr;
					char saved[512];
					char *lhs;
					char *rhs;

					*eq = '\0';
					lhs = trim_in_place(trimmed);
					rhs = trim_in_place(eq + 1);

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