#ifndef LS_LEXER_H
#define LS_LEXER_H

#include <stddef.h>

typedef enum {
	TOK_INVALID = 0,
	TOK_EOF,
	TOK_IDENT,
	TOK_NUMBER,
	TOK_LAMBDA,
	TOK_DOT,
	TOK_COLON,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_COMMA,
	TOK_NOT,
	TOK_AND,
	TOK_OR,
	TOK_IMPLIES,
	TOK_IFF,
	TOK_EQUIV,
	TOK_PLUS,
	TOK_MINUS,
	TOK_STAR,
	TOK_SLASH,
	TOK_PERCENT,
	TOK_POWER,
	TOK_EQUAL,
	TOK_SQRT,
	TOK_LN,
	TOK_SIGMA,
	TOK_INTEGRAL,
	TOK_LIMIT,
	TOK_FORALL,
	TOK_EXISTS,
	TOK_TO,
	TOK_IN,
	TOK_CONTAINS
} TokenKind;

typedef struct {
	TokenKind kind;
	const char *start;
	size_t length;
	size_t pos;
	double number;
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