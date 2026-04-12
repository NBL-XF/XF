# str.c


| File | Function | Line | Comment |
|---|---|---:|---|
| str.c | cs_arg_pat | 5 | This helper is shared by multiple regex-aware string functions, so it effectively defines the accepted pattern contract for the string module. Keep it aligned with the regex module and `~` operator behavior. |
| str.c | cs_substr | 144 | Substring APIs need very clear negative-index and bounds semantics. If XF supports clamping or truncation here, the docs should spell that out with examples. |
| str.c | cs_replace | 216 | Single replacement and replace-all should be documented together so users understand whether regex patterns, backrefs, and flags behave identically in both forms. |
| str.c | cs_replace_all | 257 | This is a likely hot path in ETL workloads. Buffer growth strategy and zero-length-match handling are worth reviewing carefully because text replacement code tends to accumulate edge cases. |
| str.c | cs_sprintf | 336 | Formatting helpers are convenient, but users will expect printf-like behavior. Any intentional deviation from standard format semantics should be documented explicitly. |
| str.c | cs_concat | 423 | Concat is simple but very widely used. It is worth keeping it boring and predictable, especially around state propagation and coercion, because many shorthands lower into ordinary function calls. |