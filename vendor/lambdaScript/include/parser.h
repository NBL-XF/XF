#ifndef LS_PARSER_H
#define LS_PARSER_H

#include "../include/ast.h"

Ast *parser_parse_expr(const char *source);
Program *parser_parse_program(const char *source);

#endif