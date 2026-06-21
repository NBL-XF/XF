#include "../include/lexer.h"

#include <ctype.h>
#include <string.h>

static int is_ident_start(unsigned char c) {
	return isalpha(c) || c == '_';
}

static int is_ident_part(unsigned char c) {
	return isalnum(c) || c == '_';
}

static int is_utf8_lambda(const char *src, size_t pos, size_t len) {
	return pos + 1 < len &&
	       (unsigned char)src[pos] == 0xCE &&
	       (unsigned char)src[pos + 1] == 0xBB;
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

static int is_utf8_not(const char *src, size_t pos, size_t len) {
	return pos + 1 < len &&
	       (unsigned char)src[pos] == 0xC2 &&
	       (unsigned char)src[pos + 1] == 0xAC;
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

void lexer_init(Lexer *lexer, const char *src) {
	lexer->src = src;
	lexer->len = strlen(src);
	lexer->pos = 0;
	lexer->current.kind = TOK_INVALID;
	lexer->current.start = src;
	lexer->current.length = 0;
	lexer->current.pos = 0;
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

	if (lexer->pos + 2 < lexer->len &&
	    src[lexer->pos] == '<' &&
	    src[lexer->pos + 1] == '-' &&
	    src[lexer->pos + 2] == '>') {
		lexer->current.kind = TOK_IFF;
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

	if (is_utf8_not(src, lexer->pos, lexer->len)) {
		lexer->current.kind = TOK_NOT;
		lexer->current.length = 2;
		lexer->pos += 2;
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
		} else {
			lexer->current.kind = TOK_IDENT;
		}
		return;
	}

	lexer->current.kind = TOK_INVALID;
	lexer->current.length = 1;
	lexer->pos++;
}