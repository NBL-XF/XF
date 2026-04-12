# XF Interpreter

A practical, state-aware scripting language for text processing, structured data transformation, regex workflows, dataset manipulation, and concurrent chunk processing.

XF is designed for users who want a language that feels lightweight in the small, but still scales to larger data scripts. It combines:

* a compact expression-oriented syntax
* explicit runtime states
* powerful collection transforms
* regex literals and regex modules
* dataset helpers
* a REPL for exploration
* a bytecode VM for execution

This guide focuses on **how to use XF effectively**.

---

## Table of Contents

1. [Why XF](#why-xf)
2. [Getting Started](#getting-started)
3. [CLI Usage](#cli-usage)
4. [REPL](#repl)
5. [A First XF Script](#a-first-xf-script)
6. [Language Basics](#language-basics)
7. [Types](#types)
8. [Runtime States](#runtime-states)
9. [Variables](#variables)
10. [Functions](#functions)
11. [Expressions and Operators](#expressions-and-operators)
12. [Collections](#collections)
13. [Shorthand Syntax](#shorthand-syntax)
14. [Modules](#modules)
15. [Strings and Regex](#strings-and-regex)
16. [Dataset Workflows](#dataset-workflows)
17. [Process and Worker APIs](#process-and-worker-apis)
18. [Common Algorithms in XF](#common-algorithms-in-xf)
19. [Worked Examples](#worked-examples)
20. [Tips for Writing Good XF](#tips-for-writing-good-xf)
21. [Troubleshooting](#troubleshooting)
22. [Current Limitations](#current-limitations)

---

## Why XF

XF is built for scripts where you care about structure, data flow, and visibility.

It works especially well for:

* CSV and line-based processing
* regex-heavy transformation
* turning raw rows into structured maps
* collection filtering and mapping
* grouping and flattening data
* experimenting in a REPL before writing a full script
* splitting work into chunks for parallel processing

A lot of scripting languages are concise but opaque. XF tries to be concise **and** explicit.

---

## Getting Started

Build the project with your Makefile:

```bash
make clean && make && make install
```

Typical binary names/locations:

* local build: `./bin/release/xf`
* installed binary: `xf`

---

## CLI Usage

XF has three main entry modes.

### Start the REPL

```bash
xf
```

### Run a file

```bash
xf -r script.xf
```

### Execute inline source

```bash
xf -e 'print 1 + 1'
```

### Summary

```text
xf                 # REPL
xf -r <file.xf>    # run file
xf -e "code"      # execute inline source
```

---

## REPL

The REPL is the easiest way to learn the language.

Example session:

```xf
>> 1 + 2
=> 3  [num, OK]

>> arr xs = [1,2,3]
>> xs
=> [1, 2, 3]  [arr, OK]

>> xs[1]
=> 2  [num, OK]

>> 10 |> print
10
=> 10  [num, OK]
```

Typical commands:

```xf
:q
:quit
:stack
```

The REPL is especially useful for:

* testing a single expression
* checking data shapes
* trying regexes
* validating callback functions before using them in larger scripts

---

## A First XF Script

Here is a small script that loads lines from a file, splits them, and prints selected rows.

```xf
BEGIN {
    str path = "data.csv"
    arr lines = core.os.lines(path)

    print "Line count: " .. core.generics.size(lines)
    print "First line: " .. lines[0]
}
```

Run it with:

```bash
xf -r script.xf
```

---

## Language Basics

XF supports:

* numeric expressions
* strings
* arrays, maps, sets, tuples
* named and anonymous functions
* if/else, while, for
* regex literals
* dataset-oriented modules
* pipeline and collection shorthand syntax

### BEGIN / END

Many XF scripts use block structure.

```xf
BEGIN {
    print "start"
}

END {
    print "end"
}
```

### Top-level declarations

You can also declare variables and functions at top level.

```xf
num fn add(num a, num b) {
    return a + b
}
```

---

## Types

Common user-facing types include:

* `num`
* `str`
* `bool`
* `arr`
* `map`
* `set`
* `tuple`
* `fn`
* `regex`
* `void`

### Examples

```xf
num n = 42
str s = "hello"
bool ok = true
arr xs = [1,2,3]
map row = {"name": "alice", "score": 90}
tuple t = (1, "a", true)
```

---

## Runtime States

XF separates **type** from **state**.

This is one of the language’s core ideas.

Common states:

* `OK`
* `ERR`
* `NAV`
* `NULL`
* `VOID`
* `UNDEF`
* `UNDET`
* `TRUE`
* `FALSE`

### Why states matter

A value can have a type and still not be in a normal success state.

This makes failures easier to inspect.

### Important distinction

* **undeclared symbol** -> `UNDET`
* **declared but not initialized** -> typed `UNDEF`

Example:

```xf
>> a
=> undet  [bool, UNDET]

>> num a
>> a
=>   [num, UNDEF]
```

### Practical meaning

* `UNDET` means “you referenced something that does not exist yet”
* `UNDEF` means “you declared it, but it does not have a value yet”
* `NAV` often means “this operation could not produce a valid navigable result”
* `ERR` represents an error value

---

## Variables

Variables are type-led.

```xf
num total = 10
str name = "alice"
arr xs = [1,2,3]
map row = {"id": 1}
```

### Uninitialized declaration

```xf
num total
```

### Assignment

```xf
num x = 10
x = x + 1
x += 5
```

### Index assignment

```xf
arr xs = [1,2,3]
xs[1] = 20

map row = {"name": "alice"}
row["name"] = "bob"
```

---

## Functions

### Named functions

Named functions require a return type.

```xf
num fn add1(num x) {
    return x + 1
}

bool fn is_even(num x) {
    return x % 2 == 0
}
```

### Anonymous functions

Anonymous functions are useful for collection transforms.

```xf
fn(num x) {
    return x + 1
}
```

### Functions over maps

```xf
map fn mark_row(map row) {
    row["checked"] = 1
    return row
}
```

### Higher-order examples

```xf
arr xs = [1,2,3,4]
print(xs [/] add1)
print(xs [*] is_even)
```

---

## Expressions and Operators

### Arithmetic

```xf
1 + 2
5 - 3
4 * 2
9 / 3
5 % 2
2 ^ 3
```

### Comparison

```xf
1 < 2
2 <= 2
3 > 1
4 >= 5
1 == 1
1 != 2
```

### Boolean logic

```xf
true && false
true || false
!false
```

### String concat

```xf
"hello" .. " " .. "world"
```

### Ternary

```xf
num x = 10
print(x > 5 ? "big" : "small")
```

### Coalesce

```xf
print(a ?? b)
```

### Regex match

```xf
print("aaab" ~ /a.*b/)
print("AAAB" ~ /a.*b/i)
```

---

## Collections

### Arrays

```xf
arr xs = [1,2,3]
print(xs[0])
push(xs, 4)
print(xs)
```

### Maps

```xf
map row = {"name": "alice", "score": 90}
print(row["name"])
```

### Sets

```xf
set s = {1,2,3}
```

### Tuples

```xf
tuple t = (1, "a", true)
```

### Delete

```xf
arr xs = [1,2,3,4]
delete xs[1]
print(xs)

map row = {"a": 1, "b": 2}
delete row["a"]
print(row)
```

---

## Shorthand Syntax

XF has a set of shorthand operators for common data operations.

### Pipe and reverse pipe

```xf
x |> f
f <| x
```

Both call `f(x)`.

Examples:

```xf
10 |> print
print <| 10
```

### Push / pop / shift / unshift

```xf
4 => a

a <=

a ==>

0 <== a
```

Equivalent ideas:

* `4 => a` -> push `4` into `a`
* `a <=` -> pop from `a`
* `a ==>` -> shift from `a`
* `0 <== a` -> unshift `0` into `a`

Example:

```xf
arr a = [1,2,3]
4 => a
print(a)
print(a <=)
print(a)
```

### Filter and transform

```xf
xs [*] pred
xs [/] fn
```

Examples:

```xf
bool fn is_even(num x) { return x % 2 == 0 }
num fn add1(num x) { return x + 1 }

print([1,2,3,4] [*] is_even)
print([1,2,3] [/] add1)
```

### Merge and split

```xf
a 3> b
text <3 ","
```

Examples:

```xf
print([1,2] 3> [3,4])
print("a,b,c" <3 ",")
```

### Expand and flatten

```xf
x =->[]
xs []->=
```

Examples:

```xf
print((1,2,3) =->[])
print([[1,2],[3,4],5] []->=)
```

### Regex constructor shorthand

```xf
(*) "a.*b"
```

Example:

```xf
print((*) "a.*b")
print("AAAB" ~ ((*) "a.*b"))
```

---

## Modules

XF includes a module-oriented standard library.

Useful modules include:

* `core.math`
* `core.str`
* `core.regex`
* `core.os`
* `core.format`
* `core.generics`
* `core.ds`
* `core.process`
* `core.edit`
* `core.img`

### Example module calls

```xf
core.os.read(path)
core.os.lines(path)
core.format.json(value)
core.generics.size(xs)
core.ds.row(dataset, 0)
core.regex.test("AAAB", /a.*b/i)
```

---

## Strings and Regex

### Basic string operations

```xf
print(core.str.contains("hello world", "world"))
print(core.str.replace_all("a b c", " ", "-"))
```

### Regex literals

```xf
/a.*b/
/a.*b/i
```

### Regex operator form

```xf
print("aaab" ~ /a.*b/)
print("AAAB" ~ /a.*b/i)
```

### Regex module form

```xf
print(core.regex.test("AAAB", /a.*b/i))
print(core.regex.split("a,b,c", /,/))
```

### Captures and groups

```xf
map m = core.regex.match("name=alice", /name=(\w+)/)
print(core.format.json(m))
```

### Replacement

```xf
print(core.regex.replace_all("A B C", /[A-Z]/, "X"))
```

---

## Dataset Workflows

XF works well with **arr-of-maps** datasets.

### Build a dataset manually

```xf
arr dataset = []
push(dataset, {"name": "alice", "score": 90, "dept": "eng"})
push(dataset, {"name": "bob", "score": 70, "dept": "eng"})
push(dataset, {"name": "carol", "score": 85, "dept": "hr"})
```

### Access rows

```xf
print(core.ds.row(dataset, 0))
```

### Sort

```xf
print(core.ds.sort(dataset, "score"))
```

### Filter

```xf
arr high = core.ds.filter(dataset, fn(map row) {
    return row["score"] > 80
})
print(core.format.json(high))
```

### Flatten nested chunk results

```xf
arr nested = [
    [{"id":1}, {"id":2}],
    [{"id":3}]
]

print(core.ds.flatten(nested))
```

---

## Process and Worker APIs

The `core.process` module is useful for large in-memory jobs.

### Split work into chunks

```xf
arr xs = [1,2,3,4,5,6,7,8]
arr chunks = core.process.split(xs, 3)
print(core.format.json(chunks))
```

### Assign over rows

```xf
map fn mark_row(map row) {
    row["_checked"] = 1
    return row
}

arr assigned = core.process.assign(dataset, mark_row)
```

### Worker jobs

```xf
arr fn process_chunk(arr chunk) {
    return core.process.assign(chunk, fn(map row) {
        row["_parallel"] = 1
        return row
    })
}

arr jobs = []
for (chunk in chunks) {
    push(jobs, core.process.worker(process_chunk, chunk))
}

arr results = core.process.run(jobs)
arr combined = core.ds.flatten(results)
print(core.generics.size(combined))
```

---

## Common Algorithms in XF

This section shows small, common patterns users often want.

### Sum an array

```xf
arr xs = [1,2,3,4]
num total = 0
num i = 0
while (i < xs.len) {
    total = total + xs[i]
    i = i + 1
}
print(total)
```

### Find max value

```xf
arr xs = [4,9,2,7]
num best = xs[0]
num i = 1
while (i < xs.len) {
    if (xs[i] > best) {
        best = xs[i]
    }
    i = i + 1
}
print(best)
```

### Count occurrences

```xf
arr xs = ["a","b","a","c","a","b"]
map counts = {}
num i = 0
while (i < xs.len) {
    str k = xs[i]
    if (counts[k].state == "OK") {
        counts[k] = counts[k] + 1
    } else {
        counts[k] = 1
    }
    i = i + 1
}
print(core.format.json(counts))
```

### Filter then map

```xf
bool fn is_even(num x) { return x % 2 == 0 }
num fn square(num x) { return x * x }

arr xs = [1,2,3,4,5,6]
arr out = (xs [*] is_even) [/] square
print(out)
```

### Split CSV-like text into rows

```xf
str text = "a,b,c\n1,2,3\n4,5,6"
arr lines = text <3 "\n"
print(lines)
```

### Merge arrays

```xf
print([1,2] 3> [3,4])
```

### Flatten nested results

```xf
arr nested = [[1,2],[3,4],5]
print(nested []->=)
```

### Regex-driven extraction

```xf
arr lines = [
    "id=1 user=alice",
    "id=2 user=bob"
]

arr users = lines [/] fn(str line) {
    map m = core.regex.match(line, /user=(\w+)/)
    return m["match"]
}

print(users)
```

---

## Worked Examples

### Example 1: Small CSV reader

```xf
BEGIN {
    str path = "people.csv"
    arr lines = core.os.lines(path)

    if (core.generics.size(lines) < 2) {
        print "empty or invalid file"
        exit(1)
    }

    arr headers = core.generics.split(lines[0], ",")
    arr rows = []

    num i = 1
    while (i < core.generics.size(lines)) {
        arr fields = core.generics.split(lines[i], ",")
        map row = {}

        num j = 0
        while (j < headers.len) {
            row[headers[j]] = fields[j]
            j = j + 1
        }

        push(rows, row)
        i = i + 1
    }

    print("Rows loaded: " .. core.generics.size(rows))
    print(core.format.json(core.ds.row(rows, 0)))
}
```

### Example 2: Mark rows and flatten worker output

```xf
BEGIN {
    arr dataset = [
        {"name": "alice", "score": 90, "dept": "eng"},
        {"name": "bob",   "score": 70, "dept": "eng"},
        {"name": "carol", "score": 85, "dept": "hr"}
    ]

    arr fn process_chunk(arr chunk) {
        return core.process.assign(chunk, fn(map row) {
            row["_parallel"] = 1
            return row
        })
    }

    arr chunks = core.process.split(dataset, 2)
    arr jobs = []

    for (chunk in chunks) {
        push(jobs, core.process.worker(process_chunk, chunk))
    }

    arr results = core.process.run(jobs)
    arr combined = core.ds.flatten(results)

    print(core.format.json(combined))
}
```

### Example 3: Regex-based log scan

```xf
BEGIN {
    arr lines = [
        "INFO user=alice action=login",
        "ERROR user=bob action=save",
        "INFO user=carol action=logout"
    ]

    arr errors = lines [*] fn(str line) {
        return line ~ /ERROR/
    }

    print(errors)
}
```

### Example 4: Group-like manual aggregation

```xf
BEGIN {
    arr rows = [
        {"dept": "eng", "score": 90},
        {"dept": "eng", "score": 70},
        {"dept": "hr",  "score": 85}
    ]

    map grouped = {}
    num i = 0
    while (i < rows.len) {
        map row = rows[i]
        str dept = row["dept"]

        if (grouped[dept].state != "OK") {
            grouped[dept] = []
        }

        push(grouped[dept], row)
        i = i + 1
    }

    print(core.format.json(grouped))
}
```

### Example 5: End-to-end dataset pipeline

```xf
BEGIN {
    arr dataset = [
        {"name": "alice", "score": 90},
        {"name": "bob",   "score": 70},
        {"name": "carol", "score": 85}
    ]

    bool fn passing(map row) {
        return row["score"] >= 80
    }

    map fn mark(map row) {
        row["status"] = "pass"
        return row
    }

    arr out = (dataset [*] passing) [/] mark
    print(core.format.json(out))
}
```

---

## Tips for Writing Good XF

### 1. Prefer arr-of-maps for structured data

This shape works well with:

* `core.ds`
* `core.process.assign`
* `core.ds.flatten`
* row-wise callbacks

### 2. Use `core.generics.size(...)` for module-level size checks

That is the safest and most consistent collection-size API.

### 3. Use the REPL before writing the full script

Especially for:

* regex testing
* callback functions
* collection pipelines
* data shape inspection

### 4. Print shapes early

```xf
print(core.format.json(core.ds.row(dataset, 0)))
```

This prevents a lot of confusion in bigger scripts.

### 5. Keep worker outputs composable

A good worker result shape is usually:

* arr of maps
* arr of scalar values
* arr of chunk results that can be flattened later

---

## Troubleshooting

### `attempt to call non-function`

This usually means one of the following:

* you called a missing module member
* you referenced a value that is not a function
* a module alias/function name differs from what you expected

Check the callee directly in REPL:

```xf
core.ds.flatten |> print
core.generics.size |> print
```

### `NAV`

This usually means an operation could not produce a valid result.

Examples:

* file open failed
* module function returned no navigable result
* bad input shape for an operation

Inspect state directly:

```xf
print(core.os.read(path).state)
print(core.os.lines(path).state)
```

### `UNDET`

You referenced something undeclared.

### `UNDEF`

You declared a variable but did not initialize it.

### Regex not matching

Test both forms:

```xf
print("AAAB" ~ /a.*b/i)
print(core.regex.test("AAAB", /a.*b/i))
```

If module form works but operator form does not, that points to an operator-path/runtime issue.

---

## Current Limitations

Some areas are still evolving.

* REPL UX may continue to improve
* some APIs may prefer `size(...)` over `length(...)`
* top-level expression/block ambiguity can still matter in some forms
* concurrent execution is improving, but long-term isolation should continue moving toward per-interpreter state

---

## Shorthand Loop Forms

XF supports standard `while` and `for` forms, and some builds may also enable shorthand loop forms for compact scripts.

For user-facing documentation, the standard forms are the safest to rely on because they are always the clearest:

```xf
num i = 0
while (i < 10) {
    print i
    i = i + 1
}

for (item in [1,2,3]) {
    print item
}
```

Use shorthand loop forms when the body is very small and the intent is obvious. Use the standard forms whenever readability matters more than brevity.

## Additional Algorithm Examples

### Fibonacci sequence

```xf
num fn fib(num n) {
    if (n <= 1) {
        return n
    }

    num a = 0
    num b = 1
    num i = 2

    while (i <= n) {
        num next = a + b
        a = b
        b = next
        i = i + 1
    }

    return b
}

BEGIN {
    arr seq = []
    num i = 0
    while (i < 10) {
        push(seq, fib(i))
        i = i + 1
    }
    print(seq)
}
```

### Tower of Hanoi

```xf
void fn hanoi(num n, str src, str aux, str dst) {
    if (n == 1) {
        print "move disk 1 from " .. src .. " to " .. dst
        return
    }

    hanoi(n - 1, src, dst, aux)
    print "move disk " .. n .. " from " .. src .. " to " .. dst
    hanoi(n - 1, aux, src, dst)
}

BEGIN {
    hanoi(3, "A", "B", "C")
}
```

### Bubble sort

```xf
arr fn bubble_sort(arr xs) {
    arr out = xs
    num n = out.len
    num i = 0

    while (i < n) {
        num j = 0
        while (j < n - i - 1) {
            if (out[j] > out[j + 1]) {
                num tmp = out[j]
                out[j] = out[j + 1]
                out[j + 1] = tmp
            }
            j = j + 1
        }
        i = i + 1
    }

    return out
}

BEGIN {
    print(bubble_sort([5,1,4,2,8]))
}
```

### Quicksort

```xf
arr fn quick_sort(arr xs) {
    if (xs.len <= 1) {
        return xs
    }

    num pivot = xs[0]
    arr less = []
    arr equal = []
    arr greater = []

    num i = 0
    while (i < xs.len) {
        num v = xs[i]
        if (v < pivot) {
            push(less, v)
        } else if (v > pivot) {
            push(greater, v)
        } else {
            push(equal, v)
        }
        i = i + 1
    }

    return quick_sort(less) 3> equal 3> quick_sort(greater)
}

BEGIN {
    print(quick_sort([5,1,4,2,8,5,3]))
}
```

### Recursive directory-style traversal pattern

```xf
void fn walk(arr nodes) {
    num i = 0
    while (i < nodes.len) {
        map node = nodes[i]
        print(node["name"])

        if (node["children"].state == "OK") {
            walk(node["children"])
        }

        i = i + 1
    }
}
```

### Parallel row marking pipeline

```xf
BEGIN {
    arr dataset = [
        {"name": "alice", "score": 90},
        {"name": "bob", "score": 70},
        {"name": "carol", "score": 85},
        {"name": "dave", "score": 92}
    ]

    arr fn process_chunk(arr chunk) {
        return core.process.assign(chunk, fn(map row) {
            row["passed"] = row["score"] >= 80
            return row
        })
    }

    arr chunks = core.process.split(dataset, 2)
    arr jobs = []

    for (chunk in chunks) {
        push(jobs, core.process.worker(process_chunk, chunk))
    }

    arr results = core.process.run(jobs)
    arr combined = core.ds.flatten(results)

    print(core.format.json(combined))
}
```

## Final Notes

XF is at its best when you:

* inspect shapes early
* use arr-of-maps for structured data
* keep transforms small and composable
* use the REPL to validate ideas first
* leverage shorthand syntax where it improves readability

A small amount of discipline goes a long way, and XF rewards scripts that make data flow explicit.