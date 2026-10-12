# Original object location, spatial membership and admission

The native services reproduce the ordinary main-view object path
`132220→133050→132E60→165010/181800`, using actual source portal-frustum outputs
and ordered object-to-cluster lists. The authority is the supplied Xbox
`assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The independent comparisons execute that unchanged executable. These services
establish CPU producer/consumer contracts; complete live object registration
and framebuffer equivalence remain separate requirements. Independent raw
spatial-pool allocation and pressure handling are implemented separately in
`source_object_spatial_pool`.

## The three sphere fields have different consumers

| Original object fields | Consumer |
| --- | --- |
| `+30/+3C` | Retained rendering/light query sphere; `133050` frustum admission |
| `+40/+4C` | Retained spatial primitive; `BF510`, `B8600`, `14A5B0` membership |
| `+50/+5C` | Retained dynamic bound cache |

`BDEF0` produces these caches after the authored node/world transform and
bounding-offset query. A render-mesh centroid or containing cluster does not
replace any of them. `SourceObjectAdmissionObject` takes the actual `+30/+3C`
rendering cache; the location and spatial-list APIs take `+40/+4C` separately.

The new read-only cold observations execute the original constructor prefixes
for all **278 SCEN, 49 MACH and 415 BLOC** placements. They add raw `+40/+4C`,
`+50/+5C`, `+28/+2C` and `+64` observations to the existing scripts in memory;
the existing scripts and reports are not changed. The scripts supply successful
storage allocation and resident authored animation resources and stop before
`B8890`. There is no selected BSP, registration is disabled, and each lifetime
is reset independently with fixture handle `12340000`.

All 742 objects begin with location words `FFFFFFFF/FFFFFFFF` and registered
flag8 clear. The existing native `BDEF0` sphere selector, given the independently
captured original query sphere/scale/definition fields, matches all primitive
and dynamic result words: **742 cases / 6,678 words**. This checks the selector;
it does not independently re-prove the native posed-node query input or a later
physical backend override. No `1D44F0` physical-bounds call occurs in these cold
prefixes. The parallel node/world-matrix proof is retained separately.

## Exact location producer

`source_object_location.hpp/.cpp` implements four distinct services:

- `source_object_point_location`: `11BED0→14A280` with the actual collision
  nodes, planes, render-leaf mapping and selected BSP index.
- `source_object_sphere_leaves`: the independent third output bank of
  `1DDE10→1DDEA0`; it retains the first 256 ordered collision-leaf IDs, including
  duplicate leaf references. It does not claim the contact/surface/edge output
  banks or the function's contact return status.
- `source_object_initial_location`: normal `BF510`, querying primitive center
  first, then the first sphere leaf when the center has no cluster, then
  object `+64` only if the sphere-leaf bank is empty.
- `source_object_default_registration_location`: `B8600`'s separate NULL
  supplied-location branch, primitive-center point followed by root-position
  point if the first result has no cluster.

The point classifier uses SSE distance
`((z*pz + y*py) + x*px) - d`, choosing the positive child on a plane boundary.
The sphere classifier follows the literal inequalities: negative only at
`distance <= 0-radius`; negative then positive at `distance < radius`; positive
otherwise. The original scratch bound is 128 simultaneous straddle planes,
not 128 total node depth. The regression includes a 320-node single-side chain.

The complete untouched original `BF510` executes its actual sphere/contact
callees during the oracle, rather than substituting a location service. All
742 authored cold inputs resolve both initial and default-registration
locations. Primitive-center point alone resolves 721; 16 SCEN, one MACH and
four BLOC require the source fallback. A caller must keep the historical
`+28/+2C` fields when `B8600` gets clusterNONE, exactly as the original does.
The helper does not invent an old location or a registration history.

## Spatial membership and PC53 precision

`source_object_light_spatial_membership` follows ordered `14A5B0` DFS over
authored portal lists, with exact `14A370→23A160` sphere/portal classification.
Its 256-entry storage preserves the prefix while the source count can continue
beyond 256. Explicit forced membership follows `14A550` ascending cluster-bit
order. Neither path substitutes a centroid or unordered set.

The source portal test evaluates its x87 plane distance and disk square root
with precision-control **53 bits** (`FPCW027F`), storing intermediate values as
single precision at the original boundaries. Two former host `long double`
expressions now use `double` to preserve that precision. The adversarial
fixture with plane `(0.707106769,0.707106769,1e-10,0)`, center
`(1e9,-1e9,1)`, radius `1e-10` is admitted by untouched PC53 but rejected by
untouched PC64 and the former wider native expression. This is an independently
measured arithmetic correction, not an epsilon adjustment.

`ObjectLightSpatialLists` preserves full handles, head insertion, removal and
update chronology. The previous untouched list oracle covers 4,096 explicit
events / 8,192 ordered checks against `1CAC60`, `1CADF0`, `2962E0`, `1CAE40`,
`B8940` and `B89B0`. Its successful allocation/free services are explicit.
It does not prove behavior under allocator pressure or infer creation order.

The additional `SourceObjectSpatialPool` overload consumes the literal cluster
heads and records directly, preserving orphan entries after independent
reference-allocation failures. It runs the same verified ordinary admission
algorithm without constructing successful-only lists. All329 frozen original
queue cases match through both storage services; each fixture verifies its
successful membership count stays inside both actual2048-record banks.
The independent raw-pool and journal proofs, including real restored Zanzibar
pool states, are recorded in `SOURCE_OBJECT_LIVE_REGISTRATION_SEAM.md`.

## Ordinary visibility queue

`source_object_admission.hpp/.cpp` visits the constructed cluster entries in
their actual creation order. Within each entry it walks the collidable list
first, then the noncollidable list, preserving source head order. Full handles
select the live registry; the source visited epoch is indexed by low16 slot.
Repeated references reuse the first successfully appended record and OR the
actual entry-ordinal association bit, fog membership and sky flag. Unregistered
objects are absent.

The source `132E60` uses the actual retained rendering sphere against the
entry's ordered group/frustum ranges. A visible sphere of radius at most 2.5
is promoted to contained in the verified context `+2A78=0`; outside spheres
are not promoted. Source object flag31 allows a non-contained object to
allocate a four-word association mask. The returned flags preserve clipping
sides (`4000/8000`), auxiliaryNULL (`1000/2000`), containment (`0800`), containing
fog (`0400`) and optional sky bypass (`0200`).

The main ordinary queue's capacity is 256 and it reserves the last slot,
admitting at most 255 records. Association masks share the BSP admission bank,
so its already allocated masks are passed in and retained. `134300` allocates
before checking object queue capacity. A full queue leaves the owner unvisited
and retains allocated masks; a later reference can allocate again. On 512-mask
exhaustion, the source returns mask index0 without clearing or setting a new
mask. The native helper retains those prefixes and masks.

The verified context is mode0, flags0, `+2A78=0`, auxiliaryNULL. Optional pure
branches are explicit: `40` skips ordinary objects, `80` forces admission,
`200` enables clipping, and `400` filters ordinary collidables to actual type7.
It does not filter noncollidables or the separate sky bypass. Flag10 belongs
to separate light-volume work. An explicit sky cluster can enable its ordinary
sky bypass; the unsupported negative-ordinal association-memory write is
rejected. No unproved live sky-owner availability is inferred.

## Actual creation and registration chronology

The source has two different passes, confirmed by the literal executable call
sites and callback tables:

1. `18F000` calls `137CA0`, then `D5560(0)`, then `C09C0`, `137E40` and
   `11C1B0(activeBSP,1)`. `D5560` attempts authored types0 through12 in order,
   and placement indices ascending within each type, subject to `D59F0`.
   `BF0F0` still performs factory/name/resource admission and actual allocation;
   `BF380` follows each attempted factory call.
2. During BSP activation `137D40` clears its three 16-word banks, then calls
   callback-table `440DE8` entries in increasing `0x24` (36-byte) stride. Object callback
   **index27**, table offset `3CC`, is `B6BB0`.
3. `B6BB0` enables registration (`4DE2F4` byte0), then runs untouched `BAEB0`
   with mask `FFFFFFFF`, required header flags0, initial cursor0. This visits
   actual nonzero-generation headers in **ascending live pool-slot order**.
   The object header bank has stride12; a full handle is signed salt16 shifted
   left16 OR slot. It is not a global scenario-content index.
4. For each root (`object+14 == NONE`) with object flag7 clear, `B6BB0` calls
   `BF510` and then `B8600` with that supplied location, even if `BF510` returned
   false. It then runs `108960`'s type inheritance activation callbacks. The
   literal type6 callback is `10A030→10A820`, assigning resource field `+134`;
   type7 and type11 inheritance activation callbacks are NULL.
5. `B8600` preserves old location on clusterNONE, chooses collidable versus
   noncollidable from actual object flag9, builds the primitive from `+40/+4C`,
   calls `1CAC60` and sets object registered flag8/header flag40. Thus ascending
   registration produces reverse head order within each cluster. Flag21 can
   request the source explicit all-cluster mask. `B87B0→1CAE40` removes those
   full-handle memberships and clears registration flags, retaining location.

The cold constructor's registration branch is different from BSP activation:
`B7B40` first calls `BD020`; registration happens only when the object global
is present and byte0 is enabled. It uses a supplied seed location when present;
otherwise it calls `BF510`, passes that location on success, and passes NULL to
`B8600` on failure. The 742 cold harnesses intentionally disable this branch.

Literal pool constructors `B67C0` and `1CABC0→296270` set **2,048 object
headers**, **2,048 owner records and 2,048 membership records per spatial bank**,
with 512 authored cluster heads. Owner record stride is32 (20-byte primitive
plus12-byte header), membership record stride12. The owner and membership
allocations in `1CAC60` are independent; unsuccessful allocation can leave a
partial registration. Pool recycling and callbacks can change later order.

`SourceObjectInitialization` is a construction ledger. Its verified attempt
ordering, primary RNG consumption and caller-supplied full handles do not by
themselves establish the actual successful source pool-slot sequence. The
ordinary visibility adapter must receive a real registered registry and event
chronology. It must not sort or synthesize links from scenario global indices,
containing clusters or the independently reset cold fixtures. Full authored
factory/child allocation and registration history remain explicit integration
boundaries until supplied by a source-backed live lifetime producer.

## Independent checks and artifacts

| Untouched original service | Cases / compared outputs | Result |
| --- | --- | --- |
| Ordinary queue and original list construction | 2,104 / 189,568 words | Bit exact |
| Synthetic PC53/capacity membership | 519 / 17,850 words | Bit exact |
| All-21 authored portal tests | 12,232 results | Exact |
| All-21 ordered membership | 5,645 cases | Exact |
| Existing segment/polygon service | 8,192 cases | Exact |
| All-21 authored decoded membership metadata | 343,284 words | Bit exact |
| Complete initial/default location | 1,770 / 18,549 words | Bit exact |
| Cold primitive/dynamic selector | 742 / 6,678 words | Bit exact |

The ordinary queue oracle runs untouched list construction with only successful
`16B840` allocation supplied. The location oracle runs all actual sphere/contact
callees and stops `B8600` at `B8654`, before list/backend registration. Source
FP state is explicit `FPCW027F` / `MXCSR1F80`.

Frozen tests retain 329 queue cases, all742 cold sphere records, 2,023 synthetic
and asset membership cases, and 1,002 location cases. All four focused CTest
targets (`retail_object_light_spatial`, `retail_source_object_admission`,
`retail_source_object_membership`, `retail_source_object_location`) pass normal
and ASAN/UBSAN builds with leak detection enabled, with no diagnostics.

Reproducible tools and reports:

- `tools/differential_source_object_admission.py` and
  `analysis/multiplayer-shading/object-admission-differential.json`.
- `tools/differential_source_object_membership_synthetic.py`,
  `tools/differential_source_object_membership.py` and the
  `object-membership-{synthetic,pc53}-original.json` reports.
- `tools/differential_source_object_spatial_cold.py` and separate
  `{scenery,machine,prop}-cold-spatial-original.json` observations.
- `tools/differential_source_object_location.py` and
  `object-location-differential.json`.

Auxiliary `163D20` admission, light-volume `132A30/C3950` services, renderer
subpart admission, negative-index sky association writes and actual live world
registration/pressure chronology are outside the verified seam. Implemented
pool pressure arithmetic does not provide the missing live event producer.
