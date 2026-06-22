#include "../include/err.h"

#include <stdarg.h>
#include <stdio.h>

static char g_errbuf[512];
static int g_has_error = 0;

void err_clear(void) {
	g_errbuf[0] = '\0';
	g_has_error = 0;
}

void err_set(const char *fmt, ...) {
	va_list args;

	g_has_error = 1;
	va_start(args, fmt);
	vsnprintf(g_errbuf, sizeof(g_errbuf), fmt, args);
	va_end(args);
}

int err_has(void) {
	return g_has_error;
}

const char *err_get(void) {
	return g_has_error ? g_errbuf : "";
}