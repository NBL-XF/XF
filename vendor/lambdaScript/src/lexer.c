#include "../include/lexer.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int is_ident_start(unsigned char c) {
	return isalpha(c) || c == '_' || c >= 0x80;
}

static int is_ident_part(unsigned char c) {
	return isalnum(c) || c == '_' || c >= 0x80;
}

static int ident_equals(const char *start, size_t length, const char *text) {
	return strlen(text) == length && memcmp(start, text, length) == 0;
}

static int is_utf8_lambda(const char *src, size_t pos, size_t len) {
	return pos + 1 < len &&
	       (unsigned char)src[pos] == 0xCE &&
	       (unsigned char)src[pos + 1] == 0xBB;
}

static int is_utf8_sigma(const char *src, size_t pos, size_t len) {
	return pos + 1 < len &&
	       (unsigned char)src[pos] == 0xCE &&
	       (unsigned char)src[pos + 1] == 0xA3;
}

static int is_utf8_and(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x88 &&
	       (unsigned char)src[pos + 2] == 0xA7;
}

static int is_utf8_or(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x88 &&
	       (unsigned char)src[pos + 2] == 0xA8;
}

static int is_utf8_integral(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x88 &&
	       (unsigned char)src[pos + 2] == 0xAB;
}

static int is_utf8_forall(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x88 &&
	       (unsigned char)src[pos + 2] == 0x80;
}

static int is_utf8_exists(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x88 &&
	       (unsigned char)src[pos + 2] == 0x83;
}

static int is_utf8_exists_alt_upper(const char *src, size_t pos, size_t len) {
	return pos + 1 < len &&
	       (unsigned char)src[pos] == 0xC6 &&
	       (unsigned char)src[pos + 1] == 0x8E;
}

static int is_utf8_exists_alt_lower(const char *src, size_t pos, size_t len) {
	return pos + 1 < len &&
	       (unsigned char)src[pos] == 0xC7 &&
	       (unsigned char)src[pos + 1] == 0x9D;
}

static int is_utf8_element_of(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x88 &&
	       (unsigned char)src[pos + 2] == 0x88;
}

static int is_utf8_contains(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x88 &&
	       (unsigned char)src[pos + 2] == 0x8B;
}

static int is_utf8_contains_alt(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x88 &&
	       (unsigned char)src[pos + 2] == 0x8D;
}

static int is_utf8_not(const char *src, size_t pos, size_t len) {
	return pos + 1 < len &&
	       (unsigned char)src[pos] == 0xC2 &&
	       (unsigned char)src[pos + 1] == 0xAC;
}

static int is_utf8_not_alt(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x8C &&
	       (unsigned char)src[pos + 2] == 0x90;
}

static int is_utf8_implies(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x86 &&
	       (unsigned char)src[pos + 2] == 0x92;
}

static int is_utf8_iff(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x86 &&
	       (unsigned char)src[pos + 2] == 0x94;
}

static int is_utf8_equiv(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x89 &&
	       (unsigned char)src[pos + 2] == 0xA3;
}

static int is_utf8_equiv_alt(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x89 &&
	       (unsigned char)src[pos + 2] == 0xA1;
}

static int is_utf8_infinity(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x88 &&
	       (unsigned char)src[pos + 2] == 0x9E;
}

static int is_utf8_econst(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x84 &&
	       (unsigned char)src[pos + 2] == 0xAF;
}

static int is_utf8_sqrt(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE2 &&
	       (unsigned char)src[pos + 1] == 0x88 &&
	       (unsigned char)src[pos + 2] == 0x9A;
}

static int is_utf8_ln(const char *src, size_t pos, size_t len) {
	return pos + 2 < len &&
	       (unsigned char)src[pos] == 0xE3 &&
	       (unsigned char)src[pos + 1] == 0x8F &&
	       (unsigned char)src[pos + 2] == 0x91;
}

static void set_number_token(Lexer *lexer, double number, size_t start, size_t length) {
	lexer->current.kind = TOK_NUMBER;
	lexer->current.start = lexer->src + start;
	lexer->current.length = length;
	lexer->current.pos = start;
	lexer->current.number = number;
}

void lexer_init(Lexer *lexer, const char *src) {
	lexer->src = src;
	lexer->len = strlen(src);
	lexer->pos = 0;
	lexer->current.kind = TOK_INVALID;
	lexer->current.start = src;
	lexer->current.length = 0;
	lexer->current.pos = 0;
	lexer->current.number = 0.0;
	lexer_next(lexer);
}

void lexer_next(Lexer *lexer) {
	const char *src = lexer->src;

	for (;;) {
		while (lexer->pos < lexer->len && isspace((unsigned char)src[lexer->pos])) {
			lexer->pos++;
		}

		if (lexer->pos < lexer->len && src[lexer->pos] == ';') {
			while (lexer->pos < lexer->len && src[lexer->pos] != '\n') {
				lexer->pos++;
			}
			continue;
		}

		if (lexer->pos + 1 < lexer->len && src[lexer->pos] == '-' && src[lexer->pos + 1] == '-') {
			lexer->pos += 2;
			while (lexer->pos < lexer->len && src[lexer->pos] != '\n') {
				lexer->pos++;
			}
			continue;
		}

		break;
	}

	lexer->current.start = src + lexer->pos;
	lexer->current.length = 0;
	lexer->current.pos = lexer->pos;
	lexer->current.number = 0.0;

	if (lexer->pos >= lexer->len) {
		lexer->current.kind = TOK_EOF;
		return;
	}

	if (is_utf8_lambda(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_LAMBDA;
		lexer->current.length = 2;
		lexer->pos += 2;
		return;
	}

	if (is_utf8_sigma(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_SIGMA;
		lexer->current.length = 2;
		lexer->pos += 2;
		return;
	}

	if (is_utf8_integral(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_INTEGRAL;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_forall(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_FORALL;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_exists(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_EXISTS;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_exists_alt_upper(src, lexer->pos, lexer->len) ||
	    is_utf8_exists_alt_lower(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_EXISTS;
		lexer->current.length = 2;
		lexer->pos += 2;
		return;
	}

	if (is_utf8_sqrt(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_SQRT;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_ln(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_LN;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_infinity(src, lexer->pos, lexer->len)) {
		set_number_token(lexer, INFINITY, lexer->pos, 3);
		lexer->pos += 3;
		return;
	}

	if (is_utf8_econst(src, lexer->pos, lexer->len)) {
		set_number_token(lexer, exp(1.0), lexer->pos, 3);
		lexer->pos += 3;
		return;
	}

	if (lexer->pos + 2 < lexer->len &&
	    src[lexer->pos] == '<' &&
	    src[lexer->pos + 1] == '-' &&
	    src[lexer->pos + 2] == '>') {
		lexer->current.kind = TOK_IFF;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (lexer->pos + 2 < lexer->len &&
	    src[lexer->pos] == '<' &&
	    src[lexer->pos + 1] == '=' &&
	    src[lexer->pos + 2] == '>') {
		lexer->current.kind = TOK_EQUIV;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (lexer->pos + 1 < lexer->len &&
	    src[lexer->pos] == '-' &&
	    src[lexer->pos + 1] == '>') {
		lexer->current.kind = TOK_IMPLIES;
		lexer->current.length = 2;
		lexer->pos += 2;
		return;
	}

	if (is_utf8_and(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_AND;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_or(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_OR;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_element_of(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_IN;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_contains(src, lexer->pos, lexer->len) ||
	    is_utf8_contains_alt(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_CONTAINS;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_not(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_NOT;
		lexer->current.length = 2;
		lexer->pos += 2;
		return;
	}

	if (is_utf8_not_alt(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_NOT;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_implies(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_IMPLIES;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_iff(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_IFF;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (is_utf8_equiv(src, lexer->pos, lexer->len) || is_utf8_equiv_alt(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_EQUIV;
		lexer->current.length = 3;
		lexer->pos += 3;
		return;
	}

	if (isdigit((unsigned char)src[lexer->pos])) {
		char *end = NULL;
		double number;
		size_t start = lexer->pos;

		number = strtod(src + lexer->pos, &end);
		lexer->pos = (size_t)(end - src);
		set_number_token(lexer, number, start, lexer->pos - start);
		return;
	}

	if (lexer->pos + 1 < lexer->len &&
	    src[lexer->pos] == '*' &&
	    src[lexer->pos + 1] == '*') {
		lexer->current.kind = TOK_POWER;
		lexer->current.length = 2;
		lexer->pos += 2;
		return;
	}

	switch ((unsigned char)src[lexer->pos]) {
	case '\\':
		lexer->current.kind = TOK_LAMBDA;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case '.':
		lexer->current.kind = TOK_DOT;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case ':':
		lexer->current.kind = TOK_COLON;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case '(':
		lexer->current.kind = TOK_LPAREN;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case ')':
		lexer->current.kind = TOK_RPAREN;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case ',':
		lexer->current.kind = TOK_COMMA;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case '~':
		lexer->current.kind = TOK_NOT;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case '^':
		lexer->current.kind = TOK_AND;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case '+':
		lexer->current.kind = TOK_PLUS;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case '-':
		lexer->current.kind = TOK_MINUS;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case '*':
		lexer->current.kind = TOK_STAR;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case '/':
		lexer->current.kind = TOK_SLASH;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case '%':
		lexer->current.kind = TOK_PERCENT;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	case '=':
		lexer->current.kind = TOK_EQUAL;
		lexer->current.length = 1;
		lexer->pos++;
		return;
	default:
		break;
	}

	if (is_ident_start((unsigned char)src[lexer->pos])) {
		size_t start = lexer->pos;

		lexer->pos++;
		while (lexer->pos < lexer->len && is_ident_part((unsigned char)src[lexer->pos])) {
			lexer->pos++;
		}

		lexer->current.start = src + start;
		lexer->current.length = lexer->pos - start;
		lexer->current.pos = start;

		if (lexer->current.length == 1 && lexer->current.start[0] == 'v') {
			lexer->current.kind = TOK_OR;
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "sqrt")) {
			lexer->current.kind = TOK_SQRT;
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "ln")) {
			lexer->current.kind = TOK_LN;
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "Sigma") ||
		    ident_equals(lexer->current.start, lexer->current.length, "sigma") ||
		    ident_equals(lexer->current.start, lexer->current.length, "SIGMA")) {
			lexer->current.kind = TOK_SIGMA;
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "Integral") ||
		    ident_equals(lexer->current.start, lexer->current.length, "integral") ||
		    ident_equals(lexer->current.start, lexer->current.length, "INTEGRAL")) {
			lexer->current.kind = TOK_INTEGRAL;
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "lim") ||
		    ident_equals(lexer->current.start, lexer->current.length, "Limit") ||
		    ident_equals(lexer->current.start, lexer->current.length, "LIMIT") ||
		    ident_equals(lexer->current.start, lexer->current.length, "limit")) {
			lexer->current.kind = TOK_LIMIT;
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "forall") ||
		    ident_equals(lexer->current.start, lexer->current.length, "FORALL")) {
			lexer->current.kind = TOK_FORALL;
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "exists") ||
		    ident_equals(lexer->current.start, lexer->current.length, "EXISTS")) {
			lexer->current.kind = TOK_EXISTS;
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "to") ||
		    ident_equals(lexer->current.start, lexer->current.length, "TO")) {
			lexer->current.kind = TOK_TO;
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "in") ||
		    ident_equals(lexer->current.start, lexer->current.length, "IN") ||
		    ident_equals(lexer->current.start, lexer->current.length, "memberof") ||
		    ident_equals(lexer->current.start, lexer->current.length, "memberOf") ||
		    ident_equals(lexer->current.start, lexer->current.length, "MEMBEROF") ||
		    ident_equals(lexer->current.start, lexer->current.length, "elementof") ||
		    ident_equals(lexer->current.start, lexer->current.length, "elementOf") ||
		    ident_equals(lexer->current.start, lexer->current.length, "ELEMENTOF") ||
		    ident_equals(lexer->current.start, lexer->current.length, "elemof") ||
		    ident_equals(lexer->current.start, lexer->current.length, "elemOf") ||
		    ident_equals(lexer->current.start, lexer->current.length, "ELEMOF")) {
			lexer->current.kind = TOK_IN;
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "contains") ||
		    ident_equals(lexer->current.start, lexer->current.length, "CONTAINS") ||
		    ident_equals(lexer->current.start, lexer->current.length, "hasmember") ||
		    ident_equals(lexer->current.start, lexer->current.length, "hasMember") ||
		    ident_equals(lexer->current.start, lexer->current.length, "HASMEMBER")) {
			lexer->current.kind = TOK_CONTAINS;
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "inf") ||
		    ident_equals(lexer->current.start, lexer->current.length, "INF") ||
		    ident_equals(lexer->current.start, lexer->current.length, "infinity") ||
		    ident_equals(lexer->current.start, lexer->current.length, "INFINITY")) {
			set_number_token(lexer, INFINITY, start, lexer->current.length);
			return;
		}
		if (ident_equals(lexer->current.start, lexer->current.length, "euler") ||
		    ident_equals(lexer->current.start, lexer->current.length, "EULER")) {
			set_number_token(lexer, exp(1.0), start, lexer->current.length);
			return;
		}

		lexer->current.kind = TOK_IDENT;
		return;
	}

	lexer->current.kind = TOK_INVALID;
	lexer->current.length = 1;
	lexer->pos++;
}
