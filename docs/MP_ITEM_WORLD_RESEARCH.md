# Original TU1.1 world-weapon initialization and first item update

Research date: 2026-10-08. This continues [the inventory research](MP_INVENTORY_RESEARCH.md) and supplies evidence for the world objects requested by the recovered netgame timer. Production code is untouched.

The narrow implementable result is the original object identity, item/weapon initialization, authored world scale, spawn transform, registration order, and new-object tick admission. Complete physical settling remains unresolved: the report executes the real Lockout BSP plane/surface path, but supplies an unavailable Havok shape query and excludes the general physics stage. SMG animation-resource and marker leaves are also explicit inputs. Do not turn those inputs into unconditional live fallbacks.

## Reproduction and authority

Run:

```sh
.tools/python/bin/python tools/oracle_mp_inventory_world_1_1.py
```

[The tool](../tools/oracle_mp_inventory_world_1_1.py) writes [the ignored report](../analysis/multiplayer/integration/inventory-source/world/report.json). It contains nine cases, 99 complete routine rows, actual collision request/result bytes, heap requests, source call order, and 93 original instruction listings. Source instructions are unchanged; every supplied service is identified separately, including which services each case actually used.

The sources are the untouched TU1.1 `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03` (SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`) and the untouched `assets/disc/maps/lockout.map` (SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`). The fixture restores every main-metadata definition/model pointer to the real cache address. It also maps the original scenario BSP allocation separately, as the executable does. The existing frozen 48-case grenade report is unchanged.

The cases cover Lockout netgame row 0 (levitating BR), row 1 (ordinary BR), and row 12 (ordinary SMG), first with world registration disabled and then with an active world. Three additional cases carry and drop those same allocated identities, using an explicitly supplied unit root-node pose derived from authored player spawn 0. That unit fixture does not recover the full unit graph, hand-marker pose, or unit update.

## Allocation and ownership order

The full original [15C110 spawn routine](../analysis/multiplayer/integration/inventory-source/world/instructions/0015c110.asm) now calls the full original [B8A30 allocator](../analysis/multiplayer/integration/inventory-source/world/instructions/000b8a30.asm), instead of receiving a preconstructed object. The earlier netgame tool remains a valid timer/allocation-request slice; it does not prove these new allocator internals.

`B8A30` performs the following relevant stages in order:

1. Unless descriptor flags `+18` bit 4 bypass them, validate the definition and invoke pre-create callbacks through `10CBD0`. `10CA30` identifies object type by the definition datum's tag-table group. Supplying only tag payload pointers is insufficient for this path: the runtime tag-table record also needs its real group at `+0`.
2. Read type from definition `i16 +0`, select type information through `46FAD0`, and allocate the type's base size through `BD170`. For weapon type 2 the base size is **0x25C**.
3. `BD170` calls the full original `16D2C0` datum allocator. That routine chooses the free slot, sets its bitmap bit, zeroes the header record, assigns its salt, and advances pool cursors/count. Only the backing object heap allocation `18FCE0` is supplied. The caller sets header bit 3 (`0x08`, new object), header type 2, and the real weapon definition datum.
4. Fill dynamic creation identity, root pose/velocities/scale, ownership/link sentinels, and runtime flags. Allocate object extra blocks through the full original `BD270`; only the heap storage growth `18FD70` is supplied.
5. Run type constructors through `10CC20`, then variant/color/damage initialization, model-node and bounds update, world location/registration, update admission, physical activation, and post-create callbacks.
6. Return the salted datum to `15C110`. The spawn routine binds that datum to the original row and writes the row index to object byte `+AF`. Only afterward does it apply the authored Levitate option and notify the game engine. These final row operations are not part of `B8A30`.

The weapon type information at `46F170` has an inline hierarchy beginning at `+84`: object `46ED88`, item `46F0A8`, weapon `46F170`, then zero. Constructor slot `+2C` is zero for object, `F9900` for item, `101BB0` for weapon. Post-create slot `+38` is `BF650` for object and zero for the other two. Update slot `+40` is zero for object, `F9B10` for item, and `1021D0` for weapon. `10CDF0` calls updates in that order and ORs their byte results. These are actual original table entries, not names inferred from nearby routines. [Constructor dispatch](../analysis/multiplayer/integration/inventory-source/world/instructions/0010cc20.asm), [post-create dispatch](../analysis/multiplayer/integration/inventory-source/world/instructions/0010cd10.asm), [update dispatch](../analysis/multiplayer/integration/inventory-source/world/instructions/0010cdf0.asm).

The original allocation primitive preserves identity through carry and drop. It must not be replaced by changing a player's current weapon definition. In these cases the same `00010002` datum and object address survive pickup/drop; `+AF` remains the originating row until the netgame replacement/unbind path clears it. The fixture's datum and address are supplied storage choices; the source owns their allocation and later transitions.

## Spawn and constructor state

The allocator copies descriptor fields directly:

| Descriptor | Runtime object | Value/source |
| --- | --- | --- |
| `+00` | `+00` | Definition datum |
| `+1C/+20/+24` | `+64/+68/+6C` | Root position |
| `+28/+2C/+30` | `+70/+74/+78` | Forward |
| `+34/+38/+3C` | `+7C/+80/+84` | Up |
| `+40/+44/+48` | `+88/+8C/+90` | Linear velocity |
| `+4C/+50/+54` | `+94/+98/+9C` | Angular velocity |
| `+58` | `+A0` | Initial scale, subsequently replaced by item initialization |

`B8820` supplies zero spawn velocities. `15C110` copies the authored position and converts the authored Euler values through unchanged `11F680→143450`. It does not force all weapon spawns upright; row 0 retains its authored tilted basis. [Descriptor creation and conversion](../analysis/multiplayer/integration/inventory-source/netgame/instructions/000b8820.asm), [Euler conversion](../analysis/multiplayer/integration/inventory-source/world/instructions/0011f680.asm).

`F9900` sets object flag `+4 |= 0x1000`; item ignore/last holder `+14C = NONE`; owner `+154 = NONE`; creation tick `+150 = TIME+8`; spin axis `+158` to original up vector; sine `+164 = 0`, cosine `+168 = 1`. The base object buffer was zeroed by `BD170`, so initial item flags `i16 +12C` are zero. It invokes `FC1E0` for item scale. [Original item constructor](../analysis/multiplayer/integration/inventory-source/world/instructions/000f9900.asm).

`FC1E0` selects a scale from the real item definition: when item flag bit 0 is clear, multiplayer options state 2 selects definition `f32 +C4`; other states select `f32 +C8`. A positive selected value is clamped to source bounds **0.5–3.0**; otherwise scale 1 is used. Held items use scale 1. `B8570` stores that scale and rebuilds nodes/bounds. It replaces the descriptor scale; it does not multiply it. [Item scale](../analysis/multiplayer/integration/inventory-source/world/instructions/000fc1e0.asm), [scale update](../analysis/multiplayer/integration/inventory-source/world/instructions/000b8570.asm).

For the actual Lockout definitions:

| Weapon | Definition datum/file root | World scale/bits | Loaded/reserve | Heat `+184` |
| --- | --- | --- | --- | --- |
| BR | `E192001D`, `01D32E00` | 1.7, `3FD9999A` | 36 / 72 | 0 |
| SMG | `E57B0406`, `01DE97E4` | 1.5, `3FC00000` | 60 / 120 | 0 |

The unchanged weapon constructor initializes those ammo values from the actual magazine definition, not from the row respawn timer. The magazine runtime record starts at object `+224`; reserve is `i16 +22A`, loaded `i16 +22C`. [Original weapon constructor](../analysis/multiplayer/integration/inventory-source/world/instructions/00101bb0.asm). Both world spawns consume zero shared RNG draws in this bounded default single-entry collection path.

The observed object extra blocks are:

| Header | BR size/offset | SMG size/offset |
| --- | --- | --- |
| `+10C` | 0 / `FFFF` | `0080` / `03D0` |
| `+110` | 0 / `FFFF` | `0080` / `0340` |
| `+114` | `00D0` / `0264` | `00D0` / `0264` |
| `+118` | `000A` / `0334` | `000A` / `0334` |
| `+11C` | `0008` / `025C` | `0008` / `025C` |
| `+120`, `+124` | 0 / `FFFF` | 0 / `FFFF` |
| `+128` | 0 / `FFFF` | `0090` / `045C` |

Each header is `i16 byte_size, i16 object_relative_offset`. Final allocated byte sizes are BR **0x33E**, SMG **0x4EC**. `BD270` adds `(1<<align_bits)-1` bytes before rounding the block offset, and zeros the whole appended range. The SMG's animation-controller availability is determined by original `1CEAC0` against real model/graph metadata; its animation command/resource leaf `1D0A60` is supplied unavailable in this report. Do not generalize these block sizes to another definition or availability condition. [Block allocation](../analysis/multiplayer/integration/inventory-source/world/instructions/000bd270.asm), [original graph admission](../analysis/multiplayer/integration/inventory-source/world/instructions/001ceac0.asm).

## Nodes, bounds, and spatial registration

`BDF10` runs `BDF80` model-node update, then `BEDE0` bounds update, then recursively updates children. `16F3C0` reads real render-model bind-node records (`stride 0x60`, quaternion `+18`, translation `+0C`) into the original local node format. Original matrix conversion/composition generates the object nodes. The BR has no admitted runtime controller in these cases; SMG animation commands are unavailable at the explicit resource boundary, so its nodes follow the recorded bind-pose path. These results are not a complete SMG idle-animation pose claim. [Node generation](../analysis/multiplayer/integration/inventory-source/world/instructions/000bdf80.asm), [bind-node reads](../analysis/multiplayer/integration/inventory-source/world/instructions/0016f3c0.asm).

`BEDE0` computes bound radius `object +3C = object scale +A0 × definition radius +4`. It transforms definition center `+8/+C/+10` through the first runtime node matrix referenced by object short `+116` to write bound center `+30`. It also writes the registration sphere `+40/+44/+48/+4C` from the selected bounds; changed registered bounds unregister/re-register. The radius is not the biped movement capsule radius. [Bounds ownership](../analysis/multiplayer/integration/inventory-source/world/instructions/000bede0.asm).

At initial world scale the original outputs are BR radius `3E828F5D` (~0.255), SMG `3E3851EB` (~0.18). The initial centers differ from root positions. For SMG row 12 the real model bind-root translation produces center `[-1.50632643699646, 10.648445129394531, -12.178000450134277]`, even though definition center `+8/+C/+10` is zero. The earlier inventory/target fixtures supplied scale 1 and a root matrix; those were conditional math/admission tests, not world-allocation results.

The scenario BSP row at file `01D29520` supplies file offset `00EC0E00`, size `0009A000`, original memory base `800AB000`, datum `F2E0115B`. Its allocation header points to root `800AB010`; root `+18` points to collision BSP `800AB3C8`. The executable's original BSP switch fragment `11D8ED..11D90E` writes `4E7918 = root`, `4E7910 = root+18 pointer`, `4E790C = root+18 pointer`, and BSP index `46FB64 = 0`. These are separate from the main metadata magic.

With world globals `4E58C4` byte 0 active, `B8A30` calls `C0400` to locate the new object, then `B94F0`. Original `11D4A0→14B9D0` uses Lockout's real BSP nodes/planes and leaf→cluster mapping. Actual initial locations (leaf, cluster, BSP) are row 0 `(1761,9,0)`, row 1 `(1352,9,0)`, row 12 `(2243,15,0)`.

`B94F0` writes location `+28/+2C/+2E` and header cluster short `+4`, selects collidable registry `4E58B0` when object flag `+4 bit9` is set (otherwise `4E58A4`), and calls `1CE650`. `1CE650→14BD00` finds actual sphere-overlapping clusters and uses original datum allocation to build both reverse object→cluster links and cluster→object links. Registry triplets are heads pointer `+0`, cluster→object pool pointer `+4`, reverse pool pointer `+8`. The reverse record stores cluster `+4` and next `+8`; the forward record stores object datum `+4`, next `+8`, and the copied 0x14-byte spatial payload `+0C`. The tool supplies their empty backing arrays/pool metadata; the source chooses and links entries. [World registration](../analysis/multiplayer/integration/inventory-source/world/instructions/000b94f0.asm), [membership allocation](../analysis/multiplayer/integration/inventory-source/world/instructions/001ce650.asm).

Registration sets object flag `+4 bit8` and header bit6. `B7C50` tests the owning cluster's activation bit in options `+11B8`; `B8180` sets header bit0 active only when the cluster is valid and the object is unparented. `B8250` sets header bit1 update admission. `B9780` sets object `+C0 bit6` and wakes an existing physical component if `+B4 != NONE`. For these actual weapon definitions `+B4` stays `NONE`; a fabricated Havok component is not required to execute the item callback. The report's active-cluster mask is an explicit all-active input, not a recovered player-driven activation schedule.

## First normal item tick and source physics boundary

The surrounding original tick `139750` visits netgame timers inside `15D7C0` before object stages `B7E00`, `C7660`, `B7F50`, `B8040`. A weapon created by the timer has header bit3 (`0x08`). **`B7E00` and `B7F50` skip that bit. Its first object tick belongs to `B8040` later in the same tick.** `B8040` requires header bits `8|1|2`, excludes bit `0x10`, clears new bit8, calls `BD360`, and calls `BD710` if header dirty bit4 is set. Pending-weapon/projectile postpass `1019F0` follows. [Ordinary admission](../analysis/multiplayer/integration/inventory-source/world/instructions/000b7e00.asm), [new-object admission](../analysis/multiplayer/integration/inventory-source/world/instructions/000b8040.asm).

The report executes that ordering: an ordinary and dirty pass leave each newly spawned object unchanged, then the new-object pass executes item/weapon callbacks. It does not execute `C7660`'s general physics stage. Subsequent ordinary admission requires header bits `1|2`, excludes new bit8 and object `+C0 bit3`. `BD360` rejects header `0x20` already-updated objects; it calls the hierarchy updates, damage update and `BF7D0`, and combines wake conditions. Positive object byte `+B3` decreases and keeps the object scheduled; an otherwise quiet object loses header update bit2. [Object update wrapper](../analysis/multiplayer/integration/inventory-source/world/instructions/000bd360.asm).

`F9B10` runs world item motion only while object registered bit `+4 bit8` is set. Nonlevitating items invoke `BC910`; `F9EA0` then performs the original integration/collision response. Its nonlevitating branch reads the tick delta and gravity, applies gravity to vertical velocity/displacement, submits mask `24909C0D` segment queries through `16B240`, and handles actual collision type/material/normal results. Angular/spin updates and contact state are source code, not host gravity/friction code. Levitating, held, stuck-to-BSP and stuck-to-object branches are separate source paths. [Item tick](../analysis/multiplayer/integration/inventory-source/world/instructions/000f9b10.asm), [motion and response](../analysis/multiplayer/integration/inventory-source/world/instructions/000f9ea0.asm).

The original physics-constant initializer `1EEFE0` writes runtime gravity from XBE float `44C7A8`: **4.17125940322876**, bits `40857AF5`. The fixture executes the initializer into supplied storage at global `526254`. At 30 Hz, an initial nonlevitating no-contact tick writes vertical velocity `BE0E6106` (~−0.13904199). Root displacement is computed by original floating-point instructions and then re-read from the rounded root position; do not substitute a hand-coded constant displacement. [Physics initialization](../analysis/multiplayer/integration/inventory-source/world/instructions/001eefe0.asm).

Bounded actual-row results:

- Row 0 BR: authored Levitate changes `+C1 bit0`; first item tick makes no collision request and keeps the original root/basis.
- Row 1 BR: first source collision query reports no hit; root Z becomes −5.842135429382324, with the source vertical velocity above. The report stops after four item ticks, before its later impact sound/effect allocation path.
- Row 12 SMG: first source plane/surface query hits type 1, material 104, fraction bits `3E5D061C`, point Z −12.200000762939453. The original response records BSP/surface/material contact, sets item flag bit5 (`0x20`) and `+C1 bit0`, and leaves the root at authored Z −12.199000358581543 with zero linear velocity. This result is conditional on the stated Havok shape-query boundary; it is not proof that every original collision service returns the same winning contact.

`16B240` combines the plane/surface BSP path `1E2020→1E20C0/1E23F0`, Havok cluster shape query `246A50`, and registered object queries `B9830→16AF90`. The first and registry lookup paths execute unchanged with real Lockout data. `246A50` attempts to construct/query runtime Havok shape objects, which requires original shape/resource/runtime initialization not present in the fixture. It is supplied unavailable and recorded. Earlier tools' clear `1E20C0` service must not be carried into live world settling: it changes row 12 from contact to falling. Static triangle rendering meshes or a guessed floor plane do not replace this source collision contract.

## Carry, drop, and the next item update

The three additional cases start with full world allocation and first-item admission. Supplied ammo/heat inputs are loaded 7, reserve 11, heat 0.5. Complete original `CE510` transfers the same datum to the unit; original `FB120/FB3F0` changes held flags and removes world registration. The held scale becomes 1; ammo and heat remain 7/11/0.5.

Complete original `CFD80` uses slot `i16` stack argument, event/mode stack argument, return-to-other-weapon byte stack argument, and unit datum **EBX**. The report calls `(slot0, mode0, choose-other1)` with EBX equal to the supplied unit. It reaches the original removal/drop chain `CFED0→CF8C0→CFB00`, `FB120`, marker attach/detach, original registration, velocity impulse, and spin setup. [Slot removal](../analysis/multiplayer/integration/inventory-source/world/instructions/000cfd80.asm), [world drop](../analysis/multiplayer/integration/inventory-source/world/instructions/000cfb00.asm), [detach and register](../analysis/multiplayer/integration/inventory-source/world/instructions/000ba980.asm).

The drop restores world scale BR1.7/SMG1.5, owner `NONE`, and creation tick to the drop tick; it records last holder `+14C = unit`. Same datum, row ownership, ammo and heat survive. All three controlled drop cases consume four shared RNG stores, seed `12345678→75432777→CD305E6A→25DBFAC1→4B5C952C`, matching the existing inventory drop oracle. The unit's aim is explicitly supplied `[1,0,0]`; hand-marker services are unavailable and the source follows its recorded root-node fallback. Those fixture velocities and positions are not an authored universal drop direction/pose.

Drop registration produces header `0x67` (registered, already-updated, dirty, scheduled, active). `BD360`'s bit `0x20` check prevents eager duplicate motion. `B8040` clears already-updated bit `0x20`; in these cases the same-tick postpass does not move the dropped item. The next tick's ordinary item update runs from the recorded state and applies source motion. The supplied parent's full unit update is excluded from this bounded item experiment; native unit/animation/aim admission still needs its own source evidence.

## Remaining limits

Recovered fields and lifecycle ordering above can support a bounded allocation/state/registration contract. Full world settling still requires original Havok BSP shape initialization/query, the general `C7660` stage, and later collision impact effect/audio services. SMG world animation-resource and hand-marker pose initialization remain explicit boundaries. Active-cluster scheduling is supplied, as is heap/pool backing storage; allocator exhaustion and graph/shape resource failure combinations are not covered by this report. No networking/world-authority registration proof follows merely from these local object writes.

Do not add guessed gravity, friction, floor heights, universal idle poses, or a definition-only pickup replacement. Preserve source row indices, salted identities, shared RNG order, and the distinction between the new-object postpass and ordinary object pass.
