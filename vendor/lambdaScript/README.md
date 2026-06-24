# LambdaScript

LambdaScript is a tiny lambda-calculus language for symbolic reduction, logic, transformations, model expressions, and a small layer of numeric and mathematical syntax.

It keeps the core deliberately small:

* variables
* lambda abstraction
* function application
* top-level definitions
* normal-order beta reduction

The practical idea is still the same: keep the evaluator monk-clean, then grow power through syntax sugar, prelude definitions, source libraries, and optional native/runtime bindings.

Today that surface layer includes:

* ASCII and Unicode lambda syntax
* Boolean logic operators
* numeric literals and arithmetic operators
* symbolic math forms such as `Σ`, `∫`, and `lim`
* actionable subscripts such as `Nₑ`, `N_e`, `N(ᵢ,ⱼ)`, and `N_(i,j)`
* set-style membership syntax such as `x ∈ S` and `S ∋ x`

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
12. [Numeric and Symbolic Math](#numeric-and-symbolic-math)
13. [Loops and Iteration](#loops-and-iteration)
14. [Common Algorithms](#common-algorithms)
15. [Worked Examples](#worked-examples)
16. [Testing](#testing)
17. [XF Integration](#xf-integration)
18. [Symbolic Models](#symbolic-models)
19. [Tips for Writing Good LambdaScript](#tips-for-writing-good-lambdascript)
20. [Troubleshooting](#troubleshooting)

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
core.lambda.file("tests/ops.lambda")
core.lambda.reduce("(\\x.x) z")
```

---

## Getting Started

Build with your project Makefile, then run the binary.

Typical local run:

```sh
./lambdaScript -q -e 'I z'
```

Expected:

```text
z
```

Run a file:

```sh
./lambdaScript -q tests/ops.lambda
```

Expected:

```text
True
```

Run the current smoke suite:

```sh
make test
```

## CLI Usage

```text
./lambdaScript [-q] [-t] [-n STEPS] [-e EXPR] [FILE] [-- ARGS...]
```

Options:

```text
-e EXPR        evaluate an expression/program string
-n STEPS       maximum reduction steps
-q             quiet mode; suppress [steps=...] on stderr
-t             trace reductions to stderr
--no-prelude   disable built-in I/K/S/TRUE/FALSE
--             stop option parsing; remaining values become ARGS
-h, --help     show help
```

Examples:

```sh
./lambdaScript -q -e 'I z'
./lambdaScript -t -e '(\x.x) z'
./lambdaScript -n 1000000 -q tests/torture.lambda
./lambdaScript -q -e 'ARG1' -- alpha
cat tests/ops.lambda | ./lambdaScript -q
```

### Script arguments

Anything after `--` is exposed to the program as `ARG1`, `ARG2`, ..., `ARGC`, and `ARGS`. `ARG0` is the source name (`<eval>`, `<stdin>`, or the file path).

Example:

```sh
./lambdaScript -q -e 'ARG1' -- alpha
```

Expected:

```text
alpha
```

## A First LambdaScript Program

```ls
ID = \x.x
ID hello
```

Run:

```sh
./lambdaScript -q first.lambda
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

### Literals and identifiers

LambdaScript now accepts numeric literals:

```ls
0
1
2.5
-3
```

Unicode identifiers are allowed, including subscript characters:

```ls
x₀
x₁
Nₑ
β
```

Plain subscripted identifiers can stay ordinary names when you define them directly:

```ls
x₀ = 8
x₀
```

but unresolved subscripted forms can also lower into symbolic math forms such as `sub N e` and `sub N i j`. See [Numeric and Symbolic Math](#numeric-and-symbolic-math).

### Program shape

A program is a list of single-line definitions followed by one final expression.

```ls
A = \x.x
B = \y.y

A B
```

The final expression is required.

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

### Top-level compound definitions

LambdaScript also supports arithmetic update sugar at the top level:

```ls
x = 10
x += 5
x
```

Expected:

```text
15
```

Supported compound forms:

```text
+=  -=  *=  /=  %=
```

These are definition sugar, not general mutable assignment inside arbitrary expressions.

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

### Logic and equivalence operators

ASCII:

```ls
~p
p ^ q
p v q
p -> q
p <-> q
p <=> q
```

Unicode:

```ls
¬p
⌐p
p ∧ q
p ∨ q
p → q
p ↔ q
p ≣ q
p ≡ q
```

Meaning:

* `~`, `¬`, and `⌐` are NOT
* `^` and `∧` are AND
* `v` and `∨` are OR
* `->` and `→` are implication
* `<->` and `↔` are boolean biconditional / IFF
* `<=>`, `≣`, and `≡` are generalized equivalence

That distinction matters:

```ls
SHOW (TRUE <-> TRUE)
SHOW (TRUE <=> TRUE)
```

Both reduce to `True`, but they are not the same operator family. `IFF` is the boolean connective. Generalized equivalence is the broader equality-like operator used for numbers, atoms, structural terms, and symbolic membership patterns.

Precedence, highest to lowest:

```text
~ ¬ ⌐
^ ∧
v ∨
-> →
<-> ↔
<=> ≣ ≡
```

Example:

```ls
SHOW (TRUE ∧ ⌐FALSE)
```

Expected:

```text
True
```

Note: if ASCII `v` is enabled as OR, do not use a bare variable named `v`.

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

## Numeric and Symbolic Math

LambdaScript now has both built-in numeric arithmetic and symbolic math surface forms.

### Numeric literals and arithmetic

Built-in numeric operators:

```ls
+
-
*
/
%
**
```

Examples:

```ls
1 + 2 * 3
2 ** 3 ** 2
√16
㏑ ℯ
```

ASCII aliases are also available:

```ls
sqrt 16
ln euler
inf
INFINITY
```

Common constants and aliases:

```text
∞           infinity
ℯ           euler
√x          sqrt x
㏑ x        ln x
```

Examples:

```ls
x₀ = 8
x₀**(-1/3)
```

Expected:

```text
0.5
```

Use parentheses around fractional or negative exponents when you care about grouping:

```ls
8**(-1/3)
```

### Symbolic math forms

These forms lower to symbolic constructors. They improve notation, but they do not magically perform calculus by themselves.

```ls
Σ i = 0 to n . xᵢ
∫ t = 0 to T . f t
lim x -> 0 . (sin x) / x
```

ASCII forms:

```ls
Sigma i = 0 to n . x_i
Integral t = 0 to T . f t
lim x -> 0 . (sin x) / x
```

Current lowering shape:

```text
Σ i = a to b . expr      -> sigma a b (\i.expr)
∫ x = a to b . expr      -> integral a b (\x.expr)
lim x -> a . expr        -> limit a (\x.expr)
```

These are ideal for symbolic models and readable math pipelines.

### Membership syntax

LambdaScript also supports set-style membership syntax.

Unicode:

```ls
x ∈ S
S ∋ x
S ∍ x
```

ASCII:

```ls
x in S
S contains x
```

Current lowering shape:

```text
x ∈ S        -> elem x S
x in S       -> elem x S
S ∋ x        -> contains S x
S ∍ x        -> contains S x
S contains x -> contains S x
```

If you define `elem` or `contains` as real predicates, these can compute to Church booleans. Otherwise they remain symbolic forms.

### Actionable subscripts

Subscripts are not just pretty names anymore.

Supported forms include:

```ls
Nₑ
N_e
N(ᵢ,ⱼ)
N_(i,j)
xᵢ
x₀
```

Unresolved subscripted forms lower into symbolic `sub` expressions:

```text
Nₑ         -> sub N e
N_e        -> sub N e
N(ᵢ,ⱼ)     -> sub N i j
N_(i,j)    -> sub N i j
xᵢ         -> sub x i
```

But exact user definitions still win:

```ls
x₀ = 8
x₀**(-1/3)
```

Expected:

```text
0.5
```

That combination gives you readable symbolic indexing without breaking normal definitional use.

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
./lambdaScript -n 1000000 -q examples/hard/02_factorial_y.lambda
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

Either ASCII or Unicode NOT works:

```ls
SHOW (~ FALSE)
```

or:

```ls
SHOW (⌐FALSE)
```

Expected:

```text
True
```

### Numeric arithmetic

```ls
1 + 2 * 3
```

Expected:

```text
7
```

```ls
2 ** 3 ** 2
```

Expected:

```text
512
```

### Symbolic sigma

```ls
Σ i = 0 to n . xᵢ
```

Expected:

```text
sigma 0 n (\x0.sub x x0)
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
./lambdaScript -q examples/easy/01_identity.ls
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
TEST_NOT = ⌐FALSE
TEST_IMP = FALSE -> TRUE
TEST_IFF = TRUE <=> TRUE

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
./lambdaScript -n 1000000 -q examples/hard/02_factorial_y.lambda
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

The repository now uses `tests/test.sh` as the primary regression suite.

Run it with:

```sh
chmod +x tests/test.sh
./tests/test.sh
```

or through the Makefile:

```sh
make test
```

The current suite covers:

* core lambda reduction
* stdin and file-based top-level definitions
* comments, trace, and step limits
* arithmetic and numeric aliases
* symbolic math forms
* actionable subscripts
* torture coverage for mixed ASCII/Unicode syntax

Representative files in `tests/`:

```text
math_ascii_aliases.lambda
math_ops.lambda
math_forms.lambda
subscripts.lambda
torture.lambda
validation.lambda
```

### `tests/math_ascii_aliases.lambda`

Checks ASCII aliases for numeric math:

```ls
A = sqrt 16
B = ln euler
C = inf
D = INFINITY
E = SQRT 25
F = LN EULER
cons A (cons B (cons C (cons D (cons E (cons F nil)))))
```

Expected output:

```text
cons 4 (cons 1 (cons ∞ (cons ∞ (cons 5 (cons 1 nil)))))
```

### `tests/math_ops.lambda`

Checks numeric operators and Unicode math forms:

```ls
A = 1 + 2 * 3
B = 2 ** 3 ** 2
C = √16
D = ㏑ ℯ
E = 10 % 4
x₀ = 41
x₀ += 1
cons A (cons B (cons C (cons D (cons E (cons x₀ nil)))))
```

Expected output:

```text
cons 7 (cons 512 (cons 4 (cons 1 (cons 2 (cons 42 nil)))))
```

### `tests/math_forms.lambda`

Checks symbolic math and membership lowering:

```ls
cons (Σ i = 0 to 3 . ADD i 1)
  (cons (∫ x = 0 to 1 . POW x 2)
    (cons (lim x -> 0 . DIV (sin x) x)
      (cons (a ∈ SET)
        (cons (SET ∋ a)
          (cons (a in SET)
            (cons (SET contains a) nil))))))
```

Expected output:

```text
cons (sigma 0 3 (\x0.ADD x0 1)) (cons (integral 0 1 (\x0.POW x0 2)) (cons (limit 0 (\x0.DIV (sin x0) x0)) (cons (elem a SET) (cons (contains SET a) (cons (elem a SET) (cons (contains SET a) nil))))))
```

### `tests/subscripts.lambda`

Checks actionable subscript lowering and numeric interaction:

```ls
A = Nₑ
B = N_e
C = N(ᵢ,ⱼ)
D = N_(i,j)
x₀ = 8
E = x₀**(-1/3)
F = Σ i = 0 to n . xᵢ
cons A (cons B (cons C (cons D (cons E (cons F nil)))))
```

Expected output:

```text
cons (sub N e) (cons (sub N e) (cons (sub N i j) (cons (sub N i j) (cons 0.5 (cons (sigma 0 n (\x0.sub x x0)) nil)))))
```

### `tests/torture.lambda`

A mixed stress test for the currently working surface:

* ASCII and Unicode lambdas
* logic aliases
* Church booleans, pairs, numerals, predecessor, equality
* fixed-point recursion
* deep nested final expression

Run it directly:

```sh
./lambdaScript -q tests/torture.lambda
```

## XF Integration

LambdaScript can be called from XF through `core.os.run`.

```xf
out = core.os.run("/Volumes/Experiments/lambdaScript/bin/lambdaScript -q /Volumes/Experiments/lambdaScript/tests/ops.lambda")
print out
```

A small assertion:

```xf
out = core.str.trim(core.os.run("/Volumes/Experiments/lambdaScript/bin/lambdaScript -q /Volumes/Experiments/lambdaScript/tests/ops.lambda"))

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

Usually an operator or token was recognized differently than you intended.

Check:

* missing parentheses
* using `v` as a variable when `v` means OR
* writing `ln(x)` when you meant adjacency form `ln x`
* mixing `<->` and `<=>` without realizing they are different operators

### `expected ')' at byte ...`

This usually means the parser saw a grouped term and did not find the closing `)` it expected.

Check:

* missing or extra parentheses
* `N(ᵢ,ⱼ)` versus normal grouping
* typos such as an extra trailing `)` in inline `-e` expressions

### Raw output like `\x0 x1.x0`

That is a Church boolean.

Use:

```ls
SHOW = \b.b True False
SHOW TRUE
```

### Symbolic output like `sigma ...`, `integral ...`, `limit ...`, `sub ...`

That is normal.

The `Σ`, `∫`, `lim`, membership, and subscript surface forms lower into symbolic constructors unless you define additional semantics around them.

### Very long reduction or step-limit warning

Your expression may be recursive, may duplicate work, or may be using a large Church numeral.

Run with:

```sh
./lambdaScript -n 1000000 -q file.lambda
```

or reduce the input size.