# ds.c


| File | Function | Line | Comment |
|---|---|---:|---|
| ds.c | ds_sort_cmp | 98 | Uses global sort context guarded by a mutex, which serializes all dataset sorts. A context pointer passed through a reentrant sort helper would scale better and avoid hidden global state. |
| ds.c | cd_sort | 114 | Copies the entire dataset before sorting, which is safe but expensive for large arrays-of-maps. Consider documenting the copy semantics or offering an in-place variant for large jobs. |
| ds.c | cd_agg | 148 | Group keys are coerced to strings on every row. For very large datasets this becomes a hot path; caching or specialized numeric grouping could reduce allocation pressure. |
| ds.c | cd_merge | 207 | Join mode is implemented as a nested scan over ds2 for every row in ds1. This is simple but O(n*m); building an index once would be much faster for larger joins. |
| ds.c | cd_index | 288 | Index construction appears to retain and duplicate many intermediate values. Double-check ownership around map/array insertion here because this path is both allocation-heavy and easy to leak. |
| ds.c | cd_flatten | 610 | The flatten helper is central to worker composition. It is worth documenting exactly which nested shapes are flattened and which are preserved, because user code depends on that contract. |
| ds.c | cd_agg_parallel | 923 | Parallel aggregation captures caller context and rebinds it in worker threads, which is the right shape. The remaining risk is hidden shared state in the callback/compiler path, so this should stay under concurrency regression tests. |
| ds.c | cd_stream | 1120 | This function mixes file reading, row-shape construction, callback dispatch, and threading concerns in one long routine. Splitting it into reader, worker, and merger helpers would make bugs much easier to isolate. |