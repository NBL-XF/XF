# math.c


| File | Function | Line | Comment |
|---|---|---:|---|
| math.c | cm_ln | 20 | This function creates an error value for domain violations, which is good. It should also release the coerced numeric argument before every return to avoid subtle lifetime leaks. |
| math.c | cm_min | 38 | Min/max are straightforward and good examples of simple native functions. They are also good templates for future numeric helpers because they use the common coercion path. |
| math.c | cm_clamp | 52 | Clamp is a high-utility helper. The implementation should clearly document whether bounds are reordered when min > max or whether that is treated as caller error. |
| math.c | cm_rand | 60 | Random helpers should document seeding behavior clearly. Users often assume deterministic output after `srand`, so the contract needs to stay explicit. |
| math.c | cm_srand | 65 | This function is stateful by nature. If XF ever supports isolated interpreter instances more aggressively, random state should be reviewed for per-instance rather than process-wide behavior. |