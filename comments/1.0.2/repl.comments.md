# repl.c


| File | Function | Line | Comment |
|---|---|---:|---|
| repl.c | `xf_repl_print_result` | 14 | The REPL clears the whole stack after printing, which is simple, but it makes the REPL session semantics fairly opinionated. Document that behavior clearly so users do not expect hidden result retention. |
| repl.c | `xf_repl_eval_line` | 27 | This function recompiles each entered line into VM chunks, which is appropriate, but it also manually manages transient VM artifacts. A dedicated “ephemeral eval chunk” abstraction would make this less fragile. |
| repl.c | `xf_repl_clear_eval_artifacts` | 58 | Manual cleanup of begin/end/rules/pattern arrays is a hotspot for lifetime mistakes. If more transient REPL state gets added, this cleanup should probably move behind a single VM helper. |
| repl.c | `xf_run_repl` | 87 | The REPL path duplicates some runtime-special binding logic from the file runner. A shared session/bootstrap helper would keep file mode and REPL mode behavior aligned. |
| repl.c | `xf_run_repl` | 87 | Now that readline/history are in use, this file is becoming a genuine UI layer. Keeping terminal UX concerns separate from evaluation concerns will make future REPL features easier to add safely. |