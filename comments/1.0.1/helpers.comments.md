# helpers.c


| File | Function | Line | Comment |
|---|---|---:|---|
| helpers.c | arg_str | 35 | The thread-local rotating string slot design is a clever way to avoid leaking coercion results, but it also creates subtle lifetime rules. A short comment in the header about pointer validity would help users of this helper. |
| helpers.c | propagate | 88 | This helper returns the first non-OK state, which is simple and consistent. It is worth documenting as part of the core module contract because many native functions depend on it. |
| helpers.c | make_str_val | 95 | This helper centralizes string value construction and is a good ownership boundary. Keep new string-returning native helpers routed through it to avoid refcount drift. |
| helpers.c | core_set_fn_caller | 114 | This is a critical concurrency boundary. The active VM/symbol context should remain thread-local, and any fallback global behavior here needs to be treated as high-risk under worker execution. |
| helpers.c | core_get_fn_caller | 128 | Getter behavior here determines whether XF callbacks succeed inside process/dataset workers. Changes to this section should always be paired with worker callback regression tests. |