# parser.c


| File | Function | Line | Comment |
|---|---|---:|---|
| parser.c | `parser_init` | 15 | The parser state is minimal and clean, but a lot of semantic lowering now happens in parse-time helpers. That makes parser correctness increasingly tied to runtime API names, which is worth documenting clearly. |
| parser.c | `parser_error` | 93 | Printing directly from parser code is practical for a CLI tool, but it tightly couples parse logic to stderr output. A structured error object would make IDE/editor integrations and unit testing cleaner. |
| parser.c | `token_is_type_kw` | 150 | Type-keyword recognition is parser policy that can drift from lexer keyword support and symbol semantics. Consider centralizing type metadata so declarations, casts, and signature parsing all share one definition. |
| parser.c | `make_call1_named` | 216 | Stringly-typed lowering into named calls is flexible, but it also means parser sugar depends on runtime/export names staying stable. A central enum or builtin registry for lowered sugar targets would reduce rename hazards. |
| parser.c | `parse_pipe` | 977 | Pipe lowering is a readability win for users, but the implementation is subtle because argument insertion depends on RHS shape. This is a good area for focused parser tests because it is easy to introduce precedence regressions. |
| parser.c | `parse_type` | 1711 | Type parsing is a user-facing contract and should remain extremely stable. Consider a table-driven implementation that reuses the same metadata as declaration parsing and signature validation. |