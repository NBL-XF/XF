# vm.c


| File | Function | Line | Comment |
|---|---|---:|---|
| vm.c | `vm_call_compiled_fn` | 24 | This helper contains a second execution loop embedded inside the VM implementation. That is powerful, but it also increases the chance that opcode behavior diverges between call paths unless kept tightly synchronized. |
| vm.c | `val_add` | 1278 | The arithmetic helpers are the semantic core of the language. Any coercion or state-propagation policy change here can ripple outward widely, so they are a great place for focused behavioral tests. |
| vm.c | `val_mod` | 1348 | Modulo semantics are often a source of surprises around coercion, division-by-zero, and non-integer inputs. A short comment on intended behavior would make maintenance easier. |
| vm.c | `vm_run_chunk` | 1459 | This is the VM hot path and has become quite large. Grouping opcode handlers into helper families would improve readability and make it easier to reason about stack effects. |
| vm.c | `OP_DELETE_IDX` | 1483 | Delete semantics are user-visible and now support more than one collection shape. This opcode should stay aligned with parser/compiler expectations so delete never silently degrades into a no-op again. |
| vm.c | `OP_MATCH / OP_NMATCH` | 2040 | Regex operator behavior is subtle because it must accept both regex values and string patterns while honoring flags. This path should stay closely tested against the regex module APIs to avoid semantic drift. |
| vm.c | `vm_run_begin` | 2424 | BEGIN execution result handling has already had user-visible edge cases. Keeping clean distinctions between normal exit, VM error, and program-requested exit is important for predictable CLI behavior. |
| vm.c | `vm_feed_record` | 2434 | Record-feeding is one of the few places where runtime semantics and streaming I/O meet directly. It deserves extra care because bugs here can look like parser failures, data-shape issues, or control-flow problems. |