#include "../include/ast.h"

#include <stdlib.h>
#include <string.h>

static char *ast_strdup(const char *src) {
	size_t len;
	char *dst;

	if (src == NULL) {
		return NULL;
	}

	len = strlen(src);
	dst = (char *)malloc(len + 1);
	if (dst == NULL) {
		return NULL;
	}

	memcpy(dst, src, len + 1);
	return dst;
}

Ast *ast_var_new(const char *name) {
	Ast *node = (Ast *)calloc(1, sizeof(*node));
	if (node == NULL) {
		return NULL;
	}

	node->kind = AST_VAR;
	node->as.var.name = ast_strdup(name);
	if (node->as.var.name == NULL) {
		free(node);
		return NULL;
	}

	return node;
}

Ast *ast_lam_new(const char *param, Ast *body) {
	Ast *node = (Ast *)calloc(1, sizeof(*node));
	if (node == NULL || body == NULL) {
		free(node);
		return NULL;
	}

	node->kind = AST_LAM;
	node->as.lam.param = ast_strdup(param);
	node->as.lam.body = body;

	if (node->as.lam.param == NULL) {
		ast_free(body);
		free(node);
		return NULL;
	}

	return node;
}

Ast *ast_app_new(Ast *fn, Ast *arg) {
	Ast *node = (Ast *)calloc(1, sizeof(*node));
	if (node == NULL || fn == NULL || arg == NULL) {
		free(node);
		return NULL;
	}

	node->kind = AST_APP;
	node->as.app.fn = fn;
	node->as.app.arg = arg;
	return node;
}

void ast_free(Ast *node) {
	if (node == NULL) {
		return;
	}

	switch (node->kind) {
	case AST_VAR:
		free(node->as.var.name);
		break;
	case AST_LAM:
		free(node->as.lam.param);
		ast_free(node->as.lam.body);
		break;
	case AST_APP:
		ast_free(node->as.app.fn);
		ast_free(node->as.app.arg);
		break;
	}

	free(node);
}

Program *program_new(void) {
	Program *program = (Program *)calloc(1, sizeof(*program));
	return program;
}

int program_add_def(Program *program, const char *name, Ast *expr) {
	AstDef *def;

	if (program == NULL || name == NULL || expr == NULL) {
		return 0;
	}

	def = (AstDef *)calloc(1, sizeof(*def));
	if (def == NULL) {
		return 0;
	}

	def->name = ast_strdup(name);
	def->expr = expr;
	def->next = NULL;

	if (def->name == NULL) {
		ast_free(expr);
		free(def);
		return 0;
	}

	if (program->defs_tail != NULL) {
		program->defs_tail->next = def;
	} else {
		program->defs = def;
	}
	program->defs_tail = def;
	return 1;
}

void program_free(Program *program) {
	AstDef *def;
	AstDef *next;

	if (program == NULL) {
		return;
	}

	def = program->defs;
	while (def != NULL) {
		next = def->next;
		free(def->name);
		ast_free(def->expr);
		free(def);
		def = next;
	}

	ast_free(program->expr);
	free(program);
}