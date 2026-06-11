# value.c


| File | Function | Line | Comment |
|---|---|---:|---|
| value.c | `xf_str_release` | 102 | The explicit underflow trap is very helpful during development. Keep it, but consider also including optional richer diagnostics (owner tag or allocation site) when reference-tracing is enabled. |
| value.c | `xf_val_ok_num` | 151 | The value-constructor family is comprehensive, but there is a lot of repetition. Small internal constructors/macros could reduce maintenance burden when the value layout evolves. |
| value.c | `xf_value_retain` | 439 | This function is central to correctness across the whole interpreter. It would benefit from a short comment describing ownership expectations for shallow vs deep-contained values, especially for new contributors. |
| value.c | `xf_arr_release` | 816 | Array release and nested element release are another high-risk lifetime area. Collection ownership rules should be documented in one place because bugs here often surface far away in parser/VM behavior. |
| value.c | `xf_arr_delete` | 879 | Deletion semantics are now user-visible and are relied on by tests. It is worth documenting whether deletion preserves order and whether it is O(n), because scripts may start depending on that behavior. |
| value.c | `xf_map_release` | 1008 | Map ownership and insertion order are both important here. A brief note on whether order is guaranteed and how deletes affect order would help users and maintainers alike. |
| value.c | `xf_coerce_num` | 1246 | Coercion functions define a lot of user-facing behavior. Keeping their semantics stable and documented is important because small changes here can alter arithmetic, printing, and comparison across the language. |
| value.c | `xf_coerce_str` | 1438 | String coercion is used almost everywhere, including diagnostics and REPL printing. This makes it a de facto language boundary; changes here should be treated as user-facing behavior changes. |