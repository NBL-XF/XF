# Makefile

.DEFAULT_GOAL := all

UNAME_S := $(shell uname -s)

LLVM_PREFIX ?= /opt/homebrew/opt/llvm/bin
CC ?= $(LLVM_PREFIX)/clang

ifeq ($(UNAME_S),Darwin)
	AR := /usr/bin/ar
	RANLIB := /usr/bin/ranlib
else
	AR ?= $(LLVM_PREFIX)/llvm-ar
	RANLIB ?= ranlib
endif

WARNFLAGS    = -Wall -Wextra -Wpedantic -std=c11
DEBUGFLAGS   = -O0 -g -fsanitize=address,leak,undefined -fno-omit-frame-pointer
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

LDLIBS ?= -lm -lpthread -lreadline

PREFIX  ?= /usr/local
DESTDIR ?=

OBJDIR = obj/$(MODE)
BINDIR = bin/$(MODE)
LIBDIR = lib/$(MODE)

BIN   = $(BINDIR)/xf
LIBXF = $(LIBDIR)/static/libxf.a

VENDOR_DIR ?= vendor

LS_ROOT   ?= $(VENDOR_DIR)/lambdaScript
LS_INC    ?= -I$(LS_ROOT)/include
LS_OBJDIR ?= $(OBJDIR)/lambdaScript
LS_LIB    ?= $(LS_OBJDIR)/libls.a

BYTE_ROOT ?= $(VENDOR_DIR)/byteLang
BYTE_INC  ?= -I$(BYTE_ROOT)
BYTE_LIB  ?= $(BYTE_ROOT)/build/bin/libbl.a

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
	src/core/img.c \
	src/core/lambda.c \
	src/core/byte.c

RUNTIME_SRCS = \
	src/ast.c \
	src/interp.c \
	src/lexer.c \
	src/parser.c \
	src/symTable.c \
	src/value.c \
	src/vm.c \
	src/gc.c \
	src/mt.c \
	$(CORE_SRCS) \
	lib/driver.c \
	lib/api.c

CLI_SRCS = \
	src/main.c \
	src/repl.c \
	src/simd.c

LS_SRCS = \
	$(LS_ROOT)/src/ast.c \
	$(LS_ROOT)/src/err.c \
	$(LS_ROOT)/src/interp.c \
	$(LS_ROOT)/src/lexer.c \
	$(LS_ROOT)/src/ls.c \
	$(LS_ROOT)/src/parser.c \
	$(LS_ROOT)/src/symTable.c \
	$(LS_ROOT)/src/value.c

RUNTIME_OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(RUNTIME_SRCS))
CLI_OBJS     = $(patsubst %.c,$(OBJDIR)/%.o,$(CLI_SRCS))
LS_OBJS      = $(patsubst $(LS_ROOT)/src/%.c,$(LS_OBJDIR)/%.o,$(LS_SRCS))

all: $(LIBXF) $(BIN)

$(patsubst %.c,$(OBJDIR)/%.o,$(CORE_SRCS)): src/core/internal.h

run: $(BIN)
ifeq ($(MODE),debug)
	ASAN_SYMBOLIZER_PATH=$(LLVM_PREFIX)/llvm-symbolizer \
	ASAN_OPTIONS=symbolize=1:detect_leaks=1:halt_on_error=1:abort_on_error=1 \
	LSAN_OPTIONS=verbosity=1:report_objects=1 \
	UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
	$(BIN) -r tests/torture.xf
else
	$(BIN) -r tests/torture.xf
endif

$(LIBXF): $(RUNTIME_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(BIN): $(CLI_OBJS) $(LIBXF) $(LS_LIB) $(BYTE_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(CLI_OBJS) $(LIBXF) $(LS_LIB) $(BYTE_LIB) $(LDFLAGS) $(LDLIBS)

$(BYTE_LIB):
	$(MAKE) -C $(BYTE_ROOT)

$(LS_LIB): $(LS_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(LS_OBJDIR)/%.o: $(LS_ROOT)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LS_INC) -c $< -o $@

$(OBJDIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LS_INC) $(BYTE_INC) -c $< -o $@

$(OBJDIR)/src/core/%.o: src/core/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LS_INC) $(BYTE_INC) -c $< -o $@

$(OBJDIR)/lib/%.o: lib/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LS_INC) $(BYTE_INC) -c $< -o $@

install: $(LIBXF) $(BIN)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/lib
	install -d $(DESTDIR)$(PREFIX)/include/xf
	install -m 755 $(BIN)   $(DESTDIR)$(PREFIX)/bin/xf
	install -m 644 $(LIBXF) $(DESTDIR)$(PREFIX)/lib/libxf.a
	install -m 644 include/core.h include/value.h include/symTable.h \
	               lib/driver.h \
	               $(DESTDIR)$(PREFIX)/include/xf/

uninstall:
	rm -f  $(DESTDIR)$(PREFIX)/bin/xf
	rm -f  $(DESTDIR)$(PREFIX)/lib/libxf.a
	rm -rf $(DESTDIR)$(PREFIX)/include/xf

clean:
	rm -rf obj/* bin/*
	rm -rf lib/debug lib/release lib/thread
	-$(MAKE) -C $(BYTE_ROOT) clean

export ASAN_SYMBOLIZER_PATH=$(LLVM_PREFIX)/llvm-symbolizer

.PHONY: all run install uninstall clean