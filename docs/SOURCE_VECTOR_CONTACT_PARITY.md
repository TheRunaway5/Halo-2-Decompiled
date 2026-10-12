# Ordinary map and dynamic vector dispatcher

`source_vector_contact` ports the ordinary portion of retail `1697C0` over
explicit map, MOPP, owner and registered spatial banks. It does not create a
registry, choose a pose, activate gameplay queries, or substitute a shape for
missing source data. Query flag `20000000` requests the subsequent source
location-repair service and is rejected explicitly.

The native contract is in `include/halo2/source_vector_contact.hpp`. The result
retains the literal 96-byte packet. The caller supplies its initial words;
fields not written by the selected original path survive. This includes the
upper half of the material dword, model identity fields during map contacts,
surface fields during fog contacts, and the final dword. A hit does not imply
that every packet field has been freshly initialized.

## Inputs and order

The caller supplies actual `BspSurfaceLighting` collision and accelerator
banks, the selected `4686C4` BSP index, the literal `47D8E0` material fallback,
the BSP material-to-global table, signed cluster fog designators and authored
fog planes, current breakable bits, the collidable raw spatial pool, and current
owner/header/attachment fields. The target callback is the complete
`1EFB40` lookup / `1EFF40` / `168B40` service already represented by
`SourceObjectVectorTarget`. It queries the complete segment independently of
the dispatcher's current closest fraction.

The source initializes type/fraction, start/end location and terrain/instance/
object sentinels. If neither direction bit `01800000` is present, it adds both.
`168AE0` derives the lower collision flags. The main BSP query always runs to
obtain the source-ordered visited leaves, even when flag 1 does not admit its
contact. The first/last leaf and its authored render cluster become the packet
locations.

For each first visited cluster, the dispatcher checks the following stages in
order against one shared packet:

1. Flag 2: signed fog designator below -1 selects its low-seven-bit BSP fog
   plane. Material NONE, same direction, endpoint, denominator and minimum
   velocity rejects are literal. A strictly closer plane changes the packet.
2. Flag 4: the actual cluster MOPP visitor calls `1691A0` in bytecode order,
   sharing its current collector fraction. Instance deduplication is global to
   this query. An admitted instance packet may replace an equal-fraction
   packet; only a strictly smaller fraction updates the accelerator collector.
   Flag `10000000` sets that collector fraction to zero on an admitted hit.
3. Flag 8: `B8940`/`B89B0` collidable head/link order is consumed without
   re-registration or sorting. Low-sixteen-bit owner deduplication precedes
   stored type/flag, whole-segment sphere and full-handle exclusion checks.
   A missing type mask receives `1FFF0`. Exact `169510` then performs current
   owner/attachment admission and strict closer target replacement.

First-contact exits also preserve a contact from an earlier stage, including
when the current stage does not improve it. The final point is always written
as the original separate `origin + delta * fraction` operations.

Both terrain and instance world planes use the crossed geometric plane index
from the lower BSP query. The surface's signed plane contributes the reversal
bit and recorded identity. `source_collision_world_plane` supplies the exact
`168B40` transform and zero-subtraction reversal; these two plane identities
are not interchangeable.

## Independent source comparison

`tools/differential_source_vector_contact.py` executes unchanged retail
`1697C0`, all of its actual map/accelerator/object callees, and independent
`168B40` target conversion against the immutable restored snapshot. It verifies
every executed instruction against the pinned original XBE. Read-only hooks
record `169510` entries and admitted `1EFB40` targets; no routine, result,
lookup, math or allocator service is replaced.

The retained state is paused at `0012B33E`, source tick 12474. The rays are
explicit diagnostic inputs through actual cached primitive spheres. They are
not observed gameplay queries and are not labeled original frame 20 or the
later captured shadow phase. Original snapshot FCW `023F`, MXCSR `1F80`, GDT
and segment descriptors are explicit replay inputs.

The frozen fixture covers:

- All 3,792 previously recorded dynamic queries with the literal initial and
  retained lighting flags and full-handle exclusions.
- All 948 corresponding static queries, including duplicate geometric rays
  owned by different actual sphere records.
- 952 additional ordinary flag/default/type-mask/first-contact controls and
  nonzero caller sentinels that expose partially written packet fields.
- 843 separately labeled synthetic fog-metadata inputs. Zanzibar's retained
  fog plane has material NONE, so these explicitly supply designators,
  material and plane values in the offline memory copy. All geometry, current
  owner, pose and spatial banks remain literal, and original instructions are
  unchanged. The final packet is a fog contact in 248 cases.

There are 6,535 cases, 2,273 ordered admitted target callbacks and 678 target
hits. The oracle verifies 2,903 original instruction sites and protects all 223
retained owner/spatial banks. The explicit fog inputs are restored after replay.

The CPU test runs every case twice. The first pass consumes the independently
recorded original target outcomes, proving the outer callback boundary. The
second pass computes those outcomes with native `source_object_posed_target`
using the independently frozen 39 literal collision/model banks, 158 current
WORLD/active-permutation owner banks and 44 actual lookup misses. Every hit's
15-field target packet, target order, status, and all 24 final packet words
match. Thus 156,840 final packet words are exact in each pass (313,680 across
both), with 10,170 independently recorded target payload words checked in the
integrated pass. No target outcome is an observed-only service in that pass.

## Reproduction and boundaries

Run the offline oracle with `.tools/python/bin/python
tools/differential_source_vector_contact.py`. The immutable source snapshot
and original XBE must be present. The checked fixture and provenance are
`tests/fixtures/retail_source_vector_contact.tsv` and its adjacent
`.provenance.json`; complete source observations are in
`analysis/multiplayer-shading/vector-contact-original.json`.

The CMake target is `source_vector_contact_tests`; CTest is
`retail_source_vector_contact`. It consumes the literal
`retail_source_object_admission_snapshot.bin` and
`retail_source_object_posed_target.tsv` banks and the original Zanzibar cache.
The module compiles with strict float behavior and contraction disabled.

Normal CTest passes, and the same target passes AddressSanitizer,
UndefinedBehaviorSanitizer and leak detection. Their separate logs are
`analysis/multiplayer-shading/vector-contact-normal.log` and
`analysis/multiplayer-shading/vector-contact-asan.log`; the checked source,
fixture and executable hashes are in `vector-contact-validation.json` there.
This proves the
ordinary dispatcher over these explicit actual and controlled inputs. Live
successful registration chronology, current pose/active-state production,
query scheduling, breakable-state updates, the location-repair branch and
dynamic lightmap attribution/sampling admission remain their own contracts.
No native gameplay activation or physical Xbox hardware claim is made here.
