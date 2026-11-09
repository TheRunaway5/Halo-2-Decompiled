# 1CD620: Havok component set-position (TU1.5), native replacement spec

Status: read-only research spec, 2026-11-08. Nothing here was built or run.
Source of truth is the translated code. Abbreviations used below:

| tag | file | notes |
|---|---|---|
| **LS** | `src/physics/physics_live_support_tu15_generated.cpp` | `static void run_<tu15>(WorldState &s)`, TU1.5 PCs |
| **WR** | `src/physics/runtime/physics_world_runtime_generated.cpp` | `run_<canonical>(ObservedWorldState&)`; canonical = MapPack11/TU1.1 address; the TU1.5 PC is the 2nd argument of `world_observation_pc(canon,tu15,s)`; global operands go through `selected()` |
| **VM** | `src/physics/vehicles/physics_vehicle_motion_generated.cpp` | canonical (TU1.1) addresses, `selected()` remap |
| **XBE** | `~/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe` (TU1.5 image) | read only, to resolve vtables and constants (section table parsed; `.rdata` VA 0x41B600 at file offset 0x417000) |

Line numbers are for the working tree on that date. "Instr" means the number
of lines in a body containing `s.services.instruction`. A `rep movs`/`rep stos`
line counts once, though it runs 13 iterations here. All addresses are TU1.5
unless marked *canon*.

**Routing.** LS calls go through `dispatch()` when the target has a case in the LS
switch (LS line 2373). Otherwise they go through `s.required()`, then
`PhysicsLiveWorldTu15::Impl::required` (`src/physics/physics_live_world_tu15.cpp`
1584), then `required_entry_original`, then `live_entry()` (table
`src/physics/physics_live_entries_tu15.hpp`, `{actual, canonical, Module}`),
then the module runner. Module::World runs WR at the canonical address.
Module::Phantom runs `physics_phantom_world_tu15_generated.cpp`. That file's
`vehicle_entries` lead to the phantom-force/vehicle units, which end in VM.

---

## 0. Summary, and corrections to the brief

* **1CD620 is a 40-instruction gate, not the Havok code.** It reads the object's
  Havok component handle (object+0xB4). It tests three component flag bits
  against two bool arguments. Then it calls **1DC270**, the real
  "set component position" routine. 1CD620 has no memory stores of its own and
  returns no value that anyone consumes.
* **1DC270 has three paths**, selected by component flags:
  * `0x400`: remove the component from the world, reset it, run BF050 node
    rebuild, rebuild it (1D73D0), then re-insert it.
  * `0x200`: move the single root body to the object's transform.
  * otherwise: move every body by the delta `object_now * inverse(reference_before)`,
    optionally Gram–Schmidt each basis, then set flag `0x10000`.
* **"Static bodies removed and re-added" happens per body in 1D8FA0.** When a
  rigid body is fixed (`rb+0x40 != 0`) and the component is in the world (flag
  `0x20`), 1D8FA0 does three things. It removes the whole component from the
  hkWorld (1D9510), sets the body transform (1E1130), and re-inserts the whole
  component (1D97F0). In a multi-body component this happens once for each
  fixed body.
* **Every other body takes the cheap path.** If the body's island is inactive,
  it is activated (315CB0). Then `motion->vtbl[0x4C](T)` runs. In the TU1.5 XBE,
  all 7 motion vtables (`0x41E3D8`, `0x41E458`, `0x41E4D8`, `0x41F1E0`,
  `0x41F260`, `0x41F2E0`, `0x41F360`) hold **323B20** at +0x4C (and 3239F0 at
  +0x50). 323B20 is an LS function: hkRigidMotion setTransform.
* **The B9460 ordering in the brief is correct** (LS 7985-8111). The order is:
  BA7E0 at B949C (only if object flag bit 8 is set and arg8 == 0). Then the
  position store at B94F2, with a ±32768 bounds check only when
  `object+0x14 == -1`. Then B7E50(…,2) at B951A, the forward/up stores and
  B7E50(…,4) at B956A. Then **1CD620** at B958A (only if arg6). Then BF050 at
  B959C (only if arg5). BA630 runs last, at B95BB.
* **Arguments on the presented-root path.**
  `PhysicsLiveWorldTu15::Impl::set_object_position` (physics_live_world_tu15.cpp
  4532) calls `B95D0(eax=&point, ecx=0, edx=0, stack: object, 0, 0)`. B95D0
  then calls `B9460(obj, &pos, NULL, NULL, 0, 1, 1, 0, 0)`. So 1CD620 receives
  `position_is_null = 0` and `allow_flag80 = 0`. As a result, **machines
  (component flag 0x80) and anchored biped/creature controllers (flag 0x100)
  are skipped**. Everything else with a component reaches 1DC270.
* **Bipeds are not skipped by default.** The TU1.5 biped/creature controller
  builder 1DEFB0 sets component flags `|= 0x203` (LS site 0x1DF0EA). It sets
  `0x100` only for anchored physical states 4/5 (site 0x1DF106). A normal biped
  therefore takes the `0x200` path. That path has a biped-only z offset (E32C0)
  and finishes in 323B20 on body 0.
* **Broadphase.** The XBE vtable apparently starting at `0x41C6C0` (preceded by float constants) has `+4 = 2EC4B0` (addObject)
  and `+0xC = 2EC9B0` (removeObject). Both are reached by virtual calls inside
  hkWorld addEntity (319370) and removeEntity (3198A0). The exact chain is OPEN
  (§7).

---

## 1. Call graph (TU1.5 addresses, instr counts, conventions)

```
1CD620 (40)  object_physics_set_position          eax=object, stack(bool pos_null, bool allow80)  ret 8   LS 114107
 +- 1DC270 (243) component_set_position            eax=component row, stack(component_index)       ret 4   LS 124131
     |-- [flag 0x400] full rebuild
     |    +- 1D9510 (218) component_remove_from_world   ecx=row                ret 0   LS 122576
     |    +- 1D8470 (190) component_reset               stack(row)             ret 4   LS 122064 (native exists)
     |    +- BF050  (35)  object_rebuild_nodes          stack(object)          ret 4   LS 13625  (246-routine LS closure)
     |    +- 1D73D0 (69)  component_rebuild             eax=component_index    ret 0   LS 121495
     |    +- 1D97F0 (218) component_insert_into_world   ecx=row                ret 0   WR 4749 (canon 1D4F30)
     |-- [flag 0x200] single root body
     |    +- BC190 (36)  object_world_transform         eax=object, stack(out34)  ret 4  VM 397 (canon BB050)
     |    |   +- 144BF0 (53) basis_from_pos_fwd_up      esi=pos,ecx=fwd,eax=up,edx=out   VM 5482 (canon 143860)
     |    |   +- 145560 (69) [only if parented]
     |    +- E32C0 (19)  biped_z_offset [bipeds]        eax=object, stack(float*) ret 4, AL   LS 36207
     |    +- 1D8ED0 ... (below)
     |-- [else] multi-body delta
          +- 1DC610 (35) component_reference_transform  edi=row, stack(out34)  ret 4   LS 124378
          |   +- 1D8B90 (55) component_body_transform    eax=i, ecx=row, esi=out34   ret 0   WR 4538 (canon 1D42D0)
          |   |   +- 2E4650 (20) hkTransform_setMulEq [flag 0x40]   ecx=this, stack(other) ret 4   WR 12507
          |   |       +- 2E44D0 (40, 4-iter loop) hkTransform_setMul ecx=out, stack(A,B) ret 8   WR 12380
          |   +- BC190 (when no valid root slot)
          |   +- 1DC680 (90) component_frame_to_object   eax=row, ecx=src34, stack(dst34) ret 4  LS 124417
          |       +- E32C0 / 145560
          +- BC190
          +- 144090 (82) transform_inverse              ecx=src34, eax=dst34   ret 0   WR 966 (canon 142D00)
          +- 145560 (69) transform_compose               ecx=A, edx=B, stack(out) ret 4  LS 79263
          +- per body i: 1D8B90, 145560, 1D8ED0

1D8ED0 (43) component_set_body_transform34   eax=src34, ecx=i, stack(row)   ret 4   LS 122329
 +- 1D8FA0 (79) component_set_body_transform40   eax=hkTransform*, ecx=i, ebx=row   ret 0   LS 122376
     +- 2E4470 (33) hkTransform_setInverse [flag 0x40]  ecx=dst, stack(src) ret 4   LS 169260
     |   +- 2E4820 (24) hkRotation_setTranspose          ecx=dst, stack(src) ret 4   WR 12628
     +- 2E4650 [flag 0x40]
     +- [fixed body AND row flag 0x20]  1D9510 ; 1E1130 ; 1D97F0
     +- [else] 315C90 (10) island_is_active   ecx=rb, stack(u8*) ret 4, eax=out   WR 23484
     |         315CB0 (18) rigid_body_activate [inactive]  ecx=rb  ret 0   WR 23498
     |         motion->vtbl[0x4C] = 323B20
1E1130 (25) rigid_body_set_transform   esi=rb, stack(hkTransform*) ret 4   LS 128475
 +- 315C90, 315CB0, motion->vtbl[0x4C] = 323B20
323B20 (28) hkRigidMotion_setTransform  ecx=motion, stack(hkTransform*) ret 4   LS 181631
 +- 2E4350 (94) hkQuaternion_setFromRotation  ecx=out quat, stack(rotation*) ret 4   LS 169162
 +- 2E4690 (26) transform_point4   ecx=out16, stack(hkTransform*, vec*) ret 8   WR 12531
```

Translated sizes of the static closures, computed by following direct calls
and LS switch cases inside LS only. World/Phantom modules are not followed:

| root | LS routines | LS static instr | `required()` targets | virtual call sites |
|---|---|---|---|---|
| 1CD620 | 251 | 23,997 | 89 | 185 |
| BF050 (inside the 0x400 path) | 246 | 23,567 | 88 | 183 |
| 1D73D0 | 64 | 5,658 | 46 | 71 |
| 1D8ED0 | 9 | 592 | 15 | 7 |
| 1D9510 | 5 | 412 | 10 | 5 |
| 1DC610 | 4 | 213 | 2 | 0 |
| 323B20 | 2 | 122 | 1 | 0 |

The WR closure of 1D97F0 (canon 1D4F30) holds 192 routines and ~9,049 static
instructions. Most of that is 282C10's dirty-island subtree, which reaches
1496F0 memory collection and 1CB220 object physics teardown.

---

## 2. Data layouts and constants

### 2.1 Globals and constants read

| address | meaning | value / use |
|---|---|---|
| `0x4ECBD0` | object datum array header | `+0x44` data base. Header rows are 12 bytes: `+0` salt (u16), `+2` flags byte (bit0 = active), `+3` object type, `+8` datum pointer. 1D9510 also reads `+0x24` (row size) and `+0x38` (capacity) |
| `0x52BE5C` | Havok component data array header | `+0x44` rows base, stride `0xA0` |
| `0x4F0414` | tag instance table base | `tag_data(h) = *(u32*)(*(u32*)0x4F0414 + (h&0xFFFF)*16 + 8)` |
| `0x51DF54` | game time globals | `+4` seconds per tick (float) |
| `0x52BE48` | hkWorld* | `this` for world add/remove calls |
| `0x52D880` | game entity listener object | added/removed per rigid body; `+4` is the activation listener interface |
| `0x48B868` | count of rigid bodies in the world (s32) | `++` in 1CC560, `--` in 1D9510; limit test `>= 0x352` (850) |
| `0x48C938` | physics allocator object | vtable `+0x10` alloc(size, tag), `+0x14` free(ptr, size, tag) |
| `0x52D690` | data array, 64-byte rows (model constraint rows; see 22D0A0) | used by 1D9510 |
| `0x46A0EC` | float 1.0e-4 (`0x38D1B717`) | normalize epsilon |
| `0x46A0D0` | float 1.0 | scale / reciprocal numerator |
| `0x46A0E8` | float 0.0 | 2E4350 trace test |
| `0x46A0CC` | float 0.5 | 2E4350 |
| `0x41BF00` | float 1.0e6 (`0x49742400`) | 323B20 adds it to motion+0x6C |
| `0x44A8A0` | float 0.15 (`0x3E19999A`) | E32C0 |
| `0x461960` | 16-byte sign mask (`0x80000000` lanes) | 2E4470 |
| `0x461970` | vtable stored in 1D9AD0's 0x70-byte record | canon `0x45CAD0` |
| `0x1D74BC` / `0x1D74A4` | 1D73D0 type-remap bytes / 6-entry jump table | see §4.3 |

### 2.2 Object datum fields read in this tree

`+0x00` definition tag handle. `+0x14` parent. `+0x18` (s8) parent node index.
`+0x64` position. `+0x70` forward. `+0x7C` up. `+0x88` linear velocity and
`+0x94` angular velocity (both read by 1D97F0). `+0xB4` component handle.
`+0xC1` byte, bit0 passed to 1CC560, which then may call 315CE0 (presumed
"start deactivated"; OPEN). `+0x116` (s16,
parent) node matrix offset; node matrices are 0x34-byte transform34 rows.
`+0x3AA` (s8, bipeds) z-offset tick count (E32C0). The header row `+3` holds the
type (same value as `+0xAA`). Bipeds also have `+0x3DC` (u8) and `+0x3E4`
(component owner), both written by 1D9510.

### 2.3 Havok component row (`0x52BE5C` data, 0xA0 bytes), fields used

| off | type | use |
|---|---|---|
| +0x04 | u32 | flags (§2.6) |
| +0x08 | u32 | owning object handle |
| +0x18 | s8 | root body slot (`-1`/0xFF = none) |
| +0x19 | u8 | root body's render-model node index (0xFF = none; sign-extended when used) |
| +0x1C | u8 | grouping result (1D73D0) |
| +0x20 | u32 | handle into `0x52D690` rows (constraint list head, 1D9510) |
| +0x30 | hkTransform (64 B) | body-offset transform, valid when flag 0x40 |
| +0x70 / +0x74 | ptr / s32 | body slots / count (slot stride 0x60) |
| +0x7C / +0x80 | ptr / s32 | constraint records (12 B; constraint at +8) / count |
| +0x88 / +0x8C / +0x90 | hkArray (ptr,size,cap\|0x80000000) of 72-byte elements | freed by 1D9510 |
| +0x94 | ptr | list header `{rows(8 B: s8 phantom-type index, u32 object handle), count}` (1D9510) |
| +0x98 / +0x9C | ptr | phantoms (removed/added with the world) |

### 2.4 Body slot (`row+0x70 + i*0x60`) and hkRigidBody fields used

Slot `+0x40` holds the `hkRigidBody* rb`. On rb: `+0x06` u16 reference count
(the same `+6` refcount convention applies to the 0x70-byte records). `+0x08` hkWorld* (0 = not in a world). `+0x30`/
`+0x34` property array (8-byte `{key,value}`; key 0x2001 = component handle,
0x2002 = slot). `+0x3C` hkMotion*. `+0x40` u8 "fixed" (non-zero = fixed motion).
`+0x44` island*. `+0x54` refcounted per-body record (replaced by 1D97F0).
`+0x5C` u16 (zeroed by 1D97F0). `+0x60`/`+0x64` activation-listener array and
count. `+0x70`/`+0x74`/`+0x78` entity-listener array, count, and capacity.
On the island: `+0x24` (s32, -1 = none), `+0x29` u8 active, `+0x40` (1CC560
compares it to 1).

### 2.5 hkMotion fields written by 323B20

`+0x20` local centre of mass (read). `+0x30` quaternion (4 floats). `+0x6C`
float, incremented by 1.0e6. `+0x70` world centre of mass (16 bytes; `+0x7C`
then zeroed). `+0x80..+0xBF` hkTransform (rotation columns, translation). The
vtable is at `+0`: `+0x18` motion type (1D9510/1D97F0 compare it to 6),
`+0x4C` setTransform, `+0x50`/`+0x54` linear/angular velocity setters.

### 2.6 Layouts: transform34 vs hkTransform40

* **transform34** (Halo `real_matrix4x3`, 0x34 bytes): `+0` scale, `+4` forward,
  `+0x10` left, `+0x1C` up, `+0x28` position.
* **hkTransform** (0x40 bytes, 16-byte aligned): `col0=forward(x,y,z,w)`, `col1=left`,
  `col2=up`, `col3=translation`. 1D8ED0 writes w = 0 and drops the scale.

### 2.7 Component flag bits used here (all verified in TU1.5 setters unless noted)

| bit | meaning (evidence) |
|---|---|
| 0x1 | component enabled/initialised (1D73D0 0x1D7480 `|=1`) |
| 0x2 | set with 0x203 by controller builder 1DEFB0 (0x1DF0EA). 1D9100 skips angular velocity when set (native_tu15_object_motion.cpp:314). 1CD620 skips when `pos_null` |
| 0x8 | cleared by 1D97F0 on insertion |
| 0x20 | in hkWorld (`|=` by 1D97F0 at TU1.5 site 0x1D9ABA, `&=~` by 1D9510 at 0x1D97E3) |
| 0x40 | body-offset transform at row+0x30 is valid |
| 0x80 | machine component (1D73D0 0x1D7449, type 7 branch). 1CD620 skips unless allow80 |
| 0x100 | anchored controller (1DEFB0 0x1DF106, physical state 4/5). 1CD620 always skips |
| 0x200 | single root body not bound to a node (1DED80 0x1DEE01 set / 0x1DEE15 clear; controller 0x1DF0EA) |
| 0x400 | constrained to the world's fixed body (1DFA80 0x1E0174 set, 0x1E0103 clear; TU1.1 native `physics_body_grouping.cpp`) |
| 0x800 | scaled shapes requested (1DDBF0 0x1DDE6C). 1D97F0 then builds scaled wrappers |
| 0x10000 | "positioned once". Set by 1DC270 (0x1DC5F8). When set, the multi-body path orthonormalises. Other RMW sites with the same constant on a `+4` word exist at 0xB9DFC (`|=`) and 0xBE680 (`&=~`); whether they target component rows is not verified (OPEN) |
| 0x20000 | header-active mirror (1D73D0) |

---

## 3. Routine specifications

Notation: `datum(h) = *(u32*)(*(u32*)(*(u32*)0x4ECBD0 + 0x44) + (h&0xFFFF)*12 + 8)`,
`hdr(h) = rows + (h&0xFFFF)*12`, `row(c) = *(u32*)(*(u32*)0x52BE5C + 0x44) + (c&0xFFFF)*0xA0`.
"Stack store" means a store into the routine's own frame or its outgoing
argument area. Prior TU1.1 replicas (biped_body_placement.cpp,
physics_body_matrix.cpp) reproduce those stores at frame addresses derived
from the entry ESP. Whether a TU1.5 replacement must do the same is a policy
choice (§8).

### 3.1 1CD620 — object_physics_set_position (LS 114107-114149, 40 instr)

Convention: `eax` = object handle. Stack `[esp+4]` = `pos_null` (byte; caller
B9460 passes `SETE(position==NULL)`). `[esp+8]` = `allow80` (byte; low byte of
B9460's 8th stack argument). Callee pops 8 bytes. Saves `esi`. Returns nothing
(EAX is garbage: the datum pointer on early exit, or 1DC270's EAX).

```c
void object_physics_set_position_1CD620(u32 object /*eax*/, u8 pos_null, u8 allow80) {
  u32 obj  = datum(object);                       // reads 0x4ECBD0
  u32 comp = *(u32*)(obj + 0xB4);
  if (comp == 0xFFFFFFFF) return;
  u32 C = row(comp);                              // reads 0x52BE5C
  u32 f = *(u32*)(C + 4);
  if ((f & 0x2) && pos_null) return;              // 1CD661..1CD66C
  if (f & 0x100) return;                          // 1CD676 -> 1CD680..1CD68A (always returns)
  if ((f & 0x80) && !allow80) return;             // 1CD67B..1CD686
  component_set_position_1DC270(/*eax=*/C, /*stack*/ comp);   // push esi; call (callee pops)
}
```
Memory stores: none (only pushes). Globals: `0x4ECBD0`, `0x52BE5C`.
Callers: B9460 (B958A), E5181 and 1EDBA3. The latter two pass `(obj, 0, 0)`.

### 3.2 1DC270 — component_set_position (LS 124131-124376, 243 instr)

Convention: `eax` = component row pointer C. `[esp+4]` = component index
(same as object+0xB4). Callee pops 4 bytes. Frame: `sub esp,0x13C; push ebx;
push edi`, so the frame base is `F = entry_esp - 0x144`. Returns nothing.

```c
void component_set_position_1DC270(u32 C /*eax*/, u32 comp_index) {
  u32 f = *(u32*)(C+4);                                      // read once (ebx)
  if (f & 0x400) {                                           // ---- full rebuild
    u8 was_in = (f >> 5) & 1;                                // bl
    if (was_in) component_remove_from_world_1D9510(/*ecx*/C);
    component_reset_1D8470(/*stack*/C);
    object_rebuild_nodes_BF050(/*stack*/ *(u32*)(C+8));
    component_rebuild_1D73D0(/*eax*/ comp_index);            // eax = [esp+0x148]
    if (was_in) component_insert_into_world_1D97F0(/*ecx*/C);
    return;
  }
  if (f & 0x200) {                                           // ---- single root body
    T34 *T = (T34*)(F+0x0C);
    object_world_transform_BC190(/*eax*/ *(u32*)(C+8), T);
    u32 obj = *(u32*)(C+8);
    u8 type = *(u8*)(hdr(obj) + 3);
    if (((1u << (type & 31)) & 1) != 0) {                    // type 0 = biped
      float *off = (float*)(F+0x08);
      if (biped_z_offset_E32C0(/*eax*/obj, off))
        T->position.z = T->position.z - *off;                // SUBSS; store F+0x3C, site 0x1DC320
    }
    component_set_body_transform34_1D8ED0(/*eax*/T, /*ecx*/0, /*stack*/C);
    return;
  }
  // ---- multi-body delta
  T34 *A = (T34*)(F+0xDC), *B = (T34*)(F+0x74), *Ai = (T34*)(F+0x40), *D = (T34*)(F+0x110);
  component_reference_transform_1DC610(/*edi*/C, A);         // reference before the move
  object_world_transform_BC190(*(u32*)(C+8), B);             // object now (caller already stored it)
  transform_inverse_144090(/*ecx*/A, /*eax*/Ai);
  transform_compose_145560(/*ecx*/B, /*edx*/Ai, D);          // D = B ∘ A^-1
  if (*(s32*)(C+0x74) > 0) {
    s32 i = 0;
    do {
      T34 *X = (T34*)(F+0xA8), *Y = (T34*)(F+0x0C);
      component_body_transform_1D8B90(/*eax*/i, /*ecx*/C, /*esi*/X);
      transform_compose_145560(/*ecx*/D, /*edx*/X, Y);       // Y = D ∘ X
      if (*(u32*)(C+4) & 0x10000) orthonormalize_1DC3C0(Y, (float*)(F+0x08));   // §3.2.1
      component_set_body_transform34_1D8ED0(Y, i, C);
      ++i;
    } while (i < *(s32*)(C+0x74));                           // count reread every iteration
  }
  *(u32*)(C+4) = *(u32*)(C+4) | 0x10000;                     // RMW of current flags, site 0x1DC5F8
}
```

#### 3.2.1 Inline orthonormalisation (1DC3C0-1DC5D9)

All "x87" lines use the source x87 control word (precision/rounding). `fst`
and `fstp` round to float32. `(ext)` means a float loaded as x87 extended.
`len` is stored at `F+0x08` each time (sites 0x1DC3EA, 0x1DC462, 0x1DC4F3,
0x1DC590). The tiny test is `FCOMIP(eps=1e-4, |len|)`, then `JA`: skip when
`eps > |len|` (ordered). NaN does **not** skip.

```c
// Forward
x87 t = (ext)fy*fy;  xmm4=fy; xmm5=fx;  t = t + (ext)fx*fx;  t = t + (ext)fz*fz;  t = sqrt(t);
*(float*)(F+8) = (float)t;
if (!(eps > fabs(t))) { float k = 1.0f / *(float*)(F+8);           // DIVSS
   fx = fx*k; fy = fy*k; xmm6 = k*fz;                               // MULSS
   store Y+4=fx (0x1DC418), Y+8=fy (0x1DC41E), Y+0xC=xmm6 (0x1DC424); }
else xmm6 = fz (reload);
// Up (normalised in registers only)
x87 t = (ext)uz*uz; xmm1=uz; xmm2=uy; t += (ext)uy*uy; xmm3=ux; t += (ext)ux*ux; t=sqrt(t);
*(float*)(F+8)=(float)t;  if (!(eps > fabs(t))) { k=1/len; xmm3=ux*k; xmm2=uy*k; xmm1=uz*k; }
// Remove the forward component from up
dot = (xmm2*xmm4 + xmm1*xmm6) + xmm5*xmm3;                         // (uy*fy + uz*fz) + fx*ux, SSE
uz' = xmm1 - xmm6*dot   -> store Y+0x24 (0x1DC4B3);  x87 t = (ext)uz'*uz'
uy' = xmm2 - xmm4*dot   -> store Y+0x20 (0x1DC4C9);  x87 push (ext)uy'*uy'
ux' = xmm3 - xmm5*dot   -> store Y+0x1C (0x1DC4DF);  x87 t = (uz'^2 + uy'^2) + (ext)ux'*ux'; sqrt
*(float*)(F+8)=(float)t;  if (!(eps > fabs(t))) { k=1/len;
   store Y+0x1C = k*ux' (0x1DC522), Y+0x20 = uy'*k (0x1DC528), Y+0x24 = uz'*k (0x1DC52E); }
// left = up x forward, x87 from the stored floats
Y.left.x = (float)((ext)uy*fz - (ext)uz*fy)        fstp Y+0x10 (0x1DC546)
Y.left.y = (float)((ext)uz*fx - (ext)ux*fz)        fstp Y+0x14 (0x1DC55C)
e        = (ext)fy*ux - (ext)uy*fx;  Y.left.z = (float)e   fst Y+0x18 (0x1DC572)
x87 t = e * (ext)Y.left.z   /* extended e times the rounded float */ ; t += (ext)ly*ly; t += (ext)lx*lx; sqrt
*(float*)(F+8)=(float)t;  if (!(eps > fabs(t))) { k=1/len;
   store Y+0x10 = k*lx (0x1DC5B9), Y+0x14 = ly*k (0x1DC5C9), Y+0x18 = lz*k (0x1DC5D9); }
```
Y.scale and Y.position are untouched. This block is 100 instructions when
every normalise is skipped and 133 when none is.

Stores of 1DC270 itself (21 sites): 20 are stack (F+0x08 and Y in F+0x0C..+0x3F).
One is non-stack: component row +4 (|0x10000). Globals: `0x4ECBD0`,
`0x46A0EC`, `0x46A0D0`.

### 3.3 1DC610 — component_reference_transform (LS 124378-124415, 35 instr)

Convention: `edi` = C. `[esp+4]` = out34. Callee pops 4 bytes. Local
L = `entry_esp-0x34` (`sub esp,0x34`).

```c
void component_reference_transform_1DC610(u32 C /*edi*/, T34 *out) {
  s8 r = *(s8*)(C+0x18);
  if (r >= 0 && (s32)r < *(s32*)(C+0x74) && (s16)r != -1) {    // third test is redundant
    component_body_transform_1D8B90(/*eax*/(s32)r, /*ecx*/C, /*esi*/L);  // push/pop esi around it
    component_frame_to_object_1DC680(/*eax*/C, /*ecx*/L, out);
  } else {
    object_world_transform_BC190(/*eax*/*(u32*)(C+8), L);
    component_frame_to_object_1DC680(C, L, out);
  }
}
```
Note: on the else branch inside 1DC270, BC190 already returns the **new**
object transform. A therefore equals B, and D ≈ identity: the bodies are not
translated, only rewritten through two compositions and possibly
orthonormalised.

### 3.4 1DC680 — component_frame_to_object (LS 124417-124509, 90 instr)

Convention: `eax` = C. `ecx` = src34. `[esp+4]` = dst34. Callee pops 4 bytes.
Saves ebx/esi/edi.

```c
void component_frame_to_object_1DC680(u32 C, const T34 *src, T34 *dst) {
  if (*(u32*)(C+4) & 0x200) {
    rep_movsd(dst, src, 13);                                  // site 0x1DC69B (DF assumed 0)
    u32 obj = *(u32*)(C+8);
    if (object_type(obj) == 0) {                              // (1<<type)&1, type from hdr+3
      float *off = (float*)(entry_esp + 4);                   // ESP+0x10 = the dst ARGUMENT SLOT
      if (biped_z_offset_E32C0(obj, off))
        dst->position.z = dst->position.z + *off;             // ADDSS, store site 0x1DC6E7
    }
    return;
  }
  s8 r = *(s8*)(C+0x18);
  if (r >= 0 && (s32)r < *(s32*)(C+0x74) && (s16)r != -1) {
    rep_movsd(dst, src, 13);                                  // site 0x1DC71A
    u8 node = *(u8*)(C+0x19);
    if (node == 0xFF) return;
    u32 def  = tag_data(*(u32*)datum(*(u32*)(C+8)));          // object+0 definition
    u32 hlmt = tag_data(*(u32*)(def + 0x38));                   // presumed model (hlmt) reference handle
    u32 mode = tag_data(*(u32*)(hlmt + 0x04));                  // presumed render-model reference handle
    T34 *inv = (T34*)(*(u32*)(mode + 0x4C) + (s32)(s8)node * 0x60 + 0x28);   // node inverse bind (scale first)
    transform_compose_145560(/*ecx*/dst, /*edx*/inv, /*out*/dst);
    return;
  }
  rep_movsd(dst, src, 13);                                    // site 0x1DC795
}
```
Stores: 13 dwords into dst (caller stack in this tree), plus the 4-byte float
into the caller's argument slot (via E32C0), plus the 145560 output.

### 3.5 E32C0 — biped_z_offset (LS 36207-36228, 19 instr)

`eax` = object, `[esp+4]` = float* out. Callee pops 4 bytes. Returns AL.

```c
u8 biped_z_offset_E32C0(u32 object, float *out) {
  s8 v = *(s8*)(datum(object) + 0x3AA);
  if (v == 0) return 0;                       // AL=0 (upper EAX = (object&0xFFFF)*3)
  float t = *(float*)(*(u32*)0x51DF54 + 4) * 0.15f;          // MULSS [0x44A8A0]
  *out = t * (float)(s32)v;                   // CVTSI2SS, MULSS, site 0xE3302
  return 1;
}
```

### 3.6 1D8ED0 — component_set_body_transform34 (LS 122329-122374, 43 instr, straight line)

`eax` = src34. `ecx` = body index (passed through to 1D8FA0). `[ebp+8]` = C.
Callee pops 4 bytes. Frame: `push ebp; mov ebp,esp; and esp,-16; sub esp,0x5C;
push ebx`, so `F = ((entry_esp-4)&~15) - 0x60` (16-byte aligned). The scale at
src+0 is **ignored**.

Stack stores in order (site → F offset = value): 1D8EE4 +0x30=left.x;
1D8EEF +0x34=left.y; 1D8EFA +0x38=left.z; 1D8F05 +0x40=up.x; 1D8F10 +0x44=up.y;
1D8F1B +0x20=fwd.x; 1D8F26 +0x48=up.z; 1D8F31 +0x24=fwd.y; 1D8F3C +0x10=pos.x;
1D8F47 +0x28=fwd.z; 1D8F50 +0x14=pos.y; 1D8F5E +0x2C=0; 1D8F64 +0x3C=0;
1D8F6A +0x4C=0; 1D8F70 +0x18=pos.z; 1D8F76 +0x1C=0; 1D8F85 +0x50..+0x5F =
16-byte copy of F+0x10. The result is an hkTransform at F+0x20. Then
`1D8FA0(eax=F+0x20, ecx=i, ebx=C)`.

### 3.7 1D8FA0 — component_set_body_transform40 (LS 122376-122457, 79 instr)

`eax` = hkTransform* M (16-byte aligned). `ecx` = i. `ebx` = C. `ret` (0).
Frame: `push ebp; and esp,-16; sub esp,0x88`, so `A ≡ 8 (mod 16)`, then
`push esi; push edi`, so `G = A-8`. Local hkTransform T = A+0x08, I = A+0x48.
Active byte at G+0x0F.

```c
void component_set_body_transform40_1D8FA0(const hkTransform *M, s32 i, u32 C /*ebx*/) {
  copy16(A+0x08, M+0x00); copy16(A+0x18, M+0x10); copy16(A+0x28, M+0x20);  // 1D8FAF/1D8FB8/1D8FC1
  u32 f = *(u32*)(C+4);                                                     // read here
  copy16(A+0x38, M+0x30);                                                   // 1D8FD6 (after the pushes)
  if (f & 0x40) {
    hkTransform_setInverse_2E4470(/*ecx*/I, /*stack*/(hkTransform*)(C+0x30));
    hkTransform_setMulEq_2E4650(/*ecx*/T, /*stack*/I);                       // T = T * offset^-1
  }
  u32 slot = *(u32*)(C+0x70) + i*0x60, rb = *(u32*)(slot+0x40);
  if (*(u8*)(rb+0x40) && (*(u32*)(C+4) & 0x20)) {                           // fixed, component in world
    component_remove_from_world_1D9510(/*ecx*/C);
    rb = *(u32*)(*(u32*)(C+0x70) + i*0x60 + 0x40);                          // reread
    rigid_body_set_transform_1E1130(/*esi*/rb, /*stack*/T);
    component_insert_into_world_1D97F0(/*ecx*/C);                           // via dispatch -> WR
    return;
  }
  island_is_active_315C90(/*ecx*/rb, (u8*)(G+0x0F));                        // stores the byte
  if (*(u8*)(G+0x0F) == 0 && *(u32*)(rb+0x44) != 0) rigid_body_activate_315CB0(rb);
  if (*(u8*)(rb+0x40) != 0 && *(u32*)(rb+8) != 0) return;                   // fixed and still in a world
  u32 motion = *(u32*)(rb+0x3C);
  VCALL(motion, 0x4C)(motion, T);                                           // = 323B20
}
```

### 3.8 1E1130 — rigid_body_set_transform (LS 128475-128502, 25 instr)

`esi` = rb. `[esp+4]` = hkTransform*. Callee pops 4 bytes. `push ecx` makes a
4-byte local; the active byte is at `entry_esp-1`. This is the same logic as the
tail of 1D8FA0: 315C90 query; 315CB0 if inactive and the island exists; return
if `rb+0x40 && rb+8`; otherwise `VCALL(motion,0x4C)(motion,T)`. Called from
1D8FA0 after 1D9510, so `rb+8` is expected to be 0 and the vcall runs.
(Expected, not verified at runtime: OPEN §7.)

### 3.9 323B20 — hkRigidMotion setTransform (LS 181631-181661, 28 instr)

`ecx` = motion. `[esp+4]` = T (16-byte aligned hkTransform*). Callee pops 4
bytes. All stores go to Havok memory (the motion).

```c
void rigid_motion_set_transform_323B20(u32 m, const hkTransform *T) {
  *(float*)(m+0x6C) = (float)((ext)*(float*)(m+0x6C) + (ext)1.0e6f);   // fld/fadd [0x41BF00]/fstp, site 0x323B3B
  copy16(m+0x80,T+0x00); copy16(m+0x90,T+0x10);                     // 0x323B41, 0x323B48
  copy16(m+0xA0,T+0x20); copy16(m+0xB0,T+0x30);                     // 0x323B50, 0x323B58
  hkQuaternion_setFromRotation_2E4350(/*ecx*/m+0x30, /*stack*/T);   // reads T, not m+0x80
  transform_point4_2E4690(/*ecx*/m+0x70, /*stack*/ m+0x80, m+0x20); // world COM, 16-byte store 0x2E46D5
  *(u32*)(m+0x7C) = 0;                                              // 0x323B6F
}
```
* **2E4350** (LS 169162-169258, 94 instr). Rotation `R` = 3 columns of 16 bytes.
  `trace = R[0x14] + R[0x00] + R[0x28]` on x87, in that order. The positive
  branch is taken iff `FCOM 0.0` gives `trace > 0` (C0 and C3 clear; unordered
  takes the else branch). Positive branch:
  `s = sqrt(trace + 1.0); h = 0.5/s; q.x = (R[0x18]-R[0x24])*h; q.y = (R[0x20]-R[0x08])*h;
  q.z = (R[0x04]-R[0x10])*h; q.w = s*0.5`. Stores are fstp to out+0/+4/+8/+0xC
  (sites 2E4387/2E4391/2E439C/2E43A7). The else branch picks the largest
  diagonal via a stack table `{1,2,0}` (stack stores at 2E43BD/2E43C7/2E43CF)
  and then the standard formula. It is identical in structure to TU1.1 2E0860,
  replicated in `source_biped_motion_transform_1_1` (biped_body_placement.cpp).
  Constants: `0x46A0E8`, `0x46A0D0`, `0x46A0CC`.
* **2E4690** (WR 12531-12559, 26 instr). `ecx` = out16. `[ebp+8]` = hkTransform*.
  `[ebp+0xC]` = vec*. Callee pops 8 bytes. All 4 lanes:
  `out = (col0*v.xxxx + col1*v.yyyy) + (col2*v.zzzz + col3)`. Exact SSE order:
  `t3 = col2*vz; t3 += col3; t2 = col1*vy; t0 = col0*vx; t0 += t2; t0 += t3`.

### 3.10 315C90 / 315CB0 — island query / body activation (WR 23484 / 23498)

* `315C90(ecx=rb, [esp+4]=u8* out)`, callee pops 4 bytes, returns `eax=out`:
  `isl = rb+0x44; *out = isl ? *(u8*)(isl+0x29) : 0` (sites 315C9B / 315CA8).
* `315CB0(ecx=rb)`, ret 0:
  `if (rb+8) { 3165D0(stack rb); isl=rb+0x44; if (*(u8*)(isl+0x29)==0 && *(s32*)(isl+0x24)!=-1) 316D40(ecx=rb+8 world, stack isl); }`.
  316D40 runs the world's island-activation listeners (322660, with virtual
  calls; the game listener is TU1.5 282870). A full native exists:
  `native_tu15_activate_physics_body`.

### 3.11 1D8B90 — component_body_transform (WR 4538-4595, 55 instr; canon 1D42D0)

`eax` = i. `ecx` = C. `esi` = out34. `ret` (0). Aligned scratch
`S = ((entry_esp-4)&~15) - 0x40`.

```c
rb = *(u32*)(*(u32*)(C+0x70) + i*0x60 + 0x40); m = *(u32*)(rb+0x3C);
x0 = load16(m+0x80); f = *(u32*)(C+4);                   // flags read after the first load
store16(S+0x00,x0) 1D8BB8; store16(S+0x10,m+0x90) 1D8BC0; store16(S+0x20,m+0xA0) 1D8BCF; store16(S+0x30,m+0xB0) 1D8BD8;
if (f & 0x40) hkTransform_setMulEq_2E4650(/*ecx*/S, /*stack*/C+0x30);   // S = S * offset
out->scale = *(float*)0x46A0D0;                           // 1.0, site 1D8BF4
out->fwd  = S[0],S[1],S[2]   (1D8BFD,1D8C08,1D8C13);  out->left = S[4..6] (1D8C1E,1D8C29,1D8C34)
out->up   = S[8..10] (1D8C3F,1D8C4A,1D8C55);          out->pos  = S[12..14] (1D8C60,1D8C6B,1D8C76)
```
* **2E4650** (WR 12507): copies `this` to an aligned local L (four 16-byte
  stores 2E465F/2E4667/2E4675/2E467F), then `2E44D0(ecx=this, A=L, B=other)`.
* **2E44D0** (WR 12380): for each of B's 4 columns,
  `out.col[k] = (A.col0*b.x + A.col1*b.y) + A.col2*b.z` (16-byte store
  2E4522 ×4). Then `out.col3 = out.col3 + A.col3` (2E4538).

### 3.12 2E4470 / 2E4820 — hkTransform setInverse (LS 169260 / WR 12628)

`2E4470(ecx=dst, [ebp+8]=src)`, callee pops 4 bytes. First
`2E4820(ecx=dst, stack src)` transposes the rotation with w = 0. Twelve
dword stores in order: dst+0=s+0, +0x14=s+0x14, +0x28=s+0x28, +0xC=0, +0x1C=0,
+0x2C=0, +4=s+0x10, +0x10=s+4, +8=s+0x20, +0x20=s+8, +0x18=s+0x24, +0x24=s+0x18
(sites 2E4826…2E4860). Then `n = src.col3 XOR mask(0x461960)` and
`dst.col3 = (dst.col0*n.xxxx + dst.col1*n.yyyy) + dst.col2*n.zzzz`
(16-byte store, site 2E44C4).

### 3.13 145560 — transform_compose (LS 79263-79334, 69 instr, no branches)

`ecx` = A. `edx` = B. `[esp+4]` = out. Callee pops 4 bytes. `out = A∘B` (B first).
Safe when out aliases A or B: every input is read before the store that could
overwrite it. Lanes are loaded as `{x,0,y,z}` (MOVSS + MOVHPS). Lane 1 is
computed on zeros, so it can raise MXCSR sticky flags only.
```
out.pos  = ((B.pos.x*A.fwd + B.pos.y*A.left) + B.pos.z*A.up) * A.scale + A.pos
out.up   =  B.up.x*A.fwd   + (B.up.y*A.left   + B.up.z*A.up)
out.left =  B.left.x*A.fwd + (B.left.y*A.left + B.left.z*A.up)
out.fwd  =  B.fwd.x*A.fwd  + (B.fwd.y*A.left  + B.fwd.z*A.up)
out.scale = B.scale * A.scale
```
Store order: out+0x28 (1455CD), +0x2C/8 B (1455D2), +0x1C (145600), +0x20/8 B
(145605), +0x10 (145633), +0x14/8 B (145638), +0x04 (145661), +0x08/8 B
(145666), +0x00 (14566A).

### 3.14 144090 — transform_inverse (WR 966-1050, 82 instr; canon 142D00)

`ecx` = src. `eax` = dst. `ret` (0). Not alias-safe (it rereads dst). If
`src.scale == 0` (UCOMISS, ordered equal), it zeroes 13 dwords (`rep stosd`,
site 1441B7). Otherwise `n = 0 - src.pos`. If `scale != 1`, then `k = 1/scale`,
`dst.scale = k`, `n = k*n`; else `dst.scale = 1.0`. The rotation is transposed
into dst. `dst.pos.x = (dst[0x1C]*nz + dst[4]*nx) + dst[0x10]*ny`;
`dst.pos.y = (dst[0x20]*nz + dst[8]*nx) + ny*dst[0x14]`;
`dst.pos.z = (dst[0xC]*nx + dst[0x24]*nz) + dst[0x18]*ny`. Store order:
1440E2|1440F6, 1440FD, 144108, 14410E, 144119, 14411E, 14412D, 144142, 14415C,
144165, 144181, 144194, 1441A9. A complete memory-level native already exists:
`native_tu15_camera_inverse_frame`.

### 3.15 BC190 — object_world_transform (VM 397-435, 36 instr; canon BB050)

`eax` = object. `[esp+4]` = out34. Callee pops 4 bytes. Returns `eax = out`.
It runs `144BF0(esi=obj+0x64, ecx=obj+0x70, eax=obj+0x7C, edx=out)` (basis:
scale 1, fwd, `left = up×fwd`, up, pos; VM 5482). Then, if
`obj+0x14 != -1`, `node = datum(parent) + (s8)obj[0x18]*0x34 + (s16)parent[0x116]`
and `145560(ecx=node, edx=out, out)`. Value-level native:
`native_tu15_object_transform` (native_tu15_object_motion.cpp).

### 3.16 1D9510 — component_remove_from_world (LS 122576-122796, 218 instr)

`ecx` = C. `ret` (0). Frame `sub esp,0xC` plus 4 pushes. Locals: result
bytes at frame+0x12/+0x13, loop counter at frame+0x14, phmo pointer at
frame+0x18.

```c
for (s32 k = 0; k < *(s32*)(C+0x80); ++k)                          // count reread
  hkWorld_removeConstraint_316AA0(/*ecx*/*(u32*)0x52BE48, &res8, *(u32*)(*(u32*)(C+0x7C)+k*12+8));
for (s32 i = 0; i < *(s32*)(C+0x74); ++i) {
  rb = *(u32*)(*(u32*)(C+0x70)+i*0x60+0x40);
  entity_remove_listener_315D60(/*ecx*/rb, *(u32*)0x52D880);       // sets rb.listeners[idx]=0 (+0x70 array);
                                                                   // if not found writes 0 at array[-1]
  if ((*(u8*)(C+4) & 1) && !*(u8*)(rb+0x40) && VCALL(rb->motion,0x18)() != 6)
    entity_remove_activation_listener_315DA0(rb, *(u32*)0x52D880 ? *(u32*)0x52D880+4 : 0);   // +0x60 array
  *(s32*)0x48B868 -= 1;                                            // site 0x1D95B3 (game global)
  hkWorld_removeEntity_3198A0(/*ecx*/*(u32*)0x52BE48, &res8b, rb);
  process_dirty_islands_282C10();                                  // native exists
}
if (u32 P = *(u32*)(C+0x94)) {
  u32 phmo = tag_data(*(u32*)(tag_data(*(u32*)(tag_data(*(u32*)datum(*(u32*)(C+8)))+0x38))+0x24));
  for (s32 j = *(s32*)(P+4) - 1; j >= 0; --j) {                    // counter kept at frame+0x14
    u32 r = *(u32*)P + j*8;
    u32 def = *(u32*)(phmo+0x2C) + (s32)*(s16*)(*(u32*)(phmo+0x44) + (s32)*(s8*)r*12 + 8) * 0x68;
    u32 h = *(u32*)(r+4);
    if (h == -1 || (h&0xFFFF) >= objects->capacity(+0x38) || hdr salt == 0 ||
        (s32)salt != (s32)h>>16 || hdr->datum == 0) continue;      // (1<<type) test is never 0
    object_wake_BBBC0(/*stack*/h, /*dl*/0);
    if (!(*(u32*)def & 0x01000000)) continue;
    if (object_type(h) != 0) continue;                             // bipeds only
    u32 b = datum(h);
    if (*(u8*)(b+0x3DC) == 1 && *(u32*)(b+0x3E4) == *(u32*)(C+8))
      *(u32*)(b+0x3E4) = 0xFFFFFFFF;                               // site 0x1D970F (object memory)
  }
  *(u32*)(*(u32*)(C+0x94)+4) = 0;                                  // site 0x1D972E
}
if (*(u8*)(C+4) & 1) {
  while (*(u32*)(C+0x20) != -1 && *(s16*)(rowof_52D690(*(u32*)(C+0x20)) + 2) != 0)
    model_constraint_remove_22D0A0(/*stack*/ *(u32*)(rowof_52D690(...) + 4));   // must advance C+0x20
  if (*(s32*)(C+0x90) >= 0)
    VCALL(*(u32*)0x48C938, 0x14)(*(u32*)(C+0x88), (*(u32*)(C+0x90)&0x7FFFFFFF)*72, 0x12);   // free
  *(u32*)(C+0x88)=0; *(u32*)(C+0x8C)=0; *(u32*)(C+0x90)=0x80000000;   // sites 1D9799/1D97A3/1D97AD
}
if (*(u32*)(C+0x98)) hkWorld_removePhantom_318680(/*ecx*/*(u32*)0x52BE48, *(u32*)(C+0x98));
if (*(u32*)(C+0x9C)) hkWorld_removePhantom_318680(*(u32*)0x52BE48, *(u32*)(C+0x9C));
*(u32*)(C+4) &= ~0x20;                                             // site 0x1D97E3
```

### 3.17 1D97F0 — component_insert_into_world (WR 4749-4969, 218 instr; canon 1D4F30)

`ecx` = C. `ret` (0). Aligned ebp frame. Stack locals: +0x0C i, +0x10 i*0x60,
+0x14 temp, +0x18 object datum pointer, +0x1C.. scale scratch.

```c
if (C+0x9C) hkWorld_addPhantom_318550(world, C+0x9C);
if (C+0x98) hkWorld_addPhantom_318550(world, C+0x98);
obj = datum(C+8);
for (i = 0; i < count(C+0x74); ++i) {                          // count reread
  rb = slot(i)+0x40;
  entity_add_listener_315E50(/*ecx*/rb, *(u32*)0x52D880);       // append to +0x70 (grows via 2E3250)
  if ((C+4)&1 && !rb[0x40] && VCALL(motion,0x18)() != 6) {
    entity_add_activation_listener_315E90(rb, L ? L+4 : 0);     // append to +0x60
    *(u16*)(rb+0x5C) = 0;                                       // TU1.5 site 0x1D98AB
  }
  world_add_rigid_body_1CC560(/*stack*/rb, *(u8*)(obj+0xC1) & 1);    // §3.18 (ECX zeroed, then MOV CL / AND 0xFFFFFF01)
  body_set_linear_velocity_1D9080(/*eax*/obj+0x88, /*ecx*/i, /*stack*/C);
  body_set_angular_velocity_1D9100(/*ecx*/obj+0x94, /*eax*/i, /*stack*/C);  // skipped when (C+4)&2
  u32 rec = VCALL(*(u32*)0x48C938, 0x10)(0x70, 0x26);           // allocate 0x70 bytes
  *(u16*)(rec+4) = 0x70;
  rec = build_body_record_1D9AD0(/*eax*/rb, /*esi*/rec, /*stack*/C+8);
        // ctor 31E820; rec+0x60 = object; rec+0 = vtable 0x461970; rec+0x64 = rb property 0x2002 or 0
  if ((C+4) & 0x800) { model scale from the tag chain; if either scale != 1.0:
        31E430 / 31E560 / 31E590 build scaled wrappers (stack scratch) }   // OPEN details
  if (rec) rec[+6]++;
  old = *(u32*)(rb+0x54);
  if (old) { if (--old[+6] == 0) { if (283450(old)) old[+6]=1; else VCALL(old,0)(1); } }
  *(u32*)(rb+0x54) = rec;
  if (--rec[+6] == 0) { if (283450(rec)) rec[+6]=1; else VCALL(rec,0)(1); }
}
for (k = 0; k < count(C+0x80); ++k) hkWorld_addConstraint_317350(world, constraint(k));
*(u32*)(C+4) = (*(u32*)(C+4) & ~0x8) | 0x20;                    // TU1.5 site 0x1D9ABA
```

### 3.18 Havok world entry points reached (summaries; full specs OPEN)

* **1CC560** (WR 2605, 42 instr; canon 1C7320) `world_add_rigid_body(stack rb, flag)`,
  callee pops 8 bytes. If `*(s32*)0x48B868 >= 0x352`, it first calls
  1CDD60 (canon 1C8AF0) with `edi` = rb's property 0x2001 (component handle);
  that is the over-limit handler, OPEN. Then `*(s32*)0x48B868 += 1` and
  `319370(ecx=world, stack rb)` (hkWorld addEntity, WR 25875, 35 instr, plus
  318020 / 316950 / 321510 …). If `flag && island(rb)+0x40 == 1`, it calls
  315CE0 (canon 3121F0, OPEN). Finally 282C10.
* **3198A0** (WR 25960, 67 instr; canon 315D50) hkWorld removeEntity
  (`ecx=world, stack(result*, rb)`).
* **282C10** (WR 10281, 276 instr; canon 27F190) processes dirty islands
  (guard bytes canon `0x5660B0`/`0x486563`). It can remove and destroy
  components and delete objects (1D9510, 1D7CA0, 16E8A0, BA570, BA8C0, 1CC310).
  Full native: `native_tu15_process_dirty_island_objects`.
* **2EC4B0 / 2EC9B0**: broadphase vtable slots +4 / +0xC at XBE `0x41C6C0`.

### 3.19 Owners used only by the 0x400 path (summaries)

* **1D8470** component reset (LS 122064, 190 instr). It releases every
  constraint and body reference, removes properties 0x2001/0x2002, frees the
  slot arrays, and zeroes `+4`, `+0x1C`, `+0x74`, `+0x80`. It sets
  `+0x18`/`+0x19` = 0xFF. Full native: `native_tu15_reset_physics_component`
  (src/physics/native_tu15_component_reset.cpp).
* **BF050** object node rebuild (LS 13625, 35 instr; 246-routine closure).
  Recursive over children. BF0C0 reads Havok bodies through 1DBBD0 → 1D8B90
  when `object+0xB4 != -1 && (object+0xC0 & 0x40)`.
* **1D73D0** component rebuild (LS 121495, 69 instr). Signed byte `obj+0xAA`
  selects through remap `0x1D74BC` into table `0x1D74A4`:
  type 0 → 1DEE30 (unit); 1 → 1DD9A0; 7 → `flags|=0x80` then
  1DDBF0(row,0,1,0) with the result stored to row+0x1C; 11 → 1DD950 (ordinary
  grouping); 12 → 1DEF50 (creature). Types 2,3,4,5,6,8,9,10 → table word 5 =
  **0** (`required(…,0,true)`, no owner). Then 1CD6A0 (object velocity →
  bodies), then `flags |= 1`, and then `|= 0x20000` or `&= ~0x20000` from the
  header active bit sampled before the builder. The TU1.1 counterpart 1D2B10
  is replicated in `physics_component_rebuild.cpp`.

---

## 4. Answers to the key questions

### 4.1 Store classification (question 1)

**Havok memory (physics pool; owner-based classification, address ranges not
checked: OPEN):**
* the motion `+0x6C`, `+0x80..+0xBF`, `+0x30..+0x3F`, `+0x70..+0x7F` (323B20,
  2E4350, 2E4690);
* everything under 315CB0/3165D0/316D40, hkWorld add/remove, broadphase, and
  constraint/phantom add/remove;
* rb `+0x54` (record swap), rb `+0x5C`, and the rb listener arrays
  `+0x60/+0x64` and `+0x70..+0x78`;
* refcounts `+6` on records, the 0x70-byte records, and allocator state behind
  `0x48C938`;
* 1D9080/1D9100 velocity stores into the motion.

**Stores NOT into Havok memory**, by path:

1. *Common path* (0x200 or multi-body, non-fixed body, island already active).
   Only stack stores plus one component-row store:
   * component row `+0x04 |= 0x10000` (1DC270 site 0x1DC5F8; multi-body path only);
   * stack: 1DC270 frame (F+0x08 length scratch; transform34 scratch at F+0x0C,
     F+0x40, F+0x74, F+0xA8, F+0xDC, F+0x110 written by its callees);
     1DC610 local (entry-0x34); 1DC680 writes the E32C0 float into **its own
     argument slot** (entry+4); 1D8B90 aligned scratch (64 bytes) and out34;
     1D8ED0's 0x60-byte aligned hkTransform build; 1D8FA0's T copy, I, and
     active byte; 1E1130's active byte; 315C90's out byte; 144090/145560/BC190
     outputs (all on the caller stack in this tree); 2E4650's aligned copy;
     2E4350's else-branch index table; plus every `push`/return address.
2. *Island-activation path* (body island inactive). 315CB0 → 3165D0 deferred
   removal (may destroy bodies through virtual destructors) and 316D40 →
   322660 listeners → TU1.5 282870 (collision-agent re-creation). Whether these
   store into game memory is OPEN (listener virtuals).
3. *Fixed-body path and 0x400 path* (1D9510 + 1D97F0):
   * global `0x48B868` (s32): `-1` per body removed (0x1D95B3), `+1` per body
     added (1CC560, TU1.5 site 0x1CC5AA);
   * component row: `+0x04` (`&~0x20`, then `(&~8)|0x20`), `+0x88=0`,
     `+0x8C=0`, `+0x90=0x80000000` (only when `flags&1`), and
     `*(row+0x94)+4 = 0` (list count);
   * other object datums: biped `+0x3E4 = -1` (0x1D970F) for bipeds whose
     `+0x3DC==1` and `+0x3E4==owner`; BBBC0 object wake for every valid object
     in the `+0x94` list (stores OPEN);
   * `0x52D690` rows and the component `+0x20` chain via 22D0A0 (OPEN);
   * stack locals of 1D9510/1D97F0/1CC560/3198A0/319370;
   * 282C10 (twice per body cycle): dirty-island processing, which can delete
     objects (BA570), defer objects (BA8C0 → object `+0xC0 |= 0x40`), destroy
     and free components, and run 1496F0 collection;
   * 1CDD60 eviction when 850 bodies are already in the world (OPEN);
   * 0x400 path only: 1D8470 (component row fields), BF050 (object nodes,
     bounds, children: object memory), 1D73D0 (row flags, row+0x1C, object
     `+0x12C` bit0 in the 1DD950 branch, and builder stores).

### 4.2 Return value and flags (question 2)

* 1CD620 returns nothing usable. B9460 overwrites AL at B958F
  (`mov al,[esp+0x2C]`) and never reads EAX or EFLAGS from the call. B9460's
  own return byte is the OOB flag at `[esp+0x13]`, set before the call.
* 1CD620 writes **no object-datum field** on the common path.
* The component flag `0x10000` (multi-body path) changes what the **next**
  1DC270 call on that component does: orthonormalisation is on from then on.
* B9460's later **BF050 reads what 1CD620 wrote**. For objects with
  `object+0xC0 & 0x40` and a component, BF0C0 → 1DBBD0 → 1D8B90 reads
  `motion+0x80` (written by 323B20) to build node matrices. 1CD620 therefore
  has to run before BF050, as in the original.
* In the fixed-body and 0x400 paths, side effects reach other objects (biped
  `+0x3E4`, wakes, 282C10 deletions) and `0x48B868`. These are visible to
  later code.

### 4.3 Which objects take which path (question 3)

| object / config | component? | 1CD620 (presented root: pos_null=0, allow80=0) | 1DC270 path | Havok work |
|---|---|---|---|---|
| no component (`+0xB4 == -1`); by the 1D73D0 table this covers weapon, equipment, garbage, projectile, scenery, control, light fixture, sound scenery (types 2-6, 8-10), assuming every component is built through 1D73D0 (OPEN) | no | 11-instr early return | — | none |
| biped / creature, normal state (1DEFB0: flags 0x203) | yes | passes | 0x200 + E32C0 z offset | body 0: setTransform (323B20), + island activation if asleep |
| biped / creature, anchored (physical state 4/5, flag 0x100) | yes | **skipped** | — | none |
| machine (type 7, flag 0x80) | yes | **skipped** (allow80=0) | — | none |
| crate / vehicle, one body, no node (`0x200`) | yes | passes | 0x200 | as biped, no z offset |
| crate / vehicle, several bodies or node-bound | yes | passes | multi-body | per body: 1D8B90 + 145560 + setTransform |
| any body with `rb+0x40 != 0` (fixed motion) in an in-world component | yes | passes | 0x200 or multi | **remove whole component + set + re-insert** (once per fixed body) |
| fixed body, component not in world (`flags&0x20 == 0`) | yes | passes | 0x200 or multi | setTransform only (rb+8 == 0) |
| component constrained to the world (`0x400`) | yes | passes | full rebuild | remove, reset, BF050, rebuild, re-insert |
| keyframed motion (not fixed) | yes | passes | as above | setTransform (+ island activation); keyframed rigid bodies take the same 323B20 owner |

Which TU1.5 objects actually carry fixed bodies is a tag property (phmo
rigid-body motion type). It is not resolvable statically (OPEN). Profiling
attributes 13% of the Zanzibar 1p frame to this remove/re-add
(docs/NATIVE_REPLACEMENT_GOAL.md).

### 4.4 Order dependence across objects (question 4)

* **Common path:** each body's setTransform only touches its own motion. It
  reads only its own component, its own object, and (through BC190) its
  **parent's node matrices**. If a parent is rebuilt earlier in the same pass,
  the child's result depends on that order. Presented roots have no parent.
* **Island activation (315CB0)** changes world island lists and runs listeners.
  The activation order sets the active-island order, which later steps consume.
  A resting (deactivated) dynamic body is woken by every 1CD620 call, even with
  an unchanged position.
* **Fixed-body and 0x400 paths are strongly order-dependent:**
  * hkWorld entity and constraint arrays (remove then append);
  * broadphase endpoint and pair structures (2EC9B0 then 2EC4B0);
  * rb listener arrays (remove nulls the slot; add appends; compaction inside
    3198A0 is OPEN);
  * allocator state (one 0x70-byte record allocated per re-inserted body, the
    old one released);
  * the `0x48B868` body count and the 850-body eviction in 1CC560;
  * 282C10 dirty-island processing.
* Nothing in 1CD620 reads a cross-object pair list directly.

---

## 5. Per-path instruction counts (TU1.5 translated, static instructions executed)

Per routine (own body only):

| routine | path | instr |
|---|---|---|
| 1CD620 | no component | 11 |
| 1CD620 | gated by 0x2&pos_null | 26 |
| 1CD620 | gated by 0x100, or 0x80 with allow80=0 | 30 (33 with bit 0x2 set and pos_null=0; 0x80-only adds 3) |
| 1CD620 | calls 1DC270 | 32 (35 with bit 0x2 set) |
| 1DC270 | 0x400, was not in world / in world | 25 / 29 (+ callees) |
| 1DC270 | 0x200 non-biped / biped no offset / biped with offset | 35 / 40 / 43 |
| 1DC270 | multi-body, n ≤ 0 | 36 |
| 1DC270 | multi-body, n > 0 | 39 + n×19, + n×(100..133) when 0x10000 is set |
| 1DC610 | valid root / no root (fails at 1st/2nd/3rd test) | 24 / 15, 19, 22 |
| 1DC680 | 0x200 non-biped / biped AL=0 / biped AL=1 | 27 / 33 / 36 |
| 1DC680 | no root (fails at 1st/2nd/3rd test) / root, node 0xFF / root + node | 19, 23, 26 / 29 / 56 |
| E32C0 | v==0 / v!=0 | 10 / 19 |
| 1D8ED0 | — | 43 |
| 1D8FA0 | non-fixed, island active / inactive (+315CB0) | 45 / 50 (+8 with 0x40) |
| 1D8FA0 | fixed & in world (+1D9510, 1E1130, 1D97F0) | 44 (+8 with 0x40) |
| 1E1130 | after removal (rb+8==0): island active / no island / activation | 20 / 23 / 25 (+315CB0) |
| 323B20 | — | 28 |
| 2E4350 | trace > 0 / else | 31 / ≤73 |
| 2E4690 | — | 26 |
| 315C90 | no island / island | 6 / 7 |
| 315CB0 | — | 18 (+3165D0 57, +316D40 45, +322660 98 + listener virtuals) |
| 1D8B90 | no 0x40 / 0x40 | 51 / 55 + 2E4650 20 + 2E44D0 86 |
| 2E4470 | — | 33 + 2E4820 24 |
| 145560 | — | 69 |
| 144090 | scale 0 / other | 13 / ~70 |
| BC190 | no parent / parent | 24+53 / 36+53+69 |

Composite paths (presented root, one body, island active, no 0x40, trace > 0):

* **Single body (0x200), non-biped:** 1CD620 32 + 1DC270 35 + BC190 77 +
  1D8ED0 43 + 1D8FA0 45 + 315C90 7 + 323B20 28 + 2E4350 31 + 2E4690 26 ≈ **324**.
  A biped adds 15 (no offset) or 27 (offset).
* **Multi-body (n bodies, root valid, node-bound root, flag 0x10000 clear):**
  32 + (39 + 19n) + 1DC610 (24 + 51 + 56 + 69) + BC190 77 + 144090 ~70 +
  145560 69 + n × (1D8B90 51 + 145560 69 + 1D8ED0 43 + 1D8FA0 45 + 315C90 7 +
  323B20 28 + 2E4350 31 + 2E4690 26 = 300). That is ≈ **487 + 319n**, plus
  100-133 per body once 0x10000 is set.
* **Asleep island:** add ≥ 5 + 18 + 57 + 45 + 98 per body, plus listener virtuals (OPEN).
* **Fixed body in world:** add 1D9510 (static 218, per body 1 world remove + 2
  listener removes + 282C10) + 3198A0 (67 + broadphase vcall) + 1D97F0 (static
  218, per body 1CC560 42 + 319370 35+ + 282C10 + 1D9080 41 + 1D9100 47 +
  allocator + 1D9AD0 30 + refcount + 283450 37). Dynamic totals are OPEN. The
  profile shows this dominating (≈0.89M instructions/frame for the B95D0 chain
  on Zanzibar 1p, NATIVE_REPLACEMENT_GOAL.md).
* **0x400 path:** adds 1D8470 (190) + BF050 closure (up to 23,567 static) +
  1D73D0 closure (up to 5,658 static) + the 1D9510/1D97F0 cost.

---

## 6. Existing native replicas (grep of the address, lower/upper case, in src/ include/ docs/, excluding `*_generated*`)

| routine | status | where |
|---|---|---|
| 1CD620 | mention only | docs/NATIVE_REPLACEMENT_GOAL.md, NATIVE_4K60_SPLITSCREEN_GOAL.md, RENDER_THREAD_OFFLOAD_PLAN.md; call-site list in tools/physics_live_support_tu15_entries.json |
| 1DC270, 1DC610, 1DC680, E32C0, 1E1130, 2E4470 | none | — |
| 1D8ED0 / 1D8FA0 | no TU1.5 replica. **Full TU1.1 replicas** of the same bodies (canon 1D4610/1D46E0): `source_biped_body_transform34_1_1` / `source_biped_body_transform40_1_1` | src/gameplay/units/biped_body_placement.cpp:361-424 (TU1.1 sites; services for remove/insert/static transform/activate; rejects motion methods other than TU1.1 31FFC0) |
| 323B20 (+2E4350, 2E4690) | TU1.5: mention only (vtable mount in src/platform/native_tu15_world_memory.cpp). **Full TU1.1 replica** of 31FFC0 + 2E0860 + 2E0BA0: `source_biped_motion_transform_1_1` | biped_body_placement.cpp:243-359 (TU1.1 constants 0x418460/0x465048/0x465030/0x46502C) |
| 1D8B90 | value-level: `native_tu15_model_constraint_body_frame` (1D8B90/2E4650/2E44D0, no stack stores); full TU1.1 memory replica `source_physics_body_matrix_1_1` (canon 1D42D0); `native_tu15_component_body_motion_snapshot` uses it as a service | include/halo2/native_tu15_body_constraint_update.hpp:56; src/physics/physics_body_matrix.cpp |
| 145560 | **register-level kernel** `kernel_145560` (build-local specialised copy, HALO2_REFERENCE_MATRIX_LEAVES / HALO2_VERIFY_MATRIX_LEAVES); value-level `product()` | src/physics/runtime/native_tu15_matrix_leaves.hpp:110; src/physics/native_tu15_object_motion.cpp:471 (anonymous namespace) |
| 144090 | **full memory-level replica** `native_tu15_camera_inverse_frame`; value-level `inverse()` | include/halo2/native_tu15_camera_projection.hpp:38; native_tu15_object_motion.cpp:490 |
| BC190 (+144BF0) | value-level `native_tu15_object_transform` (exported), `basis()` | include/halo2/native_tu15_object_motion.hpp:41; native_tu15_object_motion.cpp:461,561 |
| 315C90 | value-level `island_active()`; `native_tu15_component_has_active_body` | native_tu15_object_motion.cpp:160; native_tu15_body_constraint_update.hpp |
| 315CB0 | **full native** `native_tu15_activate_physics_body` (+3165D0, 316D40) | include/halo2/native_tu15_island_activation.hpp:40 |
| 2E4650 / 2E44D0 / 2E4690 / 2E4820 | doc/header mentions (native geometry proofs); 2E44D0/2E4690 described in include/halo2/native_tu15_geometry.hpp:20-25 | — |
| 1D9510 | TU1.5: used as a service (dirty islands, contact pressure, component state). **Full TU1.1 replica** of canon 1D4C50 | include/halo2/biped_world_removal.hpp:46 |
| 1D97F0 | TU1.5: service in native_tu15_component_state. **Full TU1.1 replica** of canon 1D4F30 | include/halo2/biped_world_insertion.hpp:41 |
| 1D8470 | **full native** `native_tu15_reset_physics_component` | src/physics/native_tu15_component_reset.cpp |
| 1D73D0 | TU1.5 service; **full TU1.1 replica** of 1D2B10 | src/physics/physics_component_rebuild.cpp |
| BF050 | mention / called from native code | src/campaign/source_campaign_object_scale_tu15.cpp:23 |
| 282C10 | **full native** `native_tu15_process_dirty_island_objects` | src/physics/native_tu15_dirty_island_objects.cpp |
| 1CC560, 319370, 315E50, 1D9AD0 | mention (entry tables / docs/MP_POWER_WORLD_MEMBERSHIP_RESEARCH.md) | — |
| 3198A0 | used as a service | src/physics/native_tu15_world_deferred.cpp:109 |
| 2EC4B0 / 2EC9B0 | vtable mount in native_tu15_world_memory.cpp; docs | — |
| 1D9080 / 1D9100 | value-level `deliver()` (1CD6A0 path) | native_tu15_object_motion.cpp:312 |

Known TU1.1 → TU1.5 offset in this region: `+0x48C0` (1D4610→1D8ED0,
1D46E0→1D8FA0, 1DC870→1E1130, 1D4C50→1D9510, 1D4F30→1D97F0, 1D42D0→1D8B90).
TU1.1 31FFC0 corresponds to TU1.5 **323B20**. Note that TU1.5 0x31FFC0 is an
unrelated pair callback (WR 28494).

---

## 7. OPEN items

1. The exact virtual chain from 319370/3198A0 to broadphase 2EC4B0/2EC9B0. Also
   the hkWorld add/remove internals and whether rb listener arrays are compacted
   (array growth per re-insert).
2. The 1CDD60 (850-body limit) behaviour and 315CE0 (canon 3121F0).
3. The stores done by BBBC0 (object wake), 22D0A0 (model constraint removal),
   31E820/31E430/31E560/31E590 (record constructor and scaled wrappers), and
   the 0x800 tag-scale fields read by 1D97F0.
4. Island-activation listener effects (322660 → 282870 and others).
5. Who sets and clears component flag 0x10000 outside 1DC270 (0xB9DFC, 0xBE680),
   and whether every component is built through 1D73D0. That decides whether
   scenery and items can ever own a component.
6. Which tags give fixed (`rb+0x40`) bodies, and the runtime path mix per map.
7. The heap address ranges used to class "Havok pool" vs "game state". The
   classification above is by owner.
8. Dynamic instruction counts for the fixed-body and 0x400 paths.
9. Whether `rb+8` is always 0 after 1D9510 before 1E1130 (it is expected).

## 8. Implementation notes and parity hazards

* **Float order.** Reproduce the SSE association orders above exactly. x87
  sections are: the 1DC270 lengths and `left`, 323B20's +1e6, and 2E4350. These
  need the source control word and extended intermediates. Note the
  extended×float product in the `left.z` length. Each `fst`/`fstp` rounds to
  float32. Existing helpers `x87_sqrt/x87_store` (native_tu15_object_motion.cpp)
  and the asm blocks in biped_body_placement.cpp show the house pattern.
* **MXCSR sticky flags.** 145560 computes lane 1 on zeros. 2E4690 and 2E44D0
  compute the w lane. A native that keeps the same packed operations reproduces
  the sticky status for free.
* **Aliasing.** 145560 is called with out==A (1DC680) and out==B (BC190).
  144090 is not alias-safe; it is never called aliased here.
* **Stack stores.** The original writes scratch into its frames, and 1DC680
  writes into its argument slot. Prior TU1.1 replicas reproduce these at
  derived frame addresses. A TU1.5 replacement either does the same or
  documents that the parity gate ignores the stack region.
* **Store order within the motion** is: +0x6C, +0x80..0xBF, +0x30..0x3C,
  +0x70..0x7F (one 16-byte store), then +0x7C = 0.
* **The cheap win.** For presented roots, 1CD620 → 1DC270 → 1D8ED0 → 1D8FA0 →
  323B20 is only ~324 instructions of pure math on Havok memory. The cost is in
  the fixed-body remove/re-insert (§4.3, row "fixed motion") and the 0x400
  rebuild. A native replica must keep those owners in the same order (§4.4) for
  parity.
