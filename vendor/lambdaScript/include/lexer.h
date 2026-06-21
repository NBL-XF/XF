#ifndef LS_LEXER_H
#define LS_LEXER_H

#include <stddef.h>

typedef enum {
	TOK_INVALID,
	TOK_LAMBDA,
	TOK_DOT,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_NOT,
	TOK_AND,
	TOK_OR,
	TOK_IMPLIES,
	TOK_IFF,
	TOK_IDENT,
	TOK_EOF
} TokenKind;

typedef struct {
	TokenKind kind;
	const char *start;
	size_t length;
	size_t pos;
} Token;

typedef struct {
	const char *src;
	size_t len;
	size_t pos;
	Token current;
} Lexer;

void lexer_init(Lexer *lexer, const char *src);
void lexer_next(Lexer *lexer);

#endif