# main.c


| File | Function | Line | Comment |
|---|---|---:|---|
| main.c | `xf_run_program` | 85 | Program execution setup currently recreates and wires multiple subsystems directly in `main.c`. A single “runtime/session init” helper would reduce duplication with the REPL path and make startup semantics easier to audit. |
| main.c | `xf_run_program` | 85 | BEGIN/record/END handling is readable, but this function mixes CLI concerns with runtime policy. Returning structured status rather than printing directly here would make embedding or testing the interpreter easier. |
| main.c | `main` | 146 | The CLI is intentionally small, which is good, but the accepted flags are now part of user-facing behavior. Keep the README and `usage()` text in lockstep to avoid stale entrypoint documentation. |