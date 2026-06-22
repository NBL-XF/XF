# LambdaScript

LambdaScript is a tiny lambda-calculus language for symbolic reduction, logic, transformations, and model expressions.

It keeps the core deliberately small:

* variables
* lambda abstraction
* function application
* top-level definitions
* normal-order beta reduction

The practical idea is simple: keep the evaluator monk-clean, then grow power through syntax sugar, prelude definitions, source libraries, and optional native/runtime bindings.

---

## Table of Contents

1. [Why LambdaScript](#why-lambdascript)
2. [Current Architecture](#current-architecture)
3. [Getting Started](#getting-started)
4. [CLI Usage](#cli-usage)
5. [A First LambdaScript Program](#a-first-lambdascript-program)
6. [Language Basics](#language-basics)
7. [Definitions](#definitions)
8. [Booleans and Logic Operators](#booleans-and-logic-operators)
9. [Branching](#branching)
10. [Pairs](#pairs)
11. [Church Numerals](#church-numerals)
12. [Loops and Iteration](#loops-and-iteration)
13. [Common Algorithms](#common-algorithms)
14. [Worked Examples](#worked-examples)
15. [Testing](#testing)
16. [XF Integration](#xf-integration)
17. [Symbolic Models](#symbolic-models)
18. [Tips for Writing Good LambdaScript](#tips-for-writing-good-lambdascript)
19. [Troubleshooting](#troubleshooting)
20. [Current Limitations](#current-limitations)
21. [Roadmap](#roadmap)

---

## Why LambdaScript

LambdaScript is useful when you want a small symbolic core that can express computation through function application.

It works especially well for:

* reduction experiments
* Church booleans and symbolic logic
* functional transformations
* symbolic model expressions
* small proof-calculus experiments
* testing language/runtime bridges
* embedding into a larger C/XF/Lua stack

It is not currently trying to be a general scripting language like XF or Lua. It is the small symbolic blade in the toolkit. 🗡️

---

## Current Architecture

The current stack shape is:

```text
C
├── Lua
├── XF
│   └── LambdaScript
├── LambdaScript
└── Scripts
    ├── Lua
    ├── XF
    │   └── LambdaScript
    └── LambdaScript
```

Meaning:

* C hosts the native runtimes.
* Lua remains useful for configuration, scripting, and glue.
* XF handles structured data, orchestration, OS calls, and pipelines.
* LambdaScript handles symbolic computation and lambda-calculus reduction.
* XF can invoke LambdaScript through `core.os.run(...)`.
* LambdaScript also exists as its own standalone binary.

The current XF bridge shells out to the LambdaScript binary. A later bridge can expose a direct native module such as:

```xf
core.lambda.eval("TRUE ^ FALSE")
core.lambda.file("tests/ops.ls")
core.lambda.reduce("(\\x.x) z")
```

---

## Getting Started

Build with your project Makefile, then run the binary.

Typical local run:

```sh
./lambdascript -q -e 'I z'
```

Expected:

```text
z
```

Run a file:

```sh
./lambdascript -q tests/ops.ls
```

Expected:

```text
True
```

---

## CLI Usage

```text
./lambdascript [-q] [-t] [-n STEPS] [-e EXPR] [FILE]
```

Options:

```text
-e EXPR        evaluate an expression/program string
-n STEPS       maximum reduction steps
-q             quiet mode; suppress [steps=...] on stderr
-t             trace reductions to stderr
--no-prelude   disable built-in prelude
-h, --help     show help
```

Examples:

```sh
./lambdascript -q -e 'I z'
./lambdascript -t -e '(\x.x) z'
./lambdascript -n 1000000 -q examples/hard/02_factorial_y.ls
cat tests/ops.ls | ./lambdascript -q
```

---

## A First LambdaScript Program

```ls
ID = \x.x
ID hello
```

Run:

```sh
./lambdascript -q first.ls
```

Expected:

```text
hello
```

A slightly larger one:

```ls
CONST = \x y.x
CONST left right
```

Expected:

```text
left
```

---

## Language Basics

### Lambda abstraction

ASCII lambda:

```ls
\x.x
```

Unicode lambda:

```ls
λx.x
```

Multiple parameters desugar into nested lambdas:

```ls
\x y.x
```

means:

```ls
\x.\y.x
```

### Function application

Application uses adjacency:

```ls
f x
```

Application associates left:

```ls
f x y
```

means:

```ls
(f x) y
```

Use parentheses to group:

```ls
f (g x)
```

### Comments

```ls
; semicolon comment
-- double-dash comment
```

### Program shape

A program is a list of single-line definitions followed by one final expression.

```ls
A = \x.x
B = \y.y

A B
```

The final expression is required.

---

## Definitions

A definition binds a name to an expression:

```ls
ID = \x.x
```

Definitions are expanded while lowering the program.

Current rule of thumb:

* define helpers before using them
* avoid forward references
* avoid direct recursive named definitions for now
* use combinators such as `Y` for recursion experiments

---

## Booleans and Logic Operators

The default prelude includes Church booleans:

```ls
TRUE = \t f.t
FALSE = \t f.f
```

Church booleans choose between two branches:

```ls
TRUE left right   ; reduces to left
FALSE left right  ; reduces to right
```

A useful display helper:

```ls
SHOW = \b.b True False
```

Then:

```ls
SHOW TRUE
```

prints:

```text
True
```

and:

```ls
SHOW FALSE
```

prints:

```text
False
```

### Logic functions

```ls
NOT = \p.p FALSE TRUE
AND = \p q.p q FALSE
OR = \p q.p TRUE q
IMP = \p q.OR (NOT p) q
IFF = \p q.AND (IMP p q) (IMP q p)
```

### Logic operators

ASCII:

```ls
~p
p ^ q
p v q
p -> q
p <-> q
```

Unicode:

```ls
¬p
p ∧ q
p ∨ q
p → q
p ↔ q
```

Precedence, highest to lowest:

```text
~
^
v
->
<->
```

Example:

```ls
SHOW (TRUE ^ ~ FALSE)
```

Expected:

```text
True
```

Note: if ASCII `v` is enabled as OR, do not use a bare variable named `v`.

---

## Branching

There is no native `if` yet. Branching is a function.

```ls
IF = \b then else.b then else
```

Example:

```ls
IF TRUE LEFT RIGHT
```

reduces to:

```text
LEFT
```

Example with logic:

```ls
IF (TRUE ^ FALSE) LEFT RIGHT
```

reduces to:

```text
RIGHT
```

---

## Pairs

Church pairs are enough to build small structured values.

```ls
PAIR = \a b f.f a b
FIRST = \p.p (\a b.a)
SECOND = \p.p (\a b.b)
```

Example:

```ls
SECOND (PAIR ALPHA BETA)
```

Expected:

```text
BETA
```

Pairs are the seed of records, tuples, state machines, and loop state.

---

## Church Numerals

Church numerals encode natural numbers as repeated function application.

```ls
ZERO = \f x.x
ONE = \f x.f x
TWO = \f x.f (f x)
THREE = \f x.f (f (f x))
```

To see a numeral, apply it to symbolic names:

```ls
THREE STEP START
```

Expected:

```text
STEP (STEP (STEP START))
```

Common arithmetic:

```ls
SUCC = \n f x.f (n f x)
PLUS = \m n f x.m f (n f x)
MULT = \m n f.m (n f)
POW = \base exponent.exponent base
```

Example:

```ls
PLUS TWO THREE STEP ZERO_SYMBOL
```

Expected:

```text
STEP (STEP (STEP (STEP (STEP ZERO_SYMBOL))))
```

Predicates:

```ls
ISZERO = \n.n (\x.FALSE) TRUE
```

Example:

```ls
SHOW (ISZERO ZERO)
```

Expected:

```text
True
```

---

## Loops and Iteration

LambdaScript does not currently have native `while` or `for`.

Loops are represented in two ways.

### 1. Bounded loops with Church numerals

A Church numeral is a loop count.

```ls
THREE STEP START
```

means:

```text
apply STEP three times to START
```

Result:

```text
STEP (STEP (STEP START))
```

This is the LambdaScript equivalent of:

```text
state = START
state = STEP(state)
state = STEP(state)
state = STEP(state)
```

### 2. Recursive loops with fixed-point combinators

The untyped lambda calculus can express recursion with a fixed-point combinator:

```ls
Y = \fnx.(\self.fnx (self self)) (\self.fnx (self self))
```

This enables experiments such as factorial, countdown, and recursive tree walks.

Use recursion carefully. It can explode reduction steps fast. For hard examples, run with a larger step limit:

```sh
./lambdascript -n 1000000 -q examples/hard/02_factorial_y.ls
```

---

## Common Algorithms

These are small patterns you can build today.

### Identity

```ls
I Z
```

Expected:

```text
Z
```

### Constant selection

```ls
K LEFT RIGHT
```

Expected:

```text
LEFT
```

### Composition

```ls
COMP = \f g x.f (g x)
COMP F G X
```

Expected:

```text
F (G X)
```

### Boolean negation

```ls
SHOW (~ FALSE)
```

Expected:

```text
True
```

### Conditional select

```ls
IF = \b then else.b then else
IF FALSE LEFT RIGHT
```

Expected:

```text
RIGHT
```

### Pair projection

```ls
PAIR = \a b f.f a b
FIRST = \p.p (\a b.a)
FIRST (PAIR APPLE ORANGE)
```

Expected:

```text
APPLE
```

### Bounded iteration

```ls
THREE STEP START
```

Expected:

```text
STEP (STEP (STEP START))
```

### Numeric equality

```ls
PRED = \n f x.n (\g h.h (g f)) (\u.x) (\u.u)
SUB = \m n.n PRED m
LEQ = \m n.ISZERO (SUB m n)
NUM_EQ = \m n.AND (LEQ m n) (LEQ n m)

SHOW (NUM_EQ TWO (PRED THREE))
```

Expected:

```text
True
```

---

## Worked Examples

This section keeps the examples directly in the README, not hidden behind file names. The examples move from easy symbolic programs to harder lambda-calculus algorithms.

LambdaScript is not XF, so these are not CSV/dataframe examples. The equivalent practical patterns are symbolic reduction, logic selection, Church booleans, Church numerals, pair-encoded state, bounded loops, and recursion through a fixed-point combinator.

### Example 1: Identity smoke test

This is the smallest useful program. It checks that application and reduction work.

```ls
ID = \x.x
ID hello
```

Expected output:

```text
hello
```

What happens:

```text
ID hello
(\x.x) hello
hello
```

Run:

```sh
./lambdascript -q examples/easy/01_identity.ls
```

---

### Example 2: Constant selection

The constant function chooses the first argument and ignores the second.

```ls
CONST = \x y.x
CONST left right
```

Expected output:

```text
left
```

This is the same basic mechanism Church booleans use for branching.

---

### Example 3: Boolean display with `SHOW`

Raw Church booleans print as lambda functions. `SHOW` turns them into readable symbolic names.

```ls
SHOW = \b.b True False
SHOW TRUE
```

Expected output:

```text
True
```

False works the same way:

```ls
SHOW = \b.b True False
SHOW FALSE
```

Expected output:

```text
False
```

---

### Example 4: Logic operator smoke test

This checks parser-level logic operators.

```ls
SHOW = \b.b True False
NOT = \p.p FALSE TRUE
AND = \p q.p q FALSE
OR = \p q.p TRUE q
IMP = \p q.OR (NOT p) q
IFF = \p q.AND (IMP p q) (IMP q p)

SHOW (TRUE ^ ~ FALSE)
```

Expected output:

```text
True
```

Equivalent expanded form:

```ls
SHOW (AND TRUE (NOT FALSE))
```

---

### Example 5: Branching with Church booleans

LambdaScript does not currently need native `if` to express branching. A boolean is already a branch selector.

```ls
IF = \b then else.b then else

IF TRUE left_branch right_branch
```

Expected output:

```text
left_branch
```

A more useful branch:

```ls
SHOW = \b.b True False
NOT = \p.p FALSE TRUE
AND = \p q.p q FALSE
OR = \p q.p TRUE q
IF = \b then else.b then else

IF (TRUE ^ ~ FALSE) accepted rejected
```

Expected output:

```text
accepted
```

---

### Example 6: Logic gate bundle

This bundles multiple boolean checks into one final result. It is a good regression test for logic behavior.

```ls
SHOW = \b.b True False
NOT = \p.p FALSE TRUE
AND = \p q.p q FALSE
OR = \p q.p TRUE q
IMP = \p q.OR (NOT p) q
IFF = \p q.AND (IMP p q) (IMP q p)

TEST_AND = TRUE ^ TRUE
TEST_OR = FALSE v TRUE
TEST_NOT = ~ FALSE
TEST_IMP = FALSE -> TRUE
TEST_IFF = TRUE <-> TRUE

ALL = TEST_AND ^ TEST_OR ^ TEST_NOT ^ TEST_IMP ^ TEST_IFF

SHOW ALL
```

Expected output:

```text
True
```

This is the LambdaScript version of a small unit-test pipeline.

---

### Example 7: Pairs as tiny records

Church pairs let LambdaScript carry two pieces of state.

```ls
PAIR = \a b f.f a b
FIRST = \p.p (\a b.a)
SECOND = \p.p (\a b.b)

person = PAIR Jay LambdaScript

FIRST person
```

Expected output:

```text
Jay
```

Second field:

```ls
PAIR = \a b f.f a b
FIRST = \p.p (\a b.a)
SECOND = \p.p (\a b.b)

person = PAIR Jay LambdaScript

SECOND person
```

Expected output:

```text
LambdaScript
```

This pattern becomes important for state machines and loop state.

---

### Example 8: Symbolic function composition

Composition lets you build pipelines.

```ls
COMP = \f g x.f (g x)

COMP clean parse input
```

Expected output:

```text
clean (parse input)
```

Another pipeline:

```ls
COMP = \f g x.f (g x)
PIPE = COMP validate transform

PIPE row
```

Expected output:

```text
validate (transform row)
```

This is not executing `validate` or `transform` as native functions. It is building and reducing symbolic application structure.

---

### Example 9: Church numerals as bounded loops

A Church numeral repeats a function a fixed number of times.

```ls
THREE = \f x.f (f (f x))

THREE step start
```

Expected output:

```text
step (step (step start))
```

This is equivalent to:

```text
state = start
state = step(state)
state = step(state)
state = step(state)
```

So Church numerals are the current LambdaScript way to write bounded loops.

---

### Example 10: Increment-like symbolic loop

This example names the loop operation `inc`.

```ls
ZERO = \f x.x
ONE = \f x.f x
TWO = \f x.f (f x)
THREE = \f x.f (f (f x))
FOUR = \f x.f (f (f (f x)))

FOUR inc zero
```

Expected output:

```text
inc (inc (inc (inc zero)))
```

This is a symbolic counter. Once native numeric primitives exist, this can connect to actual arithmetic.

---

### Example 11: Church addition

Addition combines two loop counts.

```ls
TWO = \f x.f (f x)
THREE = \f x.f (f (f x))
PLUS = \m n f x.m f (n f x)

PLUS TWO THREE step zero
```

Expected output:

```text
step (step (step (step (step zero))))
```

This represents `2 + 3 = 5`.

---

### Example 12: Church multiplication

Multiplication composes repeated application.

```ls
TWO = \f x.f (f x)
THREE = \f x.f (f (f x))
MULT = \m n f.m (n f)

MULT TWO THREE step zero
```

Expected output:

```text
step (step (step (step (step (step zero)))))
```

This represents `2 * 3 = 6`.

---

### Example 13: Zero check

`ISZERO` tests whether a Church numeral applies its function zero times.

```ls
SHOW = \b.b True False

ZERO = \f x.x
ONE = \f x.f x
ISZERO = \n.n (\x.FALSE) TRUE

SHOW (ISZERO ZERO)
```

Expected output:

```text
True
```

And:

```ls
SHOW = \b.b True False

ZERO = \f x.x
ONE = \f x.f x
ISZERO = \n.n (\x.FALSE) TRUE

SHOW (ISZERO ONE)
```

Expected output:

```text
False
```

---

### Example 14: Predecessor

Predecessor is the classic “one less” operation for Church numerals.

```ls
ZERO = \f x.x
ONE = \f x.f x
TWO = \f x.f (f x)
THREE = \f x.f (f (f x))

PRED = \n f x.n (\g h.h (g f)) (\u.x) (\u.u)

PRED THREE step zero
```

Expected output:

```text
step (step zero)
```

This represents `3 - 1 = 2`.

---

### Example 15: Numeric equality

This builds equality out of predecessor, subtraction, and less-than-or-equal.

```ls
SHOW = \b.b True False
NOT = \p.p FALSE TRUE
AND = \p q.p q FALSE
OR = \p q.p TRUE q

ZERO = \f x.x
ONE = \f x.f x
TWO = \f x.f (f x)
THREE = \f x.f (f (f x))
ISZERO = \n.n (\x.FALSE) TRUE

PRED = \n f x.n (\g h.h (g f)) (\u.x) (\u.u)
SUB = \m n.n PRED m
LEQ = \m n.ISZERO (SUB m n)
NUM_EQ = \m n.AND (LEQ m n) (LEQ n m)

SHOW (NUM_EQ TWO (PRED THREE))
```

Expected output:

```text
True
```

This checks:

```text
2 == pred(3)
```

---

### Example 16: Pair-encoded loop state

Pairs can hold loop state. This example repeatedly updates a pair.

```ls
PAIR = \a b f.f a b
FIRST = \p.p (\a b.a)
SECOND = \p.p (\a b.b)

THREE = \f x.f (f (f x))

NEXT = \state.PAIR (SECOND state) (step (SECOND state))
START = PAIR zero one

SECOND (THREE NEXT START)
```

Expected output:

```text
step (step (step one))
```

What happens conceptually:

```text
START = (zero, one)
NEXT  = (one, step one)
NEXT  = (step one, step (step one))
NEXT  = (step (step one), step (step (step one)))
```

Then `SECOND` extracts the second field.

---

### Example 17: Symbolic transformation pipeline

This is useful for modeling computation without native data structures yet.

```ls
COMP = \f g x.f (g x)

NORMALIZE = \x.normalize x
TOKENIZE = \x.tokenize x
ENCODE = \x.encode x

PIPE = COMP ENCODE (COMP TOKENIZE NORMALIZE)

PIPE raw_input
```

Expected output:

```text
encode (tokenize (normalize raw_input))
```

This is the symbolic LambdaScript cousin of an XF processing pipeline.

---

### Example 18: Symbolic model expression

LambdaScript can express model formulas as reducible symbolic structure.

```ls
ADD = \x y.add x y
MUL = \x y.mul x y
DECAY = \rate time.exp (neg (mul rate time))

PROPAGATE = \signal gate decay.mul (mul signal gate) decay

PROPAGATE input_signal context_gate decay_factor
```

Expected output:

```text
mul (mul input_signal context_gate) decay_factor
```

The symbolic names `mul`, `exp`, and `neg` remain free variables. Later, native primitive bindings can attach meaning to them.

---

### Example 19: Factorial with a fixed-point combinator

This is a hard stress test. It uses recursion through `Y`.

```ls
SHOW = \b.b True False
NOT = \p.p FALSE TRUE
AND = \p q.p q FALSE
OR = \p q.p TRUE q

ZERO = \f x.x
ONE = \f x.f x
TWO = \f x.f (f x)
THREE = \f x.f (f (f x))
MULT = \m n f.m (n f)
ISZERO = \n.n (\x.FALSE) TRUE

PRED = \n f x.n (\g h.h (g f)) (\u.x) (\u.u)

Y = \fnx.(\self.fnx (self self)) (\self.fnx (self self))
FACTF = \fact n.(ISZERO n) ONE (MULT n (fact (PRED n)))
FACT = Y FACTF

FACT THREE step zero
```

Expected output:

```text
step (step (step (step (step (step zero)))))
```

That represents `3! = 6`.

Run with a high step limit:

```sh
./lambdascript -n 1000000 -q examples/hard/02_factorial_y.ls
```

---

### Example 20: Full operator and algorithm smoke program

This combines logic, numerals, predecessor, equality, and readable boolean output.

```ls
SHOW = \b.b True False
NOT = \p.p FALSE TRUE
AND = \p q.p q FALSE
OR = \p q.p TRUE q
IMP = \p q.OR (NOT p) q
IFF = \p q.AND (IMP p q) (IMP q p)

ZERO = \f x.x
ONE = \f x.f x
TWO = \f x.f (f x)
THREE = \f x.f (f (f x))
ISZERO = \n.n (\x.FALSE) TRUE

PRED = \n f x.n (\g h.h (g f)) (\u.x) (\u.u)
SUB = \m n.n PRED m
LEQ = \m n.ISZERO (SUB m n)
NUM_EQ = \m n.AND (LEQ m n) (LEQ n m)

BOOL_TEST = (TRUE ^ ~ FALSE)
NUM_TEST = NUM_EQ TWO (PRED THREE)
ZERO_TEST = ISZERO ZERO
ALL = BOOL_TEST ^ NUM_TEST ^ ZERO_TEST

SHOW ALL
```

Expected output:

```text
True
```

This is a good “is the little machine still alive?” program. 🧪


## Testing

The tests are small LambdaScript programs, and the README includes their full contents here so the docs are useful even before you open the `tests/` directory.

Run one test file:

```sh
./lambdascript -q tests/ops.ls
```

Expected:

```text
True
```

Run the full smoke suite:

```sh
sh scripts/run-tests.sh ./lambdascript
```

Or, if the binary is installed:

```sh
sh scripts/run-tests.sh lambdascript
```

### Test 1: `tests/identity.ls`

Checks that the identity combinator returns its argument.

```ls
; Expected: Z
I Z
```

Expected output:

```text
Z
```

### Test 2: `tests/branch.ls`

Checks Church boolean branching.

```ls
; Expected: LEFT
SHOW = \b.b True False
NOT = \p.p FALSE TRUE
AND = \p q.p q FALSE
OR = \p q.p TRUE q
IMP = \p q.OR (NOT p) q
IFF = \p q.AND (IMP p q) (IMP q p)
IF = \b then else.b then else
IF (TRUE v FALSE) LEFT RIGHT
```

Expected output:

```text
LEFT
```

### Test 3: `tests/ops.ls`

Checks ASCII and Unicode logic operators, plus precedence.

```ls
; Expected: True
SHOW = \b.b True False
NOT = \p.p FALSE TRUE
AND = \p q.p q FALSE
OR = \p q.p TRUE q
IMP = \p q.OR (NOT p) q
IFF = \p q.AND (IMP p q) (IMP q p)

TEST_AND = ~ (TRUE ^ FALSE)
TEST_OR = TRUE v FALSE
TEST_NOT = ~ FALSE
TEST_IMP = FALSE -> TRUE
TEST_IFF = ~ (TRUE <-> FALSE)
TEST_U_AND = TRUE ∧ TRUE
TEST_U_IMP = FALSE → FALSE
TEST_U_IFF = TRUE ↔ TRUE
TEST_U_NOT = ¬ TRUE <-> FALSE
TEST_PREC_AND_OR = TRUE v FALSE ^ FALSE
TEST_PREC_IFF_IMP = ~ (FALSE <-> FALSE -> TRUE)

ALL = TEST_AND ^ TEST_OR ^ TEST_NOT ^ TEST_IMP ^ TEST_IFF ^ TEST_U_AND ^ TEST_U_IMP ^ TEST_U_IFF ^ TEST_U_NOT ^ TEST_PREC_AND_OR ^ TEST_PREC_IFF_IMP
SHOW ALL
```

Expected output:

```text
True
```

### Test 4: `tests/pairs.ls`

Checks Church pair construction and projection.

```ls
; Expected: BETA
PAIR = \a b f.f a b
FIRST = \p.p (\a b.a)
SECOND = \p.p (\a b.b)
SECOND (PAIR ALPHA BETA)
```

Expected output:

```text
BETA
```

### Test 5: `tests/numerals_iszero.ls`

Checks Church numeral zero detection.

```ls
; Expected: True
SHOW = \b.b True False
NOT = \p.p FALSE TRUE
AND = \p q.p q FALSE
OR = \p q.p TRUE q
IMP = \p q.OR (NOT p) q
IFF = \p q.AND (IMP p q) (IMP q p)

ZERO = \f x.x
ONE = \f x.f x
TWO = \f x.f (f x)
THREE = \f x.f (f (f x))
SUCC = \n f x.f (n f x)
PLUS = \m n f x.m f (n f x)
MULT = \m n f.m (n f)
ISZERO = \n.n (\x.FALSE) TRUE

SHOW (ISZERO ZERO)
```

Expected output:

```text
True
```

### Test 6: `tests/numerals_loop.ls`

Checks bounded iteration through a Church numeral.

```ls
; Expected: STEP (STEP (STEP START))
THREE = \f x.f (f (f x))
THREE STEP START
```

Expected output:

```text
STEP (STEP (STEP START))
```

### Test 7: `tests/arithmetic_eq.ls`

Checks predecessor, subtraction, less-than-or-equal, and numeric equality.

```ls
; Expected: True
SHOW = \b.b True False
NOT = \p.p FALSE TRUE
AND = \p q.p q FALSE
OR = \p q.p TRUE q
IMP = \p q.OR (NOT p) q
IFF = \p q.AND (IMP p q) (IMP q p)

ZERO = \f x.x
ONE = \f x.f x
TWO = \f x.f (f x)
THREE = \f x.f (f (f x))
SUCC = \n f x.f (n f x)
PLUS = \m n f x.m f (n f x)
MULT = \m n f.m (n f)
ISZERO = \n.n (\x.FALSE) TRUE

PRED = \n f x.n (\g h.h (g f)) (\u.x) (\u.u)
SUB = \m n.n PRED m
LEQ = \m n.ISZERO (SUB m n)
NUM_EQ = \m n.AND (LEQ m n) (LEQ n m)

SHOW (NUM_EQ TWO (PRED THREE))
```

Expected output:

```text
True
```

### Test Runner: `scripts/run-tests.sh`

The test runner is intentionally plain shell, so it can run anywhere without extra tooling.

```sh
#!/usr/bin/env sh
set -eu

BIN="${1:-./lambdascript}"

pass=0
fail=0

run_test() {
    file="$1"
    expected="$2"

    out="$("$BIN" -q "$file" 2>/tmp/lambdascript_test_err.$$ || true)"
    err="$(cat /tmp/lambdascript_test_err.$$ 2>/dev/null || true)"
    rm -f /tmp/lambdascript_test_err.$$

    if [ "$out" = "$expected" ]; then
        printf 'PASS %s\n' "$file"
        pass=$((pass + 1))
    else
        printf 'FAIL %s\n' "$file"
        printf '  expected: [%s]\n' "$expected"
        printf '  actual:   [%s]\n' "$out"
        if [ -n "$err" ]; then
            printf '  stderr:   [%s]\n' "$err"
        fi
        fail=$((fail + 1))
    fi
}

run_test tests/identity.ls "Z"
run_test tests/branch.ls "LEFT"
run_test tests/ops.ls "True"
run_test tests/pairs.ls "BETA"
run_test tests/numerals_iszero.ls "True"
run_test tests/numerals_loop.ls "STEP (STEP (STEP START))"
run_test tests/arithmetic_eq.ls "True"

printf '\npassed: %s\nfailed: %s\n' "$pass" "$fail"

if [ "$fail" -ne 0 ]; then
    exit 1
fi
```

Expected output:

```text
PASS tests/identity.ls
PASS tests/branch.ls
PASS tests/ops.ls
PASS tests/pairs.ls
PASS tests/numerals_iszero.ls
PASS tests/numerals_loop.ls
PASS tests/arithmetic_eq.ls

passed: 7
failed: 0
```

### Native XF Bridge Smoke Test

Once XF has `core.lambda`, add a test like this to XF:

```xf
out = core.str.trim(core.lambda.file("vendor/lambdaScript/tests/ops.ls"))

if out == "True" {
    print "PASS core.lambda.file"
} else {
    print "FAIL core.lambda.file: " + out
}
```

And inline eval:

```xf
out = core.str.trim(core.lambda.eval("SHOW = \\b.b True False\nSHOW (~ FALSE)"))

if out == "True" {
    print "PASS core.lambda.eval"
} else {
    print "FAIL core.lambda.eval: " + out
}
```

### Test Design Pattern

Because LambdaScript currently prints one final expression, each test should reduce to one final value.

Use `SHOW` for booleans:

```ls
SHOW = \b.b True False
SHOW (TRUE ^ FALSE)
```

Use symbolic names for numerals:

```ls
THREE STEP START
```

Use free variables for structural outputs:

```ls
COMP = \f g x.f (g x)
COMP F G X
```


## XF Integration

LambdaScript can be called from XF through `core.os.run`.

```xf
out = core.os.run("/Volumes/Experiments/lambdaScript/bin/lambdascript -q /Volumes/Experiments/lambdaScript/tests/ops.ls")
print out
```

A small assertion:

```xf
out = core.str.trim(core.os.run("/Volumes/Experiments/lambdaScript/bin/lambdascript -q /Volumes/Experiments/lambdaScript/tests/ops.ls"))

if out == "True" {
    print "PASS lambdaScript ops"
} else {
    print "FAIL lambdaScript ops: " + out
}
```

Use `core.os.run` for command execution and captured output.

Do not use `core.process.run` for shell commands. That module is for worker-style processing.

---

## Symbolic Models

LambdaScript can represent model structure even before it has numeric primitives.

Example symbolic algebra wrappers:

```ls
ADD = \x y.add x y
SUB = \x y.sub x y
MUL = \x y.mul x y
DIV = \x y.div x y
EXP = \x.exp x
NEG = \x.neg x
```

These do not compute numbers. They reduce LambdaScript structure while leaving primitive names such as `add`, `mul`, and `exp` as symbolic leaves.

That makes LambdaScript useful for expressing formulas, transformations, and model graphs before plugging in a real numeric backend.

---

## Tips for Writing Good LambdaScript

### 1. Keep definitions small

Use short helpers and compose them.

### 2. Put definitions before use

Forward and mutual references are not the happy path yet.

### 3. Use `SHOW` for booleans

Raw Church booleans print as lambdas. `SHOW` makes test output readable.

### 4. Display numerals with symbolic step names

```ls
THREE STEP START
```

is much easier to inspect than raw Church numeral structure.

### 5. Treat hard recursion as a stress test

Recursive programs can grow huge. Use `-n` and `-t` wisely.

### 6. Keep runnable examples single-line per definition

Until multi-line definitions exist, one definition per line is the safe format.

---

## Troubleshooting

### `unexpected trailing input`

Usually an operator/token was not recognized, or the parser did not expect another term.

Check:

* missing parentheses
* using `v` as a variable when `v` means OR
* unsupported literal syntax such as numbers or strings

### `program must end with an expression`

Add a final expression after definitions.

```ls
ID = \x.x
ID Z
```

### Raw output like `\x0 x1.x0`

That is a Church boolean.

Use:

```ls
SHOW = \b.b True False
SHOW TRUE
```

### Very long reduction or step-limit warning

Your expression may be recursive, may duplicate work, or may be using a large Church numeral.

Run with:

```sh
./lambdascript -n 1000000 -q file.ls
```

or reduce the input size.

---

## Current Limitations

LambdaScript does not yet have:

* numeric literals
* string literals
* arrays or records
* imports
* native `if`, `while`, or `for`
* CLI args exposed as `ARGS`
* native primitive bindings
* real integration/summation engines
* multi-line definitions
* direct XF native module bridge

Current data is symbolic unless encoded through lambda calculus.

---

## Roadmap

Near term:

* expand examples and docs
* keep `tests/ops.ls` as a smoke test
* add multi-line definitions
* add import/load support
* add `ARGS`, `ARG0`, `ARG1`, etc.
* add numeric and string literals
* add a source library directory

Mid term:

* native primitive bindings
* structured values
* pretty-printer improvements
* direct XF module bridge
* test runner integration

Long term:

* symbolic transformation system
* proof/rewrite tooling
* model specification layer
* numeric backend
* C embedding API