# process.c


| File | Function | Line | Comment |
|---|---|---:|---|
| process.c | cp_thread_fn | 8 | This worker thread currently handles native functions directly and returns NAV for non-native callbacks. If user-defined XF functions are expected here, this should go through the fn-caller bridge too. |
| process.c | cp_worker | 25 | Representing a worker as a map is flexible but loosely typed. A dedicated worker object or at least a documented schema would reduce the chance of accidental breakage. |
| process.c | cp_split | 40 | The second argument behaves like a chunk count, not a chunk size. That distinction matters because user scripts often assume the opposite. The README should make this explicit. |
| process.c | cp_assign | 72 | This fallback-to-original-row behavior is user-friendly, but it can hide callback failures. Consider an alternate strict mode for users who want transform errors to surface immediately. |
| process.c | cp_index | 129 | Index construction is allocation-heavy and builds nested maps-of-maps-of-arrays. That is powerful, but it deserves examples so users understand the returned shape. |
| process.c | cp_run | 214 | The run path is the heart of process concurrency. Thread creation, joining, and result merge logic here should be kept simple and heavily tested because many higher-level APIs depend on it. |