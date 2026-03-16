# XF — The Stream-Processing Scripting Language

XF is a statically-typed, stream-processing scripting language inspired by AWK, with first-class threading, a rich type system, a quantum-inspired state model, and a comprehensive standard library. It is designed to process structured text and data files — CSV, TSV, JSON — with concise, expressive syntax. XF can be used as a standalone interpreter or embedded into C applications via its public API (`libxf`).

---

## Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Program Structure](#program-structure)
4. [Types](#types)
5. [States](#states)
6. [Variables and Declarations](#variables-and-declarations)
7. [Operators](#operators)
8. [Control Flow](#control-flow)
9. [Functions](#functions)
10. [Collections](#collections)
11. [Iteration and Loops](#iteration-and-loops)
12. [Pattern-Action Rules](#pattern-action-rules)
13. [Threading](#threading)
14. [Import](#import)
15. [Core Module Reference](#core-module-reference)
16. [Built-in Functions](#built-in-functions)
17. [Small Examples (15–20)](#small-examples)
18. [Algorithms (5–10)](#algorithms)
19. [Larger Examples (5–10)](#larger-examples)

---

## Overview

XF processes input line by line (like AWK) or can run entirely in `BEGIN`/`END` blocks without any input. Key features:

- **Static types** — `num`, `str`, `arr`, `map`, `set`, `tuple`, `fn`
- **State system** — every value carries a lifecycle state (`OK`, `ERR`, `NAV`, `NULL`, `VOID`, `UNDEF`)
- **Stream processing** — `BEGIN { }`, pattern-action rules, `END { }` over input records
- **Threading** — `spawn`/`join` for parallel work
- **Core library** — `core.math`, `core.str`, `core.regex`, `core.format`, `core.os`, `core.ds`
- **Import** — load other `.xf` files

---

## Quick Start

```xf
# hello.xf
BEGIN {
    print "Hello, XF!"
}
```

```xf
# sum_csv.xf  — sum the third column of a CSV file
FS = ","
num total = 0
NR > 1 { total += num($3) }
END { print "Total:", total }
```

Run with:

```bash
xf -f hello.xf
xf -f sum_csv.xf data.csv
```

---

## Program Structure

An XF program consists of:

- **Top-level variable declarations** — outside all blocks
- **Function declarations** — `type fn name(params) { body }`
- **`BEGIN { }`** — runs once before any input is processed
- **Pattern-action rules** — `pattern { body }` — run for each input record
- **`END { }`** — runs once after all input is processed

```xf
FS = ","            # set field separator (runs before input)

num total = 0       # top-level variable

num fn double(num x) { return x * 2 }

BEGIN {
    print "Starting"
}

NR > 1 {            # skip header row
    total += num($3)
}

END {
    print "Total:", total
}
```

---

## Types

| Type    | Description                              | Example literal          |
|---------|------------------------------------------|--------------------------|
| `num`   | 64-bit floating-point number             | `42`, `3.14`, `-7`       |
| `str`   | Reference-counted UTF-8 string           | `"hello"`, `""`          |
| `arr`   | Ordered, resizable array                 | `[1, 2, 3]`              |
| `map`   | Ordered key-value map (string keys)      | `{"a": 1, "b": 2}`       |
| `set`   | Unique string set                        | `{"red", "green"}`       |
| `tuple` | Fixed-length heterogeneous sequence      | `("key", 99)`            |
| `fn`    | Function value                           | `fn(num x) { return x }` |
| `void`  | No value (return type only)              | —                        |

### Type Properties

Every value exposes three read-only properties:

```xf
str s = "hello"
print s.type    # 2  (XF_TYPE_STR)
print s.state   # 0  (XF_STATE_OK)
print s.len     # 5
```

Type numeric codes: `void=0`, `num=1`, `str=2`, `map=3`, `set=4`, `arr=5`, `fn=6`, `tuple=9`

### Type Casting

```xf
num n  = num("42")      # str → num
str s  = str(123)       # num → str
num x  = num("bad")     # fails → NAV state
print (x ?? 0)          # recover with ??
```

---

## States

Every value carries a **state** alongside its data. State takes priority over type in all operations.

| State   | Code | Meaning                                                 |
|---------|------|---------------------------------------------------------|
| `OK`    | 0    | Value is valid and usable                               |
| `ERR`   | 1    | Value carries a fault (e.g. division by zero)           |
| `VOID`  | 2    | No return expected; value was discarded                 |
| `NULL`  | 3    | No return expected; none was given                      |
| `NAV`   | 4    | Return expected, but nothing was returned               |
| `UNDEF` | 5    | Variable declared but not yet assigned                  |

States propagate through arithmetic and expressions — if one operand is `ERR`, the result is `ERR`. Use `??` (null coalescing) to recover:

```xf
num bad = 1 / 0         # ERR
num ok  = bad ?? 0      # 0  — recovered
```

Check state explicitly:

```xf
num x = some_fn()
if (x.state == 1) { print "error occurred" }
if (x.state == 4) { print "NAV — function returned nothing" }
```

---

## Variables and Declarations

Variables must be declared with a type keyword before use:

```xf
num   x = 10
str   name = "alice"
arr   items = [1, 2, 3]
map   config = {"host": "localhost", "port": "8080"}
set   colors = {"red", "green", "blue"}
tuple point = (3.0, 4.0)
```

Declared but not assigned:

```xf
num x          # state is UNDEF until assigned
x = 42         # now OK
```

### Walrus Operator `:=`

Declare and assign in expression position:

```xf
if ((result := compute()) > 0) {
    print "got:", result
}
```

### Tuple Destructuring

```xf
tuple t = ("alice", 30)
str name
num age
name, age = t
print name    # alice
print age     # 30
```

### Multi-Assignment

```xf
num a = 5
num b = 3
a, b = b, a      # swap — not yet supported; use temp var
```

---

## Operators

### Arithmetic

```xf
x + y    x - y    x * y    x / y    x % y    x ^ y
x += 1   x -= 1   x *= 2   x /= 2   x %= 3
x++      x--      ++x      --x
```

### Comparison

```xf
x == y    x != y    x < y    x > y    x <= y    x >= y
x <=> y   # spaceship: -1, 0, or 1
```

### Logical

```xf
x && y    x || y    !x
```

### String Concatenation

```xf
str s = "hello" .. ", " .. "world"
s ..= "!"      # append in-place
```

### Regex Match

```xf
str s = "hello world"
print s ~ /world/       # 1 (match)
print s !~ /xyz/        # 1 (no match)
print s ~ /WORLD/i      # 1 (case-insensitive)
```

Regex flags: `i` (case-insensitive), `m` (multiline), `g` (global), `x` (extended)

### Null Coalescing

```xf
print (x ?? default)          # use x if OK, else default
print (a ?? b ?? c ?? 0)      # chain
```

### Ternary

```xf
num max = a > b ? a : b
```

### Pipe Function

```xf
arr result = data |> transform |> filter
```

### Spaceship

```xf
print (1 <=> 2)        # -1
print ("a" <=> "b")    # -1
print (2 <=> 2)        # 0
```

---

## Control Flow

### If / Elif / Else

```xf
if (x > 0) {
    print "positive"
} elif (x < 0) {
    print "negative"
} else {
    print "zero"
}
```

### While

```xf
num i = 0
while (i < 10) {
    print i
    i += 1
}
```

### Shorthand While (`<>`)

```xf
num n = 5
n > 0 <> { print n; n -= 1 }
```

### For

```xf
for (x in [1, 2, 3, 4, 5]) {
    print x
}
```

With index:

```xf
for ((i, x) in arr) {
    print i, x
}
```

Iterating maps:

```xf
for ((k, v) in mymap) {
    print k, "=", v
}
```

### Shorthand For (`[]>`)

```xf
arr items = [10, 20, 30]
items[x] > print x

# With index
items[i, x] > print i, x

# With block body
items[x] > {
    print "item:", x
}
```

### Break / Next / Exit

```xf
while (true) {
    if (done) break
}

NR > 0 {
    if ($1 == "skip") next   # skip to next record
}

if (error) exit(1)           # exit with code
```

---

## Functions

### Declaration

```xf
num fn add(num a, num b) {
    return a + b
}

str fn greet(str name) {
    return "Hello, " .. name
}

void fn log(str msg) {
    print "[LOG]", msg
}
```

### Default Parameters

```xf
num fn power(num base, num exp = 2) {
    return base ^ exp
}
print power(3)      # 9
print power(3, 3)   # 27
```

### Anonymous Functions

```xf
fn square = fn(num x) { return x * x }
print square(5)     # 25
```

### Recursion

```xf
num fn fib(num n) {
    if (n <= 1) { return n }
    return fib(n - 1) + fib(n - 2)
}
print fib(10)    # 55
```

---

## Collections

### Arrays

```xf
arr a = [1, 2, 3]
a[0]           # 1
a[2] = 99      # set
push(a, 4)     # append
pop(a)         # remove last
shift(a)       # remove first
unshift(a, 0)  # prepend
a.len          # length
delete a[1]    # remove by index
```

### Maps

```xf
map m = {"name": "alice", "age": 30}
m["name"]             # "alice"
m["city"] = "NYC"     # add key
delete m["age"]       # remove key
has(m, "name")        # 1
keys(m)               # arr of keys
values(m)             # arr of values
```

### Sets

```xf
set colors = {"red", "green", "blue"}
push(colors, "yellow")     # add
has(colors, "red")         # 1
remove(colors, "green")    # remove
```

### Tuples

```xf
tuple t = ("alice", 30, "NYC")
t[0]       # "alice"
t[1]       # 30
t.len      # 3

# Destructure
str name
num age
str city
name, age, city = t
```

### Nested Structures

```xf
arr records = [
    {"name": "alice", "score": 95},
    {"name": "bob",   "score": 87}
]
print records[0]["name"]    # alice
print records[1]["score"]   # 87
```

---

## Iteration and Loops

Arrays, maps, sets, and tuples are all iterable:

```xf
# Array
arr a = [10, 20, 30]
for (x in a) { print x }

# Array with index
for ((i, x) in a) { print i, x }

# Map
map m = {"a": 1, "b": 2}
for ((k, v) in m) { print k, v }

# Tuple
tuple t = (1, 2, 3)
for (x in t) { print x }

# Shorthand
a[x] > print x
a[i, x] > print i, x
m[k, v] > print k, v
```

---

## Pattern-Action Rules

Pattern-action rules execute for each input record:

```xf
FS = ","

# Run for every record
{ print $1, $2 }

# Run only when NR > 1 (skip header)
NR > 1 { print $1 }

# Run when field matches regex
$2 ~ /^[A-Z]/ { print "uppercase name:", $2 }

# Built-in record variables
# $0      — full record (raw line)
# $1..$N  — individual fields
# NR      — current record number (global)
# NF      — number of fields in current record
# FNR     — record number within current file
# FS      — field separator (default: whitespace)
# RS      — record separator (default: newline)
# OFS     — output field separator
# ORS     — output record separator
```

---

## Threading

```xf
num fn worker(num id) {
    num sum = 0
    num i = 0
    while (i < 1000) { sum += i; i += 1 }
    return sum + id
}

# Spawn returns a thread handle (num)
num tid1 = spawn worker(1)
num tid2 = spawn worker(2)

# join() blocks until the thread completes and returns its result
num r1 = join(tid1)
num r2 = join(tid2)
print r1, r2

# Statement form — join without capturing return value
join tid1
```

Set the maximum concurrent jobs before spawning:

```xf
# In the embedding API: xf_set_max_jobs(xf, 8)
```

---

## Import

```xf
import "mathlib.xf"

# All top-level functions and variables from mathlib.xf
# are now available in the current script.
print clamp(150, 0, 100)    # 100
print PI                    # 3.14159...

# Double import of the same path is a no-op.
import "mathlib.xf"
```

---

## Core Module Reference

### `core.math`

```xf
core.math.sin(x)      core.math.cos(x)     core.math.tan(x)
core.math.asin(x)     core.math.acos(x)    core.math.atan(x)
core.math.atan2(y,x)
core.math.sqrt(x)     core.math.pow(x,y)   core.math.exp(x)
core.math.log(x)      core.math.log2(x)    core.math.log10(x)
core.math.abs(x)      core.math.floor(x)   core.math.ceil(x)
core.math.round(x)    core.math.int(x)
core.math.min(a,b)    core.math.max(a,b)   core.math.clamp(v,lo,hi)
core.math.rand()      core.math.srand(seed)
core.math.PI          core.math.E          core.math.INF
```

### `core.str`

```xf
core.str.len(s)
core.str.upper(s)       core.str.lower(s)
core.str.trim(s)        core.str.ltrim(s)    core.str.rtrim(s)
core.str.substr(s,start,len)
core.str.index(s,sub)
core.str.contains(s,sub)
core.str.starts_with(s,prefix)
core.str.ends_with(s,suffix)
core.str.replace(s,old,new)
core.str.replace_all(s,old,new)
core.str.repeat(s,n)
core.str.reverse(s)
core.str.split(s,sep)
core.str.join(arr,sep)
core.str.concat(...)    # variadic
core.str.sprintf(fmt, ...)
```

### `core.regex`

```xf
core.regex.test(str, pattern)              # → num (1/0)
core.regex.test(str, pattern, flags)
core.regex.match(str, pattern)             # → map {match, index, groups}
core.regex.groups(str, pattern)            # → arr of capture strings
core.regex.search(str, pattern)            # → arr of all match maps
core.regex.replace(str, pattern, rep)      # first match
core.regex.replace_all(str, pattern, rep)  # all matches
core.regex.split(str, pattern)             # → arr
```

### `core.format`

```xf
core.format.format(template, ...)          # named/positional placeholders
core.format.pad_left(s, width)
core.format.pad_right(s, width)
core.format.pad_center(s, width)
core.format.pad_left(s, width, fill)
core.format.truncate(s, max_len)
core.format.truncate(s, max_len, ellipsis)
core.format.wrap(s, width)                 # → arr of lines
core.format.indent(s, n)
core.format.indent(s, n, char)
core.format.dedent(s)
core.format.comma(n)                       # 1,234,567
core.format.comma(n, decimals)
core.format.fixed(n, decimals)             # 3.14
core.format.sci(n)                         # 1.23e+04
core.format.hex(n)                         # 0xff
core.format.bin(n)                         # 0b1010
core.format.percent(n, decimals)           # 42.5%
core.format.duration(seconds)             # "1h 2m 3s"
core.format.bytes(n)                       # "1.50 KB"
core.format.json(value)                    # → str
core.format.from_json(str)                 # → xf value
core.format.csv_row(arr)
core.format.csv_row(arr, sep)
core.format.tsv_row(arr)
core.format.table(arr_of_maps)
core.format.table(arr_of_maps, col_order)
```

### `core.os`

```xf
core.os.run(cmd)        # execute shell command, return stdout as str
core.os.lines(path)     # read file → arr of lines
core.os.env(name)       # read environment variable
core.os.time()          # unix timestamp as num
```

### `core.ds`

```xf
core.ds.column(ds, col)               # extract column as arr
core.ds.row(ds, idx)                  # get row by index → map
core.ds.filter(ds, col, val)          # filter rows → arr
core.ds.sort(ds, col)                 # sort ascending → arr
core.ds.sort(ds, col, "desc")         # sort descending → arr
core.ds.agg(ds, group_col, val_col)   # group-by aggregate → map
core.ds.index(ds, col)                # build index map col→row
core.ds.keys(ds)                      # column names → arr
core.ds.transpose(ds)                 # rows↔cols
core.ds.merge(ds1, ds2)               # join on shared key
core.ds.merge(ds1, ds2, key)          # join on explicit key
core.ds.flatten(arr_of_arr)           # flatten nested arrays
```

---

## Built-in Functions

These are always available without a module prefix:

```xf
# Collections
push(coll, val)         # append to arr / add to set
pop(arr)                # remove and return last element
shift(arr)              # remove and return first element
unshift(arr, val)       # prepend to arr
remove(coll, key)       # remove by index (arr) or key (map/set)
has(coll, key)          # → 1 if key exists
keys(map)               # → arr of string keys
values(map)             # → arr of values

# I/O
read(path_or_cmd)       # read file or pipe → str
lines(path_or_cmd)      # read file or pipe → arr of lines
write(path, str)        # write str to file
append(path, str)       # append str to file
close(path)             # flush and close cached file handle
flush()                 # flush all open output handles
flush(path)             # flush one handle

# System
system(cmd)             # run shell command, return exit code
exit(code)              # exit with code

# Threading
join(handle)            # block on thread handle, return result
```

---

## Small Examples

### 1 — FizzBuzz

```xf
BEGIN {
    num i = 1
    while (i <= 30) {
        if (i % 15 == 0) { print "FizzBuzz" }
        elif (i % 3 == 0) { print "Fizz" }
        elif (i % 5 == 0) { print "Buzz" }
        else { print i }
        i += 1
    }
}
```

### 2 — Fibonacci

```xf
num fn fib(num n) {
    if (n <= 1) { return n }
    return fib(n - 1) + fib(n - 2)
}

BEGIN {
    num i = 0
    while (i <= 15) {
        print i, fib(i)
        i += 1
    }
}
```

### 3 — Factorial

```xf
num fn factorial(num n) {
    if (n <= 1) { return 1 }
    return n * factorial(n - 1)
}

BEGIN {
    print factorial(0)    # 1
    print factorial(5)    # 120
    print factorial(10)   # 3628800
}
```

### 4 — Reverse a String

```xf
str fn reverse_str(str s) {
    str out = ""
    num i = s.len - 1
    while (i >= 0) {
        out ..= core.str.substr(s, i, 1)
        i -= 1
    }
    return out
}

BEGIN {
    print reverse_str("hello")     # olleh
    print reverse_str("racecar")   # racecar
}
```

### 5 — Count Words in a Stream

```xf
# Run: xf -f wordcount.xf file.txt
num words = 0
{ words += NF }
END { print "words:", words }
```

### 6 — State and Error Recovery

```xf
BEGIN {
    num a = 1 / 0           # ERR
    print a.state           # 1

    num b = a + 10          # ERR propagates
    print b.state           # 1

    num c = b ?? 99         # recover
    print c                 # 99
    print c.state           # 0 (OK)
}
```

### 7 — Map Word Frequency

```xf
map freq = {}

{
    for (w in core.str.split($0, " ")) {
        if (!has(freq, w)) { freq[w] = 0 }
        freq[w] += 1
    }
}

END {
    freq[word, count] > print word, count
}
```

### 8 — Sorting an Array

```xf
BEGIN {
    arr nums = [5, 2, 8, 1, 9, 3]

    # Bubble sort
    num n = nums.len
    num i = 0
    while (i < n - 1) {
        num j = 0
        while (j < n - i - 1) {
            if (nums[j] > nums[j + 1]) {
                num tmp = nums[j]
                nums[j] = nums[j + 1]
                nums[j + 1] = tmp
            }
            j += 1
        }
        i += 1
    }
    nums[x] > print x
}
```

### 9 — Regex Extraction

```xf
BEGIN {
    str text = "Call us at 555-1234 or 800-9876"
    arr matches = core.regex.search(text, "[0-9]{3}-[0-9]{4}")
    matches[m] > print m["match"]
    # 555-1234
    # 800-9876
}
```

### 10 — JSON Round-Trip

```xf
BEGIN {
    map data = {"name": "alice", "scores": [90, 85, 92]}
    str json = core.format.json(data)
    print json
    # {"name":"alice","scores":[90,85,92]}

    map back = core.format.from_json(json)
    print back["name"]        # alice
    print back["scores"][0]   # 90
}
```

### 11 — String Padding / Table

```xf
BEGIN {
    arr rows = [
        {"name": "Alice",   "score": "95"},
        {"name": "Bob",     "score": "82"},
        {"name": "Charlie", "score": "78"}
    ]
    print core.format.table(rows)
}
```

### 12 — Tuple Destructuring

```xf
tuple fn min_max(arr a) {
    num lo = a[0]
    num hi = a[0]
    a[x] > {
        if (x < lo) { lo = x }
        if (x > hi) { hi = x }
    }
    return (lo, hi)
}

BEGIN {
    num lo
    num hi
    lo, hi = min_max([3, 1, 4, 1, 5, 9, 2, 6])
    print "min:", lo    # 1
    print "max:", hi    # 9
}
```

### 13 — Import a Library

```xf
# mathlib.xf
num fn clamp(num val, num lo, num hi) {
    if (val < lo) { return lo }
    if (val > hi) { return hi }
    return val
}
num PI = 3.14159265358979
```

```xf
# main.xf
import "mathlib.xf"

BEGIN {
    print clamp(150, 0, 100)     # 100
    print PI * 5 * 5             # 78.539...
}
```

### 14 — Walrus Operator

```xf
str fn find_first(arr items, str prefix) {
    items[x] > {
        if ((m := core.regex.match(x, "^" .. prefix)) != NAV) {
            return m["match"]
        }
    }
    return ""
}

BEGIN {
    arr words = ["apple", "banana", "avocado", "cherry"]
    print find_first(words, "a")    # apple
}
```

### 15 — Parallel Sum

```xf
num fn sum_range(num from, num to) {
    num s = 0
    num i = from
    while (i <= to) { s += i; i += 1 }
    return s
}

BEGIN {
    num t1 = spawn sum_range(1, 500000)
    num t2 = spawn sum_range(500001, 1000000)
    num r1 = join(t1)
    num r2 = join(t2)
    print r1 + r2    # 500000500000
}
```

### 16 — Null Coalescing Chain

```xf
BEGIN {
    num a               # UNDEF
    num b = 1 / 0       # ERR
    num c = 42

    print (a ?? b ?? c)    # 42
    print (a ?? 0)         # 0
    print (b ?? -1)        # -1
}
```

### 17 — CSV Header Processing

```xf
FS = ","

arr headers = []

NR == 1 {
    headers = core.str.split($0, ",")
    next
}

NR > 1 {
    map row = {}
    num i = 0
    while (i < headers.len) {
        row[headers[i]] = $(i + 1)
        i += 1
    }
    print core.format.json(row)
}
```

### 18 — Set Operations

```xf
BEGIN {
    set a = {"apple", "banana", "cherry"}
    set b = {"banana", "cherry", "date"}

    # Intersection
    set inter = {}
    a[fruit] > {
        if (has(b, fruit)) { push(inter, fruit) }
    }
    inter[x] > print "both:", x

    # Difference
    set diff = {}
    a[fruit] > {
        if (!has(b, fruit)) { push(diff, fruit) }
    }
    diff[x] > print "only a:", x
}
```

### 19 — Running Statistics

```xf
FS = ","
num n = 0
num sum = 0
num sumsq = 0

NR > 1 {
    num val = num($2)
    if (val.state == 0) {
        n += 1
        sum += val
        sumsq += val * val
    }
}

END {
    num avg = sum / n
    num variance = (sumsq / n) - (avg * avg)
    num stddev = core.math.sqrt(variance)
    print "n:", n
    print "mean:", core.format.fixed(avg, 4)
    print "stddev:", core.format.fixed(stddev, 4)
}
```

### 20 — Pattern-Action with Multiple Rules

```xf
FS = ","

num errors = 0
num warnings = 0
num ok_count = 0

$3 ~ /ERROR/   { errors += 1;   print "ERR:", $2 }
$3 ~ /WARN/    { warnings += 1; print "WRN:", $2 }
$3 ~ /OK/      { ok_count += 1 }

END {
    print "Errors:", errors
    print "Warnings:", warnings
    print "OK:", ok_count
}
```

---

## Algorithms

### Binary Search

```xf
num fn binary_search(arr a, num target) {
    num lo = 0
    num hi = a.len - 1
    while (lo <= hi) {
        num mid = core.math.int((lo + hi) / 2)
        if (a[mid] == target) { return mid }
        elif (a[mid] < target) { lo = mid + 1 }
        else { hi = mid - 1 }
    }
    return -1
}

BEGIN {
    arr sorted = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19]
    print binary_search(sorted, 7)     # 3
    print binary_search(sorted, 10)    # -1
    print binary_search(sorted, 1)     # 0
    print binary_search(sorted, 19)    # 9
}
```

### Quicksort

```xf
arr fn quicksort(arr a) {
    if (a.len <= 1) { return a }
    num pivot = a[0]
    arr less = []
    arr greater = []
    num i = 1
    while (i < a.len) {
        if (a[i] <= pivot) { push(less, a[i]) }
        else               { push(greater, a[i]) }
        i += 1
    }
    arr sorted_less = quicksort(less)
    arr sorted_greater = quicksort(greater)
    arr result = []
    sorted_less[x]    > push(result, x)
    push(result, pivot)
    sorted_greater[x] > push(result, x)
    return result
}

BEGIN {
    arr data = [3, 6, 8, 10, 1, 2, 1]
    arr sorted = quicksort(data)
    sorted[x] > print x    # 1 1 2 3 6 8 10
}
```

### Merge Sort

```xf
arr fn merge(arr left, arr right) {
    arr result = []
    num i = 0
    num j = 0
    while (i < left.len && j < right.len) {
        if (left[i] <= right[j]) {
            push(result, left[i]); i += 1
        } else {
            push(result, right[j]); j += 1
        }
    }
    while (i < left.len)  { push(result, left[i]);  i += 1 }
    while (j < right.len) { push(result, right[j]); j += 1 }
    return result
}

arr fn mergesort(arr a) {
    if (a.len <= 1) { return a }
    num mid = core.math.int(a.len / 2)
    arr left = []
    arr right = []
    num i = 0
    while (i < mid)      { push(left,  a[i]); i += 1 }
    while (i < a.len)    { push(right, a[i]); i += 1 }
    return merge(mergesort(left), mergesort(right))
}

BEGIN {
    arr sorted = mergesort([5, 3, 8, 1, 9, 2, 7])
    sorted[x] > print x    # 1 2 3 5 7 8 9
}
```

### Sieve of Eratosthenes

```xf
arr fn sieve(num limit) {
    arr is_prime = []
    num i = 0
    while (i <= limit) { push(is_prime, 1); i += 1 }
    is_prime[0] = 0
    is_prime[1] = 0
    num p = 2
    while (p * p <= limit) {
        if (is_prime[p]) {
            num mult = p * p
            while (mult <= limit) {
                is_prime[mult] = 0
                mult += p
            }
        }
        p += 1
    }
    arr primes = []
    i = 2
    while (i <= limit) {
        if (is_prime[i]) { push(primes, i) }
        i += 1
    }
    return primes
}

BEGIN {
    arr primes = sieve(50)
    primes[p] > print p
    # 2 3 5 7 11 13 17 19 23 29 31 37 41 43 47
}
```

### GCD and LCM

```xf
num fn gcd(num a, num b) {
    while (b != 0) {
        num t = b
        b = a % b
        a = t
    }
    return a
}

num fn lcm(num a, num b) {
    return (a / gcd(a, b)) * b
}

BEGIN {
    print gcd(48, 18)       # 6
    print gcd(100, 75)      # 25
    print lcm(4, 6)         # 12
    print lcm(12, 18)       # 36
}
```

### Tower of Hanoi

```xf
void fn hanoi(num n, str from, str to, str aux) {
    if (n == 0) { return }
    hanoi(n - 1, from, aux, to)
    print "Move disk", n, "from", from, "to", to
    hanoi(n - 1, aux, to, from)
}

BEGIN {
    hanoi(3, "A", "C", "B")
}
```

### Matrix Multiplication

```xf
# Represent matrix as arr-of-arr
arr fn mat_mul(arr A, arr B, num n) {
    arr C = []
    num i = 0
    while (i < n) {
        arr row = []
        num j = 0
        while (j < n) { push(row, 0); j += 1 }
        push(C, row)
        i += 1
    }
    i = 0
    while (i < n) {
        num j = 0
        while (j < n) {
            num k = 0
            while (k < n) {
                C[i][j] += A[i][k] * B[k][j]
                k += 1
            }
            j += 1
        }
        i += 1
    }
    return C
}

BEGIN {
    arr A = [[1, 2], [3, 4]]
    arr B = [[5, 6], [7, 8]]
    arr C = mat_mul(A, B, 2)
    print C[0][0], C[0][1]    # 19 22
    print C[1][0], C[1][1]    # 43 50
}
```

---

## Larger Examples

### 1 — CSV Analytics Pipeline

Compute per-group statistics from a CSV file.

```xf
# analytics.xf
# Input: CSV with columns id,name,department,salary
# Usage: xf -f analytics.xf employees.csv

FS = ","

map dept_total = {}
map dept_count = {}
num grand_total = 0
num grand_n = 0

NR == 1 { next }   # skip header

NR > 1 {
    str dept = $3
    num salary = num($4)
    if (salary.state != 0) { next }

    if (!has(dept_total, dept)) {
        dept_total[dept] = 0
        dept_count[dept] = 0
    }

    dept_total[dept] += salary
    dept_count[dept] += 1
    grand_total += salary
    grand_n += 1
}

END {
    print "Department Averages"
    print "-------------------"
    dept_total[dept, total] > {
        num avg = total / dept_count[dept]
        print dept, core.format.fixed(avg, 2)
    }
    print ""
    print "Company Average:", core.format.fixed(grand_total / grand_n, 2)
}
```

---

### 2 — Parallel Word Count

Count word frequencies across a large file using threads.

```xf
# parallel_wc.xf
BEGIN {
    str path = ARGV[1] ?? "input.txt"
    arr file_lines = lines(path)
    num n = file_lines.len
    num chunk = core.math.int(n / 4)

    num fn count_chunk(arr chunk_lines) {
        map freq = {}
        chunk_lines[line] > {
            arr words = core.str.split(line, " ")
            words[w] > {
                str word = core.str.lower(core.str.trim(w))
                if (word.len > 0) {
                    if (!has(freq, word)) { freq[word] = 0 }
                    freq[word] += 1
                }
            }
        }
        return core.format.json(freq)
    }

    # Split file into 4 chunks and count in parallel
    arr t = []
    num i = 0
    while (i < 4) {
        num from = i * chunk
        num to   = (i == 3) ? n : from + chunk
        arr slice = []
        num j = from
        while (j < to) { push(slice, file_lines[j]); j += 1 }
        push(t, spawn count_chunk(slice))
        i += 1
    }

    # Merge results
    map total = {}
    t[tid] > {
        map partial = core.format.from_json(join(tid))
        partial[word, count] > {
            if (!has(total, word)) { total[word] = 0 }
            total[word] += count
        }
    }

    # Print top 20 words
    arr pairs = []
    total[w, c] > push(pairs, (c, w))
    arr sorted = core.ds.sort(pairs, 0, "desc")
    num k = 0
    while (k < 20 && k < sorted.len) {
        print sorted[k][1], sorted[k][0]
        k += 1
    }
}
```

---

### 3 — Log Analyzer

Parse nginx/apache access logs, report error rates and top paths.

```xf
# log_analyzer.xf
# Input: Combined log format
# 127.0.0.1 - frank [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.0" 200 2326

map status_count = {}
map path_count = {}
num total = 0

{
    # Extract status code and path from log line
    map m = core.regex.match($0, "\"(GET|POST|PUT|DELETE) ([^ ]+) HTTP/[0-9.]+ ([0-9]{3})")
    if (m.state != 0) { next }

    arr groups = m["groups"]
    str method = groups[0]
    str path   = groups[1]
    str status = groups[2]

    total += 1
    if (!has(status_count, status)) { status_count[status] = 0 }
    status_count[status] += 1

    if (!has(path_count, path)) { path_count[path] = 0 }
    path_count[path] += 1
}

END {
    print "Total requests:", total
    print ""
    print "Status codes:"
    status_count[code, n] > {
        print " ", code, n, core.format.percent(n / total, 1)
    }

    print ""
    print "Top 10 paths:"
    arr by_hits = []
    path_count[p, c] > push(by_hits, {"path": p, "hits": c})
    arr sorted = core.ds.sort(by_hits, "hits", "desc")
    num i = 0
    while (i < 10 && i < sorted.len) {
        print " ", sorted[i]["hits"], sorted[i]["path"]
        i += 1
    }
}
```

---

### 4 — Simple Key-Value Store

An in-memory KV store exposed through a REPL-like interface.

```xf
# kv_store.xf
BEGIN {
    map store = {}
    map ttl   = {}
    num now   = core.os.time()

    void fn kv_set(str key, str val) {
        store[key] = val
        print "OK"
    }

    str fn kv_get(str key) {
        if (!has(store, key)) { return "(nil)" }
        return store[key]
    }

    void fn kv_del(str key) {
        if (has(store, key)) {
            delete store[key]
            print "1"
        } else {
            print "0"
        }
    }

    void fn kv_keys() {
        store[k, v] > print k
    }

    arr cmds = lines("commands.txt")
    cmds[line] > {
        arr parts = core.str.split(core.str.trim(line), " ")
        str cmd = parts[0]
        if (cmd == "SET") {
            kv_set(parts[1], parts[2])
        } elif (cmd == "GET") {
            print kv_get(parts[1])
        } elif (cmd == "DEL") {
            kv_del(parts[1])
        } elif (cmd == "KEYS") {
            kv_keys()
        }
    }
}
```

---

### 5 — Parallel Statistics on CSV

Compute mean and standard deviation in parallel across a large CSV.

```xf
# parallel_stats.xf
FS = ","

str fn compute_stats(arr rows) {
    num n = 0
    num sum = 0
    num sumsq = 0
    rows[row] > {
        num val = num(row[2])
        if (val.state == 0) {
            n += 1
            sum += val
            sumsq += val * val
        }
    }
    if (n == 0) { return core.format.json({"n": 0}) }
    num avg = sum / n
    num variance = (sumsq / n) - (avg * avg)
    num stddev = core.math.sqrt(variance)
    return core.format.json({
        "n":      n,
        "sum":    sum,
        "sumsq":  sumsq,
        "mean":   avg,
        "stddev": stddev
    })
}

arr all_rows = []

NR > 1 {
    push(all_rows, [$1, $2, $3])
}

END {
    num chunk_size = core.math.int(all_rows.len / 4)
    arr tids = []
    num i = 0
    while (i < 4) {
        arr chunk = []
        num from = i * chunk_size
        num to   = i == 3 ? all_rows.len : from + chunk_size
        num j = from
        while (j < to) { push(chunk, all_rows[j]); j += 1 }
        num tid = spawn compute_stats(chunk)
        push(tids, tid)
        i += 1
    }

    num total_n = 0
    num total_sum = 0
    num total_sumsq = 0

    tids[tid] > {
        map r = core.format.from_json(join(tid))
        if (r["n"] > 0) {
            total_n     += r["n"]
            total_sum   += r["sum"]
            total_sumsq += r["sumsq"]
        }
    }

    num mean = total_sum / total_n
    num variance = (total_sumsq / total_n) - (mean * mean)
    num stddev = core.math.sqrt(variance)

    print "n:     ", total_n
    print "mean:  ", core.format.fixed(mean, 6)
    print "stddev:", core.format.fixed(stddev, 6)
}
```

---

### 6 — Sentiment Analyzer (Stream + Aggregation)

Classify rows from a dataset by rating and compute per-group metrics — the same workload used in the IMDB validation run.

```xf
# sentiment.xf
# Input CSV: id,text,label,rating
FS  = ","
OFS = "|"

map label_count = {}
map label_score = {}
num total = 0

NR == 1 { next }    # header

NR > 1 {
    str label  = $3
    num rating = num($4)

    if (rating.state != 0) { next }

    total += 1
    if (!has(label_count, label)) {
        label_count[label] = 0
        label_score[label] = 0
    }
    label_count[label] += 1
    label_score[label] += rating
}

END {
    print "label", "count", "avg_rating"
    label_count[lbl, cnt] > {
        num avg = label_score[lbl] / cnt
        print lbl, cnt, core.format.fixed(avg, 4)
    }
    print "total records:", total
}
```

---

### 7 — Data Transformation Pipeline

Read JSON, transform records, output CSV.

```xf
# transform.xf
BEGIN {
    str raw = read("data.json")
    arr records = core.format.from_json(raw)

    # Normalize and tag each record
    arr out = []
    records[rec] > {
        str name  = core.str.upper(core.str.trim(rec["name"] ?? ""))
        num score = num(rec["score"] ?? 0)
        str grade = score >= 90 ? "A"
                  : score >= 80 ? "B"
                  : score >= 70 ? "C"
                  : "F"
        push(out, {"name": name, "score": score, "grade": grade})
    }

    # Sort by score descending
    arr sorted = core.ds.sort(out, "score", "desc")

    # Print as CSV
    print "name,score,grade"
    sorted[row] > {
        print core.format.csv_row([row["name"], str(row["score"]), row["grade"]])
    }
}
```

---

## Embedding XF in C

XF can be embedded via `libxf`:

```c
#include "xf.h"

xf_State *xf = xf_newstate();
xf_set_format(xf, XF_FMT_CSV, XF_FMT_TEXT);
xf_set_max_jobs(xf, 8);

xf_load_string(xf, "BEGIN { print \"hello\" }");
xf_feed_file(xf, stdin);
xf_run_end(xf);

if (xf_had_error(xf)) {
    fprintf(stderr, "Error: %s\n", xf_last_error(xf));
}

xf_close(xf);
```

Key API functions:

| Function | Description |
|----------|-------------|
| `xf_newstate()` | Allocate a new XF interpreter state |
| `xf_close(xf)` | Free all resources |
| `xf_load_string(xf, src)` | Load script source |
| `xf_feed_line(xf, line, len)` | Feed one record |
| `xf_feed_file(xf, fp)` | Feed from a FILE* |
| `xf_run_end(xf)` | Trigger END block |
| `xf_set_format(xf, in, out)` | Set input/output format |
| `xf_set_max_jobs(xf, n)` | Set thread pool size |
| `xf_had_error(xf)` | Check for errors |
| `xf_last_error(xf)` | Get last error message |
| `xf_clear_error(xf)` | Clear error state |