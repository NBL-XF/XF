# regex.c


| File | Function | Line | Comment |
|---|---|---:|---|
| regex.c | cr_parse_flags | 5 | Flag parsing is intentionally small, which is good. Keep the accepted flag set documented so users know exactly which regex modifiers XF supports. |
| regex.c | cr_compile | 14 | The Perl-style shorthand expansion is a great usability feature, but it is also a compatibility boundary. Any change here can silently alter user regex behavior across the whole language. |
| regex.c | cr_apply_replacement | 51 | Replacement expansion is currently fixed-buffer based. For very large replacements or many groups, a growable buffer would be safer than a hard-coded stack array. |
| regex.c | cr_match | 132 | Match should be documented with return shape, especially group capture layout. Regex APIs become much easier to use when users know whether they get arrays, maps, or plain strings back. |
| regex.c | cr_replace_impl | 220 | This is a high-traffic path for text transformation. Pay close attention to resource release on early compile or argument failures, because leaks here will show up in batch workloads. |
| regex.c | cr_test | 329 | Test is a simple boolean-like API and a good baseline for operator-path comparisons. It is useful for users when `~` and module regex behavior need to be compared during debugging. |