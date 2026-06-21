#include "../include/ls.h"

#include "../include/err.h"
#include "../include/interp.h"
#include "../include/parser.h"
#include "../include/value.h"

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

static char *merge_source_with_prelude(const char *source, int use_prelude) {
	size_t prelude_len = use_prelude ? strlen(LS_PRELUDE) : 0;
	size_t source_len = strlen(source);
	char *merged = (char *)malloc(prelude_len + source_len + 2);

	if (merged == NULL) {
		return NULL;
	}

	merged[0] = '\0';
	if (use_prelude) {
		memcpy(merged, LS_PRELUDE, prelude_len);
		merged[prelude_len] = '\0';
	}
	memcpy(merged + prelude_len, source, source_len + 1);

	return merged;
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

	ls_result_free(result);

	merged = merge_source_with_prelude(source, options->use_prelude);
	if (merged == NULL) {
		err_set("out of memory");
		goto fail;
	}

	program = parser_parse_program(merged);
	if (program == NULL) {
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
	char *source;
	int ok;

	source = read_file_text(path);
	if (source == NULL) {
		err_set("failed to read '%s'", path);
		return 0;
	}

	ok = ls_eval_string(L, source, options, result);
	free(source);
	return ok;
}

const char *ls_errmsg(const ls_State *L) {
	(void)L;
	return err_get();
}