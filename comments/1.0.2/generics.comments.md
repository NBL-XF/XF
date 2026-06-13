# generics.c


| File | Function | Line | Comment |
|---|---|---:|---|
| generics.c | cg_join | 3 | This function overloads join semantics across numbers, strings, arrays, maps, and sets. The flexibility is useful, but the behavior matrix is large enough that it should be documented explicitly. |
| generics.c | cg_join | 3 | The numeric branch attempts to coerce the joined result back to a number. That is surprising behavior for a function named join and could confuse users who expect a string result every time. |
| generics.c | cg_split | 175 | Split is a very user-visible primitive and should stay consistent with the `<3` shorthand. If either changes, the other should be regression-tested alongside it. |
| generics.c | cg_contains | 420 | Contains is often used in control flow. Make sure its behavior across strings, arrays, maps, and sets is documented because users will assume different semantics for each shape. |
| generics.c | cg_length | 487 | There appears to be both `size` and `length`-style API expectations in user code. If this is the implementation behind only one alias, the README should call that out to avoid avoidable NAV errors. |