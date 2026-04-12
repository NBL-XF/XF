# ast.c


| File | Function | Line | Comment |
|---|---|---:|---|
| ast.c | `ast_program_free` | 588 | The free path is deeply recursive through expressions, statements, and top-level nodes. Large generated ASTs may risk stack-heavy destruction; an iterative teardown path would make the runtime more robust for huge inputs. |
| ast.c | `ast_expr_free` | 604 | This function owns many shape-specific cleanup paths. It is a good candidate for smaller helper functions per expression family to reduce the chance of future ownership bugs when new expression kinds are added. |
| ast.c | `ast_stmt_free` | 740 | Statement cleanup mirrors parser/compiler growth closely. Add a default audit comment or assertion for newly introduced StmtKind values so missing free branches fail loudly during development. |
| ast.c | `ast_top_free` | 849 | Top-level cleanup is another place where new language features can silently leak if not wired in. Consider a compile-time “exhaustive switch” pattern or a debug assertion in the default branch. |
| ast.c | `ast_expr_print` | 924 | Pretty-printing is useful, but this file now mixes allocation, destruction, and debug formatting responsibilities. Splitting printers into a dedicated debug/print module would keep AST lifecycle logic easier to maintain. |