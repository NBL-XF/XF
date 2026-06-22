.DEFAULT_GOAL := all

UNAME_S := $(shell uname -s)
LLVM_PREFIX ?= /opt/homebrew/opt/llvm/bin

ifeq ($(wildcard $(LLVM_PREFIX)/clang),)
CC ?= clang
else
CC := $(LLVM_PREFIX)/clang
endif

ifeq ($(UNAME_S),Darwin)
AR := /usr/bin/ar
RANLIB := /usr/bin/ranlib
else
AR ?= ar
RANLIB ?= ranlib
endif

WARNFLAGS := -Wall -Wextra -Wpedantic -std=c11
CFLAGS ?= -O2 $(WARNFLAGS)

BUILD_DIR := build
OBJDIR := $(BUILD_DIR)/obj
BINDIR := $(BUILD_DIR)/bin

LIB_SRCS := interpreter.c parser.c lexer.c vm.c value.c
LIB_OBJS := $(patsubst %.c,$(OBJDIR)/%.o,$(LIB_SRCS))
CLI_OBJS := $(OBJDIR)/main.o
LIB := $(BINDIR)/libbl.a
BIN := $(BINDIR)/bytelang
TEST_RUNNER := tests/run_tests.sh

.PHONY: all clean check

all: $(LIB) $(BIN)

$(OBJDIR) $(BINDIR):
	mkdir -p $@

$(OBJDIR)/%.o: %.c *.h | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(LIB_OBJS) | $(BINDIR)
	$(AR) rcs $@ $(LIB_OBJS)
	$(RANLIB) $@

$(BIN): $(CLI_OBJS) $(LIB) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(CLI_OBJS) $(LIB)

check: $(BIN)
	$(TEST_RUNNER) ./$(BIN)

clean:
	rm -rf $(BUILD_DIR)
