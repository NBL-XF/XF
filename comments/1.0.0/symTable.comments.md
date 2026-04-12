# symTable.c


| File | Function | Line | Comment |
|---|---|---:|---|
| symTable.c | `call_any_fn` | 15 | This helper relies on external fn-caller hooks from another compilation unit, which makes callback execution depend on hidden global/TLS runtime state. That dependency should be documented prominently because it affects concurrency and embedding. |
| symTable.c | `sym_init` | 51 | Initialization is straightforward, but the symbol table still owns both semantic and runtime-facing data. Long term it may help to distinguish compile-time symbol metadata from runtime values more explicitly. |
| symTable.c | `scope_grow` | 151 | Open-addressing growth logic is performance-sensitive and easy to get subtly wrong. This is a good candidate for dedicated property-based tests because symbol correctness failures can look like parser or runtime bugs elsewhere. |
| symTable.c | `sym_register_builtins` | 375 | Builtin registration is user-visible API surface. A table-driven registration format with aliases, return types, and doc strings would make future growth easier and reduce manual inconsistency. |
| symTable.c | `sym_print_scope` | 399 | Debug printing is useful, but it lives next to core symbol operations. Moving debug dumps behind a dedicated diagnostics module would keep the main symbol-table logic leaner. |