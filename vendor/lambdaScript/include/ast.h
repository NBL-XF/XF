#ifndef LS_AST_H
#define LS_AST_H

typedef enum {
	AST_VAR,
	AST_LAM,
	AST_APP
} AstKind;

typedef struct Ast Ast;
typedef struct AstDef AstDef;
typedef struct Program Program;

struct Ast {
	AstKind kind;
	union {
		struct {
			char *name;
		} var;
		struct {
			char *param;
			Ast *body;
		} lam;
		struct {
			Ast *fn;
			Ast *arg;
		} app;
	} as;
};

struct AstDef {
	char *name;
	Ast *expr;
	AstDef *next;
};

struct Program {
	AstDef *defs;
	AstDef *defs_tail;
	Ast *expr;
};

Ast *ast_var_new(const char *name);
Ast *ast_lam_new(const char *param, Ast *body);
Ast *ast_app_new(Ast *fn, Ast *arg);
void ast_free(Ast *node);

Program *program_new(void);
int program_add_def(Program *program, const char *name, Ast *expr);
void program_free(Program *program);

#endif