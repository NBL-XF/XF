# XF Version Roadmap

# v0.9.11 — Runtime Correctness Pass

Goal: **eliminate correctness bugs before v1**

### Ownership / reference audit

Review hot paths:

* `interp_eval_expr`
* `interp_eval_stmt`
* loop bindings
* tuple construction
* map/set operations
* builtin coercions
* function call argument binding
* return values

Focus on:

* missing `xf_value_release`
* double release
* borrowed vs owned confusion

### Error propagation cleanup

Ensure:

* errors don't silently convert to NAV
* partial mutations don't survive failed operations
* destructuring failures are atomic

### Builtin contract audit

Normalize behavior for:

```
push
pop
shift
unshift
remove
keys
values
has
```

Decide:

* mutation vs returned copy
* NAV vs ERR vs NULL

### Exit criteria

* interpreter stable under normal usage
* no obvious memory hazards

---

# v0.9.12 — Concurrency Scope Decision

Goal: **define what concurrency means in v1**

### Architectural limitation

XF functions cannot safely run in pthreads without interpreter cloning.

### Options

#### Option A (recommended for v1)

Limit concurrency support.

* `spawn/join` supported
* `core.process.run` accepts **native functions only**
* XF-language functions return `NAV` (documented)

#### Option B

Implement interpreter-per-thread execution.

Requires:

* thread-local `Interp`
* shared VM
* copied global scope

Large refactor.

### Also in this pass

* spawn/join handle validation
* thread result ownership
* join lifecycle cleanup

### Exit criteria

Concurrency model explicitly defined.

---

# v0.9.13 — API Consistency Audit

Goal: **make the standard library feel coherent**

Review all core modules:

```
core.str
core.ds
core.process
core.math
```

Check:

* naming conventions
* argument order
* return types
* mutation semantics
* collection shapes

Examples:

```
push(arr, x)
remove(arr, i)

keys(map)
values(map)

assign(data)
flatten(data)
```

### Exit criteria

API conventions documented and consistent.

---

# v0.9.14 — Error Quality & Edge Hardening

Goal: **make failures understandable**

Improve error messages for:

* destructuring mismatch
* invalid iteration
* type coercion failures
* function call resolution
* invalid regex
* invalid subscripts

Edge cases:

* empty collections
* nested tuples
* invalid imports
* malformed pipelines

### Exit criteria

Errors are readable and interpreter state remains stable.

---

# v0.9.15 — Test Suite

Goal: **introduce regression safety**

Currently:

```
No test suite
```

Add harness and tests for:

### Core language

* literals
* coercion
* operators

### Containers

* arrays
* tuples
* maps
* sets

### Control flow

* loops
* destructuring
* functions

### Modules

* process
* ds
* str

### Runtime

* REPL
* imports
* concurrency

### Exit criteria

Every bug fixed earlier gets a regression test.

---

# v0.9.16 — Profiling & Freeze Candidate

Goal: **validate performance and stability**

### Profiling

Identify hotspots:

* coercion
* string concat
* loop iteration
* map/set lookup
* printing

Optimize only obvious bottlenecks.

### Stability testing

Run:

* large datasets
* long pipelines
* repeated REPL sessions

### Exit criteria

Performance measured and acceptable.

---

# v1.0.0 — XF Stable Release

Requirements:

✔ dataset pipeline composition works
✔ container semantics stable
✔ destructuring reliable
✔ interpreter memory stable
✔ API surface coherent
✔ concurrency scope defined
✔ baseline tests exist

### What v1 means

* XF is stable for real data pipelines
* container model finalized
* standard library consistent
* interpreter robust enough for production use

---

# Post-v1 roadmap

## v1.1.0

* parallel aggregation in `core.ds`
* richer dataset utilities
* improved module ergonomics

## v1.2.0

* interpreter-safe threaded XF execution
* deeper concurrency support
* runtime optimization work
