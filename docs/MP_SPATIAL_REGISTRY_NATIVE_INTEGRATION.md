# Native unsorted spatial registration

The native `1CE650` registration and `1CE830` removal paths match four original
calls, twelve complete states and 94 ordered stores in normal and
address/undefined/leak sanitizer builds. Full raw pool headers, all elements,
bitmaps, registry heads, reverse links, salts, copied payloads, source visit
flags and overflow are compared. This is a bounded native library comparison;
the playable runtime has no caller for these functions.

The exporter proves 72 physical aliases against original pointers and complete
buffer bytes. It independently derives all 384 legacy chain views, including
52 records, from the complete original heads and forward pool, checking full
salted datums, next links, object identities, record bytes and order. These
decoded views are verified derived data rather than additional physical memory.

Two `14BD00` cluster walks remain explicit original full-output services. The
measured registration inputs have positive radius and a valid location without
an explicit cluster mask. Absent location/radius, positive debug-fill, explicit
masks, allocator exhaustion and positive portal traversal have no measured
native comparison. They are not substitutes for live terrain/Havok admission,
continuous object settling or the original full frame route.

Authority is the unchanged, independently repeated original transform producer
suite described in [the source research](MP_ITEM_TRANSFORM_PRODUCERS_RESEARCH.md).
The exporter pins its six owning artifacts, 179 prior dependencies and 65
byte-identically repeated instruction files. No new guest inputs, successful
callbacks, readiness flags or physical results are supplied by this comparison.

Validation artifacts:

- [Normal comparison](../analysis/multiplayer/integration/spatial-registry-differential.json)
- [Sanitizer comparison](../analysis/multiplayer/integration/spatial-registry-asan-differential.json)
- [Normal focused checks](../analysis/multiplayer/integration/spatial-registry-focused-normal.log)
- [Sanitizer focused checks](../analysis/multiplayer/integration/spatial-registry-focused-asan.log)

Reproduce with `spatial_registry_tests` and
`tools/differential_spatial_registry.py`. The source fixture is retained outside
the repository under ignored analysis storage; its CTest skips when absent.
