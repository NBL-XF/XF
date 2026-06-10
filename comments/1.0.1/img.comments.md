# img.c


| File | Function | Line | Comment |
|---|---|---:|---|
| img.c | img_map_set_str | 28 | This helper is compact but depends on careful ownership transfer through temporary string values. Keep the retain/release pattern consistent here because image metadata maps are built frequently. |
| img.c | img_value_to_size | 59 | Size coercion silently truncates floating-point values. That is acceptable for image dimensions, but users may benefit from clearer validation or rounding rules. |
| img.c | img_clamp_byte_from_value | 77 | Normalized and raw-byte modes are both supported, which is useful. A small doc note with examples would make the API much easier to use correctly. |
| img.c | ci_vectorize | 107 | This function does a lot: file load, mode selection, normalization, and shape conversion. Splitting decode and XF-value construction would make failures easier to diagnose. |
| img.c | ci_unvectorize | 216 | Reverse conversion should be documented with expected input shape and channel layout. Without that, users will have to infer whether they should pass flat arrays, nested tuples, or normalized values. |