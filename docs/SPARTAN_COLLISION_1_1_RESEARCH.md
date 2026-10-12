# Original disc 1.1 Spartan posed collision

The unchanged disc 1.1 instructions now execute the Spartan's real model and collision BSP query, including region/permutation selection, node transforms, 3D and 2D BSP traversal, surface selection and model-material lookup. The oracle supplies explicit poses, active permutation bytes and finite segments. It does not supply a lower intersection or a native expected result. Live animation poses, object broadphase, world location and object creation remain separate unresolved producers.

## Authority and reproduction

| Original input | SHA-256 |
|---|---|
| `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Original `.text` | `cc2f07253ca6d531ff068442df31cb91773a03ae2d6d25779f8be72ee95a5131` |
| `assets/disc/maps/lockout.map` | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |

`tools/oracle_spartan_collision_1_1.py` maps those original inputs and executes them in Unicorn. Each query asserts that no external service ran. It removes the earlier impact oracle's supplied `1E20C0` intersection service. The original paths are:

```
16AF90 -> 1F3500 -> 1F3900 -> 1F3590 / 1F3660 / 142D00
                         -> 1E20C0 -> 1E23F0 -> 29C7D0 / 1E25C0
16AF90 -> DA0E0 / 16A5C0
```

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_spartan_collision_1_1.py --suite analysis/multiplayer/map-pack/spartan-collision-suite
.tools/python/bin/python tools/oracle_spartan_collision_1_1.py --edge-suite analysis/multiplayer/map-pack/spartan-surface-edge-1.1-oracle.json
.tools/python/bin/python tools/oracle_spartan_collision_1_1.py --instruction-inventory analysis/multiplayer/map-pack/spartan-collision-original-instructions
.tools/python/bin/python tools/oracle_projectile_boundaries_1_1.py
```

The instruction inventory pins 15 routine listings and reachable instruction-byte hashes. Original floating-point environment: x87 control word `027F` (53-bit precision), MXCSR `1F80`; the trace resets the floating-point state before each call. All formulas below mean separate original multiply/add/subtract instructions, with their original rounding, rather than fused operations or algebraic reassociation. The listings remain the exact arithmetic authority.

| Frozen research output | SHA-256 |
|---|---|
| `tools/oracle_spartan_collision_1_1.py` | `99beb6fc599d06086fe8321a2b7346b1e494c77d60112c8622c2d57a85830e8e` |
| `spartan-collision-suite/suite.json` | `0852d54c6b22439d72d68ff6fa67f91bae8c4e1df987bce8d06ab1a29e31fa2c` |
| `spartan-surface-edge-1.1-oracle.json` | `ba6fa3dc537b49357f6b3f4c175cea4f37acfb496cc13a0caac5a95f12af5567` |
| `spartan-collision-original-instructions/inventory.json` | `2a65cb968b9c575a1f5f4ee21d52869dee2009cf3ffd1a1c3d3ef3eb13fa0f69` |

The suite pins every constituent case file's hash. Full lower output snapshots include source scratch/visited-leaf storage; only fields actually written by the path are valid outputs. On a miss the lower header retains caller bytes and is not a hit identity. Upper result position fields likewise remain caller-owned in the direct helper.

## Actual model route and authored schema

The oracle resolves `objects\characters\masterchief\masterchief_mp` by its `bipd` tag name, then follows `bipd+38` to `hlmt`, and `hlmt+0C` to `coll`.

| Tag | Datum | Root file offset |
|---|---|---:|
| Spartan multiplayer `bipd` | `E9F2086D` | 32230152 |
| Spartan `hlmt` | `E9F3086E` | 32232440 |
| Spartan `coll` | `E9170792` | 32019912 |

Original metadata pointers in this cache are file offsets plus magic `7E42C800`. Block descriptors are count/pointer pairs. Retain the following raw indices and original order.

| Owner | Offset | Stride | Meaning |
|---|---:|---:|---|
| `hlmt` | `58` | `14` | Model materials; damage-section i16 at entry+6, global-material i16 at +10 |
| `hlmt` | `70` | `10` | Model regions; collision-region byte at entry+4 |
| model region | `8` | `8` | Model permutations; collision-permutation byte at entry+5 |
| `hlmt` | `78` | `5C` | 33 nodes; authored default inverse matrix at entry+28, 52 bytes |
| `coll` | `1C` | `0C` | Collision regions |
| collision region | `4` | `14` | Collision permutations |
| collision permutation | `4` | `44` | Collision BSP records; node i16 at entry+0 |

The 44-byte object BSP includes the node prefix. `1F3660` supplies the traversal with a pointer to BSP+4, so its block-relative offsets are four smaller than the complete-record offsets below.

| Complete BSP offset | Block | Stride | Fields |
|---:|---|---:|---|
| `04` | 3D nodes | `08` | Plane i16 +0; signed packed 24-bit children at +2 and +5 |
| `0C` | Planes | `10` | Normal xyz floats +0/+4/+8, distance D +C |
| `14` | Leaves | `04` | Flags byte +0, 2D-reference count byte +1, first-reference u16 +2 |
| `1C` | 2D references | `04` | Signed plane i16 +0, signed 2D-node i16 +2 |
| `24` | 2D nodes | `10` | Normal xy floats +0/+4, D +8; signed children +C/+E |
| `2C` | Surfaces | `08` | Signed plane i16 +0, first edge u16 +2, flags byte +4, breakable byte +5, model-material i16 +6 |
| `34` | Edges | `0C` | Start/end vertex u16 +0/+2, next edges +4/+6, adjacent surfaces +8/+A |
| `3C` | Vertices | `10` | xyz floats; first edge u16 +C |

The authored 16 BSPs are arms region 0 (nodes 14,16,17,18,19,20), body region 1 (nodes 0,3,6), helmet region 2 (node 15), legs region 3 (nodes 1,2,4,5,7,8). All regions select authored permutation 0 in the stock fixture. Helmet surfaces select model-material entry 0; the other surfaces select entry 1. Actual `hlmt` materials are head/section 1/global material 56 and body/section 0/global material 56. Region and damage section are different indices; do not substitute one for the other.

## Context, permutations and iteration

`1F3500` produces a 20-byte context: +0 object datum, +4 model root, +8 collision root, +C active model-permutation bytes, +10 current node matrices. In the fixture, object-relative i16 +116 is the node-matrix offset, +118 is the region-state block size, and +11A is its relative offset. The first four region-state bytes supply the four active model permutations. The fixture explicitly sets these storage offsets; it does not claim to execute their original allocator.

`1F3590` starts 16 selected collision-permutation bytes at -1. For up to 16 authored model regions, it reads the active signed permutation byte, validates it against that model region's permutation count, then maps through the collision-region byte and collision-permutation byte. `FF` mappings or invalid active indices are skipped. With no model mapping pointer it copies the explicit collision selections, and with no selection pointer it selects permutation zero; those alternate input branches are in the listing, but the stock oracle uses the actual model mapping.

`1F3660` iterates collision regions in ascending authored order and the selected permutation's BSPs in ascending authored order. Inactive/out-of-range permutations and empty BSP arrays are skipped. The packed result is `(region << 24) | (permutation << 16) | (bspIndex << 8) | nodeByte`. The current node matrix is selected by that low node byte.

## Scaled transforms and plane output

Each current node matrix is 13 floats: scale s, three basis columns F/L/U, and position T. `142D00` produces the scaled inverse: for zero scale it writes 13 zero dwords; otherwise inverse scale is 1/s (the scale-one branch copies 1). It transposes the basis. Let `a = 0-T.x`, `b = 0-T.y`, `c = 0-T.z`, each separately rounded, and multiply each by inverse scale when scale is not one. Inverse position x/y are rounded as `(inverse.U[k]*c + inverse.F[k]*a) + inverse.L[k]*b`. Inverse position z is `(inverse.F.z*a + inverse.U.z*c) + inverse.L.z*b`; the first two addends are issued in the other order. It does not compute a general arbitrary-matrix inverse.

`1F3900` first scales each world origin/delta coordinate by inverse scale, except when inverse scale is one, then uses the inverse matrix I. Its exact origin expressions are:

```
local.x = ((I.L.x*y + I.F.x*x) + I.U.x*z) + I.T.x
local.y = ((I.U.y*z + I.L.y*y) + I.F.y*x) + I.T.y
local.z = ((I.U.z*z + I.L.z*y) + I.F.z*x) + I.T.z
```

The delta expressions have the same component-specific addition order and omit translation. There is no normalization here.

`16A5C0` transforms the selected local plane normal by the current matrix basis, in each component's z/y/x addition order: `(U[k]*nz + L[k]*ny) + F[k]*nx`. It does not multiply the normal by scale. World D is `((T.z*worldNz + s*localD) + T.y*worldNy) + T.x*worldNx`. Surface plane bit 15 reverses the normal and D through `0-value` operations. It copies the source leaf, surface, signed plane index, surface flags, breakable byte and material entry into the outer collision record. `DA0E0` resolves the actual model-material entry to the global-material index.

## 3D traversal and fraction boundaries

`1F3900` initializes lower result fraction from original VA `452044`, bits `7F7FFFFF` (maximum finite float). For each BSP it retains the best fraction, clamps negative fraction to zero, and calls `1E20C0` with [0, min(best,1)]. Descriptor prior leaf and prior plane are -1; prior leaf class is 0. It resets that BSP's visited-leaf count.

`1E20C0` begins at 3D node zero. A packed child with bit `800000` is a leaf; signed child -1 is the empty sentinel. The plane computations are separately rounded:

```
p = ((origin.z*nz + origin.y*ny) + origin.x*nx) - D
q = (delta.z*nz + delta.y*ny) + delta.x*nx
side(t) = (q*t + p) >= 0
```

When both interval endpoints select the same side, side false uses the packed child at node+2 and side true uses node+5. When they differ, tCross is computed as x87 `p/q`, sign reversed and stored to float. The first recursive interval [tMin,tCross] uses child index `(q <= 0)` (index 0 is +2, index 1 is +5). If it returns a hit, return immediately. Otherwise, if tCross >= current best fraction, return false. The traversal records this plane and follows child `(q > 0)` over [tCross,tMax]. No epsilon appears in these plane-side or fraction tests.

Leaf classification is literal: empty = 3; ordinary leaf flags bit 0 clear = 1; leaf flags bit 0 set = 2. Candidate selection is:

| Lower flag | Candidate transition |
|---:|---|
| `1` | Prior class 1 or 2 -> current class 3: prior leaf |
| `2` | Prior class 3 -> current class 1 or 2: current leaf |
| `4` clear | Class 2 -> class 2: prior leaf if flag 1 set, otherwise current leaf; request explicit edge containment |
| `8` | Reject a candidate surface with surface flags bit `2` |
| `10` | Reject a candidate surface with surface flags bit `8` |

These are verified class/bit transitions; broader labels such as entry/exit are unnecessary to implement them. Actual projectile outer mask `2480000F` passes through original `16A560` to lower flags `9`. That conversion extracts outer bits 23..28 into lower bits 0..5. The old supplied-intersection impact fixture used lower flags `100`; it proves the later field/event consumer, not this real geometry query.

When a candidate succeeds, `1E20C0` stores crossing fraction and its plane pointer, candidate leaf, surface index, signed surface plane index, flags, breakable byte and model-material entry. If no candidate succeeds it records visited nonempty leaves, up to 256, and updates descriptor prior leaf/class. The trace preserves those original operations.

## 2D references, projection and surface edges

`1E23F0` iterates the candidate leaf's references in authored order, requiring `(referencePlane & 7FFF) == priorCrossingPlane`. Dominant projection axis ties prefer Z, then Y, then X: choose Z if absZ>=absY and absZ>=absX; otherwise Y if absY>=absX; otherwise X. Projection orientation is `(plane[axis] > 0) != referencePlaneBit15`.

The unchanged coordinate-order table is 36 bytes at original VA `4471E4`. `analysis/multiplayer/map-pack/spartan-projection-table.json` pins its bytes/hash. Each of its six 3-i16 rows is indexed by `2*axis + orientation`; the first two entries select the projected coordinates. Rows are `(2,1,0)`, `(1,2,0)`, `(0,2,1)`, `(2,0,1)`, `(1,0,2)`, `(0,1,2)`. Load this user-owned original table through the existing original-data provider rather than embedding a substitute in production.

The base image has exactly the same 36 bytes at VA `440B94`, SHA-256 `afaf7ee3fe87b6989541d81f79a6a737b1e7cadede1ab3ed6737c6a29b43c5db`. `tools/verify_collision_projection_table.py` pins the base full-image SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` and the above TU pin, checks both data and actual instruction references. Base references at `1DEB1B`/`1DEC3D` correspond to TU `1E250B`/`1E262D`. Its paired report is `analysis/multiplayer/map-pack/paired-spartan-projection-table.json`.

The local crossing point is `delta[k]*tCross + origin[k]`, separately rounded in each component. `29C7D0` traverses signed i16 2D nodes using `(ny*y + nx*x) - D`: negative selects +C, zero or positive selects +E. Signed negative child -1 returns no surface; other negative children return `child & 7FFF`.

Only the class-2-to-class-2 candidate requests `1E25C0` explicit surface edge containment. Its optional breakable bitset rejects a breakable surface when its indexed bit is clear and the index is within the supplied count. The stock query supplies no bitset. It begins at the surface's first edge. For each edge, `r = (edge.rightSurface == selectedSurface)`; A is vertex r, B is vertex 1-r, and the next edge is +4+2*r. In the selected projection:

```
cross = (B.y-A.y)*(point.x-A.x) - (point.y-A.y)*(B.x-A.x)
```

Each subtraction/multiply is separate. Cross > 0 rejects; equality is accepted. Walk until the next edge returns to the original first edge. Preserve the authored edge graph; triangle fan expansion is not this routine.

## Outer result and ties

`16AF90` accepts a lower result only if lower fraction is strictly less than the caller's previous fraction. The standard fixture initializes previous fraction to 1, so a lower endpoint at exactly 1 cannot replace it. The accepted output includes kind 4, fraction, world plane, object datum, region, node, packed BSP identity, surface, model-material entry and global-material index. Its position fields remain caller-owned: the full outer `16B240` computes/corrects the world point later. A direct helper fixture with zeros in those position fields does not assert a world hit point of zero.

Within `1F3900`, the original authored BSP order and changing interval bound keep the earlier authored hit when fractions are equal. The suite executes real arms and body BSPs with explicitly arranged node matrices: node 0 identity; node 18 translated on x by bits `BD304394`; all other nodes translated by +100 on x. Arms alone and body alone both return `3EFAE313`. Combined, both lower and upper helpers retain arms region 0/node 18/surface 8 rather than body region 1/node 0/surface 0. Neighboring node-translation bit fixtures `BD304393` and `BD304395` give the same exact tie and selection. No original metadata was changed.

The caller-fraction fixtures independently confirm outer strict comparison: previous bits `3EFAE312` or `3EFAE313` reject the canonical hit; previous `3EFAE314` accepts it. Endpoint fixtures shorten the canonical delta to float-rounded `4*fraction`, and inspect its neighbors. The derived endpoint and two smaller float-bit values miss in the lower and upper helpers; one larger value hits with fraction `3F7FFFFF`, two larger with `3F7FFFFE`. This is a pinned finite segment observation, alongside the source's exact >= plane-side and fraction comparisons.

## Current coverage and explicit producer boundary

The frozen suite contains 292 complete unchanged queries and 234 hits, including 192 rays through the average of authored BSP vertices (16 BSPs, three axes, four pose fixtures). All 192 hit. Python constructs those explicit ray inputs; it supplies no expected hit. Pose fixtures are identity matrices; matrices obtained by running original `142D00` on each authored default inverse; the latter shifted/rotated 90 degrees about Z; and the latter scaled by 2/shifted. These are recorded matrix inputs, not a claim that the original live animator uses that static pose at a given frame. Active/inactive/invalid permutations and flags 0/1/2/3/9/256, prior fractions, endpoints, inside/reverse/away/zero-length rays and exact posed BSP ties are exercised.

The stock complete queries never reach `1E25C0`, so the separate primitive suite executes that unchanged helper directly on all 191 actual Spartan surfaces, edges and vertices. It supplies the recorded projection axis/orientation and 2D point, with no breakable bitset. Its 2,350 cases include centroids, distant points, vertices, edge midpoints and adjacent float-bit values; 1,696 are accepted. All 191 centroid points are accepted and all 191 distant points rejected. Each edge's original computed cross bits are recorded. This proves the edge helper under explicit inputs, not that the stock outer query invokes that special branch.

Canonical identity-pose ray origin (-2,0,0), delta (4,0,0), all regions active returns fraction `3EFAE313`, region 1, node 0, packed BSP `01000000`, surface 0, material entry 1, global material 56. World normal bits are `BF800000/354CC003/B5B7283D`; world D is `3D239DA4`.

The native collision module can consume the real schema and explicit current matrices/permutation bytes now. Full live parity still needs the original node-pose/region-state producers and normal world broadphase/location call context. Do not put the authored inverse fixture into live player state under an inferred animation claim.

The direct `16AF90` fixture passes its eligibility/bounds-bypass argument as true, and sets sibling/child object links to -1. That isolates the posed intersection and selected result without claiming to execute `16A490` eligibility, `11FD00` object bound testing or child-object traversal. Original normal projectile masks and original model/material lookup still execute.

## Existing native BSP parser

`src/content.cpp`'s `Cache::load_bsp_content` reads scenario BSP 40-byte records, while object `coll` records have the additional node prefix and 44-byte stride. Its `collision()` helper flattens surfaces into triangle fans and preserves surface index, signed plane index, normal and scenario/instance identity. It discards plane D, 3D nodes/leaves, 2D references/nodes and the authored edge/vertex graph. `BspContent` contains only expanded triangles and navigation sectors. The checked block/float reading helpers can be reused; those flattened triangles cannot replace this original posed BSP query without losing authored traversal and identity. A raw collision BSP representation is needed.

## Normalization and clear range-40 callback evidence

`tools/oracle_projectile_boundaries_1_1.py` SHA-256 `0a31cc0d185fd2a5a77e0687861e9aa0cd3b77b1f6b3aa995eb1e380a00349cf`; output `analysis/multiplayer/map-pack/projectile-boundaries-1.1-oracle.json` SHA-256 `6957b2a833f1514d5e54dc0eeb0f27747a54edfb1ee7fc7d52b84c7e140b5825`.

It executes original `30BD0` for seven recorded input vectors. Epsilon is VA `46504C`, bits `38D1B717`, float 0.00009999999747378752. A length below epsilon returns zero and leaves the input vector unchanged. Equality normalizes. This epsilon belongs to vector normalization; it is not a tolerance added to the above collision planes.

The same report extracts unchanged frozen clear-range inputs/outputs from `smg-impact-suite/clear_range_sequence-mode1.json` (SHA-256 `d7c91437c6aed6fc641ddfc811481b0c54389e88f35757551460d8110d192413`). Each tick includes full 428-byte projectile state, object header, position/velocity/distance and original callback events. Original `FCEC0` receives PI `10002`, delta time bits `3D088889`, tick rate 30, supplied zero gravity with authored SMG air gravity zero. Travelled-distance bits after ticks 1,2,3 are `41555555`, `41D55555`, `42200000`; state is `1300`, header `0A`. Tick 4 retains distance `42200000`, sets state `1301` and header `1A`; there is no further RNG consumption. Later original removal clears the header/pointer/salt as detailed in `docs/SMG_IMPACT_1_1_RESEARCH.md`.

This is disc 1.1 evidence. It does not establish the final update released for Halo 2 or complete multiplayer parity.
