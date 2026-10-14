# Original TU1.5 decal cache and native export

The original strings at `45F8A8` and `45C600` identify **decals** and the
**decal vertex cache**. Earlier attachment notes called this a light pool;
that identification was incorrect. `D6D10` is a lighting dependency of decal
creation, not the identity of the cache itself.

`1800B0` constructs the real 1,024-record decal pool, `3C2C` context and
`43880` vertex cache. The source allocator reserves `0x500` 64-byte pages.
`180170` resets the context, fills its first `3800` bytes with `FFFFFFFF`, sets
the unassigned head to NONE and makes the original pool active. Native startup
now invokes both original bodies in the order of their literal lifecycle rows.

The initialization regression is intentionally separate from first-tick
graphics acceptance. Its original red receipt is
`analysis/map-world-features/graphics-owner-20261013/regression-before-fix.log`.
The state-only check now passes. Ordinary Zanzibar still stops at `439F4` when
no native graphics owner is bound.

## Completed CPU capture

`graphics-owner-20261013/repro.cpp` minimizes that failure to an actual source
world with one player and no native scenery. Zero players do not trigger it.
The `capture` mode explicitly supplies unsubmitted D3D command-recording input
and reads actual geometry sectors through the native resource I/O boundary.
It does not install a production fallback or render anything.

That mode completes one actual Zanzibar player update, retaining **28 decal
records / 32 quads**, all layer1. `capture.log`, the binary cache outputs and
`cache-fixture.json` preserve the result. The ordinary map contains 40 authored
placements; the capture is not proof that all 40 have been displayed. The
diagnostic command adapter is absent from the clean launcher runtime.

`source_decal_vertex_cache_tu15` exports the actual retained geometry without
allocating source records, changing salts, clipping substitute polygons or
advancing lifetimes. It requires a completed, quiescent source update.

| Source field | Meaning established by original writes/reads |
| --- | --- |
| Decal+0 | Full record salt, combined with its actual pool ordinal |
| +2 / +3 | Flags / layer |
| +4 | Full salted vertex-cache token |
| +A | Cluster |
| +C | Source projection center |
| +18 / +1C / +20 | Birth tick / lifetime / decay |
| +24 | Original packed ARGB |
| +29 | Source byte passed to `144D0` at `440C2`; complete state semantics remain open |
| +2A | Quad count |
| +2C | Actual decal definition datum |
| Cache allocation+4 / +8 | Allocated page count / first page |

Original `183C10` writes four 16-byte vertices per page: XYZ floats followed
by packed signed 11/11/10 components. Its projection calculations feed U/V
and baked brightness into `183AF0`; these are not packed normal vectors or
packed colors. The decoder follows the signed component normalization in the
retained xemu NV2A vertex implementation (`pgraph/glsl/vsh.c`). The original
packing routine independently passes 64 whole-function comparisons.

The new cache test checks all retained record/quad counts, original first-quad
positions and UV/brightness corners, stale full cache salts, allocation bounds
and read-only repeatability. The captured addresses are fixture observations,
not hardcoded native allocation addresses.

## Remaining graphics binding

`439C0` touches the actual cache epoch, then emits `41710,0` through the D3D
driver at `411568`. The method is named `VTXBUF_VALIDATE` in the primary
[envytools NV20 register description](https://github.com/envytools/envytools/blob/master/rnndb/graph/nv20_3d.xml).
That register name alone does not prove an Xbox cache-coherency implementation.
A native binding must consume the actual written cache and submit it through
the Vulkan backend; an unsubmitted command bank does not meet that requirement.

Original `43A30` prepares decal render state, and `44030` walks each layer's
cluster chain, stamps the primary record to avoid duplicate submission and
draws its cache range as a quad list. `1CC10` selects rasterizer global vertex
shader index3, classification0. The actual Zanzibar asset is
`rasterizer\\vertex_shaders_xbox\\decal`: 212 bytes, declaration `{0,3}`.
Its third packed projection component remains live in the vertex program.

The current generic world UV input supplies only two components, and the
existing native triangle decal adapter admits only multiply layers2/3. It
cannot establish the lit layer1 output in this capture. Original per-layer
combiner state, color input10, UV/brightness, draw order, lifetime fade and
cache-write consumption must be bound before claiming rendered decal parity.
No such claim or all-map completion is made by this CPU export.
