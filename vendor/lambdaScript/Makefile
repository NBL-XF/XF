# Makefile

CC := cc
AR := ar
RANLIB := ranlib

CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS :=
LIB_TARGET := lib/libls.a
CLI_TARGET := bin/lambdaScript
LOCAL_LIB_SRC := Libraries

GLOBAL_LIB_DIR := $(shell \
	if [ -n "$$LAMBDASCRIPT_LIB_DIR" ]; then \
		printf '%s' "$$LAMBDASCRIPT_LIB_DIR"; \
	elif [ -n "$$XDG_DATA_HOME" ]; then \
		printf '%s/lambdascript/lib' "$$XDG_DATA_HOME"; \
	elif [ -n "$$HOME" ]; then \
		printf '%s/.lambdascript/lib' "$$HOME"; \
	else \
		printf '%s/.lambdascript/lib' "$$(pwd)"; \
	fi)

LIB_SRC := \
	src/ast.c \
	src/err.c \
	src/interp.c \
	src/lexer.c \
	src/ls.c \
	src/parser.c \
	src/symTable.c \
	src/value.c

CLI_SRC := src/main.c

LIB_OBJ := $(patsubst src/%.c,obj/%.o,$(LIB_SRC))
CLI_OBJ := $(patsubst src/%.c,obj/%.o,$(CLI_SRC))

.PHONY: all clean dirs smoke test install-libs

all: dirs $(LIB_TARGET) $(CLI_TARGET) install-libs

dirs:
	mkdir -p bin lib obj tests public

$(LIB_TARGET): $(LIB_OBJ)
	$(AR) rcs $@ $(LIB_OBJ)
	$(RANLIB) $@

$(CLI_TARGET): $(CLI_OBJ) $(LIB_TARGET)
	$(CC) $(CLI_OBJ) $(LIB_TARGET) $(LDFLAGS) -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

install-libs:
	@mkdir -p "$(GLOBAL_LIB_DIR)"
	@if [ -d "$(LOCAL_LIB_SRC)" ]; then \
		cp -R "$(LOCAL_LIB_SRC)"/. "$(GLOBAL_LIB_DIR)"/; \
		printf 'Synced %s -> %s\n' "$(LOCAL_LIB_SRC)" "$(GLOBAL_LIB_DIR)"; \
	else \
		printf 'Skipping lib sync: %s does not exist\n' "$(LOCAL_LIB_SRC)"; \
	fi

smoke: $(CLI_TARGET)
	./bin/lambdaScript -e '(\x.x) (\y.y)'
	./bin/lambdaScript -e 'I z'
	./bin/lambdaScript -e '\f x.f x'
	printf '%s\n' 'ID = \x.x' 'ID z' | ./bin/lambdaScript
	./bin/lambdaScript -q -n 1 -e '(\x.x) ((\y.y) z)'
	./bin/lambdaScript -t -e '(\x.x) ((\y.y) z)'

test: $(CLI_TARGET)
	chmod +x tests/test.sh
	./tests/test.sh

clean:
	rm -rf obj/*.o $(LIB_TARGET) $(CLI_TARGET)