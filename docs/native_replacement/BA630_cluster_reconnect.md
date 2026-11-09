# BA630: object reconnect to structure-BSP clusters (TU1.5), native replacement spec

Status: read-only research spec, 2026-11-08. Nothing here has been built or run.
Source of truth: the translated bodies in
`src/physics/physics_live_support_tu15_generated.cpp` (abbreviated **LS** below)
and, for the two Havok World-module services, in
`src/physics/runtime/physics_world_runtime_generated.cpp` (**WR**, canonical
addresses mapped through `world_observation_pc`). Line numbers refer to the
working tree on that date. Instruction counts are the number of lines in each
body that contain `s.services.instruction`. A `rep stos`/`rep movs` line counts
once.

## 0. Summary and corrections to the task brief

* BA630 is TU1.5's `object_reconnect_to_map(object_index, location*)`. It is
  the same routine that the retail-image research calls **B8600** (see
  `docs/SOURCE_OBJECT_ADMISSION_PARITY.md` and
  `docs/SHIELD_OBJECT_REGISTRY_RESEARCH.md`). The retail/TU1.5 equivalences
  used by the existing host models are:

  | retail (host-model naming) | TU1.5 (this spec) | role |
  |---|---|---|
  | B8600 | BA630 | object reconnect / registration |
  | 1CAC60 | 1D2F10 | cluster-partition reconnect (membership + list insertion) |
  | 14A5B0 | 14D0E0 | portal flood fill |
  | 14A370 | 14CEA0 | sphere-vs-portal test |
  | 23A160 | 243450 | 2-D circle-vs-portal-polygon test |
  | 11BED0 -> 14A280 | 11E600 -> 14CDB0 | point -> location / BSP3D leaf |
  | 1CADF0 / 1CAE40 | 1D30A0 / 1D30F0 | payload update / partition disconnect |

* BA630 does **not** call 14D0E0, 14CDB0 or 14CEA0 directly. The paths are:
  * `BA630 -> 11E600 -> 14CDB0` (location from point, only when the caller passes NULL),
  * `BA630 -> 1D2F10 -> {14D080 | 14D0E0 -> 14CEA0 -> {123210, 243450}} -> 16E6A0` (cluster list and list insertion),
  * `BA630 -> C0F60 -> C4D30 -> {11E600, 1D2F10 (light partition 0x4ECBE0)}`. Lights attached to the object are reconnected through the same flood code.
* After the cluster work, BA630 also runs: the light-attachment recursion
  (C0F60), the activation decision (B8D90 -> B92C0 activate, or B1870 -> BA570
  delete, or B9330 deactivate), and, only when the object was previously
  unconnected, a Havok tail (1496F0, 1CC290, 282C10, 1496F0).
* The B9460 ordering in the brief matches the code: `B9460` calls `1CD620` at
  B958A, then `BF050` (conditional) at B959C, then `BA630` last at B95BB. That
  call happens only if object flag bit 8 was set when B9460 was entered and
  B9460's 9th argument byte is 0. In the `set_object_position` path
  (`B95D0(object,0,0)` -> `B9460(..., arg5=B95D0.arg2=0, ...)`), BA630
  receives **location = NULL**, so it always runs both 11E600 lookups.
* The hot leaves (docs/NATIVE_REPLACEMENT_GOAL.md census, Zanzibar 1p, per
  frame) are 14CEA0 (118k instructions), 14D0E0 (103k), 14CDB0 (57k), 16E6A0
  (34k) and 1D2F10 (20k).

## 1. Call graph

```
BA630 (138)  object_reconnect_to_map                         stdcall(object, location*)  ret 8
 +- 11E600 (27) location_from_point  [x2 when location==NULL]  esi=out, edx=point
 |   +- 14CDB0 (39) bsp3d_leaf_from_point                    eax=node, ecx=bsp, edx=point
 +- BA910 (29) object_cluster_payload                        edx=object, edi=out[0x14]
 |   +- 16E290 (73) object_cluster_payload_flags             eax=object
 +- 1D2F10 (111) cluster_partition_reconnect                 eax=bitvector?, 9 stack args, ret 0x24
 |   +- 14D080 (32) clusters_from_bitvector                  edi=&total, ebx=bits, 2 stack, ret 8
 |   +- 14D0E0 (86) cluster_portal_flood (recursive)         5 stack args, ret 0x14
 |   |   +- 14CEA0 (137) portal_intersects_sphere            ax=portal, ecx=center, edx=sbsp, 1 stack, ret 4
 |   |       +- 123210 (31) dominant_axis                    eax=vector
 |   |       +- 243450 (60) circle_vs_portal_polygon         ecx=verts, edi=point, xmm0=r, 1 stack, ret 4
 |   +- 16E6A0 (78 translated; native hook) datum_new       edx=data_array
 +- C0F60 (97) object_connect_lights_recursive               stdcall 5 args, ret 0x14
 |   +- BBD10 (19) object_or_ancestor_deleting
 |   +- C5250 (20) light_disconnect -> 1D30F0
 |   +- C4D30 (262) light_reconnect -> 11E600, 1D2F10(0x4ECBE0), ...
 |   +- C3750 (73) object_owned_lights_reconnect -> 1D30F0, C4D30
 +- B8D90 (53) object_in_active_clusters                     eax=object, 1 stack, ret 4
 |   +- 10DDE0 (29) type-callback "is active" (machines only)
 +- B92C0 (33) object_activate                               edi=object
 |   +- 1CC3F0 (22) -> 1CC310 (Havok body flags; ~6.1k-instruction subtree)
 +- B1870 (17) object_delete_protected (mode 4)              eax=object
 +- BA570 (60) object_delete                                 stdcall(object), ret 4 (~3.7k-instruction subtree)
 +- B9330 (30) object_deactivate                             edx=object
 |   +- 1CC440 (47) -> E3E20, 1CC310
 +- 1496F0 (WR canonical 148360, 123) Havok service, no args
 +- 1CC290 (40) object_physics_create_recursive              stdcall(root, object), ret 8
 |   +- 1496F0, 1CC190 (74; Havok creation, ~5.9k subtree), 282C10, 1496F0, 1CC290 (recursion)
 +- 282C10 (WR canonical 27F190, 276) dirty-island processing, no args
```

Translated-instruction totals for the static closure of each subtree in LS
(distinct routines, each counted once):

| subtree | routines | instructions | notes |
|---|---|---|---|
| 11E600 | 2 | 66 | |
| BA910 | 2 | 102 | |
| 1D2F10 | 7 | 535 | 16E6A0 is already natively hooked |
| C0F60 | 46 | 2801 | plus 12 routines outside LS |
| B8D90 | 2 | 82 | plus the dynamic type callbacks |
| B92C0 / B9330 / 1CC290 | about 120-129 | about 5.9k-6.2k | Havok (1CC310 / 1D9510 / 1DDBF0 ...) |
| BA570 | 82 | 3749 | |
| whole BA630 closure | 211 | 10223 | |

The **cluster core** this spec covers in full detail is BA630, 11E600, 14CDB0,
BA910, 16E290, 1D2F10, 14D080, 14D0E0, 14CEA0, 123210, 243450 and 16E6A0. That
is about 840 translated instructions.

## 2. Data structures

All pointers are 32-bit addresses in the emulated image. "rdN/wrN" mean
little-endian reads and writes of N bits.

### 2.1 Object datum array (global `0x4ECBD0` -> data_array*)

`header = rd32(rd32(0x4ECBD0) + 0x44) + (object_index & 0xFFFF) * 12`

| offset | size | field | used here |
|---|---|---|---|
| +0x0 | 2 | salt | not read |
| +0x2 | 1 | header flags | bit0 active (B92C0 sets, B9330 clears with `&=0xFA`, which also clears bit2), bit4 0x10 being deleted (BA570 sets; 16E290 and C0F60 read it), bit6 0x40 connected to map (BA630 sets), bit7 0x80 attached/parented (host model) |
| +0x3 | 1 | object type | B8D90, B92C0, B9330, 1CC440 |
| +0x4 | 2 | cluster index (int16, -1 = unconnected) | BA630 reads and writes, B8D90, B92C0 |
| +0x6 | 2 | body size | not read |
| +0x8 | 4 | body address | everywhere |

Capacity is 2048 (`source_object_admission_memory.cpp` asserts `+0x20==2048`, stride 12).

### 2.2 Object body fields used by this tree

| offset | size | meaning (as used here) |
|---|---|---|
| +0x04 | 4 | object flags. bit1 0x2 always active (B8D90). bit4 0x10 has attachment block (C0F60). bit6 0x40 owns lights (C0F60 -> C3750). bit8 0x100 connected to map (BA630 sets; B9460 tests). bit9 0x200 collideable: selects partition 0x4EABB0 and enables 16E290 flags. bit14 0x4000 counted in `[[0x4EABC4]+4]` while active. bit17 0x20000 delete-when-deactivated (BA630 -> B1870/BA570). bit18 0x40000 outside map (BA630 sets/clears). bit20/bit22/bit31 and byte+4 bit0 feed 16E290. bit21 0x200000 connect to all clusters (forced bitvector). |
| +0x0C | 4 | next sibling object (C0F60, 1CC290) |
| +0x10 | 4 | first child object (C0F60, 1CC290) |
| +0x14 | 4 | parent object (B92C0: activates only roots) |
| +0x28 | 8 | `s_location` of the object (written by BA630 only when the new cluster != -1) |
| +0x40 | 12 | cluster-primitive sphere centre (the "+0x40 cluster sphere" in the brief) |
| +0x4C | 4 | cluster-primitive sphere radius (float) |
| +0x60 | 4 | head of this object's **cluster reference** list (datum handle, -1 = empty) |
| +0x64 | 12 | root position (11E600 fallback) |
| +0xAA | 1 | object type (int8) |
| +0xB4 | 4 | physics component handle (-1 = none) (1CC290, 1CC3F0, 1CC440) |
| +0xD4 | 4 | B1870: delete-protected when != -1 in game mode 4 |
| +0x10A, +0x348 | 1 | biped bytes read by 16E290 (bit2, bit3) |
| +0x11C / +0x11E | 2 / 2 | attachment block byte size (int16, entries = size>>3 with a logical shift) and offset from body (int16) |
| +0x12C | 4 | type 11 (crate) dword read by 16E290 (bit1) |

### 2.3 `s_location` (8 bytes)

| offset | size | field |
|---|---|---|
| +0 | 4 | leaf index (int32, -1 = none) |
| +4 | 2 | cluster index (int16, -1 = none) |
| +6 | 2 | structure BSP index (int16) |

BA630 copies `+4..+7` as one dword into `object+0x2C`.

### 2.4 Data array header (as used by 16E6A0 and 1D2F10)

| offset | size | field |
|---|---|---|
| +0x00 | 32 | name |
| +0x20 | 4 | maximum count (2048 for the pools here) |
| +0x24 | 4 | element size |
| +0x34 | 4 | first-possibly-free cursor |
| +0x38 | 4 | high-water mark (one past the highest ever used) |
| +0x3C | 4 | allocated count |
| +0x40 | 2 | next salt (wraps 0xFFFF -> 0x8000) |
| +0x44 | 4 | element base address |
| +0x48 | 4 | occupied bitvector address (bit set = in use) |

### 2.5 Cluster partitions

A cluster partition is a 12-byte global:

| offset | field |
|---|---|
| +0 | address of `heads[]`: one dword datum handle per cluster (head of that cluster's entry list, -1 = empty); 512 entries |
| +4 | data_array* **cluster entries** ("owner records"): 32-byte records |
| +8 | data_array* **object cluster references** ("membership records"): 12-byte records |

The partitions in TU1.5:

| global | meaning | selected by |
|---|---|---|
| `0x4EABB0` (+0 heads, `0x4EABB4` entries, `0x4EABB8` references) | collideable objects | object flag bit9 set |
| `0x4EABA4` (`0x4EABA8`, `0x4EABAC`) | non-collideable objects | object flag bit9 clear |
| `0x4ECBE0` | lights (C4D30 / C5250 / C3750) | lights, payload size 0 |

Cluster entry record (32 bytes; partition+4 pool):

| offset | size | field |
|---|---|---|
| +0x00 | 2 | salt (written by 16E6A0); the rest of the record is zeroed on allocation |
| +0x04 | 4 | owner handle (object or light datum) |
| +0x08 | 4 | next entry handle in the same cluster (-1 terminates) |
| +0x0C | 0x14 | payload copied by 1D2F10 (objects: the BA910 packet, see 2.6; lights: nothing) |

Object cluster reference record (12 bytes; partition+8 pool):

| offset | size | field |
|---|---|---|
| +0x0 | 2 | salt |
| +0x4 | 4 | cluster index, sign-extended to int32 |
| +0x8 | 4 | next reference handle in this owner's list (head at object+0x60, or light+0x10) |

Insertion always **prepends** to both lists. The two allocations are
independent: a failed reference allocation can still create a cluster entry,
which leaves an orphan.

### 2.6 Cluster payload (0x14 bytes, built by BA910)

| offset | size | value |
|---|---|---|
| +0x00 | 2 | `(int16)(int8)object[+0xAA]` (object type, sign-extended) |
| +0x02 | 2 | `object_flags & 0x200 ? 16E290(object) : 0` |
| +0x04 | 12 | copy of object+0x40..+0x4B (sphere centre) |
| +0x10 | 4 | copy of object+0x4C (radius) |

### 2.7 Structure BSP and collision BSP

* `0x474C24` int16: current structure BSP index (-1 means none). It is
  mounted as dword `0x0001FFFF` initially; only the low half is the index.
* `0x4ECC18`: address of the current structure BSP tag data (**sbsp**).
* `0x4ECC0C`: address of the current collision BSP (normally equal to
  `rd32(sbsp+0x18)`; 14CEA0 reads both, see 3.9).

Structure BSP fields (block = count dword followed by address dword):

| offset | field |
|---|---|
| +0x14 / +0x18 | collision BSP block (64-byte elements; +0x18 is element 0) |
| +0x2C / +0x30 | leaves (8 bytes each; +0 int16 cluster) |
| +0x5C / +0x60 | cluster portals (0x24 bytes each) |
| +0x9C / +0xA0 | clusters (0xB0 bytes each) |

Cluster (0xB0 bytes): `+0x8C` portal-index count (int32), `+0x90` address of an int16 portal-index array.

Cluster portal (0x24 bytes):

| offset | size | field |
|---|---|---|
| +0x00 | 2 | cluster A (host model calls this `back`) |
| +0x02 | 2 | cluster B (`front`) |
| +0x04 | 4 | plane index (into the collision BSP planes) |
| +0x08 | 12 | bounding-sphere centre |
| +0x14 | 4 | bounding-sphere radius |
| +0x18 | 4 | not read in this tree (OPEN: flags) |
| +0x1C / +0x20 | 4 / 4 | vertex count / address of vertices (12-byte points) |

Collision BSP: `+0x00/+0x04` 3-D nodes (8 bytes: int16 plane at +0, signed
24-bit child[0] at +2, child[1] at +5; a child with bit23 set is a leaf
`child & 0x7FFFFF`, and all ones is "no leaf"). `+0x08/+0x0C` planes (16 bytes:
i, j, k, d floats).

### 2.8 Flood globals

| address | size | use |
|---|---|---|
| `0x4F3CE4` | 4 | flood stamp counter. 1D2F10 pre-increments it before every flood. 16C620 (camera/collision query) increments the same counter. |
| `0x4F3CE1` | 1 | "flood in progress": set to 1 around the 14D0E0 call, 0 after. 16C620 also sets it. |
| `0x4F3CE8` | 512*4 | per-cluster visit stamp: `stamp[c] == counter` means visited. Ends at `0x4F44E8`. |

These are mounted by `src/platform/native_tu15_world_memory.cpp:1287-1295`.

### 2.9 Cluster bitvectors

* **Forced all-clusters bitvector (BA630 local, 16 dwords = 512 bits)**: when
  object flag bit21 is set, BA630 fills `n = (cluster_count + 31) >> 5`
  dwords (arithmetic shift) with `0xFFFFFFFF` and passes it to 1D2F10 in eax.
* **Active-cluster bitvector**: `rd32(0x4F3218) + 0x11B8`, 16 dwords in the
  game globals. It is cleared by the BSP visibility reset
  (`native_campaign_bsp_reset_visibility_tu15`, sibling masks at +0x1138 and
  +0x1178). B8D90 reads it.

### 2.10 Constants

* `0x46A0E8` = `0x00000000` (+0.0f): the 1D2F10 radius gate.
* `0x44AC8C`: projection table, 6 rows x 3 int16 (row = `axis*2 + (plane[axis] > 0)`). 14CEA0 uses only the first two columns (u, v):

  | row | axis, sign | u, v, (w) |
  |---|---|---|
  | 0 | x, not positive | 2, 1, (0) |
  | 1 | x, positive | 1, 2, (0) |
  | 2 | y, not positive | 0, 2, (1) |
  | 3 | y, positive | 2, 0, (1) |
  | 4 | z, not positive | 1, 0, (2) |
  | 5 | z, positive | 0, 1, (2) |

  Values: `src/platform/native_tu15_operands.generated.cpp:4117` and the mount
  at `native_tu15_world_memory.cpp:1346`. Fingerprint check:
  `src/assets/original_data.cpp:388`.
* `0x474B90`: object-type definition pointer table, indexed by object type (10DDE0).
* `0x4EABC4`: address of an object-globals block. `word [+4]` is an
  active-object counter for flag bit14 (B92C0 increments, B9330 decrements).
  Its name is OPEN.

## 3. Routines

Conventions in the pseudo-C:

* `E` is the routine's stack pointer on entry, so `[E]` holds the return address.
* Stack-local stores (locals and reused argument slots) are listed for
  completeness. They are not externally visible. A native replacement can drop
  them unless it has to satisfy a store-sequence oracle that records stack writes.
* Every non-stack store is listed with the original PC of the store
  instruction. Existing native kernels report that PC in
  `SourcePhysicsContactRecordStore{pc, address, before, written}`.
* Saved-register pushes and return-address pushes are not listed.

### 3.1 BA630: `object_reconnect_to_map` (LS 9205-9345, 138 instructions)

**Convention:** `void __stdcall BA630(u32 object_index, const s_location *location)`, `ret 8`.
No register inputs. It preserves ebx, ebp, esi and edi and clobbers eax, ecx
and edx (none of them is a result). The frame is `sub esp,0x60` plus 4 pushes.
Locals: `[E-0x5D]` byte `was_unconnected`, `[E-0x5C]` 8-byte `s_location`,
`[E-0x54]` 0x14-byte payload, `[E-0x40]` 64-byte forced bitvector. The arg2
slot `[E+8]` is reused for the bitvector pointer. The arg1 slot `[E+4]` receives
1D2F10's overflow byte.

**Globals read:** 0x4ECBD0, 0x4ECC18 (only for flag bit21), 0x4F3218. The callees read more.

```c
void BA630(u32 object, const s_location *location) {
  u8 *objects_data = rd32(rd32(0x4ECBD0) + 0x44);
  u8 *hdr = objects_data + (object & 0xFFFF) * 12;
  u8 *obj = rd32(hdr + 8);
  u8 was_unconnected = (rd16(hdr + 4) == 0xFFFF);           // BA65D: stack byte [E-0x5D]
  s_location local;                                          // [E-0x5C]
  if (location == NULL) {
    11E600(&local /*esi*/, obj + 0x40 /*edx*/);              // primitive centre
    location = &local;
    if ((i16)rd16(&local.cluster) == -1)
      11E600(&local, obj + 0x64);                            // root position fallback
  }
  u32 flags;
  if ((i16)location->cluster != -1) {
    wr32(obj + 0x28, location->leaf);                         // BA68F
    wr32(obj + 0x2C, rd32((u8*)location + 4));                // BA698 (cluster | bsp<<16)
    wr16(hdr + 4, location->cluster);                         // BA69B
    flags = rd32(obj + 4) & ~0x00040000u;
  } else {
    flags = rd32(obj + 4) |  0x00040000u;   // obj+0x28/+0x2C and hdr+4 keep their OLD values
  }
  wr32(obj + 4, flags);                                       // BA6BB
  u32 *forced = NULL;                                         // BA6BE: [E+8] = 0 (stack)
  if (flags & 0x00200000u) {                                  // tested on the value just stored
    i32 n = ((i32)rd32(rd32(0x4ECC18) + 0x9C) + 31) >> 5;     // SAR
    for (i32 k = 0; k < n; k++) forced_bits[k] = 0xFFFFFFFF;  // BA6E6 rep stosd (stack); BA6ED byte tail is always 0
    forced = forced_bits;                                     // BA6F3: [E+8] = E-0x40 (stack)
  }
  BA910(object /*edx*/, payload /*edi = E-0x54*/);
  u32 partition = (rd32(obj + 4) & 0x200) ? 0x4EABB0 : 0x4EABA4;   // re-read after BA910
  1D2F10(forced /*eax*/, partition, object, obj + 0x60, obj + 0x40,
         /*radius bits*/ rd32(obj + 0x4C), obj + 0x28, 0x14, payload,
         /*overflow out*/ (u8*)(E + 4));
  wr32(obj + 4, rd32(obj + 4) | 0x100);                       // BA752
  wr8 (hdr + 2, rd8(hdr + 2) | 0x40);                         // BA760
  C0F60(object, 0, 1, 0, 0);                                  // reconnect attached lights
  if (B8D90(object /*eax*/, rd32(0x4F3218) + 0x11B8)) {
    B92C0(object /*edi*/);
  } else if ((rd32(obj + 4) & 0x20000) && !B1870(object /*eax*/)) {
    BA570(object);
  } else {
    B9330(object /*edx*/);
  }
  if (was_unconnected && rd16(hdr + 4) != 0xFFFF) {           // hdr+4 re-read here
    call_1496F0();
    1CC290(object, object);
    call_282C10();
    call_1496F0();
  }
}
```

Notes:
* When the new cluster is -1, 1D2F10 still receives `obj+0x28`, so the flood
  starts from the **stale** previous location. The host model documents the
  same behaviour ("caller must retain old object+28/+2C").
* Branch order is exact. B8D90's `al` decides activation. Flag bit17 is read
  after C0F60 and B8D90 have run.
* `hdr` is not re-derived after BA570: the tail reads `hdr+4` at the same
  address even if the object was just deleted.
* The forced bitvector holds 512 bits. With more than 512 clusters the
  original overruns its frame. Natively, use 16 dwords and reject or clamp
  larger counts; 512 is the established maximum.

Stores (non-stack): BA68F, BA698, BA69B (conditional); BA6BB, BA752, BA760
(always). The callees' stores are in their own sections.

### 3.2 11E600: `location_from_point` (LS 69886-69915, 27 instructions)

**Convention:** usercall. `esi` = `s_location *out`, `edx` = `const float point[3]`, `ret 0`.
Preserves edi, esi and edx. Clobbers eax and ecx. **Globals:** 0x474C24, 0x4ECC0C, 0x4ECC18.

```c
void 11E600(s_location *out, const float *p) {
  u16 bsp = rd16(0x474C24);
  if (bsp == 0xFFFF) {
    wr16(out+4, 0xFFFF);                 // 11E60E
    wr16(out+6, 0xFFFF);                 // 11E612
    wr32(out+0, 0xFFFFFFFF);             // 11E616
    return;
  }
  i32 leaf = 14CDB0(/*eax*/0, /*ecx*/rd32(0x4ECC0C), /*edx*/p);
  wr32(out+0, leaf);                     // 11E62E
  if (leaf != -1) {
    u16 c = rd16(rd32(rd32(0x4ECC18) + 0x30) + leaf*8);
    wr16(out+6, bsp);                    // 11E63F
    wr16(out+4, c);                      // 11E643
  } else {
    wr16(out+6, bsp);                    // 11E64C
    wr16(out+4, 0xFFFF);                 // 11E650
  }
}
```
Callees: 14CDB0. Existing native: see section 5 (full memory-level replica).

### 3.3 14CDB0: `bsp3d_leaf_from_point` (LS 79910-79951, 39 instructions)

**Convention:** usercall. `eax` = start node, `ecx` = collision BSP, `edx` =
point; returns `eax` = leaf index or -1. Clobbers ecx, xmm0, xmm1 and xmm2.
Preserves edx, esi and edi. No stores.

```c
i32 14CDB0(u32 node, const u8 *bsp, const float *p) {
  const u8 *nodes = rd32(bsp+4), *planes = rd32(bsp+0xC);
  for (;;) {
    const u8 *n = nodes + (node & 0x7FFFFF) * 8;
    const float *pl = planes + (i32)(i16)rd16(n) * 16;
    float d = ((pl[2]*p[2] + pl[1]*p[1]) + pl[0]*p[0]) - pl[3];   // SSE single, this order
    u32 side = (d < 0.0f || isnan(d)) ? 0 : 1;                     // comiss vs +0; jb
    node = (u32)((i32)rd32(n + 1 + 3*side) >> 8);                  // child[0] at n+2, child[1] at n+5
    if (node & 0x800000) return node == 0xFFFFFFFF ? -1 : (i32)(node & 0x7FFFFF);
  }
}
```

### 3.4 BA910: `object_cluster_payload` (LS 9448-9479, 29 instructions)

**Convention:** usercall. `edx` = object handle, `edi` = `u8 out[0x14]`, `ret 0`.
Preserves esi and edi. Clobbers eax, ecx and edx.

```c
void BA910(u32 h, u8 *out) {
  u8 *obj = rd32(rd32(rd32(0x4ECBD0)+0x44) + (h&0xFFFF)*12 + 8);
  u16 f = (rd32(obj+4) & 0x200) ? (u16)16E290(h) : 0;
  wr16(out+2,  f);                        // BA945
  wr16(out+0,  (u16)(i16)(i8)obj[0xAA]);  // BA949
  wr32(out+4,  rd32(obj+0x40));           // BA954
  wr32(out+8,  rd32(obj+0x44));           // BA959
  wr32(out+12, rd32(obj+0x48));           // BA95F
  wr32(out+16, rd32(obj+0x4C));           // BA965
}
```
In BA630, `out` is a stack local.

### 3.5 16E290: `object_cluster_payload_flags` (LS 95254-95340, 73 instructions)

**Convention:** usercall. `eax` = object handle; returns `ax` (eax bits 16-31 are 0). No stores.

```c
u32 16E290(u32 h) {
  u8 *hdr = rd32(rd32(0x4ECBD0)+0x44) + (h&0xFFFF)*12, *obj = rd32(hdr+8);
  i8 type = obj[0xAA];
  u32 r = ((hdr[2] & 0x10) || (obj[4] & 1)) ? 1 : 0;
  u32 f = rd32(obj+4);
  if (f & (1u<<20)) r |= 2;  else r &= 0xFFFD;
  if (!(f & (1u<<22))) r |= 4; else r &= 0xFFFB;
  if (!(f & (1u<<31))) r |= 8;
  if (type == 0)       { r = (obj[0x10A] & 4) ? r|0x10 : r&0xFFEF;
                         r = (obj[0x348] & 8) ? r|0x20 : r&0xFFDF; }
  else if (type == 11) { r = (rd32(obj+0x12C) & 2) ? r|0x40 : r&0xFFBF; }
  return r;
}
```

### 3.6 1D2F10: `cluster_partition_reconnect` (LS 117335-117448, 111 instructions)

**Convention:** `eax` = `const u32 *forced_bits` (NULL means use the location
and a flood) plus 9 stack dwords, `ret 0x24`:

| arg | BA630 passes | C4D30 passes |
|---|---|---|
| 1 partition | 0x4EABB0 / 0x4EABA4 | 0x4ECBE0 |
| 2 owner handle | object | light |
| 3 `u32 *owner_ref_head` | obj+0x60 | light+0x10 |
| 4 `const float *center` | obj+0x40 | light+0x18 |
| 5 radius (float bits) | rd32(obj+0x4C) | rd32(light+0x24) |
| 6 `const s_location *loc` | obj+0x28 | local location |
| 7 payload size | 0x14 | 0 |
| 8 `const void *payload` | &payload | NULL |
| 9 `u8 *overflow_out` | BA630 arg1 slot | C4D30 local byte |

Returns nothing. eax, ecx and edx are clobbered and dead at both call sites.
Preserves ebx, ebp, esi and edi. Frame: 0x204 bytes of locals. `[E-0x204]` is
the dword `total`, later reused as the loop counter. `[E-0x200]` is
`i16 clusters[256]`.

**Globals:** 0x46A0E8 (+0.0f), 0x4F3CE4 (read/write), 0x4F3CE1 (write).

```c
void 1D2F10(const u32 *forced, u8 *part, u32 owner, u32 *ref_head, const float *c,
            float radius, const s_location *loc, u32 size, const u8 *payload, u8 *ovf) {
  i16 clusters[256]; i32 total32;   // only the low 16 bits of total32 matter
  i16 stored, total;
  if (forced) {
    stored = 14D080(/*edi*/&total32, /*ebx*/forced, 256, clusters);  // writes the low word of total32
    total  = (i16)total32;
  } else {
    i16 cl = loc->cluster; i32 count = 0;
    if (cl != -1) {
      if (!(radius > f32(rd32(0x46A0E8)))) {         // comiss; jbe: radius <= 0 or NaN
        count = 1; clusters[0] = cl;                 // 1D2FA6 (stack)
      } else {
        wr32(0x4F3CE4, rd32(0x4F3CE4) + 1);          // 1D2F72
        wr8 (0x4F3CE1, 1);                           // 1D2F8C
        count = 14D0E0(/*low16*/cl, c, radius, 256, clusters);   // pushes (loc dword-hi | cl)
        wr8 (0x4F3CE1, 0);                           // 1D2F98
      }
    }
    total  = (i16)count;
    stored = ((i16)count > 256) ? 256 : (i16)count;  // signed 16-bit compare
  }
  *ovf = (total > stored);                           // 1D2FC8 (1 byte)
  if (stored <= 0) return;
  for (u32 n = (u16)stored, i = 0; n != 0; n--, i++) {   // counter kept at [E-0x204]: 1D2FD7 / 1D307A (stack)
    i16 k = clusters[i];
    u8 *refs = rd32(part + 8);
    u32 r = 16E6A0(refs);                            // edx preserved by the callee
    if (r != 0xFFFFFFFF) {
      u8 *rec = rd32(refs+0x44) + (r & 0xFFFF) * rd32(refs+0x24);   // imul
      wr32(rec+4, (u32)(i32)k);                      // 1D3004
      wr32(rec+8, *ref_head);                        // 1D3010
      *ref_head = r;                                 // 1D3013
    }
    u32 *head = (u32*)(rd32(part+0) + (i32)k*4);
    u8 *ents = rd32(part + 4);
    u32 e = 16E6A0(ents);
    if (e != 0xFFFFFFFF) {
      u8 *rec = rd32(ents+0x24) * (e & 0xFFFF) + rd32(ents+0x44);
      wr32(rec+4, owner);                            // 1D3045
      wr32(rec+8, *head);                            // 1D304A
      *head = e;                                     // 1D304D
      if (payload) {                                 // rep movsd, then rep movsb (DF = 0)
        for (j = 0; j < size>>2; j++) wr32(rec+0xC+4*j, rd32(payload+4*j));  // 1D3069
        for (j = 0; j < (size&3); j++) wr8(...);                              // 1D3070
      }
    }
  }
}
```

Notes:
* The flood path increments `0x4F3CE4` and toggles `0x4F3CE1`. The forced
  path and the single-cluster path do neither.
* The 16E6A0 calls alternate per cluster in this order: reference, then entry.
  That order defines allocation order, salts and list order. It must be kept.
* `k` is used sign-extended for both the record field and the head index.

### 3.7 14D080: `clusters_from_bitvector` (LS 80135-80170, 32 instructions)

**Convention:** usercall. `edi` = `i16 *out_total`, `ebx` = `const u32 *bits`.
Stack: `[E+4]` max (low word), `[E+8]` `i16 *out`. `ret 8`. Returns `ax`
(eax is a 32-bit counter). Clobbers ecx and edx. **Globals:** 0x4ECC18.

```c
i32 14D080(i16 *out_total, const u32 *bits, i16 max, i16 *out) {
  i32 stored = 0;
  wr16(out_total, 0);                                    // 14D089 (1D2F10 stack)
  u8 *sbsp = rd32(0x4ECC18);
  for (i32 i = 0; i < (i32)rd32(sbsp+0x9C); i++) {       // count re-read each step; no loop if <= 0
    if (bits[i >> 5] & (1u << (i & 31))) {
      if ((i16)stored < max) { wr16(out + (i16)stored, (u16)i); stored++; }   // 14D0C4 (stack)
      wr16(out_total, rd16(out_total) + 1);              // 14D0CA (stack)
    }
  }
  return stored;
}
```

### 3.8 14D0E0: `cluster_portal_flood` (LS 80171-80259, 86 instructions, recursive)

**Convention:** `__stdcall` with 5 dword arguments, `ret 0x14`:
`(cluster [low16 used], const float *center, float radius, max [low16 tested], i16 *out)`.
Returns `ax` = number of clusters reached (eax is the count, zero-extended in
practice). Register outputs: the `push ecx` slot is overwritten with the sbsp
pointer, so **ecx returns `rd32(0x4ECC18)`**. edx is path-dependent garbage.
Neither is used by the callers in LS (1D2F10 and itself). For the other callers
(BD080, 181320, 1839F0, 383C90) this is OPEN. Preserves ebx, ebp, esi and edi.

**Globals:** 0x4ECC18, 0x4F3CE4 (read), 0x4F3CE8 (read/write).

```c
i32 14D0E0(i16 cluster, const float *c, float radius, i32 max, i16 *out) {
  u8 *sbsp = rd32(0x4ECC18);                              // captured once; 14D112 stores it to [E-4]
  u8 *cl = rd32(sbsp+0xA0) + (i32)cluster*0xB0;
  i32 remaining = max - 1;                                // 32-bit; only its low word is tested
  if ((i16)max > 0) { wr16(out, cluster); out++; }        // 14D118 (1D2F10 stack buffer)
  u32 stamp = rd32(0x4F3CE4);
  if (rd32(0x4F3CE8 + cluster*4) != stamp)
    wr32(0x4F3CE8 + cluster*4, stamp);                    // 14D12D, conditional store
  i32 count = 1;                                          // 14D141: arg5 slot = 1 (stack)
  // 14D145: arg4 slot = loop index i = 0 (stack)
  for (i32 i = 0; i < (i32)rd32(cl+0x8C); i = (i16)(i+1)) {     // 14D1DD; count re-read
    i16 pi = rd16(rd32(cl+0x90) + i*2);
    u8 *portal = rd32(sbsp+0x60) + (i32)pi*0x24;
    i16 a = rd16(portal+0);
    i16 next = (a == cluster) ? rd16(portal+2) : a;
    if (rd32(0x4F3CE8 + (i32)next*4) == rd32(0x4F3CE4)) continue;  // already visited
    if (!14CEA0(/*ax*/pi, /*ecx*/c, /*edx*/sbsp, radius)) continue;
    i32 n = 14D0E0(next, c, radius, remaining, out);       // full eax used below
    out       += (i16)n;                                   // movsx
    count     += n;                                        // 14D1CC (stack)
    remaining -= n;
  }
  return (u16)count;                                       // `mov ax,[count]`; returns 1 if no portals
}
```

Semantics: a depth-first preorder over the portal graph. Each cluster is
marked when entered. A neighbour is tested against the portal only if it is not
yet marked, and it is entered only if 14CEA0 succeeds. Every reached cluster
counts, but it is written only while `remaining > 0`, so `count` can exceed
`max`. That is how 1D2F10 detects overflow.

### 3.9 14CEA0: `portal_intersects_sphere` (LS 79994-80133, 137 instructions)

**Convention:** usercall. `ax` = portal index (int16), `ecx` = centre,
`edx` = sbsp, `[E+4]` = radius (float). `ret 4`. Returns `al` (0/1).
Preserves ebx, esi, edi and ebp. Clobbers ecx, edx, xmm0-xmm7 and the x87
stack (balanced). The frame is 0x420 bytes: dist float, portal pointer,
projected point, 2-D point, and `float verts[128][2]` at `[E-0x400]`.

**Globals:** 0x4ECC0C, 0x44AC8C (table).

```c
bool 14CEA0(i16 pi, const float *c, const u8 *sbsp, float radius) {
  const u8 *portal = rd32(sbsp+0x60) + (i32)pi*0x24;           // 14CEC4 saves it (stack)
  u32 poff = rd32(portal+4) << 4;
  const float *pl = (float*)(rd32(rd32(sbsp+0x18) + 0xC) + poff);   // planes of THIS sbsp's collision BSP
  /* x87 under the captured control word (PC=53 assumed, see section 4): */
  double dx87 = ((double)pl[1]*c[1] + (double)pl[2]*c[2]) + (double)c[0]*pl[0];
  dx87 = dx87 - (double)pl[3];
  float dist = (float)dx87;                                     // 14CEDF fst dword (stack)
  if (!((double)radius > fabs(dx87))) return false;            // fcomip: false if radius <= |d| or unordered
  /* SSE single: */
  float dz = f32(portal+0x10) - c[2], dx = f32(portal+0x08) - c[0], dy = f32(portal+0x0C) - c[1];
  float rr = f32(portal+0x14) + radius;
  float d2 = (dz*dz + dx*dx) + dy*dy;
  if (!(rr*rr > d2)) return false;                             // comiss; jbe
  const float *pl2 = (float*)(rd32(rd32(0x4ECC0C) + 0xC) + poff);   // GLOBAL collision BSP planes
  i16 axis = 123210(pl2);
  u32 pos = (pl2[axis] > 0.0f) ? 1 : 0;                         // comiss; jbe -> 0
  float neg = 0.0f - dist;
  float p[3] = { pl2[0]*neg + c[0], pl2[1]*neg + c[1], pl2[2]*neg + c[2] };  // 14CF8F/14CFB9/14CFD0 (stack)
  i16 u = rd16(0x44AC8C + (axis*2+pos)*6), v = rd16(0x44AC8E + (axis*2+pos)*6);
  float pt[2] = { p[u], p[v] };                                 // 14CFDF/14CFEF (stack)
  i32 nv = (i32)rd32(portal+0x1C);
  if (nv > 0) {
    const float *vb = rd32(portal+0x20);                        // 14CFFC (stack)
    i32 i = 0;
    do {                                                        // raw 32-bit copies, no FP conversion
      verts[(i16)i][1] = vb[(i16)i*3 + v];                      // 14D016 (stack)
      verts[(i16)i][0] = vb[(i16)i*3 + u];                      // 14D01E (stack)
      i++;
    } while ((i16)i < nv);
  }
  double disk2 = (double)radius*radius - (double)dist*dist;     // x87
  float disk = (float)sqrt(disk2);                              // fsqrt; 14D04F fstp dword (stack)
  return 243450(/*ecx*/verts, /*edi*/pt, /*xmm0*/disk, /*stack*/(u16)rd16(portal+0x1C));
}
```

Notes:
* The distance plane comes from `*(sbsp+0x18)`. The axis and projection plane
  come from `*(0x4ECC0C)`. Both are expected to be the same pointer. A native
  that reads memory should keep the two reads as written.
* `dist` (the float-rounded value) feeds both the projection and the disk
  radius. The rejection test uses the unrounded x87 value.
* At most 128 vertices fit in the original scratch. The host decoder rejects
  portals with more than 128 vertices; do the same.

### 3.10 123210: `dominant_axis` (LS 71967-72000, 31 instructions)

**Convention:** usercall. `eax` = `const float v[3]`; returns `ax` in {0,1,2}.
Callers use `movsx`. The stack stores of |v| are scratch.

```c
i16 123210(const float *v) {
  float ax = fabsf(v[0]), ay = fabsf(v[1]), az = fabsf(v[2]);   // fabs; fstp (exact)
  if (!(az < ay) && !(az < ax)) return 2;   // fcomip / comiss: CF = less-or-unordered
  if (!(ay < ax)) return 1;
  return 0;
}
```
This equals `posed_collision.cpp::projection_axis` (`z>=y && z>=x -> 2; y>=x -> 1; else 0`), including NaN handling.

### 3.11 243450: `circle_vs_portal_polygon` (LS 153439-153501, 60 instructions)

**Convention:** usercall. `ecx` = `const float (*verts)[2]`, `edi` =
`const float pt[2]`, `xmm0` = circle radius, `[E+4]` = vertex count (int16).
`ret 4`. Returns `al`. Clobbers xmm0-xmm7 and edx. No stores.

```c
bool 243450(const float (*vt)[2], const float *pt, float r, i16 count) {
  float r2 = r * r;
  if (count <= 0) return true;
  for (i16 i = 0; i < count; i++) {
    i32 j = (i + 1 >= count) ? 0 : i + 1;       // setge/dec/and idiom
    float px = pt[0] - vt[i][0], py = pt[1] - vt[i][1];
    float ex = vt[j][0] - vt[i][0], ey = vt[j][1] - vt[i][1];
    float len2 = ex*ex + ey*ey;
    if (len2 == 0.0f) continue;                 // ucomiss/lahf/test ah,44h/jnp: skips only an ORDERED equal; NaN proceeds
    float cross = px*ey - py*ex;
    if (!(cross > 0.0f)) continue;              // comiss; jbe
    if (cross*cross > len2 * r2) return false;  // comiss; ja
  }
  return true;
}
```

### 3.12 16E6A0: `datum_new` (LS 95533-95623; 78 translated instructions behind a native hook)

**Convention:** usercall. `edx` = data_array*; returns `eax` = handle
`(salt<<16)|index` or -1. Preserves ebx, ebp, esi, edi and edx. Clobbers ecx.

```c
u32 16E6A0(u8 *a) {
  i32 i = rd32(a+0x34), hw = rd32(a+0x38); i32 idx = -1;
  u32 *bits = rd32(a+0x48);              // read inside the branch at 16E6B1 and again at 16E6F0 (same value)
  if (i < hw) {
    for (; i < (i32)rd32(a+0x38); i++)
      if (!(bits[i>>5] & (1u<<(i&31)))) { idx = i; break; }
    if (idx != -1) goto take;               // the original also guards i == -1
  }
  if (hw >= (i32)rd32(a+0x20) || hw == -1) return 0xFFFFFFFF;   // hw = value read at entry
  idx = hw;
take:
  u8 *rec = rd32(a+0x44) + rd32(a+0x24)*idx;
  wr32(&bits[idx>>5], bits[idx>>5] | (1u<<(idx&31)));          // 16E70E
  wr32(a+0x3C, rd32(a+0x3C)+1);                                 // 16E71C
  wr32(a+0x34, idx+1);                                          // 16E71F
  if (!((i32)rd32(a+0x38) > idx)) wr32(a+0x38, idx+1);          // 16E724
  memset32/8(rec, 0, rd32(a+0x24));                             // 16E733 dwords, 16E73A bytes
  wr16(rec, rd16(a+0x40));                                      // 16E740
  wr16(a+0x40, rd16(a+0x40)+1);                                 // 16E743
  if (rd16(a+0x40) == 0xFFFF) wr16(a+0x40, 0x8000);             // 16E74E
  return ((u32)rd16(rec) << 16) | idx;
}
```
Already native (see section 5).

### 3.13 C0F60: `object_connect_lights_recursive` (LS 15489-15588, 97 instructions)

**Convention:** `__stdcall(u32 object, disconnect, reconnect, iterate_siblings, known_live)`, `ret 0x14`.
Only the low byte of the flag arguments is tested. BA630 passes `(object,0,1,0,0)`.
BA570 passes `(object,1,0,0,0)`.

```c
void C0F60(u32 o, u8 disc, u8 reco, u8 sibl, u8 known_live) {
  for (;;) {
    u8 *hdr = rd32(rd32(0x4ECBD0)+0x44) + (o&0xFFFF)*12, *obj = rd32(hdr+8);
    u8 bl;
    if (known_live) { if (hdr[2] & 0x10) return; if (obj[4] & 1) return; bl = 0; }
    else            { bl = BBD10(/*ecx*/o); if (bl) return; }
    if (rd32(obj+4) & 0x10) {
      u32 n = (u32)(i32)(i16)rd16(obj+0x11C) >> 3;            // movsx then logical shr
      if ((i32)n > 0) {
        u8 *e = obj + (i32)(i16)rd16(obj+0x11E);
        for (; n; n--, e += 8) {
          if (e[0] != 0) continue;                              // attachment kind 0 = light
          u32 L = rd32(e+4); if (L == 0xFFFFFFFF || bl) continue;
          if (disc) C5250(/*eax*/L);
          if (reco) C4D30(L);
        }
      }
    }
    if (rd32(obj+4) & 0x40) C3750(o, disc, reco);
    if (rd32(obj+0x10) != 0xFFFFFFFF) C0F60(rd32(obj+0x10), disc, reco, 1, bl == 0);
    if (!sibl) return;
    u32 nx = rd32(obj+0xC); if (nx == 0xFFFFFFFF) return;
    o = nx;                                                     // C1069 stores into the arg1 slot (stack)
  }
}
```
C4D30 (262 instructions) reconnects one light. It uses light array
`rd32(0x4ECBDC)` (0x110-byte records), 11E600 on light+0x18, and 1D2F10 with
partition 0x4ECBE0 and no payload, then sets light byte+2 bit3. C5250
disconnects through 1D30F0 and clears bit3. C3750 walks every light whose
+0x58 owner equals the object. **These need their own spec.**

### 3.14 B8D90: `object_in_active_clusters` (LS 7444-7499, 53 instructions) and 10DDE0 (LS 62109-62140, 29)

**Convention (B8D90):** usercall. `eax` = object, `[E+4]` = `const u32 *bits`,
`ret 4`, returns `al`. Writes byte 0 to its own argument slot `[E+4]`
(B8DB0, stack) and uses it as the result byte.

```c
bool B8D90(u32 o, const u32 *bits) {
  u8 *hdr = ..., *obj = rd32(hdr+8); u8 result = 0;            // B8DB0 (stack)
  if (rd32(obj+4) & 2) return 1;
  if ((u8)(1u << (hdr[3] & 31)) & 0x80) {                      // only object type 7 (machine)
    if (10DDE0(/*edi*/o, /*ebx*/&result, bits)) return result;
  }
  i16 c = rd16(hdr+4);
  if (c == -1) return result;
  return (bits[c >> 5] & (1u << (c & 31))) != 0;
}
// 10DDE0: edi=object, ebx=u8* out, [E+4]=bits, ret 4, returns al
u8 10DDE0(u32 o, u8 *out, const u32 *bits) {
  u8 *def = rd32(0x474B90 + (i8)obj(o)[0xAA]*4);
  u8 al = 0;
  for (u32 *p = (u32*)(def+0x84); rd32(p) != 0 && al == 0; p++) {
    u32 fn = rd32(rd32(p)+0x48);
    if (fn) al = call_stdcall3(fn, o, bits, out);              // OPEN: callback identities
  }
  return al;
}
```

### 3.15 B92C0: `object_activate` (LS 7914-7949, 33 instructions)

`edi` = object, `ret 0`.
```c
void B92C0(u32 o) {
  u8 *hdr = ...; u8 f = hdr[2];
  if (f & 1) return;
  if (rd16(hdr+4) == 0xFFFF) return;
  u8 *obj = rd32(hdr+8);
  if (rd32(obj+0x14) != 0xFFFFFFFF) return;                    // roots only
  wr8(hdr+2, f | 1);                                           // B92F4
  if ((1u << hdr[3]) & 0x1883) 1CC3F0(/*ecx*/o);               // types 0,1,7,11,12
  if (rd32(obj+4) & 0x4000) { u8 *g = rd32(0x4EABC4); wr16(g+4, rd16(g+4)+1); }   // B931F
}
```
1CC3F0 (22): if `obj+0xB4 != -1` and component flag bit17 is clear in the 0xA0-byte
record at `rd32(rd32(0x52BE5C)+0x44) + idx*0xA0`, it calls `1CC310(o, 0)`.

### 3.16 B9330: `object_deactivate` (LS 7951-7983, 30 instructions)

`edx` = object, `ret 0`.
```c
void B9330(u32 o) {
  u8 *hdr = ...; u8 f = hdr[2];
  if (!(f & 1)) return;
  u8 *obj = rd32(hdr+8);
  wr8(hdr+2, f & 0xFA);                                        // B9356
  if ((1u << hdr[3]) & 0x1883) 1CC440(o);                      // stdcall, ret 4
  if (rd32(obj+4) & 0x4000) { u8 *g = rd32(0x4EABC4); wr16(g+4, rd16(g+4)-1); }   // B9383
}
```
1CC440 (47): for bipeds with `byte+0x34B==1 && byte+0x3DC==3` it calls
`E3E20(eax=o, push 0)`. Then, if the component exists with flag bit17 set, it
calls `1CC310(o,0)`.

### 3.17 B1870: `object_delete_protected` (LS 5539-5558, 17 instructions)

`eax` = object; returns `al = (byte[rd32(0x4F3218)+0xC] == 4) && rd32(obj+0xD4) != -1`. No stores.

### 3.18 BA570: `object_delete` (LS 9141-9203, 60 instructions; subtree about 3.7k)

`__stdcall(object)`, `ret 4`. Out of scope here. A full native exists (section 5).

### 3.19 1CC290: `object_physics_create_recursive` (LS 113376-113418, 40 instructions)

`__stdcall(u32 root, u32 object)`, `ret 8`.
```c
void 1CC290(u32 root, u32 o) {
  u8 *obj = body(o);
  u32 child = rd32(obj+0x10);                                  // read BEFORE the creation calls
  if (rd32(obj+0xB4) == 0xFFFFFFFF) { call_1496F0(); 1CC190(o); call_282C10(); call_1496F0(); }
  while (child != 0xFFFFFFFF) {
    u8 *cb = body(child);
    if (child == root) break;
    1CC290(root, child);
    child = rd32(cb+0xC);
  }
}
```

### 3.20 Havok boundary services

* **1496F0** (WR `run_148360`, 123 instructions): no stack arguments. It is
  re-entrancy guarded by the byte at canonical `0x486563` (mapped through
  `selected()`). Native service name elsewhere: `collect_memory`. No native
  replica. OPEN.
* **282C10** (WR `run_27F190`, 276 instructions): no stack arguments. It exits
  early on two byte globals. Native: `native_tu15_process_dirty_island_objects`.

## 4. Floating point that must be bit-exact

Environment: MXCSR `0x1F80` (round to nearest, all exceptions masked, **no
FTZ/DAZ**) and an x87 control word that has to be checked. The game runs with
`0x027F` (PC = 53-bit, RC = nearest), which existing natives assert in several
places. Compile native kernels with `-fno-fast-math -ffp-contract=off` (see
`cmake/CampaignEnvironmentTu15.cmake`). x86-64 float math is SSE with
`FLT_EVAL_METHOD 0`.

1. **14CDB0 plane side (SSE single):** `d = ((k*z + j*y) + i*x) - w`.
   Side 1 iff `!(d < 0)`. NaN gives side 0; `-0.0` gives side 1. Keep the
   operation order and do not use FMA.
2. **14CEA0 plane distance (x87):** `((j*y + k*z) + x*i) - w`, evaluated
   under the live control word. With PC=53 this is exactly IEEE `double`
   arithmetic: each float x float product is exact, and each add/sub rounds to
   53 bits. The **reject test uses the unrounded value** (`radius > |d|`;
   unordered rejects). The **projection and disk use `float(d)`**. With PC=64
   (or a host `long double`) results differ. The adversarial case
   (plane `(0.707106769,0.707106769,1e-10,0)`, centre `(1e9,-1e9,1)`, radius
   `1e-10`) is admitted at PC53 and rejected at PC64
   (docs/SOURCE_OBJECT_ADMISSION_PARITY.md). A native must use `double`, not
   `long double`, and should assert `(cw & 0x300) == 0x200` and `(cw & 0xC00) == 0`,
   or run the x87 sequence in inline asm under the captured CW, as
   `native_tu15_camera_geometric_query.cpp`'s `Environment` does.
3. **14CEA0 portal bounding-sphere test (SSE):** `rr = pr + radius`,
   `d2 = (dz*dz + dx*dx) + dy*dy` with `dz,dx,dy = portal - centre`.
   Continue iff `rr*rr > d2`.
4. **123210:** fabs and the comparisons are exact. Ties prefer z, then y.
5. **Projection (SSE):** `neg = 0.0f - dist` (gives +0 for both signed
   zeros), `p[k] = plane2[k]*neg + c[k]`, a separate multiply then add.
6. **Disk radius (x87):** `float(sqrt(double(r)*r - double(dist)*dist))`.
   Both squares are exact in double, the difference rounds once, the sqrt is
   correctly rounded, then rounded to float. This double rounding is what the
   original does. It cannot go negative: `|dist_x87| < r` implies
   `|float(dist)| <= r`.
7. **243450 (SSE):** `r2 = disk*disk` uses the float-rounded disk. Per edge:
   `len2 = ex*ex + ey*ey`, `cross = px*ey - py*ex`, reject iff
   `cross > 0 && cross*cross > len2*r2`. Edges with ordered `len2 == 0` are
   skipped; a NaN `len2` is evaluated.
8. **1D2F10 radius gate:** flood iff `radius > +0.0f`. A NaN radius takes the
   single-cluster path.
9. BA630, BA910, 16E290, 14D080, 14D0E0 and 16E6A0 are integer only. Floats
   are copied as raw bits.

## 5. Existing native replicas in the repository

Searched `src/`, `include/` and `docs/` for each address in lower and upper
case, excluding `*_generated*` and `*.generated.*`. Hits that are only hex
substrings inside hashes are flagged as spurious.

| routine | replica | kind | notes |
|---|---|---|---|
| **BA630** | `SourceObjectRegistrationJournal::link / register_owner / register_default` (`src/gameplay/objects/source_object_spatial_pool.cpp`, `include/halo2/source_object_spatial_pool.hpp`), named after retail B8600 | **algorithmic host model, partial** | Covers: keep the old location when the cluster is NONE, bit18, bit21 forced mask, membership, collideable/noncollideable choice by bit9, pool insertion, setting bit 0x100 and header 0x40, and returns `old_none_to_valid` / `activation_services_required`. Works on host structs, not emulated memory. Missing: the C0F60/B8D90/B92C0/B9330/BA570/Havok tail, the 0x4F3CE* side effects, memory stores and store PCs. Used only by tests (`tests/source_object_spatial_pool_tests.cpp`, `source_object_hierarchy_tests.cpp`). Docs mentions only: NATIVE_REPLACEMENT_GOAL.md, RENDER_THREAD_OFFLOAD_PLAN.md, NATIVE_4K60_SPLITSCREEN_GOAL.md, MCC_GAMEPLAY_GOAL_CHECKLIST.md, DAMAGE_FATAL_ENTITY_CLEANUP_HANDOFF.md. `src/rendering/models/mcc_sky_geometry_profiles.inc` is spurious. |
| **11E600** | `native_tu15_constraint_effect_location` (`src/physics/native_tu15_constraint_effect_location.cpp`) | **full native, memory-level** | Same branches and store order and the same PCs (11E60E...11E650). Directly reusable. Host model: `source_object_point_location` and `source_object_default_registration_location` (`src/gameplay/objects/source_object_location.cpp`), which is exactly BA630's NULL-location double lookup. Other mentions are callers that invoke the translated routine (sound kernels, `multiplayer_object_runtime.cpp`) or docs. |
| **14CDB0** | `native_tu15_constraint_effect_point_leaf` (same file) | **full native, memory-level** (read-only) | SSE asm helpers keep the operation order. Directly reusable. |
| BA910 | `native_tu15_write_object_spatial_packet` / `native_tu15_build_object_spatial_packet` (`src/physics/native_tu15_object_spatial_packet.cpp`) | **full native** | The host form builds the 20-byte payload without a stack address, which is what a native BA630 needs. |
| 16E290 | `native_tu15_object_spatial_flags` (same file) | **full native** | |
| **1D2F10** | `source_object_light_spatial_membership` (`src/gameplay/objects/object_light_spatial.cpp`) + `SourceObjectSpatialPool::insert` | **algorithmic host model** (retail 1CAC60) | The membership list is identical (forced / single / flood, 256 storage, truncated flag). It does **not** touch 0x4F3CE4/E1/E8. **NaN radius differs**: the host floods, the original takes the single-cluster path. `native_tu15_world_memory.cpp:1287-1295` mounts its globals (mention). Address collisions, unrelated routines: `physics_live_entries_tu15.hpp {0x1d77d0,0x1d2f10}` and `physics_contact_lifecycle.hpp` "1D2F10 ret0x10" both refer to canonical World-image addresses. |
| 14D080 | the forced-bitset branch of `source_object_light_spatial_membership` | algorithmic host model | `mcc_native_pc_world_programs.inc` hit is spurious. |
| **14D0E0** | the DFS in `source_object_light_spatial_membership` | **algorithmic host model** (retail 14A5B0) | Same visit order, neighbour rule (`back==cluster ? front : back`, with back = +0) and visited-before-test order. Uses a `std::vector<bool>` instead of the stamps. `native_tu15_world_memory.cpp` comment (mention). |
| **14CEA0** | `source_object_light_portal_intersects` (`object_light_spatial.cpp`) | **algorithmic host model, bit-exact arithmetic** (retail 14A370) | PC53 `double` distance, unrounded reject, `float(d)` projection, double sqrt, the same projection table and the same edge test. Reads the decoded `ObjectLightSpatialBsp` from the cache file and uses one plane set for both reads (see 3.9). `native_tu15_world_memory.cpp:1344` mounts its table (mention). |
| 123210 | inline in `source_object_light_portal_intersects`; `posed_collision.cpp::projection_axis` | algorithmic | Other hits (fonts, sky profiles, weapon docs) are spurious or unrelated. |
| 243450 | the edge loop in `source_object_light_portal_intersects` | algorithmic | The sky profiles hit is spurious. |
| 16E6A0 | in-unit hook `native_weapon_effect_datum_allocate_tu15_run` (`src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp:1638`) and `native_tu15_data_array_allocate[_abi]` (`src/core/native_tu15_data_array_allocate.cpp`) | **full native, memory-level** | Host model: `SourceObjectSpatialDatumPool::allocate`. |
| C0F60 | `native_tu15_detach_object_tree` (`src/physics/native_tu15_object_detachment.cpp`) | **full native** with C5250/C4D30/C3750 as services | The header labels C4D30 "delete_attachment". The code here shows C4D30 is a light *reconnect* (11E600 plus 1D2F10 on 0x4ECBE0). Worth re-checking that label. |
| B1870 | `native_tu15_object_phantom_delete_protected` (`src/physics/native_tu15_phantom_added_listener.cpp`) | **full native** | |
| BA570 | `native_tu15_delete_object` (`src/physics/native_tu15_object_deletion.cpp`) | **full native** with services | |
| 282C10 | `native_tu15_process_dirty_island_objects` (`src/physics/native_tu15_dirty_island_objects.cpp`) | **full native** with services | |
| 1496F0 | none (service slot `collect_memory` only) | mention | |
| B8D90, 10DDE0, B92C0, B9330, 1CC290, 1CC3F0, 1CC440, 1CC190 | none found | none | 1CC310 is referenced only as a service (`set_component_flags`). |

### 5.1 The three files named in the brief

* **`src/platform/native_tu15_world_memory.cpp`** contains **no code** for
  14D0E0, 14CDB0 or 14CEA0. It only declares their memory ABI:
  `0x4F3CE4` (4 bytes, "1D2F10 physics query invocation counter"),
  `0x4F3CE1` (1 byte, "active traversal flag"), `0x4F3CE8` (512x4 stamps,
  ending at `0x4F44E8`), the `0x44AC8C` projection table (exact values),
  `0x474C24` initial `0x0001FFFF`, the `0x4ECC0C` 16-byte binding block and
  `0x46A0E8 = 0`. A native must read and write these exact addresses. Nothing
  else here is reusable.
* **`src/physics/native_tu15_camera_geometric_query.cpp`** is the native
  **16C620** camera/collision query. It reuses the native 14CDB0 and 11E600
  (`native_tu15_constraint_effect_point_leaf` / `_location`) through `child()`
  wrappers. It drives the same epoch machinery for its own leaf-cluster walk:
  `0x4F3CE4 += 1` at PC 16C843, `0x4F3CE1 = 1` at 16C849, stamps at 16C8D8.
  It does **not** implement 14D0E0 or 14CEA0 (it walks leaves from a BSP
  segment query, not the portal graph). Reusable parts: its `Memory` /
  `Environment` helpers (x87 CW install with fnsave/frstor, SSE inline-asm
  ops, store events with PCs) as a template, and the two natives it calls.
* **`src/campaign/source_campaign_environment_view_tu15.cpp`** has **no
  native** for these routines. It calls the **translated** 14CDB0 through
  `world.run_actual(0x14cdb0u, query)` with `eax=0, ecx=rd32(sbsp+0x18),
  edx=eye`. It could switch to `native_tu15_constraint_effect_point_leaf`,
  which is read-only with identical semantics.

## 6. Implementation guidance

Recommended order, smallest and hottest first. All kernels should be
memory-level (`SourcePhysicsContactMemory` plus an observer with the original
store PCs), following the 16E6A0 / 11E600 natives.

1. **14CEA0 + 123210 + 243450 as one native**: port
   `source_object_light_portal_intersects` to read the portal, its plane
   (through `*(sbsp+0x18)`) and the axis/projection plane (through
   `*(0x4ECC0C)`) from memory. It has no externally visible stores. Return a
   bool. Hook it at 14CEA0 if needed. The only live register output is `al`;
   ecx and edx are dead in 14D0E0.
2. **14D0E0 native** (calls 1): an exact DFS. It must emit the conditional
   stamp store (14D12D) only when the value differs, re-read `0x4F3CE4` for
   each neighbour check, re-read `cluster+0x8C`, write out-buffer entries only
   while `remaining > 0`, and return the full count. Iterative or recursive is
   fine if the order is the same. Recursion depth is at most the cluster count
   (512). Hooking at 14D0E0 also accelerates BD080, 181320 and 1839F0, but
   check their use of ecx and edx (OPEN; the original returns
   `ecx = rd32(0x4ECC18)`).
3. **14D080 and 1D2F10 natives**: use 1, 2 and
   `native_tu15_data_array_allocate` for both pools, with the stores listed in
   3.6. That covers both the object partitions and the light partition (C4D30
   passes size 0 and payload NULL).
4. **BA630 native**: `native_tu15_constraint_effect_location` (twice),
   `native_tu15_build_object_spatial_packet`, native 1D2F10, then
   `native_tu15_detach_object_tree(object,0,1,0,0)`. C4D30 stays a service
   until it is ported (it is itself a 1D2F10 client). Then small new natives
   for B8D90 (10DDE0 callbacks as services), B92C0 and B9330 (1CC3F0 / 1CC440
   as Havok services), the native B1870, the native BA570, and 1CC290
   (1CC190 / 1496F0 / 282C10 as services). The host model
   `SourceObjectRegistrationJournal::link` is a correct reference for the
   registration prefix and can serve as the oracle in tests.
5. Validation: compare store sequences against the translated body per call
   (the existing `SourcePhysicsContactRecordStore` observer pattern), then
   byte-compare frames per the parity-capture workflow.

## 7. OPEN items

* The function pointers in the object-type definition table `0x474B90`
  (`def+0x84[i]+0x48`) used by 10DDE0 for machines (type 7). What they do and
  which stores they make are not determined here.
* The meaning of portal `+0x18`. Not read by this tree.
* The name and purpose of `[[0x4EABC4]+4]` (the active counter for object flag bit14).
* Full specs for C4D30 (262 instructions), C3750, C5250 / 1D30F0, and for the
  Havok subtrees under 1CC3F0 / 1CC440 / 1CC190 / 1CC310 and WR 1496F0.
  These are boundary services in this spec.
* The register outputs (ecx/edx) of 14D0E0 as consumed by its other callers
  BD080, 181320, 1839F0 and 383C90.
* That the x87 control word at every BA630 entry is in fact `0x027F` (PC53).
  The natives should assert it rather than assume it.
* That `rd32(sbsp+0x18) == rd32(0x4ECC0C)` holds for every BSP. It is assumed
  by the host model and is not proven here.
