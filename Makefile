CC      = /opt/homebrew/opt/llvm/bin/clang
AR = /opt/homebrew/opt/llvm/bin/llvm-ar
CFLAGS  = -O0 -g -fsanitize=address,leak,undefined -fno-omit-frame-pointer  -Wall -Wextra -Wpedantic -std=c11
#CFLAGS  = -O0 -g -fsanitize=thread -fno-omit-frame-pointer  -Wall -Wextra -Wpedantic -std=c11
LDFLAGS = -fsanitize=address,leak,undefined
LDLIBS  ?= -lm -lpthread
PREFIX  ?= /usr/local
DESTDIR ?=
OBJDIR  = obj
BINDIR  = bin
LIBDIR  = lib

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
	src/core/process.c

RUNTIME_SRCS = \
	src/ast.c \
	src/interp.c \
 src/gc.c \
	src/lexer.c \
	src/parser.c \
	src/symTable.c \
	src/value.c \
	src/vm.c \
	lib/driver.c \
	lib/api.c \
	$(CORE_SRCS)

CLI_SRCS = \
	src/main.c \
	src/repl.c

RUNTIME_OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(RUNTIME_SRCS))
CLI_OBJS     = $(patsubst %.c,$(OBJDIR)/%.o,$(CLI_SRCS))

# All core objects depend on the shared header
$(patsubst %.c,$(OBJDIR)/%.o,$(CORE_SRCS)): src/core/internal.h

all: $(LIBXF) $(BIN)
run:
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
	LSAN_OPTIONS=verbosity=1:report_objects=1 \
UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
		./bin/xf -r tests/torture.xf
$(LIBXF): $(RUNTIME_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(BIN): $(CLI_OBJS) $(LIBXF)
	@mkdir -p $(BINDIR)
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
	install -m 644 include/core.h include/value.h include/symTable.h \
	               lib/driver.h \
	               $(DESTDIR)$(PREFIX)/include/xf/

uninstall:
	rm -f  $(DESTDIR)$(PREFIX)/bin/xf
	rm -f  $(DESTDIR)$(PREFIX)/lib/libxf.a
	rm -rf $(DESTDIR)$(PREFIX)/include/xf

clean:
	rm -rf $(OBJDIR) $(BIN) $(LIBXF)

.PHONY: all run install uninstall clean