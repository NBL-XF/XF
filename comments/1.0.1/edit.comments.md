# edit.c


| File | Function | Line | Comment |
|---|---|---:|---|
| edit.c | ce_read_file | 8 | This helper uses fseek/ftell without checking every intermediate failure path and assumes the whole file can be buffered. A safer version would validate all I/O and stream large files when possible. |
| edit.c | ce_write_file | 17 | Writes are not checked for short-write or flush failure. Returning false only on fopen failure can hide disk-full and permission issues that happen after the file is opened. |
| edit.c | ce_split_lines | 24 | The line splitting helper allocates one string per line eagerly. That is easy to work with, but it can become very allocation-heavy for large files. |
| edit.c | ce_regex_replace_str | 52 | This replacement helper manually manages dynamic buffers and regex loops. It would benefit from a dedicated grow helper and explicit comments around zero-length-match handling. |
| edit.c | ce_insert | 102 | Line-oriented editing functions mutate full file images in memory. That is fine for small files, but users should not assume these APIs are cheap on large files. |
| edit.c | ce_diff | 305 | Diff generation is user-facing and worth documenting carefully. Even a brief note on whether it is line-based, stable, or minimal would make the API easier to trust. |