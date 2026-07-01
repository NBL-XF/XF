# ByteLang

ByteLang is a programmable scripting language built around **deliberate lack of ready-made abstraction**.

It is meant to be a small, moldable language surface where you can hand-roll the abstractions you want instead of inheriting a large fixed worldview. The point is not to ship a mountain of batteries. The point is to give you a controllable substrate for building both **low-level** and **high-level** development styles in your own shape.

That means ByteLang is designed around a few core ideas:

- **minimal built-in abstraction**
- **programmable syntax surface**
- **user-defined structure over framework-defined structure**
- **direct low-level work when you want it**
- **room to grow custom higher-level abstractions on top**

Today’s implementation is still early, but the direction is already visible: named functions, manual state, buffers, arrays, control flow, and reserved syntax for macro/native extension.

---

## What ByteLang is

ByteLang is:

- a small imperative scripting language
- a language for hand-rolled abstractions
- a place to build custom conventions instead of inheriting fixed ones
- useful for low-level tasks like buffer work and memory-shaped programming
- also usable for higher-level scripting once you define your own helpers and patterns

ByteLang is not trying to be “tiny JavaScript,” “tiny Python,” or “tiny Lua with different keywords.”

It is closer to:

- a programmable substrate
- a language core you can shape
- a scripting system that can stay close to the machine when needed, but does not force you to stay there

---

## What ByteLang is not

ByteLang is **not** about shipping lots of ready-made abstractions such as:

- classes
- modules
- objects
- closures
- rich standard containers
- built-in iterators
- built-in algebraic data types
- large standard-library opinions

The intended style is:

1. start from a small core
2. define your own helpers
3. define your own conventions
4. layer your own abstractions upward as needed

---

## Current Implementation Snapshot

The current codebase already supports a usable core:

- integers
- strings
- buffers
- arrays
- variables
- assignment
- arithmetic and comparison
- `if` / `else`
- `while`
- named functions
- manual allocation-oriented operations
- a few runtime builtins such as `print`, `println`, `assert`, `len`, `array_*`, `malloc`, and `realloc`

The parser also already reserves syntax for extension-oriented forms:

- `define macro ...;`
- `#define fn ...;`

Those forms are part of the language direction, but in the current tree they are still placeholder/parsing hooks rather than a full macro-expansion or native-binding system.

---

## Why ByteLang

ByteLang exists for cases where you do **not** want a language to decide your abstractions for you too early.

Good fits include:

- experimenting with custom DSL layers
- building your own library conventions
- low-level scripting around buffers and arrays
- embedding a small language in a C codebase
- prototyping syntax and semantics before a larger language layer exists
- mixing low-level control with higher-level helpers you define yourself

In other words: ByteLang tries to stay small enough that you can still bend it.

---

## Building

The provided Makefile builds both the library and the CLI.

```sh
make
```

Outputs:

```text
build/bin/bytelang
build/bin/libbl.a
```

Clean:

```sh
make clean
```

Run the test target if present:

```sh
make check
```

---

## CLI Usage

```text
build/bin/bytelang [--check] [--print-result] file.bl
build/bin/bytelang --help
```

Options:

```text
--check         parse/validate only
--print-result  print the final returned value, if any
--help          show usage
```

Notes:

- normal script output usually comes from `print(...)` or `println(...)`
- `--print-result` is only for a script's final returned value
- `print` does not append a newline
- `println` does append a newline

Example:

```sh
build/bin/bytelang hello.bl
build/bin/bytelang --check hello.bl
build/bin/bytelang --print-result returns_value.bl
```

---

## A Tiny First Program

```bl
println("hello, bytelang");
```

---

## Language Shape

ByteLang currently uses C-like statement syntax with semicolons and braces.

### Comments

```bl
// line comment
```

### Variables

Variables are declared with `new`:

```bl
new int count = 0;
new string name = "ByteLang";
new buffer mem = malloc 16;
new array xs = array_new(4);
```

The type word is currently descriptive rather than strongly enforced at parse time. The runtime value model is the real source of truth.

### Assignment

```bl
count = count + 1;
mem[0] = 65;
```

### Expressions

Current expression forms include:

- integer literals
- string literals
- identifiers
- unary `-` and `!`
- `+ - * / %`
- `== != < <= > >=`
- assignment
- function calls
- buffer indexing

### Blocks

```bl
{
    println("inside block");
}
```

### Conditionals

```bl
if (count < 10) {
    println("small");
} else {
    println("big");
}
```

### Loops

```bl
new int i = 0;
while (i < 3) {
    println(i);
    i = i + 1;
}
```

---

## Functions

ByteLang supports named functions:

```bl
define fn add(a, b) {
    return a + b;
}

println(add(2, 3));
```

This is the current main abstraction-building tool: you make your own vocabulary by defining functions instead of relying on lots of built-in forms.

### Important current semantics

In the current implementation:

- functions are called by **name**
- indirect calls are not supported
- functions are **not** first-class values
- functions do **not** capture closures
- function execution runs in a fresh environment chained to globals

So the present model is still intentionally simple and explicit.

---

## The ByteLang Style: Hand-Rolled Abstraction

The point of ByteLang is not the builtins by themselves. The point is what you build from them.

A ByteLang program is expected to grow its own surface area through named helpers and conventions.

Example:

```bl
define fn inc(x) {
    return x + 1;
}

define fn clamp_small(x) {
    if (x < 0) {
        return 0;
    }
    if (x > 255) {
        return 255;
    }
    return x;
}

new int value = 41;
value = inc(value);
value = clamp_small(value);
println(value);
```

That is the intended direction: build your own layer instead of asking the runtime to pre-package one for you.

---

## Low-Level Development

ByteLang is comfortable staying low-level when needed.

### Buffers

Allocate a raw buffer:

```bl
new buffer mem = malloc 8;
```

Resize it:

```bl
mem = realloc mem 16;
```

Indexing works on buffers:

```bl
mem[0] = 65;
mem[1] = 66;
println(mem[0]);
println(mem[1]);
```

Release underlying storage:

```bl
free mem;
delete mem;
```

### Notes

- buffer indexing expects an integer index
- assigned values are stored as bytes
- out-of-bounds indexing is a runtime error

Example:

```bl
new buffer mem = malloc 4;
mem[0] = 72;
mem[1] = 73;
println(mem[0]);
println(mem[1]);
free mem;
delete mem;
```

---

## Higher-Level Development

ByteLang can also move upward, but it expects you to do the lifting.

You can build your own higher-level conventions by composing functions and arrays instead of waiting for the language to provide fixed abstractions.

Example:

```bl
define fn sum3(a, b, c) {
    return a + b + c;
}

define fn is_nonzero(x) {
    return x != 0;
}

new int total = sum3(10, 20, 30);

if (is_nonzero(total)) {
    println(total);
}
```

That is a small example, but it reflects the larger idea: **ByteLang should be able to serve both low-level and high-level work without forcing either style.**

---

## Strings

Strings are built-in runtime values.

```bl
new string a = "hello";
new string b = string_new("world");
println(a);
println(b);
```

Useful helpers:

```bl
len(value)
string_eq(a, b)
```

Example:

```bl
new string a = "byte";
new string b = "lang";

println(len(a));
println(string_eq(a, b));
```

---

## Arrays

Arrays are available as a minimal mutable container.

Create one:

```bl
new array xs = array_new(2);
```

Set and get elements:

```bl
array_set(xs, 0, 10);
array_set(xs, 1, 20);

println(array_get(xs, 0));
println(array_get(xs, 1));
println(array_len(xs));
```

Grow an array:

```bl
array_grow(xs, 8);
```

Release storage:

```bl
free xs;
delete xs;
```

This is intentionally bare. The expectation is that richer collection behavior gets built as conventions and helpers, not gifted wholesale by the core language.

---

## Builtins

The current runtime exposes these builtins:

```text
print(value)
println(value)
assert(condition)

len(value)

string_new(value)
string_eq(a, b)

array_new(size)
array_grow(array, size)
array_set(array, index, value)
array_get(array, index)
array_len(array)

malloc size
realloc buffer size
```

Notes:

- `print` writes without a newline
- `println` writes with a newline
- `assert` raises a runtime error if the value is not truthy
- `malloc` / `realloc` are parsed as special forms
- `len` works on strings, buffers, and arrays

---

## Truthiness

Current truthiness is intentionally simple:

- `null` is falsey
- integer `0` is falsey
- empty strings are falsey
- empty buffers/arrays are falsey
- everything else is truthy

That makes `if` and `while` useful without needing a dedicated boolean type yet.

---

## Custom Syntax and Extensibility

A major part of ByteLang’s identity is that it is **meant to admit custom syntax and custom abstraction layers**.

The current parser already reserves these forms:

```bl
define macro NAME VALUE;
#define fn name "native snippet";
```

In the current repository state, these are **early hooks / placeholders**, not a full macro engine or complete native extension system.

So the honest way to describe ByteLang today is:

- it already points toward programmable syntax
- it already avoids overcommitting to fixed abstractions
- it already supports hand-rolled abstraction through functions and conventions
- its custom-syntax story is part of the design direction, with parser-level groundwork present now

---

## Worked Examples

### Example 1: manual counter

```bl
new int i = 0;
while (i < 5) {
    println(i);
    i = i + 1;
}
```

### Example 2: small abstraction layer

```bl
define fn twice(x) {
    return x * 2;
}

define fn bump(x) {
    return x + 1;
}

new int value = 10;
value = bump(twice(value));
println(value);
```

### Example 3: raw buffer work

```bl
new buffer mem = malloc 4;
mem[0] = 10;
mem[1] = 20;
mem[2] = 30;
mem[3] = 40;

println(mem[2]);

free mem;
delete mem;
```

### Example 4: simple array workflow

```bl
new array xs = array_new(3);

array_set(xs, 0, 7);
array_set(xs, 1, 8);
array_set(xs, 2, 9);

println(array_get(xs, 1));
println(array_len(xs));

free xs;
delete xs;
```

### Example 5: script returns a value

```bl
define fn add(a, b) {
    return a + b;
}

return add(20, 22);
```

Run:

```sh
build/bin/bytelang --print-result return_value.bl
```

Expected output:

```text
42
```

---

## Embedding in C

ByteLang also ships a small C API through `bl.h`.

Core entry points:

```c
BLInterpreter *bl_interpreter_create(void);
void bl_interpreter_destroy(BLInterpreter *interp);

BLStatus bl_check_file(BLInterpreter *interp, const char *path);
BLStatus bl_check_source(BLInterpreter *interp, const char *virtual_path, const char *source);

BLStatus bl_run_file(BLInterpreter *interp, const char *path, BLValueView *out_value);
BLStatus bl_run_source(BLInterpreter *interp, const char *virtual_path, const char *source, BLValueView *out_value);

const char *bl_last_error(const BLInterpreter *interp);
```

Minimal example:

```c
#include "bl.h"
#include <stdio.h>

int main(void) {
    const char *source =
        "define fn add(a, b) { return a + b; }"
        "return add(20, 22);";

    BLInterpreter *interp = bl_interpreter_create();
    BLValueView out = {0};

    if (bl_run_source(interp, "<memory>", source, &out) != BL_STATUS_OK) {
        fprintf(stderr, "%s\n", bl_last_error(interp));
        bl_interpreter_destroy(interp);
        return 1;
    }

    if (out.kind == BL_VALUE_INT) {
        printf("%lld\n", out.as.int_value);
    }

    bl_interpreter_destroy(interp);
    return 0;
}
```

---

## Current Limitations

ByteLang is intentionally small, and the current implementation is still early.

Right now it does **not** yet provide:

- first-class functions
- closures
- modules/imports
- user-defined types
- objects/classes
- rich standard-library abstractions
- a completed macro-expansion system
- a completed native-binding system behind `#define fn`
- full indirect dispatch
- block-local lexical scope separate from the current environment model

This is not a bug in the language identity. A lot of it is the point: keep the core small, programmable, and open to hand-built structure.

---

## Design Summary

ByteLang is best understood like this:

> a programmable scripting language with no real ready-made abstractions, built so you can create your own syntax and hand-rolled abstraction layers for both low-level and high-level development

That is the center of gravity.

The current implementation already shows the shape:

- minimal core
- explicit state
- low-level access
- user-defined helpers
- reserved extension syntax
- room to grow upward without giving up control