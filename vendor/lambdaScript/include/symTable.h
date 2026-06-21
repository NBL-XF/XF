#ifndef LS_SYMTABLE_H
#define LS_SYMTABLE_H

#include <stddef.h>

typedef struct {
	char **items;
	size_t count;
	size_t capacity;
} SymTable;

void sym_table_init(SymTable *table);
void sym_table_free(SymTable *table);
int sym_table_push(SymTable *table, const char *name);
void sym_table_pop(SymTable *table);
int sym_table_lookup(const SymTable *table, const char *name, size_t *index_out);

#endif