# Original BSP and instance admission

`source_bsp_admission` implements original `134390→132E60→165010/181800` and
`134300` for main mode-0 and spherical-light mode-1 contexts after `1348E0` reset and a fresh instance
visited epoch. `source_bsp_fog_admission` supplies exact `16E210` membership
from the existing native atmosphere bank. Authority is `assets/disc/default.xbe`,
SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

These services consume the actual `SourcePortalVisibilitySet` emitted by the
source portal constructor. They do not infer visibility from instance centroids
or replace the constructor with PVS. This change establishes the CPU admission
contract; renderer activation and whole-frame comparisons are separate gates.

## Authored inputs and ordering

`Cache::load_source_bsp_admission` retains each literal source field:

- Cluster stride `B0`: unsigned geometry count `+24`, six min/max bounds `+54`,
  ordered unsigned-short instance IDs at count/pointer `+98/+9C`.
- Instance stride `58`: signed definition index `+34`, authored world sphere
  center `+3C/+40/+44`, radius `+48`.
- Definition stride `C8`: unsigned geometry count `+24`.

All 21 map caches contain 721 clusters, 1,846 instances, 556 definitions and
2,424 literal cluster-to-instance references. Instance spheres are read from
the cache directly; no render-vertex or transform-derived replacement is used.

`134390` iterates constructed cluster entries in creation order. The separate
`134950` sorted-cluster indirection is not used here. Each entry marks its
authored cluster and records its entry ordinal even when the cluster has no
geometry. A nonzero cluster geometry count appends the authored cluster ID,
flags, zero auxiliary stream word and association `FFFF` to the BSP queue.

For an instance with nonempty geometry, `132E60` calls the exact `165010`
consumer against that constructed entry's ordered group/frustum ranges. A
visible intersecting sphere is promoted to contained when context `+2A78` is
zero and radius is at most **2.5**, inclusive. An outside sphere is not promoted.
The original source sphere and its actual emitted frustums are used.

After the first successful queue append, later references retain that record's
side/containment/auxiliary flags. If it is not contained, they OR their
constructed entry bit into its association group. They also OR fog flag `0400`
when the later referencing authored cluster belongs to the active fog. Thus
the result is neither a single containing owner nor an OR over every authored
membership regardless of frustum admission.

## Queue limits, flags and overflow

Untouched `131FF0→134980` establishes the main six-short initializer tuple
`[1,128,64,256,384,0]`. The BSP queue capacity is 128 and instance capacity is
384. `134390` reserves the last slot, admitting at most 127 and 383 records.
The auxiliary context's tuple is `[6,64,1,128,96,256]`; its BSP/instance capacities
are 64/96. The native defaults use the proven main values. Explicit alternative
capacities are available for other caller contexts and saturation fixtures.

The main verified context uses mode 0, flags 0, context `+2A78=0`, and auxiliary
pointer `NULL`. Queue records preserve the source flags:

| Flag | Meaning at this seam |
| --- | --- |
| `0400` | Authored cluster belongs to active fog, including later-instance OR |
| `0800` | Instance contained result; association index remains `FFFF` |
| `1000` / `2000` | `132E60` auxiliary outputs, both true for auxiliary `NULL` |
| `4000` / `8000` | Positive/negative optional clipping-plane sides |

Optional isolated `134390` branches `20` (skip), `80` (force otherwise rejected
instance admission) and `200` (clipping plane) are independently tested. The
`132B80` dispatcher may choose another constructor mode for flag `80`; these
pure branch fixtures do not authorize using a mode-0 constructor for that live
dispatcher state. Mode1 accepts the actual prior cluster/instance queues for
flag1's `163D20` membership. Other flags and mode2 remain rejected.

For mode1, sphere overlap follows the original SSE `z²+x²+y²` order and
containment follows x87 PC53 `z²+y²+x²`; the main-view 2.5 promotion does not
apply. A missing prior cluster with nonempty geometry skips that cluster's
instance walk too. Empty-geometry clusters still walk their instance lists.
An admitted instance records actual prior instance membership in flags
`1000/2000`. A forced outer admission after a sphere-overlap miss preserves
the original early return's default auxiliary outputs.

`source_bsp_sphere_clusters` separately implements `132D60→14A5B0/14A370` with
the source512 traversal capacity. It preserves creation order and the separate
sorted-cluster indirection. It does not call the portal-frustum constructor or
invent frustum ranges. The admission seam currently retains its128 constructed
entry bound; the512 constructor alone does not remove that boundary.

For flag `200`, cluster center/extents follow the original SSE midpoint and
subtraction order. Cluster sphere radius uses x87 PC53 squared sum/root in
`z²+y²+x²` order; plane distance uses float
`(plane.z*center.z + plane.y*center.y) + center.x*plane.x - plane.d`.
Instance classification uses its authored float radius.
Equality belongs to both sides.

`134300` allocates a four-word mask before checking instance queue capacity.
Mask bits index the constructed entry ordinal, not authored cluster ID. There
are 512 association groups. On exhaustion the source clamps the count to 512
and returns index zero without clearing or setting a new mask. Queue-full
instances remain unvisited, so later references can allocate more groups even
though no record was appended. The helper retains those exact masks and queue
prefixes. Its capacity diagnostic is not an invented original flag.

## Exact containing-fog producer

`SourceBspFogAdmissionInput` keeps four source inputs distinct:

- Actual scenario pointer presence `4E0350` and current BSP presence `4E0348`.
- Active datum `4B9F8C`, render fog bank `4B9EF0+9C`.
- Explicit `4E64C8` flags, whose bit 4 overrides authored membership.

`12E5E0` initializes the fog datum to `NONE`; `12FEA0` selects it from the
camera cluster's visible/authored plane or palette, subject to the authored fog
definition flags. `130DE0` may move submerged density into an atmospheric bank
but preserves the datum. `2BA10` copies the full 72-word state to `4B9EF0`.
`BspFogSample.planar_datum` represents that retained `+9C` value.

`16E210` returns false when either active pointer is absent or cluster is
`NONE`. With override bit 4 set it returns true before reading a designator.
Otherwise it reads signed cluster `+70`: `-1` is no membership. With active
datum `NONE`, every other designator belongs without dereferencing a palette.
With an actual datum, a negative designator indexes BSP fog planes using its
low seven bits and reads the signed palette short at plane `+0`; positive
designators directly name the palette. Palette `NONE` returns false. Scenario
palette `+8` must equal the actual active datum.

The native atmosphere already retains these literal signed designators,
fog-plane palette indices and scenario datum fields. The new helper includes
the pointer-presence and forced-membership branches rather than assuming
false for every cluster. `BspAtmosphere.scenario_flags` is an authored scenario
field and is **not** substituted for `4E64C8`. The executable's only direct
`4E64C8` reference is this read; a nonzero live override producer has not been
established. It remains an explicit caller input. Zero means the override is
disabled, not that authored fog membership is disabled.

## Independent verification

`tools/differential_source_bsp_admission.py` executes untouched `134390` and
every pure admission/fog callee with no hooks. It separately executes
`131FF0→134980`, supplying only external memory allocation services, and reads
the actual constructed queue capacities. All cache cases first execute the
untouched `441B0→173130` constructor with explicit fixture camera fields and
an all-open portal mask, then feed its actual ranges/frustums to `134390`.

The production-linked comparison passes **2,865 cases / 654,123 words bit
exactly**, including every one of the 721 authored cluster inputs across all
21 maps. Synthetic cases cover all six groups, randomized construction order,
empty geometry, duplicate references, zero group/range inputs, radius endpoints,
128-entry masks spanning all four words, queue truncation and 512-group overflow.
The frozen fixture retains 369 cases; a separate all-21 asset fixture compares
every decoded field, sphere word and ordered membership byte independently.

`tools/differential_source_bsp_fog_admission.py` executes untouched `16E210`
without any hooks. It passes **3,032 cases / 536,436 results bit exactly**,
including all 256 signed-byte designators, negative-plane palette aliases,
missing-palette sentinel, active datum matches/misses/`NONE`, absent pointers,
override and unrelated flag bits. The native atmosphere fields independently
match actual cache bytes on all 21 maps. Its frozen fixture has 1,112 cases.

Both production-linked CTest targets pass regular and address/undefined
sanitizer builds with leak detection enabled, with no diagnostics. Reports and
native/source hashes are retained in
`analysis/multiplayer-shading/bsp-admission-differential.json` and
`bsp-fog-admission-differential.json`; final logs use the `*-final-oracle.log`
suffix. Floating-point execution is explicit x87 `027F` / MXCSR `1F80`.

The new sphere/prior-queue comparison independently passes1,120 cases and
215,996 words against unchanged `134390/132E60/163D20/134300/16E210`.
`132D60` also passes119 whole original calls over the retained Lockout BSP.
Reports are in `analysis/multiplayer-shading/world-effects-goal-20261011/`
(`light-sphere-admission-oracle-v3.json` and
`light-sphere-clusters-oracle/report.json`). These additions pass focused
regular tests; the historical sanitizer claim above does not cover them.

## Coverage and remaining services

| Map | Clusters | Instances | Definitions | Memberships | Fog planes | Fog palettes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ascension | 7 | 66 | 19 | 66 | 1 | 1 |
| beavercreek | 33 | 12 | 7 | 19 | 1 | 0 |
| burial_mounds | 27 | 45 | 9 | 54 | 0 | 0 |
| coagulation | 48 | 63 | 22 | 89 | 0 | 0 |
| colossus | 34 | 51 | 24 | 75 | 0 | 0 |
| cyclotron | 31 | 187 | 45 | 238 | 1 | 1 |
| foundation | 19 | 29 | 14 | 49 | 0 | 0 |
| headlong | 65 | 137 | 32 | 155 | 1 | 1 |
| lockout | 16 | 75 | 13 | 90 | 1 | 1 |
| midship | 11 | 14 | 14 | 14 | 0 | 0 |
| waterworks | 56 | 146 | 14 | 154 | 1 | 1 |
| zanzibar | 55 | 62 | 19 | 73 | 1 | 1 |
| containment | 49 | 161 | 21 | 196 | 0 | 0 |
| warlock | 34 | 91 | 57 | 174 | 0 | 0 |
| deltatap | 21 | 78 | 46 | 92 | 0 | 0 |
| turf | 53 | 214 | 63 | 296 | 0 | 0 |
| backwash | 43 | 15 | 11 | 29 | 1 | 3 |
| dune | 24 | 48 | 26 | 62 | 1 | 1 |
| elongation | 42 | 167 | 28 | 268 | 0 | 0 |
| gemini | 25 | 88 | 35 | 102 | 0 | 1 |
| triplicate | 28 | 97 | 37 | 129 | 0 | 0 |

`geometry_visible` selects actual queue admission for authored BSP cluster
encoding `>=0` and instance encoding `-2-instance`. `geometry_contains_fog`
reads the source record's retained flag. Unowned/dynamic encoding `-1` remains
an explicit boundary: it is admitted by that adapter and does not receive BSP
fog membership. Source dynamic-object admission `132220`, auxiliary admission,
later per-part clipping/sorting, and material raster/HUD state are not covered
by this service. The all-open Gemini fixture is an explicit test input, not a
claim that its eight dynamic door portals are always open during gameplay.
