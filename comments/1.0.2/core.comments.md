# core.c


| File | Function | Line | Comment |
|---|---|---:|---|
| core.c | `core_register` | 3 | Module registration is all-or-nothing but does not check for allocation failures from each build_* call. A defensive failure path would make startup errors easier to diagnose instead of relying on downstream null handling. |
| core.c | `core_register` | 3 | The list of core submodules is centralized here, which is convenient, but it also means alias consistency lives in one fragile place. Consider a table-driven registration structure so naming and export metadata are less manual. |
| core.c | `core_register` | 3 | The symbol for `core` is declared and then manually filled. A tiny helper for “declare module builtin” would remove repeated ownership and const/defined bookkeeping patterns that appear in other files too. |