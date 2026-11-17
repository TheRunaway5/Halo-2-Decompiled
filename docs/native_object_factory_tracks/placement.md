# Track `placement` (world placement)

Roots: C1540, BA630, BA8C0, BA7E0, B9460, B95D0, 1CD620. Sources:

- `src/gameplay/objects/native_object_factory_placement_tu15.cpp`
- `include/halo2/native_object_factory_placement_tu15.hpp`
- `tests/native_object_factory_placement_kernels.cpp`
- `tools/native_object_factory_cases_placement.py`

Every owner works on the live world memory. Stores keep the original order,
width and value. SSE steps run under the live MXCSR; x87 steps run the
original instruction bytes under the live control word. An unported callee is
a named `detail::refuse()`.

## Ported owners

"Cases" counts physical original cases. Each case is compared twice
(observer and publisher). Coverage lists the reachable original instructions
that no case executes; all of them sit behind a refused callee.

| Owner | Native | Cases | Coverage (missing) |
|---|---|---|---|
| 14CDB0 | `bsp3d_leaf_from_point` | 300 | 0/39 |
| 11E600 | `location_from_point`, `location_of_point` | 300 | 0/27 |
| 11E5C0 | `location_from_leaf` | 100 | 0/15 |
| C1540 (+1E60C0/1E6150 leaf walk) | `object_get_location` | 600 | C1540 0/43, 1E60C0 0/35, 1E64D0 0/52, 120FA0 0/76. 1E6150: 8 unreachable from C1540 (stop flag +21F is always 0). 1E6570: 9 unreachable (breakable-surface test; +4 is always 0) |
| BA7E0 | `object_disconnect_from_map` | 400 | 0/37 (C0F60: only the C4D30 site) |
| C3750 | `owned_lights` (C0F60's children service, BA7E0 and BA630) | through BA7E0 and BA630 (2026-11-15) | 0/73 |
| BA8C0 | `object_defer_physics` | 60 | 7/20: 1D97F0 (Havok) |
| B9390 | `object_mark_for_update` | reuse | native `native_tu15_wake_object_ancestors` |
| 123210 | `dominant_axis` | 300 | 0/31 |
| 243450 | `circle_inside_portal` | 400 | 0/60 |
| 14CEA0 | `portal_intersects_sphere` | 600 | 0/137 |
| 14D0E0 | `cluster_portal_flood` | 400 | 0/86 |
| 14D080 | `clusters_from_bits` | via 1D2F10 | 0/32 |
| 1D2F10 | `cluster_partition_reconnect` | 500 + 150 (`_bare`) | 1/111: 1D2FB3 (a flood reaching more than 256 clusters) |
| B8D90 (+10DDE0, 118120) | `object_in_active_clusters` | 500 | 0/53, 0/29, 0/69 |
| B92C0 (+1CC3F0) | `object_activate` | 300 | 0/33. 1CC3F0: 3/22, the 1CC310 call (Havok) |
| B9330 (+1CC440) | `object_deactivate` | 300 | 0/30. 1CC440: 6/47, E3E20 and the 1CC310 call |
| B1870 | `object_delete_protected` | 100 | 0/17 |
| 1CC190 (+BCFB0, 1CC520) | `object_create_physics` | 300 | 35/74: Havok creation (1D7360, 1D73D0, 1D97F0, C1E70) |
| 1CC290 | `object_create_physics_tree` | 300 | 0/40 |
| 282C10 | `process_dirty_islands` | 40 | both early exits. The island walk is the existing native `native_tu15_process_dirty_island_objects`; its Havok services are refused |
| BA630 | `object_reconnect_to_map` | 800, plus 600 with sphere-light attachments (`object_reconnect_lights`) | 3/138: the BA570 call |
| BAC70 | `object_attachment_marker` | 200 | 0/23 |
| BC330 | `object_root_location` | 100 | 0/25 |
| BACD0 (+FC510) | `object_marker_owner` | 200 | 0/44 (2026-11-15: the FC510 item path is native: an item, types 2..4 with +4 bit 0, in parent mode, +12C bit 0 without bit 1, names its +154 owner; the marker fixture now carries such items) |
| 1E11B0 | `model_marker_index` | 200 | 0/28 |
| 144910 | `quaternion_rotation` | 600 | 0/77 |
| 1E1200 (+145560 from nodebuild) | `model_marker_matrices` | 500 (node maps since 2026-11-15) | 0/106 |
| BAD60 | `object_marker_matrices` | 600 | 0/118 |
| C4D30 | `light_reconnect` | 600 (sphere and frustum lights) | the first-person marker's success path (C4DE5..C4E1C) is covered by earlier runs; the 16D740 branch (C5088..C50C1) is native since 2026-11-16 (see [Collision sphere fit](#collision-sphere-fit-16d740-2026-11-16)) |
| C2870 | `light_frame_point` (C4D30 for +2 bit 2, 2026-11-15) | through C4D30 and BA630 | 0/104 |
| C39B0 | scenario light shapes in `light_shape_record` (2026-11-15) | through C4D30 and BA630 | 0/34 |
| 145070 (+C4ECB frame) | `transform_point`; the parent-node light frame (+58 object, +5C node) | through C4D30 (2026-11-15) | 0/49 |
| 1692E0 (+16918F, 158290, 169112, 16945E, 11E060, 1509B0, BCDF0, 1E1360) | `first_person_marker` (C4D30, definition bit 5) | through C4D30 and BA630 (2026-11-15) | 1692E0 0/58, 16918F 0/105, 158290 0/29, 169112 0/26, 16945E 1/19 (its NONE-user exit, unreachable from 16918F), 11E060 2/20, 1E1360 0/17, 1509B0 0/9 |
| C48E0 | `light_update_bounds` | through C4D30 and BA630 | 0/228 |
| C4C80 | `light_update_sphere` | through C4D30 and BA630 | 0/60 |
| C3A20 (+C3960) | light shape record | through C4D30 and BA630 | C3960 0/33. C3A20 2/42: the type-1 adjust (C3AB0 refuses type 1) |
| C3AB0 | light frustum | through C4D30 and BA630 | 12/194: the sphere branch (unreachable through C4D30). The degenerate-target fallback (C3B75: the frame's forward axis at unit distance) and the orthogonal type 1 (C3C0B: its slopes from the global zero 2-D vector [0x474CD4]) are native since 2026-11-16 |
| C3820 | frustum record check | through C55D0 | 8/77: the sphere branch (unreachable through C55D0) |
| C55D0 | frustum corners | through C4C80 | 0/165 |
| 1E7330 | `points_bounding_sphere` | 500, and through C4C80 | 0/253 |
| B7E50 | `simulation_entity_dirty` | 200 | 0/22 |
| 1CD620 | `object_update_physics_position` | 200 | 2/40: the 1DC270 call (Havok) |
| E24F0 (+30BD0) | `biped_relocation_filter` | 500 | 0/60, 30BD0 0/38 |
| 10E340 | `object_type_relocation` | 300 | 0/35 |
| B9460 | `object_relocate` | 800, plus 600 with BF050 (`object_relocate_nodes`) | 0/123 |
| B95D0 | `object_set_position` | 600 | 2/76: the BA570 call (census-unexecuted) |
| BF050 (track nodebuild, bound by default) | `object_rebuild_nodes` | through the two kernels above | 0/35 |

All 37 placement kernels pass together under `HALO2_ORACLE_STRICT_WRITES=1`:
14,050 physical original cases, 28,100 native comparisons and 652,105
identical ordered stores. The command was `HALO2_FACTORY_TRACKS=placement`,
with `--only` for each kernel and the harness built with
`-DHALO2_NATIVE_OBJECT_FACTORY_TRACKS="placement;core;nodes;nodebuild"`.

Reused existing natives:

- C0F60: `native_tu15_detach_object_tree`
- C5250 and 1D30F0: `native_tu15_detach_light_attachment` and `native_tu15_remove_spatial_chain`
- BA910 and 16E290: `native_tu15_build_object_spatial_packet`
- 16E6A0 and 16E8A0: the data-array allocate and delete owners
- 1CC310: the havok track's `havok_component_state_flags` (since
  2026-11-12; it binds `native_tu15_set_component_state_flags` with its
  native 1D8470 reset and the 1D9510/1D73D0/1D97F0 refusals, so the Havok
  blockers sit in one place)
- BA8C0's flag publication: `native_tu15_defer_component_object`
- 282C10: `native_tu15_process_dirty_island_objects`

From the core track:

- 1496F0 and 149AD0: `physics_memory_reserve` and `physics_memory_pressure`
- the 0x474B90 type table: `object_type_chain` and `object_type_callbacks`

## Notes

- **C1540 and 1E60C0.** C1540 consumes only the first collision leaf and
  whether any leaf was recorded. The surface, edge and vertex lists that
  1E64D0 and 1E6570 write go to C1540's stack buffer and are dead. The cases
  still run them on well-formed BSPs.
- **1E6150 plane stack.** The stack has 128 slots and the original never
  bounds it: slots 128..130 run over the walk record's projection state
  (see [Collision sphere fit](#collision-sphere-fit-16d740-2026-11-16),
  the walk record). C1540's leaf walk keeps the part it observes: the stop
  byte a marked plane at slot 128 sets. A push at slot 131 overwrites
  1E60C0's return address; the original faults there, and the native
  refuses.
- **14CEA0 FP.** The plane distance (`fst`/`fabs`/`fcomip`) and the disk
  (`fsubp st(1)`, `fsqrt`) run in x87 under the live control word. The rest is
  SSE. Cases cover every FP control in `FP_CONTROLS`, including PC 24/53/64,
  DAZ/FTZ and all rounding modes.
- **.rdata literals.** These are checked against the XBE when the cases load:
  the 0x44AC8C projection rows, 0x46A0E8 (0.0f), 0x46AA04/0x46A530 (world
  bounds), 0x46A0EC (normalize epsilon) and 0x46A0D0 (1.0f).
- **30BD0.** It is ported locally on caller-local vectors. B95D0's copies are
  native locals, not memory. The shared native
  (`native_tu15_normalize_vector_30bd0` in
  `src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp`) is not linked
  into the factory harness.
- **Frustum lights.** C3AB0's half-angle tangent runs the original x87
  `FPTAN` under the live control word. The FST-rounded tangent and the
  unrounded product with the aspect are kept as the original does. 1E7330's
  half root and point distances are x87 as well; the distances use the
  original instruction bytes. The shape records and corner arrays are the
  original's stack frames, mirrored in a private scratch extent.
- **BA630's local stack data.** BA630's all-cluster vector, its packet and the
  1D2F10 overflow byte are caller-local and are not stored. The vector is
  refused beyond its 512-cluster buffer.

## Mutations

31 source mutations were run, each with its owners' oracle cases. 29 are
rejected. Four of those were rejected only after the fixtures gained the
exact-boundary cases below:

- a sphere touching the root plane (d = ±r);
- a point on a plane (d = ±0);
- a portal sphere at rr² = d²;
- a forward vector whose length is exactly the 30BD0 epsilon.

The rejected mutations, by area:

- **Leaf walk and locations:** leaf straddle order, back bound, point side,
  location store order, leaf NONE cluster.
- **BA7E0:** flag mask, partition selection.
- **Portal test:** disk operand order, sphere bound; 123210 axis tie.
- **Flood and 1D2F10:** stamp store, word maximum, flood counter, overflow
  compare, payload tail.
- **Activation and physics creation:** always-active bit, roots-only
  activation, deactivation mask, physics count width, physics tree root stop.
- **BA630:** location store order, first-connection tail.
- **Relocation:** entity mode, biped forward z, normalize epsilon, world upper
  bound, up source, keep links, physics force gate.

A second set of 13 mutations covers the marker and light owners, and all 13
are rejected:

- attachment bound and root location order;
- owner walk flag, marker name 0 and quaternion epsilon;
- quaternion operand, signed marker region and marker maximum;
- fallback return;
- light frame sum, light size choice, registered flag and location fix-up.

Four of them were rejected only after these fixtures were added: a group
named 0, a norm exactly the epsilon, region bytes 0x80..0xFE, and NaN/±0
light sizes with a negative minimum radius.

A third set of 11 mutations covers the frustum owners (C3AB0, C3820, C55D0,
1E7330 and C48E0's frustum branch), and 10 are rejected:

- tangent order, left-axis order and bitmap ratio;
- corner signs and horizontal sign;
- pair strictness and the grow gate;
- view-gate bit, frustum minimum radius and spread threshold.

Two of those were rejected only after these fixtures were added: a zero
light scale against a -0.0 minimum radius (an FCOMIP tie), and a zero angle
with zero near width (C3820's spread gate fails into C4C80's default). The
survivor, C3AB0's type-2 near-plane offset, is equivalent here: C4D30's
consumers never read record fields +28..+34.

Two more mutations are equivalent:

- 118120 `first >= end`: the original's JGE cannot fire once the count is
  positive, so the native drops the line.
- 243450 DAZ on the zero-length test: under DAZ the ADDSS that forms the
  squared length already zeroes a denormal square, so a denormal never reaches
  the UCOMISS.

## Havok owners (not ported)

1D97F0, 1D9510, 1D8470, 1D73D0, 1DC270, 1D7CA0 and 1D7360 are still refused.
Their executed closures are the Havok world (body add/remove, broadphase,
islands, shape virtuals): thousands of instructions with indirect shape and
listener calls. The nodebuild track ports parts of the 1DE170, 1DEFB0,
1DF6B0 and 185C00 tree for BFF20 (1DC7A0). A port of 1D73D0 (1DDBF0,
1DD9A0, 1DEE30, 1CD6A0) should reuse those owners.

## Refused dependencies

**Havok:**

| Owner | Reached from |
|---|---|
| 1D97F0 | BA8C0, 1CC190 |
| 1D9510, 1D73D0, 1D97F0 | 1CC310 (track havok's owner; its 1D8470 reset is native there) |
| 1DC270 | 1CD620 |
| 1D7CA0 | 282C10 |
| 1D7360 / 1D73D0 | 1CC190 with `[0x48B870]` set |
| level 2+ reclamation | 1496F0 (core) |

**Other tracks:** none refused. BF050 is nodebuild's `object_rebuild_nodes`,
called by default; `PlacementServices::rebuild_nodes` can override it.
Nodebuild's BFF20 still refuses BA7E0/BA630 for connected objects whose
cluster sphere moved. Those are this track's owners, and nodebuild could bind
them. The BF050 cases therefore use the model-less BF0C0 path and
unconnected children.

**Not native yet:**

- 16D740's object query (16D2A0 flags bit 3: 16D0E0 with 1F7E00..1F84F0
  and 24DFB0's callers). No 16D740 caller sets bit 3 (C4D30 passes
  0x0D800001, the effects' 17D625 0x0D800005); only the AI's 1A676B calls
  16D2A0 itself with it. The cluster walk before it is native, and the
  first object not yet visited is the refusal. The instanced-geometry query
  (bit 2, 16CF00) is native since 2026-11-16 (see
  [Instanced geometry](#instanced-geometry-16cf00-2026-11-16-continuation)).
- BA570: deletion. It is unexecuted in the census and is reached from BA630
  bit 17, B95D0 out of bounds, and 282C10.
- E3E20: deactivating a seated biped. Unexecuted in the census.

B95D0 and B9460's BF050 path are proved with nodebuild's native BF050. Build
with `-DHALO2_NATIVE_OBJECT_FACTORY_TRACKS="placement;core;nodes;nodebuild"`.

## Owned lights and first-person light markers (2026-11-15)

The node-rebuild map sweep reached both through BFF20's connected path
(BA7E0/BA630): C3750 on Ascension, Elongation, Gemini, Lockout, Turf and
Zanzibar, 1692E0 on Beavercreek and Dune (4 players).

- **C3750** (owner datum, detach, reconnect): every live light of the
  light pool (0x4ECBDC: +24 stride, +38 count, +44 rows, +48 live bits)
  whose +58 owner is the object, in index order, is detached (C5250's
  steps, `native_tu15_detach_light_attachment`) and/or reconnected (C4D30).
- **1692E0** (C4D30 for a light definition with +0 bit 5): when the light
  owner's root is a unit (BCDF0 mask 3) whose player (+13C; player rows
  [0x4F54F4] +28) is a local user with first-person view state ([0x4F6498]
  + 0x20CC * user, +C bit 0), 16918F takes the marker from the user's
  slot-0 weapon: the user holding the weapon (169112, the last match), a
  first-person director (158290: mode 245400 with a zero +4), the slot
  (16945E), the weapon definition's first-person model entry (11E060: +2A8
  count, +2AC rows of 0x10 by variant +8) or, for name 0xA00000B6, the hands
  model ([0x4ECC1C] +13C rows of 0xBC); 1E1360 = 1E11B0 + 1E1200 through
  the slot's node map (+FC / +1FC, dwords) and node matrices (+308), then
  each marker's world matrix is premultiplied by the view matrix (+2060).
  A first-person marker publishes the light frame (+84, +AC, +B8) and its
  target (+90, the x sum position-first); otherwise C4D30 falls back to
  BAD60. 1E1200 now follows a node map natively.
- Fixtures: owned lights in `object_disconnect_from_map` and
  `object_reconnect_lights` (with marker owners, since a +58 light without
  one takes the refused 145070 path); a first-person world (an image .bss
  block at 0x560000 with four users, players, hands rows and the weapon
  definition's +2A8/+2AC rows; directors in the 0x4F5500 absolute block);
  node maps in `model_marker_matrices`; the 11E060 jump table is declared.
  Proof: `PASS 2100 physical original evaluator tails, 155446 ordered
  original stores` (light_reconnect, object_reconnect_lights,
  object_disconnect_from_map, model_marker_matrices; strict writes).
  Mutation screen: C3750's four mutants (detach or reconnect skipped,
  owner from +4C, datum without salt) are rejected.
- **145070** (the second re-sweep reached it through C3750's reconnected
  owned lights, which have no marker owner): C4D30 transforms the light's
  local frame (+60 position, +6C forward, +78 up) through the +58 object's
  node matrix (+5C node of its body +116 block): the position by 145070
  (scaled first unless the scale is exactly 1), both axes by the rotation
  (lane 0 sums (z + y) + x, lanes 1 and 2 (x + z) + y), and the target
  +90 = position + forward. Fixtures: parent-node lights without a marker
  owner. Proof: `PASS 2100 ... 159715 ordered original stores`, 145070
  49/49, C4D30 223/262; the four 145070/frame mutants and the four
  first-person/node-map mutants (slot-1 weapon, view matrix skipped, any
  director, node map ignored) are rejected.

## Collision sphere fit (16D740, 2026-11-16)

C4D30 fits an unplaced light's sphere (+18, radius +24) into the
structure's collision geometry when the definition has +0 bit 15: 16D740
with flags 0x0D800001. The fitted centre is stored to +18 and copied to the
location point +38. `collision_sphere_fit` is native, with its whole
reachable closure, in a new section of the placement source. It follows the
code standard: everything between the inputs and 16D740's four output
stores lives on the original's 0xB96C-byte stack frame, so the native keeps
it in host structs (`SphereContacts`, `CollisionFeatures` with
`FeatureVertex`/`FeatureEdge`/`FeatureSurface`), reads the collision BSP
through a named view (`CollisionBsp`), and runs the x87 steps as small
helpers under the live control word.

| Owner | Native | What |
|---|---|---|
| 16D740 | `collision_sphere_fit` | Radius <= 0 returns the input. Otherwise, for every surface feature: the polygon point nearest the centre (the centre's projection when inside every edge, else the nearest edge point; the polygon is unprojected from its dominant-axis coordinates, the third coordinate solved from the plane unless the plane is nearly parallel to the axis). Within the radius: a centre on the plane (0.01, x87) and inside it, or a degenerate or short (<= 0.01) offset to the nearest point, casts one ray along the surface normal (span: centre to hit); otherwise rays both ways along the offset direction. The span's midpoint and half length (x87) go to one of two groups (along the normal first); the first group with spans gives the averaged centre and the smallest half length (AL 1). The nearest-point record persists across surfaces as the original's single stack record; its uninitialised use (a radius whose square overflows) is a refusal. |
| 16D2A0 | `collision_features`, `query_clusters` | The BSP walk on [0x4ECC10] with the radius grown by 1/16 (FLD, FADD, FSTP), the breakable bits [0x4F9B50] + index * 32 + 1 and a 256 limit; then the vertex, edge and surface features of its contacts (bit 0); then the clusters of the walk's leaves, each once, with their instanced geometry (bit 2, 16CF00) and their object chains (bit 3; 16D0E0 is a refusal). See [Instanced geometry](#instanced-geometry-16cf00-2026-11-16-continuation). |
| 1E60C0, 1E6150 | `SphereContactWalk::visit` | The 3-D walk with its plane stack (a straddled plane pushed with bit 15 sign-extended while the first child is walked, then plain); leaves (256) and their BSP2D references on a straddled plane, on the same side; the centre projected onto the reference plane in its dominant-axis coordinates (x87 \|n\|, the 123210 axis rule) |
| 1E64D0 | `visit_bsp2d` | BSP2D lines within the radius of the projected point |
| 1E6570 | `visit_surface` | A breakable surface (+4 bit 3) with its bit clear is skipped; ring vertices within the radius (SUBPS/MULPS order), ring edges touching the sphere (120FA0), else the projected centre inside every edge; vertices, edges and surfaces de-duplicated with 256 limits |
| 120FA0 | `segment_touches_sphere` | |
| 24EDB0, 24E710, 24DFB0 | `collision_features` | Vertex features (point, radius 0) |
| 24E850, 24E0F0 | `add_edge_feature` | Convex edges (the cross product of the two surfaces' planes along the edge above -1e-4 for equal flips, below 1e-4 otherwise) and edges whose surfaces share a plane index; coplanar surface words are skipped |
| 24EB60, 1E6050, 14CE30, 24E580, 123210 | `add_surface_feature` | The ring polygon (more than 8 vertices overrun 24EB60's stack buffer: a refusal), the plane (negated with 0 - x for surface bit 15), its dominant axis and sign, the (u, v) polygon |
| 24FCC0 | `ray_through_features` | The nearest feature hit facing the ray (dot below -1e-4), vertices, edges, surfaces in order |
| 24F4B0, 30BD0 | `ray_hits_vertex`, `normalize3` | x87 entry (FSQRT, FSUBR, FCOMIP against the extended value); (0, 0, 1) for a zero normal |
| 24F670 | `ray_hits_edge` | The capsule quadratic, x87 FSQRT, the [0, 1] clamps and the segment span; (1, 0, 0) for a zero normal |
| 24F9C0 | `ray_hits_surface` | The slab (x87 plane dot against 1e-4) and the polygon's edge half-planes |

16D740's arguments 3 and 4 to 16D2A0 are 0, so no feature is extruded and
every feature radius is 0: 24DFB0/24E0F0/24E580's extrusion is unreachable
from 16D740. The structure passes no transform; an instance (16CF00)
passes its matrix to 24E710, 24E850 and 24EB60 (native since the
continuation). 32A660 (the stack probe) has no observable effect.

C3AB0's degenerate-target fallback (C3B75) is native as well: a target on
or behind the light's forward plane takes the frame's forward axis at unit
distance. The light tick's oracle (track lights) reached it through C4D30.

**Fixtures.** `collision_sphere_fit` (1,500 cases): scenes of 1..6 convex
polygons (3..8 vertices) around the sphere, axis-aligned, random and
unnormalised (1e-5, 2) normals, planes stored negated with surface bit 15,
heights 0, +-0.004, +-r and random, the centre on a vertex, edge midpoint or
centroid, a second polygon's edge through the end of the normal ray (the
capsule exit clamp), a BSP chain with one node per surface plane, leaves
referencing the surfaces (bit 15 random) through direct BSP2D leaves or a
split line, edge sides without a surface (0xFFFF names the row before the
surfaces), breakable surfaces, flags 0x0D800001, 1, 0x11, 0x0D800000 and 0,
radii 0, -0, -1, 1e-30, denormal and NaN, and the output aliasing the
centre (as C4D30 passes it). The light worlds (`light_world`) add +0 bit 15
definitions with a small scene around the first light; map_world's random
collision BSP now carries the vertices' first edges and the 0xFFFF surface
row, which 16D740's features read.

**Proof** (strict writes, seed 0x5C41, the all-tracks harness, final
fixtures): `collision_sphere_fit` with `light_reconnect`,
`object_reconnect_lights`, `object_disconnect_from_map`,
`model_marker_matrices` and `points_bounding_sphere`: `PASS 4100 physical
original evaluator tails, 8200 native comparisons, 148173 ordered original
stores`.
Coverage (all reachable instructions): 16D740 627/627, 1E60C0 35/35,
1E6150 252/260 (the stop flag's exits: 1E60C0 always clears it), 1E64D0
52/52, 1E6570 242/242, 120FA0 76/76, 24EDB0 94/94, 1E6050 41/41, 14CE30
37/37, 123210 31/31, 24FCC0 188/188, 24F4B0 109/109, 24F670 214/214, 30BD0
38/38, 24F9C0 203/203, C4D30 262/262. Unreachable from 16D740: 16D2A0
74/234 (instances and objects), 24E710 36/83 and 24E850 121/210 and 24EB60
50/148 (the transform matrix), 24DFB0 39/92, 24E0F0 56/307 and 24E580
84/119 (the extrusion), 32A660 19/27 (the small-frame probe).
ASan/UBSan (with the light kernels): see [lights.md](lights.md).

**Mutation screen** (out of tree, the in-tree SHA-256 checked before and
after each round; `HALO2_SPHERE_FIT_CASES=500` for round 1, full counts
after): 44 mutants over 16D740's closure, C4D30's call and C3AB0's
fallback; 32 rejected. Round 1 rejected 24; round 2 (the survivors at full counts with
targeted fixtures: dominant-axis ties, exact tie scenes with BSP2D lines
and vertices at the walk radius, coplanar neighbours, centres just outside
an edge, a vertex at or an edge line through the end of the normal ray)
rejected 7 more (axis tie, BSP2D front tie, zero convexity, capsule bound
order, facing threshold, the 0.01 offset threshold, the extent's x87
order); round 3 (frustum
targets on or behind the light, zero normals) rejected C3AB0's fallback
without its unit distance. The survivors are equivalent within
the domain C4D30 reaches:

- 16D2A0's 1/16 margin in SSE: the x87 sum of two floats is exact before
  its single FSTP unless their exponents differ by 40, where the smaller is
  below half an ulp; a denormal radius never passes 16D740's COMISS under
  DAZ.
- 1E6570's vertex test strict: a vertex exactly at the walk radius (the
  radius plus 1/16) lies beyond every ray, whose length is the radius.
- 24FCC0 nearest inclusive: features hit at an equal t give the same point,
  the only result 16D740 reads.
- 24F9C0's parallel test inclusive: a ray parallel to the surface never
  passes 24FCC0's facing test.
- 16D740 with a zero radius not exiting: no surface then passes r^2 > d^2.
- 24F4B0's entry in SSE: zero-radius vertex spheres make the discriminant a
  rounding residue, too small for the x87 and SSE roundings to part.
- Operand-order and SSE-for-x87 mutants that differ only in NaN payloads
  (both operands NaN) or in denormal coordinates under DAZ/FTZ (1E6150's
  projection, 16D740's ray end and offset).
- 16D740's nearest point kept per surface: the persistent record matters
  only when every edge candidate's distance squared overflows.
- 16D740's candidate A test strict: it parts only when the foot lies on A
  along the edge but not across it, which rounding cannot produce here.
- 24E580's sign test inclusive: the dominant component is zero only for a
  zero normal, whose surface no ray enters and whose unprojected polygon is
  the same either way.

## Oracle notes

- **Opaque callees.** Every root statically reaches the whole object_new
  closure (2,208 functions, with unbounded indirect jumps). The track module
  wraps `metadata()`:
  - `OPAQUE[kernel]` keeps the Havok entries, BA570/C3750/C4D30/E3E20 and the
    other tracks' roots out of that kernel's authenticated closure. Entering
    one is rejected.
  - `INDIRECT[kernel]` authenticates the type callbacks the cases reach:
    118120 for machines and E24F0 for bipeds.
- **Eight argument words.** The fixture argument bank holds eight words.
  - 1D2F10 reuses words: the payload is the centre block, and the overflow
    byte is the location.
  - B9460 takes `rebuild_nodes` from the position pointer's low byte, which
    is always 0.

## Continuation plan: C2260 (light attachment creator for C0CD0)

Scoped on 2026-11-12 and not started: context ran out. The track is at a
clean, validated state (strict-writes PASS above).

**Call shape.** C2260 is `stdcall(5 dwords)` with `ret 14h` and returns a
light datum in EAX (NONE on refusal). Its first argument is the light
definition tag (`& 0xFFFF` indexes `[0x4F0414]`). It reads
`[[0x4F3218]+8]` (game mode 5 short-circuits). It calls 16E6A0 on the light
pool `[0x4ECBDC]`, then C2CB0.

Proposed native, for the coordinator to wire into C0CD0:

```cpp
std::uint32_t light_attachment_create(SourcePhysicsContactMemory&, std::uint32_t a1, std::uint32_t a2,
    std::uint32_t a3, std::uint32_t a4, std::uint32_t a5, PlacementFp&)
```

Name the five arguments once they are decoded. C0CD0 passes them from its
attachment loop; see modelstate's C0CD0 notes.

**Executed slice still to port** (union census; executed instructions):
C2CB0 322, 13DE70 155, C2260 76, C2B30 63, 17F8A0 56, 134870 48, 17F790 45,
134730 30. That is about 800 instructions.

**Already native:**

| Owner | Native |
|---|---|
| C4D30 (and its C48E0/C3AB0/C55D0/1E7330 subtree) | `light_reconnect` |
| C5250 | `native_tu15_detach_light_attachment` |
| 16E6A0 | `native_tu15_data_array_allocate` |
| BBD50 | base `object_root_header_bit6` |
| BBD10 | `native_tu15_object_or_ancestor_deleting` |
| BCFB0 | local `root_object` / `native_tu15_render_root_owner` |
| 37DC0A | CRT |

**Ownership.**

- BCB70 belongs to the attachments track. Reuse or refuse it.
- Modelstate notes that BCB70 leads to 134730/13DE70 (function evaluation).
  Before porting 13DE70, 134730 or 134870, check whether the attachments or
  modelstate tracks have landed them, and reuse whichever lands first.
- 17F790 and 17F8A0 are not yet attributed. Check attachments.md (not
  present on 2026-11-12) before porting.

**Fixtures.** Reuse `light_world` (light pool, definitions, light partition,
marker owners) and `placement_world`. C2260 needs a free light-pool slot and
the definition fields C2CB0 copies.

## Light frames and scenario light shapes (2026-11-15)

- **C2870** (C4D30 for light +2 bit 2; ESI +84, EAX +90, stack +A0,
  +9C): the x87 offsets +90 - +84 (FSTP dword each; the z term squares the
  extended z by its stored float), the FSQRT length (FST dword) and 1e-4 >
  |length| (FCOMIP of the extended value) run as the original's sequence
  under the live control word. Then it marches t = 0, 0.001, 0.003, ...
  (the step doubling to at most 0.1) while length > t. At each point it
  takes the collision BSP leaf (14CDB0, `bsp3d_leaf_from_point` on a
  private point) when [0x474C24] is set. The first leaf whose structure
  cluster word is not NONE stores the distance (+9C), then the point (+A0),
  and returns 1.
- **C39B0** (C3A20 for a light whose +8 scenario entry, rows of 0x6C at
  [[0x4ECC20] + EC], has +3E bit 0): the record is zeroed, then +0 is the
  entry's +3C type. A sphere takes the entry's +68 twice, the second
  scaled by definition +1C / +18 unless +1C is an ordered zero. Other types
  take the entry's +58..+68.
- **C48E0** reads its sphere sizes from the shape record (+4, +8), not the
  definition: they differ for C39B0's scenario lights. The first run of the
  new fixture found this (light radius 3.4 against the native's -1.0).

Fixtures: light +2 flags with bit 2 (6, 7, 0xE, 0xF7); light +8 scenario
indices 0..3 over four scenario rows (+3C types, +3E bit 0, sizes +58..+68),
frustum entry types only in frustum-definition worlds (their frames are
built for frusta); definition +1C zero, -0, denormal or NaN in a fifth of
the worlds. Proof (strict writes, `light_reconnect`,
`object_reconnect_lights`, `object_disconnect_from_map`,
`model_marker_matrices`): `PASS 2100 physical original evaluator tails,
4200 native comparisons, 143803 ordered original stores`; on the final
fixtures (also lights whose target sits within 1e-4 of the position),
`PASS 2100 ..., 142934 ordered original stores`; C2870 104/104, C39B0
34/34, C48E0 228/228. ASan/UBSan (`light_reconnect`,
`object_reconnect_lights`): `PASS 1200 ..., 124302 ordered original
stores`, no report. Mutation screen (out of tree, in-tree SHA-256
unchanged): 11 mutants (C2870 step, doubling, cap, distance, cluster test,
length floor; C39B0 zero test, ratio, entry offset, +3E bit; C48E0's
definition sizes), all rejected. The length-floor mutant was rejected
after the near-zero targets were added. Receipts:
`build/test-results/nodebuild-rare-20261115/`.

## Overrun plane stack and orthogonal frusta (2026-11-16, continuation)

The light tick made C4D30 (with 16D740 and C3AB0) native by default, so
its last refusals would stop the game. No sweep reached them; they are
now replicated or are original faults:

- **1E6150's plane stack past 128 planes** (16D740's walk and C1540's).
  1E60C0's walk record ends with 128 stack slots (+1C), then the
  projection state: the axis word (+21C), the flipped byte (+21E), the
  walk's stop byte (+21F, cleared by 1E60C0) and the projected point
  (+220, +224). The original never bounds the stack, so slots 128..130 run
  over that state. The native keeps the record as it is (`WalkRecord`):
  a plane marked for the first child at slot 128 sets the stop byte (a set
  stop byte records a leaf without its references, and after a child
  returns with a leaf recorded the walk ends without popping: 1E624F,
  1E628A, 1E630F); the plain plane clears it; a projection rewrites slot
  128's low three bytes and slots 129/130, which later membership tests
  read. C1540's leaf walk (`SphereLeaves`) observes only the first leaf and
  whether any was recorded, so it keeps the stop byte alone. A push at
  slot 131 writes 1E60C0's return address: the original faults on return,
  and the native refuses ("the original overwrites 1E60C0's return
  address").
- **More than 8 polygon vertices** (24EB60): the ninth overwrites 24EB60's
  return address (its 0x60-byte buffer ends at the return address), so the
  original faults on return; the native's refusal now says so.
- **C3AB0's orthogonal frustum (type 1)**: its two slopes (+14, +18 of the
  frustum frame) come from the global zero 2-D vector ([0x474CD4], 0x44ABEC
  in the image) instead of the half-angle's FPTAN.

Fixtures: `collision_sphere_fit` gives a quarter of its cases a chain of
129..131 nodes the sphere straddles (planes through its centre) over a
two-surface scene: nodes 0..127 walk their next node first (a NONE leaf
may come first), node 128 marks its plane for a next node walked first or
stacks it plain after a NONE leaf, nodes 128..130 use planes no other node
uses, and their leaves hold two to four references (those planes, marked
or plain, and planes 0..2) onto alternating surfaces; centres sit on a
coordinate plane. `object_get_location` gives a fifth of its cases (no
structure BSP) a chain of 126..140 straddled nodes with random child
orders, cut where a Python model of 1E6150's control flow
(`straddle_walk_faults`) says the original would push slot 131. The light
worlds draw definition and scenario-light type 1 and set [0x474CD4] to a
random 2-D vector.

Proof (strict writes, seed 0x5C41): `collision_sphere_fit`,
`object_get_location`, `light_reconnect` and `object_reconnect_lights`:
`PASS 3300 physical original evaluator tails, 6600 native comparisons,
133699 ordered original stores` (the first deep fixture); the final
`collision_sphere_fit` fixture alone `PASS 1500 ..., 6000 ordered original
stores`. Every placement kernel on the final sources and fixtures (38 kernels):
`PASS 15550 physical original evaluator tails, 31100 native comparisons,
653295 ordered original stores`. Coverage: 1E6150 260/260 (its
stop-byte exits included), 16D740 627/627, 1E60C0 35/35, C1540 43/43, C3A20
42/42, C3AB0 182/194 (the sphere branch, unreachable through C4D30), C4D30
251/262 (the first-person marker's success path, covered by the light
tick's cases). ASan/UBSan (the sanitizer harness, `halt_on_error=1`,
with the light and attachments kernels below): see
[lights.md](lights.md) and the claims doc; no report.

Mutation screen (out of tree, the in-tree SHA-256 checked after each
round; `HALO2_SPHERE_FIT_CASES=800`): 16 mutants over the walk record,
C1540's leaf walk and C3AB0's type 1; 13 rejected. The first deep fixture
(random child orders) let the record's overlap mutants survive: no leaf
was ever visited past slot 128 with the stop byte clear. The final
fixture (next nodes first, node 128 marked or after a NONE leaf, unique
planes at slots 128..130, alternating surfaces) rejects every overlap
mutant (slots 128..130 not scanned, the projected point kept apart from
slots 129/130, slot 128 skipped). The three survivors are equivalent:

- a projection that clears the stop byte: projections happen only while
  it is clear;
- the stop byte read as slot 128's bit 23: a marked plane sets both, and
  a plain plane or a projection leaves both clear;
- C1540's stop byte never cleared: C1540 observes only the first leaf and
  whether one was recorded, which an earlier end cannot change (it only
  avoids a fault).

## Instanced geometry (16CF00, 2026-11-16, continuation)

16D2A0's instanced-geometry query (flags bit 2: the effects' 17D625 calls
16D740 with 0x0D800005) is native, with 16D2A0's cluster loops. Nothing
live reaches it yet (the effects are the paused group E), so the oracle is
the evidence. The code follows the native code standard: named views for
the structure BSP (`StructureBsp`), the instance transform (`Transform`)
and the .bss visit blocks (`VisitStamps`); each matrix site keeps its own
summation order in a small named function.

| Owner | Native | What |
|---|---|---|
| 16D2A0 (16D36B..16D5DA) | `query_clusters`, `VisitStamps` | When the structure walk recorded a leaf and bit 2 is set, or bit 3 is set: the cluster counter [0x4F3CE4] is incremented and its busy byte [0x4F3CE1] set, then the instances' ([0x4F44EC], [0x4F44E8]) and the objects' ([0x4EABCC], [0x4EABC8]); an object query without flags bits 4..16 sets them all (only 16D0E0 reads them). For each recorded leaf, its cluster (structure +2C leaves, int16) unless already stamped at [0x4F3CE8 + cluster * 4]; with bit 2 each listed instance (cluster +98, int16) not stamped at [0x4F44F0 + i * 4] (16CF00); with bit 3 the cluster's object chain ([0x4EABB0] heads, [0x4EABB4] entries +4 object, +8 next; an entry with a NONE object ends it), each object not stamped at [0x4EABD0 + index * 4] being 16D0E0's refusal. The busy bytes are cleared objects, instances, clusters. A NONE index reads the counter itself, so it always counts as visited |
| 16CF00 | `add_instance_features` | The instance (structure +140 rows of 0x58: +0 transform, +34 definition, +3C/+48 bounding sphere) passes 16B810; its bounding sphere reaches the query ((r + radius)^2 >= (dz^2 + dx^2) + dy^2, COMISS); the centre goes into the instance's space and the radius is multiplied by the inverse scale; the definition's collision BSP (+70, inline) is walked there with the breakable limit 8; its contacts' features come back through the transform (24EDB0) |
| 16B810 | `instance_collides`, `StructureBsp::instance_has_pathfinding` | No collision surfaces (BSP +28 count) or bit 4 on the last surface's flags excludes the instance; with flags bit 18 so does an instance the pathfinding data (structure +C4; its first row's +40 instanced-geometry references, 4 bytes) does not name (out of range or NONE). No pathfinding data or a null block admits it |
| 144090 | `inverse` | The transposed rotation, the reciprocal scale (unless 1) and the negated position through both, each lane summed in the original's order; a zero scale (UCOMISS, so a denormal under DAZ) gives the zero transform |
| 186E30 | `breakable_bits` | [0x4F9B50] + structure BSP * 1024 + instance + 0x201 (one byte per instance); the NONE path (+ structure BSP * 32 + 1) is 16D2A0's inlined structure bits |
| 24E710, 24E850, 24EB60 (matrix paths) | `vertex_to_world`, `edge_direction_to_world`, `edge_start_to_world`, `polygon_point_to_world`, `plane_to_world` | Points scaled first unless the scale is exactly 1, then rotated and translated; edge directions rotated only; the plane normal rotated and its offset moved by the position along the new normal plus the scale times the old offset (always scaled). The convexity test of 24E850 runs before the transform, in the BSP's own space |

16D740 passes 16D2A0 a zero height and feature radius, so 16CF00 extrudes
nothing either. Features are appended after the structure's, in
16D2A0's order, to the same 256-entry lists.

**Fixtures** (`collision_sphere_fit_instances`, 1,500 cases, entry 16D740):
a structure scene of one or two polygons (a tenth with the deep plane
chain, a twelfth with only NONE leaves); a structure BSP view whose 64
leaves map to 1..8 clusters (and NONE), each listing 0..4 instances (and
NONE); 1..3 definitions whose collision BSPs are small `fit_scene`s in
their own space (a third around the origin, where a zero transform's
inverse puts the query centre; mostly at the query's size), copied to
definition +70, with surface counts 0 or NONE, bit 4 on the last surface
and breakable indices below 8; 1..6 instances with identity, signed axis
permutations and random rotations, scales 1, 2, 0.5, 0.75, 1.5, random,
-1, 0, a denormal and NaN, placed so that the query centre lands on (or
near) the definition's scene centre; bounding spheres that reach, miss,
are NaN, or sit at an exact tie of the original's (z + x) + y sum; the
pathfinding data's count, block and references; cluster, instance and
object stamps random, current or already visited; object chains (bit 3)
with entry sizes 0xC/0x10/0x20, NONE heads and NONE objects, every object
the walk reaches already visited (the objects after a NONE entry are not);
flags 0x0D800005 (most), 0x0D800004, 5, 4, 0x40005, 0x0D840005, 0x40004,
0xC, 0x0D80000D, 8, 9 and 0x0D800001. The instance stamps are an image
.bss block (0x4F44E8); the cluster and object stamps use the 0x4F3CE0 and
0x4EABA0 blocks.

**Proof** (strict writes, seed 0x5C41, the all-tracks harness):
every placement kernel with the new one (39
kernels) on the final sources and fixtures: `PASS 17050 physical original
evaluator tails, 34100 native comparisons, 661688 ordered original
stores`; the lights kernels (C4D30 with the reworked 16D2A0): `PASS 1800
..., 297687 ordered original stores`. The first fixture alone (300 cases)
passed on its first run. Receipts:
`build/test-results/instanced-geometry-20261116/`.

**Coverage** (all reachable instructions): 16CF00 120/120, 16B810 25/25,
144090 82/82, 24E710 83/83, 24E850 210/210, 24EB60 148/148, 24EDB0 94/94,
16D740 627/627, 1E60C0 35/35, 1E6150 260/260, 1E64D0 52/52, 1E6570
242/242, 24FCC0 188/188, 24F4B0 109/109, 24F670 214/214, 24F9C0 203/203.
16D2A0 214/234: 16D3EF is dead (1E6150 never records a NONE leaf) and
16D516..16D55C is the 16D0E0 call (the refusal). 186E30 8/13: its NONE
path is unreachable from 16CF00 (a NONE instance always reads as
visited).

**ASan/UBSan** (the sanitizer harness, `halt_on_error=1`): both fit
kernels, `object_get_location`, `light_reconnect`,
`object_reconnect_lights`, `lights_update` and `light_delete`: `PASS 6000
physical original evaluator tails, 12000 native comparisons, 367490
ordered original stores`, no report.

**Mutation screen** (out of tree, the in-tree SHA-256 checked after each
round): 51 mutants over 16CF00, 16B810, 144090,
186E30, the transform paths, the visit loops and the object chain; 45
rejected. Round 1 (600 cases, the fixture growing during the round)
rejected 39; round 2 (2,000 cases, the final fixture: exact (z + x) + y
ties with three nonzero offsets, zero-scale instances near the query and
scenes around the definition's origin, scenes at the query's size, a
structure with only NONE leaves) rejected the distance sum order, 24EB60's
polygon order, 24E850's unscaled direction and the object query without
leaves; round 3 (6,000 cases) rejected 24E850's direction and start
orders. The survivors:

- 24EB60's normal z with the operands of its first ADDSS swapped: equivalent
  (an addition is commutative; only both-NaN payloads part).
- 144090 without its zero test, or testing zero without DAZ: equivalent
  for 16CF00. A zero (or DAZ-denormal) scale gives the zero transform, so
  the walk runs at radius 0 from the origin; the mutants' infinite inverse
  scale makes the centre non-finite. Neither straddles a plane (radius 0
  admits no straddle, a NaN or infinite distance takes one child), so
  neither records a contact or a feature.
- The object chain cut after its first entry: equivalent within the
  native's domain. A later entry matters only through 16D0E0, the refusal;
  an object already visited makes no store.
- 144090's x-lane order and 16CF00's centre order: these move the
  instance-space centre by an ulp, which changes 16D740's result only when
  a plane, vertex or edge of the instance's BSP lies within an ulp of the
  walk sphere. Not distinguished by 6,000 cases; not argued equivalent.

**16D0E0 (objects) is not ported.** No 16D740 caller sets bit 3, so the
native refuses at the first object not yet visited (the cluster loop and
its stamps before it are native and proven). Its closure (about 700
instructions: a type switch at 16D199, 1F84F0 with 1F7E90/1F7F60 and
144090, 1F7E00, E1B40/BBE00, 16B870 and 24DFB0's extruded spheres when
arguments 4 and 5 are nonzero) is reached only through the AI's 1A676B,
which calls 16D2A0 directly with 0x500038.
