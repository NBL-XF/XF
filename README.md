# XF

A practical, AWK-inspired scripting language for text processing, structured data transformation, regex workflows, dataset manipulation, and concurrent chunk processing. XF compiles to a bytecode VM and ships with a REPL, a multithreaded record dispatcher, SIMD-accelerated field splitting, and a rich standard library.

---

## Table of Contents

1. [Why XF](#why-xf)
2. [Building](#building)
3. [CLI Usage](#cli-usage)
4. [REPL](#repl)
5. [Language Basics](#language-basics)
6. [Types](#types)
7. [Runtime States](#runtime-states)
8. [Variables](#variables)
9. [Functions](#functions)
10. [Expressions and Operators](#expressions-and-operators)
11. [Collections](#collections)
12. [Shorthand Syntax](#shorthand-syntax)
13. [AWK-style Record Processing](#awk-style-record-processing)
14. [Concurrency — spawn / join](#concurrency--spawn--join)
15. [Standard Library](#standard-library)
    - [core.math](#coremath)
    - [core.str](#corestr)
    - [core.regex](#coreregex)
    - [core.os](#coreos)
    - [core.generics](#coregenerics)
    - [core.format](#coreformat)
    - [core.ds](#coreds)
    - [core.process](#coreprocess)
    - [core.edit](#coreedit)
    - [core.img](#coreimg)
    - [core.lambda](#corelambda)
16. [Common Algorithms](#common-algorithms)
17. [Worked Examples](#worked-examples)
18. [Tips](#tips)
19. [Troubleshooting](#troubleshooting)
20. [Known Limitations](#known-limitations)

---

## Why XF

XF is built for scripts where structure, data flow, and observability matter.

It works especially well for:

- CSV and line-based record processing
- regex-heavy transformation pipelines
- turning raw rows into structured maps
- collection filtering and mapping
- grouping, sorting, and flattening datasets
- splitting work into parallel chunks
- experimenting in a REPL before committing to a full script
- running LambdaScript (untyped lambda calculus) expressions from within a data pipeline

XF is concise and explicit. Runtime states make failures visible without exceptions.

---

## Building

```bash
# release build (default)
make

# debug build with ASAN/UBSAN
make MODE=debug

# thread-sanitizer build
make MODE=thread

# install to /usr/local
make install

# uninstall
make uninstall

# clean
make clean
```

The build uses Clang (defaulting to the Homebrew LLVM at `/opt/homebrew/opt/llvm/bin/clang`). Override with `CC=`.

The LambdaScript interpreter is vendored under `vendor/lambdaScript` and compiled into a static library (`libls.a`) that XF links automatically. To build without it, use `core_WithoutLS.c` in place of `core.c` — it omits `core.lambda`.

The runtime is also installed as a static library (`libxf.a`) with public headers under `include/xf/`, so XF can be embedded in other C projects.

### Dependencies

- Clang / LLVM (or any C11 compiler)
- POSIX threads (`-lpthread`)
- GNU readline (`-lreadline`)
- libm (`-lm`)

---

## CLI Usage

```text
xf                          # start the REPL
xf -r <file.xf>             # run a script file
xf -e "code"                # execute inline source
xf -j <N> -r <file.xf>     # run with N worker threads
xf -j <N> -e "code"        # inline with N worker threads
xf -v / --version           # print version
```

Worker count is capped at the number of hardware threads detected via `sysconf(_SC_NPROCESSORS_ONLN)`.

---

## REPL

```bash
xf
```

The REPL supports readline with persistent history.

```
>> 1 + 2
=> 3  [num, OK]

>> arr xs = [1,2,3]
>> xs[1]
=> 2  [num, OK]

>> 10 |> print
10
=> 10  [num, OK]
```

**REPL commands:**

| Command  | Effect                      |
|----------|-----------------------------|
| `:quit`  | exit                        |
| `:q`     | exit (shorthand)            |
| `:stack` | dump the VM value stack     |

---

## Language Basics

### BEGIN / END

Scripts can have optional `BEGIN` and `END` blocks that run before and after record processing.

```xf
BEGIN {
    print "start"
}

END {
    print "end"
}
```

### Pattern / action rules

XF is AWK-inspired. Rules match each stdin record and fire their body.

```xf
/error/ {
    print $0
}

NR > 5 {
    exit
}
```

### Top-level declarations

Functions and variables can be declared at top level, outside any block.

```xf
num fn add(num a, num b) {
    return a + b
}
```

### Comments

```xf
# this is a comment
```

---

## Types

| Keyword | Description                        |
|---------|------------------------------------|
| `num`   | 64-bit IEEE 754 double             |
| `str`   | immutable reference-counted string |
| `bool`  | boolean (`true` / `false`)         |
| `arr`   | dynamic array                      |
| `map`   | insertion-ordered string-keyed map |
| `set`   | unordered set                      |
| `tuple` | fixed-length heterogeneous value   |
| `fn`    | first-class function               |
| `regex` | compiled regex literal             |
| `void`  | no return value                    |

### Literals

```xf
num n      = 42
str s      = "hello"
bool ok    = true
arr xs     = [1, 2, 3]
map row    = {"name": "alice", "score": 90}
set unique = {1, 2, 3}
tuple t    = (1, "a", true)
```

---

## Runtime States

XF separates **type** from **state**. Every value carries both.

| State   | Meaning                                           |
|---------|---------------------------------------------------|
| `OK`    | normal success value                              |
| `ERR`   | error value (carries an error object)             |
| `NAV`   | not a valid / navigable result                    |
| `NULL`  | explicit null                                     |
| `VOID`  | no value (void return)                            |
| `UNDEF` | declared but not yet assigned                     |
| `UNDET` | referenced before declaration                     |
| `TRUE`  | boolean true (carried in state field)             |
| `FALSE` | boolean false (carried in state field)            |

States propagate through operations: if an input is `NAV` or `ERR`, most operations pass it through rather than crash.

```xf
>> a
=> undet  [bool, UNDET]    # undeclared

>> num a
>> a
=>   [num, UNDEF]          # declared, not initialized
```

Check a value's state at runtime:

```xf
if (result.state == "NAV") {
    print "operation failed"
}
```

---

## Variables

```xf
num total = 10
str name  = "alice"
arr xs    = [1, 2, 3]
map row   = {"id": 1}

num total   # declared but UNDEF until assigned
```

### Assignment

```xf
x  = x + 1
x += 5
x -= 2
x *= 3
x /= 4
x %= 2
```

### Index assignment

```xf
arr xs = [1, 2, 3]
xs[1] = 20

map row = {"name": "alice"}
row["name"] = "bob"
```

### Walrus operator

```xf
if (n := core.os.time()) {
    print n
}
```

---

## Functions

### Named functions

```xf
num fn add1(num x) {
    return x + 1
}

bool fn is_even(num x) {
    return x % 2 == 0
}

map fn mark_row(map row) {
    row["checked"] = 1
    return row
}
```

### Anonymous functions

```xf
fn(num x) {
    return x + 1
}
```

Anonymous functions are the standard way to pass callbacks into collection operators and module functions.

### Closures

Functions close over their enclosing scope.

```xf
num fn make_adder(num n) {
    return fn(num x) { return x + n }
}
```

### Higher-order usage

```xf
arr xs = [1, 2, 3, 4]
print(xs [/] add1)        # transform
print(xs [*] is_even)     # filter
```

---

## Expressions and Operators

### Arithmetic

```xf
1 + 2    5 - 3    4 * 2    9 / 3    5 % 2    2 ^ 3
```

### Comparison

```xf
1 < 2    2 <= 2    3 > 1    4 >= 5    1 == 1    1 != 2
```

Spaceship operator for three-way comparison:

```xf
a <=> b    # -1, 0, or 1
```

### Boolean

```xf
true && false
true || false
!false
```

### String concatenation

```xf
"hello" .. " " .. "world"
```

### Ternary

```xf
x > 5 ? "big" : "small"
```

### Null coalescing

```xf
a ?? b    # returns b if a is NAV/NULL/UNDEF/UNDET
```

### Regex match / no-match

```xf
"aaab" ~ /a.*b/       # match
"aaab" !~ /xyz/       # no-match
"AAAB" ~ /a.*b/i      # case-insensitive
```

### Pipeline

```xf
x |> f       # f(x)
f <| x       # f(x)
```

### Dot accessors

```xf
xs.len       # length of arr, str, map, set, tuple
xs.type      # type name as string
xs.state     # state name as string
```

### Range

```xf
1..5        # exclusive: 1,2,3,4
1..=5       # inclusive: 1,2,3,4,5
```

---

## Collections

### Arrays

```xf
arr xs = [1, 2, 3]
print(xs[0])
push(xs, 4)       # append
print(xs.len)
```

Built-in: `push`, `pop`, `shift`, `unshift`.

### Maps

```xf
map row = {"name": "alice", "score": 90}
print(row["name"])
row["score"] = 95
```

### Sets

```xf
set s = {1, 2, 3}
```

### Tuples

```xf
tuple t = (1, "a", true)
print(t[0])
```

### Delete

```xf
arr xs = [1, 2, 3, 4]
delete xs[1]

map row = {"a": 1, "b": 2}
delete row["a"]
```

---

## Shorthand Syntax

### Push / pop / shift / unshift

```xf
4 => a       # push 4 onto a
a <=         # pop from a
a ==>        # shift from a (remove first)
0 <== a      # unshift 0 into a
```

### Filter and transform

```xf
xs [*] pred    # filter: keep elements where pred returns true
xs [/] fn      # transform: apply fn to every element
```

```xf
bool fn is_even(num x) { return x % 2 == 0 }
num  fn square(num x)  { return x * x }

print([1,2,3,4] [*] is_even)          # [2, 4]
print([1,2,3]   [/] square)           # [1, 4, 9]
print(([1..5] [*] is_even) [/] square) # chain
```

### Merge arrays

```xf
[1,2] 3> [3,4]        # [1,2,3,4]
```

### Split string

```xf
"a,b,c" <3 ","        # ["a","b","c"]
```

### Expand tuple to array

```xf
(1,2,3) =->[]
```

### Flatten array of arrays

```xf
[[1,2],[3,4],5] []->=
```

### Regex constructor

```xf
(*) "a.*b"         # build regex from string
(*) "a.*b" "i"     # with flags
```

---

## AWK-style Record Processing

When stdin is not a terminal, XF reads records (lines by default) and runs each matching rule against them.

### Built-in variables

| Variable | Meaning                                |
|----------|----------------------------------------|
| `$0`     | full current record                    |
| `$1..$N` | individual fields (1-based)            |
| `NR`     | total record number                    |
| `NF`     | field count in current record          |
| `FNR`    | record number within current file      |
| `FS`     | field separator (default: whitespace)  |
| `RS`     | record separator (default: `\n`)       |
| `OFS`    | output field separator                 |
| `ORS`    | output record separator                |
| `OFMT`   | numeric output format (default `%.6g`) |
| `file`   | current input filename                 |
| `match`  | last regex match string                |
| `captures` | last regex capture groups (arr)    |
| `err`    | last error value                       |
| `ARGS`   | script arguments (arr of str)          |

### Pattern rules

```xf
BEGIN { FS = "," }

/error/ { print $1, $2 }

NR == 1 { print "header:", $0 }

{ print NR, $0 }

END { print "done" }
```

### Multi-threaded record dispatch

```bash
xf -j 4 -r script.xf
```

With `-j N`, XF creates a work-stealing thread pool. Each record is dispatched to the least-loaded worker via a Chase-Lev deque. Workers share globals under a readers-writer lock. GC sweeps coordinate via a stop-the-world safe-point barrier.

---

## Concurrency — spawn / join

XF has first-class language-level concurrency via `spawn` and `join`.

```xf
num task = spawn fn() {
    return core.math.sqrt(144)
}

num result = join task
print(result)    # 12
```

`spawn` submits a function to the thread pool and returns a task handle. `join` blocks until the task completes and returns its value.

---

## Standard Library

All modules live under the `core` global.

---

### core.math

```xf
core.math.sin(x)     core.math.cos(x)     core.math.tan(x)
core.math.asin(x)    core.math.acos(x)    core.math.atan(x)
core.math.atan2(y,x) core.math.sqrt(x)    core.math.ln(x)
core.math.log(x)     core.math.log2(x)    core.math.log10(x)
core.math.exp(x)     core.math.pow(x,y)
core.math.abs(x)     core.math.floor(x)   core.math.ceil(x)
core.math.round(x)   core.math.int(x)     core.math.clamp(v,lo,hi)
core.math.min(a,b)   core.math.max(a,b)
core.math.rand()     core.math.srand(seed)
```

Constants: `core.math.pi`, `core.math.e`, `core.math.INF`, `core.math.NAN`, `core.math.i` (imaginary unit).

---

### core.str

```xf
core.str.len(s)
core.str.upper(s)          core.str.lower(s)
core.str.capitalize(s)
core.str.trim(s)           core.str.ltrim(s)      core.str.rtrim(s)
core.str.substr(s, start, len)
core.str.index(s, sub)                      # first occurrence position, or -1
core.str.contains(s, sub)
core.str.starts_with(s, prefix)             core.str.ends_with(s, suffix)
core.str.replace(s, old, new)               core.str.replace_all(s, old, new)
core.str.repeat(s, n)
core.str.reverse(s)
core.str.sprintf(fmt, ...)
core.str.concat(a, b)
core.str.comp(a, b)                         # strcmp-style: -1 / 0 / 1
```

`replace` and `replace_all` accept plain strings or regex literals as the pattern.

---

### core.regex

```xf
core.regex.test(subject, pattern)           # -> num (1/0)
core.regex.match(subject, pattern)          # -> map {match, index, groups}
core.regex.search(subject, pattern)         # -> arr of match maps
core.regex.groups(subject, pattern)         # -> arr of capture strings
core.regex.split(subject, pattern)          # -> arr of parts
core.regex.replace(subject, pattern, repl)
core.regex.replace_all(subject, pattern, repl)
```

Patterns can be regex literals (`/pat/flags`) or plain strings. Flags: `i` (case-insensitive), `m` (multiline), `g` (global, where applicable), `x` (extended).

---

### core.os

**File I/O:**

```xf
core.os.read(path)             # -> str (whole file)
core.os.write(path, data)      # -> num (1 on success)
core.os.append(path, data)     # -> num
core.os.lines(path)            # -> arr of str (one per line)
```

**File handles (streaming reads):**

```xf
num h = core.os.open(path)
arr chunk = core.os.chunk(h, 100)    # read next 100 lines
num pos = core.os.tell(h)            # lines read so far
core.os.close(h)
```

**Shell:**

```xf
core.os.execute(cmd)           # -> num (exit code); alias: exec
core.os.run(cmd)               # -> str (stdout, trimmed)
core.os.run_lines(cmd)         # -> arr of str
core.os.exit(code)
```

**Environment:**

```xf
core.os.time()                 # -> num (Unix timestamp)
core.os.env("HOME")            # -> str or NAV
```

**Recursive grep:**

```xf
arr hits = core.os.grep(pattern, path)
arr hits = core.os.grep(pattern, path, "i")         # flags string
arr hits = core.os.grep(pattern, path, "", 50)       # limit results

# each hit is a map: {file, line, text, index}
```

`path` may be a file or directory (recursive on Linux via `nftw`, portable fallback elsewhere).

---

### core.generics

Cross-type utilities that work on any collection.

```xf
core.generics.size(coll)            # length of arr, map, set, str, tuple
core.generics.join(coll, sep)       # join arr elements with separator
core.generics.split(s, sep)         # split string by separator -> arr
core.generics.strip(s)              # trim whitespace
core.generics.contains(coll, val)   # membership test
```

---

### core.format

Formatting and serialization.

```xf
core.format.json(value)            # -> str (pretty JSON)
core.format.from_json(s)           # -> map / arr
core.format.csv_row(arr)           # -> str (comma-separated, quoted)
core.format.tsv_row(arr)           # -> str (tab-separated)
core.format.table(arr_of_maps)     # -> str (ASCII table)

core.format.fixed(n, decimals)     # "3.14"
core.format.sci(n, decimals)       # "3.14e+00"
core.format.hex(n)                 # "0x1f"
core.format.bin(n)                 # "0b1101"
core.format.percent(n, decimals)   # "42.50%"
core.format.comma(n)               # "1,234,567"
core.format.duration(seconds)      # "1h 2m 3s"
core.format.bytes(n)               # "1.2 MB"

core.format.pad_left(s, width)
core.format.pad_right(s, width)
core.format.pad_center(s, width)
core.format.truncate(s, width)
core.format.wrap(s, width)         # -> arr of lines
core.format.indent(s, n)
core.format.dedent(s)

core.format.format(fmt, ...)       # printf-style formatting
```

---

### core.ds

Dataset operations on `arr`-of-`map` structures.

```xf
core.ds.row(dataset, idx)                  # -> map
core.ds.column(dataset, "field")           # -> arr of cell values
core.ds.keys(dataset)                      # -> arr of column names
core.ds.values(dataset)                    # -> arr of value arrays

core.ds.sort(dataset, "field")             # ascending
core.ds.sort(dataset, "field", "desc")     # descending

core.ds.filter(dataset, fn(map row) { ... })
core.ds.agg(dataset, "field", "sum")       # sum/count/min/max/mean
core.ds.agg_parallel(dataset, "field", "sum")   # parallel version

core.ds.merge(ds1, ds2)                    # row-wise concat
core.ds.join(dataset, "field")             # string join of column values
core.ds.index(dataset, "field")            # -> map {value -> [row indices]}
core.ds.transpose(dataset)                 # -> map {col -> arr}
core.ds.expand(dataset, "field")           # unnest nested arrays
core.ds.flatten(arr_of_arr)                # flatten one level
core.ds.stream(dataset, fn)                # apply fn to each row (streaming)
```

---

### core.process

Parallel chunk processing for large in-memory datasets.

```xf
# split an array into N chunks
arr chunks = core.process.split(dataset, 4)

# apply a transform function to each row in a chunk
arr out = core.process.assign(chunk, fn(map row) {
    row["_done"] = 1
    return row
})

# build an inverted index from a chunk
map idx = core.process.index(chunk, fn(map row) { return row }, 0)

# build worker descriptors and run them in parallel threads
arr jobs = []
for (chunk in chunks) {
    push(jobs, core.process.worker(my_fn, chunk))
}
arr results = core.process.run(jobs)

# then flatten
arr combined = core.ds.flatten(results)
```

`core.process.run` spawns one pthread per native-function worker. XF-language workers execute inline on the calling thread (the VM is not re-entrant across threads).

---

### core.edit

File editing and filesystem utilities.

```xf
core.edit.read(path)                       # -> str
core.edit.write(path, content)             # -> num
core.edit.lines(path)                      # -> arr of str
core.edit.line_count(path)                 # -> num

core.edit.insert(path, line_num, text)     # insert line
core.edit.delete_lines(path, from, to)     # delete range
core.edit.replace_line(path, line_num, text)
core.edit.replace_all(path, pattern, repl) # regex replace in file

core.edit.exists(path)                     # -> num (1/0)
core.edit.stat(path)                       # -> map {size, mtime, ...}
core.edit.glob(pattern)                    # -> arr of matching paths
core.edit.find(dir, pattern)               # recursive find
core.edit.mkdir(path)                      # -> num
core.edit.rename(src, dst)                 # -> num
core.edit.unlink(path)                     # delete file -> num
core.edit.diff(path_a, path_b)             # -> arr of diff lines
```

---

### core.img

Image loading and pixel-level access via `stb_image`.

```xf
# load image as arr of RGB tuples
map img = core.img.vectorize("photo.png")
map img = core.img.vectorize("photo.png", "gray")     # grayscale (expanded to rgb)
map img = core.img.vectorize("photo.png", "rgb", 1)   # normalized 0.0–1.0

# img is: {width, height, channels, mode, normalized, data: arr<tuple<num,num,num>>}
num w = img["width"]
tuple px = img["data"][0]    # first pixel: (r, g, b)

# write back
core.img.unvectorize(img, "out.png")
```

---

### core.lambda

Evaluates LambdaScript (untyped lambda calculus) expressions from within XF scripts. The evaluator uses de Bruijn indices and a normal-order reducer.

```xf
# evaluate a lambda expression and get the reduced string
str result = core.lambda.eval("(\\x.x) z")

# evaluate from a file
str result = core.lambda.file("church.ls")

# get full result map
map r = core.lambda.run("(\\x.x) z")
# r: {ok, output, trace, steps, reached_limit, error}

map r = core.lambda.run_file("church.ls")

# trace each reduction step
map r = core.lambda.trace("S K K z")
print(r["trace"])
```

Optional arguments: `core.lambda.eval(source, max_steps, use_prelude)`.

The prelude (enabled by default) defines `I`, `K`, `S`, `TRUE`, `FALSE`, `AND`, `OR`, `NOT`, `IMP`, `IFF`.

Both `\` and Unicode `λ` are accepted as lambda binders.

---

## Common Algorithms

### Sum an array

```xf
arr xs = [1, 2, 3, 4]
num total = 0
for (x in xs) { total += x }
print(total)
```

### Filter then transform

```xf
bool fn is_even(num x) { return x % 2 == 0 }
num  fn square(num x)  { return x * x }

arr out = ([1,2,3,4,5,6] [*] is_even) [/] square
print(out)    # [4, 16, 36]
```

### Count occurrences

```xf
arr xs = ["a","b","a","c","a","b"]
map counts = {}
for (k in xs) {
    if (counts[k].state == "OK") {
        counts[k] += 1
    } else {
        counts[k] = 1
    }
}
print(core.format.json(counts))
```

### Fibonacci

```xf
num fn fib(num n) {
    if (n <= 1) { return n }
    num a = 0
    num b = 1
    num i = 2
    while (i <= n) {
        num next = a + b
        a = b
        b = next
        i += 1
    }
    return b
}

BEGIN {
    arr seq = []
    num i = 0
    while (i < 10) { push(seq, fib(i)); i += 1 }
    print(seq)
}
```

### Quicksort

```xf
arr fn quick_sort(arr xs) {
    if (xs.len <= 1) { return xs }
    num pivot = xs[0]
    arr less = [], equal = [], greater = []
    for (v in xs) {
        if (v < pivot)      { push(less, v) }
        elif (v > pivot)    { push(greater, v) }
        else                { push(equal, v) }
    }
    return quick_sort(less) 3> equal 3> quick_sort(greater)
}

BEGIN { print(quick_sort([5,1,4,2,8,5,3])) }
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

BEGIN { hanoi(3, "A", "B", "C") }
```

---

## Worked Examples

### CSV reader

```xf
BEGIN {
    arr lines = core.os.lines("people.csv")
    arr headers = lines[0] <3 ","
    arr rows = []
    num i = 1
    while (i < lines.len) {
        arr fields = lines[i] <3 ","
        map row = {}
        num j = 0
        while (j < headers.len) {
            row[headers[j]] = fields[j]
            j += 1
        }
        push(rows, row)
        i += 1
    }
    print("rows: " .. rows.len)
    print(core.format.json(core.ds.row(rows, 0)))
}
```

### Parallel row marking

```xf
BEGIN {
    arr dataset = [
        {"name": "alice", "score": 90},
        {"name": "bob",   "score": 70},
        {"name": "carol", "score": 85}
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
    arr combined = core.ds.flatten(core.process.run(jobs))
    print(core.format.json(combined))
}
```

### Regex log scan

```xf
BEGIN {
    arr lines = core.os.lines("app.log")
    arr errors = lines [*] fn(str line) { return line ~ /ERROR/ }
    print("errors found: " .. errors.len)
    for (line in errors) { print line }
}
```

### Group-by aggregation

```xf
BEGIN {
    arr rows = [
        {"dept": "eng", "score": 90},
        {"dept": "eng", "score": 70},
        {"dept": "hr",  "score": 85}
    ]
    map grouped = {}
    for (row in rows) {
        str dept = row["dept"]
        if (grouped[dept].state != "OK") { grouped[dept] = [] }
        push(grouped[dept], row)
    }
    print(core.format.json(grouped))
}
```

### AWK-style field processing

```xf
BEGIN { FS = ":" }

NR > 1 {
    print $1, "->", $3
}

END { print NR - 1, "records processed" }
```

Run: `xf -r script.xf < /etc/passwd`

### Lambda calculus from XF

```xf
BEGIN {
    # Church numeral 2 applied twice
    str result = core.lambda.eval("TWO = \\f.\\x.f(f x)\nTWO")
    print(result)

    # Step-by-step trace
    map r = core.lambda.trace("S K K z", 20)
    print(r["trace"])
    print("steps: " .. r["steps"])
}
```

---

## Tips

**Use `arr`-of-`map` for structured data.** It works naturally with `core.ds`, `core.process.assign`, and `core.ds.flatten`.

**Inspect shapes early.** `core.format.json(core.ds.row(dataset, 0))` saves a lot of debugging.

**Use the REPL first.** Test regexes, callbacks, and collection pipelines interactively before putting them in a script.

**Keep worker functions simple.** The best worker output shapes are `arr` of `map`, `arr` of scalars, or chunks that can be flattened. Avoid returning deeply nested structures.

**Check `.state` when an operation might fail.** `core.os.read` returns `NAV` on failure; `?.state == "OK"` guards that cheaply.

**`core.generics.size` is the universal size function.** It works on `arr`, `map`, `set`, `str`, and `tuple`.

---

## Troubleshooting

**`attempt to call non-function`** — the callee is not a function. Either the module member name is wrong or a value that isn't callable was used. Verify in the REPL:

```xf
core.ds.flatten |> print
```

**`NAV` result** — an operation could not produce a valid result. Common causes: file not found, bad input shape, out-of-range index, regex compile failure. Check the specific call's return state before using the result.

**`UNDET`** — you referenced a name before it was declared.

**`UNDEF`** — you declared a variable but never assigned it.

**Regex not matching** — test both forms to isolate whether it's an operator-path or pattern issue:

```xf
print("AAAB" ~ /a.*b/i)
print(core.regex.test("AAAB", /a.*b/i))
```

**Worker function not running in parallel** — `core.process.run` only creates pthreads for native functions. XF-language functions run inline. Use `-j N` with `spawn`/`join` for language-level parallelism, or `core.process.run` for native-only parallel chunks.