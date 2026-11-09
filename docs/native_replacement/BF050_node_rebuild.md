# BF050: object node-matrix rebuild after a move (TU1.5). Native replacement spec

Status: read-only research spec, 2026-11-08. All addresses are TU1.5 (u5) image
addresses. The source is the translated unit
`src/physics/physics_live_support_tu15_generated.cpp`. Each routine is
`static void run_<addr>(WorldState &s)`. Line numbers below refer to that file.
Nothing here was built or run. Everything comes from reading the translated
instructions. Items that could not be pinned down are marked **OPEN**.

## 0. Notation

* `NONE` = `0xFFFFFFFF`. `sx8/sx16` = sign-extend. `rd8/rd16/rd32/wr32` = emulated memory access.
* `obj_body(h)` = `rd32(rd32(rd32(0x4ECBD0)+0x44) + (h&0xFFFF)*12 + 8)`. No routine in this
  tree checks the salt (the high 16 bits).
* `tag_addr(t)` = `rd32(rd32(0x4F0414) + (t&0xFFFF)*16 + 8)` gives a tag instance's address.
* A `real_matrix4x3` (0x34 bytes) is laid out as: `+00 scale`, `+04 forward xyz`,
  `+10 left xyz`, `+1C up xyz`, `+28 position xyz`.
* A `real_orientation` (0x20 bytes) is laid out as: `+00 quaternion i,j,k,w`,
  `+10 translation xyz`, `+1C scale`.
* "ordered-equal(a,b)" is the `UCOMISS; LAHF; TEST AH,44h; JNP` idiom. It is true only when
  `a==b` and neither operand is NaN. NaN therefore always counts as "not equal".
* `a > b` written for a `UCOMISS a,b; JA` test is false for NaN.
* FP is SSE single precision unless noted. Operand order is written as in the original.
  Multiplication and addition are commutative bit-for-bit in IEEE, so only the grouping and
  the subtraction order matter. Do not compile with FP contraction (FMA).
* `L_xx` = the routine's own stack local `[esp+xx]` after its prologue pushes.
* "Instr" = the number of lines inside the translated body that contain `s.services.instruction(`.
* x86 `SHL r,CL` masks the count to 5 bits. The type tests therefore operate on `type & 31`.

## 1. Findings (answers to the questions asked)

### 1.1 Ordering inside B9460 (checked)

`B9460` (line 7985) calls the following, in this order. Every call is conditional:
* `BA7E0` at `B949C`. Called only if body flags bit 8 (connected) is set and the stack arg byte
  `[esp+38h]` is 0.
* `B7E50` at `B951A` and at `B956A`.
* `1CD620` at `B958A`.
* `BF050` at `B959C`. Called only if the local byte `[esp+2Ch]` is nonzero.
* `BA630` at `B95BB`. Called only if the object was originally connected and the arg byte is 0.

The world-bounds position store was **not** re-verified here; it belongs to the B9460 spec.
Corrections to the profiled picture:

* **BFB90 is not a direct callee of BF050.** The chain is BF050 → BF0C0 → BFB90.
* BF050 calls **BFF20** directly after BF0C0, then recurses into the children.
* **BFF20 can itself call BA7E0/BA630** when the object is still connected (body+4 bit 8).
  * For the root inside B9460, BA7E0 has normally already cleared bit 8. In that case BFF20
    only rewrites the bounding spheres, and B9460 reconnects afterwards.
  * Attached children are never disconnected by B9460. For a connected child, BFF20 runs
    BA7E0 and BA630 itself when its sphere moved by more than 0.1 units (distance² > 0.01) or
    its radius changed.

### 1.2 What BF0C0 computes

BF0C0 does **not** just re-root the existing node matrices. It rebuilds every node matrix
from the 32-byte local orientations. Unless one of the exclusions below applies, it first
re-evaluates those orientations, as follows.

1. **Local orientation bank.** The bank is `body + sx16(body+0x112)` if `rd16(body+0x112) != 0xFFFF`.
   Otherwise it is the static scratch `0x4E8BC0` (255 × 32 bytes). In that case the flag
   "has own bank" (L_12) is 0.
2. **Bind-pose reset and animation.** This step is skipped when body+4 **bit 29** is set
   ("local orientations already valid"; compare docs/MP_ITEM_TRANSFORM_PRODUCERS_RESEARCH.md
   for TU1.1). It is also skipped when the object is a biped (definition byte `&31 == 0`) with
   `byte body+0x34B != 0`. Otherwise:
   * `1707A0` copies the render model's default node pose into the bank.
   * If the object has an animation manager (`sx16 body+0x12A != -1`), is not frozen
     (`byte body+0xB3 == 0`), and `mgr+0x68 != NONE`, then `BFB90` re-samples the animation
     graph at the manager's **current** state. BFB90 runs:
     * the base or blend channels: `1D5840` → `1CF210`, with the blend factor computed by 1D61B0;
     * the jmad "object function" overlays: `BF9A0`;
     * the per-type orientation hooks: `10E100` → unit `114750`, vehicle `F34B0`, device `10C0B0`;
     * the cross-fade toward the cached previous orientations at `body+sx16(+0x10E)`: `1E1720`.

     No store into the manager's channels was found in BFB90, 1D5840 or 1CF210. BF9A0
     samples through a stack temporary channel. Animation time is therefore **not advanced**
     (sampling only).
   * BFB90 may restrict sampling to the non-render-only nodes. The mask is
     `~hlmt.render_only_node_flags` (hlmt+0xA4), kept in the static `0x56EB8C`. It is used
     only when the object has its own bank (L_12 = 1). Body+4 bit 30 records whether this mask
     was applied.
3. **Physics-driven nodes.** This step runs when the physics component is active
   (`body+0xB4 != NONE` and `byte body+0xC0 & 0x40`). `1DBBD0` writes those node matrices
   from the Havok bodies and sets their bits in a 256-bit node mask (L_70). Those nodes are
   then skipped by the animation composition below.
4. **Root (node 0)**, unless mask bit 0 is set:
   * World-relative animation (animation flags byte +0x16 bit 1): `node0 = 144CF0(orient0)`.
   * Otherwise: `node0 = BFC70(pos, fwd, up, scale, mirror bit10, parent node matrix) ∘ 144CF0(orient0)`.
     Network prediction can override the position and basis first (B3AE0/B3B70).
   * Then `10E180` runs the per-type matrix hooks: unit `1154A0`, scenery `10F440`. Both
     pre-multiply node 0 by another object's node 0.
5. **Nodes i = 1 .. mode.node_count-1** in model order, skipping masked nodes:
   `node[i] = node[parent(i)] ∘ 144CF0(orient[i])`. `parent(i) = sx16(mode.nodes[i]+4)`.
6. Special cases:
   * Biped with `byte body+0x34B == 2`: BF0C0 only clears body+4 bit 30 and returns. No
     node writes.
   * No `hlmt` or no render model: only node 0 is written, by BFC70 directly from body
     +64/+70/+7C/+A0 and the parent node.

What each type gets (dispatch tables decoded from `native_tu15_object_storage_data.cpp`;
descriptor slot `+6C` is the orientation hook, `+70` is the matrix hook):

| type (`&31`) | +6C hook (10E100) | +70 hook (10E180) | other type-specific behaviour |
|---|---|---|---|
| 0 biped | unit 114750 | unit 1154A0 | state byte +34B (2 = no-op, ≠0 = keep orientations); 1DBBD0 z offset E3BC0 |
| 1 vehicle | unit 114750, vehicle F34B0 | unit 1154A0 | — |
| 2 weapon, 3 equipment, 4 garbage, 5 projectile | — | — | — |
| 6 scenery | — | scenery 10F440 | — |
| 7 machine, 8 control, 9 light fixture | device 10C0B0 (foreign, via `required()`) | — | BF050 does **not** recurse into machine (7) children |
| 10 sound scenery, 12 creature | — | — | — |
| 11 crate | — | — | BFF20 uses physics AABB sphere (1DC7A0) when its physics component is active |

### 1.3 Memory written

The full store lists are in §4.

* Node matrices at `body + sx16(+0x116)`, stride 0x34. The size word `+0x114` gives
  `count = size/52`. The loops actually use `mode+0x48` as their bound, and the havok path
  uses the phmo node indices.
* The local orientation bank at `body + sx16(+0x112)`, stride 0x20, size word `+0x110`, or
  the static `0x4E8BC0`.
* `body+4`: bit 30 is cleared, then set or cleared by C1E50 when the animation path runs.
* Statics:
  * `0x56EBAC` (lazy flag dword);
  * `0x56EB8C..0x56EBAB` (render-only node mask);
  * `0x56E684` (flag) and `0x56E680` (epsilon), written lazily by 144CF0;
  * `0x56E060..0x56E07F` (combined node mask), written by 1CF210 whenever both a caller mask
    and an animation mask exist.
* BFF20 writes:
  * `body+0x30..0x3C` (bounding sphere);
  * `body+0x50..0x5C` (alternate sphere);
  * `body+0x40..0x4C` (registration sphere), unless it returns 0 early;
  * cluster membership via BA7E0/BA630 when the object is connected.
* Animation side effects:
  * resource requests through `1E5C80 → 126200`, reached from 1E3150, BF9A0 and 1CF210;
  * decode scratch written by the 1CF210 callees (283A90/2835B0/284090). Per the
    `native_tu15_world_memory.cpp` mount comments these include `0x56E000`, `0x50EF10`,
    `0x510F10`, `0x510F30` and `0x56E688`. Not traced here: **OPEN**.
* Physics delivery (1DBBD0 → 1D8B90 / Havok virtuals / 1DBFD0) writes node matrices inside
  the body. Whether it writes anything else is **OPEN**.
* Large amounts of stack scratch. Pointers to stack locals are passed to callees (L_70 mask,
  pos/fwd/up locals, L_90, L_C4). So while those callees stay translated, the locals must
  live in emulated stack memory.

## 2. Data used by the tree

**Object body** (fields read or written in this tree):

| off | type | meaning (behavioural) | used by |
|---|---|---|---|
| +00 | u32 | definition tag index | BF0C0, BFF20, 1DBBD0 |
| +04 | u32 | flags: bit8 connected-to-map; bit10 mirrored (left negated); bit29 orientations valid (skip re-animation); bit30 render-only mask applied | BF0C0, BFF20, BFC70 args, BA7E0 (clears bit8) |
| +0C | u32 | next sibling | BF050 |
| +10 | u32 | first child | BF050 |
| +14 | u32 | parent | BF0C0 |
| +18 | s8 | parent node index | BF0C0 |
| +30/+3C | vec3/f32 | bounding sphere | BFF20 (written), 1DC7A0 (written via args) |
| +40/+4C | vec3/f32 | registration (cluster) sphere | BFF20 |
| +50/+5C | vec3/f32 | alternate sphere | BFF20 |
| +64/+70/+7C | vec3 | position / forward / up | BF0C0, BFC70 |
| +A0 | f32 | scale | BF0C0, BFF20 |
| +AA | s8 | object type | BF050, BFF20, 10E100, 10E180 |
| +B3 | u8 | nonzero = animation frozen (C1CC0) | BF0C0 |
| +B4 | u32 | physics component index (pool 0x52BE5C, 0xA0 bytes each) | BF0C0, BFF20 |
| +C0 | u8 | bit6 physics component active | BF0C0, BFF20 |
| +D4 / +D8 | u32 / u8 bit0 | simulation (network) entity / prediction enabled | BF0C0 → B3AE0/B3B70 |
| +10E | s16 | offset of cached previous orientations | BFB90 → 1E1720 |
| +110 / +112 | s16 | local orientation bank size (bytes) / offset (−1 none) | BF0C0, 1DBBD0 |
| +114 / +116 | s16 | node matrix bank size (bytes) / offset | everywhere |
| +12A | s16 | animation manager offset (−1 none) | BF0C0, C15D0 |
| biped +34B | u8 | physics/ragdoll state (1, 2) | BF0C0, E3BC0 |
| biped +3DC, +3EC | u8, struct | E3BC0 (state 3; 1F4920 reads `+3EC+5C` timestamp) | E3BC0 |
| unit +33E | s16 | offset of block whose +28 is an object handle | 1154A0 |
| scenery +138 | u32 | object handle | 10F440 |

**Object definition (obje)**:
* `+00` u8: type (biped test).
* `+02` u16: flags. Bit `0x1000` selects the alternate sphere for registration.
* `+04`: bounding radius.
* `+08..+10`: bounding offset.
* `+20`: alternate radius.
* `+24..+2C`: alternate offset.
* `+38`: hlmt tag index.

**hlmt**:
* `+04`: render model (mode) tag index.
* `+24`: phmo tag index (1DBBD0).
* `+A4`: 32-byte render-only node flags.

**mode**:
* `+48`: node count.
* `+4C`: node block. Stride 0x60: `+04` s16 parent, `+06` first child, `+08` next sibling,
  `+0C` default translation, `+18` default rotation quaternion.

**Animation manager** (`body + sx16(+0x12A)`):
* `+00` jmad tag index.
* `+04` packed animation id: low16 = inheritance index (0xFFFF = own graph), high16 = animation index.
* Channels at `+00`, `+20` and `+40`, 0x20 bytes each. The animation index word is at
  `+06/+26/+46`. Byte `+10` is a resource gate in 1CF210.
* `+64`: interpolation state. Byte `+65` = active; byte `+67` bit1 = suppress.
* `+68`: tag index of the graph used for the overlays and for the "valid" checks.
* `+80`: blend fraction.

**jmad**:
* `+30`: animations block. Stride 0x6C: `+14` s16 frame count; `+16` u8 flags (bit1
  world-relative, bit4 resource-backed); `+24` resource row; `+30/+32` s16 chain links.
* `+44/+48`: overlay block, count and pointer. Stride 0x14: `+04` packed animation,
  `+0A` u16 function controls (0 = frame, 1 = scale), `+0C` function name.
* `+4C/+50`: inheritance list. Stride 0x20; `+04` is the tag index.
* `+B0`: resource rows. Stride 0x14; byte `+10` is checked.

**Globals**:

| address | meaning |
|---|---|
| `0x4ECBD0` | object array header pointer |
| `0x4F0414` | tag instance table |
| `0x52BE5C` | physics component pool |
| `0x474B90` | 13 object type descriptors (`+6C`, `+70` hooks; `+84` NULL-terminated inheritance list) |
| `0x4E8BC0` | static orientation scratch |
| `0x474D38` → `0x44AC04` | identity orientation `{0,0,0,1, 0,0,0, 1}` |
| `0x56EBAC` | lazy flag dword |
| `0x56EB8C` | 32-byte node mask |
| `0x56E680` / `0x56E684` | 144CF0 epsilon and lazy flag |
| `0x51DF54` | game-time globals (`+04` f32, `+08` tick) |
| `0x4DC04C` / `0x4DC054` | simulation globals (B3A50) |
| `0x47C1B4` / `0x47C1B5` | bytes, authored = 1 |
| `0x51DF20` / `0x51DF21` | resource streaming gates |

**Constants**:

| address | value |
|---|---|
| `0x46A0D0` | 1.0f |
| `0x46A0E8` | 0.0f |
| `0x46A0EC` | 1e-4f (0x38D1B717) |
| `0x46A560` | 0.01f (0x3C23D70B) |
| `0x45F490` | {0x80000000, 0, 0, 0x80000000} |
| `0x46A188` | 0x3F7FF972 (≈0.99990) |
| `0x46A184` | 0x3F80068E (≈1.0002) |
| `0x46A410` | −0.125f |
| `0x46A314` | 6.6666665f (0x40D55555) |
| `0x46A0C8` | 4294967296.0f (0x4F800000) |
| `0x46A17C` | 1/30 (0x3D088889) |

## 3. Call tree and sizes

Main tree:

| routine | lines | Instr | role | callees |
|---|---|---|---|---|
| **BF050** | 13625–13662 | 35 | recursive driver | BF0C0, BFF20, BF050 |
| **BF0C0** | 13664–14060 | 394 | node matrices for one object | E3BC0, 1DBBD0, BAC00, 1707A0, C15D0, C1CC0, C1F40, C1EB0, C1F00, BFB90, C1E50, B87B0, B8780, B3AE0, B3B70, BFC70, 144CF0 (×3 sites), 145560 (×2 sites), 10E180 |
| **BFF20** | 14448–14652 | 202 | spheres + registration refresh | 1DC7A0, BA7E0, BA630 |
| **BFB90** | 14209–14293 | 82 | local orientation evaluation | 141010, 1E3150, 1D5840, BF9A0, 10E100, 1E1720 |
| BFC70 | 14295–14446 | 149 | root transform | 145560 ×2–3 |
| 144CF0 | 78844–78931 | 85 | orientation → matrix | — |
| 145560 | 79263–79334 | 69 | matrix product | — |
| 1707A0 | 96075–96123 | 46 | bind pose copy | — |
| 10E180 | 62350–62387 | 35 | type `+70` hook loop | 1154A0 / 10F440 (dispatch) |
| 10E100 | 62307–62348 | 39 | type `+6C` hook loop | 114750 / F34B0 / 10C0B0 (dispatch) |

Small leaves:

| routine | lines | Instr | role | callees |
|---|---|---|---|---|
| BAC00 | 9634–9647 | 11 | node matrix address | — |
| C15D0 | 15894–15906 | 10 | has animation manager | — |
| C1CC0 | 16216–16231 | 13 | frozen test | — |
| C1E50 | 16338–16352 | 12 | set/clear bit | — |
| C1EB0 | 16384–16414 | 28 | invert 256-bit mask | — |
| C1F00 | 16416–16443 | 25 | clamp mask to count | — |
| C1F40 | 16445–16479 | 32 | mask all-zero test | — |
| B87B0 | 7229–7243 | 12 | manager valid | — |
| B8780 | 7213–7227 | 12 | manager → animation definition | 1E3150 |
| E3BC0 | 36812–36828 | 14 | biped physics z offset | 1F4920 (23) |
| 141010 | 78345–78399 | 52 | highest set bit | — |
| B3AE0 | 6832–6880 | 46 | predicted position | B3A50 (43) → vtable+88 |
| B3B70 | 6882–6936 | 52 | predicted basis | B3A50 |

Animation subtree:

| routine | lines | Instr | role | callees |
|---|---|---|---|---|
| 1D5840 | 120671–120770 | 97 | channel sampling and blend | 1CF210 ×4, 1D61B0 (19), 1D63C0 (foreign) |
| BF9A0 | 14062–14207 | 143 | function overlays | 1E5C80, 1D36C0 (49), 1E3150, BCB70 (138), 1CE810 (50), 1CF210, CRT 37DC0A (foreign) |
| 1E1720 | 128941–129078 | 135 | cross-fade | 1E1620 (49) |
| 1E3150 | 130547–130601 | 52 | animation lookup | 1E5C80 (76) → 126200, 1E3270, 1E5AF0 |
| 1CF210 | 115614–115753 | 137 | sample channel | 1E3150, 1E5C80, 283A90, 284090, dispatch 1E32A0 |

Physics, bounds and registration:

| routine | lines | Instr | role | callees |
|---|---|---|---|---|
| 1DBBD0 | 123656–123939 | 281 | physics node delivery | 1D8B90 (foreign), Havok virtuals, 145560, 1DBFD0 (186, recursive) |
| 1DC7A0 | 124511–124745 | 232 | crate physics AABB sphere | Havok virtual |
| BA7E0 | 9347–9386 | 37 | disconnect | C0F60, 1D30F0 |
| BA630 | 9205–9345 | 138 | reconnect | 11E600, 1CC290, 1D2F10, B1870, B8D90, B92C0, B9330, BA570, BA910, C0F60, 1496F0/282C10 (foreign) |

Type hooks:

| routine | lines | Instr | callees |
|---|---|---|---|
| 1154A0 | 67709–67740 | 29 | BCDF0 (foreign), 145560 |
| 10F440 | 63644–63673 | 27 | BCDF0, 145560 |
| F34B0 | 49984–50001 | 15 | F2CC0 (522) |
| 114750 | 66856–67707 | 849 | 1CF210 ×8 and others |

"Foreign" means the target has no `run_` body in this unit. `dispatch()` falls through to
`s.required(site,target,tail)`. BCDF0 and 10C0B0 live in other generated units, for example
`source_scene_device_constructor_tu15_generated.cpp`.

Callers of BF050 in this unit:
* `B9757` (the object-scale path; see native_tu15_object_scale);
* `B959C` (B9460), `BA295`, `BB808`;
* `ECB2B`, `ED006`, `EE80E`, `F0BCA`, `10F8B6`;
* `1DC29F`.

BF0C0 is also called directly at `BE892`, `C0F0D`, and at `B919F` in the unit object runtime.
BFF20 is also called at `BE941`.

## 4. Routine specifications

### 4.1 BF050: `void __stdcall BF050(u32 h)`, RET 4

Preserves ESI and EDI. EAX on return is whatever the last callee or shift left; there is no
defined result, and B9460 ignores it. Globals read: `0x4ECBD0`.

```c
void BF050(u32 h) {
  u32 body = obj_body(h);            // EDI, read BEFORE the calls
  BF0C0(h);                          // push h; call
  BFF20(h);                          // push h; call
  u32 c = rd32(body + 0x10);         // first child, read AFTER both calls
  while (c != NONE) {
    u32 cb = obj_body(c);
    // MOV EAX,1; SHL EAX,CL(type); TEST AL,AL; JS skip  => skip iff (type & 31) == 7 (machine)
    if (((1u << (rd8(cb + 0xAA) & 31)) & 0x80) == 0)
      BF050(c);
    c = rd32(cb + 0x0C);             // next sibling read from the child body after recursion
  }
}
```
Memory stores: stack only (saved regs, args, return addresses).

### 4.2 BF0C0: `void __stdcall BF0C0(u32 h)`, RET 4

The prologue is `SUB ESP,0E8h; PUSH EBX,EBP,ESI,EDI`. The argument is at `L_FC`. EAX on
return is undefined.

Globals read:
* `0x4ECBD0`, `0x4F0414`, `0x52BE5C`;
* `0x56EBAC`, `0x474D38`;
* the definition, hlmt, mode and manager fields listed in §2.

Frame locals:

| local | contents |
|---|---|
| `L_11` | u8, parent mirrored |
| `L_12` | u8, has own orientation bank |
| `L_13` | u8, world-relative animation |
| `L_14` | f32 physics z offset, later reused for the mode pointer |
| `L_18` | orientation bank pointer |
| `L_1C` | node matrix bank pointer |
| `L_20` | definition pointer |
| `L_24` | count = size(+114)/52 |
| `L_28` | parent node matrix pointer or 0 |
| `L_2C..L_34` | temp vec3 |
| `L_38..L_40` | up |
| `L_44..L_4C` | position |
| `L_50..L_58` | forward |
| `L_5C` | body |
| `L_60` | hlmt pointer |
| `L_64..L_6C` | temp vec3 |
| `L_70..L_8F` | 256-bit physics node mask |
| `L_90..L_C3` | matrix of orientation 0 |
| `L_C4..L_F7` | root transform from BFC70 |

```c
void BF0C0(u32 h) {
  u32 defn = tag_addr(rd32(obj_body(h)));       // BF0D6..BF0F9
  u32 body = obj_body(h);                       // EBX
  s32 nbytes = sx16(body+0x114);
  u32 count = (u32)(((u64)0x4EC4EC4Fu * (u32)nbytes) >> 32) >> 4;   // MUL (unsigned), SHR 4: == nbytes/52 for 0<=nbytes<2^16
  L_1C = body + sx16(body+0x116);               // ST BF114
  wr32(body+4, rd32(body+4) & 0xBFFFFFFF);      // ST BF118  (heap, always)
  L_20 = defn;                                  // ST BF11F
  L_24 = count;                                 // ST BF131
  L_12 = 1;                                     // ST BF137 (byte)
  bool biped = (rd8(defn) & 31) == 0;           // (1<<type)&1
  if (biped && rd8(obj_body(h)+0x34B) == 2) return;                // BF14B -> BF625
  memset32(&L_70, 0, (s32)(count + 31) >> 5);   // BF16C REP STOSD (SAR 5); BF173 REP STOSB has count 0
  u32 phys = rd32(body+0xB4);
  if (phys != NONE && (rd8(body+0xC0) & 0x40)) {                   // BF175..BF18C
    u32 comp = rd32(rd32(0x52BE5C)+0x44) + (phys & 0xFFFF)*0xA0;
    f32 off = biped ? E3BC0(/*EAX=*/h) : 0.0f;                     // XMM0
    L_14 = off;                                                    // ST BF1C6
    1DBBD0(comp, &L_70, off);                                      // stdcall(3)
  }
  u32 b2 = obj_body(h);
  L_5C = b2;                                                       // ST BF1F7
  if (rd16(b2+0x112) != 0xFFFF) L_18 = body + sx16(body+0x112);    // ST BF206
  else { L_18 = 0x4E8BC0; /*ST BF20C*/  L_12 = 0; /*ST BF214*/ }
  u32 hl = rd32(defn+0x38);           if (hl == NONE) goto no_model;
  u32 hlmt = tag_addr(hl); L_60 = hlmt;                            // ST BF235
  u32 mi = rd32(hlmt+4);              if (mi == NONE) goto no_model;
  u32 parent = rd32(body+0x14);
  u32 mode = tag_addr(mi);
  L_14 = mode; L_28 = 0; L_11 = 0; L_13 = 0;                       // ST BF257, BF25B, BF263, BF268
  if (parent != NONE) {
    // push dword whose low16 = (s16)(s8)rd8(body+0x18); upper16 = upper16 of [0x4ECBD0] value (ECX)
    L_28 = BAC00(/*EAX=*/parent, (s16)(s8)rd8(body+0x18));         // ST BF282
    L_11 = (rd32(obj_body(parent)+4) >> 10) & 1;                    // ST BF299 (byte)
  }
  u32 b = L_5C;
  if (!(rd32(b+4) & 0x20000000) && !(biped && rd8(b+0x34B) != 0)) {   // BF29D..BF2C5
    1707A0(mode, L_18);                                            // bind pose
    if (C15D0(/*EAX=*/h) && !C1CC0(/*ECX=*/h)) {                   // AL tests
      u32 mgr = body + sx16(body+0x12A);                           // EBP
      if (rd32(mgr+0x68) != NONE) {
        if (!(rd8(0x56EBAC) & 1)) wr32(0x56EBAC, rd32(0x56EBAC) | 1);   // ST BF31A
        u32 mask = 0;                                              // ESI
        if (L_12 && !C1F40(/*ECX=*/hlmt+0xA4)) {                   // not all-zero
          for (i=0;i<8;i++) wr32(0x56EB8C+4*i, rd32(hlmt+0xA4+4*i)); // ST BF351 x8 (REP MOVSD)
          C1EB0(/*EAX=*/0x56EB8C);
          C1F00(/*EAX=*/count, /*ESI=*/0x56EB8C);
          mask = 0x56EB8C;
        }
        BFB90(mode, mgr, L_18, /*EAX=*/count, /*ECX=*/mask, /*EDX=*/h);
        C1E50(/*EAX=*/body+4, /*ECX=*/30, /*DL=*/mask != 0);        // RMW store body+4 (bit30)
        if (rd32(mgr+0x68) != NONE && B87B0(/*EAX=*/mgr)) {
          u32 anim = B8780(/*ECX=*/mgr);                            // 0 possible -> reads [0x16]
          L_13 = (rd8(anim+0x16) >> 1) & 1;                         // ST BF3B9
        }
      }
    }
  }
  // BF3C1
  if (rd32(mode+0x48) == 0)
    for (i=0;i<8;i++) wr32(L_18+4*i, rd32(rd32(0x474D38)+4*i));    // ST BF3D7 x8 (identity orientation)
  if (!(rd8(&L_70) & 1)) {
    if (L_13) {
      144CF0(L_1C, L_18);                                          // world-relative root
    } else {
      // order of stores: BF40B..BF448
      L_44=pos.x; L_48=pos.y; L_50=fwd.x; L_4C=pos.z; L_54=fwd.y; L_58=fwd.z; L_40=up.z; L_38=up.x; L_3C=up.y;
      if (rd32(body+0xD4) != NONE && (rd8(body+0xD8) & 1)) {
        if (B3AE0(/*ECX=*/h, /*ESI=*/&L_2C))                       // AL
          { L_44=L_2C; L_48=L_30; L_4C=L_34; }                     // ST BF477,BF47B,BF47F
        if (B3B70(/*ECX=*/h, /*ESI=*/&L_64, /*EDI=*/&L_2C))
          { L_50=L_2C; L_54=L_30; L_58=L_34; L_38=L_64; L_3C=L_68; L_40=L_6C; }  // ST BF4A7..BF4C7
      }
      BFC70(/*EAX=*/&L_38, /*ECX=*/&L_50, /*BL=*/L_11,
            &L_44, rd32(body+0xA0), (rd32(body+4)>>10) & 0xFFFFFF01, L_28, &L_C4);
      144CF0(&L_90, L_18);
      145560(/*ECX(A)=*/&L_C4, /*EDX(B)=*/&L_90, /*out=*/L_1C);
    }
    10E180(/*EBX=*/h, L_1C);
  }
  // BF53C: children of the render-model hierarchy
  if ((s32)rd32(mode+0x48) <= 1) return;
  for (s32 i = 1; i < (s32)rd32(L_14+0x48); i++) {                 // bound re-read every iteration
    u32 rec = rd32(L_14+0x4C) + i*0x60;
    if (((u32*)&L_70)[i >> 5] & (1u << (i & 31))) continue;        // SAR 5
    u32 node = L_1C + i*0x34;
    144CF0(node, L_18 + i*0x20);
    145560(/*A=*/L_1C + sx16(rec+4)*0x34, /*B=*/node, /*out=*/node);
  }
  return;
no_model:                                                          // BF5C3
  u32 p = rd32(body+0x14), pm = 0; u8 pmir = 0;
  if (p != NONE) { u32 pb = obj_body(p); pm = pb + sx16(pb+0x116) + (s8)rd8(body+0x18)*0x34; pmir = (rd32(pb+4)>>10)&1; }
  BFC70(/*EAX=*/body+0x7C, /*ECX=*/body+0x70, /*BL=*/pmir,
        body+0x64, rd32(body+0xA0), (rd32(body+4)>>10) & 0xFFFFFF01, pm, /*out=*/L_1C);
}
```

Notes:
* The `(flags>>10) & 0xFFFFFF01` dword is pushed in full. BFC70 reads only its low byte.
* The BAC00 node argument is pushed as a full dword whose upper 16 bits are junk (ECX). This
  matters only for stack-store parity.
* With no model, the physics mask and the orientations are ignored. Node 0 is overwritten
  even if 1DBBD0 delivered it.

### 4.3 BFF20: `u8 __stdcall BFF20(u32 h)`, RET 4

Returns AL = 1 (the spheres were stored) or 0 (registered and unchanged).

Frame: `SUB ESP,14h` then four pushes. The locals are:
* `F+13` (u8): connected flag;
* `F+14` (f32): chosen radius;
* `F+18..F+20`: chosen center;
* `F+28`: the argument.

Globals read: `0x4ECBD0`, `0x4F0414`, `0x52BE5C`, `0x46A0D0`, `0x46A0E8`, `0x46A560`.

```c
u8 BFF20(u32 h) {
  u32 body = obj_body(h), defn = tag_addr(rd32(body));
  u8 conn = (rd32(body+4) >> 8) & 1;  F13 = conn;                    // ST BFF68
  u32 phys = rd32(body+0xB4);
  if (phys != NONE && ((1u << (rd8(body+0xAA)&31)) & 0x800) /*crate*/ && (rd8(body+0xC0) & 0x40)) {
    u32 comp = rd32(rd32(0x52BE5C)+0x44) + (phys&0xFFFF)*0xA0;
    if (1DC7A0(comp, body+0x30, body+0x3C) & 0xFF) goto spheres;    // stdcall(3), AL
  }
  {                                                                  // BFFC0
    u32 m = body + sx16(body+0x116);                                 // node 0
    f32 s = m.scale, ox = defn[+8], oy = defn[+0xC], oz = defn[+0x10];
    if (!ordered_equal(s, 1.0f)) { ox = s*ox; oy = s*oy; oz = s*oz; }
    wr(body+0x30, ((m.up.x*oz + m.left.x*oy) + m.fwd.x*ox) + m.pos.x);   // ST C003F
    wr(body+0x34, ((m.up.y*oz + m.left.y*oy) + m.fwd.y*ox) + m.pos.y);   // ST C006B
    wr(body+0x38, ((m.up.z*oz + m.left.z*oy) + m.fwd.z*ox) + m.pos.z);   // ST C0098
    wr(body+0x3C, body.scale(+A0) * defn.radius(+4));                    // ST C00AA
  }
spheres:                                                             // C00AF
  if (defn[+0x20] > 0.0f) {                                          // JA (ordered, strictly greater)
    wr(body+0x50, rd(body+0x30) + defn[+0x24]);                      // ST C00C9
    wr(body+0x54, rd(body+0x34) + defn[+0x28]);                      // ST C00D7
    wr(body+0x58, rd(body+0x38) + defn[+0x2C]);                      // ST C00E6
    wr(body+0x5C, body[+0xA0] * defn[+0x20]);                        // ST C00F8
  } else {
    wr32(body+0x50, rd32(body+0x30)); wr32(body+0x54, rd32(body+0x34)); wr32(body+0x58, rd32(body+0x38)); // C0105,C010D,C0110
    wr32(body+0x5C, rd32(body+0x3C));                                // C0116
  }
  bool alt = rd16(defn+2) & 0x1000;
  vec3 c = alt ? body[+0x50] : body[+0x30];  f32 r = alt ? body[+0x5C] : body[+0x3C];
  F18 = c.x; F1C = c.y; F20 = c.z; F14 = r;                           // ST C0149,C014D,C0151,C0155
  if (conn) {
    f32 dz = c.z - body[+0x48], dx = c.x - body[+0x40], dy = c.y - body[+0x44];  // from the stack copies
    f32 d2 = (dz*dz + dx*dx) + dy*dy;
    if (!(d2 > 0.01f) && ordered_equal(body[+0x4C], r)) return 0;    // C01F4: no sphere store
    BA7E0(h);
  }
  wr32(body+0x40, c.x); wr32(body+0x44, c.y); wr32(body+0x48, c.z); wr(body+0x4C, r);  // C01CD..C01D5
  if (conn) BA630(h, 0);
  return 1;
}
```
* Register detail: the return is EAX = `(body+0x40 or +0x50) & ~0xFF | AL`. LAHF also writes AH earlier.
* Value analogue (non-crate path): `source_shadow_sphere` in `src/rendering/shadows/source_shadow.cpp` (see §5).

### 4.4 BFB90: `void BFB90(u32 mode, u32 mgr, u32 orient)` + `EAX=count, ECX=mask (0 = none), EDX=h`, RET 0xC

Preserves EBX, EBP, ESI, EDI. Globals read: `0x4F0414`, `0x4ECBD0`.

```c
void BFB90(mode, mgr, orient; EAX count, ECX mask, EDX h) {
  s32 n = count;                                     // ESI
  if (mask) {
    s32 hi = 141010(/*EAX=*/count, /*EDI=*/mask);    // highest set bit index < count, else -1
    if (hi >= 0 && hi + 1 < n) n = hi + 1;
  }
  if (n == 0) return;                                // TEST ESI,ESI; JE
  if (rd32(mgr+0x68) != NONE && rd32(mgr) != NONE && rd16(mgr+6) != 0xFFFF) {
    (void)1E3150(/*EDX=*/tag_addr(rd32(mgr)), rd32(mgr+4));  // result discarded (resource request side effect)
    1D5840(mgr, mode, 1.0f /*0x3F800000*/, mask, orient, 0, h, /*ESI=*/n);
  }
  BF9A0(h, mode, mgr, mask, n, orient);
  10E100(/*EBX=*/h, mask, n, orient);
  if (rd8(mgr+0x65) && !(rd8(mgr+0x67) & 2)) {
    u32 b = obj_body(h);
    1E1720(/*EAX=*/mgr+0x64, /*ECX=*/orient, /*EBX=*/(s32)(s16)n, mask, b + sx16(b+0x10E));
  }
}
```
Memory stores: stack only. Everything else happens in the callees.

### 4.5 BFC70: root transform

Signature: `void BFC70(vec3* P, u32 scale_bits, u32 mirror_dw, mat* M_or_0, mat* OUT)`
plus `EAX=U(up*), ECX=F(fwd*), BL=parent_mirrored`. RET 0x14.

Frame: `SUB ESP,9Ch`, then EBP, ESI and EDI are pushed. Let F be ESP after those pushes.
Three local matrices:
* `F+0C`: S, or the parent copy M';
* `F+40`: T (translation);
* `F+74`: R (rotation).

Each holds 13 dwords. Globals read: `0x46A0D0`.

```c
void BFC70(...) {
  f32 lx = (U.y*F.z) - (F.y*U.z);     // all products computed from memory before any store
  f32 ly = (F.x*U.z) - (U.x*F.z);
  f32 lz = (F.y*U.x) - (F.x*U.y);
  // stores, in this exact order (F-relative):
  F+68=P.x; F+70=P.z; F+6C=P.y; F+78=F.x; F+80=F.z; F+98=U.z;
  F+40=1; F+44=1; F+48=0; F+4C=0; F+50=0; F+54=1; F+58=0; F+5C=0; F+60=0; F+64=1;
  F+74=1; F+7C=F.y; F+84=lx; F+88=ly; F+8C=lz; F+90=U.x; F+94=U.y; F+9C=0; F+A0=0; F+A4=0;
  // T = {1,(1,0,0),(0,1,0),(0,0,1),P}   R = {1,F,(lx,ly,lz),U,(0,0,0)}
  if (mirror_dw & 0xFF) { F+88 = 0.0f-ly; F+84 = 0.0f-lx; F+8C = 0.0f-lz; }    // store order 88,84,8C
  if (!ordered_equal(scale, 1.0f)) {
    push(&R);                                             // stack store
    F+0C=scale; F+10=1; F+14=0; F+18=0; F+1C=0; F+20=1; F+24=0; F+28=0; F+2C=0; F+30=1; F+34=0; F+38=0; F+3C=0;
    145560(/*A=*/&R, /*B=*/&S, /*out=*/&R);               // R.scale = scale*1; other fields unchanged for finite input
  }
  if (M) {
    mat* A = M;
    if (!ordered_equal(M->scale, 1.0f) || BL) {
      copy 13 dwords M -> F+0C (M');                      // REP MOVSD, ascending
      if (!ordered_equal(M'.scale, 1.0f)) {
        F+68 = F+68 * M'.scale; F+6C = F+6C * M'.scale; F+70 = F+70 * M'.scale;   // T.pos *= parent scale
        F+0C = 1.0f;                                      // parent scale is not propagated
      }
      if (BL) { F+1C = 0-F+1C; F+20 = 0-F+20; F+24 = 0-F+24; }   // negate parent left (store order 1C,20,24)
      A = &M';
    }
    145560(A, /*B=*/&T, OUT);                             // OUT = M ∘ T
    145560(OUT, /*B=*/&R, OUT);                           // OUT = OUT ∘ R
  } else {
    145560(&T, &R, OUT);                                  // OUT = T ∘ R
  }
}
```

Result: `OUT = Parent' ∘ Translate(P') ∘ [scale·Basis(F, ±(U×F), U)]`. Every composition must
go through the exact 145560 arithmetic. Do not shortcut, even for identity-looking factors:
signed zeros, Inf and NaN behave differently.

The rotation R equals the result of `144BF0` basis with zero position. Compare
`basis()` in `src/physics/native_tu15_object_motion.cpp`: its products are the same up to
multiplication commutativity.

### 4.6 144CF0: `void __stdcall 144CF0(mat* out, orient* q)`, RET 8

The routine builds an EBP frame. Globals read and written:
* reads `0x56E684` (flag), `0x46A0EC`, `0x56E680`, `0x46A0D0`, `0x45F490`;
* writes `0x56E684` and `0x56E680` (once, lazily).

```c
if (!(rd8(0x56E684) & 1)) { wr32(0x56E684, rd32(0x56E684) | 1); wr32(0x56E680, rd32(0x46A0EC)); } // ST 144D12, 144D18
v4 Q = {q.i,q.j,q.k,q.w};                  // MOVLPS+MOVHPS
v4 sq = Q*Q;                               // packed
n = (sq0 + sq2) + (sq1 + sq3);             // MOVHLPS + ADDPS + SHUFPS 55h + ADDSS. Lanes 2,3 use stale caller XMM1 lanes (value-irrelevant; MXCSR flags only)
if (!(n > eps /*[0x56E680]*/)) {           // includes NaN
  // XMM0 = {1.0,0,0,0}
  out+04 = {1,0}(8B); out+0C = {0,0}(8B); out+14 = {1,0}(8B); out+1C = {0,0}(8B); out+24 = 1.0;   // ST 144D55..144D65
} else {
  r = RCPSS(n);  s = ((r + r) - ((n*r)*r)); s = s + s;        // Newton–Raphson 2/n, RCPSS bits matter
  qs = Q*s (packed);  I = i*qs;  W = w*qs;  J = j*qs;  kk = (k*s)*k;
  // with I=[i(is),i(js),i(ks),i(ws)], W=[w(is),w(js),w(ks),w(ws)], J likewise:
  X = shuffle(I,0x99) + (shuffle(W,0x66) ^ {sign,0,0,sign})
    = [i(js)-w(ks), i(ks)+w(js), i(js)+w(ks), i(ks)-w(js)]
  out+08 = X[2],X[3] (8B)   // fwd.y, fwd.z            ST 144DD1
  out+10 = X[0]             // left.x                  ST 144DD5
  out+18 = X[0],X[1] (8B)   // left.z (temp), up.x     ST 144DDA
  out+04 = (1 - kk) - j(js)            // fwd.x        ST 144DF9
  out+14 = (1 - kk) - i(is)            // left.y       ST 144E02
  out+24 = (1 - i(is)) - j(js)         // up.z         ST 144E17
  out+20 = j(ks) - w(is)               // up.y         ST 144E24
  out+18 = j(ks) + w(is)               // left.z final ST 144E2D
}
out+00 = q+1C (scale); out+28 = q+10,q+14 (8B); out+30 = q+18.     // ST 144E37, 144E3F, 144E48
```
Aliasing hazard: `q` is re-read after the stores. Out must not alias q. The callers in this
tree never alias them.

### 4.7 145560: `out = A ∘ B`

Calling convention: `ECX=A`, `EDX=B`, `[ESP+4]=out`; RET 4; returns EAX = out.

```c
s = A.scale;
pos  = ((B.px*A.f + B.py*A.l) + B.pz*A.u) * s + A.pos;   // packed lanes {x,0,y,z}
up   = B.ux*A.f + (B.uy*A.l + B.uz*A.u);
left = B.lx*A.f + (B.ly*A.l + B.lz*A.u);
fwd  = B.fx*A.f + (B.fy*A.l + B.fz*A.u);
scale = B.scale * A.scale;
```

Store order:
1. `out+28` (4 B), then `out+2C` (8 B) (sites 1455CD/1455D2);
2. `out+1C` (4 B), then `out+20` (8 B) (145600/145605);
3. `out+10`, then `out+14` (145633/145638);
4. `out+04`, then `out+08` (145661/145666);
5. `out+00` (14566A).

A is read completely before the first store. B's fields are read just before each row's
store, so `out==A` and `out==B` are both safe. Lane 1 is computed but never stored. It can
affect only the MXCSR sticky flags.

### 4.8 1707A0: `void __stdcall 1707A0(mode* m, orient* o)`, RET 8

For each node, it copies the default pose:

```c
for (s32 i = 0; i < (s32)rd32(m+0x48) /*re-read each iteration*/; i++) {
  rec = rd32(m+0x4C) + i*0x60;  d = o + i*0x20;
  d+00=rec+18; d+04=rec+1C; d+08=rec+20; d+0C=rec+24;   // quaternion     ST 1707CF,1707D4,1707DA,1707E0
  d+10=rec+0C; d+14=rec+10; d+18=rec+14;                // translation    ST 1707EB,1707F0,1707F6
  d+1C=rd32(0x46A0D0);                                  // scale 1.0      ST 1707FD (1.0 loaded once)
}
```
There is no bounds check against the bank size (the static bank holds 255 entries).

### 4.9 Small leaves

* **BAC00**: `EAX=h`, `[ESP+4]` = s16 node. Returns `EAX = body + sx16(body+0x116) + node*0x34`.
  RET 4. On return ECX = body and EDX = node bank.
* **C15D0**: `EAX=h`. `AL = (rd16(body+0x12A) != 0xFFFF)`. The upper bits of EAX are the
  upper bits of the body pointer. RET 0.
* **C1CC0**: `ECX=h`. Only AL is written. `AL = (h != NONE && rd8(body+0xB3) != 0)`.
* **C1E50**: `EAX=p`, `CL=bit`, `DL=set`.
  * If DL is set: `*p |= 1<<CL`.
  * Otherwise: `*p &= ~(1<<CL)`. One 4-byte store either way.
* **C1EB0**: `EAX=v` (8 dwords).
  * For i = 0..6: `v[i] = ~v[i]` (stores at +0, +4, …, +18).
  * Then `v[7] = ~v[7]` (store), then `v[7] = (~v_old7) & 0xFF` (a second store to +1C).
* **C1F00**: `EAX=count`, `ESI=v`.
  * `last = ((s32)(count+31) >> 5) - 1`, `words = last + 1`.
  * If `words < 8`: zero `v[words..7]` (REP STOSD).
  * `b = count & 31`; `m = b ? 0xFFFFFFFF >> (32-b) : 0xFFFFFFFF`; `v[last] &= m` (RMW store).
  * Edge case: count 0 gives `v[-1] &= ~0`, which stores the same value to 0x56EB88.
* **C1F40**: `ECX=v`. `AL = all eight dwords are 0`. EAX's upper bits come from v[7].
* **B87B0**: `EAX=mgr`. `EAX = (mgr+68 != NONE && mgr+0 != NONE && rd16(mgr+6) != 0xFFFF)`.
* **B8780**: `ECX=mgr`. Returns 0 if `rd16(mgr+6)==0xFFFF`. Otherwise returns
  `1E3150(EDX=tag_addr(mgr+0); push mgr+4)`.
* **E3BC0**: `EAX=h`. Returns XMM0, initially 0.
  * If `body+34B==1 && body+3DC==3`: `XMM0 = 1F4920(EAX=body+0x3EC) * (-0.125f)`.
  * 1F4920 reads `t = rd32(p+0x5C)`. If t is NONE it returns 0. Otherwise:
    * `x = 1 - ((f32)(s32)(tick - t) * gt[+4]) * 6.6666665f`, where `tick = rd32(rd32(0x51DF54)+8)`
      and gt[+4] is the f32 at `rd32(0x51DF54)+4`;
    * return 0 if `0 > x`, 1.0 if `x > 1`, otherwise x.
* **141010**: `EAX=count`, `EDI=v`. Returns the index of the highest set bit below count,
  else −1. It uses BSR on the masked last word, then walks down. It writes two dwords of its
  own stack frame. Edge case: count 0 reads `v[-1]`.

### 4.10 B3AE0 / B3B70: network prediction overrides

Both use `ECX=h` and return AL.
* B3AE0 outputs to `ESI` (vec3 position).
* B3B70 outputs `ESI` (up) and `EDI` (forward).

Each one:
1. Returns 0 unless `rd32(body+0x14)==NONE` (unparented) and `rd8(body+0xD8)&1`.
2. Zeroes a 0x24-byte stack record.
3. Calls `B3A50(ECX=h; &record)`. B3A50 requires `rd32(rd32(0x4DC04C)+8)` to be 4 or 5,
   and a valid entity `rd32(body+0xD4)` (index `&0x3FF`, row stride 0x24 at
   `rd32(0x4DC054)…+0x20AC`). It calls the virtual `[[entity]+0x88]`. The target is **OPEN**.
4. On success:
   * if record byte bit0 is set, B3AE0 copies record+04..+0C into ESI;
   * if bit1 is set, B3B70 copies record+1C..+24 into EDI and record+10..+18 into ESI.

### 4.11 10E180 and 10E100: per-type hooks

**10E180**: `EBX=h`, one stack arg (nodes). RET 4.
* For each descriptor `d` in `rd32(0x474B90 + sx8(body+0xAA)*4) + 0x84`
  (NULL-terminated, index kept as s16): if `fn = rd32(d+0x70)`, call `fn(h, nodes)`
  (stdcall 2).
* Hooks per type: unit `1154A0`, scenery `10F440`.
* **1154A0**:
  * `o = rd32(body + sx16(body+0x33E) + 0x28)`;
  * if `o != NONE`, `ob = BCDF0(EAX=o; push −1)` (verify and get the object, foreign);
  * if `ob` is nonzero: `145560(A=ob+sx16(ob+0x116), B=nodes, out=nodes)`.
  * The second arg is the nodes pointer; the result goes back in place.
* **10F440**: identical, with `o = rd32(body+0x138)`.

**10E100**: `EBX=h`, stack `(mask, n, orient)`. RET 0xC. Same walk over slot `+0x6C`, calling
`fn(h, mask, n, orient)` (stdcall 4).
* Hooks per type: unit `114750` (849 instr: aiming/looking/IK channel blends, 8× 1CF210),
  vehicle `F34B0` (builds a {mask, n, orient} stack record and calls `F2CC0(h, 0xF2C40, &rec)`),
  device `10C0B0` (foreign).
* The internals of 114750, F2CC0 and 10C0B0 are **OPEN**: not specified here.

### 4.12 1D5840: channel sampling

Signature: `void __stdcall 1D5840(mgr, mode, weight, mask, orient, a5, a6)` plus **ESI = node
count**. RET 0x1C.

`1CF210` takes `EBX=channel` and stack `(weight, mask, ESI count, orient)`, RET 0x10. It
samples one animation channel into the orientations. It never writes the channel state. Its
stores go to its own frame and, on one path, to a static scratch:
* When the caller mask and the animation's 32-byte node mask (a data pointer + 0x7C) are both
  non-null and differ, it copies the animation mask into static `0x56E060..0x56E07F`
  (REP MOVSD, site 1CF315). It then combines the two masks with
  `284090(ECX=caller mask, EAX=0x56E060)` and samples with the combined mask.
* If channel byte `+10` and animation byte `+10` are both set, it requests the resource via
  1E5C80 and returns without sampling.

```c
if (rd8(0x47C1B5) && rd32(mgr+0x40) != NONE && rd16(mgr+0x46) != 0xFFFF) {
  1CF210(EBX=mgr+0x40; weight, mask, n, orient);
  if (n <= 0) return;  if (mask && !(rd8(mask) & 1)) return;
  1D63C0(EBX=mgr, EDI=orient; push weight);   // foreign via required(); OPEN
  return;
}
if (rd8(0x47C1B4) && rd32(mgr+0x20) != NONE) {
  f32 c = [0x46A188], f = mgr[+0x80];
  if (rd16(mgr+0x26) != 0xFFFF && !(f < c /*JB: true for NaN*/)) { 1CF210(EBX=mgr+0x20; weight,...); return; }
  if (rd32(mgr+0x20) != NONE && rd16(mgr+0x26) != 0xFFFF && !(eps >= f) && !(f >= c)) {   // eps=[0x46A0EC]
    f32 b = 1D61B0(EAX=mgr);                 // clamp(f+eps,0,c)*1.0002, then clamp(.,0,1)
    arg0_slot = b;                           // ST 1D5918: overwrites this routine's own stack arg slot [mgr]
    1CF210(EBX=mgr;      weight,     mask, n, orient);
    1CF210(EBX=mgr+0x20; b*weight /*= arg0_slot*arg2*/, mask, n, orient);    // ST 1D5939 = weight into pushed slot
    return;
  }
}
1CF210(EBX=mgr; weight, mask, n, orient);
```

**1D61B0** (`EAX=mgr` in, XMM0 out):
1. `x = f + eps`.
2. If `0 > x`: x = 0. Else if `x > c`: x = c.
3. `x *= 1.0002f` (`[0x46A184]`).
4. If `0 > x`: x = 0. Else if `x > 1`: x = 1.

### 4.13 BF9A0: function overlays

Signature: `void __stdcall BF9A0(h, mode, mgr, mask, n, orient)`, RET 0x18. It uses the x87 FPU.

1. It initialises a stack temporary channel at F+28..F+47. The stores are, in order:
   * F+44 = 0.0;
   * w16 F+24 = −1 (loop index);
   * F+28 = −1; w16 F+2C = −1; w16 F+2E = −1;
   * b F+38 = 0; b F+39 = 0; w16 F+3A = 0;
   * F+40 = 1.0; w16 F+3C = 0; w16 F+3E = 0;
   * F+30 = −1; w16 F+36 = −1; b F+34 = 0xFF; b F+35 = 0xFF.

   Here F is ESP after the four pushes. The first store happens after only three pushes.
2. For `i` in `0 .. rd32(g+0x44)-1`, where `g = tag_addr(rd32(mgr+0x68))` is re-read every
   iteration:
   1. Load the entry `e = rd32(g+0x48) + i*20`. Store `w16 F+24 = i` and `F+18 = rd32(e+4)`.
   2. If `rd16(e+6) != 0xFFFF`, call `1E5C80(EDX=g; push e+4)` (resource request).
   3. Skip the entry if `rd32(e+0xC) == 0` or `rd32(mgr+0x68) == NONE`.
   4. Call `1D36C0(EAX=mgr, EBX=0x7F; &chan, rd32(e+4), −1, −1, −1, −1)` (RET 0x18). If AL
      is 0, skip the entry.
   5. Look up `anim = (chan+6 != 0xFFFF) ? 1E3150(EDX=tag_addr(chan+0); chan+4) : 0`.
   6. Call `BCB70(h, rd32(e+0xC), &F10)` (RET 0xC). It writes the function value v to F+10.
   7. Branch on `controls = rd16(e+0xA)`:
      * `controls == 0`: frame = `(f32)(sx16(anim+0x14) - 1) * v`. Call `1CE810(EAX=&chan; frame)`
        (RET 4), then `1CF210(EBX=&chan; 1.0f, mask, n, orient)`.
      * `controls == 1`:
        * `F+14 = tick + h` (u32). The tick is `rd32(rd32(0x51DF54)+8)`, and h is the full
          handle, used as a phase offset.
        * Compute, in x87: `x = (f80)(s32)(tick+h)`, plus 2^32 if the value is negative.
          Then `x = x * gt[+4] * (1/30)`.
        * `F+14 = sx16(anim+0x14)`, then `frame = fmod(x, (f80)frames)` via foreign CRT
          `0x37DC0A`, rounded by FSTP to f32.
        * Call `1CE810(&chan; frame)`, then `1CF210(EBX=&chan; v, mask, n, orient)`.
      * Any other value: skip the entry.

The precision of the x87 steps depends on the FPU control word. The semantics of the
`37DC0A` call are **OPEN**.

### 4.14 1E1720: cross-fade

Signature: `EAX=interp(mgr+0x64)`, `ECX=cur orient`, `EBX=n`, stack `(mask, prev orient)`.
RET 8. It uses x87.

1. If `rd8(interp+1)==0`, return.
2. `t = 1E1620(EAX=interp)` (XMM0, clamped to [0,1]; a switch on `interp+2` selects linear
   or one of the eases 110FC0/1E1920/quadratic/cubic).
3. For each node `i < n` with `!mask || bit i of mask`:
   1. `d = ((p0*c0 + c1*p1) + c2*p2) + p3*c3`.
   2. `t1 = (0 > d) ? (0 - t) : t`.
   3. Blend the quaternion, storing each component in turn:
      * `c0 = p0*(1-t) + c0*t1`;
      * `c1 = c1*t1 + (1-t)*p1`;
      * `c2 = c2*t1 + (1-t)*p2`;
      * `c3 = p3*(1-t) + t1*c3`.
   4. `L = ((c0² + c1²) + c2²) + c3²` (using the freshly stored values). Store
      `[esp+0Ch] = L`. That is the saved-ECX slot, so ECX on return is the last L.
   5. Normalise:
      * If `L > 0`: in x87, `k = 1/sqrt(L)`, then `c_i = (f32)(k*c_i)` for i = 0..3.
      * Otherwise: `c = (0,0,0,1)`.
   6. For each translation component and the scale: `c = (c - p)*t + p`.

Stores are written only to the current bank. prev is read-only.

### 4.15 1E3150 / 1E5C80: animation lookup and resource request

**1E3150**: `EDX=jmad`, `[ESP+4]=packed id`. RET 4. Returns the animation definition
pointer, or 0.
* If the high16 of the id is −1, return 0.
* If the low16 is −1, use the graph's own animations: `jmad+0x30 + hi*0x6C`.
* Otherwise, if `lo < rd32(jmad+0x4C)`, use the inherited graph
  `g = tag_addr(rd32(rd32(jmad+0x50) + lo*32 + 4))`, and the animation is
  `rd32(g+0x30) + hi*0x6C`.
  * If lo is out of range, g is 0 and `[0x30]` is read: a latent null dereference.
* If the result is nonzero, call `1E5C80(ESI=anim; push id)`.

**1E5C80**: gated by `rd8(0x51DF20) && rd8(0x51DF21)`. It walks the animation chain (`+30`,
`+32`). For animations with flag `+16 & 0x10` and a resource row `rd32(jmad+0xB0) + anim[+24]*20`
whose byte `+10` is nonzero, it calls `126200(ECX=1, EAX=row)`: a resource admission
request (persistent cache). The internals of 126200 are **OPEN**.

### 4.16 1DBBD0: physics node delivery (summary; verify before implementing)

Signature: `void __stdcall 1DBBD0(comp, u32* mask, f32 z_offset)`, RET 0xC. The frame is
16-byte aligned (`AND ESP,-16`).

1. Return if `(comp[+4] >> 9) & 1` or `comp[+0x74] <= 0`.
2. Resolve the object `comp[+8]`, and from it the definition, hlmt, mode (`hlmt+4`) and
   phmo (`hlmt+0x24`).
3. Zero a local 256-bit mask (8 stores). Set `orient_count = sx16(body+0x110) >> 5`
   (logical) and `orient = body + sx16(+0x112)` when the object has its own bank (else 0).
4. For each record `i < comp[+0x74]`, at `rec = comp[+0x70] + i*0x60`, and for each `j <
   rec[+0x4C]`:
   1. `rb = phmo[+0x3C] + sx8(rd8(rec[+0x48] + j))*0x90`.
   2. Skip if `rd16(rb) == 0xFFFF`.
   3. Skip if the virtual `[[[rec+0x40]+0x3C]] + 0x18` returns 6. The meaning of 6 is **OPEN**.
   4. Skip if mask bit `node = sx16(rb)` is already set.
   5. Walk the phmo node parents (`phmo+0xCC`, stride 12, `+6` parent):
      * if `node` descends from the record's root rigid-body node but is not that root, skip;
      * otherwise, deliver:
        1. Call `1D8B90(ECX=comp, EAX=i, ESI=node matrix)` (foreign).
        2. Set `node.pos.z = node.pos.z + z_offset`.
        3. For a non-root rigid body, build a matrix X from the virtual `[vt+0x34](i, &tmp)`
           result (rows at ret+0x10…) and set `node = node ∘ X` (145560).
        4. Set the bit in the caller's mask, set L_17 = 1, and mark the node and all its phmo
           ancestors in the local mask.
5. If anything was delivered and `mode+0x48 > 0`, call
   `1DBFD0(ECX=comp; 0, mask, &local_mask, orient, orient_count)` (recursive, 186 instr).
   This presumably fills the dependent nodes from orientations. **OPEN**.

### 4.17 1DC7A0, BA7E0, BA630 (summaries)

* **1DC7A0** `(comp, vec3* center, f32* radius)` → AL. RET 0xC. It builds an AABB over the
  component's rigid bodies via a Havok virtual (`+0x20`). It writes the midpoint center and
  the radius through its arguments. Whether it stores on failure is **OPEN**.
* **BA7E0** `(h)`. RET 4. It calls `C0F60(h,1,0,0,0)` and `1D30F0(h, body+0x60)` (cluster
  list 0x4EABB0 or 0x4EABA4, chosen by flag bit 9). Then
  `wr32(body+4, rd32(body+4) & ~0x100)` and `wr8(rec+2, rd8(rec+2) & 0xBF)`, where rec is
  the datum header.
* **BA630** `(h, a1)`. RET 8. This is cluster re-registration: BA910, 1D2F10 query, C0F60,
  B8D90, B92C0, B1870, BA570, B9330, 1CC290. It also writes body +28/+2C, header +4, body+4,
  rec+2 and a 0x78-byte stack scratch. Specify it with the B9460/BA630 family; out of scope
  here.

## 5. Existing native code

The search covered `src/`, `include/` and `docs/`, excluding `*_generated*` and `*.inc`. It
used word-boundary matching on the lowercase and uppercase forms. TU1.1-image counterparts
were also checked, because the repo already holds native work for them.

| routine | existing code | classification |
|---|---|---|
| BF050 | `include/halo2/native_tu15_object_scale.hpp` (comment: refresh = whole BF050); `src/campaign/source_campaign_object_scale_tu15.cpp:23` (`call(0xbf050u, …)` runs the **translated** routine via `world.run_actual`); docs NATIVE_REPLACEMENT_GOAL / RENDER_THREAD_OFFLOAD_PLAN / NATIVE_4K60_SPLITSCREEN_GOAL (census 4.4%) | mention / call site only. `docs/CAMPAIGN_BSP_RESEARCH.md` + `analysis/campaign/bsp-bf050.txt` is a **different image's** routine at 0xBF050 (name binding), unrelated |
| BF0C0 | `src/gameplay/objects/object_construction.cpp:205` (`o.call(0xb919f,0xbf0c0,…)` invokes the translated routine); `src/platform/native_tu15_world_memory.cpp` (mounts 0x56EBAC, 0x56EB8C, 0x4E8BC0 citing BF0C0); `src/rendering/models/model_pose.cpp:451` `pose_world_object_nodes_frame` + `include/halo2/model.hpp:153`, `frontend_model_primary.hpp:27` (host-side asset-level pose of world objects); `death_world_runtime_entries.hpp` (address list) | value-level host analogue (model_pose: parsed assets, not memory/store exact) plus mentions. **No TU1.5 memory-level replica.** |
| BF0C0 (TU1.1 = BDF80) | `src/gameplay/objects/object_current_nodes.cpp` `source_update_object_current_nodes_1_1` (+ `include/halo2/object_current_nodes.hpp`); `src/gameplay/objects/item_nodes.cpp` `source_item_current_nodes` (measured unparented items) | **native orchestration replica for the TU1.1 image**. Memory-level, with the store sites retained; animation/physics/type owners are delegated to services; 143960/1441D0 are native; BEB30 is native only with no parent and not mirrored. It does not write all of BF0C0's frame locals (L_1C/L_20/L_24/L_12/L_14/L_18/L_28/L_5C/L_60/L_11/L_13). It is the best starting point for a TU1.5 port (§6.1). |
| BFF20 | none in src/include. `src/rendering/shadows/source_shadow.cpp` `source_shadow_sphere` reproduces the non-crate center/radius formula (no address cited); docs MP_GRENADE_SCENE_RESEARCH (TU1.1 BEDE0 ↔ BFF20) | value-level analogue (partial) |
| BFF20 (TU1.1 = BEDE0) | `src/gameplay/objects/item_bounds.cpp` `source_item_bounds` (`include/halo2/item_bounds.hpp`) | memory-span native replica of the physical-NONE path; it fails when a registered sphere changes |
| BFB90 | `include/halo2/physics_live_world_tu15.hpp:380` (comment), `death_world_runtime_entries.hpp` (list), docs | mention only |
| BFC70 | `include/halo2/source_scene_physics.hpp:17` (comment); `src/rendering/shadows/source_shadow.cpp` `source_object_transform` (host Transform math: mirror, scale, parent scale/mirror, composition), used by `src/runtime/source_scene_physics.cpp` | value-level host analogue. Bit-exactness depends on `compose_transform` matching the 145560 grouping (**OPEN**, not checked). Not store-order exact. |
| BFC70 (TU1.1 = BEB30) | `item_transform_math.cpp` `source_item_root_matrix` (no parent, not mirrored) | memory-span native replica (subset) |
| 145560 | `src/physics/runtime/native_tu15_matrix_leaves.hpp` `kernel_145560` (installed by `tools/specialize_native_operands.py` LEAVES, body SHA 3ce39c10…) | **register-level kernel** (exact stores, registers, flags) |
| 145560 | `src/campaign/ai/native_campaign_object_markers_tu15.cpp` `native_campaign_object_transform_product_tu15` (store sites 1455CD… retained) | **full native replication** (memory/store-order exact) |
| 145560 | `src/physics/native_tu15_object_motion.cpp:471` `product()` | value-level native replication (typed, no stores) |
| 145560 | `src/gameplay/weapons/native_weapon_marker_math_tu15_internal.hpp` `marker_transform_product` (+ dispatch in `native_weapon_effect_dispatch_tu15.cpp:1653/1667`) | register-level per-instruction re-implementation |
| 145560 | `src/physics/physics_live_entries_tu15.hpp:587` (Module::Phantom entry) | mention |
| 145560 (TU1.1 = 1441D0) | `item_transform_math.cpp` `source_compose_item_matrix` (alias-aware) | full replica (span-based) |
| 144CF0 | `native_tu15_matrix_leaves.hpp` `kernel_144cf0` (SHA c6d3ad70…) | **register-level kernel** |
| 144CF0 | `src/campaign/native_campaign_model_pose_tu15.cpp` `local_transform()` | native value replication. It returns private values, publishes only the lazy-epsilon stores, and models the stale XMM1 lanes as `normalization_pair` |
| 144CF0 (TU1.1 = 143960) | `object_current_nodes.cpp` (native with an explicit RCPSS provider), `item_transform_math.cpp` `source_item_node_matrix` (positive branch only) | native replicas (TU1.1) |
| 1707A0 | none. (`native_campaign_model_pose_tu15.cpp` replicates the sibling **170820** hierarchy composition, not 1707A0. `object_current_nodes.cpp` inlines the TU1.1 16F3C0 copy.) | — |
| 10E180, 10E100, 1D5840, BF9A0, 1E1720, 1E3150, 1CF210, 1E5C80, 141010, C1xxx, B3AE0/B3B70, E3BC0, 1DC7A0, BA7E0, BA630 | none (C1EB0: comment in world_memory; 1E1720: docs MP_ANIMATION_RENDER_INTERPOLATION_RESEARCH TU1.1 1DCE60 ↔ 1E1720; BAC00: docs MP_SOUND_OBJECT_REQUEST_NATIVE_INTEGRATION) | mention only |
| BF9A0 | `src/gameplay/objects/source_object_render_flags.cpp`, `src/rendering/renderer/source_render_contact.cpp`, `include/halo2/{object_light_contacts,scene,source_object_render_flags}.hpp` | **address collision**: these describe a TU1.1-image BF9A0 (B7B40 flag-bit31 admission), not this TU1.5 overlay sampler |
| 1DBBD0 (TU1.1 = 1D7310 + 1D7710) | `src/physics/physics_node_delivery.cpp` `source_physics_deliver_nodes_1_1` | native replica, TU1.1 image |
| 1D8B90 (TU1.1 = 1D42D0) | `src/physics/physics_body_matrix.cpp` `source_physics_body_matrix_1_1` (header says the TU1.5 counterpart is 1D8B90) | native replica, TU1.1 image |
| 1154A0, 10F440, F34B0, 114750, 10C0B0 | `native_tu15_object_storage_data.cpp` (descriptor tables only) | data, not code |

## 6. Implementation guidance

### 6.1 Recommended route

Port `source_update_object_current_nodes_1_1` (TU1.1) to TU1.5 addresses, then wrap it in a
native BF050 and BFF20. The mappings below are confirmed by the docs or by identical call
shapes. Items marked "inferred" are unverified.

Routines:

| TU1.1 | TU1.5 | note |
|---|---|---|
| BDF80 | BF0C0 | |
| BEA50 | BFB90 | |
| BEB30 | BFC70 | |
| BEDE0 | BFF20 | |
| 143960 | 144CF0 | |
| 1441D0 | 145560 | |
| 16F3C0 | 1707A0 | |
| C0490 | C15D0 | |
| 10D260 | 10E180 | |
| 1D7310 | 1DBBD0 | |
| 1D7710 | 1DBFD0 | |
| 1D42D0 | 1D8B90 | |
| 1C9E30 | B8780 | inferred: ECX=manager, result +16 used |
| AA690 / AA720 | B3AE0 / B3B70 | inferred |
| 194CE0 | C1E50 | |

Globals and constants:

| TU1.1 | TU1.5 |
|---|---|
| 0x4E78D0 | 0x4ECBD0 |
| 0x4EB114 | 0x4F0414 |
| 0x526248 | 0x52BE5C |
| 0x4E38C0 | 0x4E8BC0 |
| 0x566C8C | 0x56EBAC |
| 0x566C6C | 0x56EB8C |
| 0x46FC78 | 0x474D38 |
| 0x465030 | 0x46A0D0 |
| 0x46504C | 0x46A0EC |
| 0x566760 / 0x566764 | 0x56E680 / 0x56E684 |
| 0x45A8C0 | 0x45F490 |
| 0x4654C0 | 0x46A560 |

Then make 145560, 144CF0, BFC70 (all branches) and 1707A0 fully native. They are leaf
arithmetic. Keep BFB90, 1DBBD0, 10E180, 1DC7A0, BA7E0, BA630 and B3AE0/B3B70 as translated
services until they are replaced.

### 6.2 Cheap cases

The TU1.5 BF0C0 skips all orientation work when:
* body+4 bit 29 is set;
* the object has no animation manager;
* the object is frozen (`+B3`); or
* the object is a biped with `+34B != 0`.

Then only the node composition (§1.2 steps 4–5) runs. Even then, do **not** reuse the old
matrices with a delta transform. The original recomputes `node[parent] ∘ M(orient)` in model
order. Float association differs, so the result is not bit-identical to a "re-root".

### 6.3 Parity hazards

* RCPSS (144CF0) is approximate. Use the same host instruction. The repo already notes that
  host-versus-Xbox equality is a platform boundary.
* x87 steps in BF9A0 (fmod path) and 1E1720 (sqrt/divide) depend on the FPU control word's
  precision control. Pass the retained x87 control word, as `NativeTu15PhysicsFrameFp` does.
* Packed-lane junk:
  * 144CF0 uses stale XMM1 lanes 2 and 3;
  * 145560 computes a lane 1.

  Neither affects stored values, but both can set MXCSR sticky flags. If MXCSR parity is
  required, use packed ops with the same lane contents, as the existing kernels do.
* Stack locals passed by pointer to translated callees must stay in emulated stack memory:
  * L_70 (mask, written by 1DBBD0);
  * L_38/L_44/L_50 (read by BFC70);
  * L_2C/L_64 (written by B3AE0/B3B70);
  * L_90/L_C4.
* Register-level contracts:
  * BFC70 takes BL;
  * BFB90 takes EAX, ECX and EDX;
  * 1D5840 takes ESI;
  * 10E100 and 10E180 take EBX;
  * 1E1720 takes EAX, ECX and EBX;
  * C1E50 takes EAX, ECX and DL;
  * C1F00 takes EAX and ESI;
  * 141010 takes EAX and EDI.
  * 1E1720 returns ECX equal to the last L.
* Latent original edge cases that native code must reproduce or explicitly guard:
  * B8780 returning 0 makes BF0C0 read `[0x16]`;
  * 1E3150 with an out-of-range inheritance index reads `[0x30]`;
  * C1F00 with count 0 does an RMW of 0x56EB88;
  * 141010 with count 0 reads `mask-4`.
* The count mismatch: the node mask is sized by `+114/52`, while the loops use `mode+0x48`.

## 7. OPEN items

1. The names and semantics of body+4 bits 29 and 30, `+B3`, the biped `+34B` states and
   `+3DC`, and `+D4/+D8`. The behaviour above is exact; only the names are guesses.
2. What callers of BF050 other than B9460 do with EAX. BF050 has no defined return.
3. The 1DBBD0 internals: what the Havok virtual `+0x18` return value 6 means, the result
   layout of the virtual `+0x34`, 1D8B90 (foreign in this unit), and 1DBFD0.
4. 1D63C0, 10C0B0, BCDF0 and CRT 37DC0A. All are foreign to this unit, reached through
   `required()`.
5. The internals of 114750 (849 instr), F2CC0 (522), 1D36C0, 1CE810, BCB70, 1E1620's eases,
   1CF210's decode chain (283A90/2835B0/284090) and its global scratch writes, and 126200
   resource admission.
6. 1DC7A0's exact arithmetic and whether it stores on failure.
7. The full store lists of BA7E0 and BA630. They belong to the B9460/BA630 spec.
8. The B3A50 virtual `[+0x88]` target, the meaning of the `0x4DC04C+8 ∈ {4,5}` mode, and the
   meaning of the game-time `+4` field.
9. Whether `compose_transform` in source_shadow.cpp matches the 145560 grouping bit for bit.
