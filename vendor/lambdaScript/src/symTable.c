#include "../include/symTable.h"

#include <stdlib.h>
#include <string.h>

static char *sym_strdup(const char *src) {
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

void sym_table_init(SymTable *table) {
	table->items = NULL;
	table->count = 0;
	table->capacity = 0;
}

void sym_table_free(SymTable *table) {
	size_t i;

	if (table == NULL) {
		return;
	}

	for (i = 0; i < table->count; i++) {
		free(table->items[i]);
	}

	free(table->items);
	table->items = NULL;
	table->count = 0;
	table->capacity = 0;
}

int sym_table_push(SymTable *table, const char *name) {
	char **new_items;
	char *copy;
	size_t new_capacity;

	if (table->count == table->capacity) {
		new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
		new_items = (char **)realloc(table->items, new_capacity * sizeof(*new_items));
		if (new_items == NULL) {
			return 0;
		}
		table->items = new_items;
		table->capacity = new_capacity;
	}

	copy = sym_strdup(name);
	if (copy == NULL) {
		return 0;
	}

	table->items[table->count++] = copy;
	return 1;
}

void sym_table_pop(SymTable *table) {
	if (table->count == 0) {
		return;
	}

	table->count--;
	free(table->items[table->count]);
	table->items[table->count] = NULL;
}

int sym_table_lookup(const SymTable *table, const char *name, size_t *index_out) {
	size_t i;

	for (i = table->count; i > 0; i--) {
		if (strcmp(table->items[i - 1], name) == 0) {
			*index_out = table->count - i;
			return 1;
		}
	}

	return 0;
}