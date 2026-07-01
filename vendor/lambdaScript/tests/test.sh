#!/bin/sh

set -eu

BIN="./bin/lambdaScript"
PASS=0
FAIL=0

run_case() {
	NAME="$1"
	EXPECT_CODE="$2"
	EXPECT_OUT="$3"
	EXPECT_ERR="$4"
	shift 4

	OUT_FILE="$(mktemp)"
	ERR_FILE="$(mktemp)"

	if "$@" >"$OUT_FILE" 2>"$ERR_FILE"; then
		ACTUAL_CODE=0
	else
		ACTUAL_CODE=$?
	fi

	ACTUAL_OUT="$(cat "$OUT_FILE")"
	ACTUAL_ERR="$(cat "$ERR_FILE")"

	rm -f "$OUT_FILE" "$ERR_FILE"

	if [ "$ACTUAL_CODE" = "$EXPECT_CODE" ] &&
	   [ "$ACTUAL_OUT" = "$EXPECT_OUT" ] &&
	   [ "$ACTUAL_ERR" = "$EXPECT_ERR" ]; then
		printf 'PASS %s\n' "$NAME"
		PASS=$((PASS + 1))
	else
		printf 'FAIL %s\n' "$NAME"
		printf '  expected exit: %s\n' "$EXPECT_CODE"
		printf '  actual exit:   %s\n' "$ACTUAL_CODE"
		printf '  expected out: [%s]\n' "$EXPECT_OUT"
		printf '  actual out:   [%s]\n' "$ACTUAL_OUT"
		printf '  expected err: [%s]\n' "$EXPECT_ERR"
		printf '  actual err:   [%s]\n' "$ACTUAL_ERR"
		FAIL=$((FAIL + 1))
	fi
}

run_stdin_case() {
	NAME="$1"
	EXPECT_CODE="$2"
	EXPECT_OUT="$3"
	EXPECT_ERR="$4"
	INPUT="$5"
	shift 5

	OUT_FILE="$(mktemp)"
	ERR_FILE="$(mktemp)"
	IN_FILE="$(mktemp)"

	printf '%s' "$INPUT" >"$IN_FILE"

	if "$@" <"$IN_FILE" >"$OUT_FILE" 2>"$ERR_FILE"; then
		ACTUAL_CODE=0
	else
		ACTUAL_CODE=$?
	fi

	ACTUAL_OUT="$(cat "$OUT_FILE")"
	ACTUAL_ERR="$(cat "$ERR_FILE")"

	rm -f "$OUT_FILE" "$ERR_FILE" "$IN_FILE"

	if [ "$ACTUAL_CODE" = "$EXPECT_CODE" ] &&
	   [ "$ACTUAL_OUT" = "$EXPECT_OUT" ] &&
	   [ "$ACTUAL_ERR" = "$EXPECT_ERR" ]; then
		printf 'PASS %s\n' "$NAME"
		PASS=$((PASS + 1))
	else
		printf 'FAIL %s\n' "$NAME"
		printf '  expected exit: %s\n' "$EXPECT_CODE"
		printf '  actual exit:   %s\n' "$ACTUAL_CODE"
		printf '  expected out: [%s]\n' "$EXPECT_OUT"
		printf '  actual out:   [%s]\n' "$ACTUAL_OUT"
		printf '  expected err: [%s]\n' "$EXPECT_ERR"
		printf '  actual err:   [%s]\n' "$ACTUAL_ERR"
		FAIL=$((FAIL + 1))
	fi
}

run_smoke() {
	NAME="$1"
	shift 1

	OUT_FILE="$(mktemp)"
	ERR_FILE="$(mktemp)"

	if "$@" >"$OUT_FILE" 2>"$ERR_FILE"; then
		ACTUAL_CODE=0
	else
		ACTUAL_CODE=$?
	fi

	ACTUAL_OUT="$(cat "$OUT_FILE")"
	ACTUAL_ERR="$(cat "$ERR_FILE")"

	rm -f "$OUT_FILE" "$ERR_FILE"

	if [ "$ACTUAL_CODE" = "0" ]; then
		printf 'PASS %s\n' "$NAME"
		PASS=$((PASS + 1))
	else
		printf 'FAIL %s\n' "$NAME"
		printf '  expected exit: 0\n'
		printf '  actual exit:   %s\n' "$ACTUAL_CODE"
		printf '  actual out:   [%s]\n' "$ACTUAL_OUT"
		printf '  actual err:   [%s]\n' "$ACTUAL_ERR"
		FAIL=$((FAIL + 1))
	fi
}

run_file_case() {
	NAME="$1"
	EXPECT_CODE="$2"
	EXPECT_OUT="$3"
	EXPECT_ERR="$4"
	FILE_TEXT="$5"
	shift 5

	TMP_FILE="$(mktemp)"
	printf '%s' "$FILE_TEXT" >"$TMP_FILE"

	run_case "$NAME" "$EXPECT_CODE" "$EXPECT_OUT" "$EXPECT_ERR" "$@" "$TMP_FILE"

	rm -f "$TMP_FILE"
}

run_case \
	"identity" \
	0 \
	'\x0.x0' \
	'[steps=1]' \
	"$BIN" -e '(\x.x) (\y.y)'

run_case \
	"prelude-I" \
	0 \
	'z' \
	'[steps=1]' \
	"$BIN" -e 'I z'

run_case \
	"multi-arg-lambda" \
	0 \
	'\x0 x1.x0 x1' \
	'[steps=0]' \
	"$BIN" -e '\f x.f x'

run_case \
	"unicode-lambda" \
	0 \
	'\x0 x1.x0 x1' \
	'[steps=0]' \
	"$BIN" -e 'λf x.f x'

run_stdin_case \
	"top-level-definitions-stdin-2step" \
	0 \
	'z' \
	'[steps=3]' \
	'ID = \x.x
APPLY = \f.\x.f x
APPLY ID z
' \
	"$BIN" --no-prelude

run_file_case \
	"top-level-definitions-file" \
	0 \
	'z' \
	'[steps=3]' \
	'K = \x.\y.x
I = \x.x
I (K z w)
' \
	"$BIN" --no-prelude

run_stdin_case \
	"comments" \
	0 \
	'z' \
	'[steps=1]' \
	'; comment
I z -- inline
' \
	"$BIN"

run_case \
	"quiet-step-limit" \
	0 \
	'(\x0.x0) z' \
	'warning: reduction stopped after reaching step limit (1)' \
	"$BIN" -q -n 1 -e '(\x.x) ((\y.y) z)'

run_case \
	"trace" \
	0 \
	'z' \
	'[0] (\x0.x0) ((\x0.x0) z)
[1] (\x0.x0) z
[2] z
[steps=2]' \
	"$BIN" -t -e '(\x.x) ((\y.y) z)'

run_stdin_case \
	"definition-lhs-must-be-bare-identifier" \
	1 \
	'' \
	'error: line 1: invalid token at byte 5' \
	'ID z = w
' \
	"$BIN" --no-prelude

run_stdin_case \
	"forward-definition-reference-error" \
	1 \
	'' \
	"error: unresolved forward or mutual definition reference 'B'" \
	'A = B
B = \x.x
A
' \
	"$BIN" --no-prelude

run_case \
	"math-ascii-aliases" \
	0 \
	'cons 4 (cons 1 (cons ∞ (cons ∞ (cons 5 (cons 1 nil)))))' \
	'' \
	"$BIN" -q tests/math_ascii_aliases.lambda

run_case \
	"math-ops" \
	0 \
	'cons 7 (cons 512 (cons 4 (cons 1 (cons 2 (cons 42 nil)))))' \
	'' \
	"$BIN" -q tests/math_ops.lambda

run_case \
	"math-forms" \
	0 \
	'cons (sigma 0 3 (\x0.ADD x0 1)) (cons (integral 0 1 (\x0.POW x0 2)) (cons (limit 0 (\x0.DIV (sin x0) x0)) (cons (elem a SET) (cons (contains SET a) (cons (elem a SET) (cons (contains SET a) nil))))))' \
	'' \
	"$BIN" -q tests/math_forms.lambda

run_case \
	"math-manual" \
	0 \
	'cons (sigma 0 N (\x0.mul x0 x0)) (cons (integral 0 1 (\x0.mul x0 x0)) (cons (limit 0 (\x0.div (sin x0) x0)) nil))' \
	'' \
	"$BIN" -q tests/math.lambda

run_case \
	"subscripts" \
	0 \
	'cons (sub N e) (cons (sub N e) (cons (sub N i j) (cons (sub N i j) (cons 0.5 (cons (sigma 0 n (\x0.sub x x0)) nil)))))' \
	'' \
	"$BIN" -q tests/subscripts.lambda

run_case \
	"membership" \
	0 \
	"cons True (cons True (cons True (cons True (cons True (cons True (cons True (cons True (nil))))))))" \
	'' \
	"$BIN" -q tests/membership.lambda

run_case \
	"theorem-equivalence" \
	0 \
	'cons True (cons True nil)' \
	'' \
	"$BIN" -q tests/theorem_equiv.lambda

run_case \
	"import-main" \
	0 \
	'z' \
	'' \
	"$BIN" -q tests/import_main.lambda

run_case \
	"import-duplicate" \
	1 \
	'' \
	"error: duplicate definition 'A' at tests/import_duplicate.lambda:2 (already defined at tests/import_duplicate.lambda:1)" \
	"$BIN" -q tests/import_duplicate.lambda

run_case \
	"torture" \
	0 \
	'cons True (cons True (cons True (cons ALPHA (cons BETA (cons (STEP (STEP (STEP START))) (cons (STEP (STEP (STEP (STEP (STEP ZERO_SYMBOL))))) (cons (STEP (STEP (STEP (STEP (STEP (STEP ZERO_SYMBOL)))))) (cons (STEP (STEP (STEP ZERO_SYMBOL))) (cons True (cons (STEP (STEP (STEP (STEP (STEP (STEP ZERO_SYMBOL)))))) (cons UNICODE_OK nil)))))))))))' \
	'' \
	"$BIN" -q tests/torture.lambda

run_case \
	"torture-script" \
	0 \
	'PASS torture' \
	'' \
	tests/run_torture.sh "$BIN" tests/torture.lambda

run_case \
	"ops" \
	0 \
	'True' \
	'' \
	"$BIN" -q tests/ops.lambda

run_smoke "ops-check" "$BIN" -q tests/ops_check.lambda

run_case \
	"args-arg1" \
	0 \
	'alpha' \
	'' \
	"$BIN" -q tests/args_arg1.lambda -- alpha

run_case \
	"args-arg2" \
	0 \
	'beta' \
	'' \
	"$BIN" -q tests/args_arg2.lambda -- alpha beta

run_case \
	"args-argc" \
	0 \
	'step (step (step zero))' \
	'' \
	"$BIN" -q tests/args_argc.lambda -- alpha beta gamma

run_case \
	"args-head" \
	0 \
	'alpha' \
	'' \
	"$BIN" -q tests/args_head.lambda -- alpha beta gamma

run_case \
	"args-list-multi" \
	0 \
	'triple alpha beta gamma' \
	'' \
	"$BIN" -q tests/args_list_multi.lambda -- alpha beta gamma

run_case \
	"args-multi" \
	0 \
	'triple alpha beta gamma' \
	'' \
	"$BIN" -q tests/args_multi.lambda -- alpha beta gamma

run_case \
	"args-multi-count" \
	0 \
	'step (step (step zero))' \
	'' \
	"$BIN" -q tests/args_multi_count.lambda -- alpha beta gamma

run_smoke "validation-smoke" "$BIN" -q tests/validation.lambda

printf '\n'
printf 'passed: %d\n' "$PASS"
printf 'failed: %d\n' "$FAIL"

if [ "$FAIL" -ne 0 ]; then
	exit 1
fi
