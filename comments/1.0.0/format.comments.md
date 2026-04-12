# format.c


| File | Function | Line | Comment |
|---|---|---:|---|
| format.c | cf_pad_left | 5 | Several pad helpers duplicate nearly identical width/pad-character logic. A shared internal padding helper would reduce maintenance cost and keep behavior consistent. |
| format.c | cf_wrap | 78 | The wrap logic is readable but custom. Edge cases around long tokens, tabs, and existing newlines should be covered explicitly because users will treat formatting routines as deterministic. |
| format.c | cf_format | 159 | This is effectively a mini templating engine. It should be documented with supported placeholder forms because users will otherwise discover behavior by trial and error. |
| format.c | cf_json | 481 | JSON serialization is a core interoperability path. Any formatting or escaping bug here becomes very visible, so this function deserves strong round-trip examples in the docs. |
| format.c | jp_parse | 530 | The parser is doing a lot of recursive descent in one function family. Depth limits and error messages are worth reviewing because malformed user input will hit this path often. |
| format.c | cf_from_json | 641 | The from-json entrypoint is a high-value API for users. Consider documenting the exact value mapping from JSON null/boolean/array/object into XF types and states. |
| format.c | cf_table | 692 | Table rendering is useful but expensive for large maps/arrays because width calculation and formatting require multiple passes. That should be noted in docs or mitigated with a streaming mode. |