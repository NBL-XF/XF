Architectural gaps (known, not trivially fixable):

core.process.run and core.ds.stream return NAV for XF-language (non-native) functions — re-entering the interpreter from a raw pthread isn't safe without a full interpreter clone per thread. The only clean fix is giving each spawn thread its own Interp instance sharing just the VM, which is a larger refactor.
core.process.assign result shape ({columns, rows}) isn't handled by core.ds.flatten — flatten knows arr-of-maps and arr-of-arr-of-maps, but not that semi-relational format. Either assign should be changed to return a plain arr-of-maps, or flatten needs a third branch.

Missing entirely:

Test suite — nothing exists, zero coverage
core.ds parallel aggregation via spawn/join (the interp-level version, distinct from the pthread agg_parallel) — never started
API consistency audit — naming, argument order, return type conventions across all modules haven't been reviewed

1.0.0 prep (untouched):

Performance profiling / runtime optimization
Error message quality
Production stability / edge case hardening


The assign result shape mismatch is probably the most immediately broken thing — it means process.run → ds.flatten doesn't actually produce a usable dataset without extra unwrapping. Want to fix that first by having assign return a plain arr-of-maps instead?