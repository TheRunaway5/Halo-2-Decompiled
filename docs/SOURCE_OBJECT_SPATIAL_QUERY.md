# Original nearby-object enumeration

`source_object_spatial_query` implements retail `BB050`, including its
`14A5B0` portal traversal and independent collidable/noncollidable forward
lists. `source_control_spatial_candidates` connects that query to `151110`'s
parent rejection, literal `FFFFEBCF` type mask, capacity 64 and radius
`(unit_bound_radius + .4) + 3.0`. It consumes current source registration,
primitive and location inputs; it does not derive them from render meshes.

Both visit banks persist in `SourceObjectSpatialQueryState`. Unsigned epoch
wrap retains existing stamps. Type filtering precedes duplicate stamping;
distance and capacity checks follow it. Therefore an out-of-range duplicate
can suppress a later record, and a full result bank still stamps its next
eligible candidate before returning. Results retain complete handles and
cluster/link order. The two list kinds have different squared-distance
accumulation orders, preserved at explicit floating-point boundaries.

`tools/differential_source_object_spatial_query.py` executes unchanged
`BB050`, `B8940`, `B8A10`, `14A5B0`, `14A370`, `120850` and `23A160`, with no
replaced service calls. All executed instruction bytes are checked against
the pinned disc executable. The final comparison passes **818 cases / 4,685
output words**, including returned handles, epochs and FNV hashes of the
complete object/cluster visit banks. The supplied geometry and spatial pools
remain byte-identical; unused output bytes retain their sentinel values.
The comparison uses nearest arithmetic, x87 PC53 and default non-FTZ SSE.
Other caller floating-point environments are not established by this check.

There are 812 declared boundary/random cases and six diagnostic queries over
the retained Zanzibar snapshot's actual geometry, forward-list records and
visit stamps. These use 55 clusters, 125 portals and the unchanged consumed
planes. Backing addresses are relocated and plane indices remapped to those
same plane values. The immutable RAM hash is
`ab7e518eda315ecd55e681d440c988d2814260c292bbbd69972754ac7d32bf55`.

At the actual control bound, the original and native capacity-64 query both
return `E2850016` first, followed by the same eight neighboring objects. At
the retained unit bound, both return only that unit. Capacity-zero/one variants
also match. These are declared diagnostic queries, not captured player input
or an invented player pose beside the computer.

Reproduce:

```sh
cmake --build build-map-world-20261011 --target source_object_spatial_query_tests -j 6
.tools/python/bin/python tools/differential_source_object_spatial_query.py \
  --binary build-map-world-20261011/source_object_spatial_query_tests
ctest --test-dir build-map-world-20261011 --output-on-failure \
  -R '^(source_object_spatial_query_guards|retail_source_object_spatial_query)$'
```

The frozen fixture runs without the RAM export. Recreating the original
comparison requires the retained export under
`analysis/multiplayer-shading/registration-snapshot-restore-20261008`.
Reports, build and verification logs use the `control-spatial-query` prefix
under `analysis/map-world-features/`.

Seven focused CTests and the address/undefined sanitizer fixture run pass.
The full native build also passes. Its frozen executable then passes four
live regressions with no Vulkan validation errors: snow enabled, disabled,
identical enabled repeat, and ordinary Zanzibar startup. The snow on/off
comparison still changes 2,452 RGB pixels. Source and shader observations stay
unchanged during those captures. The executable and verification record are
under `analysis/map-world-features/control-spatial-query-live/`.
Those startup/capture checks do not establish control input or locomotion
parity; the logs retain the missing-original-operands native gait warning.

The [unit-center kernel](SOURCE_UNIT_CENTER.md) now supplies `CAFC0`'s scalar
branches and its observer/head-offset arithmetic with original comparisons.
Live registration/primitive chronology, actual unit-center services/posed state,
posed control markers and `186AB0 → 14F820` input dispatch remain required.
The addresses here belong to the pinned base disc executable. The separate
TU1.1 `MultiplayerObjectRuntime` bank requires verified corresponding routines
and field bindings; matching numeric addresses do not establish equivalence.
This query kernel does not yet make Zanzibar's computer interactive. The
[all-map goal](MAP_WORLD_FEATURES.md) remains active, with
[visible Lockout snow](SOURCE_WEATHER_LIVE_SNOW.md) as its live checkpoint.
