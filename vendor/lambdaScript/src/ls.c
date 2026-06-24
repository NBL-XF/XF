#include "../include/ls.h"

#include "../include/err.h"
#include "../include/interp.h"
#include "../include/parser.h"
#include "../include/value.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

struct ls_State {
	int reserved;
};

typedef struct {
	char *data;
	size_t length;
	size_t capacity;
} LsBuf;

static char *ls_strdup(const char *src);
static int buf_append_n(LsBuf *buf, const char *text, size_t len);
static int buf_append(LsBuf *buf, const char *text);
static char *read_file_text(const char *path);


typedef struct DefSeen DefSeen;
struct DefSeen {
	char *name;
	char *path;
	size_t line;
	DefSeen *next;
};

typedef struct ImportStack ImportStack;
struct ImportStack {
	const char *path;
	ImportStack *next;
};

static void defseen_free_all(DefSeen *seen) {
	DefSeen *next;
	while (seen != NULL) {
		next = seen->next;
		free(seen->name);
		free(seen->path);
		free(seen);
		seen = next;
	}
}

static int pp_is_ident_start_byte(unsigned char c) {
	return isalpha(c) || c == '_' || c >= 0x80;
}

static int pp_is_ident_part_byte(unsigned char c) {
	return isalnum(c) || c == '_' || c >= 0x80;
}

static int pp_is_ident_span(const char *start, size_t length) {
	size_t i;

	if (start == NULL || length == 0) {
		return 0;
	}
	if (!pp_is_ident_start_byte((unsigned char)start[0])) {
		return 0;
	}
	for (i = 1; i < length; i++) {
		if (!pp_is_ident_part_byte((unsigned char)start[i])) {
			return 0;
		}
	}
	return 1;
}

typedef struct {
	int is_definition;
	char *name;
} PreDefScan;

static int pp_parse_definition_name(const char *line, PreDefScan *out) {
	const char *p = line;
	const char *lhs_start;
	const char *lhs_end;
	const char *eq;

	out->is_definition = 0;
	out->name = NULL;

	while (*p != '\0' && isspace((unsigned char)*p)) {
		p++;
	}
	if (*p == '\0' || *p == ';' || (*p == '-' && p[1] == '-')) {
		return 1;
	}

	lhs_start = p;
	eq = strchr(p, '=');
	while (eq != NULL) {
		if (!(eq > lhs_start && eq[-1] == '<' && eq[1] == '>')) {
			break;
		}
		eq = strchr(eq + 1, '=');
	}

	if (eq == NULL) {
		return 1;
	}

	lhs_end = eq;
	if (eq > lhs_start) {
		switch (eq[-1]) {
		case '+':
		case '-':
		case '*':
		case '/':
		case '%':
			lhs_end = eq - 1;
			break;
		default:
			break;
		}
	}

	while (lhs_end > lhs_start && isspace((unsigned char)lhs_end[-1])) {
		lhs_end--;
	}

	if (!pp_is_ident_span(lhs_start, (size_t)(lhs_end - lhs_start))) {
		return 1;
	}

	out->name = ls_strdup("");
	if (out->name == NULL) {
		return 0;
	}
	free(out->name);
	out->name = (char *)malloc((size_t)(lhs_end - lhs_start) + 1);
	if (out->name == NULL) {
		return 0;
	}
	memcpy(out->name, lhs_start, (size_t)(lhs_end - lhs_start));
	out->name[lhs_end - lhs_start] = '\0';
	out->is_definition = 1;
	return 1;
}

static int defseen_add_unique(DefSeen **seen, const char *name, const char *path, size_t line) {
	DefSeen *it;
	DefSeen *node;

	for (it = *seen; it != NULL; it = it->next) {
		if (strcmp(it->name, name) == 0) {
			err_set("duplicate definition '%s' at %s:%zu (already defined at %s:%zu)",
			        name, path, line, it->path, it->line);
			return 0;
		}
	}

	node = (DefSeen *)calloc(1, sizeof(*node));
	if (node == NULL) {
		err_set("out of memory");
		return 0;
	}

	node->name = ls_strdup(name);
	node->path = ls_strdup(path != NULL ? path : "<input>");
	node->line = line;

	if (node->name == NULL || node->path == NULL) {
		free(node->name);
		free(node->path);
		free(node);
		err_set("out of memory");
		return 0;
	}

	node->next = *seen;
	*seen = node;
	return 1;
}

static int scan_duplicate_definitions_in_source(const char *source) {
	const char *line_start = source;
	const char *cursor = source;
	size_t line_no = 1;
	DefSeen *seen = NULL;

	while (1) {
		if (*cursor != '\n' && *cursor != '\0') {
			cursor++;
			continue;
		}

		{
			char *raw_line = (char *)malloc((size_t)(cursor - line_start) + 1);
			PreDefScan scan;
			int ok;

			if (raw_line == NULL) {
				defseen_free_all(seen);
				err_set("out of memory");
				return 0;
			}

			memcpy(raw_line, line_start, (size_t)(cursor - line_start));
			raw_line[cursor - line_start] = '\0';

			ok = pp_parse_definition_name(raw_line, &scan);
			free(raw_line);
			if (!ok) {
				defseen_free_all(seen);
				err_set("out of memory");
				return 0;
			}

			if (scan.is_definition) {
				ok = defseen_add_unique(&seen, scan.name, "<merged>", line_no);
				free(scan.name);
				if (!ok) {
					defseen_free_all(seen);
					err_set("line %zu: %s", line_no, err_get());
					return 0;
				}
			}
		}

		if (*cursor == '\0') {
			break;
		}
		cursor++;
		line_start = cursor;
		line_no++;
	}

	defseen_free_all(seen);
	return 1;
}

static int parse_import_line(const char *trimmed, char **out_path) {
	const char *p = trimmed;
	const char *start;
	const char *end;
	size_t len;
	char *path;

	*out_path = NULL;

	if (strncmp(p, "import", 6) != 0 || !isspace((unsigned char)p[6])) {
		return 0;
	}

	p += 6;
	while (*p != '\0' && isspace((unsigned char)*p)) {
		p++;
	}

	if (*p != '"') {
		return -1;
	}
	p++;
	start = p;

	while (*p != '\0' && *p != '"') {
		if (*p == '\\' && p[1] != '\0') {
			p += 2;
			continue;
		}
		p++;
	}

	if (*p != '"') {
		return -1;
	}

	end = p;
	p++;

	while (*p != '\0' && isspace((unsigned char)*p)) {
		p++;
	}

	if (*p != '\0') {
		return -1;
	}

	len = (size_t)(end - start);
	path = (char *)malloc(len + 1);
	if (path == NULL) {
		err_set("out of memory");
		return -1;
	}

	memcpy(path, start, len);
	path[len] = '\0';
	*out_path = path;
	return 1;
}

static char *dirname_from_path(const char *path) {
	const char *slash;
	size_t len;
	char *dir;

	slash = strrchr(path, '/');
	if (slash == NULL) {
		return ls_strdup(".");
	}

	len = (size_t)(slash - path);
	if (len == 0) {
		return ls_strdup("/");
	}

	dir = (char *)malloc(len + 1);
	if (dir == NULL) {
		return NULL;
	}
	memcpy(dir, path, len);
	dir[len] = '\0';
	return dir;
}

static char *join_path(const char *dir, const char *rel) {
	size_t dlen = strlen(dir);
	size_t rlen = strlen(rel);
	char *out = (char *)malloc(dlen + 1 + rlen + 1);

	if (out == NULL) {
		return NULL;
	}

	memcpy(out, dir, dlen);
	out[dlen] = '/';
	memcpy(out + dlen + 1, rel, rlen);
	out[dlen + 1 + rlen] = '\0';
	return out;
}

static char *resolve_import_path(const char *importing_name, const char *import_path) {
	char *dir;
	char *joined;

	if (import_path[0] == '/') {
		return ls_strdup(import_path);
	}

	if (importing_name == NULL || importing_name[0] == '<') {
		err_set("relative import '%s' requires a file-backed source", import_path);
		return NULL;
	}

	dir = dirname_from_path(importing_name);
	if (dir == NULL) {
		err_set("out of memory");
		return NULL;
	}

	joined = join_path(dir, import_path);
	free(dir);
	if (joined == NULL) {
		err_set("out of memory");
		return NULL;
	}

#if defined(_POSIX_VERSION)
	resolved = realpath(joined, NULL);
	if (resolved != NULL) {
		free(joined);
		return resolved;
	}
#endif

	return joined;
}

static int import_stack_contains(const ImportStack *stack, const char *path) {
	while (stack != NULL) {
		if (strcmp(stack->path, path) == 0) {
			return 1;
		}
		stack = stack->next;
	}
	return 0;
}

static int preprocess_source_recursive(const char *source,
                                       const char *source_name,
                                       int library_mode,
                                       DefSeen **seen_defs,
                                       const ImportStack *stack,
                                       LsBuf *out);

static int append_line_with_newline(LsBuf *out, const char *start, size_t len) {
	return buf_append_n(out, start, len) && buf_append(out, "\n");
}

static int preprocess_source_recursive(const char *source,
                                       const char *source_name,
                                       int library_mode,
                                       DefSeen **seen_defs,
                                       const ImportStack *stack,
                                       LsBuf *out) {
	const char *line_start = source;
	const char *cursor = source;
	size_t line_no = 1;

	while (1) {
		char *raw_line;
		char *trimmed;
		char *import_path = NULL;
		int import_kind;
		PreDefScan scan;
		int ok;

		if (*cursor != '\n' && *cursor != '\0') {
			cursor++;
			continue;
		}

		raw_line = (char *)malloc((size_t)(cursor - line_start) + 1);
		if (raw_line == NULL) {
			err_set("out of memory");
			return 0;
		}
		memcpy(raw_line, line_start, (size_t)(cursor - line_start));
		raw_line[cursor - line_start] = '\0';

		trimmed = raw_line;
		while (*trimmed != '\0' && isspace((unsigned char)*trimmed)) {
			trimmed++;
		}
		{
			size_t len = strlen(trimmed);
			while (len > 0 && isspace((unsigned char)trimmed[len - 1])) {
				trimmed[--len] = '\0';
			}
		}

		if (trimmed[0] == '\0' || trimmed[0] == ';' || (trimmed[0] == '-' && trimmed[1] == '-')) {
			ok = append_line_with_newline(out, line_start, (size_t)(cursor - line_start));
			free(raw_line);
			if (!ok) {
				err_set("out of memory");
				return 0;
			}
		} else {
			import_kind = parse_import_line(trimmed, &import_path);
			if (import_kind == 1) {
				char *resolved = resolve_import_path(source_name, import_path);
				char *child_source;
				ImportStack child_stack;

				free(import_path);
				import_path = NULL;

				if (resolved == NULL) {
					free(raw_line);
					return 0;
				}
				if (import_stack_contains(stack, resolved)) {
					err_set("import cycle detected involving '%s'", resolved);
					free(resolved);
					free(raw_line);
					return 0;
				}

				child_source = read_file_text(resolved);
				if (child_source == NULL) {
					err_set("failed to read import '%s'", resolved);
					free(resolved);
					free(raw_line);
					return 0;
				}

				child_stack.path = resolved;
				child_stack.next = (ImportStack *)stack;
				ok = preprocess_source_recursive(child_source, resolved, 1, seen_defs, &child_stack, out);
				free(child_source);
				free(resolved);
				free(raw_line);
				if (!ok) {
					return 0;
				}
			} else {
				if (import_kind < 0) {
					err_set("%s:%zu: invalid import syntax", source_name != NULL ? source_name : "<input>", line_no);
					free(raw_line);
					return 0;
				}

				scan.is_definition = 0;
				scan.name = NULL;
				ok = pp_parse_definition_name(trimmed, &scan);
				if (!ok) {
					free(raw_line);
					err_set("out of memory");
					return 0;
				}

				if (scan.is_definition) {
					ok = defseen_add_unique(seen_defs, scan.name, source_name != NULL ? source_name : "<input>", line_no);
					free(scan.name);
					if (!ok) {
						free(raw_line);
						return 0;
					}
				} else if (library_mode) {
					err_set("%s:%zu: imported files may only contain definitions, comments, blank lines, and imports",
					        source_name != NULL ? source_name : "<input>", line_no);
					free(raw_line);
					return 0;
				}

				ok = append_line_with_newline(out, line_start, (size_t)(cursor - line_start));
				free(raw_line);
				if (!ok) {
					err_set("out of memory");
					return 0;
				}
			}
		}

		if (*cursor == '\0') {
			break;
		}
		cursor++;
		line_start = cursor;
		line_no++;
	}

	return 1;
}

static char *preprocess_imports(const char *source, const char *source_name) {
	LsBuf out = {0};
	DefSeen *seen = NULL;
	ImportStack root;

	root.path = source_name != NULL ? source_name : "<input>";
	root.next = NULL;

	if (!preprocess_source_recursive(source, source_name, 0, &seen, &root, &out)) {
		defseen_free_all(seen);
		free(out.data);
		return NULL;
	}

	defseen_free_all(seen);
	if (out.data == NULL) {
		return ls_strdup("");
	}
	return out.data;
}

static const char *LS_PRELUDE =
	"I = \\x.x\n"
	"K = \\x.\\y.x\n"
	"S = \\f.\\g.\\x.f x (g x)\n"
	"TRUE = \\t.\\f.t\n"
	"FALSE = \\t.\\f.f\n"
	"AND = \\p.\\q.p q FALSE\n"
	"OR = \\p.\\q.p TRUE q\n"
	"NOT = \\p.p FALSE TRUE\n"
	"IMP = \\p.\\q.OR (NOT p) q\n"
	"IFF = \\p.\\q.AND (IMP p q) (IMP q p)\n";

static char *ls_strdup(const char *src) {
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

static int buf_reserve(LsBuf *buf, size_t needed) {
	char *new_data;
	size_t new_capacity = buf->capacity == 0 ? 64 : buf->capacity;

	while (new_capacity < needed) {
		new_capacity *= 2;
	}

	new_data = (char *)realloc(buf->data, new_capacity);
	if (new_data == NULL) {
		return 0;
	}

	buf->data = new_data;
	buf->capacity = new_capacity;
	return 1;
}

static int buf_append_n(LsBuf *buf, const char *text, size_t len) {
	if (!buf_reserve(buf, buf->length + len + 1)) {
		return 0;
	}

	memcpy(buf->data + buf->length, text, len);
	buf->length += len;
	buf->data[buf->length] = '\0';
	return 1;
}

static int buf_append(LsBuf *buf, const char *text) {
	return buf_append_n(buf, text, strlen(text));
}

static int buf_append_value_line(LsBuf *buf, const Value *value, const char *prefix) {
	char *printed;

	if (prefix != NULL && !buf_append(buf, prefix)) {
		return 0;
	}

	printed = value_to_string(value);
	if (printed == NULL) {
		return 0;
	}

	if (!buf_append(buf, printed) || !buf_append(buf, "\n")) {
		free(printed);
		return 0;
	}

	free(printed);
	return 1;
}

char *ls_read_all_stream(FILE *fp) {
	char *buffer = NULL;
	size_t length = 0;
	size_t capacity = 0;

	for (;;) {
		int ch = fgetc(fp);
		char *new_buffer;

		if (ch == EOF) {
			break;
		}

		if (length + 1 >= capacity) {
			size_t new_capacity = capacity == 0 ? 256 : capacity * 2;
			new_buffer = (char *)realloc(buffer, new_capacity);
			if (new_buffer == NULL) {
				free(buffer);
				return NULL;
			}
			buffer = new_buffer;
			capacity = new_capacity;
		}

		buffer[length++] = (char)ch;
	}

	if (buffer == NULL) {
		buffer = (char *)malloc(1);
		if (buffer == NULL) {
			return NULL;
		}
	}

	buffer[length] = '\0';
	return buffer;
}

static char *read_file_text(const char *path) {
	FILE *fp;
	char *source;

	fp = fopen(path, "rb");
	if (fp == NULL) {
		return NULL;
	}

	source = ls_read_all_stream(fp);
	fclose(fp);
	return source;
}

static int ls_is_ident_text(const char *s) {
	const unsigned char *p;

	if (s == NULL || s[0] == '\0') {
		return 0;
	}

	if (!(isalpha((unsigned char)s[0]) || s[0] == '_')) {
		return 0;
	}

	for (p = (const unsigned char *)s + 1; *p != '\0'; p++) {
		if (!(isalnum(*p) || *p == '_')) {
			return 0;
		}
	}

	return 1;
}

static char *ls_symbol_from_text(const char *text) {
	static const char hex[] = "0123456789ABCDEF";
	size_t len;
	char *out;
	size_t i;
	const unsigned char *bytes;

	if (text == NULL) {
		text = "";
	}

	if (ls_is_ident_text(text)) {
		return ls_strdup(text);
	}

	len = strlen(text);
	if (len == 0) {
		return ls_strdup("ARGVAL_EMPTY");
	}

	out = (char *)malloc(strlen("ARGVAL_") + len * 2 + 1);
	if (out == NULL) {
		return NULL;
	}

	memcpy(out, "ARGVAL_", strlen("ARGVAL_"));
	bytes = (const unsigned char *)text;
	for (i = 0; i < len; i++) {
		out[strlen("ARGVAL_") + i * 2] = hex[(bytes[i] >> 4) & 0xF];
		out[strlen("ARGVAL_") + i * 2 + 1] = hex[bytes[i] & 0xF];
	}
	out[strlen("ARGVAL_") + len * 2] = '\0';
	return out;
}

static int buf_append_church_numeral(LsBuf *buf, size_t n) {
	size_t i;

	if (!buf_append(buf, "\\f.\\x.")) {
		return 0;
	}

	for (i = 0; i < n; i++) {
		if (!buf_append(buf, "f (")) {
			return 0;
		}
	}

	if (!buf_append(buf, "x")) {
		return 0;
	}

	for (i = 0; i < n; i++) {
		if (!buf_append(buf, ")")) {
			return 0;
		}
	}

	return 1;
}

static int buf_append_symbol_def(LsBuf *buf, const char *name, const char *value) {
	char *sym;
	int ok;

	sym = ls_symbol_from_text(value);
	if (sym == NULL) {
		return 0;
	}

	ok = buf_append(buf, name) &&
	     buf_append(buf, " = ") &&
	     buf_append(buf, sym) &&
	     buf_append(buf, "\n");

	free(sym);
	return ok;
}

static int buf_append_arg_builtins(LsBuf *buf, const ls_Options *options) {
	size_t argc = options != NULL ? options->argc : 0;
	const char *const *argv = options != NULL ? options->argv : NULL;
	const char *source_name = options != NULL ? options->source_name : NULL;
	size_t i;

	if (source_name == NULL) {
		source_name = "<eval>";
	}
if (!buf_append(buf,
                "\n"
                "; LambdaScript argument builtins\n"
                "NIL = \\c.\\n.n\n"
                "CONS = \\h.\\t.\\c.\\n.c h t\n")) {
    return 0;
}
	if (!buf_append_symbol_def(buf, "ARG0", source_name)) {
		return 0;
	}

	if (!buf_append(buf, "ARGC = ") || !buf_append_church_numeral(buf, argc) || !buf_append(buf, "\n")) {
		return 0;
	}

	for (i = 0; i < argc; i++) {
		char name[64];
		snprintf(name, sizeof(name), "ARG%zu", i + 1);
		if (!buf_append_symbol_def(buf, name, argv != NULL ? argv[i] : NULL)) {
			return 0;
		}
	}

	if (!buf_append(buf, "ARGS = ")) {
		return 0;
	}

	if (argc == 0) {
		if (!buf_append(buf, "NIL")) {
			return 0;
		}
	} else {
		for (i = 0; i < argc; i++) {
			char name[64];
			snprintf(name, sizeof(name), "ARG%zu", i + 1);
			if (!buf_append(buf, "CONS ") || !buf_append(buf, name) || !buf_append(buf, " (")) {
				return 0;
			}
		}
		if (!buf_append(buf, "NIL")) {
			return 0;
		}
		for (i = 0; i < argc; i++) {
			if (!buf_append(buf, ")")) {
				return 0;
			}
		}
	}

	return buf_append(buf, "\n\n");
}

static size_t count_newlines_n(const char *text, size_t length) {
	size_t i;
	size_t count = 0;

	for (i = 0; i < length; i++) {
		if (text[i] == '\n') {
			count++;
		}
	}

	return count;
}

static char *merge_source_with_builtins(const char *source, const ls_Options *options, size_t *injected_lines) {
	LsBuf buf = {0};
	int use_prelude = options == NULL || options->use_prelude;

	if (injected_lines != NULL) {
		*injected_lines = 0;
	}

	if (use_prelude && !buf_append(&buf, LS_PRELUDE)) {
		free(buf.data);
		return NULL;
	}

	if (!buf_append_arg_builtins(&buf, options)) {
		free(buf.data);
		return NULL;
	}

	if (injected_lines != NULL) {
		*injected_lines = count_newlines_n(buf.data, buf.length);
	}

	if (!buf_append(&buf, source)) {
		free(buf.data);
		return NULL;
	}

	if (buf.data == NULL) {
		return ls_strdup("");
	}

	return buf.data;
}

static void rebase_parse_error_lines(size_t injected_lines) {
	const char *msg;
	size_t line_no;
	char detail[512];

	if (injected_lines == 0) {
		return;
	}

	msg = err_get();
	if (sscanf(msg, "line %zu: %511[^\n]", &line_no, detail) == 2 && line_no > injected_lines) {
		err_set("line %zu: %s", line_no - injected_lines, detail);
	}
}

static Value *reduce_with_trace(const Value *term, size_t max_steps, size_t *steps_taken, int *reached_limit, char **trace_out) {
	Value *current;
	size_t steps = 0;
	bool hit_limit = false;
	LsBuf buf = {0};

	if (steps_taken != NULL) {
		*steps_taken = 0;
	}
	if (reached_limit != NULL) {
		*reached_limit = 0;
	}
	if (trace_out != NULL) {
		*trace_out = NULL;
	}

	err_clear();

	current = value_clone(term);
	if (current == NULL) {
		err_set("out of memory");
		return NULL;
	}

	if (!buf_append_value_line(&buf, current, "[0] ")) {
		value_free(current);
		free(buf.data);
		err_set("out of memory");
		return NULL;
	}

	while (steps < max_steps) {
		Value *next;

		err_clear();
		next = interp_step_normal(current);

		if (next == NULL) {
			if (err_has()) {
				value_free(current);
				free(buf.data);
				return NULL;
			}
			break;
		}

		steps++;
		{
			char prefix[64];
			snprintf(prefix, sizeof(prefix), "[%zu] ", steps);
			if (!buf_append_value_line(&buf, next, prefix)) {
				value_free(current);
				value_free(next);
				free(buf.data);
				err_set("out of memory");
				return NULL;
			}
		}

		value_free(current);
		current = next;
	}

	if (steps == max_steps) {
		Value *probe;

		err_clear();
		probe = interp_step_normal(current);
		if (probe == NULL) {
			if (err_has()) {
				value_free(current);
				free(buf.data);
				return NULL;
			}
		} else {
			hit_limit = true;
			value_free(probe);
		}
	}

	if (reached_limit != NULL) {
		*reached_limit = hit_limit ? 1 : 0;
	}
	if (steps_taken != NULL) {
		*steps_taken = steps;
	}
	if (trace_out != NULL) {
		*trace_out = buf.data;
	} else {
		free(buf.data);
	}

	return current;
}

ls_State *ls_newstate(void) {
	ls_State *L = (ls_State *)calloc(1, sizeof(*L));
	return L;
}

void ls_close(ls_State *L) {
	free(L);
}

void ls_options_init(ls_Options *options) {
	options->max_steps = 100000;
	options->trace = 0;
	options->quiet = 0;
	options->use_prelude = 1;
	options->source_name = NULL;
	options->argc = 0;
	options->argv = NULL;
}

void ls_result_init(ls_Result *result) {
	result->output = NULL;
	result->trace = NULL;
	result->steps = 0;
	result->reached_step_limit = 0;
}

void ls_result_free(ls_Result *result) {
	if (result == NULL) {
		return;
	}

	free(result->output);
	free(result->trace);
	result->output = NULL;
	result->trace = NULL;
	result->steps = 0;
	result->reached_step_limit = 0;
}

int ls_eval_string(ls_State *L, const char *source, const ls_Options *options, ls_Result *result) {
	ls_Options local_options;
	ls_Options effective_options;
	size_t injected_lines = 0;
	char *merged = NULL;
	Program *program = NULL;
	Value *term = NULL;
	Value *reduced = NULL;
	char *rendered = NULL;
	char *trace = NULL;

	(void)L;

	if (source == NULL || result == NULL) {
		err_set("invalid arguments");
		return 0;
	}

	if (options == NULL) {
		ls_options_init(&local_options);
		options = &local_options;
	}

	effective_options = *options;
	if (effective_options.source_name == NULL) {
		effective_options.source_name = "<eval>";
	}
	options = &effective_options;

	ls_result_free(result);

	{
		char *preprocessed = preprocess_imports(source, options->source_name);
		if (preprocessed == NULL) {
			goto fail;
		}
		merged = merge_source_with_builtins(preprocessed, options, &injected_lines);
		free(preprocessed);
	}
	if (merged == NULL) {
		err_set("out of memory");
		goto fail;
	}

	if (!scan_duplicate_definitions_in_source(merged)) {
		rebase_parse_error_lines(injected_lines);
		goto fail;
	}

	program = parser_parse_program(merged);
	if (program == NULL) {
		rebase_parse_error_lines(injected_lines);
		goto fail;
	}

	term = value_from_program(program);
	if (term == NULL) {
		goto fail;
	}

	if (options->trace) {
		reduced = reduce_with_trace(term, options->max_steps, &result->steps, &result->reached_step_limit, &trace);
	} else {
		reduced = interp_reduce_normal(term, options->max_steps, &result->steps, &result->reached_step_limit);
	}

	if (reduced == NULL) {
		goto fail;
	}

	rendered = value_to_string(reduced);
	if (rendered == NULL) {
		goto fail;
	}

	result->output = rendered;
	result->trace = trace;
	rendered = NULL;
	trace = NULL;

	value_free(reduced);
	value_free(term);
	program_free(program);
	free(merged);
	return 1;

fail:
	free(rendered);
	free(trace);
	value_free(reduced);
	value_free(term);
	program_free(program);
	free(merged);
	return 0;
}

int ls_eval_file(ls_State *L, const char *path, const ls_Options *options, ls_Result *result) {
	ls_Options local_options;
	ls_Options effective_options;
	char *source;
	int ok;

	if (options == NULL) {
		ls_options_init(&local_options);
		options = &local_options;
	}

	effective_options = *options;
	if (effective_options.source_name == NULL) {
		effective_options.source_name = path;
	}

	source = read_file_text(path);
	if (source == NULL) {
		err_set("failed to read '%s'", path);
		return 0;
	}

	ok = ls_eval_string(L, source, &effective_options, result);
	free(source);
	return ok;
}

const char *ls_errmsg(const ls_State *L) {
	(void)L;
	return err_get();
}