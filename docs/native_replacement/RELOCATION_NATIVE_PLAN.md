# Native relocation plan: B95D0 / B9460 and their subtrees (TU1.5)

Status: implementation plan, 2026-11-08. Read-only research. Nothing was built,
run or edited except this file. Tree HEAD `46d2364a` (dirty working tree).

Inputs, all read in full:

| Tag | File |
|---|---|
| **[driver]** | `docs/native_replacement/B95D0_B9460_relocation_driver.md` |
| **[reconnect]** | `docs/native_replacement/BA630_cluster_reconnect.md` |
| **[nodes]** | `docs/native_replacement/BF050_node_rebuild.md` |
| **[havok]** | `docs/native_replacement/1CD620_havok_set_position.md` |
| scope | `src/rendering/shadows/source_presented_model_pose_tu15.cpp` |
| goal | `docs/NATIVE_REPLACEMENT_GOAL.md` |

Section, line and PC references below were checked against the working tree
on this date. Anything not determined is marked **OPEN**. The per-routine
pseudo-C, store PCs and conventions are in the four specs. This document does
not repeat them. It says what to build, in what order, how to plug it in and
how to prove it.

---

## 0. Summary

**Cost.** Measured on Zanzibar 4p (main thread), B95D0 takes 10.4% in total:

| Part | Share |
|---|---|
| 1CD620 | 5.4% |
| BF050 | 2.7% |
| BA630 | 1.7% |
| BA7E0 | 0.24% |
| Everything else: driver, 10E340, B7E50, B9390 and the call hops | 0.36% |

At about 45 ms per frame and about 40 relocated roots per frame, that is
roughly 4.7 ms per frame, or 118 µs per call. About 61 µs of each call is
under 1CD620. These are derived figures: shares times frame time.

**Phases.** The phases come in dependency order. Each one ships behind its own
reference switch and stays default-off until its oracle and the frame-parity
cases pass.

| # | Phase | Effort (engineer-days) | Expected gain, Zanzibar 4p main thread |
|---|---|---|---|
| 0 | Path-class census for the presented roots. Measurement only, no behaviour change. | 1 | Decides how much Phase 2 can take and what Phases 4 and 5 must cover |
| 1 | Native driver: B95D0, 10E340/E24F0, B9460, BA7E0, B7E50, B9390, 1D30F0, plus the oracle infrastructure. 1CD620, BF050 and BA630 stay translated behind exact residual calls. | 3 | ≤0.6% (BA7E0 plus the driver). Can be about 0 after the new call hops. This phase is enabling work. |
| 1b | Serve the static intra-unit call sites (21 + 8 B95D0 sites, 5 B9460 sites) with the same native driver through a build-local splice. | 1 | Tick callers become native. Small. |
| 2 | Skip the Havok update inside the presented scope only, for path classes proven unconsumed. Only if the poison experiment and the scope-level oracle pass. | 2–3 | Up to 5.4%, limited to the admitted classes (Phase 0 measures this) |
| 3 | BA630 cluster core plus shell as memory-level natives | 4–5 | Most of 1.7% |
| 4 | BF050 / BF0C0 orchestration / BFF20 / BFC70 / 1707A0 natives, with the animation and physics-delivery subtrees as residual calls | 5–7 | Most of 2.7%, if bit 29 or no-animation dominates (Phase 0) |
| 4b | BFB90 animation subtree (shared with the object tick: 114750, F34B0) | 10–15 | The rest of BF050, plus tick gains |
| 5 | 1CD620 common path as a native (0x200 and multi-body, non-fixed bodies) | 4 | Tick callers. It also supplies the overlay for classes that Phase 2 could not admit. |
| 6 | Fixed-body and 0x400 paths: hkWorld remove/add, broadphase, listeners | 15+ (shared with the Havok-world native effort) | Whatever is left of 5.4%, and the tick |
| 7 | Retire the translated route: flip defaults, census shows zero, remove the bodies | 1 | — |

**Key decisions:**

1. **One entry point.** A single ABI-level native entry,
   `native_tu15_object_relocation_run(memory, entry, abi, services, observe)`,
   serves both B95D0 and B9460.
   - It is installed at the top of
     `PhysicsLiveWorldTu15::Impl::required_entry_original`
     (`src/physics/physics_live_world_tu15.cpp:1896`), next to the existing
     0xC1270 and 0xC12F0 intercepts.
   - That one location covers:
     - `set_object_position` (through `run_actual`);
     - every `dispatch()`-routed caller in six other units;
     - the death-cycle copy's `required()` call to B9460.
   - Static `run_b95d0` / `run_b9460` calls are forwarded to the same place by a
     build-local splice (Phase 1b).
2. **Memory access.** Native code reads and writes source memory only through
   `SourcePhysicsContactMemory` (the live `SourceRuntimeMemory` bank).
   - Every store carries the original PC.
   - Every store goes through `memory.write()`, so write guards, write
     observers (the presented-scope journal, undo journals) and the topology
     guard all keep working (§2.1).
3. **Translated subtrees as residual calls.** Subtrees that are not yet native
   stay translated behind residual calls. Each residual call reproduces the
   original stack image: argument slots, return PCs and callee-saved register
   values at the original ESP. Translated callees then behave, and leave stack
   residue, exactly as before (§2.6).
4. **Oracle.** For each call, the native candidate runs first under a
   `SourceRuntimeUndoJournal`. Then the translated reference runs live. The
   two are compared on:
   - final bytes;
   - the PC-labelled store sequence;
   - stack residue;
   - registers, flags and FP state;
   - host-side effects.

   The live state is always the reference's while verifying (§4).
5. **Havok elision.** Skipping Havok work is not part of the native
   replication. It is a separate switch that only the presented scope can
   reach, through a new `present_object_position` entry, and it is admitted
   per object path class. The poison experiment is necessary but not
   sufficient, because it does not see reads that happen inside the
   relocation itself (§5).

---

## 1. Facts this plan depends on

### 1.1 Call shape on the presented path

- `PhysicsLiveWorldTu15::Impl::set_object_position`
  (`src/physics/physics_live_world_tu15.cpp:4532`):
  1. Validates the input.
  2. Writes the point to `0x60060000`.
  3. Builds `frame({object,0,0})`: ESP = `0x600F0000`, sentinel return
     `0x60300000`, FP = `{0x027F, 0x1F80}`.
  4. Sets EAX = point.
  5. Calls `run_actual(0xB95D0, a)`.
- The effective worker call is `B9460(obj, &local_pos, 0, 0, 0, 1, 1, 0, 0)`
  ([driver] §2.1). From there:
  - forward and up are NULL, so B9460 skips the orientation block;
  - 10E340 can only reach E24F0, for bipeds, and E24F0 only reads;
  - 1CD620 gets `(pos_null=0, allow80=0)`;
  - BA630 gets `location=NULL`, so it runs 11E600 once or twice.
- The B9460 order is confirmed ([driver] §4.2). "Connected" (`body+4` bit 8)
  is sampled before BA7E0 runs.

  ```
  [BA7E0 if connected && !keep_links]
    -> bounds check + position store + B7E50(+D4, 2)
    -> [forward/up + B7E50(+D4, 4)]
    -> [1CD620 if update_physics]
    -> [BF050 if rebuild_nodes]
    -> [BA630 if connected && !keep_links]
  ```

- The only production caller of `set_object_position` in the render path is
  `source_presented_model_pose_tu15.cpp:357`.
  - `src/gameplay/simulation.cpp:3133` calls it from gameplay.
  - Four test files also call it.
  - So `set_object_position` must keep exact simulation semantics. Any
    presented-only elision needs its own entry (§2.5).

### 1.2 Routing today

- `run_actual` → `Impl::required` → `required_entry` → `required_entry_original`.
  The first matching `source_*_owns_entry` wins.
- Owner of each routine in this tree (checked in the owner switches on this date):
  - **0xB95D0:** the death-cycle unit (`:2691`) wins over live support (`:2843`).
  - **0x10E340, 0xE24F0:** both units own them. Both B95D0 copies call them statically.
  - **B9460, BA7E0, B7E50, 1CD620, 1DC270, BF050, BF0C0, BFF20, BFB90, 1DBBD0,
    BA630, BA570, C0F60, C3750, C4D30, C5250, 1D30F0, 1D2F10, 14D0E0, B8D90,
    1CC290, 1D9510, 1D8ED0, 323B20:** owned by live support. The death-cycle
    unit owns none of them.
  - **0xB9390:** live support does not own it. It routes to the
    scene-device-constructor copy (`:2711`), ahead of the native unit-action
    owner (`:2918`).
  - **0x1D97F0:** live support does not own it. It is a World-module (WR)
    routine.
- `Impl::required` (`:1584-1802`) has target-specific intercepts. None of them
  matches any routine in this tree.
- Inside a generated unit:
  - A static call (`s.push(ret);run_X(s);`) never reaches `required()`.
  - An out-of-unit call goes through `dispatch()`, then `s.required()`, then
    `services.required_owner`. Each hop saves and loads the full ABI
    (`src/physics/runtime/physics_contact_runtime_internal.hpp`, `State::required`).
- The live-support unit is partitioned into 32 translation units
  (`cmake/PhysicsLiveWorldTu15.cmake:875`).
- Both generated units are specialized into a build-local copy by
  `tools/specialize_native_operands.py` (`cmake/NativeOperandSpecialization.cmake`).
  That same mechanism installs the 145560 and 144CF0 leaf kernels.

### 1.3 How stores reach guards, observers and the scope journal

- `SourceRuntimeMemory::write` (`src/runtime/source_runtime_memory.cpp:634`)
  calls `source_runtime_validate_write(memory, address, size, observe=true)`
  (`src/runtime/source_runtime_write_protection.inc:336`) before it stores.
  This also applies on the writable-direct-page fast path. That call runs:
  - every registered write guard (resource protections, topology guards);
  - every write observer: the presented-scope journal and any
    `SourceRuntimeUndoJournal`.
- The translated code stores through `physics_step_detail::w()`
  (`src/physics/runtime/physics_step_internal.hpp:28`).
  - `w()` calls `m.write()`.
  - Only when a store observer is installed does it build a
    `SourcePhysicsContactRecordStore{pc, address, before, written}` and
    deliver it.
  - 16-byte stores use `vw()`: two 8-byte writes, same PC.
  - `rep stos` / `rep movs` are emitted as per-dword `w()` calls.
- `run_actual` composes the caller's observer with the registered
  `physics_store_observer` through `source_physics_store_observers_tu15`
  (`include/halo2/source_physics_store_observers_tu15.hpp`). It also updates
  `last_instruction` from each store's PC, which is used for fault reports.
- Native code must store only through `SourcePhysicsContactMemory::write`, or
  through helpers built on it. It must never write through `mutable_view()` or
  `direct_page()->bytes`. Direct pages are for reads only, as in `r()` and
  `v()`.

### 1.4 What the presented scope does with relocation output

- The scope runs in `SourcePresentedModelPoseScopeTu15::SourcePresentedModelPoseScopeTu15`,
  `source_presented_model_pose_tu15.cpp:217-374`.
- It first records every presented node bank in its journal.
- It then registers a write observer that journals every store, with these
  exceptions:
  - stores inside `[0x60000000, 0x60100000)` (the caller stack and argument
    arena);
  - stores into persistent resource owners
    (`SourcePresentationResourceRegionsTu15`), which are counted but not
    journaled.
- The journal is bounded at 16 MiB. A topology guard throws on any topology
  change.
- For each presented root it then does one of two things:
  - A root that is "unmoved" (no child, `+0x11C == 0`, position/forward/up/scale
    within 1e-4) gets `+0x64` written directly. `HALO2_REFERENCE_PRESENTED_RELOCATION=1`
    disables this skip.
  - Any other root goes through `world.set_object_position`.
- After the relocation the scope:
  1. writes `+0x70`, `+0x7C` and `+0xA0` for every root;
  2. writes `+0x30/+0x3C` for every presented object;
  3. runs `relocation_diagnostics()` (census, restore and poison experiments);
  4. builds the presented-model binding.
- **Render runs while the scope is alive.** The destructor restores every
  journaled byte in reverse order after rendering. Render-time writes are not
  journaled.
- Earlier evidence (goal doc, 2026-11-07, Zanzibar endpoint and `--motion`
  cases): each of the following was frame-identical:
  - restoring every relocation store except the root and sphere words;
  - restoring only Havok;
  - restoring only physical pages;
  - skipping the relocation for every root.
- The poison experiment now running
  (`HALO2_EXPERIMENT_RELOCATION_RESTORE=<havok owner>` plus
  `HALO2_EXPERIMENT_RELOCATION_POISON=<byte>`) poisons, after all roots have
  been relocated, every Havok-heap byte that changed. Identical frames prove
  that **render** does not read those bytes. They do not prove that later
  steps **inside the relocation** do not read them (§5.1).

---

## 2. Part 1: native entry, memory access and plug-in points

### 2.1 Memory access rules for native code

Use the house pattern that existing TU1.5 natives already use:

| Need | Use | Precedent |
|---|---|---|
| Memory interface | `SourcePhysicsContactMemory&`: `read`, `write`, `read_vector`, `direct_page`, `try_read_view` | `include/halo2/physics_contact_record.hpp:26` |
| Scalar read | `physics_step_detail::r(m, addr, width)`: direct-page fast path, otherwise `m.read` | `src/physics/runtime/physics_step_internal.hpp:13` |
| Labelled store | A file-local `Memory` helper: `word(addr, width)` for reads; `word(pc, addr, value, width)` for stores, building `SourcePhysicsContactRecordStore` only when an observer is present, then calling `m.write` | `src/physics/native_tu15_activation_services.cpp:7-13`, `src/physics/native_tu15_camera_geometric_query.cpp:16-24` |
| 16-byte store | `vw()` semantics: two 8-byte writes with the same PC | `physics_step_internal.hpp` |
| `rep movs` / `rep stos` | Per-dword stores with the original PC. Block copies would change the observer sequence. | translated `0xBF16C` / `0xBF351` |
| FP environment | `NativeTu15PhysicsFrameFp{control_word, status, mxcsr}` taken from `abi.floating_point` and `abi.x87_status`, plus an RAII `Environment` that installs the source x87 control word, status and MXCSR, saves and restores the host state (`fnsave`/`frstor`, `_mm_setcsr`) and publishes the state back | `include/halo2/native_tu15_physics_frame.hpp`, `native_tu15_camera_geometric_query.cpp:25-33` |
| SSE arithmetic | Inline-asm `mulss`/`addss`/`subss`/`divss`/`comiss`/`ucomiss` helpers, so the compiler cannot reorder or contract them | `native_tu15_camera_geometric_query.cpp:35-48` |
| x87 arithmetic | Inline-asm sequences under the installed control word | same file; `biped_body_placement.cpp` (TU1.1 323B20/2E4350 replica) |
| Observer composition | Take `observe` from the caller (`run_actual` has already composed `physics_store_observer`). Wrap it so `last_instruction` follows each store's PC. | `physics_live_world_tu15.cpp:1607`, `:1683` |
| Entry diagnostics | `SourcePhysicsEntryObservation diagnostic(m, entry, [&]{return a;})` around each native entry | `physics_live_world_tu15.cpp:1662` |

Compile every new native source with `-fno-fast-math -ffp-contract=off
-frounding-math`, or `/fp:strict` on MSVC. This matches
`halo2_native_pair_consumer` (`cmake/PhysicsLiveWorldTu15.cmake:50-73`).

Reusable natives, with their exact entry points:

| Routine | Native | Kind |
|---|---|---|
| B9390 | `native_tu15_wake_object_ancestors` (`src/physics/native_tu15_activation_services.cpp:50`) | full, store PC 0xB93C8 |
| B7E50 | `network_dirty` (`src/physics/native_tu15_object_motion.cpp:372`) | full but file-local and unlabelled. Promote it to an exported, labelled function (store PC 0xB7E97). |
| C0F60 | `native_tu15_detach_object_tree` (`src/physics/native_tu15_object_detachment.cpp:12`) | full, with C5250/C4D30/C3750 as services |
| BA570 | `native_tu15_delete_object` (`src/physics/native_tu15_object_deletion.cpp:201`) | full, with about ten services |
| BBD10 | `native_tu15_object_or_ancestor_deleting` | full |
| 11E600 / 14CDB0 | `native_tu15_constraint_effect_location` / `_point_leaf` (`src/physics/native_tu15_constraint_effect_location.cpp`) | full, memory-level |
| BA910 / 16E290 | `native_tu15_build_object_spatial_packet` / `native_tu15_object_spatial_flags` | full |
| 16E6A0 / 16E8A0 | `native_tu15_data_array_allocate` / `native_tu15_data_array_delete` | full |
| B1870 | `native_tu15_object_phantom_delete_protected` | full |
| 282C10 | `native_tu15_process_dirty_island_objects` (+ binding) | full, with services |
| 315CB0 | `native_tu15_activate_physics_body` | full |
| 1D8470 | `native_tu15_reset_physics_component` | full |
| 145560 / 144CF0 | `kernel_145560` / `kernel_144cf0` (`src/physics/runtime/native_tu15_matrix_leaves.hpp`, register-level); `native_campaign_object_transform_product_tu15` (store-exact 145560) | reuse the `Io` template for store-exact value use |
| 144090 | `native_tu15_camera_inverse_frame` | full, memory-level |
| BC190 (+144BF0) | `native_tu15_object_transform` | value-level. Check it against the store order before reuse. |
| TU1.1 ports | `object_current_nodes.cpp` (BF0C0), `item_bounds.cpp` (BFF20), `biped_body_placement.cpp` (1D8ED0/1D8FA0/323B20/2E4350), `physics_body_matrix.cpp` (1D8B90), `physics_node_delivery.cpp` (1DBBD0), `biped_world_removal/insertion` (1D9510/1D97F0), `physics_component_rebuild.cpp` (1D73D0), `object_light_spatial.cpp` (host model of 1D2F10/14D0E0/14CEA0/123210/243450) | port these. Address maps are in [nodes] §6.1 and [havok] §6 (+0x48C0). |

### 2.2 The native entry (ABI level)

```cpp
// include/halo2/native_tu15_object_relocation.hpp
namespace halo2 {
struct NativeTu15ObjectRelocationOptions {
    bool elide_presented_havok{};   // §5; only present_object_position sets it
};
struct NativeTu15ObjectRelocationStats { /* calls, residual calls by target, OOB deletes, ... */ };
// ActualB95D0 / ActualB9460. `abi` holds the caller's real register file
// and ESP ([ESP] = return address). On return, `abi` holds the original
// exit state: ESP popped by 0x10 (B95D0) or 0x28 (B9460), callee-saved
// registers preserved, documented volatile registers and FP state published.
void native_tu15_object_relocation_run(SourcePhysicsContactMemory&, std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi&, const SourcePhysicsContactRuntimeServices&,
    const SourcePhysicsContactRecordObserver& = {},
    const NativeTu15ObjectRelocationOptions& = {}, NativeTu15ObjectRelocationStats* = nullptr);
bool native_tu15_object_relocation_owns_entry(std::uint32_t entry); // 0xB95D0, 0xB9460
}
```

The signature follows existing ABI-level natives:
- `native_tu15_render_object_ownership_run(bank, entry, a, observer)`
  (`physics_live_world_tu15.cpp:1593-1595`, `:3405-3414`);
- `native_weapon_effect_dispatch_tu15_run(m, entry, abi, services, observe)`.

Internally, the algorithms are typed functions over memory and do not
interpret registers:

```cpp
// B95D0: copies, 10E340 handlers, B9460, BA570 on out-of-bounds, B9390
void native_tu15_relocate_object(Memory&, Frame&, const Request&, Services&);
// B9460: returns position_ok
bool native_tu15_relocate_object_worker(Memory&, Frame&, const WorkerArgs&, Services&);
void native_tu15_disconnect_object_from_map(Memory&, Frame&, std::uint32_t object, Services&);  // BA7E0
void native_tu15_release_object_cluster_references(Memory&, std::uint32_t system,
    std::uint32_t object, std::uint32_t head);                                                  // 1D30F0
void native_tu15_object_type_relocation(Memory&, Frame&, std::uint32_t object,
    std::uint32_t pos, std::uint32_t fwd, std::uint32_t up, Services&);                         // 10E340 + E24F0
```

`Frame` carries:
- the entry ESP;
- the emulated addresses of the routine's locals and argument slots, which
  are the original addresses: B95D0 `pos/fwd/up` locals at `E-0x24/-0x18/-0x0C`,
  B9460's `position_ok` byte at `E'-1`;
- the callee-saved register values the original holds at each call site
  (§2.6).

`Services` holds one `std::function` per callee. Each one is labelled with its
address (`//BF050`, `//BA630`, ...). It is bound either to a native or to a
residual translated call (§2.6). This follows the house style of
`NativeTu15ObjectDeletionServices` and `NativeTu15PhysicsFrameServices`.

### 2.3 Plug point A: `required_entry_original` (covers the hot path)

Insert this at the top of `required_entry_original`
(`physics_live_world_tu15.cpp:1896`, after the bank check, next to the
0xC1270/0xC12F0 intercepts):

```cpp
if(image==SourcePhysicsContactImage::TitleUpdate15 && native_tu15_object_relocation_owns_entry(c.target)
   && !object_relocation_reference(c.target) && !object_relocation_bypass()) {
    SourcePhysicsEntryObservation diagnostic(m,c.target,[&]{return a;});
    SourcePhysicsContactRecordObserver observed;
    if(o)observed=[&](const auto&e){last_instruction=e.instruction;o(e);};
    if(object_relocation_verify(c.target)){verify_object_relocation(m,c,a,observed);return;}  // §4
    native_tu15_object_relocation_run(m,c.target,a,services(),observed,relocation_options(c,a),&native_relocations);
    return;
}
```

This catches the following, because each of them reaches
`required_entry_original`:

- `set_object_position` → `run_actual(0xB95D0)`. `run_actual` still installs
  its RAII bindings (attachment events, handoffs, entry observation), so any
  residual translated callee sees the same environment as today.
- The 0xB95D0 `dispatch()` sites in source_ai_actor, source_ai_frame,
  source_campaign_activation, source_ctf_activation, source_damage_feedback,
  source_teleporter and source_combat_action ([driver] §10.3).
- The death-cycle B95D0's `dispatch(0xB9460)`.
- The live-support entry switch, for any other `required(0xB95D0 | 0xB9460)`.

Because the intercept sits ahead of every owner, the death-cycle-first quirk
for 0xB95D0 no longer matters. B9390 is called natively
(`native_tu15_wake_object_ancestors`), which behaves exactly like the
scene-device-constructor copy ([driver] §7, §12.8).

### 2.4 Plug point B: static intra-unit calls (Phase 1b)

The 21 static `run_b95d0` sites in live support, the 8 in death cycle and the
5 static `run_b9460` sites in live support never reach `required()`. Serve
them with a prefix splice in the build-local specialized copy, following the
`LEAVES` mechanism in `tools/specialize_native_operands.py:38-75`:

- Bind each splice to the SHA-256 of the literal body: `run_b95d0` in both
  units and `run_b9460` in live support. A changed body keeps its translated
  route.
- Each prefix forwards the call into the required chain:

  ```cpp
      // BEGIN native relocationb95d0
      if(halo2::native_tu15_relocation_forward(s,0xb95d0u))return;
      // END native relocationb95d0
  ```

  `native_tu15_relocation_forward` (header
  `src/physics/runtime/native_tu15_object_relocation_bridge.hpp`):
  1. returns false when the reference switch or bypass is set, or the image
     is not TU1.5;
  2. otherwise does `auto abi=s.save(); s.services.required_owner(s.memory,{0xb95d0u,0xb95d0u,false},abi,s.observe); s.load(abi);`
     and returns true.
- Routing every static caller through plug point A keeps one native, one
  oracle and one switch. The cost is one ABI save/load per call, which is the
  same as any `dispatch()` hop.
- **Bypass flag.** The reference path inside plug point A runs the translated
  body by calling `source_run_death_cycle_runtime_tu15` /
  `source_run_physics_live_support_tu15` directly, with a thread-local bypass
  set (the pattern of `native_tu15_leaves::bypass()`). Without it, the
  reference would re-enter the splice and recurse.
- A build with `HALO2_SPECIALIZE_NATIVE_OPERANDS=OFF` has no splice, so static
  callers keep the translated body. That is still correct.
- Once the route is proven, move the splice into the generators:
  - `tools/generate_physics_live_support_tu15.py`: next to the `install_native_*`
    hooks at `:1127-1184`, under `if 0xb95d0 in ENTRIES`;
  - `tools/generate_death_cycle_runtime_tu15.py:494-497`;
  - in a new module `tools/native_object_relocation_bridge_tu15.py`, modelled on
    `native_normalize_vector_bridge_tu15.py`.

  Regenerating the 19.7 MB unit should wait until it is needed, because other
  sessions edit these files concurrently.

### 2.5 The presented-only entry

- Add `PhysicsLiveWorldTu15::present_object_position(std::uint32_t object, Vec3 position)`.
  It has the same validation and point write as `set_object_position`.
- It sets an `Impl` member, `presented_relocation_root`, to the root frame's
  ESP for the duration of the `run_actual(0xB95D0)` call.
- `relocation_options(c,a)` sets `elide_presented_havok` only when all of these
  hold:
  - `c.target == 0xB95D0`;
  - `a.gp[4] == presented_relocation_root`;
  - `word(a.gp[4]) == 0x60300000`, the root sentinel;
  - Phase 2's switch admits it.

  Nested and tick calls can never inherit the elision.
- Change only `source_presented_model_pose_tu15.cpp:357` to call
  `present_object_position`. `simulation.cpp:3133` and the tests keep
  `set_object_position`, which keeps exact simulation semantics.

### 2.6 Residual (translated) calls from native code

Model: the residual-call lambda in
`native_tu15_step_physics_frame_with_residuals`
(`src/physics/native_tu15_physics_frame_residuals.cpp:25-60`) and its invoker
(`physics_live_world_tu15.cpp:3352-3389`). For each translated callee:

1. **Stack image.** Write the outgoing argument slots and the **original
   return PC** at the **original addresses**, then set ESP to the original
   callee-entry ESP.
   - With E = B95D0 entry ESP and E' = B9460 entry ESP (= E-0x5C when called
     from B95D0), the callee-entry ESPs are:

     | Call | Callee-entry ESP | Return PC | Arguments |
     |---|---|---|---|
     | 10E340 | E-0x40 | 0xB963C | (pp, pf) |
     | B9460 | E-0x5C | 0xB9655 | 9 args |
     | BA570 | E-0x3C | 0xB9690 | |
     | B9390 | E-0x38 | 0xB9697 | |
     | BA7E0 | E'-0x1C | 0xB94A1 | |
     | B7E50 | E'-0x20 | 0xB951F / 0xB956F | |
     | 1CD620 | E'-0x20 | 0xB958F | EAX = object |
     | BF050 | E'-0x1C | 0xB95A1 | |
     | BA630 | E'-0x10 | 0xB95C0 | (object, location) |

     Derived from the push tables in [driver] §3.3 and §4.3.
   - Return PCs matter for two reasons:
     - Stack residue.
     - Some `Impl::required` intercepts validate their return addresses and
       caller PCs (for example the 0x30270 intercept at `:1596-1603`). A wrong
       return PC makes them throw.
2. **Registers at the call.** Set the callee's register inputs. Set EBX, EBP,
   ESI and EDI to the values the original holds at that call site, because the
   translated callee pushes them, which becomes stack residue.
   - Known values from [driver] §4.1: EBP = objects header pointer, EDI = body
     (then body+0x7C after B9544), EBX bit 0 = connected byte, ESI =
     `(object&0xFFFF)*12` until B9558.
   - Transcribe the exact full-width values from the translated bodies:
     B9460 at `LS:7985-8110`, B95D0 at `LS:8112-8190`.
3. **Invoke.** `services.required_owner(m,{call_pc,target,false},abi,o)`. This
   is the exact current routing.
   - Check `abi.gp[4] == callee_esp + 4 + callee_pop` on return, as
     `native_tu15_physics_frame_residuals.cpp:57` does.
   - Carry `abi.floating_point` and `abi.x87_status` back into the native's
     `NativeTu15PhysicsFrameFp`, before and after the call.
4. **Re-read after the call.** Re-read memory after every residual call. Cache
   only the values the original keeps in registers across calls:
   - B9460 keeps `objects` (EBP), `body` (EDI), `connected` (BL) and the
     `position_ok` byte, but re-derives `body` before each `+0xD4` read.
   - BA630 does not re-derive `hdr` after BA570.

**Hop cost.** Each residual call is one required-chain walk plus a LiveState
load/save. With three hops per relocation (1CD620, BF050, BA630) at about 40
calls per frame, Phase 1 adds about 120 hops per frame. If the profile shows
that cost:
- add a verified direct-owner fast path that calls
  `source_run_physics_live_support_tu15(m,target,a,fp,services(),o)` for
  targets proven to have live support as their first owner (all of the
  residual targets above);
- assert that in a debug build by evaluating the owner predicates once.

### 2.7 Stack frame and residue policy

- The native uses the original frame addresses below the entry ESP, which are
  free by the x86 ABI.
- While any callee is still translated, reproduce the original stack writes:
  pushes, return addresses, locals, argument-slot overwrites, and B9460's
  `push ecx` slot holding `position_ok`.
  - They cost a few dozen 4-byte stores per call.
  - They keep the oracle strict (§4.3, L3).
  - The scope's comment (`:277-280`) says render kernels reuse the caller
    stack and keep "uncleared caller residue", so residue is not
    automatically dead.
- Elide stack residue only after:
  - every callee in the subtree is native; and
  - a parity run with residue elided is frame-identical; and
  - the L3 oracle has been downgraded to diagnostic for that subtree.

  The normalize bridge is the precedent: it keeps the final residue bytes.

### 2.8 Register, flag and XMM outputs

| Routine | Exit state to reproduce |
|---|---|
| B95D0 | ESP += 0x10. EBX/EBP/ESI/EDI preserved. EAX/ECX/EDX and EFLAGS as B9390 leaves them: EAX = object on early exit else 0xFFFFFFFF, ECX = rows, EDX = `(object&0xFFFF)*3` ([driver] §3.1, §7). |
| B9460 | ESP += 0x28. AL = `position_ok`. ECX = `(ECX_entry & 0x00FFFFFF) \| (position_ok << 24)`. EAX bits 8-31 come from whatever ran last before B95C0 ([driver] §4.1). |
| XMM0-2 | Clobbered on the bounds path. |

- While BA630 is the last residual callee, the translated BA630 sets the
  natural volatile state.
- After BA630 becomes native, reproduce the volatile state exactly, or prove
  every caller dead. The XMM liveness audit is **OPEN** ([driver] §12.6).
  The oracle compares everything (L4).

---

## 3. Part 2: port order, residual subtrees and layout

### 3.1 Phase 0: path-class census (measurement only)

Add `HALO2_RELOCATION_PATH_CENSUS=1` to the presented scope. It reads source
memory immediately before each `set_object_position`, prints every 120
frames, and does not change behaviour. Per relocated root it records:

- object type;
- children and attachment size;
- connected bit;
- component class:
  - none;
  - gated by 0x80 / 0x100 / (0x2 && pos_null);
  - 0x400;
  - 0x200;
  - multi-body;
  - any body fixed in the world (`rb+0x40 && flags&0x20`);
  - any island asleep (`rb+0x44 && !island+0x29`);
- `body+0xC0` bit 6;
- for the component: `row+4 & 0x200` and `row+0x74` (whether 1DBBD0 would deliver);
- BF0C0 branch:
  - body+4 bit 29;
  - biped `+0x34B`;
  - animation manager present;
  - frozen (`+0xB3`);
- for each child: whether BFF20 reconnects it.

Read after the call:
- whether BA630 changed header bit 0 (an activation transition);
- the persistent-write count, which the scope already tracks;
- the profile share per class, from `tools/perf_4k60/callees.py` on a
  `profile_run.sh` capture.

This data decides:
- which classes Phase 2 can admit;
- whether Phase 4 without the animation subtree covers BF050's cost;
- where the 5.4% under 1CD620 actually goes. Static research cannot tell
  whether it is fixed-body churn, island wake-ups or multi-body math
  ([havok] §4.3, §5).

### 3.2 Phase list

**Phase 1: native driver.** The cheapest work, and it establishes the
plumbing and the oracle.

- **Native:**
  - B95D0: copies, 10E340 handler walk, OOB/raw-mode test, B9390 via
    `native_tu15_wake_object_ancestors`.
  - 10E340 and E24F0. Walk the runtime table `0x474B90[type]+0x84`. E24F0 is
    the only TU1.5 handler. Any other non-zero `+0x68` handler goes to a
    residual call through its address.
  - B9460, including the COMISS bounds check in the original short-circuit
    order through `Flags::scalar_compare` semantics
    (`physics_contact_runtime_internal.hpp:81`).
  - B7E50: promote it with store PC 0xB7E97.
  - BA7E0: `native_tu15_detach_object_tree(obj,1,0,0,0)`, a new 1D30F0, then
    the RMWs at 0xBA82E and 0xBA83D.
  - 1D30F0 (50 instructions): new, using `native_tu15_data_array_delete` for
    16E8A0. Store PCs are 0x1D311D (argument slot), 0x1D3165 and 0x1D3176.
- **Residual (translated):** 1CD620, BF050, BA630, BA570 (out-of-bounds path
  only, rare), and C0F60's C5250/C4D30/C3750 services.
- **Acceptance:**
  - per-call oracle L1-L5 clean on the Zanzibar and Coagulation parity runs;
  - frames identical against `HALO2_REFERENCE_OBJECT_RELOCATION=1`.

**Phase 1b: static callers.** The build-local splice from §2.4.
- Adds coverage of non-null forward/up, `location != 0`, `keep_links=1`
  (BE610) and the BB200 predecessor.
- The question of whether forward/up can be NULL at BAF10, BB300 and BB3E0 is
  answered empirically: the oracle refuses NULL up with non-NULL forward
  rather than reading address 0. **OPEN** ([driver] §13).

**Phase 2: presented-only Havok elision.** Requirements are in §5. It needs
Phase 1, because 1CD620 is a static call inside `run_b9460`.
- If the poison experiment fails, drop this phase. Phases 5 and 6 then become
  the 1CD620 route.

**Phase 3: BA630.** Order from [reconnect] §6:
1. 14CEA0 + 123210 + 243450: port `source_object_light_portal_intersects` to
   memory reads. Keep the two plane reads, `*(sbsp+0x18)` and `*(0x4ECC0C)`.
   Use x87 PC53 through the `Environment`.
2. 14D0E0: exact depth-first search, with the conditional stamp store 0x14D12D.
3. 14D080 and 1D2F10: use `native_tu15_data_array_allocate` in strict
   reference-then-entry order. Toggle 0x4F3CE4 / 0x4F3CE1 on the flood path
   only.
4. BA630 shell:
   - 11E600 twice;
   - BA910 via `native_tu15_build_object_spatial_packet`, but write the
     payload to the original stack local, because the stack residue rule
     applies;
   - 1D2F10;
   - the RMWs at 0xBA752 and 0xBA760;
   - `native_tu15_detach_object_tree(obj,0,1,0,0)` with C4D30 as a service
     (native once its own spec exists, which reuses 1D2F10 on 0x4ECBE0);
   - B8D90, with 10DDE0's callbacks as residual calls;
   - B92C0 and B9330, with 1CC3F0/1CC440 as residual calls;
   - B1870 native;
   - BA570 residual, or native with services;
   - the Havok tail (1496F0, 1CC290, 282C10) residual, only when the object
     was previously unconnected.
- The bit-exact host model `SourceObjectRegistrationJournal::link` serves as
  a unit-test reference for the registration prefix.

**Phase 4: BF050.** Port `source_update_object_current_nodes_1_1` (TU1.1
`object_current_nodes.cpp`) to TU1.5 addresses, using the maps in [nodes]
§6.1.
- **Native:** BF050, BF0C0 orchestration, BFF20 (non-crate path), BFC70 (all
  branches), 1707A0, the C1xxx leaves, BAC00, C15D0, C1CC0, B87B0, B8780, and
  the 10E180 walk with the 1154A0 and 10F440 hooks (BCDF0 stays residual).
  - 144CF0 and 145560 run through the `native_tu15_leaves` `Io` template with
    a store-exact real `Io`.
- **Residual:** BFB90 (the whole animation subtree), 1DBBD0 (physics node
  delivery), B3AE0/B3B70, E3BC0 and 1DC7A0.
- **Stack locals.** Locals passed by pointer to residual callees must stay in
  emulated stack memory at their original addresses:
  - L_70: the mask, written by 1DBBD0;
  - L_38/L_44/L_50: read by BFC70;
  - L_2C/L_64: written by B3AE0/B3B70;
  - L_90/L_C4.
- **Register contracts:**
  - BFC70: BL;
  - BFB90: EAX/ECX/EDX;
  - 10E180 / 10E100: EBX.
  - Full list in [nodes] §6.3.
- **No shortcut.** Never re-root existing matrices with a delta transform. The
  original recomputes `parent ∘ M(orient)` in model order ([nodes] §6.2).

**Phase 4b: animation subtree.** BFB90, 1D5840, 1CF210 and its decode chain,
BF9A0 (x87 and CRT fmod 0x37DC0A), 1E1720 (x87), 1E3150 and 1E5C80/126200
(persistent resource admissions), and the 114750 / F34B0→F2CC0 / 10C0B0
hooks.
- Coordinate this with the object-tick backlog: 114750 is rank 22 and F34B0
  rank 25 in the goal doc.

**Phase 5: 1CD620 common path.** Port the TU1.1 replicas with the +0x48C0
address offset ([havok] §6):
- **Native:**
  - 1CD620 and 1DC270, both the 0x200 path and the multi-body path (the
    latter with the x87 Gram–Schmidt);
  - 1DC610, 1DC680 (including its write into its own argument slot) and
    E32C0;
  - BC190 / 144BF0 / 145560 / 144090;
  - 1D8ED0 and 1D8FA0;
  - 315C90, plus 315CB0 via `native_tu15_activate_physics_body`;
  - 323B20 + 2E4350 + 2E4690;
  - 2E4470 / 2E4820 / 2E4650 / 2E44D0.
- **Residual:** the fixed-in-world branch (1D9510, 1E1130, 1D97F0), the 0x400
  branch (1D9510, `native_tu15_reset_physics_component`, BF050 native from
  Phase 4, 1D73D0, 1D97F0), and the island-activation listener virtuals.
- **Why it is needed:**
  - Tick callers (BAF10, BB300, BB3E0 through B9460; E5181 and 1EDBA3 through
    1CD620).
  - It supplies the motion overlay (§5.6) for presented classes that Phase 2
    could not admit.

**Phase 6: fixed-body and 0x400 paths.**
- 1D9510 and 1D97F0: port the TU1.1 replicas `biped_world_removal` /
  `biped_world_insertion`.
- 1E1130.
- 1D73D0: port `physics_component_rebuild.cpp`.
- hkWorld removeEntity / addEntity (3198A0 / 319370), the broadphase
  (2EC9B0 / 2EC4B0, goal-doc backlog ranks 5 and 11), the entity and
  activation listener arrays, the allocator (0x48C938 virtuals), 1CC560 with
  the 850-body eviction (1CDD60), and 22D0A0 / BBBC0.
- This is shared with the Havok-world native effort. The chain from hkWorld
  to the broadphase is **OPEN** ([havok] §7).

**Phase 7: retire.**
1. Flip each default to native once its subtree is proven.
2. Keep `HALO2_REFERENCE_OBJECT_RELOCATION` until the whole tree is native.
3. Remove the translated bodies from the routes once
   `HALO2_INSTRUCTION_CENSUS` shows zero executions on every parity map at 1p
   and 4p.

### 3.3 File and namespace layout

All code lives in `namespace halo2`.
- Exported functions are prefixed `native_tu15_` and carry an `// Actual<ADDR>`
  comment.
- The `Memory`/`Environment` helpers live in each file's anonymous namespace.
- Each exported function gets a `...Stats` struct and a `...Services` struct.

| File | Contents | Phase |
|---|---|---|
| `include/halo2/native_tu15_object_relocation.hpp` | entry, options, services, stats | 1 |
| `src/physics/native_tu15_object_relocation.cpp` | B95D0, B9460, 10E340/E24F0, BA7E0, B7E50 | 1 |
| `src/physics/native_tu15_cluster_reference_release.cpp` (+ `.hpp`) | 1D30F0, and later C5250 | 1 |
| `include/halo2/native_tu15_object_relocation_binding.hpp` | production builder: binds each service to a native or a residual call (cf. `native_tu15_object_deletion_spatial_binding.hpp`) | 1 |
| `src/physics/runtime/native_tu15_object_relocation_bridge.hpp` | `native_tu15_relocation_forward(State&, entry)` and the bypass flag | 1b |
| `src/physics/native_tu15_object_relocation_oracle.cpp` | per-call and scope-level oracles (§4) | 1, 2 |
| `src/physics/native_tu15_cluster_partition.cpp` | 1D2F10, 14D080, 14D0E0, 14CEA0, 123210, 243450 | 3 |
| `src/physics/native_tu15_object_map_reconnect.cpp` | BA630, B8D90, B92C0, B9330, 1CC290 | 3 |
| `src/physics/native_tu15_object_node_rebuild.cpp` | BF050, BF0C0, BFF20, BFC70, 1707A0, leaves, 10E180 + hooks | 4 |
| `src/physics/native_tu15_component_set_position.cpp` | 1CD620, 1DC270, 1DC610, 1DC680, E32C0, 1D8ED0, 1D8FA0, 1E1130, 323B20, 2E4350, 2E4690, 2E4470, 2E4820, 2E4650, 2E44D0 | 5 |
| `cmake/NativeObjectRelocationTu15.cmake` | sources added to `halo2_native_pair_consumer` (or a sibling static library with the same strict FP flags), included from `CMakeLists.txt` | 1 |
| `tools/specialize_native_operands.py` | `RELOCATION` table with body SHA-256 values, alongside `LEAVES` | 1b |
| `tests/native_tu15_object_relocation_tests.cpp` | synthetic leaf tests, plus a live test that skips (return code 77) without assets | 1+ |

Edits to `physics_live_world_tu15.cpp` stay small and local:
- the plug-point-A block;
- `present_object_position` (implementation plus the
  `include/halo2/physics_live_world_tu15.hpp` declaration);
- the oracle hook;
- stats.

Other sessions edit these files concurrently.

---

## 4. Part 3: the oracle

### 4.1 Switches

Values are read once with `getenv`, in house style. A list value names
entries or subtrees: `b95d0`, `b9460`, `ba7e0`, `1d30f0`, `ba630`, `bf050`,
`1cd620`.

| Switch | Meaning |
|---|---|
| `HALO2_REFERENCE_OBJECT_RELOCATION=1` | Run the translated B95D0/B9460 route exactly as today (plug point A falls through; the splice returns false). |
| `HALO2_REFERENCE_OBJECT_RELOCATION=<list>` | Keep only the listed subtrees translated. Their services bind to residual calls. |
| `HALO2_VERIFY_OBJECT_RELOCATION=1` or `<list>` | Per-call oracle (§4.2) for the listed entries or subtrees. Throws `FormatError` on the first difference. |
| `HALO2_REFERENCE_PRESENTED_HAVOK=1` | Disable the Phase 2 elision. 1CD620 runs inside the presented scope as today. |
| `HALO2_VERIFY_PRESENTED_HAVOK_ELISION=1` | Scope-level oracle (§4.5). |
| `HALO2_RELOCATION_PATH_CENSUS=1` | Phase 0 census (§3.1). |

### 4.2 Per-call comparison: candidate under undo, then reference live

For each verified call, at plug point A, with the input ABI `in`:

```
candidate:
  { SourceRuntimeUndoJournal undo(bank, packed, throw_on_topology_change);
    record C = tee(store observer -> C.stores (pc,address,before,written), registered collector);
    abi_c = in;  native_tu15_object_relocation_run(bank, entry, abi_c, services(), C);
    C.final  = bank bytes at every address in C.stores (after the run);
    C.exit   = abi_c (gp, eflags, xmm, x87 depth/status, mxcsr);
    C.host   = deltas of host effect counters (§4.3 L5) }
  // undo destructor restores every store in reverse order
  check rollback: every address in C.stores holds its earliest 'before' byte
reference (live):
  record R likewise; abi_r = in;
  with bypass: run the translated body through the owner the chain would pick
               (death cycle for B95D0, live support for B9460);
  R.exit = abi_r
compare (§4.3); on any difference throw FormatError("Native TU1.5 relocation differs ..."),
  reporting the first differing pc/address/expected/actual
a = abi_r   // the live result is always the reference's
```

Why the candidate runs first, under undo:
- A native bug can never corrupt the live state.
- Persistent resource admissions made inside the candidate (for example 126200
  reached through a residual BF050) are rolled back by the undo journal, then
  redone by the reference.

Constraints and caveats:
- `SourceRuntimeUndoJournal` requires no topology change. The presented scope
  already enforces that.
- Within the scope, verification roughly triples the journal traffic:
  candidate stores, rollback writes and reference stores. Keep an eye on the
  16 MiB bound (`source_presented_model_pose_tu15.cpp:264,289`). Verify with
  fewer players, or raise the bound only in verify mode.
- Residual translated callees run twice, so host counters double. Compare
  per-run deltas, not absolute values.

### 4.3 What is compared

| Level | Requirement | Status |
|---|---|---|
| L1 final bytes | For every address in `addrs(C) ∪ addrs(R)` outside the call's stack range: (`C.final[a]` if the candidate stored to `a`, else the pre-call byte) equals the live byte after the reference run. | required |
| L2 store sequence | The ordered sequences of `(pc, address, width, written)` for non-stack stores are identical. This covers nested residual translated stores, which must also match. | required |
| L3 stack | Final bytes in `[lowest ESP touched, entry ESP + 4 + argument bytes)` are identical, and the stack store sequence is identical. | required while any callee in the subtree is translated; diagnostic afterwards (§2.7) |
| L4 exit state | Identical: ESP, EBX/EBP/ESI/EDI, EAX/ECX/EDX, the six arithmetic flags, XMM0-7, x87 depth and status word, MXCSR. | required. Relax a field only with a written dead-value proof per caller (§2.8). |
| L5 host effects | Identical deltas: `model_attachments` and `vehicle_input_resets` event captures; the `run_actual` handoff counters (contact features, simplex, vehicle flip, object velocity, lift, rate, schema); `geometry_request_stats`; the list of persistent-region writes (address, bytes). | required |

### 4.4 Subtree oracles

- The same harness is keyed by subtree. With `HALO2_VERIFY_OBJECT_RELOCATION=ba630`:
  - the B9460 driver runs as configured;
  - each BA630 service invocation is verified separately: the native BA630
    candidate runs under undo, then the translated `run_ba630` runs as the
    reference through a residual call.
- So each phase can be proven while its neighbours are still translated.
- Pure leaves (14CEA0, 243450, 144CF0, 145560, BFC70, E32C0, 2E4350) can also
  use the matrix-leaf overlay technique:
  - compute in a `PureIo` overlay;
  - re-enter the original with the native route bypassed;
  - compare bytes, registers and flags.

  See `native_tu15_matrix_leaves.hpp:260-296`.

### 4.5 Scope-level oracle (for Phase 2)

`HALO2_VERIFY_PRESENTED_HAVOK_ELISION=1`, inside the presented-scope
constructor, wraps the root loop (`:326-362`).

**Candidate run, under undo:**
1. Run the loop with the elision enabled.
2. Snapshot every touched byte that is outside the caller stack range and not
   in a Havok-owned region.
3. Record each call's exit FP state, the persistent-region writes and the host
   effect deltas.

**Rollback**, then **reference run, live:** run the loop with the elision
disabled.

**Compare:**
- the non-Havok bytes over the union of touched addresses;
- the persistent writes;
- the host effects.

Report differences by owner and by body field offset. The census code
(`census()` and `relocation_diagnostics()`) already classifies them that way.

**Havok-owned** means the region-metadata owners of the physics-pool heap, as
printed by `HALO2_RELOCATION_CHANGE_CENSUS`. The exact owner strings are
**OPEN**. Confirm that every 1CD620-subtree store lands in those owners. The
store PCs to check are:
- 0x323B3B;
- 0x323B41 / 0x323B48 / 0x323B50 / 0x323B58;
- 0x2E4387 / 0x2E4391 / 0x2E439C / 0x2E43A7, or the else-branch sites;
- 0x2E46D5;
- 0x323B6F;
- the 315CB0 subtree.

### 4.6 Frame parity

Use `tools/perf_4k60/parity.py` through `tools/run_test.py`. Run reference and
candidate back-to-back in one session, because Vulkan pipeline-cache warmth
changes 4p output between sessions.

```
python3 tools/run_test.py -- python3 tools/perf_4k60/parity.py <report.json> \
    "HALO2_REFERENCE_OBJECT_RELOCATION=1@$PWD/build/halo2_native" "$PWD/build/halo2_native"
```

Cases (the twelve from the goal doc): Zanzibar and Coagulation; 4:150, 2:240
and 1:100 with `--motion --render-fraction 0.5`; plus the default endpoint
cases.

Repeat the set with `HALO2_REFERENCE_PRESENTED_RELOCATION=1` on both sides.
That relocates every root, which maximizes coverage of path classes. Also run
the set with `HALO2_VERIFY_OBJECT_RELOCATION=1`. It must complete without
throwing.

For Phase 2, also run:
- `HALO2_REFERENCE_PRESENTED_HAVOK=1@runtime` against the default;
- `HALO2_VERIFY_PRESENTED_HAVOK_ELISION=1`.

Measure frame time per map at 1p and 4p after every phase (`tools/perf_4k60/ab.sh`).

### 4.7 Tests and storage

- CMake validation goes through `tools/run_tests.py`.
- One-off probes go through `tools/run_test.py`, with outputs under
  `HALO2_TEST_SCRATCH` (AGENTS.md).
- Synthetic tests build small `SourceRuntimeMemory` banks covering:
  - B7E50: modes 3/4/5;
  - B9390: NONE, already marked, ancestor chain;
  - 1D30F0: list shapes;
  - the bounds check: ±32768 edges, NaN, -0.0, denormals, with MXCSR sticky
    flags compared;
  - 14CDB0..243450: a crafted BSP and portals, including the PC53/PC64
    adversarial case from [reconnect] §4;
  - BFC70 / 144CF0 / 145560 / E32C0 bit patterns.
- Live tests skip with return code 77 when the authority cache is missing.

---

## 5. Part 4: skipping the Havok update inside the presented scope

### 5.1 What the poison experiment does and does not prove

- If frames stay identical, the **render** work running inside the scope
  reads none of the Havok bytes that the relocation changed. Combined with the
  scope's rollback, the Havok changes never escape the frame.
- It does **not** cover reads that happen **inside the relocation loop**,
  after 1CD620:
  - by the same root's BF050 and BA630;
  - by later roots' relocations.

  Those reads happen before the poison is written. Skipping 1CD620 can change
  what they compute, and their outputs are non-Havok bytes: node banks,
  cluster spheres and lists, header bits, component flags.

### 5.2 In-scope consumers of 1CD620's writes

| Consumer | Reads | When |
|---|---|---|
| BF0C0 → 1DBBD0 → 1D8B90 (same root) | rigid-body motion `+0x80..+0xBF` written by 323B20 | `body+0xB4 != NONE`, `body+0xC0 & 0x40`, component `row+4 & 0x200` clear, `row+0x74 > 0` ([nodes] §4.16 step 1, [havok] §4.2). For bipeds (controller flags 0x203) and single-root 0x200 components, 1DBBD0 returns at once. |
| BFF20 → 1DC7A0 (same root) | rigid-body AABB through a Havok shape virtual, which depends on the motion transforms | type 11 crate, with a component, and `+0xC0 & 0x40`. It writes `+0x30/+0x3C` (overwritten later by the scope) and feeds `+0x40/+0x4C`, which BA630 uses. |
| BA630 → B92C0/B9330 → 1CC3F0/1CC440 → 1CC310 (same root) | component flags and Havok activation state | only when BA630 toggles header bit 0 (an activation transition) |
| BA630 tail: 1496F0, 1CC290, 282C10 | Havok world | only when the root was previously unconnected and is now clustered |
| Later roots' 1CD620, when not elided | island and broadphase state shared across roots | always possible |
| Render kernels during the scope | ? | the poison experiment |
| Next 1DC270 on the same component | component flag 0x10000 | not within one scope (each root is relocated once) |

### 5.3 Non-Havok writes of 1CD620, by path class

These are the stores that must still be reproduced, or that force a refusal.

| Class (from [havok] §4.3) | Non-Havok writes | Elision |
|---|---|---|
| No component | none (11-instruction early return) | nothing to elide |
| Gated: `0x2 && pos_null`, `0x100`, `0x80 && !allow80` | none | nothing to elide |
| 0x200, body not fixed, island active | stack only: BC190/E32C0/1D8ED0/1D8FA0/315C90 scratch | **admit** when §5.2 shows no consumer. Nothing to reproduce except optional stack residue (§2.7). |
| Multi-body, no fixed body, all islands active | component `row+4 \|= 0x10000` (RMW, PC 0x1DC5F8); the rest is stack: 1DC610/1DC680 argument-slot write, 1D8B90/144090/145560 scratch | **admit** when 1DBBD0 does not deliver (`+0xC0` bit 6 clear) and the object is not an active crate. **Reproduce** the 0x10000 RMW natively: one store, which keeps the in-scope state identical apart from Havok memory. |
| Any body whose island is asleep (315CB0 runs) | 3165D0 deferred removal, 316D40 → 322660 → listener 282870 (collision-agent re-creation). Writes to game memory are **OPEN**. | **refuse** until the scope oracle shows no non-Havok writes for this class |
| Any fixed body in an in-world component (1D9510 + 1E1130 + 1D97F0) | `0x48B868` −1 then +1; component `+4` (`&~0x20`, then `(&~8)\|0x20`, which clears bit 3 overall); component `+0x88/+0x8C/+0x90` reset when `flags&1`; `*(row+0x94)+4 = 0`; biped `+0x3E4 = -1` for bipeds standing on the component (not restored by the insert); BBBC0 wakes and 22D0A0 model-constraint removals (stores **OPEN**); 282C10 twice (can delete objects through BA570, defer objects with `+0xC0 \|= 0x40`, destroy components); 1CDD60 eviction at 850 bodies (**OPEN**) | **refuse** at first. Admit only if the census and scope oracle show the **OPEN** owners make no stores for presented roots. Then reproduce the deterministic non-Havok stores natively, in original order, with original PCs. |
| 0x400 rebuild | 1D8470 component fields; BF050 (object nodes, spheres, child reconnects, **persistent** animation resource admissions through 126200); 1D73D0 (row flags, row+0x1C, object `+0x12C` bit 0) | **refuse**. Persistent writes would escape the scope. |

### 5.4 Admission classifier

The native B9460 runs the classifier in place of the 1CD620 call, only when
`options.elide_presented_havok` is set:

```cpp
// Same reads as 1CD620/1DC270 up to the path split; no stores.
HavokClass classify(object, pos_null=0, allow80=physics_force&0xFF) {
  comp = body+0xB4;          if(comp==NONE) return NoComponent;
  f = row(comp)+4;
  if((f&2)&&pos_null || (f&0x100) || ((f&0x80)&&!allow80)) return Gated;
  if(f&0x400) return Rebuild;
  for each body the path will touch (slot 0 if f&0x200, else every slot i < row+0x74):
     rb = slot+0x40;
     if(rb[0x40] && (f&0x20)) return FixedInWorld;
     if(rb[0x44] && !island(rb)[0x29]) return Asleep;
  return (f&0x200) ? Single : Multi;
}
admit = class in {Single, Multi}
     && !(class==Multi  && (body[0xC0]&0x40))        // 1DBBD0 would read motion
     && !(type==11      && (body[0xC0]&0x40))        // 1DC7A0 would read the AABB
     && phase0_admitted(class)                       // per-class allow list from evidence
     && !HALO2_REFERENCE_PRESENTED_HAVOK;
if(admit){ if(class==Multi) store(0x1dc5f8u,row+4,f|0x10000u); /* skip the rest */ }
else       call 1CD620 (native when Phase 5 lands, else residual)
```

**Tripwire.** If BA630 then toggles header bit 0 after an elided 1CD620,
count it.
- In verify mode, it fails the call.
- In production it stays benign only if the scope oracle has shown no
  difference for that case. Otherwise remove the class from the allow list.

### 5.5 Requirements checklist (all must hold before the default flips)

1. **R1, render consumption.** The poison experiment is frame-identical on
   all twelve parity cases at 1p, 2p and 4p on Zanzibar and Coagulation, and
   on at least one map with crates and vehicles in motion. The poisoned owner
   set must cover every byte the admitted classes' 1CD620 would change
   (§4.5 PC check).
2. **R2, in-relocation consumption.** The scope-level oracle shows **zero**
   non-Havok byte differences over the same runs. This covers the same root's
   BF050/BA630 and all later roots.
3. **R3, no escape.** No persistent-region writes are attributable to the
   admitted classes' 1CD620. Census: `persistent_writes` deltas around each
   call.
4. **R4, host effects.** The `run_actual` handoff counters and event captures
   are identical with and without elision.
5. **R5, FP state.**
   - Each public call starts from a fresh FP state (`caller_abi()` sets
     `{0x027F, 0x1F80}`), so sticky flags cannot carry across roots.
   - Within a call, MXCSR and x87 sticky bits entering BF050/BA630 differ when
     1CD620 is skipped.
   - The scope oracle must also compare each call's exit FP state, or show
     that no consumer reads it. The only candidate found is the CRT fmod
     0x37DC0A in BF9A0's `controls == 1` path, which may inspect status for
     its error path.
6. **R6, presented-only.** Elision is reachable only through
   `present_object_position` at the root frame (§2.5). The simulation callers
   and tests keep the full route.
7. **R7, gating.** Each class must be on the evidence-based allow list
   (Phase 0 plus R1–R5). The asleep, fixed and rebuild classes stay refused
   until their **OPEN** stores are characterised. The tripwire is enabled.
8. **R8, reproduce the cheap non-Havok stores.** Do the multi-body 0x10000
   RMW natively, so the in-scope non-Havok state is byte-identical, not just
   unconsumed.

### 5.6 Option for classes with in-scope consumers: the motion overlay

For multi-body components where 1DBBD0 delivers nodes, and for active crates,
the elision changes BF050's input.

Instead of refusing, a Phase 5 native 1CD620 can compute in "dry-run" mode the
exact hkTransform that 323B20 would store for each body:
1. 1DC270 math;
2. 1D8ED0's transform34 → hkTransform build;
3. 1D8FA0's optional `offset⁻¹` multiply;
4. the copy into `+0x80..+0xBF`.

These values go into a per-call overlay keyed by `rb`. A native 1DBBD0/1D8B90
(Phase 4 follow-up) reads the overlay instead of motion memory. The overlay
must reproduce the round trip bit for bit: `T·offset⁻¹` stored, then `·offset`
applied again on the read. This needs native 1DBBD0 and, for crates, native
1DC7A0, so it comes after Phase 4.

---

## 6. Part 5: risks

1. **Float order.**
   - SSE association must follow the specs:
     - the BFF20 sphere: `((up·oz + left·oy) + fwd·ox) + pos`;
     - 145560 lane grouping;
     - 14CDB0: `((k·z + j·y) + i·x) − w`;
     - 243450 edge tests;
     - 2E4690: `(c0·x + c1·y) + (c2·z + c3)`;
     - 1DC270's dot product.
   - Never use FMA. GCC defaults to `-ffp-contract=fast` in GNU mode, hence
     the explicit flags.
   - Use inline-asm scalar ops for every source SSE instruction.
   - x87 sections run under the captured control word, never with host
     `long double` defaults:
     - 14CEA0: PC53 distance, a reject test on the unrounded value, a
       projection on `float(d)`, a double sqrt;
     - the 1DC270 Gram–Schmidt (including an extended×float product);
     - 323B20's `+1e6` add;
     - 2E4350's quaternion;
     - BF9A0 and 1E1720.
2. **MXCSR and x87 sticky flags.**
   - The COMISS short-circuit order in the bounds check: NaN sets IE, a
     denormal sets DE.
   - Junk lanes:
     - 145560 computes lane 1 on zeros;
     - 2E4690 and 2E44D0 compute the w lane;
     - 144CF0 uses stale XMM1 lanes 2-3, whose contents depend on the
       preceding instruction stream ([nodes] §4.6).
   - Use packed ops with the same lane contents, as the existing kernels do,
     or prove the status is dead.
   - FP state resets at each public call. Within a call, compare it (oracle
     L4).
3. **Estimate instructions.**
   - The translated 144CF0 uses the host `_mm_rcp_ss` (`LS:78876`). A native
     must use the same host instruction.
   - Some translated units route RSQRTSS through the
     `services.reciprocal_sqrt` provider (`source_estimate`). A native must
     call the same provider wherever the translated routine does, and never
     the host `rsqrtss`.
4. **errno.**
   - The CRT path (fmod 0x37DC0A from BF9A0; the math-error service
     `source_math_error_tu15.cpp`, "native thread errno storage for original
     00330179") writes the source errno in emulated memory. A native must
     reproduce that store or refuse through the existing
     `SourceMathErrorServiceRequiredTu15` boundary.
   - Host errno must not change visibly:
     - use instructions (`fsqrt`, `sqrtss`), not libm;
     - save and restore errno around host allocations (precedent:
       `physics_live_world_tu15.cpp:305-307`).
5. **Persistent resource admissions.**
   - 1E3150 / 1E5C80 → 126200 (reached from BFB90, BF9A0 and 1CF210, and
     from BF050 inside the 0x400 path) write the persistent resource owners.
     The scope deliberately does not roll those back.
   - Natives must issue the same admissions, in the same order, under the
     same conditions.
   - Verify mode undoes the candidate's admissions.
   - Phase 2 must refuse any class whose skipped work would admit resources.
6. **Stack residue.** Render kernels reuse the caller stack and keep its
   residue. Keep L3 strict while any callee is translated (§2.7).
7. **Register outputs consumed by translated callers.** Candidates are EAX
   upper bits, B9460's ECX top byte, B95D0's ECX/EDX from B9390, and XMM0-2.
   The audit is **OPEN**. Reproduce the outputs until a per-caller proof
   exists.
8. **Reentrancy and stale caches.**
   - BF050 → BFF20 → BA7E0/BA630 (connected children).
   - 1CD620 → 1DC270 → BF050 (0x400).
   - BA630 → BA570 → 10E4B0 → B9390.
   - 282C10 can delete or defer objects in the middle of the relocation.
   - Re-read memory after every callee. Cache exactly what the original keeps
     in registers.
9. **Routing.**
   - Static calls bypass `required()`, so the splice is mandatory for full
     coverage.
   - The death-cycle copy wins for 0xB95D0.
   - B9390 resolves to the scene-device-constructor copy.
   - The live-support unit is split across 32 partitions, and the partitioner
     owns only hex-named bodies with the canonical trailer. Splices must keep
     the original body intact after the prefix, as the leaf and normalize
     splices do.
   - The SHA-bound splice silently keeps the translated route when a body
     changes. The stats must show that.
10. **Journal bound and topology.**
    - The 16 MiB presented journal can overflow in verify mode.
    - Any native allocation that commits pages trips the topology guard. None
      is expected, because Havok and object pools are pre-committed. Check
      this in Phase 6.
11. **Latent original edge cases.** Reproduce these, or refuse to the
    reference explicitly:
    - B8780 returning 0, so BF0C0 reads `[0x16]`;
    - 1E3150 with an out-of-range inheritance index reads `[0x30]`;
    - C1F00 with count 0 does an RMW of 0x56EB88;
    - 141010 with count 0 reads `mask-4`;
    - BA630's forced bit vector with more than 512 clusters;
    - 14CEA0 with more than 128 vertices;
    - B9460 reads `up` without a NULL check;
    - the death-cycle FAA23 site passes `EBX+0x64` / `EBX+0x28` (**OPEN**).
12. **Paths the parity cases do not exercise.**
    - out-of-bounds delete through BA570, and the raw-mode-4 keep;
    - machines with `physics_force=1` from static callers;
    - non-biped 10E340 handlers in other data;
    - the 850-body eviction;
    - lights attached to moved roots (C0F60 → C4D30/C3750).

    The per-call oracle must run on campaign and other MP maps before the
    defaults flip, not only on the two perf maps.
13. **Performance regression in Phase 1.** About 120 extra required-chain
    hops per frame could cancel the driver gain. Measure, and add the
    verified direct-owner fast path if needed (§2.6).
14. **Concurrent edits.** `physics_live_world_tu15.cpp`, the generators and
    the generated units are edited by other sessions. Keep changes surgical
    and prefer build-local splices over regenerating the 19.7 MB unit.

---

## 7. OPEN items

- **Path-class mix of the ~40 presented roots per frame on Zanzibar 4p.** We
  do not yet know which classes carry the 5.4% under 1CD620: fixed-body
  churn, island wake-ups, multi-body math or 0x400. Phase 0 measures it.
- **Result of the running poison experiment.**
- **The physics-pool heap's region-metadata owner strings**, and whether the
  component pool rows (`0x52BE5C`) and Havok motion memory sit in the
  poisoned owner set.
- **Unknown stores in 1CD620's asleep, fixed and 0x400 paths:**
  - stores made by 316D40 → 322660 → listener 282870;
  - BBBC0 and 22D0A0;
  - 1CDD60;
  - the hkWorld → broadphase chain ([havok] §7).
- **Activation transitions inside the scope.** How often BA630 toggles header
  bit 0 during the presented relocation (tripwire count).
- **XMM0-2, EAX upper bits and B9460's ECX after the call.** Whether any
  translated caller consumes them.
- **NULL forward/up.** Whether forward and up can be NULL at the B9460 sites
  in BAF10, BB300 and BB3E0.
- **Full-width register values at the call sites.** The exact EBX, EBP, ESI
  and EDI values the original holds at each B95D0/B9460 call site are needed
  for stack-residue parity. Transcribe them from `LS:7985-8190`.
- **Whether the CRT fmod 0x37DC0A consults x87 status sticky bits.** This
  decides R5's relevance for Phase 2.
- **The semantic open items carried over from the four specs:**
  - raw mode 4;
  - `*[0x474CE8]`;
  - cluster-system naming;
  - component flag names;
  - BF0C0, BFB90 and 1DBBD0 internals;
  - C4D30 and C3750.
