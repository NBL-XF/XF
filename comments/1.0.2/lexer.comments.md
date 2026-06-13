# lexer.c


| File | Function | Line | Comment |
|---|---|---:|---|
| lexer.c | `xf_keyword_lookup` | 399 | The keyword table is compact and readable, but it is easy for lexer and parser type/state keywords to drift apart. Consider generating or centrally defining keyword metadata to avoid subtle inconsistencies. |
| lexer.c | `skip_ws` | 481 | Whitespace and newline handling are surprisingly semantic in XF, especially across REPL and file modes. This function should stay heavily commented because statement-separator behavior is language-defining, not cosmetic. |
| lexer.c | `scan_regex` | 729 | Regex scanning is naturally one of the trickiest lexing paths. It would benefit from more internal structure or comments around literal-vs-division disambiguation so future shorthand/operator additions do not destabilize it. |
| lexer.c | `scan_subst` | 755 | Substitution syntax is specialized enough that it may deserve its own focused helper module if the language keeps adding text-editing operators. Right now it is easy for lexing complexity to accumulate here. |
| lexer.c | `next` | 901 | The main token dispatch function is long and central to many recent syntax additions. This is a prime place for regressions when adding new shorthand tokens, so keeping operator recognition grouped and table-driven would help. |
| lexer.c | `xf_token_kind_name` | 1315 | This is effectively the source of truth for debug output. Whenever new tokens are added, tests should verify that token names are wired here too, otherwise diagnostics become harder to trust. |