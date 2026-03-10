# XF Version Roadmap

# v0.9.11 — Runtime Correctness Pass

Goal: **eliminate correctness bugs before v1**

### Ownership / reference audit

Function call argument handling (EXPR_CALL)

args[] values are not released on many return paths

Interpreted-function parameter binding uses:

ps->value = av

Should be:

ps->value = xf_value_retain(av)

Problems caused:

argument values leak

borrowed values may be stored in scope

Fix

release args[] before returning

retain values when binding parameters

Return value lifetime (STMT_RETURN)

Problem pattern:

it->return_val = interp_eval_expr(...)
ret = it->return_val
scope_pop()
return ret

Risk:

scope_pop() may release the value being returned.

Fix

xf_Value ret = xf_value_retain(it->return_val)
scope_pop()
return ret
Pipe function call (EXPR_PIPE_FN)

Problems:

left argument passed into function without retain

obj, callee, left not released on many exits

Fix

treat left exactly like function call arguments

ensure cleanup releases:

left

obj

callee

Major Memory Leaks
Binary operations (EXPR_BINARY)

Values created:

xf_Value a = interp_eval_expr(...)
xf_Value b = interp_eval_expr(...)

Most operator branches:

never release a or b

Fix pattern

result = op(a,b)
xf_value_release(a)
xf_value_release(b)
return result
Unary operations (EXPR_UNARY)

Temporary values created via:

xf_coerce_num

xf_coerce_str

Often returned without releasing the original value.

Fix

release original temporary values.

Assignment (EXPR_ASSIGN)

Problems:

cur = lvalue_load(...)
rhs = interp_eval_expr(...)
rhs = apply_assign_op(cur, rhs)

Leaks:

cur never released

original rhs may be lost when replaced

Fix

release cur

release old rhs when overwritten

Map / Set Literal Construction
EXPR_MAP_LIT

Current pattern:

ks = xf_coerce_str(...)
xf_str_release(ks.data.str)

Problems:

inconsistent with rest of interpreter

bypasses xf_value_release

Fix

xf_value_release(ks)

Also ensure release of:

kv

vv

EXPR_SET_LIT

Same issue as map literal.

Ensure release of:

v

coercion temporaries

Smaller Expression Leaks

These nodes evaluate values but never release them:

EXPR_LEN

EXPR_CAST

EXPR_STATE

EXPR_TYPE

EXPR_MEMBER

Typical pattern:

v = interp_eval_expr(...)
result = something(v)
return result

Fix

result = something(v)
xf_value_release(v)
return result
Things That Look Correct

These areas appear well-managed:

Loop binding

values retained when stored in loop variables

Worker thread parameter binding

uses xf_value_retain(av)

Tuple construction

partially constructed tuples cleaned up on failure

Iterator cleanup

temporary values released on:

break

continue

return

error

These sections demonstrate the correct ownership model.

Global Rule To Adopt

Treat every result of interp_eval_expr() as owned.

If storing in a symbol or container
xf_value_retain(v)
If returning past scope cleanup
xf_value_retain(v)
If value is temporary
xf_value_release(v)
Structural Refactor Recommendation

Use cleanup blocks for hot paths.

Example pattern:

xf_Value a = interp_eval_expr(...)
xf_Value b = interp_eval_expr(...)
xf_Value result = NAV

switch(op) {
    case ADD:
        result = val_add(a,b)
        break
}

cleanup:
xf_value_release(a)
xf_value_release(b)
return result

This prevents:

forgotten releases

inconsistent return paths

double frees

Fix Priority Order

EXPR_CALL

STMT_RETURN

EXPR_PIPE_FN

EXPR_BINARY

EXPR_ASSIGN

EXPR_MAP_LIT

EXPR_SET_LIT

small expression nodes (LEN, CAST, etc.)


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
