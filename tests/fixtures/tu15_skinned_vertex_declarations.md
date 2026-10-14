# Original TU1.5 local vertex declarations and compression

`tu15_skinned_vertex_declarations.original.json` retains whole unchanged
TU1.5 `1C310` outputs for the authored sphere-light classes and literal original
stream table at `4494C8`. The relocated context/tag/output/stack banks are
explicit test inputs. It executes143 distinct instructions across12 calls,
replaces no instruction or callee, and includes both floating and compressed
position formats. Original format words establish byte UNORM index/weight
fetches and their alignment gaps independently of the native adapter.

`tu15_cairo_model_compression.original.json` runs unchanged TU1.5 `1CDB0` on
917 actual Cairo model bounds/flags. The explicit cached-active control admits
identity cases. The comparison ends at `1CF12`, immediately before device
submission; it retains the12 original constant words and does not claim the
device or cache lifetime. Seventy-one distinct original instructions execute.

Both reports authenticate the owned executable SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
retain source cache/tool/executed-bank hashes, and identify their limits.
They use original Xbox caches, not Vista or modified original instructions.

Generation tools: `tools/oracle_tu15_skinned_vertex_declarations.py` and
`tools/oracle_tu15_model_compression.py`, using `.tools/python/bin/python` with
explicit `--xbe`, `--map` and fresh `--output` arguments.
See `docs/TU15_SKINNED_LIGHT_VERTEX_20261013.md` for the native and GPU checks.
