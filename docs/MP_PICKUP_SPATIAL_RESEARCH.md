# Original pickup spatial registry and Lockout terrain visibility

Ordinary weapon pickup can use the original registered world-weapon identities,
their source-produced spheres and the current BSP's real terrain intersections.
The executed pickup visibility masks exclude registered-object and Havok shape
tests. A supplied clear collision result is unnecessary for this terrain path.
The unit's animated root pose, dynamic shapes and full item settling remain
separate boundaries.

This note extends the frozen [controls and action contract](MP_PICKUP_CONTROLS_RESEARCH.md),
[world allocation contract](MP_ITEM_WORLD_RESEARCH.md) and
[inventory contract](MP_INVENTORY_RESEARCH.md). It does not change production.

## Authority and reproduction

The untouched title update is
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The actual cache is `assets/disc/maps/lockout.map`, SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
Original machine instructions own the calling conventions and decisions;
decompiler exports and Assembly's metadata schema are supporting references.

Run:

```sh
.tools/python/bin/python tools/oracle_mp_inventory_spatial_1_1.py
```

The [oracle](../tools/oracle_mp_inventory_spatial_1_1.py) writes a separate ignored
[report](../analysis/multiplayer/integration/inventory-source/spatial/report.json)
and instruction directory. It executes 599 complete routine calls or bounded
fragments, 207 visibility calls and 1,506 original collision segments. Its report
retains all 66 authored start records, root-pose inputs, original query outputs,
registry links, cache surface/plane/material bytes and explicitly used services.
The separate [audit](../analysis/multiplayer/integration/inventory-source/spatial/audit.json)
checks instruction bytes, metadata provenance, hashes and documentation links.
Earlier evidence is preserved.

The source default loadout `1A28A0` creates and carries SMG datum `00010002`.
Full authored allocation `15C110→B8A30` then creates BR rows 0 and 1 as
`00020003` and `00030004`, and SMG row 12 as `00040005`. These are original
allocator results, including salts; they are not definition substitutions or
synthetic pickup candidates. The heap leaf supplies backing memory, while
original object/registry allocators own identity, bitmap and links.
[Loadout](../analysis/multiplayer/integration/inventory-source/spatial/instructions/001a28a0.asm),
[authored allocation](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0015c110.asm),
[datum allocator](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0016d2c0.asm).

## Sphere and registry ownership

`BEDE0` transforms definition center floats `+8/+C/+10` through the first runtime
node matrix at object-relative signed offset `+116`, writes bound center
`object+30`, and writes bound radius `object+3C = object.scale(+A0) ×
definition.radius(+4)`. It also produces the registration sphere at `+40..+4C`,
using the definition's secondary sphere only when its source flag selects that
sphere. The registration payload is copied from this sphere; it must not be
replaced by a native movement capsule or the weapon root position.
[Sphere producer](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000bede0.asm),
[payload producer](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000b97d0.asm).

Lockout `masterchief_mp` supplies bound radius `3EE147AE` (approximately 0.44)
and center `[-0.02,0,0]`. For authored start 0, root
`[7.0335698,-0.134619,-8.2790098]` produces bound center
`[7.0419626,-0.1164652,-8.2790098]` through the original basis/matrix math.
The original BSP point locator returns leaf 15 / cluster 1 / BSP 0.
BR's initial world radius is `3E828F5D` (approximately 0.255), and SMG's is
`3E3851EB` (approximately 0.18). Their transformed centers and world scales are
retained in `allocated_world_objects`; these initial allocation poses are not
claimed to be fully settled live poses.
[Root basis](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0011f680.asm),
[matrix](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000beb30.asm),
[location](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0011d4a0.asm),
[allocation/physical limits](MP_ITEM_WORLD_RESEARCH.md).

`B94F0` stores source location `+28` (leaf i32, cluster i16 `+2C`, BSP i16
`+2E`) and header cluster i16 `+4`. Runtime object flag `+4 bit9` chooses the
collidable registry triplet at `4E58B0`; otherwise it chooses `4E58A4`.
Each triplet contains heads pointer `+0`, cluster→object pool pointer `+4`, and
object→cluster pool pointer `+8`. The fixture supplies empty backing arrays and
pool metadata; original `1CE650→14BD00/16D2C0` creates the entries.
[Registration](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000b94f0.asm),
[membership allocation](../analysis/multiplayer/integration/inventory-source/spatial/instructions/001ce650.asm).

| Record | Stride | Source fields |
| --- | --- | --- |
| Cluster→object | `20` | allocated datum state `+0`; full object datum `+4`; next link datum `+8`; copied payload `+C` |
| Spatial payload | `14` | type byte `+0`; source collision flags short `+2`; center floats `+4/+8/+C`; radius float `+10` |
| Object→cluster | `C` | allocated datum state `+0`; cluster i32 `+4`; next link datum `+8` |
| Object membership | — | reverse-link head datum `object+60` |
| Pool | — | stride `+24`, records pointer `+44`, bitmap pointer `+48` |

New forward records prepend to their cluster head. Moving an already registered
object requires removing old memberships: `BEDE0` calls `B96A0→1CE830` before
writing a changed sphere and re-registering through `B94F0`. A bare repeated
`B94F0` call is not a substitute for that removal order. The order-isolation
cases explicitly execute the original unregister/register pair.
[Sphere update](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000bede0.asm),
[unregister](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000b96a0.asm),
[unlink/free](../analysis/multiplayer/integration/inventory-source/spatial/instructions/001ce830.asm).

Initial source records place both BR objects in cluster 9, in order
`[00030004,00020003]`, and SMG `00040005` in cluster 15. Relocating BR row 0 to
actual authored portal 22's center is an explicit pose input; original bounds
and registration then create memberships in clusters 9 and 15. The broad query
returns that full datum once. This exercises source duplicate suppression with
real entries in both adjacent clusters.
[Execution report](../analysis/multiplayer/integration/inventory-source/spatial/report.json).

## Original enumeration and eligibility

The complete `BBF40` interface is:

```text
i16 BBF40(u32 registry_mask, u32 object_type_mask,
          location8*, center3f*, float query_radius,
          full_datum_out[], i16 maximum)
```

Zero masks expand to all bits. Registry bit 0 scans collidable lists; bit 1
scans noncollidable lists. Invalid location cluster `FFFF` supplies no clusters
and returns zero. Radius `<=0` scans only the current cluster; a positive radius
uses original `14BD00`, clamped to 512 query clusters. Within each cluster,
collidable records precede noncollidable records, and each list follows its
stored head/next order.
[Complete query](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000bbf40.asm),
[collidable head](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000b9830.asm),
[noncollidable head](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000b9900.asm).

`14BD00` performs depth-first traversal in the authored cluster portal-list
order. BSP cluster records are `BSP+A0` pointer, stride `B0`, with portal-count
`+8C` and pointer `+90` to signed-short portal indices. Portal records are
`BSP+60` pointer, stride `24`: adjacent cluster shorts `+0/+2`, plane index `+4`,
center `+8/+C/+10`, radius `+14`, vertex count `+1C`, and vertex pointer `+20`.
`14BAC0` applies strict plane-distance and sphere tests, then projects to the
portal polygon and runs the original containment test. Sorting clusters or
scanning all nearby objects bypasses this original spatial ownership.
[Traversal](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0014bd00.asm),
[portal test](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0014bac0.asm),
[polygon containment](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0023c260.asm).

For the broad radius-100 query from authored start 0, the actual cluster order
is `[1,9,15,13,11,12,4,3,5,6,8,10,7,14,0,2]`. Initial weapon output is
`[00030004,00020003,00040005]`; maximum 1 returns its first element. A
noncollidable-only or nonweapon-type query returns nothing. Zero-radius,
zero-type-mask, zero-maximum and original outside-BSP location cases also execute
without supplied enumeration results.
[Execution report](../analysis/multiplayer/integration/inventory-source/spatial/report.json).

For each stored record, `BBF40` tests its payload type bit, then marks the object
low index as seen before applying the inclusive sphere test:
`distance²(payload.center,query.center) <= (payload.radius+query.radius)²`.
Query generation `4E58CC` and marks at `4E58D0 + low_index*4` deduplicate objects
across clusters and registries. The output retains the full salted datum and
stops at the caller's maximum; it does not sort by proximity.
[Complete query](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000bbf40.asm).

`1527C0` uses the unit's source bound center and radius. Its first query is
registry mask 0, type mask 4, radius `unit+3C + 0.4`; the second is type mask
`FFFFEBCB`, radius another 3.0 larger. Lockout's standing fixture yields radius
bits `3F570A3E` and `4075C290`. The second pass covers other interaction types;
this fixture registers only the actual world weapons, so it is empty here.
Source player control generation calls this scan before the later pickup action
request. [Candidate scan](../analysis/multiplayer/integration/inventory-source/spatial/instructions/001527c0.asm),
[control/action stage](MP_PICKUP_CONTROLS_RESEARCH.md).

`152E90` rejects held `item+12C bit0` and last holder `item+14C == unit`, then
requires strict bound-center distance below
`unit.radius + 0.3 + (weapon_definition.i16(+290)!=0 ? 0.1 : 0)`.
This narrower test does not add the weapon radius. `CEC00` owns inventory and
animation-capability admission. `152A30` first prefers greater priority, then
strictly closer root positions; exact equal-distance ties preserve the first
enumerated candidate. In the executed coincident BR cases, changing original
registration order changes the chosen full datum accordingly. A rejected
visibility test restores the previous candidate.
[Weapon admission](../analysis/multiplayer/integration/inventory-source/spatial/instructions/00152e90.asm),
[inventory admission](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000cec00.asm),
[ranking](../analysis/multiplayer/integration/inventory-source/spatial/instructions/00152a30.asm).

## Actual terrain visibility

`C57A0` receives target datum in ESI and unit datum in EDI. The geometric branch
requires unit header type bit 0 and target header type in mask `1C`; other types
return true. `E0C20` creates a capsule from the unit root and real `bipd` metadata,
independently of the 0.44 interaction bound radius. For Lockout's standing
fixture, metadata flags `+264 = 4`, standing height `+268 = 0.725`, crouching
height `+26C = 0.5`, and capsule radius `+270 = 0.175`. Bottom is root Z+0.175,
extent is `lerp(standing,crouching,unit+2C4) - 2*radius`, and top is bottom+extent.
The executed crouch field is explicitly zero.
[Visibility](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000c57a0.asm),
[capsule producer](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000e0c20.asm).

`FBFA0` obtains the weapon interaction point. The initial uncontacted items use
their root through `BACC0`. For contact flag `item+12C bit5`, it attempts an
original marker query and terrain-plane offset, then falls back to the root
when that marker is unavailable. The initial allocation sweep does not recover
animated/contact marker availability or claim that fallback is the live pose.
[Interaction point](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000fbfa0.asm),
[root producer](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000bacc0.asm).

The original visibility sequence is top→weapon, then bottom→weapon, both mask
`14800001`, ignoring the unit and target datums. A clear cast accepts immediately.
If both hit, the source normalizes their horizontal perpendicular (with its
original zero-length fallback), creates unit side points at ±0.075 and target
side points at ±0.05, clips two target-side points with mask `24800001`, then
tests four corresponding top/bottom side segments with `14800001`. It rejects
only if all four remain blocked. The original `16BAC0` wrapper computes each
endpoint delta; no collision return is supplied.
[Visibility sequence](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000c57a0.asm),
[segment wrapper](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0016bac0.asm).

Both masks have low bits `&E == 0`, so full `16B240` executes its terrain
`1E2020→1E20C0/1E23F0` path and bypasses Havok/registered-object branches.
All 1,506 segments use the actual mapped Lockout collision BSP. The report
retains every hit fraction, point, normal, leaf, surface, plane and material.
For all 1,112 terrain hits, the captured surface, plane and collision-material
records are byte-checked against the cache, including their file offsets.
[Collision dispatcher](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0016b240.asm),
[BSP traversal](../analysis/multiplayer/integration/inventory-source/spatial/instructions/001e20c0.asm),
[surface admission](../analysis/multiplayer/integration/inventory-source/spatial/instructions/001e23f0.asm).

### Raw terrain input/output contract

The scenario BSP-reference block is `scnr+210`, count/pointer, stride `44`.
Its first record contains file offset `+0 = EC0E00`, byte length `+4 = 9A000`,
and mapped base `+8 = 800AB000`. The chunk header's pointer at file `EC0E04`
selects BSP root `800AB010`. Pointers inside this chunk convert to file offsets
with **BSP magic `7F1EA200`**, rather than the main tag metadata magic
`7E42C800`. Root `+14/+18` is the ordinary collision-BSP count/pointer pair,
stride `40`; Lockout's one ordinary collision record is `800AB3C8`, file
`EC11C8`. Collision-material count/pointer is root `+C/+10`, stride `14`, with
global material i16 at record `+8`. The report retains the raw block bytes and
their verified file offsets.
[Mapped source data](../analysis/multiplayer/integration/inventory-source/spatial/report.json),
[sbsp schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/sbsp.xml).

Every collision sub-block uses a u32 count and u32 pointer, with the pointer four
bytes after the listed descriptor offset:

| Ordinary collision descriptor offset | Block | Stride | Lockout count | File offset |
| --- | --- | --- | ---: | --- |
| `00` | 3D nodes | `8` | 4388 | `EC1208` |
| `08` | Planes | `10` | 1331 | `EC9B30` |
| `10` | Collision leaves | `4` | 2313 | `ECEE60` |
| `18` | 2D references | `4` | 5989 | `ED1284` |
| `20` | 2D nodes | `10` | 4020 | `ED7020` |
| `28` | Surfaces | `8` | 3162 | `EE6B60` |
| `30` | Edges | `C` | 5947 | `EECE30` |
| `38` | Vertices | `10` | 2587 | `EFE500` |

Retain the original packed 24-bit 3D children, authored 2D references and edge
graph. This is the same original `1E20C0/1E23F0/29C7D0/1E25C0` family already
recovered for Spartan model collision, applied here to the ordinary sbsp record.
The [existing raw traversal contract](SPARTAN_COLLISION_1_1_RESEARCH.md#3d-traversal-and-fraction-boundaries)
records its exact float operation order and class transitions. The different
descriptor inputs below belong to the terrain wrapper; a generic BVH return
does not establish this behavior.

`1E2020` receives lower flags in ECX, the raw collision record pointer in EDX,
lower output pointer in EAX, and initial best fraction in XMM0. Its four stack
arguments are breakable-count i16, breakable-bitset pointer, origin pointer and
delta pointer; it returns with stack cleanup `10`. It constructs this 40-byte
descriptor and calls `1E20C0(descriptor*, root_node, t_min, t_max)`:

| Lower descriptor offset | Input/state |
| --- | --- |
| `00` | u32 lower flags |
| `04` | Raw collision-BSP pointer |
| `08` | i16 breakable-bit count; `+A` is unused padding |
| `0C` | Optional breakable-bitset pointer |
| `10`, `14` | Origin and delta pointers |
| `18` | Lower output pointer |
| `1C` | Prior leaf i32 |
| `20` | Prior leaf-class byte; following bytes are padding |
| `24` | Prior crossing-plane i32 |

All 1,506 full visibility segments capture root node 0, range
`[00000000,3F800000]` = [0,1], initial best fraction **`7F7FFFFF`**, prior leaf
and plane `FFFFFFFF`, and prior class 0. Initial best fraction is maximum finite
float, not 1.0, even though the interval ends at 1.0. Outer mask `14800001`
passes through original `16A560` to lower flags `29`; side clipping mask
`24800001` gives lower flags `9`. Preserve these descriptor values when reusing
the raw traversal.
[Wrapper](../analysis/multiplayer/integration/inventory-source/spatial/instructions/001e2020.asm),
[mask conversion](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0016a560.asm),
[captured descriptors](../analysis/multiplayer/integration/inventory-source/spatial/report.json).

The breakable-count input is `100` (256), and the pointer is
`*[4F4850] + 1 + current_bsp*20`. Backing storage is a fixture, but original
full-reset fragment `15B670..15B680` fills its `4201` bytes with FF; thus all
breakable bits are intact in this bounded initial-world fixture. The surrounding
full-reset lifecycle/option branch is excluded. This runtime bitset is distinct
from authored surface flags and must be an explicit runtime input if destruction
is later supported. Surface-edge admission reaches `1E25C0` in this real terrain
sweep, so its optional bitset must not be left as an invalid pointer.
[Source reset](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0015b460.asm),
[edge/bitset admission](../analysis/multiplayer/integration/inventory-source/spatial/instructions/001e25c0.asm).

`1E20C0` returns hit in AL and writes a `420`-byte lower result:

| Lower result offset | Output |
| --- | --- |
| `00` | Float fraction |
| `04` | Selected source plane pointer |
| `08`, `0C` | Candidate leaf and surface indices, i32 |
| `10` | Sign-extended surface plane index, i32 |
| `14`, `15`, `16` | Surface flags byte, breakable-index byte, BSP-material i16 |
| `18` | Visited nonempty leaf count, maximum 256 |
| `1C` | 256 visited-leaf i32 slots |
| `418` | Overflow leaf overwrites the final visited-leaf slot; original store at `1E23D6` |

Only initialized/successful fields should be consumed; a clear result does not
make stale hit fields valid. Full `16B240` constructs the outer `5C`-byte result:
type u32 `+0` (1 for ordinary terrain), fraction float `+4`, point `+8`, initial
and final location8 `+14/+1C`, global material i16 `+24`, oriented plane4f `+28`,
then lower leaf/surface/signed-plane `+4C/+50/+54` and flags/breakable/material
`+58/+59/+5A`. Its terrain owner/instance/object identities `+38/+3C/+40` are
NONE. It also computes visited-leaf locations and contact orientation; the raw
lower result is not already this outer result.
[Raw traversal](../analysis/multiplayer/integration/inventory-source/spatial/instructions/001e20c0.asm),
[outer assembly](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0016b240.asm).

For side clipping, outer bit `20000000` can invoke source endpoint correction
when the returned point's BSP leaf disagrees with its recorded final leaf.
`16B240` first offsets by oriented normal × original `4652EC = 39800000`
(0.000244140625), and can retreat along the segment using its source loop and
`4652E8 = 3D000000` (0.03125) fallback. Preserve that wrapper behavior if a
clipped point is fed back into `C57A0`; the oracle retains both lower and final
outer records for all 372 such clipping segments.
[Endpoint correction](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0016b240.asm).

This ordinary terrain block is separate from sbsp instanced-geometry definitions
at root `+138/+13C` and instance records `+140/+144`. `16B240` mask value 1 selects
ordinary terrain; value 2 selects its separate cluster-plane branch; value 4
selects the runtime cluster-shape/Havok path `246A50`; value 8 selects registered
objects through `B9830→16AF90`. The executed pickup masks only select ordinary
terrain, so they neither prove nor require recovery of those other collision
branches. Full item-world collision masks do include other branches, which
remain explicitly unresolved in the [world report](MP_ITEM_WORLD_RESEARCH.md).
[Dispatcher](../analysis/multiplayer/integration/inventory-source/spatial/instructions/0016b240.asm),
[runtime cluster shapes](../analysis/multiplayer/integration/inventory-source/spatial/instructions/00246a50.asm),
[sbsp instance schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/sbsp.xml).

For authored start 0→BR row 1, the original eight casts reject visibility.
The first segment hits leaf 23 / surface 1194 / plane 159, global material 104,
with fraction bits `3F26560C`, point Z `C0D00002` (approximately -6.500001), and
normal approximately `[0,0,-1]`. The bottom cast hits the same ceiling with
fraction bits `3F352BD8`. These are actual cache contacts, not a supplied floor
or blocking predicate. Across the 198 direct authored-start/weapon visibility
checks, BR row 0 is visible from four starts, BR row 1 from one, and SMG row 12
from eight. The direct sweep deliberately bypasses pickup distance; it is not
an interaction availability count.
[Execution report](../analysis/multiplayer/integration/inventory-source/spatial/report.json).

All 66 authored starts produce no ordinary pickup candidate because these three
rows are outside the narrow distance gate. Explicit nearby root inputs do
produce the actual source identities: both BRs yield priority 1 / flags 1,
and SMG yields priority 1 / flags 4. With the initial SMG still at full ammo,
the latter is secondary-hand admission (`CEC00` bytes `[0,1,0,0]`), not a
primary-hand replacement. Its animation capability is a named supplied service;
this result does not prove a complete live dual-wield animation/action path.
BR admission bytes are `[1,0,1,0]`. The shared weapon RNG remains `12345678`
through allocation, queries, visibility and order cases.
[Execution report](../analysis/multiplayer/integration/inventory-source/spatial/report.json),
[admission](../analysis/multiplayer/integration/inventory-source/spatial/instructions/000cec00.asm).

## Integration boundary

The implementable weapon slice is source registry membership and insertion
order, actual sphere/portal queries, stable salted candidate identity, source
distance/ranking and this BSP-only visibility sequence. Its inputs include the
current source unit pose and crouch value, current weapon pose and marker state,
real definitions, the current collision BSP, and source-created world records.
It composes with the already recovered controls and type-20 inventory request;
the candidate's datum must remain the datum later carried or replaced.
[Controls](MP_PICKUP_CONTROLS_RESEARCH.md), [world state](MP_ITEM_WORLD_RESEARCH.md).

This bounded registry contains only the three allocated world weapons. The unit
root/node matrix is a supplied pose input; full unit allocation, animated poses,
unit/nonweapon registration and their second-pass interactions are excluded.
The active-cluster mask, heap backing, marker/animation/audio leaves are explicit.
Havok remains unresolved for item motion and full world shape queries, although
it is not reached by these terrain-only visibility masks. Initial row poses
remain initial allocation poses; the frozen world report owns the bounded first
item tick, and neither report establishes complete physical settling or a live
Xbox game. These limits should remain visible at the native API boundary.
