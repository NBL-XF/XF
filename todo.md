1.  The VM is not re-entrant: XF-language functions passed to `core.process.run` execute on the calling thread, not in parallel.
2.  `core.os.read` and `csy_run` buffer up to 64 KB; larger files should use `core.os.open` / `core.os.chunk`.
3. `core.img.unvectorize` always writes PNG regardless of the output path extension.
4. The REPL does not persist history across sessions (history is in-process only).
5. Top-level expression / block ambiguity can arise in some edge cases — wrapping in `BEGIN { }` resolves it.
6. Java Interop/FFI
7. Python Interop/FFI
8. NUMA