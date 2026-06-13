# internal.h


| File | Function | Line | Comment |
|---|---|---:|---|
| internal.h | NEED | 18 | This macro is convenient, but it always returns NAV(void) on arity failure. If user-facing diagnostics matter, consider a richer error path for missing arguments. |
| internal.h | FN | 21 | Module registration through macros keeps files short, but it also hides the exact registration logic. A small comment explaining ownership and duplicate-key behavior would help maintainers. |
| internal.h | MATH1 | 24 | The math coercion macro is concise, but macro debugging can be painful when ownership bugs show up. Inline helpers would be easier to instrument if this code grows further. |
| internal.h | MATH2 | 33 | Like MATH1, this macro is useful but opaque. It is worth keeping these macros very small and very stable because many module functions depend on them. |
| internal.h | core_set_fn_caller | 59 | The helpers declarations in this header effectively define the cross-module callback ABI. Any signature or lifetime change here affects process, ds, and symTable code together. |