# B95D0 / B9460: object relocation driver, native replacement spec

Read-only research spec (no build, no runs), 2026-11-08, tree HEAD `46d2364a`
(dirty working tree). Every statement below was transcribed from the
translated bodies. Anything not determined is marked **OPEN**.

Source snapshot (sha256, first 16 hex digits):

| File | sha256/16 |
|---|---|
| `src/physics/physics_live_support_tu15_generated.cpp` | `51a5f9a4a07a8ae4` |
| `src/gameplay/units/death_cycle_runtime_tu15_generated.cpp` | `7586f87225aa75dc` |
| `src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp` | `fae1561a752933f5` |
| `src/physics/physics_live_world_tu15.cpp` | `164ae84a41fc65b8` |
| `src/gameplay/objects/native_tu15_object_storage_data.cpp` | `1a840ac382f5ac83` |

The build-local copy
`build-perf-4k60/native-operands/physics/physics_live_support_tu15_generated.cpp`
has the same bodies for B95D0, B9460, BA7E0, B7E50, 10E340, BA570, BA630, BF050
and 1CD620. The only difference is how instructions are instrumented
(`s.instruction_literal(pc)` instead of `if(s.services.instruction)...`).

Notation used in this spec:

- `E` = ESP at routine entry, so `[E]` holds the return address and `[E+4]` is the first stack argument.
- `objects = *(u32*)0x4ECBD0` is the object data-array header. `rows = *(u32*)(objects+0x44)`.
- `hdr(i) = rows + (i & 0xFFFF)*12` and `body(i) = *(u32*)(hdr(i)+8)`.
- No routine here checks the salt. `NONE = 0xFFFFFFFF`.
- "Store PC" is the PC that the translated code passes to `w(...)`, and so to the store observer. A native replica should label its data stores with the same PCs.

---

## 0. Summary and corrections to the profiling picture

1. **B95D0 does not walk children or attachments itself.** It does the following, in order:
   - Copies the three optional input vectors into stack locals.
   - Lets per-object-type handlers rewrite those locals (10E340; in TU1.5 data only bipeds have a handler, E24F0).
   - Calls B9460 once for the object.
   - Deletes the object through BA570 if B9460 reports that the root position is out of the world bounds. This is skipped in raw-mode 4 when the object has a simulation entity.
   - Marks the object and all of its ancestors for update (B9390).
2. **The B9460 order in the profile is confirmed**, with these conditions:
   `[BA7E0 if connected && !keep_links] -> bounds check + position store + B7E50(+D4,2) -> forward/up store + B7E50(+D4,4) -> [1CD620 if update_physics] -> [BF050 if rebuild_nodes] -> [BA630 if connected && !keep_links]`.
   "Connected" is `body+4` bit 8, sampled before BA7E0 runs.
3. **Who walks what:**
   - BF050 walks children recursively over `+0x10` (first child) and `+0x0C` (next sibling). It skips children of type 7 (machine).
   - C0F60 walks children and attachments. It is reached from BA7E0, BA630 and BA570.
   - B9390 walks ancestors over `+0x14`.
4. **`+0x11C` is not an attachment count.** It is a signed 16-bit **byte size** of an array of 8-byte attachment descriptors (count = size >> 3, logical shift). `+0x11E` is the signed 16-bit offset of that array from the body.
5. **The Havok remove/re-add (2EC9B0/2EC4B0) is not statically reachable from B9460.** A BFS over `run_X(s)`, `dispatch` targets and `required` targets in the live-support unit never reaches 2EC9B0, 2EC4B0, 2E9CF0 or 2EAFB0. They are reached through indirect (virtual) calls below `1CD620 -> 1DC270`. Static closure sizes in the live-support unit: B95D0 344 routines, of which 89 are resolved out-of-unit through `required()`; B9460 342.
6. **Runtime routing surprise (section 2.3):**
   - The native root `run_actual(0xB95D0)` runs the **death-cycle unit's copy** of B95D0, not the live-support copy.
   - That copy reaches B9460 and BA570 through `required()` into the live-support unit.
   - It reaches B9390 through `required()` into the **scene-device-constructor unit's translated B9390**. The existing native B9390 route is never reached on this path.

---

## 1. Data model used by these routines

### 1.1 Globals

| Address | Type | Use |
|---|---|---|
| `0x4ECBD0` | u32 | Object data-array header. `+0x44` is the element base (`rows`). Elements are 12 bytes. |
| `0x46AA04` | f32 | `0xC7000000` = -32768.0f, world lower bound (mounted immutable at `src/platform/native_tu15_world_memory.cpp:1505`). |
| `0x46A530` | f32 | `0x47000000` = +32768.0f, world upper bound (`...world_memory.cpp:1507`). |
| `0x4F3218` | u32 | Game options/globals pointer. `+0x0C` u8 is the "raw mode"; B95D0 compares it with 4. `+0x11B8` is passed by address to B8D90 by BA630. The semantic name of mode 4 is **OPEN** (repo code calls it `raw_mode`). |
| `0x4DC04C` | u32 | Simulation/network world (B7E50). `+0x04` is the state pointer, `+0x08` is the mode (i32). |
| `0x474B90` | u32[13] | Object type definition pointers, indexed by the type byte. |
| `0x474D08` | u32 | Pointer to the default forward vector (`0x44AC28`). |
| `0x474D10` | u32 | Pointer to the default up vector (`0x44AC40`). |
| `0x474CE8` | u32 | Pointer to the default position vector, used by the 10F710/11DC00 call sites. Exact content **OPEN**. |
| `0x46A0E8` | f32 | 0.0f (E24F0 degenerate-length test). |
| `0x4F0414` | u32 | Tag instance table. Rows are 16 bytes; `+8` is the definition address (E24F0). |
| `0x52BE5C` | u32 | Physics component pool (1CD620). `+0x44` is rows; records are 160 bytes; `+4` is the flags dword. |
| `0x4EABA4`, `0x4EABB0` | struct[12] | Two cluster-reference systems: `+0` per-cluster head array, `+4` cluster-to-object reference pool, `+8` object-to-cluster reference pool. Body flag bit 9 set selects `0x4EABB0`. Which one is "collideable" is **OPEN**. |
| `0x4ECC18` | u32 | `+0x9C` is the cluster count (BA630 bit-vector sizing). |

### 1.2 Object header element (12 bytes, `hdr(i)`)

| Off | W | Meaning (as used here) |
|---|---|---|
| +0x00 | 2 | Salt (not checked here). |
| +0x02 | 1 | Flags:<br>`0x02` marked for update (set by B9390).<br>`0x10` deleted, deletion pending (set by BA570; tested by BBD10 and C0F60).<br>`0x40` connected-to-map mirror (set by BA630, cleared by BA7E0). |
| +0x03 | 1 | Object type copy. BA570 tests `(1<<type) & 0x40`. |
| +0x04 | 2 | Cached cluster index. BA630 writes it; `0xFFFF` means none. |
| +0x08 | 4 | Body (datum) pointer. |

### 1.3 Object body fields touched

| Off | W | Use |
|---|---|---|
| +0x00 | 4 | Definition tag handle (E24F0). |
| +0x04 | 4 | Flags:<br>bit 0 (byte & 1): skip in C0F60/BBD10.<br>bit 4 `0x10`: has attachment descriptors (C0F60).<br>bit 6 `0x40`: call C3750 (C0F60).<br>**bit 8 `0x100`: connected to map** (B9460 tests it, BA7E0 clears it, BA630 sets it).<br>bit 9 `0x200`: cluster system select.<br>bit 17 `0x20000`: BA630 runs the B1870 check.<br>bit 18 `0x40000`: BA630 "no cluster" flag.<br>bit 21 `0x200000`: BA630 uses the all-clusters bit vector. |
| +0x0C | 4 | Next sibling. |
| +0x10 | 4 | First child. |
| +0x14 | 4 | Parent. NONE means a root, which gets the world-bounds check. |
| +0x28 | 4 | Scenario location dword 0 (BA630). |
| +0x2C | 4 | Scenario location dword 1; its low word is the cluster index (BA630). |
| +0x30 / +0x3C | 12 / 4 | Bounding sphere center / radius (BFF20). |
| +0x40 / +0x4C | 12 / 4 | Cluster sphere center / radius (BFF20, BA630). |
| +0x60 | 4 | Head of this object's cluster-reference chain (BA7E0 through 1D30F0, BA630 through 1D2F10). |
| +0x64 | 12 | Position (B9460). Interpreted as parent-relative when +0x14 != NONE (bounds check skipped). |
| +0x70 | 12 | Forward (B9460). |
| +0x7C | 12 | Up (B9460). |
| +0xAA | 1 | Object type: 0 bipd, 1 vehi, 2 weap, 3 eqip, 4 garb, 5 proj, 6 scen, 7 mach, 8 ctrl, 9 lifi, 10 ssce, 11 bloc (crate), 12 crea. |
| +0xB4 | 4 | Physics component index (1CD620). |
| +0xD4 | 4 | Simulation entity index, or NONE (B95D0, B9460 to B7E50). |
| +0x116 | 2 | Node array offset (not used directly here). |
| +0x11C / +0x11E | 2 / 2 | Attachment descriptor byte size / offset (signed 16, C0F60). |
| +0x3DC | 1 | Biped byte; E24F0 skips if it equals 3. |

---

## 2. How native code enters B95D0

### 2.1 The native caller

`src/physics/physics_live_world_tu15.cpp`:

```cpp
// :7019
void PhysicsLiveWorldTu15::set_object_position(std::uint32_t o,Vec3 p){Impl::PublicCall active(*impl_);impl_->set_object_position(o,p);}
// :4532
void set_object_position(std::uint32_t object,Vec3 position) {
    if(!session_configured || !word(bank,0x52be48u) ||
        !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
        throw FormatError("Object position input requires a configured source world and finite point");
    object_body(object);                         // :3498, live header + non-null body, else FormatError
    constexpr std::uint32_t point=0x60060000u;   // scratch DATA in the bank
    for(unsigned n=0;n<3;++n)put(bank,point+n*4u,bit_cast<u32>(position[n]));
    const std::array args{object,0u,0u};auto a=frame(args);a.gp[0]=point;
    run_actual(0xb95d0u,a);
}
```

So the call is B95D0 with:

- EAX = `0x60060000` (position)
- ECX = 0 and EDX = 0 (no forward or up)
- stack `{object, location=0, physics_force=0}`

The effective B9460 call is therefore `B9460(obj, &local_p, 0, 0, 0, 1, 1, 0, 0)`.

- `frame(args)` (`:3163`) starts from `caller_abi()` (`:3158`): all GP registers are 0, `ESP = stack` (or the nested caller frame), `EFLAGS = 2`, `fs_base = tls`, and `floating_point = fp`.
- It writes the sentinel return `0x60300000` at `[ESP]` and the arguments at `[ESP+4..]`.
- `PublicCall` (`:614`) enforces the executor thread and no publication in progress.
- The only production caller is the presented-pose scope:
  - `src/rendering/shadows/source_presented_model_pose_tu15.cpp:357` calls `world.set_object_position(owner,pose.root.position)` for every presented **root** (`body+0x14 == NONE`).
  - It skips the call, and writes `+0x64` directly, for "unmoved" roots: no child (`+0x10 == NONE`), `+0x11C == 0`, and position/forward/up/scale all within 1e-4. The env var `HALO2_REFERENCE_PRESENTED_RELOCATION=1` disables the skip.
  - Afterwards it writes `+0x70`, `+0x7C`, `+0xA0`, `+0x30` and `+0x3C` itself.
  - A write observer journals every store outside `0x60000000..0x60100000` and rolls it back at scope end.

### 2.2 How native code invokes translated routines

- `call_actual(entry,args)` (`:3309`) is `frame(args)`, then `run_actual(entry,a)`, then returns EAX.
- `run_actual(entry,a,o,composed)` (`:3390`) does the following, in order:
  1. Marks the root (`source_execution_untracked`, `Execution`) and checks the thread.
  2. Has fast paths for `0x156480` and render-ownership entries.
  3. Installs RAII bindings: attachment events, vehicle input, fade/status phases, lift/rate/schema consumers, contact-feature, simplex and vehicle-flip handoffs, and object-velocity handoff.
  4. Installs a `SourcePhysicsEntryObservation`.
  5. Calls `required(bank,{entry,entry,false},a,actual_store_observer)`.
- `Impl::required` (`:1584`) handles target-specific intercepts; none apply to B95D0/B9460/BA570/B9390. The general case calls `required_entry` (call at `:1796`; definition at `:1860`, whose only intercept is the hitscan path for FDEB0/16C620), which calls `required_entry_original` (`:1896`..`:3157`).
- `required_entry_original` is an ordered chain of `source_*_owns_entry(target)` tests. **The first owner wins.** It calls `source_run_<unit>(m,target,a,fp,services(),o)`, which builds a `LiveState`, runs `s.load(abi)`, switches on the entry, and saves back.
- Inside a generated unit:
  - A static call is `s.push(ret);run_X(s);`.
  - A call to a routine outside the unit is `dispatch(s,target,site,tail)`. Its `default:` arm is `s.required(site,target,tail)`.
  - `s.required` goes to `services.required_owner`, which is the same `Impl::required` (set at `:1437`).
  - Each such hop saves and reloads the full ABI: GP registers, flags, x87, XMM and FP control.

### 2.3 Effective routing for the B95D0 tree (verified by owner order)

| Target | First owner in `required_entry_original` | Line |
|---|---|---|
| `0xB95D0` | `source_death_cycle_runtime_tu15_owns_entry`. The live-support unit also owns it, but later. | `:2691` (live support at `:2843`) |
| `0xB9460` | `source_physics_live_support_tu15_owns_entry` (the only owner) | `:2843` |
| `0xBA570` | `source_physics_live_support_tu15_owns_entry` (the only owner) | `:2843` |
| `0x10E340` | death cycle and live support own it, but both B95D0 copies call it statically | n/a |
| `0xB9390` | `source_scene_device_constructor_tu15_owns_entry`, which runs the translated B9390 at `source_scene_device_constructor_tu15_generated.cpp:1671` | `:2711` |

- The native `native_campaign_unit_action_support_tu15_owns_call(0xB9390)` at `:2918` and `source_ai_actor_tu15_owns_entry` at `:2972` come after the scene-device owner. They are therefore never used for 0xB9390.
- The death-cycle B95D0 (`death_cycle_runtime_tu15_generated.cpp:948-1026`, 76 instructions) has the same instruction sequence as the live-support copy. Differences:
  - It calls `run_10e340` statically (its own copy at `:8089`).
  - It dispatches 0xB9460, 0xBA570 and 0xB9390 through its `dispatch`. None of these are in that unit, so they go to `s.required`.
  - Its `selected()` remap is the identity for TU1.5.

---

## 3. B95D0: relocation driver

- Range `0xB95D0..0xB969E`.
- Translated copies:
  - `physics_live_support_tu15_generated.cpp:8112-8190`, **76 instructions**.
  - `death_cycle_runtime_tu15_generated.cpp:948-1026`, 76 instructions.
- Older-image counterpart: B8490 (`docs/DAMAGE_FATAL_ENTITY_CLEANUP_HANDOFF.md:117`).

### 3.1 Calling convention (custom, LTCG)

| In | Meaning |
|---|---|
| EAX | `const f32 *position` (nullable) |
| ECX | `const f32 *forward` (nullable) |
| EDX | `const f32 *up` (nullable) |
| `[E+0x04]` | object index |
| `[E+0x08]` | `const void *location`: 8-byte scenario location (nullable). Forwarded as B9460 arg5, then to BA630. |
| `[E+0x0C]` | `physics_force` dword. Forwarded as B9460 arg8, then to 1CD620. Only the low byte matters there. |

- Return: `ret 0Ch`. EBX, EBP, ESI and EDI are preserved. There is no meaningful result.
- Volatile outputs are whatever B9390 leaves:
  - EAX = the object index if B9390 exits early (header bit `0x02` already set, or object == NONE), otherwise `0xFFFFFFFF`.
  - ECX = `rows`.
  - EDX = `(object & 0xFFFF)*3`.
  - No consumer was seen. A full audit is **OPEN**.
- Frame: `sub esp,24h`, then push EBX, EBP, ESI, EDI. Saved registers are at `E-0x28`, `E-0x2C`, `E-0x30`, `E-0x34`.
- Locals:
  - `pos` at `E-0x24` (x), `E-0x20`, `E-0x1C`
  - `fwd` at `E-0x18`, `E-0x14`, `E-0x10`
  - `up` at `E-0x0C`, `E-0x08`, `E-0x04`

### 3.2 Pseudo-C (every branch)

```c
void B95D0(const f32 *position /*EAX*/, const f32 *forward /*ECX*/, const f32 *up /*EDX*/,
           u32 object, const void *location, u32 physics_force)
{
    f32 *pp = 0, *pf = 0, *pu = 0;          // EBP, ESI, EBX (zeroed by xor at B95D6/B95D8/B95DA)
    if (position) {                         // test eax,eax @B95DC
        loc_pos[0] = position[0];           // raw 32-bit word copies (no FP op)
        loc_pos[1] = position[1];
        loc_pos[2] = position[2];
        pp = loc_pos;                       // lea ebp,[esp+10h] = E-0x24
    }
    if (forward) {                          // @B95FD
        loc_fwd[0..2] = forward[0..2];      // pf = E-0x18
        pf = loc_fwd;
    }
    if (up) {                               // @B9619
        loc_up[0..2] = up[0..2];            // pu = E-0x0C
        pu = loc_up;
    }
    // 10E340: EDI = object, EBX = pu (implicit), stack (pp, pf); ret 8.
    // Handlers may rewrite loc_pos/loc_fwd/loc_up through these pointers.
    B10E340(object, pp, pf, pu);
    u8 ok = B9460(object, pp, pf, pu, location, /*rebuild_nodes*/1, /*update_physics*/1,
                  physics_force, /*keep_links*/0);           // only AL tested @B9655
    if (!ok) {
        u8 mode = *(u8*)(*(u32*)0x4F3218 + 0x0C);            // @B9659/@B965F
        if (mode != 4 ||
            *(u32*)(body(object) + 0xD4) == NONE)            // @B9665..@B9688 (reads 0x4ECBD0 again)
            BA570(object);                                   // stdcall 1 arg, ret 4
    }
    B9390(object /*EAX*/);                                   // via dispatch -> required(), plain ret
}
```

Notes:

- `physics_force` is read at `[esp+40h]` and `location` at `[esp+3Ch]`, both after 10E340 returns. The mode-4 test is a byte compare.
- The world-bounds result only matters for roots (B9460 skips the check when `+0x14 != NONE`). It is also only possible when `position != 0`.

### 3.3 Memory stores

| Store PC | Address | W | Value | Condition |
|---|---|---|---|---|
| B95E7 / B95F1 / B95F5 | E-0x24 / E-0x20 / E-0x1C | 4 | position[0] / [1] / [2] | position != 0 |
| B9603 / B960D / B9611 | E-0x18 / E-0x14 / E-0x10 | 4 | forward[0..2] | forward != 0 |
| B9625 / B9629 / B962D | E-0x0C / E-0x08 / E-0x04 | 4 | up[0..2] | up != 0 |

Read and store order:

- Position: read x, store x, read y, read z, store y, store z.
- Forward: read x, store x, read y, read z, store y, store z.
- Up: read x, y, z, then store x, y, z.

Push stores (stack residue; a fully native replica may elide them, see section 12):

| PC | Address | Value |
|---|---|---|
| B95D3 | E-0x28 | EBX |
| B95D4 | E-0x2C | EBP |
| B95D5 | E-0x30 | ESI |
| B95DE | E-0x34 | EDI |
| B9635 | E-0x38 | pf |
| B9636 | E-0x3C | pp |
| B9637 | E-0x40 | 0xB963C (10E340 return address) |
| B9644 | E-0x38 | 0 |
| B9646 | E-0x3C | physics_force |
| B9647 | E-0x40 | 1 |
| B9649 | E-0x44 | 1 |
| B964B | E-0x48 | location |
| B964C | E-0x4C | pu |
| B964D | E-0x50 | pf |
| B964E | E-0x54 | pp |
| B964F | E-0x58 | object |
| B9650 | E-0x5C | 0xB9655 |
| B968A | E-0x38 | object (out-of-bounds path only) |
| B968B | E-0x3C | 0xB9690 (out-of-bounds path only) |
| B9692 | E-0x38 | 0xB9697 |

### 3.4 Calls

| Site | Callee | Arguments | Condition |
|---|---|---|---|
| B9637 | 10E340 | EDI=object, EBX=pu, stack (pp, pf); `ret 8` | always |
| B9650 | B9460 | (object, pp, pf, pu, location, 1, 1, physics_force, 0); `ret 24h` | always |
| B968B | BA570 | (object); `ret 4` | `!AL && (mode != 4 \|\| body->+D4 == NONE)` |
| B9692 | B9390 | EAX=object; plain `ret`; through `dispatch` | always |

Globals read: `0x4F3218` (and its `+0x0C`) and `0x4ECBD0` (rows, then `body+0xD4`), both only on the out-of-bounds path.

Callees reached: 10E340 (then E24F0, then 30BD0), B9460 (full tree in section 4), BA570 (and its tree), B9390.

---

## 4. B9460: per-object relocation worker

- Range `0xB9460..0xB95C5`.
- Translated: `physics_live_support_tu15_generated.cpp:7985-8110`, **123 instructions**. This is the only translated copy.
- Older-image counterpart: B8320.

### 4.1 Calling convention

`stdcall`, 9 dword stack arguments, `ret 24h`.

| Slot | Name | Meaning |
|---|---|---|
| `[E+0x04]` | object | Object index |
| `[E+0x08]` | position | `const f32*`, nullable |
| `[E+0x0C]` | forward | `const f32*`, nullable |
| `[E+0x10]` | up | `const f32*`. Read only when forward != 0. **Not null-checked.** |
| `[E+0x14]` | location | Forwarded only to BA630 |
| `[E+0x18]` | rebuild_nodes | Byte. Nonzero calls BF050. |
| `[E+0x1C]` | update_physics | Byte. Nonzero calls 1CD620. |
| `[E+0x20]` | physics_force | Dword, pushed as the second stack argument of 1CD620 (a byte there) |
| `[E+0x24]` | keep_links | Byte. Nonzero skips both BA7E0 and BA630. |

Result:

- AL = `position_ok` (1 unless a root position failed the bounds check; 1 when position == 0).
- EAX bits 8..31 are residue from the last callee or computation, so they depend on the path.
- ECX on return = the local slot popped at B95C4, which is `(ECX_at_entry & 0x00FFFFFF) | (position_ok << 24)`.
- EBX, EBP, ESI and EDI are preserved.
- XMM0, XMM1 and XMM2 are clobbered on the bounds path; the callees clobber more.

Frame:

- `push ecx` creates the local slot `E-4..E-1`; byte `E-1` = `position_ok`.
- Then push EBX (`E-8`), EBP (`E-0xC`), ESI (`E-0x10`), EDI (`E-0x14`).

Values held in registers:

- EBP = `objects`, loaded once at B9467.
- ESI = `(object & 0xFFFF)*12` until it is reused at B9558.
- EDI = `body`, loaded once at B947F. At B9544 it becomes `body+0x7C` for the up store.
- BL = `(body->+4 >> 8) & 1`, computed at B9483..B9489 **before** BA7E0.

### 4.2 Pseudo-C (every branch)

```c
u8 __stdcall B9460(u32 object, const f32 *position, const f32 *forward, const f32 *up,
                   const void *location, u8 rebuild_nodes, u8 update_physics,
                   u32 physics_force, u8 keep_links)
{
    u8 *position_ok = (u8*)(E - 1);               // inside the slot pushed from ECX
    u32 off  = (object & 0xFFFF) * 12;
    u32 body = *(u32*)(*(u32*)(objects + 0x44) + off + 8);
    u8 connected = (*(u32*)(body + 4) >> 8) & 1;  // BL
    *position_ok = 1;                             // @B948C
    if (connected && !keep_links)                 // @B9491 / @B9493 re-reads [E+24h]
        BA7E0(object);                            // stdcall 1 arg

    if (position) {                               // @B94A1..A7
        if (*(u32*)(body + 0x14) == NONE) {       // root only (@B94A9)
            // COMISS sequence; CF=1 (below or unordered) => out of bounds.
            f32 lo = *(f32*)0x46AA04, hi = *(f32*)0x46A530;
            if (!(position[0] >= lo)) goto oob;   // comiss x,lo  jb  (@B94BB/BE)
            if (!(hi >= position[0])) goto oob;   // comiss hi,x  jb  (@B94C8/CB), x re-read from memory
            if (!(position[1] >= lo)) goto oob;   // @B94D2/D5
            if (!(hi >= position[1])) goto oob;   // @B94D7/DA
            if (!(position[2] >= lo)) goto oob;   // @B94E1/E4
            if (!(hi >= position[2])) goto oob;   // @B94E6/E9 (jnb -> store)
        }
        *(u32*)(body + 0x64) = ((u32*)position)[0];   // @B94F7
        *(u32*)(body + 0x68) = ((u32*)position)[1];   // @B94FC
        *(u32*)(body + 0x6C) = ((u32*)position)[2];   // @B9502
        u32 ent = *(u32*)(*(u32*)(*(u32*)(objects + 0x44) + off + 8) + 0xD4);  // body re-read
        if (ent != NONE) B7E50(ent, 2);           // @B951A
        goto orientation;
oob:    *position_ok = 0;                         // @B94EB, position NOT stored
    }
orientation:
    if (forward) {                                // @B9523..29
        *(u32*)(body + 0x70) = ((u32*)forward)[0];    // @B9530
        *(u32*)(body + 0x74) = ((u32*)forward)[1];    // @B9535
        *(u32*)(body + 0x78) = ((u32*)forward)[2];    // @B953B
        *(u32*)(body + 0x7C) = ((u32*)up)[0];         // @B9547 (up not null-checked)
        *(u32*)(body + 0x80) = ((u32*)up)[1];         // @B954C
        *(u32*)(body + 0x84) = ((u32*)up)[2];         // @B9552
        u32 ent = *(u32*)(*(u32*)(*(u32*)(objects + 0x44) + off + 8) + 0xD4);  // body re-read
        if (ent != NONE) B7E50(ent, 4);           // @B956A
    }
    if (update_physics)                           // @B9573
        B1CD620(object /*EAX*/, /*position_was_null*/ position == 0, physics_force);
    if (rebuild_nodes)                            // @B958F
        BF050(object);                            // stdcall 1 arg
    if (connected && !keep_links)                 // BL from entry; [E+24h] re-read @B95A9
        BA630(object, location);                  // stdcall 2 args (object, location)
    return *position_ok;                          // mov al,[esp+3] @B95C0
}
```

The bounds check is equivalent to requiring every one of x, y, z to be ordered and inside `[-32768, 32768]`. NaN goes to `oob`. The comparisons short-circuit in the order x>=lo, x<=hi, y>=lo, y<=hi, z>=lo, z<=hi. This order matters for MXCSR sticky flags (section 12).

In the out-of-bounds case, 1CD620 still runs (if `update_physics`) with `position_was_null = 0`, and the body still holds its old position.

### 4.3 Memory stores

| Store PC | Address | W | Value | Condition |
|---|---|---|---|---|
| B9460 | E-0x04 | 4 | ECX at entry (push) | always |
| B9465 / B9466 / B9474 / B947E | E-0x08 / -0x0C / -0x10 / -0x14 | 4 | EBX / EBP / ESI / EDI | always |
| B948C | E-0x01 | 1 | 1 | always |
| B949B / B949C | E-0x18 / E-0x1C | 4 | object / 0xB94A1 | BA7E0 call |
| B94EB | E-0x01 | 1 | 0 | position out of bounds |
| B94F7 / B94FC / B9502 | body+0x64 / +0x68 / +0x6C | 4 | position words | position stored |
| B9517 / B9519 / B951A | E-0x18 / -0x1C / -0x20 | 4 | 2 / ent / 0xB951F | B7E50 call (position) |
| B9530 / B9535 / B953B | body+0x70 / +0x74 / +0x78 | 4 | forward words | forward != 0 |
| B9547 / B954C / B9552 | body+0x7C / +0x80 / +0x84 | 4 | up words | forward != 0 |
| B9567 / B9569 / B956A | E-0x18 / -0x1C / -0x20 | 4 | 4 / ent / 0xB956F | B7E50 call (orientation) |
| B9584 / B9585 / B958A | E-0x18 / -0x1C / -0x20 | 4 | physics_force / EAX (AL = position==0, bits 8..31 residue) / 0xB958F | 1CD620 call |
| B959B / B959C | E-0x18 / -0x1C | 4 | object / 0xB95A1 | BF050 call |
| B95B9 / B95BA / B95BB | E-0x08 / -0x0C / -0x10 | 4 | location / object / 0xB95C0 | BA630 call (after the 4 pops, ESP = E-4) |

### 4.4 Calls

| Site | Callee | Arguments | Condition |
|---|---|---|---|
| B949C | BA7E0 | (object); `ret 4` | connected && !keep_links |
| B951A | B7E50 | (body->+D4, 2); `ret 8` | position stored && +D4 != NONE |
| B956A | B7E50 | (body->+D4, 4); `ret 8` | forward != 0 && +D4 != NONE |
| B958A | 1CD620 | EAX=object; stack (u8 position==0, physics_force); `ret 8` | update_physics |
| B959C | BF050 | (object); `ret 4` | rebuild_nodes |
| B95BB | BA630 | (object, location); `ret 8` | connected && !keep_links |

Globals read: `0x4ECBD0`, `0x46AA04` and `0x46A530` (roots only).

Callees reached: BA7E0 (C0F60, 1D30F0, 16E8A0), B7E50, 1CD620 (1DC270, then BF050 again and indirect Havok), BF050 (BF0C0, BFF20, which can call BA7E0/BA630 itself), BA630 (11E600, BA910, 1D2F10, C0F60, B8D90, B92C0, B1870, BA570, B9330, 1496F0, 1CC290, 282C10).

---

## 5. BA7E0: disconnect object from map clusters

- Range `0xBA7E0..0xBA841`.
- Translated: `physics_live_support_tu15_generated.cpp:9347-9386`, **37 instructions**.
- Convention: `stdcall (u32 object)`, `ret 4`. EBX, EBP, ESI and EDI are preserved. No result. AL ends as `hdr+2 & 0xBF`.

```c
void __stdcall BA7E0(u32 object)
{
    u32 rows = *(u32*)(*(u32*)0x4ECBD0 + 0x44);
    u32 hdr  = rows + (object & 0xFFFF) * 12;        // EBX, captured before the calls
    u32 body = *(u32*)(hdr + 8);                     // ESI, captured before the calls
    C0F60(object, 1, 0, 0, 0);                       // stdcall 5 args, ret 14h (@BA80B)
    u32 system = ((*(u32*)(body + 4) >> 9) & 1) ? 0x4EABB0 : 0x4EABA4;  // flags re-read
    B1D30F0(object, body + 0x60) /* EDI = system */; // ret 8 (@BA829)
    *(u32*)(body + 4) &= 0xFFFFFEFF;                 // @BA82E, clears "connected" (RMW, fresh read)
    *(u8*)(hdr + 2)   &= 0xBF;                       // @BA83D, clears header 0x40 (RMW, fresh read)
}
```

Stores:

- Push stores:

  | PC | Address | Value |
  |---|---|---|
  | BA7E9 | E-4 | EBX |
  | BA7EA | E-8 | EBP |
  | BA7EF | E-0xC | ESI |
  | BA7F0 | E-0x10 | EDI |
  | BA7F1 | E-0x14 | 0 |
  | BA7F3 | E-0x18 | 0 |
  | BA7FC | E-0x1C | 0 |
  | BA805 | E-0x20 | 1 |
  | BA80A | E-0x24 | object |
  | BA80B | E-0x28 | 0xBA810 |
  | BA827 | E-0x14 | body+0x60 |
  | BA828 | E-0x18 | object |
  | BA829 | E-0x1C | 0xBA82E |

- Data stores: BA82E (4 bytes) and BA83D (1 byte), as shown in the code.

Globals read: `0x4ECBD0`.

Callees reached: C0F60 (section 9.5: BBD10, C5250 per attachment, C3750, recursion) and 1D30F0 (section 9.6: 16E8A0).

---

## 6. B7E50: simulation entity dirty mark

- Range `0xB7E50..0xB7E9A`.
- Translated: `physics_live_support_tu15_generated.cpp:7030-7054`, **22 instructions**.
- Older-image counterpart: B6760.
- Convention: `stdcall (u32 entity, u32 mask)`, `ret 8`. Only EAX, ECX and EDX are written. EAX/ECX/EDX on return are path-dependent.

```c
void __stdcall B7E50(u32 entity, u32 mask)
{
    u32 sim  = *(u32*)0x4DC04C;
    i32 mode = *(i32*)(sim + 8);
    if (mode != 4 && mode != 5) return;              // @B7E59..61
    if (mode == 3 || mode == 5) return;              // @B7E63..6B, so only mode 4 continues
    u32 state = *(u32*)(sim + 4);
    u32 idx   = entity & 0x3FF;
    u32 table = *(u32*)(state + 0x20A4);
    if (!(*(u8*)(table + idx*8 + 0x44) & 4)) return; // @B7E85
    *(u32*)(state + 0x20B8 + idx*0x24) |= mask;      // @B7E97: eax=state+0x2098, +idx*36+0x14, then +0xC
}
```

Globals read: `0x4DC04C`. Callees: none.

---

## 7. B9390: mark object and ancestors for update

- Range `0xB9390..0xB93DB`.
- **26 instructions**.
- Not translated in the live-support or death-cycle units. Translated copies:
  - `source_scene_device_constructor_tu15_generated.cpp:1671-1699` (the one used at runtime, section 2.3)
  - `source_ai_actor_tu15_generated.cpp:518-546`
  - `source_ai_frame_tu15_generated.cpp:2441-2469`
- Older-image counterpart: B8250.
- Convention: EAX = object, plain `ret`. EBX and ESI are preserved.

```c
void B9390(u32 object /*EAX*/)
{
    u32 objects = *(u32*)0x4ECBD0;                   // ESI
    u32 rows = *(u32*)(objects + 0x44);              // ECX
    if (*(u8*)(rows + (object & 0xFFFF)*12 + 2) & 2) return;  // read happens even for NONE
    if (object == NONE) return;
    do {
        u32 off = (object & 0xFFFF) * 12;
        *(u8*)(rows + off + 2) |= 2;                 // @B93C8
        rows = *(u32*)(objects + 0x44);
        object = *(u32*)(*(u32*)(rows + off + 8) + 0x14);   // parent
    } while (object != NONE);
}
```

Register outputs: EAX = NONE after the loop, or the original object on early exit; ECX = rows; EDX = `(object0 & 0xFFFF)*3`.

---

## 8. 10E340: object-type relocation notification (and E24F0)

- Range `0x10E340..0x10E3A1`.
- **35 instructions**.
- Copies: live support `:62430-62467`, death cycle `:8089-8126`.
- Convention (custom):
  - EDI = object.
  - EBX = the up pointer, passed through to the handlers but not set by 10E340 itself (it inherits B95D0's EBX).
  - Stack: `[E+4]` position pointer, `[E+8]` forward pointer.
  - `ret 8`. EBP and ESI are preserved.

```c
void B10E340(u32 object /*EDI*/, f32 *pos, f32 *fwd, f32 *up /*EBX*/)
{
    u32 body = *(u32*)(*(u32*)(*(u32*)0x4ECBD0 + 0x44) + (object & 0xFFFF)*12 + 8);
    u32 def  = *(u32*)(0x474B90 + (i32)(i8)*(u8*)(body + 0xAA) * 4);   // movsx
    u32 *chain = (u32*)(def + 0x84);                 // NULL-terminated list of definitions
    if (!chain[0]) return;
    do {
        u32 handler = *(u32*)(chain[0] + 0x68);
        if (handler)                                 // stdcall (object, pos, fwd, up), ret 10h
            handler(object, pos, fwd, up);           // through dispatch(target = EAX)
        ++chain;
    } while (chain[0]);
}
```

Stores (push only): 10E372 EBP, 10E379 ESI. For each handler: 10E38D EBX, 10E38E EBP (fwd), 10E38F EDX (pos), 10E390 EDI, 10E391 return address 0x10E393.

**TU1.5 handler table:** read from the mounted definitions in `src/gameplay/objects/native_tu15_object_storage_data.cpp`.

- Every chain starts with `obje` (`0x473E48`). Examples:
  - biped = [obje, unit `0x473F10`, bipd `0x473FD8`]
  - weapon = [obje, item `0x474168`, weap]
  - machine = [obje, devi `0x4747A8`, mach]
- `+0x68` is zero for every definition except **bipd: `0xE24F0`**. Only type 0 reaches a handler.
- A native replica should still walk the runtime table. Alternatively it can assert that the table matches and fall back if it does not.

### 8.1 E24F0: biped relocation handler

- **60 instructions**.
- Copies: live support `:35369-35431`, death cycle `:3816`.
- Convention: `stdcall (object, pos, fwd, up)`, `ret 10h`. ESI is preserved.

```c
void __stdcall E24F0(u32 object, f32 *pos, f32 *fwd, f32 *up)
{
    u32 body = body(object);
    if (*(u32*)(body + 0x14) != NONE) return;                    // only roots
    u32 tagdef = *(u32*)(*(u32*)0x4F0414 + (*(u32*)body & 0xFFFF)*16 + 8);
    if ((*(u32*)(tagdef + 0x264) >> 3) & 1) return;              // biped definition flag bit 3
    if (*(u8*)(body + 0x3DC) == 3) return;
    if (up) {
        f32 *gu = *(f32**)0x474D10;
        if (!(up[2] == gu[2]))                                   // ucomiss; unordered counts as not equal
            { up[0]=gu[0]; up[1]=gu[1]; up[2]=gu[2]; }           // @E255E/@E2563/@E2569 (raw words)
    }
    if (fwd && !(fwd[2] == 0.0f)) {                              // ucomiss vs xorps 0
        fwd[2] = +0.0f;                                          // @E2588
        f32 len = B30BD0(fwd);                                   // normalize in place, EAX=fwd, length in XMM0
        if (len == *(f32*)0x46A0E8 /*0.0f*/)                     // equal and ordered
            { f32 *gf = *(f32**)0x474D08; fwd[0..2] = gf[0..2]; } // @E25A6/@E25AB/@E25B1
    }
}
```

`pos` is not used. On the set_object_position path (forward = up = 0), the handler only reads memory.

---

## 9. Other callees in the B95D0/B9460 closure

### 9.1 BA570: object delete (deferred)

- **60 instructions**, `:9141-9203`.
- Convention: `stdcall (object)`, `ret 4`.
- Older-image counterpart: B9430. Documentation calls it "marks header bit10 for deletion".

```c
void __stdcall BA570(u32 object)
{
    hdr = hdr(object); body = body(object);              // EDI, EBP
    if (!BBD10(object /*ECX*/) && BBD50(object /*EAX*/)) // "object or ancestor deleting", "root attached"
        C0F60(object, 1, 0, 0, 0);
    *(u8*)(hdr + 2) |= 0x10;                             // @BA5B5
    BABA0();                                             // registers: EDI = hdr, EBP = body (OPEN)
    if ((1u << *(u8*)(hdr + 3)) & 0x40) B10F300(object); // type 6 only
    C10C0(/*EDI*/ object);
    *(u8*)(body + 0xAB)  = 0xFF;                         // @BA5D9
    *(u16*)(body + 0xA8) = 0xFFFF;                       // @BA5E0
    *(u32*)(body + 0xA4) = NONE;                         // @BA5E9
    if (*(u8*)(body + 0xAF) != 0xFF) B15DC40(/*EAX*/ object, (i32)(i8)*(u8*)(body + 0xAF));
    B10E4B0(object);                                     // reaches B9390
    BD980(/*EAX*/ object, 0, NONE);
    B1C90(/*EAX*/ object);
    B10FD90(/*EDX*/ object);
}
```

A native version already exists: `native_tu15_delete_object` (section 11).

### 9.2 1CD620: physics component transform update gate

- **40 instructions**, `:114107-114149`.
- Convention: EAX = object; stack `[E+4]` u8 `position_was_null`, `[E+8]` u8 `physics_force`; `ret 8`. ESI is preserved.

```c
void B1CD620(u32 object /*EAX*/, u8 position_was_null, u8 physics_force)
{
    u32 comp = *(u32*)(body(object) + 0xB4);
    if (comp == NONE) return;
    u32 rec = *(u32*)(*(u32*)0x52BE5C + 0x44) + (comp & 0xFFFF) * 160;
    u32 f = *(u32*)(rec + 4);
    if ((f & 0x002) && position_was_null) return;
    if (f & 0x100) return;
    if ((f & 0x080) && !physics_force) return;
    B1DC270(comp);                   // stdcall 1 arg, ret 4 (@1CD68D)
}
```

Component flag names (bits 1, 7, 8) are **OPEN**.

### 9.3 BF050: node rebuild, recursive over children

- **35 instructions**, `:13625-13662`.
- Convention: `stdcall (object)`, `ret 4`. ESI and EDI are preserved.

```c
void __stdcall BF050(u32 object)
{
    u32 body = body(object);
    BF0C0(object);                   // stdcall 1, ret 4
    BFF20(object);                   // stdcall 1, ret 4
    for (u32 c = *(u32*)(body + 0x10); c != NONE; ) {
        u32 cb = body(c);
        if (!((1u << (*(u8*)(cb + 0xAA) & 31)) & 0x80))   // shl eax,cl; test al,al; js
            BF050(c);                                     // all types except 7 (machine)
        c = *(u32*)(cb + 0x0C);
    }
}
```

### 9.4 BA630: reconnect object to map clusters

- **138 instructions**, `:9205-9345`.
- Convention: `stdcall (object, location)`, `ret 8`. 0x60 bytes of locals. EBX, EBP, ESI and EDI are preserved.

```c
void __stdcall BA630(u32 object, const u32 *location)
{
    hdr = hdr(object); body = body(object);
    u8 was_unclustered = (*(u16*)(hdr + 4) == 0xFFFF);      // sete [E-5Dh] @BA65D
    const u32 *loc = location;
    if (!loc) {
        B11E600(/*EDX*/ body + 0x40, /*ESI out*/ local_loc);       // E-0x5C (8 bytes)
        if (*(u16*)(local_loc + 4) == 0xFFFF)
            B11E600(/*EDX*/ body + 0x64, /*ESI*/ local_loc);
        loc = local_loc;
    }
    u32 f;
    if (*(u16*)((u8*)loc + 4) != 0xFFFF) {
        *(u32*)(body + 0x28) = loc[0];                     // @BA68F
        *(u32*)(body + 0x2C) = loc[1];                     // @BA698
        *(u16*)(hdr + 4)     = (u16)loc[1];                // @BA69B
        f = *(u32*)(body + 4) & ~0x40000;
    } else f = *(u32*)(body + 4) | 0x40000;
    *(u32*)(body + 4) = f;                                 // @BA6BB
    [E+8] = 0;                                             // @BA6BE, overwrites own arg slot
    if ((f >> 21) & 1) {
        n = (((i32)(*(u32*)(*(u32*)0x4ECC18 + 0x9C) + 31)) >> 5) * 4;   // bytes
        memset32(E-0x40, 0xFFFFFFFF, n/4);                 // rep stosd @BA6E6, then n&3 bytes @BA6ED
        [E+8] = E-0x40;                                    // @BA6F3
    }
    BA910(/*EDX*/ object, /*EDI out*/ E-0x54);             // spatial packet
    u32 system = ((*(u32*)(body + 4) >> 9) & 1) ? 0x4EABB0 : 0x4EABA4;
    B1D2F10(/*EAX*/ [E+8], system, object, body+0x60, body+0x40, *(u32*)(body+0x4C),
            body+0x28, 0x14, E-0x54, /*&object arg slot*/ E+4);    // 9 stack args, ret 24h
    *(u32*)(body + 4) |= 0x100;                            // @BA752, connected
    *(u8*)(hdr + 2)   |= 0x40;                             // @BA760
    C0F60(object, 0, 1, 0, 0);
    if (B8D90(/*EAX*/ object, *(u32*)0x4F3218 + 0x11B8))  // stack 1 arg
        B92C0(/*EDI*/ object);
    else if (((*(u32*)(body + 4) >> 17) & 1) && !B1870(/*EAX*/ object))
        BA570(object);                                     // delete
    else
        B9330(/*EDX*/ object);
    if (was_unclustered && *(u16*)(hdr + 4) != 0xFFFF) {
        B1496F0(); B1CC290(object, object); B282C10(); B1496F0();   // 1496F0/282C10 through dispatch
    }
}
```

The names of the callees inside BA630 are tentative (**OPEN**). The control flow and arguments are transcribed from the translated body.

### 9.5 C0F60: detach/attach object tree (children + attachments)

- **97 instructions**, `:15489-15588`.
- Convention: `stdcall (object, a2, a3, a4, a5)` (bytes), `ret 14h`.
- A native version already exists: `native_tu15_detach_object_tree`. Its request field order is `{datum, detach_attachments=a2, delete_attachments=a3, follow_siblings=a4, current_only=a5}`.

```c
void __stdcall C0F60(u32 object, u8 a2, u8 a3, u8 a4, u8 a5)
{
  for (;;) {                                        // tail loop over siblings
    hdr = hdr(object); body = body(object); u8 bl;
    if (a5) { if ((*(u8*)(hdr+2) & 0x10) || (*(u8*)(body+4) & 1)) return; bl = 0; }
    else    { bl = BBD10(/*ECX*/ object); if (bl) return; }
    if ((*(u32*)(body+4) >> 4) & 1) {
        i32 cnt = (u32)(i32)*(i16*)(body+0x11C) >> 3;
        u32 d = body + (i32)*(i16*)(body+0x11E);
        for (; cnt > 0; --cnt, d += 8)
            if (*(u8*)d == 0 && *(u32*)(d+4) != NONE && !bl) {
                if (a2) C5250(/*EAX*/ *(u32*)(d+4));
                if (a3) C4D30(*(u32*)(d+4));        // re-read, stdcall 1
            }
    }
    if ((*(u32*)(body+4) >> 6) & 1) C3750(object, a2, a3);  // stdcall 3
    u32 child = *(u32*)(body+0x10);
    if (child != NONE) C0F60(child, a2, a3, 1, bl == 0);
    if (!a4) return;
    u32 sib = *(u32*)(body+0x0C);
    if (sib == NONE) return;
    object = sib;                                   // @C1069 stores into [E+4]
  }
}
```

Callers in this tree: BA7E0 and BA570 pass `(obj,1,0,0,0)`; BA630 passes `(obj,0,1,0,0)`. In BA630, which reconnects, `a3=1` drives C4D30, which the native header names `delete_attachment`. The semantic naming is **OPEN**: it looks like the "connect" half.

### 9.6 1D30F0: release object cluster references

- **50 instructions**, `:117480-117532`.
- Convention: EDI = system (`0x4EABA4` / `0x4EABB0`); stack `(object, u32 *head)`; `ret 8`. EBX, EBP and ESI are preserved.

```c
void B1D30F0(u32 object, u32 *head /*EDI = sys*/)
{
    u32 ref = *head;
    while (ref != NONE) {
        pool = *(u32*)(sys + 8);
        rec  = *(u32*)(pool+0x44) + (ref & 0xFFFF) * *(u32*)(pool+0x24);
        i16 cluster = *(i16*)(rec + 4);
        [E+4] = *(u32*)(rec + 8);                    // @1D311D, next ref (reuses the object arg slot)
        B16E8A0(/*EAX*/ ref, /*EDX*/ pool);          // datum delete, via dispatch
        u32 *slot = (u32*)(*(u32*)sys + cluster*4);
        if (*slot != NONE) {
            cpool = *(u32*)(sys + 4);
            for (;;) {
                crec = *(u32*)(cpool+0x44) + (*slot & 0xFFFF) * *(u32*)(cpool+0x24);
                if (*(u32*)(crec + 4) == object) {
                    u32 nx = *(u32*)(crec + 8);
                    B16E8A0(/*EAX*/ *slot, /*EDX*/ cpool);
                    *slot = nx;                      // @1D3165
                    break;
                }
                slot = (u32*)(crec + 8);
                if (*slot == NONE) break;
            }
        }
        ref = [E+4];
    }
    *head = NONE;                                    // @1D3176
}
```

### 9.7 Not specified here (internals OPEN)

- **BF0C0** (394 instructions, `stdcall(object)`, `ret 4`): node matrix and attachment rebuild. Callees: 10E180, 144CF0, 145560, 1707A0, 1DBBD0, B3AE0, B3B70, B8780, B87B0, BAC00, BFB90, BFC70, C15D0, C1CC0, C1E50, C1EB0, C1F00, C1F40, E3BC0.
- **BFF20** (202 instructions, `stdcall(object)`, `ret 4`):
  - Recomputes the bounding sphere (`+0x30`/`+0x3C`) and the cluster sphere (`+0x40..+0x4C`).
  - For type 11 with a physics component (`+0xB4 != NONE`) and `body+0xC0` bit 6, it queries 1DC7A0.
  - If the object is connected and the cluster sphere moved beyond `[0x46A560]` or its radius changed, it calls BA7E0, stores, then calls BA630(object, 0). When reached from B9460 with `keep_links = 0`, the object is already disconnected, so BFF20 only stores. The exact gating is **OPEN**.
- **BFB90** (82 instructions, `ret 0Ch`): called from BF0C0.
- **1DC270** (243 instructions, `stdcall(component)`, `ret 4`): Havok body transform update. It calls BF050 again, and through indirect calls it reaches the broadphase remove/add (2EC9B0/2EC4B0, per profiling). **OPEN**.

---

## 10. Translated call sites (would be served by a native replica)

Register columns: `&loc` = caller-local vector; `ptr` = pointer held in a register or frame slot; `G.fwd` / `G.up` / `G.pos` = `*[0x474D08]` / `*[0x474D10]` / `*[0x474CE8]`. In every listed site, arg3 (`physics_force`) is **0**.

### 10.1 B95D0: static `run_b95d0(s)` in the live-support unit (21 sites)

| Caller | Site | EAX pos | ECX fwd | EDX up | arg1 | arg2 location |
|---|---|---|---|---|---|---|
| BB910 | BBA68 | &loc | &loc | &loc | ESI | 0 |
| BE200 | BE292 | &loc | 0 | 0 | EDI | &loc (only when its cluster word != 0xFFFF) |
| C7940 | C7BCA | &loc | 0 | 0 | ESI (frame) | &loc |
| C7940 | C7BF6 | ptr (frame, non-null) | 0 | 0 | ECX | 0 |
| E4A40 | E4BB0 | &loc | 0 | 0 | ESI | 0 |
| E9E50 | EA29A | &loc | 0 | 0 | ECX | 0 |
| EDD90 | EDE23 | 0 | &loc | G.up | EBX | 0 (only when 30BD0 length > 0.0) |
| EE680 | EE76A | 0 | &loc | G.up | EBP | 0 (same gate) |
| EF5A0 | EF81E | 0 | &loc | &loc | ECX | 0 |
| F09C0 | F0B5E | &loc | &loc | G.up | EBP (frame) | 0 |
| FADC0 | FB705 | &loc | 0 | 0 | ESI | &loc |
| FC540 | FC634 | &loc | 0 | 0 | ESI (gated on FCEC0) | 0 |
| FDEB0 | FE4D2 | 0 | &loc | &loc | ECX (frame) | 0 |
| FDEB0 | FEF11 | &loc | &loc | &loc | ESI | &loc |
| 101330 | 101614 | &loc | 0 | 0 | EDI | 0 |
| 102560 | 102640 | ptr (frame) | 0 | 0 | ESI | ptr (frame) |
| 10EAB0 | 10ED71 | &loc | &loc | &loc | ECX (frame) | 0 |
| 10EAB0 | 10EE16 | &loc | 0 | 0 | ECX (frame) | 0 |
| 10F710 | 10F861 | G.pos | G.fwd | G.up | EDI | 0 |
| 151DA0 | 151EEB | &loc | &loc (fsin) | G.up | EDX | 0 |
| 204610 | 204673 | &loc | 0 | 0 | EBX (only when parent == NONE) | 0 |

### 10.2 B95D0: static `run_b95d0(s)` in the death-cycle unit (8 sites)

| Caller | Site | EAX | ECX | EDX | arg1 | arg2 |
|---|---|---|---|---|---|---|
| BB910 | BBA68 | as in 10.1 | | | | |
| BE200 | BE292 | as in 10.1 | | | | |
| 10EAB0 | 10ED71 | as in 10.1 | | | | |
| 10EAB0 | 10EE16 | as in 10.1 | | | | |
| E0B70 | E0F08 | 0 | &loc | ESI ptr | EBP | 0 |
| E6F70 | E707A | ESI ptr | 0 | 0 | EBX | 0 |
| FA8C0 | FAA23 | EBX+0x64 | EDI | ESI | EBP | EBX+0x28 (EBX is probably another body: its position and location; **OPEN**) |
| 1F5780 | 1F597D | &loc | &loc | &loc | ESI | 0 |

### 10.3 B95D0: `dispatch(0xB95D0)` in other units (routed to the death-cycle copy)

| Unit | Caller | Site | Notes |
|---|---|---|---|
| source_ai_actor | 11DDF0 | 11DFE4 | &loc ×3, arg1 = ESI |
| source_ai_frame | 11DC00 | 11DCFD | G.pos / G.fwd / G.up, arg1 = ESI |
| source_ai_frame | 11DDF0 | 11DFE4 | as ai_actor |
| source_campaign_activation | 14E1D0 | 14E38F | pos = ESI ptr, 0, 0, arg1 = EDI |
| source_ctf_activation | 160DA0 | 160E03 | pos = frame ptr, G.fwd, G.up, arg1 = ESI |
| source_damage_feedback | E3E20 | E3FA8 | 0, &loc, G.up, arg1 = ESI |
| source_teleporter | 1A6A40 | 1A6C60 | &loc, &loc, G.up, arg1 = ECX |
| source_combat_action | FDEB0 (FE4D2, FEF11), 101330 (101614), 102560 (102640), 10EAB0 (10ED71, 10EE16), BE200 (BE292), E9E50 (EA29A) | | duplicate translations of the 10.1 callers |

Every caller that passes ECX != 0 also passes EDX != 0, as B9460 requires.

### 10.4 B9460 call sites

| Caller | Site | Mechanism | Arguments (object, pos, fwd, up, location, rebuild, physics, force, keep_links) |
|---|---|---|---|
| B95D0 (live support) | B9650 | static | (obj, pp, pf, pu, location, 1, 1, force, 0) |
| B95D0 (death cycle) | B9650 | dispatch → required → live support | same |
| BAF10 | BB1CD | static | (EBP, &loc, ESI, EDI, 0, 1, 1, 0, 0) |
| BB300 | BB3C5 | static | (EBX, &loc, ESI, EDI, 0, 1, 1, 0, 0) |
| BB3E0 | BB7DF | static | (frame, &loc, ESI_old, EDI, 0, 1, 1, 0, 0) |
| BE610 | BE701 | static | (frame, &loc, EBX, EAX, 0, 0, 0, 0, **1**). BE610 then reads `[esp+13h]`, not AL. |

- BB200 runs immediately before B9460 in BAF10, BB300, BB3E0 and BE610.
- Whether fwd/up can be NULL in those four callers is **OPEN**.
- Native or kernel callers: only `run_actual(0xB95D0)` in `physics_live_world_tu15.cpp:4541`. No native code calls B9460.

---

## 11. Existing native replicas and mentions

Search: hex address in lowercase and uppercase, without `0x`, in `src/`, `include/` and `docs/`, excluding `*_generated*`. Hits in `.inc`/`.json` data blobs were checked and are false positives.

| Routine | Location | Kind |
|---|---|---|
| **B95D0** | none | No replica. Mentions:<br>`src/physics/physics_live_world_tu15.cpp:4541` (native caller, `run_actual`)<br>`include/halo2/physics_live_world_tu15.hpp:586`<br>`src/rendering/shadows/source_presented_model_pose_tu15.cpp:281,295,303` (comments; the skip logic is at :329-357)<br>`src/gameplay/multiplayer/nonhost_motion_adoption.cpp:221,269,274`<br>`include/halo2/nonhost_motion_adoption.hpp:8,32,134`<br>`include/halo2/native_tu15_object_motion.hpp:21`<br>Docs: NATIVE_REPLACEMENT_GOAL, NATIVE_4K60_SPLITSCREEN_GOAL, RENDER_THREAD_OFFLOAD_PLAN, DAMAGE_FATAL_ENTITY_CLEANUP_HANDOFF:117, DAMAGE_FEEDBACK_NEXT_SERVICES_HANDOFF:54, MP_SPARTAN_KILL_TRIGGER_RESEARCH:80, NETCODE_ENHANCED_GOAL:81,148, MULTIPLAYER_ENGINE_GOAL_CHECKLIST, GAME_MODE_PROGRESS, MP_CTF_ASSAULT_*, WEAPONS_TU15_PARITY_GOAL, SWORD_LUNGE_DAMAGE_TU15_20261025, MCC_GAMEPLAY_GOAL_CHECKLIST:666, PHYSICS_TU15_REMAINING_CHECKLIST:2291, CAMPAIGN_NATIVE_KERNEL_AUDIT_20261028. |
| **B9460** | none | No replica. Mentions: `source_presented_model_pose_tu15.cpp:295`; docs MP_SPARTAN_KILL_TRIGGER_RESEARCH:81, NATIVE_REPLACEMENT_GOAL:117, NATIVE_4K60_SPLITSCREEN_GOAL:869,948, RENDER_THREAD_OFFLOAD_PLAN:11,311, OPENING_CINEMATIC_STATUS:1915. |
| **BA7E0** | none | No replica. Mention: `docs/MP_GRENADE_SCENE_RESEARCH.md:45`. |
| **B7E50** | `src/physics/native_tu15_object_motion.cpp:372` `network_dirty` | **Full native replication** (generic mask; anonymous namespace, file-local). |
| | `src/campaign/ai/native_campaign_ai_orders_camouflage_tu15.cpp:12` `dirty` | **Full native**, specialized to mask `0x800000`, file-local. |
| | `include/halo2/native_tu15_object_deletion.hpp:12`, `src/physics/native_tu15_object_deletion.cpp:53` | Service hook (`actor_flags`) that the caller must supply. Not a replica. |
| | `src/gameplay/multiplayer/objective_native/activation.hpp:19,53`, `goal_queries.hpp:150`, `objects.hpp:73`, `plant.hpp:24,29,31`, `return_reset.hpp:60,64,70` | Register-level kernels that **call the translated** B7E50 through `dispatch`. |
| | `include/halo2/source_round_lifecycle.hpp:37`, `include/halo2/native_campaign_ai_orders_camouflage_tu15.hpp:6`, `include/halo2/native_tu15_object_motion.hpp:7`, `src/physics/physics_live_entries_tu15.hpp:669` (`{0xb7e50,0xb6760,Damage}`) | Mentions / address map. |
| **B9390** | `src/physics/native_tu15_activation_services.cpp:50` `native_tu15_wake_object_ancestors` (declared `include/halo2/native_tu15_activation_services.hpp:32-35`, store PC 0xB93C8) | **Full native**, exported. Best reuse candidate. |
| | `src/campaign/ai/native_campaign_unit_action_support_tu15.cpp:16` `native_campaign_unit_mark_update_tu15`, bound at `..._binding.cpp:8-24` | **Full native**. Its route at `required_entry_original:2918` is shadowed for 0xB9390 by the scene-device owner at `:2711`. |
| | `src/physics/native_tu15_object_motion.cpp:347` `propagate` | Full native, file-local. |
| | `src/gameplay/units/unit_action_dispatch.cpp:84` `mark_source_object_for_update` | Counterpart for an older image (store PC 0xB8288, different memory model). Not TU1.5. |
| | `src/gameplay/objects/source_region_transition_native_tu15_internal.hpp:129` | Register-level kernel that calls the translated B9390. |
| | `src/physics/physics_live_entries_tu15.hpp:15`, `include/halo2/native_tu15_object_scale.hpp:11`, several docs | Mentions. |
| **10E340** | none | No replica. Only `tools/` scripts mention it. |
| BA570 (callee) | `src/physics/native_tu15_object_deletion.cpp:201` `native_tu15_delete_object` | Full native with service hooks (C0F60, BA910, 1D30A0, 1CE340, 1D9F10, B7E50, 1F5620, E3E20, B7F00, 10DF30). |
| C0F60 (callee) | `src/physics/native_tu15_object_detachment.cpp:12` `native_tu15_detach_object_tree` | Full native with service hooks (C5250, C4D30, C3750). |
| BBD10 (callee) | `src/physics/native_tu15_object_deletion.cpp:143` `native_tu15_object_or_ancestor_deleting` | Full native. |
| 30BD0 (callee) | `include/halo2/native_tu15_normalize_vector.hpp`, bridge `src/physics/runtime/native_tu15_normalize_vector_bridge.hpp` | Full native, already spliced into `run_30bd0`. |
| BF050 (callee) | `src/campaign/source_campaign_object_scale_tu15.cpp:23` | Calls the translated BF050. No replica. |
| 1CD620, BA630, 1D30F0, BFF20, 1DC270, E24F0 | none | No replica. E24F0 appears only as a pointer in the type-definition data. |

---

## 12. Implementation notes (parity)

1. **Where to install.**
   - Replace the bodies of `run_b95d0` in **both** `physics_live_support_tu15_generated.cpp` and `death_cycle_runtime_tu15_generated.cpp`, and the body of `run_b9460` in the live-support unit, through the generators (`tools/generate_physics_live_support_tu15.py`, `tools/generate_death_cycle_runtime_tu15.py`). Use the same pattern as `run_bac70 -> native_tu15_render_object_ownership_bridge(s,0xbac70u)` and the `// BEGIN native normalize30BD0` splice in `run_30bd0`.
   - This serves all static intra-unit callers: 21 + 8 B95D0 sites and 5 B9460 sites. It also serves every `required()`-routed call (`run_actual`, `dispatch` from other units), because those end in the owning unit's entry switch, which calls the same `run_X`.
   - Optionally, add an early `0xB95D0` route in `required_entry_original` before `:2691` to save one LiveState load/save.
2. **Bridge ABI.**
   - Read register arguments from `s.gp` and stack arguments from `s.memory` at `s.gp[4]`.
   - On exit, set `ESP += 4 + imm`: B95D0 `+0x10`, B9460 `+0x28`, BA7E0 `+8`, B7E50 `+0xC`.
   - Preserve EBX, EBP, ESI and EDI. Set AL for B9460.
   - The other volatile register values listed in sections 3.1, 4.1 and 7 are only needed if a caller consumes them. None was found; a full audit is **OPEN**.
3. **Data stores** must go through the bank (`SourcePhysicsContactMemory` or the `w(...)` equivalent) with `s.observe`. The presented-pose scope journals every store outside `0x60000000..0x60100000` and rolls it back. Store-sequence oracles compare PCs. Use the store PCs given in this spec.
4. **Stack traffic.**
   - Pushes and caller-local copies live in the caller stack range (`0x60000000..0x60100000` for native roots). The presented journal does not record that range.
   - Existing natives elide transient call-stack events (see the normalize bridge comment).
   - If a native B95D0/B9460 calls a **translated** callee, it must still build that callee's exact argument image in emulated memory: callees read their arguments from `[ESP+n]`, and some (1D30F0, C0F60, BA630) write into their own argument slots.
   - B95D0's local copies **must exist in addressable memory** if the translated 10E340/E24F0 is used, because the handler writes through the pointers.
5. **FP environment.**
   - The bounds check uses COMISS: QNaN or SNaN sets MXCSR.IE, and a denormal sets DE. Use the same instruction in the same short-circuit order, for example the inline-asm `Flags::scalar_compare(a,b,false)` in `src/physics/runtime/physics_contact_runtime_internal.hpp:81`, so the sticky flags match.
   - E24F0 uses UCOMISS (IE only for SNaN) and 30BD0 (x87 + SSE, already native).
   - The vector copies are raw 32-bit moves: NaN payloads and -0.0 pass through unchanged, and no FP flags are raised.
6. **XMM liveness.** Before dropping B9460's XMM0..2 writes, confirm that no translated caller reads XMM0..XMM2 after `call B95D0`/`call B9460` without reloading. **OPEN**. This compiler keeps some XMM values live across known callees (see `include/halo2/native_tu15_object_scale.hpp:11`).
7. **Hot specialization.** set_object_position always passes `ECX = EDX = 0`, `location = 0` and `physics_force = 0`.
   - 10E340 can only reach E24F0 for bipeds, and there it is a no-op apart from reads.
   - B9460 skips the orientation block and calls 1CD620 with `(position_was_null = 0, force = 0)`.
   - BA630 recomputes the location through 11E600.
   - The dominant cost is under 1CD620 (Havok, about 30% of B9460), BF050/BF0C0 (about 41%) and BA630 (about 17%) (`docs/RENDER_THREAD_OFFLOAD_PLAN.md:311`). B95D0, B9460, BA7E0 and B7E50 themselves are only about 260 instructions combined.
8. **B9390 at the end of B95D0** can call `native_tu15_wake_object_ancestors` directly. Its semantics are identical to the translated copy, including reading the first header byte before testing NONE.
9. **Reentrancy.** BF050 → BFF20 can call BA7E0/BA630 again. 1CD620 → 1DC270 calls BF050 again. BA630 → BA570 → 10E4B0 → B9390. A native B9460 that keeps translated callees must therefore tolerate nested translated activity on the same object.

---

## 13. OPEN items

- Semantic meaning of `[[0x4F3218]+0x0C] == 4` (repo calls it `raw_mode`; probably the network/simulation role).
- Content of `*[0x474CE8]` (default position used by the 10F710 and 11DC00 call sites).
- Which cluster system (`0x4EABA4` or `0x4EABB0`) is collideable or non-collideable, and the meaning of body flag bits 9, 17 and 21.
- Physics component flag bits 1, 7 and 8 used by 1CD620.
- Exact BFF20 gating, and the internals of BF0C0, BFB90 and 1DC270, including the indirect path to Havok 2EC9B0/2EC4B0.
- Whether any translated caller consumes the volatile register or XMM outputs of B95D0/B9460 (EAX upper bits, ECX, EDX, XMM0-2).
- Whether fwd/up can be NULL at the BAF10, BB300 and BB3E0 B9460 sites.
- What EBX points to at the FAA23 site (probably another object's body: position `+0x64` and location `+0x28`).
- C0F60 `a3` / C4D30 naming in the existing native header (`delete_attachment`) compared with its use from BA630 (reconnect).
- Callee names inside BA630 (11E600, BA910, 1D2F10, B8D90, B92C0, B1870, B9330, 1496F0, 1CC290, 282C10) are tentative.
