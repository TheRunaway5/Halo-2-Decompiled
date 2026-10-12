# Live object registration and render collector seam

The source location, primitive, spatial-pool, registration-journal and
object-admission helpers can consume a real ordered native event history. A
current object array cannot reconstruct that history. The original registration
prefix and independent pool lifecycle are implemented and tested, while
successful global factory chronology, physics callbacks and all owner
categories remain separate requirements. This work changes no scene, runtime
or renderer code.

The authority is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The inventory reports read unchanged executable/cache bytes and existing
immutable original observations; their consistency checks are not function
execution oracles. The separate pool/journal oracle below executes untouched
original functions. This agent reads only immutable exports and performs no
live guest reads or actions. The earlier construction-order and pure-service
oracles retain their separate boundaries in `SOURCE_OBJECT_REGISTRATION_PARITY.md`,
`SOURCE_OBJECT_ADMISSION_PARITY.md` and `SOURCE_RENDER_CACHE_PARITY.md`.

## Actual root/child and attachment evidence

`108A90` walks the descriptor's `+84` inheritance table and invokes `+2C`
constructors. `108960` walks the same table for `+20` activation callbacks;
`108B80` invokes `+38` post-constructor callbacks. The actual tables for the
742 previously captured cold prefixes are:

| Type | Constructor chain | Activation chain | Post-constructor chain |
| --- | --- | --- | --- |
| SCEN6 | `10A1B0` | `10A030→10A820` | Base `BE760` |
| MACH7 | `106510→115190` | Empty | Base `BE760` |
| BLOC11 | `11BBF0` | Empty | Base `BE760` |

All these inheritance chains have `+74` node callbacks NULL. This closes that
specific `BE1D0→109140` callback branch, not the rest of pose construction.
The machine chain also has `+48=115A40`, used by interested-cluster activation;
it is unrelated to the `+20` BSP activation callback.

`BE760` reads the actual signed selected variant byte `object+B1`, resolves
the authored `hlmt` variant bank at `+50` (stride56), then its `+1C/+20` child
bank (stride16). A non-NONE child definition calls `B7B40`, then attaches its
successful result through `B8EE0`. This can allocate children between root
factory attempts in the general case. For **all 742 actual selected variants**
in this census, the child bank is empty. Every corresponding object definition
also has zero widgets (`+9C`) and creation-effect datum NONE (`+50`). This is a
bounded source fact about these selected initial variants, not an assumption
that original object factories never create children.

| Map | Captured cold roots | Selected model children | Attachments |
| --- | ---: | ---: | ---: |
| ascension | 7 | 0 | 7 |
| beavercreek | 50 | 0 | 4 |
| burial_mounds | 27 | 0 | 33 |
| coagulation | 6 | 0 | 12 |
| colossus | 10 | 0 | 3 |
| cyclotron | 40 | 0 | 13 |
| foundation | 36 | 0 | 12 |
| headlong | 89 | 0 | 9 |
| lockout | 15 | 0 | 2 |
| midship | 42 | 0 | 26 |
| waterworks | 77 | 0 | 8 |
| zanzibar | 91 | 0 | 12 |
| backwash | 9 | 0 | 18 |
| containment | 24 | 0 | 24 |
| deltatap | 9 | 0 | 9 |
| dune | 26 | 0 | 15 |
| elongation | 47 | 0 | 8 |
| gemini | 35 | 0 | 20 |
| triplicate | 26 | 0 | 24 |
| turf | 55 | 0 | 6 |
| warlock | 21 | 0 | 24 |
| **Total** | **742** | **0** | **289** |

The 289 authored attachment records are distinct from model children:
92 `effe`, 137 `lsnd`, 31 `ligh`, 11 `lens`, 3 `MGS2`, and 15 NONE.
`BECA0` calls `1766B0`, `188F90` and `C0230` for admitted effect, sound and light
references. Successful light/sound allocations set object flag4/5. Lens/MGS2
select attachment enum5 but this switch does not allocate their object.
These services remain necessary for complete effects/light-volume state; an
empty model-child bank does not close them. This table counts records, not
successful attachment allocations.

The independently checked retained paused Zanzibar sample contains 91 raw
SCEN/MACH/BLOC records (27/6/58). All have `parent+14`, `child-head+10` and
`sibling+C` NONE. This directly corroborates root-only hierarchy in that sample.
It does not establish every later attachment or every owner category.

## Exact registration phase and lifetime events

Pre-BSP `D5560` constructor attempts occur in type0..12/placement order. Actual
successful header allocation and intervening allocations still decide pool
slots. `B6BB0` is activation-table row27; the missing automatic placement pass
`D4E30→D5640` is later row57. The intervening callbacks must stay represented.

`B6BB0` enables registration, then `BAEB0` traverses nonzero-generation occupied
headers in ascending actual slot order. The iterator rereads the high-water
limit; callbacks can mutate later slots. For roots (`+14==NONE`) with object
flag7 clear it calls `BF510` and then `B8600` with the returned eight-byte
packet **even when `BF510` reports failure**. It invokes `108960` for every
iterated owner. A sorted final vector cannot replace this event.

Later `B7B40` constructors run with registration already enabled. A valid
request seed supplies its packet; otherwise `BF510` success supplies its
packet and failure passes NULL. `B8600(NULL)` uses point location at primitive
center `+40`, then root position `+64`, with no sphere-leaf fallback. The
initial activation and constructor paths must remain separate operations.

`B8600` preserves old location/header cluster when the supplied cluster is NONE,
sets object flag18, and still attempts membership insertion. A valid packet
writes exactly eight bytes and clears flag18. It chooses the two spatial banks
from actual flag9 and sets object flag8/header bit6 after insertion, including
partial allocation failures. Registration does not require successful
membership insertion.

| Source event | Persistent operation |
| --- | --- |
| Initial BSP activation `B6BB0` | Ordered supplied-packet registration of admitted actual roots, then inheritance callbacks |
| Later constructor `B7B40` | Seeded, initial-packet or NULL registration at the actual constructor event |
| Bound refresh `BDEF0`, unchanged | Retain `+40/+4C` and list order |
| Bound refresh, squared displacement above source `.01` or unequal radius | Unregister; write new primitive; NULL registration, changing head order |
| Transform `B7430` with registered input and `param9==0` | Unregister before admitted transform; optional bound refresh; supplied registration |
| Flag update `B8B70`, registered collidable root | Replace primitive bytes through `1CADF0`; preserve membership/order |
| Unregister `B87B0` | Unlink through `1CAE40`; clear object flag8/header bit6; retain location |
| Destroy `B8460` | Root detach/deactivate/unregister, attachment callbacks, child recursion, physics/type callbacks, then free generation |
| BSP deactivate `B6C50` | Actual ascending slot iteration; remove, callbacks and reset locations; disable registration after traversal |

The source bound-change comparison uses retained primitive `+40/+4C`; render
admission uses the separate `+30/+3C` sphere. The native scene `center()` method
is neither producer and must not supply either sphere by substitution.

## Spatial pool lifecycle and pressure

`B67C0→1CABC0` allocates 512 cluster heads for each bank. `1CABC0` sets
`EBX=0x800`; `296270` passes that unchanged as the count to `16B570`.
Each bank therefore has separate **2048 owner-reference records, stride12** and
**2048 cluster-entry records, stride32** (12-byte link prefix plus20-byte
primitive; the entry pool uses the source alignment flag5).
`B69D0` fills both sets of512 heads with NONE and activates/resets each original
data array through `16B7A0`. `B6AB0` deactivates arrays; `B68C0` releases pointers.

For each ordered membership, `1CAC60` independently tries an owner-reference
allocation and a cluster-entry allocation. Successful owner references prepend
to object `+60`; successful entries prepend to cluster head. The second attempt
still runs if the first fails. Thus pressure can leave an entry with no owner
reference (later removal cannot discover it), or an owner reference with no
entry. A simplified successful-list adapter is valid only under an explicit
no-pressure contract verified against the two real pool counts. A 256-member
cap or a small root count alone does not prove that contract for the complete
global registry.

`ObjectLightSpatialLists` preserves insertion, update and removal under its
documented successful-allocation assumption. `SourceObjectSpatialPool` now
implements the independent pressure outcomes using literal record arrays,
occupancy, allocation cursors, salts, owner reference heads and cluster heads.
The render cache's separate256-record allocator is not this pool.

## Implemented standalone pool and registration journal

`source_object_spatial_pool.hpp/.cpp` contains the raw2048-record data arrays,
both independent spatial banks and `SourceObjectRegistrationJournal`. The
journal consumes actual caller-supplied birth, registration, bound-change,
primitive-update, unregister and free events. Each packet retains the actual
full handle, pool rank, classification fields, three hierarchy links,
eight-byte location, rendering sphere, primitive sphere and root position.
It never discovers event chronology from a current object vector.

The original pool reset `16B7A0`, allocation `16B840`, release `16BA40`, ordered
forced insertion `1CAC60→14A550`, in-place update `1CADF0→2962E0`, removal
`1CAE40` and iterator `B8940/B89B0` execute unchanged in the independent oracle.
The supplied-packet registration prefix executes `B8600` through `B8733`,
stopping before `BEF30`. No tested callee has a fabricated return or patched
instruction. Memberships and lifecycle events remain explicit inputs; this
oracle does not claim a complete successful global constructor history.

The differential covers **10,187 cases / 123,929 exact output words**. It
exercises independent reference/entry exhaustion, orphan entries, reference
records with no entry, high-water/cursor behavior, free/reuse, generation wrap,
debug fills and original registration bits even when both pools are full.
Native composition controls additionally verify literal event order, retained
invalid-location fields, NULL-location fallback, separate rendering-sphere
updates, the source primitive movement threshold, relink order, lifetime reuse
and complete copy/restore. These controls compose proven services; their
expected results are not additional independently captured original events.

Production pool initialization is independently grounded by the unchanged
`B67C0→1CABC0→16B7A0` bytes and all four freshly exported source headers. Entry
names begin `cl`, yielding reset generation `EC63`; collidable reference names
begin `co`, yielding `EF63`; noncollidable references begin `no`, yielding
`EF6E`. Debug fill is `BABABABA`. The fixture checks these reset constants
against unchanged original execution rather than merely supplying native
prefixes as expected values.

Two fixture rows import **both complete restored Zanzibar pools**, including
their512 heads, all2048 records and64 occupancy words, literal header
cursor/high-water/count/generation values and every actual owner reference
head. Untouched original update, delete and reinsertion then produce the
expected raw pool digests and cluster iteration order. The snapshot is
`saved zanzibar-shading-baseline`, restored independently with no resume or
input. Its clock is tick12474, delta bits `3D088889`; CPU registers and clock
are unchanged before/after export. It is **not the later Capture20 frame**.
The report retains all raw input hashes and does not infer the events which
originally created that state.

Malformed native event packets are rejected: duplicate live identities,
ambiguous pool ranks, invalid capacity, a primitive update which changes
registration/bounds, or a free without preceding unregister/child services.
This rejection policy is a native input boundary, not original exception
behavior. A failed mutation is not transactional; callers must reject invalid
inputs before emission or restore a complete checkpoint after an exception.
The journal reports `activation_services_required` after registration/relink.
Attachment lights, interested-cluster activation, physics, authority and
old-NONE transition callbacks remain explicit caller services.

## Raw-pool contact consumer

`source_object_spatial_scan.hpp/.cpp` adds a raw-pool overload of
`source_object_light_spatial_scan`. The existing successful-list overload
remains available. The new overload scans literal collidable cluster heads,
including orphan entries. Each collision leaf's first cluster visit runs its
plane/instance service before object iteration. It preserves source low16
object stamps before filtering, full-handle exclusions, primitive type/flags,
the original segment/sphere predicate and optional first-contact termination.
It does not rebuild the spatial pool from live owner metadata.

The separate oracle executes complete untouched `1697C0`, including `16B500`,
`B8940/B89B0` and `11E5E0`, against **eight raw banks / 4,096 scans / 49,786 exact
output words**. These banks are produced by original independent allocations,
forced registrations and removals. They include233 orphan entries after a
full reference pool,211 surviving orphan entries after removal and additional
partial-entry/reference failures. Ordered callback logs and hit flags match
exactly, including repeated leaves, shared low16 generation indices,
plane-first and object-first early termination controls.

The oracle supplies explicit controlled `1DE630` collision-leaf outputs,
`244980` instance-contact outcomes and `169510` posed-target outcomes. Those
three return services are the boundary; the test proves their ordered
dispatch, filtering and composition, not their contact geometry or live
producer values. Full source registration chronology, current attachment
poses and actual closest-contact packets remain required inputs for live
dynamic contact integration. No source result is substituted for the scan
being tested.

## Retained list order is concrete evidence, not an event log

`object-live-registration-retained.py` independently checks the existing raw
Zanzibar collidable heads and all linked32-byte records against full generation
links. The sample has55 clusters,187 linked records and155 unique owners;
record types are11/3/7/2/6/1/0/8 with68/45/24/21/20/7/1/1 entries. It includes
categories absent from the native SCEN/MACH/BLOC registry.

Three current cluster lists are not reverse current object-slot order. For
example cluster0 starts with object slots166,119,129,126,80,79; cluster7 and38
also contain ascending pairs. This proves that a one-time ascending-slot
insert of the current registry cannot reproduce this retained order. It does
not prove which individual relink, allocation/reuse or callback caused each
pair. The immutable report retains exact source full handles and order.

The record pool header independently corroborates capacity2048/stride32.
The original observation is labelled paused Zanzibar; its exporter did not
retain a GPU-frame/time stamp. This audit does **not** label it a matched
Capture20 frame, reconstruct all202 then-live owners from91 raw records, or
infer missing noncollidable lists.

## Remaining activation and backend services

After insertion `B8600` invokes `BEF30` for attachment light updates and `B6D60`
with the actual interested-cluster bitmap at game globals`+11B8`.

`B6D60` admits object flag1 immediately. Otherwise type7 calls `108D30`, whose
machine override `115A40` consumes actual `+1DA/+1DC` portal range, BSP portal
index records and the interested bitmap. A positive range means this override
handled the decision; its output is true only when either authored adjacent
cluster of a referenced portal is interested. With no handled override, a
valid header cluster tests that bit; NONE produces false. It is not a visible
cluster test from the current camera.

`B7290/B7300` change header active bits and invoke physical activation for types
selected by mask`0x1883`; object flag14 contributes an active counter. If
interest is false, source object flag17 and authority predicate `A7670` can
instead enter `B8540` dormancy/visibility/callback services. All742 retained
cold-prefix flag17 values are clear, closing that branch for those exact input
bytes only. Cold harness physics datum NONE is an explicit isolated backend
allocation boundary, not evidence that production physics never allocates.

An old-NONE to valid cluster transition also executes `1C36F0`, `278F00` and
clock/backend brackets. Those services and all post-constructor lifecycle
mutations remain outside the recovered registration-prefix seam.

## Proposed typed native service contract

A persistent adapter should accept actual events from the caller rather than
sort a completed scene. Its smallest packet/state contract is:

```cpp
struct SourceRegistrationOwnerState {
    uint32_t full_handle;          // Actual native lifetime; no Xbox relabel.
    uint16_t actual_pool_rank;     // Explicit zero-based iterator rank.
    int8_t type;
    uint8_t header_flags;
    uint32_t object_flags;
    uint32_t parent, child_head, sibling;
    SourceObjectLocation location; // Retained literal eight-byte packet.
    int16_t header_cluster;
    ObjectLightSpatialSphere rendering_sphere; // +30/+3C.
    ObjectLightSpatialSphere primitive_sphere; // +40/+4C.
    Vec3 root_position;            // +64; NULL-location fallback only.
    SourceObjectPrimitiveInput classification;
};
```

This packet sketches the required producer fields; the compiled packet is
`SourceObjectRegistrationOwner` in `source_object_spatial_pool.hpp`.
`SourceObjectLocation`/primitive helpers supply the pure location and
classification operations. The caller additionally needs an event kind,
monotonic event ordinal, actual construction phase, BSP identity and optional
source seed packet. It must distinguish supplied packet from NULL, including
an invalid supplied packet. The adapter owns previous spheres/locations,
registered bits, full-handle links, both spatial pools and epoch/reset state.

Constructor, callback, register, relink, in-place update, unregister and free
events must be observed where they occur. Native `created_tick` is insufficient
for same-tick ordering; reused handles must invalidate old state. Restore must
restore the journal cursor, complete spatial/header state and render-cache
history with the scene checkpoint. The actual interested bitmap, machine portal
ranges and backend/attachment outcomes stay explicit service inputs.

The resulting `SourceObjectAdmissionObject` registry feeds actual rendering
spheres and flags into `source_object_admission`, using actual portal output,
containing-fog bytes and the existing ordered lists. Its queue order is then the
input for `133720→133790→4B5D0`, including first accepted owner, history and
transient collector-token allocation. A material vector order or per-kind
MACH→BLOC→SCEN append order cannot substitute for that queue. All owner types
needed by the queue, including actors/held weapons/equipment/vehicles, must
participate in the same explicit registry or remain an acknowledged boundary.

## Bounded implementation paths

1. Add a native lifecycle/event observer before initial construction and at
   BSP activation, without changing actual native allocation identities. Run
   the verified source registration algorithm on those actual native events.
   Start with the742 known SCEN/MACH/BLOC cold-root definitions and their
   proven empty model-child banks. Require exact retained sphere, location and
   classification inputs; reject missing ones. Keep unknown activation and
   physics services explicit. This establishes original algorithms on native
   chronology, **not equal original global success/slot history**.
2. Use the implemented independent raw spatial pools and journal, retaining
   actual caller events and complete checkpoints. The successful-allocation
   list service remains a narrower alternative only when actual simultaneous
   links stay below2048 independently in each pool/bank. The standalone
   allocation and registration-prefix proof does not supply missing live
   activation/physics or successful original factory-history inputs.
3. Connect original object admission to the ordered owner render collector only
   after the live registry exists. Initially preserve explicit missing-category,
   child/backend/activation boundaries. Recover additional constructor/effect
   and runtime producers to expand that boundary; do not change it by sorting
   metadata or assuming zero flags.

Current `SceneObjects` has actual native successful handles and birth ticks,
but no event journal, header/object flag bank, hierarchy, location, primitive
state or spatial membership. `SourceSceneryScene`, `SourceMachineScene` and
`SourcePropScene` synchronize by source placement identity and compose current
poses; they do not produce the missing registration events. Their per-view
geometry/lighting helpers remain independently useful. The standalone services
provide a concrete integration seam; production object admission still needs
the actual lifecycle producers and explicit activation/backend services.

## Reproducible artifacts

- `analysis/multiplayer-shading/object-live-registration-inventory.{py,json,log}`:
  all742 actual initial-variant/callback/attachment identities; every input
  report SHA-256 is retained.
- `analysis/multiplayer-shading/object-live-registration-retained.{py,json,log}`:
  independent full-link raw-byte check, exact retained91 hierarchy/primitive
  fields, list-order counterexamples and per-input SHA-256.
- `analysis/multiplayer-shading/object-spatial-pool-original.{py,json,log}`:
  unchanged original function execution and complete raw-pool snapshot inputs.
- `analysis/multiplayer-shading/object-spatial-pool-scan-original.{py,json,log}`
  and `tests/fixtures/retail_source_object_spatial_scan.tsv`: unchanged original
  contact dispatch over pressure/orphan banks; fixture SHA-256
  `a0b4c9a971ffbc857db99904e078d90b989d1f582bbc6e09055ece5f4884b294`.
- `tests/fixtures/retail_source_object_spatial_pool.tsv`: frozen original output
  words, SHA-256 `2d88ffabe213bea9dfc4566e63357eb363c50a04f065a7f4c50964db3d9cf086`.
- `analysis/multiplayer-shading/registration-snapshot-restore-20261008/export.json`
  and `exports/`: independently restored complete source registry and pools;
  snapshot label, CPU/clock checks and every exported file hash are retained.
- `retail_source_object_spatial_pool` tests the standalone production-linked
  service in private normal and sanitizer builds;
  `retail_source_object_spatial_scan` tests its raw contact consumer. Existing immutable source
  bytes, original sessions and prior exports remain unchanged. This CPU proof
  does not establish live renderer admission or framebuffer equality.
