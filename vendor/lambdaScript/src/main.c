#include "../include/ls.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *ls_read_all_stream(FILE *fp);

static int parse_step_limit(const char *text, size_t *out) {
	char *end = NULL;
	unsigned long long value;

	errno = 0;
	value = strtoull(text, &end, 10);
	if (errno != 0 || end == text || *end != '\0') {
		return 0;
	}

	*out = (size_t)value;
	return 1;
}

static void usage(const char *argv0) {
	fprintf(stderr, "usage: %s [-q] [-t] [-n STEPS] [-e EXPR] [FILE] [-- ARGS...]\n", argv0);
	fprintf(stderr, "\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "  -e EXPR        evaluate an expression/program string\n");
	fprintf(stderr, "  -n STEPS       max reduction steps\n");
	fprintf(stderr, "  -q             quiet; suppress [steps=...]\n");
	fprintf(stderr, "  -t             trace reductions to stderr\n");
	fprintf(stderr, "  --no-prelude   disable built-in I/K/S/TRUE/FALSE\n");
	fprintf(stderr, "  --             stop option parsing; remaining values become ARGS\n");
	fprintf(stderr, "  -h, --help     show help\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "program format:\n");
	fprintf(stderr, "  one definition per line: NAME = EXPR\n");
	fprintf(stderr, "  final non-definition line is the program expression\n");
	fprintf(stderr, "  script args are exposed as ARG1, ARG2, ..., ARGC, and ARGS\n");
	fprintf(stderr, "  logic aliases: ~ or ⌐ or ¬, -> or →, <-> or <=> or ↔ or ≣\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "examples:\n");
	fprintf(stderr, "  %s -e 'I z'\n", argv0);
	fprintf(stderr, "  %s -e 'ID = \\\\x.x\nID z'\n", argv0);
	fprintf(stderr, "  %s file.ls alpha beta\n", argv0);
	fprintf(stderr, "  %s -e 'ARG1' -- alpha\n", argv0);
}


int main(int argc, char **argv) {
	const char *expr = NULL;
	const char *file_path = NULL;
	const char *const *script_argv = NULL;
	size_t script_argc = 0;
	char *stdin_source = NULL;
	ls_State *L = NULL;
	ls_Options options;
	ls_Result result;
	int exit_code = 1;
	int i;

	ls_options_init(&options);
	ls_result_init(&result);

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--") == 0) {
			script_argv = (const char *const *)&argv[i + 1];
			script_argc = (size_t)(argc - i - 1);
			break;
		}

		if (strcmp(argv[i], "-e") == 0) {
			if (i + 1 >= argc) {
				usage(argv[0]);
				return 1;
			}
			expr = argv[++i];
			continue;
		}

		if (strcmp(argv[i], "-n") == 0) {
			if (i + 1 >= argc || !parse_step_limit(argv[i + 1], &options.max_steps)) {
				fprintf(stderr, "error: invalid step limit\n");
				return 1;
			}
			i++;
			continue;
		}

		if (strcmp(argv[i], "-q") == 0) {
			options.quiet = 1;
			continue;
		}

		if (strcmp(argv[i], "-t") == 0) {
			options.trace = 1;
			continue;
		}

		if (strcmp(argv[i], "--no-prelude") == 0) {
			options.use_prelude = 0;
			continue;
		}

		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			usage(argv[0]);
			return 0;
		}

		if (argv[i][0] == '-') {
			fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
			usage(argv[0]);
			return 1;
		}

		if (expr != NULL) {
			script_argv = (const char *const *)&argv[i];
			script_argc = (size_t)(argc - i);
			break;
		}

		if (file_path == NULL) {
			file_path = argv[i];
			continue;
		}

		script_argv = (const char *const *)&argv[i];
		script_argc = (size_t)(argc - i);
		break;
	}

	if (expr != NULL && file_path != NULL) {
		fprintf(stderr, "error: use either -e EXPR or FILE, not both\n");
		return 1;
	}

	options.argc = script_argc;
	options.argv = script_argv;
	if (expr != NULL) {
		options.source_name = "<eval>";
	} else if (file_path != NULL) {
		options.source_name = file_path;
	} else {
		options.source_name = "<stdin>";
	}

	L = ls_newstate();
	if (L == NULL) {
		fprintf(stderr, "fatal: out of memory\n");
		return 1;
	}

	if (expr != NULL) {
		if (!ls_eval_string(L, expr, &options, &result)) {
			fprintf(stderr, "error: %s\n", ls_errmsg(L));
			goto done;
		}
	} else if (file_path != NULL) {
		if (!ls_eval_file(L, file_path, &options, &result)) {
			fprintf(stderr, "error: %s\n", ls_errmsg(L));
			goto done;
		}
	} else {
		stdin_source = ls_read_all_stream(stdin);
		if (stdin_source == NULL) {
			fprintf(stderr, "fatal: out of memory\n");
			goto done;
		}
		if (!ls_eval_string(L, stdin_source, &options, &result)) {
			fprintf(stderr, "error: %s\n", ls_errmsg(L));
			goto done;
		}
	}

	if (result.trace != NULL) {
		fputs(result.trace, stderr);
	}
	fputs(result.output, stdout);
	fputc('\n', stdout);

	if (!options.quiet) {
		fprintf(stderr, "[steps=%zu]\n", result.steps);
	}
	if (result.reached_step_limit) {
		fprintf(stderr, "warning: reduction stopped after reaching step limit (%zu)\n", options.max_steps);
	}

	exit_code = 0;

done:
	free(stdin_source);
	ls_result_free(&result);
	ls_close(L);
	return exit_code;
}