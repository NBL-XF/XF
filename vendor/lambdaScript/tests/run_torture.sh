#!/bin/sh
set -eu

BIN="${1:-./bin/lambdaScript}"
FILE="${2:-tests/torture.lambda}"
EXPECT='cons True (cons True (cons True (cons ALPHA (cons BETA (cons (STEP (STEP (STEP START))) (cons (STEP (STEP (STEP (STEP (STEP ZERO_SYMBOL))))) (cons (STEP (STEP (STEP (STEP (STEP (STEP ZERO_SYMBOL)))))) (cons (STEP (STEP (STEP ZERO_SYMBOL))) (cons True (cons (STEP (STEP (STEP (STEP (STEP (STEP ZERO_SYMBOL)))))) (cons UNICODE_OK nil)))))))))))'

OUT="$("$BIN" -q -n 1000000 "$FILE")"

if [ "$OUT" = "$EXPECT" ]; then
    printf 'PASS torture\n'
else
    printf 'FAIL torture\n'
    printf 'expected: [%s]\n' "$EXPECT"
    printf 'actual:   [%s]\n' "$OUT"
    exit 1
fi
