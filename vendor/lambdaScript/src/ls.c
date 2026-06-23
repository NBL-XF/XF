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

struct ls_State {
	int reserved;
};

typedef struct {
	char *data;
	size_t length;
	size_t capacity;
} LsBuf;

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

	merged = merge_source_with_builtins(source, options, &injected_lines);
	if (merged == NULL) {
		err_set("out of memory");
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