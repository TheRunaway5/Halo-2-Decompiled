# Object tick children: BE850 / BE610 / C0F60 / 10DD80 / C0200 / 1D5790 / 1E15D0 / C0910, and the inline 14DD70

Continues `B8F40_object_tick.md` (remaining work items 2, 3 and 4). It uses the
same method and machinery (`Machine`/`Trace`/`Replay`/`execute` from
`native_tu15_object_tick.hpp`).

| Owner | Caller | Role | Census before (instr/frame) |
|---|---|---|---|
| BE610 | BE850 | cluster (phantom) move: 1D9EC0/1D9F50/1DC610/BB200/B9460/1D8C80/1D8D80/B9770, parent flag, game-mode release (1E1980), +0xC1 bit; RET 4 | 949 |
| C0910 | BE4A0 | node lights: per row, base colour copy, weighted contributions (BCB70, 134730 with XMM5), scales (BCB70, MULSS), clamp to [0, [46A0D0]] (COMISS); RET 4 | 840 |
| BE850 | B9090/B9180 | post-physics move of one object and its children (recursive): BE610, BF0C0 (required) or the cluster-flag frame clear (REP STOSD/STOSB) and 1DBBD0, 10DD80, parent-flag bookkeeping, BFF20 (required), C0F60, child chain, C0200; RET 4 | 788 |
| C0F60 | BE850 | attachment refresh: BBD10, node attachments (C5250/C4D30), placement (C3750), children (recursive), siblings (in-frame loop); RET 0x14 | 639 |
| 10DD80 | BE850 | type move callbacks `[474B90+type*4]+0x84 -> [comp+0x44]`, object in EDI; RET 0 | 529 |
| 1E15D0 | 1D5790 | one countdown timer at ECX (counter, limit, flags +3); RET 0 | 510 |
| C0200 | BE850 | node notification (BAC30, 10E1F0) when the model has both node tags; RET 0 | 446 |
| 1D5790 | BE4A0 | the two timers at ESI+0x64/+0x60, reset flag, AL = OR; RET 0 | 290 |

The census figures are from the B8F40 checkpoint ranking (honest Armory
census, `HALO2_CENSUS_NATIVE_BRIDGES=1`).

## Native route

Rewritten to the code standard on 2026-11-16, with the same split as the
object tick owners (`B8F40_object_tick.md`, "Native route"): game logic, plus
a thin boundary adapter. The machine-shaped closures are gone.

**Logic** (`native_object_subtree` in
`src/physics/runtime/native_tu15_object_subtree_systems.hpp`; it reuses the
object tick layouts and adds `NodeLightAttachment`, `TagInstance`,
`ObjectDefinition`, `ModelDefinition`, `LightBlockRow`, `LightEntry`,
`ColourRow`, `CountdownTimer` and their flag namespaces):

| Owner | System | Kept across child calls | Result |
|---|---|---|---|
| BE850 | `object_move::run` | `Move` (handle, object, header, child) | none |
| BE610 | `cluster_move::run` (`move_phantoms`) | `ClusterMove` (handle, object, record, in_cluster) | in_cluster (the exit) |
| C0F60 | `attachment_refresh::run` | `Refresh` (handle, object, node entry, remaining, blocked) | none; the sibling loop continues through the frame |
| 10DD80 | `type_moves::run` | `ChainWalk` (handle, slot) | none |
| C0200 | `node_notify::run` | none | none |
| 1E15D0 | `countdown::run` | none | idle, held, running, reached_limit |
| 1D5790 | `timers::run` | `Timers` (block, timer, first) | the OR of both timers |
| C0910 | `node_lights::run` | `LitRow` (light row, lit row, entry, scale index and offset) | exit, rows, value |

The frames supply the stack arguments (C0F60's five, read where the original
reads them), the frame slots (BE610's object slot and phantom result byte,
C0200's matrix slot that BAC30 fills, C0910's row locals), C0F60's argument
rewrite for its sibling loop, BE850's node bit clear, and every child call.
C0910's colours move as bit patterns; MULSS keeps the destination-operand
order and COMISS/JA runs on the live MXCSR (`native_object_subtree::sse`).
The original's divide-by-multiply formulas (BE850's node bytes by 0x34,
C0910's rows by 24) are written out exactly, with comments.

**Adapter** (`native_object_subtree_detail` in
`src/physics/runtime/native_tu15_object_subtree.hpp`): one boundary per owner
on `boundary::Port`, with role -> instruction tables, register effects, the
frame, the epilogues and the coverage edges, as for the tick owners. Special
cases:

- BE850's REP STOSD/STOSB node bit clear steps in the live direction flag
  (`j_clear_up`/`j_clear_down` are marked from the flag);
- BE610 keeps the handle in EBX, then in ESI after the collision query; the
  adapter maps it per call site;
- C0F60's node walk mirrors the original's ESI/EBP stepping (ADD 8, DEC)
  between calls;
- C0910's XMM0/1/2/5 (all lanes) are rebuilt from the stored bits and the
  loads, so the adapter does no float arithmetic;
- the adapter takes every derived value the logic passes (BE610's record,
  BE850's record push, C0F60's reconnected light, B9770's default, 134730's
  weight) and derives only flags from it; it recomputes nothing the logic
  decides;
- outcomes that the instruction order cannot separate are decided by the
  value read at the next access (`j_attach_changed`/`j_not_attached`,
  `l_node_none`/`l_node_blocked`).

As before, every original memory access happens in program order at the same
address and width through the unit's store helper, partial-register writes
keep the upper bytes, flags are those of the deciding instruction, and the
cursor is published before each access and set to the final RET.

Every child stays genuine and is entered through the unit's own route with
the exact register, flag, XMM, x87 and stack ABI:

| Closure | Children (route) |
|---|---|
| BE850 | BE610, 1DBBD0, 10DD80, C0F60, BE850, C0200 (direct); BF0C0 at BE892 and BFF20 at BE941 (required owner) |
| BE610 | 1D9F50, 1DC610, BB200, B9460, 1E1980 (direct); 1D9EC0, 1D8C80, 1D8D80, B9770 (dispatch) |
| C0F60 | BBD10, C5250, C3750, C0F60 (direct); C4D30 at C100F (required owner: the light callers' boundary) |
| 10DD80 | each move callback `[comp+0x44]` (dispatch) |
| C0200 | BAC30, 10E1F0 (direct) |
| 1D5790 | 1E15D0 (direct, twice) |
| 1E15D0 | none |
| C0910 | BCB70 (twice per entry), 134730 (direct) |

BF0C0/BFF20 go through `v.required(site,target,false)` at their original
sites, so the live world's node-rebuild routing (`node_rebuild_site`, opt-in
`HALO2_NATIVE_NODE_REBUILD_TU15`) is unchanged.

Admission is the object tick admission: TU1.5 image, no active trace,
observation-only stores, literal cursor or no instruction callbacks, no
service site in the closure, and `HALO2_CENSUS_NATIVE_BRIDGES` honoured.

### Reuse of the object-factory C0200/C0910

The object-factory tracks hold semantic C0200 (`object_notify_nodes`) and
C0910 (modelstate) natives. They operate on the memory alone: they do not
replicate the stack frame, register results, flags or the child ABI, and they
refuse callbacks other than 1157D0. The tick path needs the ABI-exact closure
(BE850 consumes C0200's registers, BE4A0 consumes C0910's AL and flags, and
the record/replay verifier checks every ordered access), so those natives are
not reused here. The two are independent routes to the same original bodies.

### The inline 14DD70 (131470)

`native_tu15_cluster_distance_flood.hpp` now runs the proven leaf kernel
(`kernel_14dd70`, `native_tu15_matrix_leaves.hpp`) inline at 131470's
original child boundary. Admission is the scene device binding's:
`x87_room(s,2)` (two logical slots, masked x87 exceptions, no MMX), a literal
cursor or the census diagnostic, and outside `HALO2_REFERENCE_MATRIX_LEAVES`
and `HALO2_VERIFY_MATRIX_LEAVES` (the leaf verifier re-enters a translated
body that the environment unit does not hold). Otherwise the unit's dispatch
(the required owner round trip) runs it, as before.

The round trip saves and reloads the complete ABI unchanged under masked
controls (its load refuses unmasked controls, and 131470 is admitted only
with masked MXCSR), so both routes leave the same registers, flags, XMM, x87
lanes/status/C1 and MXCSR. The child boundary stays in the verify record.

### Switches

- `HALO2_REFERENCE_OBJECT_SUBTREE=1` restores the eight translated bodies.
- `HALO2_VERIFY_OBJECT_SUBTREE=1` records and replays every admitted call
  (children run once), as `HALO2_VERIFY_OBJECT_TICK` does. The summary line is
  `Object subtree verify: ...`.
- `HALO2_REFERENCE_CLUSTER_DISTANCE_CHILD=1` keeps the native 131470 but
  enters 14DD70 through the dispatch round trip.
  `HALO2_REFERENCE_CLUSTER_DISTANCE=1` still restores the translated 131470.

### Binding

`tools/native_object_subtree_replacements.py` (marker `run_be850`) is called
from `spatial_owner_bindings` in `tools/specialize_native_operands.py`, after
the object tick binder. The receipt key is `native_spatial_owners`. It is a
build dependency in `cmake/NativeOperandSpecialization.cmake`.

Each canonical body is pinned (state type normalized). Every direct child must
keep its body in the unit and every routed child its original route text
(the required sites of BE850, the four constant dispatches of BE610, the
indirect dispatch of 10DD80). Otherwise the translated route is kept. The
replay references are the canonical bodies with only their child calls
(`run_X(s)`, `dispatch(s,...)`, `s.required(...)`) routed to
`native_object_tick_detail::replay`, in the unit prefix.

## Proof

`tools/validate_native_object_subtree_tu15.py` (CTest
`native_tu15_object_subtree`, registered in `cmake/NativeTu15ObjectTick.cmake`)
compiles `tests/native_tu15_object_subtree_tests.cpp` against the pinned
bodies and generated references. Every child, including the required-owner
children, is a stand-in whose effects are a hash of its complete entry ABI,
return address, arguments and the flag bytes it can see (stores into the
object table and data, AL results, callee-saved clobbers, stack imbalance,
flags, XMM; BCB70 writes its weight through its out pointer and 134730
accumulates into the row at EDI).

Fixtures author complete worlds: object tables with acyclic child/sibling
chains, type move callback chains (negative type bytes, empty and null
callbacks), cluster records (+4 flags, +0x74 counts including zero and
negative), model tags (missing model and node tags), node attachment lists,
countdown timers, node light rows with contribution/scale lists and special
floats (NaN payloads, infinities, denormals, -0, a NaN-dense shape for
dual-NaN MULSS operands) under every MXCSR rounding/DAZ/FTZ mode, both
direction flags, invalid handles, and fault injection (one page removed).

Each case compares ordered stores (pc, address, preimage, bytes), the child
call log, the whole bank, GP, EFLAGS, XMM, x87 status, MXCSR and the cursor.
Fault cases compare the exception, cursor and store prefix.

| | Object subtree |
|---|---|
| Release comparisons | 14,000 (1,750 per owner) |
| Ordered stores | 278,399 |
| Genuine child calls | 38,590 |
| Fault cases | 2,198 |
| Census mode | 3,500 |
| Verify mode (record/replay) | 3,500 |
| ASan/UBSan | 2,800 + 500 verify |
| Branches | 94/94 |
| Mutants rejected | 63/63 (default mode, then verify mode); after the 2026-11-16 rewrite 78/78 |

The rewrite (2026-11-16, report
`build/test-results/object-update-legibility-20261116/object-subtree-rewrite.json`)
gives the same figures with the unchanged harness: 14,000 comparisons,
278,399 ordered stores, 38,590 children, 2,198 faults, census 3,500, verify
3,500, ASan/UBSan 2,800 + 500, 94/94 branches. The mutation screen names the
file of each defect (`SYSTEMS` for the logic, `HEADER` for the adapter). It
carries the 63 defects ported to the new code and 15 new ones: the kept-struct
hand-back of each owner, the adapter's register mirror (C0F60's node walk,
BE610's extent local and index mask, 10DD80's slot, 1D5790's flags), the
logic's access order (BE850, C0F60's ready byte, C0910's colour copy) and
BE850's machine type test. The first full run's survivor was a real flaw:
BE610's adapter recomputed the cluster record address, which left the logic's
computation dead. The adapter now takes the record from the logic.

Two candidate mutants were dropped as equivalent: BE850's `(mulhi>>4)+0x1F` is
always positive (SAR equals SHR there), and its byte count `edx&3` is always
zero (EDX is a multiple of four, so the REP STOSB never stores).

Refusals checked: MapPack11, full callbacks without a cursor, an active trace,
an arbitrary observer. Binding checks: install, idempotence, a changed BE610
body, C0F60 without its C3750 body, BE850 with a changed BF0C0 route, C0F60
with a direct C4D30 call (since 2026-11-16), and references in the unit
prefix.

### C0F60's C100F boundary (2026-11-16)

The light callers' work (`tools/regenerate_light_caller_boundaries_tu15.py`)
re-emitted C0F60's C4D30 call at C100F as a required boundary
(`s.push(0xc100fu,0xc1014u);s.required(0xc100fu,0xc4d30u,false);`). That
changed C0F60's canonical body, so its pin no longer matched, the binder kept
the translated C0F60, and the validator stopped before its own checks.

C0F60 is native again:

- the pin is the new body's (`ec94382b…`);
- `ROUTES['c0f60']` requires the required-boundary text, so a direct
  `run_c4d30` call (or any other route) refuses the binding;
- `REQUIRED_CHILDREN` has `('c0f60', 'c4d30')`: the native bridge's C4D30
  child leaves through the World's required owner at its original site
  (`v.required(ret-5, 0xc4d30, false)`), where the light callers' binding runs
  placement's native `light_reconnect` by default and the unit's C4D30 under
  `HALO2_REFERENCE_LIGHT_CALLERS_TU15=1` (`physics_live_world_tu15.cpp`,
  `light_caller_site`);
- the harness's required-owner stand-in accepts C4D30 at C100F, and the
  native's C4D30 child takes the production route; the reference routes the
  translated `s.required` to the replay as before.

Proof on the machine-shaped closures, before the rewrite (report
`build/test-results/object-update-legibility-20261116/subtree-c0f60-repin.json`):
the figures above unchanged (14,000 comparisons, 278,399 ordered stores,
38,590 children, 2,198 faults, census 3,500, verify 3,500, ASan/UBSan
2,800 + 500, 94/94 branches, 63/63 mutants).

`tools/validate_native_cluster_distance_flood_tu15.py` (CTest
`native_tu15_cluster_distance_flood`) adds inline modes. The native closure
runs the leaf kernel on its own state, while the translated 131470's dispatch
reaches the unmodified translated 14DD70 of the scene device unit (pinned to
its leaf pin) through a modeled required-owner round trip. Those fixtures
author real portal segments (row table, origins, radii, out-of-table rows),
x87 stacks of depth 0..7 (7 keeps the round trip), every x87 precision and
rounding control and the MXCSR modes; x87 lanes, status, C1 and control word
are compared too. MXCSR is compared on completed cases: a fault inside 14DD70
stops the game, and the round trip's fault path reloads the caller's saved
controls.

| Cluster distance | Comparisons | Faults | Ordered stores |
|---|---|---|---|
| Dispatch route (stand-in): release / census / verify | 8,000 / 2,000 / 2,000 | 2,108 | 845,366 |
| Inline route: release / census / verify | 8,000 / 2,000 / 2,000 | 1,915 | 765,701 |
| ASan/UBSan dispatch / inline | 1,600 + 400 / 1,600 + 400 | | |
| Branches | 17/17 (adds `inline_child`, `routed_child`) | | |
| Mutants rejected | 24/24 (adds inline room, double kernel, missing fallback); 30/30 after the 2026-11-16 rewrite | | |

The dispatch-route figures are identical to the B8F40 checkpoint.

## Production results

The binary was `halo2_native.subtree` (sha256 `aa06c265…`, removed after the runs), built in the shared
`build-perf-4k60` tree. Its specialized live-support unit carries all eight
bindings and the reference block (receipt `native_spatial_owners` adds
`be850`, `be610`, `c0f60`, `10dd80`, `c0200`, `1d5790`, `1e15d0` and
`c0910`); the environment unit carries the inline 14DD70 through the
cluster-distance header.

### Armory parity

01a, deterministic `--campaign-reference-capture`, 300 frames at 1280x720.
Each route gives frame `5627a51badf47762` and trace `8a9b58b38bef`:

- reference (`HALO2_REFERENCE_OBJECT_SUBTREE=1 HALO2_REFERENCE_CLUSTER_DISTANCE_CHILD=1`);
- native under the Khronos validation layer (0 messages);
- the reference again;
- verify (`HALO2_VERIFY_OBJECT_SUBTREE=1 HALO2_VERIFY_CLUSTER_DISTANCE=1 HALO2_VERIFY_OBJECT_TICK=1`);
- both census runs.

The verify run checked every call:

- `Object subtree verify: BE850 3874, BE610 3398, C0F60 4110, 10DD80 3874,
  C0200 4042, 1D5790 7934, 1E15D0 20318, C0910 11450 calls; 791945 ordered
  accesses (251515 stores) and 64784 child boundaries identical`.
- `Cluster distance verify: 300 calls; 978600 ordered accesses (235500 stores)
  and 51300 child boundaries identical`. Every one of the 51,300 14DD70
  boundaries took the inline kernel (coverage `inline_child` set,
  `routed_child` clear), so no `s.required` round trip remains in 131470.
- `Object tick verify` is unchanged from the B8F40 checkpoint (B8F40/B9090/
  B9180 450, BE4A0/10DD10 11282 calls; 3,059,873 accesses, 90,593 children).

Live branch coverage is 29/45 move and 18/49 attach branches (Armory),
33/45 and 23/49 with the MP runs added. The harness covers every branch.

### Multiplayer (Coagulation, `tools/perf_4k60/parity.py`, cases 4:90, 4:150 and 1:150 at 3840x2160)

Reference against native is identical in every case (`9c14a330…`,
`13fa114e…`, `05f31264…`, unchanged from the B8F40 checkpoint). Reference
against verify is identical too, and the `--keep-lines` verify run checked
every call:

| Case | BE850 | BE610 | C0F60 | 10DD80 | C0200 | 1D5790 | 1E15D0 | C0910 | Accesses | Stores | Child boundaries |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 4:90 | 2,356 | 894 | 10,265 | 2,356 | 2,457 | 1,721 | 8,277 | 1,853 | 602,543 | 175,340 | 47,001 |
| 4:150 | 3,535 | 1,188 | 18,540 | 3,535 | 3,840 | 2,930 | 12,420 | 3,624 | 986,659 | 283,024 | 74,648 |
| 1:150 | 2,199 | 659 | 13,637 | 2,199 | 2,345 | 1,606 | 6,672 | 2,070 | 659,690 | 183,210 | 48,598 |

All were identical. The tool reports FAILED only because the kept verify
line exists in the candidate role alone, as in earlier verify runs.

### Census (same binary, Armory, 300 frames, `HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1 HALO2_CENSUS_NATIVE_BRIDGES=1`)

| Run | Translated instructions/frame |
|---|---|
| Reference switches on | 368,244 |
| Native | 363,249 (−4,995, −1.36%) |

Removed per frame: BE610 949, C0910 840, BE850 788, C0F60 639, 10DD80 529,
1E15D0 513, C0200 446 and 1D5790 291, all now 0. (14DD70 and 131470 were
already 0; the inline removes the round trips, not census instructions.)
Elsewhere only timing noise moved: 12EDF0 +1.4, 140430 +0.7, 12F110 +0.1,
33B590 +0.1 and 217090 −1 per frame, with identical frames and traces.

The total is lower than the B8F40 checkpoint's 378,225 because concurrent
tracks landed in the same tree; both rows here are the same binary.

The native top entries are now: 136EA0 17.4k and 135970 16.7k (render `Frame`
bridges, native in production), 304350 10.4k, 2847D0 9.1k, 1F5FE0 8.2k, 296AD0
7.5k (node rebuild/codec), 166C00 7.0k (render bridge), 1362A0 6.7k, 134D30
5.7k, BF0C0 5.3k (node rebuild), 30E930 4.8k, 145560 4.0k, 1707A0 3.9k and
2EAFB0 3.9k.

## Remaining work (ranked; native census, translated instructions/frame)

1. **Device update callbacks** in the scene device constructor unit, reached
   from 10DD10 through `dispatch` -> required owner -> the device unit:
   - 1179C0 (machine), 3.3k: 472 instructions, 317 `selected()` operands
     (identity for TU1.5; the binder must check identity as the 14DD70 leaf
     binding does), 46 x87 operations on the logical `s.fp` stack (FLD/FABS/
     FCOMIP via `compare_and_pop`, C1 clearing) and 64 SSE;
   - 10B840 (machine, control, light fixture), 2.9k: 394 instructions, 15 x87
     and 59 SSE.

   The closure must keep `s.fp` lanes, count, TOP and status exactly (the
   14DD70 kernel shows the host-x87 discipline for FSQRT/FABS spills). Their
   children are 10C290, 10C5D0, 10CA30, 10CD70, 1181F0, 118350, 121890,
   1CE8D0, 1CEBD0, 1DA750, BAD60, BD080 (native) and BDA50 plus three
   dispatches each.
2. **D8330**, 1.8k (dispatched from BE4A0; damage delivery unit).
3. **1D6FB0**, 1.6k (BE4A0's cluster owner update: 191 instructions, SSE and
   x87 FSQRT/FDIV, nine dispatches, FA430 direct). Its dispatched children
   1D9EC0 (1.6k, phantom world; also BE610's first child) and 1D7260 (1.5k)
   follow.
4. **Profiler depth** (from B8F40): `tools/native_cpu_profiler.c` keeps 48
   frames, which truncates deep object-update stacks.

## Continuation notes (2026-11-15, read-only survey; nothing ported)

The next agent's response was stopped partway through, and it changed nothing.
Its survey found the following.

### 1179C0 and 10B840

These are the device update callbacks.

- **Location and state type.** They are in
  `src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp`,
  at about lines 6449 and 4761. The state is `LiveState` (alias
  `WorldState`). The unit is not partitioned and already takes
  `spatial_owner_bindings`; BD080 is bound there.
- **Operands.** Every `selected()` operand is the identity under TU1.5:
  102 operands in 1179C0 and 85 in 10B840. The `identity_operands` check in
  `tools/native_object_sphere_query_replacements.py` applies.
- **Dispatched children.** In 1179C0, B7E50, BBFF0 and BF050 go through
  `dispatch` → `s.required`. In 10B840, B8760 (twice) and BF050 do. Keep the
  BF050 route unchanged. Both functions return with RET 4, with exit flags
  from `ADD ESP`.
- **FP behaviour.** Both use the logical `s.fp` stack, at most two deep
  (FLD/FABS/FXCH/FSTP, and FCOMIP via `compare_and_pop` into `s.flags`).
  1179C0 also does FSUB/FMUL m32, FADDP, FST m32 (`native_single_spill`) and
  FISTP (`integer_spill`). The SSE work includes UCOMISS followed by
  LAHF/`test ah,0x44`/JP, so AH lands in EAX.
- **Admission.** Admit like 14DD70 (`x87_room(s,2)` with masked MXCSR).
- **Dead branch.** The fall-through at 1180E2 in 1179C0 is never taken,
  because BL is 0 there.

### Other owners

- **1D6FB0** is in `src/physics/physics_live_support_tu15_generated.cpp`, at
  about line 121408.
- **1D9EC0** is in `physics_phantom_world_tu15_generated.cpp`
  (`run_1d9ec0(MathState &)`), which has a different state type and
  generator.
- **D8330 and 1D7260** have no `run_` body in any unit. Before writing a
  binder, check how their generators emit them.

### 1179C0 and 10B840: done (2026-11-17)

Both device update callbacks are native and bound; see
[1179C0_10B840_device_update.md](1179C0_10B840_device_update.md) and the
"Device update callbacks 1179C0/10B840" section of
[REMAINING_TRANSLATED_20261115.md](REMAINING_TRANSLATED_20261115.md).
