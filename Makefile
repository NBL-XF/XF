.DEFAULT_GOAL := all

CC      = /opt/homebrew/opt/llvm/bin/clang
AR      = /opt/homebrew/opt/llvm/bin/llvm-ar

WARNFLAGS    = -Wall -Wextra -Wpedantic -std=c11
DEBUGFLAGS   = -O1 -g -fsanitize=address,leak,undefined -fno-omit-frame-pointer
THREADFLAGS  = -O0 -g -fsanitize=thread -fno-omit-frame-pointer
RELEASEFLAGS = -O3

MODE ?= release

ifeq ($(MODE),debug)
	CFLAGS  = $(DEBUGFLAGS) $(WARNFLAGS)
	LDFLAGS = -fsanitize=address,leak,undefined
else ifeq ($(MODE),thread)
	CFLAGS  = $(THREADFLAGS) $(WARNFLAGS)
	LDFLAGS = -fsanitize=thread
else
	CFLAGS  = $(RELEASEFLAGS) $(WARNFLAGS)
	LDFLAGS =
endif

LDLIBS  ?= -lm -lpthread -lreadline
PREFIX  ?= /usr/local
DESTDIR ?=

OBJDIR  = obj/$(MODE)
BINDIR  = bin/$(MODE)
LIBDIR  = lib/$(MODE)

BIN     = $(BINDIR)/xf
LIBXF   = $(LIBDIR)/static/libxf.a

CORE_SRCS = \
	src/core.c \
	src/core/helpers.c \
	src/core/math.c \
	src/core/str.c \
	src/core/os.c \
	src/core/generics.c \
	src/core/regex.c \
	src/core/format.c \
	src/core/ds.c \
	src/core/edit.c \
	src/core/process.c \
	src/core/img.c

LIB_API_SRCS = \
	lib/driver.c \
	lib/api.c

RUNTIME_SRCS = \
	src/ast.c \
	src/interp.c \
	src/lexer.c \
	src/parser.c \
	src/symTable.c \
	src/value.c \
	src/vm.c \
	$(CORE_SRCS) \
	$(LIB_API_SRCS)

CLI_SRCS = \
	src/main.c \
	src/repl.c

RUNTIME_OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(RUNTIME_SRCS))
CLI_OBJS     = $(patsubst %.c,$(OBJDIR)/%.o,$(CLI_SRCS))

all: $(LIBXF) $(BIN)

# Rebuild core objs when shared internal header changes
$(patsubst %.c,$(OBJDIR)/%.o,$(CORE_SRCS)): src/core/internal.h

run: $(BIN)
ifeq ($(MODE),debug)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
	LSAN_OPTIONS=verbosity=1:report_objects=1 \
	UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
	$(BIN) -r tests/torture.xf
else
	$(BIN) -r tests/torture.xf
endif

$(LIBXF): $(RUNTIME_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(BIN): $(CLI_OBJS) $(LIBXF)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(CLI_OBJS) $(LIBXF) $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

install: $(LIBXF) $(BIN)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/lib
	install -d $(DESTDIR)$(PREFIX)/include/xf

	install -m 755 $(BIN)   $(DESTDIR)$(PREFIX)/bin/xf
	install -m 644 $(LIBXF) $(DESTDIR)$(PREFIX)/lib/libxf.a

	install -m 644 \
		include/core.h \
		include/value.h \
		include/symTable.h \
		lib/api.h \
		lib/driver.h \
		$(DESTDIR)$(PREFIX)/include/xf/

uninstall:
	rm -f  $(DESTDIR)$(PREFIX)/bin/xf
	rm -f  $(DESTDIR)$(PREFIX)/lib/libxf.a
	rm -rf $(DESTDIR)$(PREFIX)/include/xf

clean:
	rm -rf obj/* bin/* lib/release/static lib/debug/static lib/thread/static

.PHONY: all run install uninstall clean