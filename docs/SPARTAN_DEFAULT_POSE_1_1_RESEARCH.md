# Original disc 1.1 Spartan default pose producer

The original executable builds all 33 Spartan current node matrices from the actual render model's default local node transforms. It also resolves the authored default model variant and initializes the real active region-permutation bytes. This closes the static default-pose producer boundary for the recorded fixture. It does not establish ordinary animated multiplayer poses: the animation manager is explicitly inactive in this oracle.

## Reproduction and authority

```sh
.tools/python/bin/python tools/oracle_spartan_default_pose_1_1.py
.tools/python/bin/python tools/oracle_spartan_default_pose_1_1.py --instruction-inventory analysis/multiplayer/map-pack/spartan-default-pose-original-instructions
.tools/python/bin/python tools/verify_collision_projection_table.py
```

Full original inputs remain pinned: disc 1.1 `default.xbe.03` SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`; Lockout SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. This tool verifies its frozen collision-oracle dependency SHA-256 `99beb6fc599d06086fe8321a2b7346b1e494c77d60112c8622c2d57a85830e8e`. It runs original instructions unchanged. No engine service executes in any recorded case.

| Research output | SHA-256 |
|---|---|
| `tools/oracle_spartan_default_pose_1_1.py` | `d3a8a87bcadd810ff0e47beb57f0b03fa1fc592164cb64cfe8819eccf7c6a0d4` |
| `spartan-default-pose-1.1-oracle.json` | `31f62d65ad2abe95bcfc70e0afb67449704cabd6e0a3d39883010cca77124504` |
| `spartan-default-pose-original-instructions/inventory.json` | `000654172014ecad923eac07f313c184f5a945d67e61459f913568e4968b8514` |
| `tools/verify_collision_projection_table.py` | `6204e36128d442ceeb0c9f5f158fdd60f263d2268dd73d68908bdc0e99e9efe2` |
| `paired-spartan-projection-table.json` | `83a48e33db3a9ee2c5c6b7adf94eca42345f83601b503e2ea773582ecec17903` |

All report paths are under `analysis/multiplayer/map-pack/`. The pose inventory has 28 traced routines, including two actual indirect callback entries that the prior Ghidra export did not identify as separate functions (`C52C0`, `114580`). The source bytes/explicit call entries establish those boundaries; a missing decompiled function file is not evidence that the callback is absent.

## Real metadata route

The route is actual Spartan `bipd E9F2086D` -> model at bipd+38, `hlmt E9F3086E` -> render model at hlmt+4, `mode E9070782`, `objects\characters\masterchief\masterchief`, root file offset 31991196. The previous collision oracle's supplied hlmt-inverse matrices are independent pose fixtures; this producer consumes the render model node records that the original code actually reads.

The mode node block is +48/+4C, stride 60, count 33. Each original record contains StringID +0, parent i16 +4, first-child i16 +6, next-sibling i16 +8, local translation floats +C/+10/+14 and local quaternion floats +18/+1C/+20/+24. The report records every complete original node record and names resolved through the real cache StringID table.

The bipd's authored model-variant StringID at +30 is `07000001`, `default`. No variant override is supplied. Original `BB430` reads +30 when its ECX override is zero, then calls `16EC00` to select variant index 0 and writes object byte +B1. This is source-resolved selection rather than an inferred permutation.

## Original initial region state

The fixture executes `BB480(UI, 0)` after `BB430`. Its mask argument is explicitly zero; the complete object-spawn descriptor that supplies that mask is outside this fixture.

`BB580` obtains region count as signed object size +118 divided by 10 and returns pointers to active permutation bytes at region-state base, the next count bytes, and the 8-byte-per-region state entries at base+2*count. `BB480` calls original `16ED00` using the actual model, selected variant and those pointers. Candidate construction and `16F550`/`16F760` execute on real metadata. All four active model permutation bytes are zero. They map to the real collision permutations through the already verified `1F3590` query.

The resulting 40-byte region-state block in this fixture is:

```
0000000000000000000000eeffffffff000000eeffffffff000000eeffffffff000000eeffffffff
```

The `EE` bytes are deliberate initial-storage sentinels retained by the original producer; they are not authored initial values. Preserve the actual written-field boundary. There are zero shared-RNG stores in all 24 default-selection cases because each executed selection has one eligible candidate.

## Default local transforms to world matrices

The fixture supplies an unparented object position at +64, forward vector at +70, up vector at +7C, scale at +A0 and mirror flag bit 10 at +4. The exact matrix/region storage offsets are explicit arena allocations. There is no animation manager (+12A=-1), physical body (+B4=-1), object parent (+14=-1) or unit attachment. Object +34B is zero. Local transform storage is present at object-relative +112, and the current matrix count/offset are +114/+116.

Original `BDF80` resolves the actual render model and performs:

1. `16F3C0`: copy each mode node's four quaternion floats and three translation floats into its 32-byte local transform, append scale 1.
2. `BEB30`: construct the world object matrix from the explicit position/forward/up/scale/mirror and no parent matrix.
3. `143960`: convert each local quaternion/translation/scale into a 52-byte matrix.
4. `1441D0`: compose the root local matrix with the world object matrix; for other nodes compose their local matrix with the already produced matrix selected by the authored parent i16.
5. `10D260`: dispatch original type-specific root pose callbacks. The actual unit descriptor at VA `46EE50` selects `114580`; the unit attachment datum is explicitly -1, so that callback returns without an attachment transform.

The source `C52C0` unit initializer allocates the unit-extension block at field pair +33C/+33E and calls `110A20`; `110A20` writes extension+28 to -1. The oracle supplies that exact inactive attachment field/storage boundary, rather than allowing an uninitialized extension pointer to accidentally select another object.

The report records every original local transform, all current matrices and input/output bytes at each `143960`/`1441D0` call. It also records all metadata lookup/selection calls and original RNG stores. Floating-point state is the existing original oracle's x87 `027F`, MXCSR `1F80`.

`143960` uses original `RCPSS` plus separately rounded reciprocal refinement and quaternion arithmetic. `1441D0` uses original packed SSE multiplies/adds with component-specific order. Do not replace those operations with a quaternion library or reassociate them under a parity claim; the attached instruction listings and stage outputs establish a direct native differential boundary. This execution is instruction-oracle evidence, not a physical Xbox CPU capture.

Canonical position zero, forward (1,0,0), up (0,0,1), scale 1, mirror false gives root position bits `00000000/00000000/3EC9ECCB` (z 0.3943847119808197). Its complete root matrix is:

```
0000803f000080330000b8b50000803fffff7f3f00006436000000b3000062b60000803f0000c0350000000000000000cbecc93e
```

## Coverage and limits

The separate suite runs 24 original producer cases: two placements, three forward directions, and scale 1/2/0.5 or the mirror branch. It records 792 complete node matrices. Each case feeds those original produced matrices and original selected permutation bytes into the frozen original posed collision consumer; 14 of the explicit world segments hit. No native implementation supplies expected matrices or collision outputs.

This producer is safe to port as the original default-pose fallback under the same explicit inactive-animation conditions. It is not sufficient to replace moving players' poses with a static default pose.

## Next ordinary animated producer and phase evidence

The original startup path `B8A30` calls `BB430`, `BB480` and then `BDF10`, which calls `BDF80` and the object bound producer `BEDE0`. `BDF80` selects the animated path only with a valid +12A manager and a valid graph at manager+68, with its other source gates satisfied. That path calls `BEA50`: original animation sampling (`1D0F80`/`1CA950`), effect/secondary sampling (`BE860`) and the unit's local-pose callback (`10D1E0` -> actual callback `113830`), then converts/composes the resulting local transforms. Its full graph/state/resource inputs and decoded animation samples remain to be recovered alongside the renderer's pose work.

Normal tick order is established by original `139750`: `B7E00`, then `1C7660`, then `B7F50`, then `B8040`. In `B7E00`, each eligible object runs `BD360`; with a valid animation manager, that calls `1D0ED0` before the type-specific object update. `B7F50` runs `BD710` for updated existing objects, which calls `BDF80` for the Spartan type before the later `1019F0` projectile dispatch pass. `B8040` performs the corresponding update/pose pass for newly created objects, another `1019F0` dispatch and object removals. These are inspected original ordering statements; this bounded tool does not execute the full multiplayer tick.

The remaining live integration gate is the real +12A animation manager/selected graph state and sample producer, including unit callback inputs, physical pose overrides, current unit mode +34B, parent/attachment and animation-region transitions. The named original code locates that gate; no fallback state or guessed idle clip is being substituted for it.
