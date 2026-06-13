# interp.c


| File | Function | Line | Comment |
|---|---|---:|---|
| interp.c | `compile_expr` | 13 | This file is doing too much: compile-state management, statement lowering, expression lowering, import compilation, and control-flow patching. Splitting it into `compile_expr.c`, `compile_stmt.c`, and `compile_state.c` would make future changes much safer. |
| interp.c | `g_compile_depth` | 24 | The move to `_Thread_local` is a good emergency concurrency improvement, but it is still a stopgap. These compiler globals should ultimately live in `Interp` so multiple compile sessions in the same thread cannot accidentally share state. |
| interp.c | `compile_named_function` | 186 | Function compilation mutates the shared compile context via `g_fn_ctx`. Even with thread-local storage, nested and re-entrant compile paths will remain tricky; a per-function compile context object passed explicitly would be cleaner. |
| interp.c | `bind_hidden_global` | 291 | Hidden name generation depends on a mutable counter. This avoids collisions most of the time, but it still encodes compiler state globally rather than structurally; moving the counter into interpreter compile state would improve predictability. |
| interp.c | `interp_reset_global_bindings` | 409 | The implementation currently resets thread-local global-binding tables, but the public name suggests object-local behavior. Either make the implementation truly `Interp`-owned or rename/document it as thread-local compile state. |
| interp.c | `interp_compile_program` | 423 | This function is very large and mixes top-level orchestration with detailed compile policy. Breaking out top-level item compilation into separate helpers would improve readability and reduce regression risk. |
| interp.c | `compile_expr` | 960 | Expression lowering handles many unrelated concerns, including regex literals, pipelines, assignments, and anonymous functions. This is the most likely place for operator regressions; smaller operator-family helpers would pay off quickly. |
| interp.c | `compile_expr / anon_counter` | 993 | The anonymous function name counter is still a plain static variable. This is a remaining concurrency hotspot and should at least be thread-local or, preferably, part of compile-state owned by `Interp`. |
| interp.c | `compile_for_stmt` | 1713 | The `for` compiler path is already specialized and likely to keep growing as collection semantics evolve. A dedicated file or helper family for loop lowering would make control-flow bugs easier to reason about. |