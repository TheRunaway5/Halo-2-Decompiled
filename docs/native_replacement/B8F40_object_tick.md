# Object tick owners: B8F40 / B9090 / B9180 / BE4A0 / 10DD10, and 131470

Continues `BD080_object_sphere_query.md` (remaining work item 3). Six
translated owners at the top of the honest Armory census
(`HALO2_CENSUS_NATIVE_BRIDGES=1`) are now native closures:

| Owner | Unit | Role | Census before (instr/frame) |
|---|---|---|---|
| B9180 | live support | post-update: late updates (BE4A0/BE850), 102910, flag-0x10 objects (BA3E0/BA490), tail C13B0 | 15,065 |
| B8F40 | live support | object update: player roots (BE4A0, 10E630), then every active object (BE4A0) | 8,320 |
| 131470 | campaign environment | cluster distance flood through portals (14DD70 child) | 7,376 |
| B9090 | live support | post-physics: player roots (BE850, 10E710), moved objects (BE850), 102910 | 7,265 |
| BE4A0 | live support | update one object and its child chain (recursive) | 2,928 |
| 10DD10 | live support | type update callbacks of the object's type chain | 2,245 |

The "B8A30 10.6k" row of the earlier ranking is not a TU1.5 entry: no
translated TU1.5 routine owns PC B8A30 (only the MapPack11
`unit_object_runtime_generated.cpp` body uses that address, and the census
index excludes that image). The owners above are the honest object-tick
entries on the same 453,973-instruction/frame census.

B8F40, B9090 and B9180 are called once per tick from
`PhysicsLiveWorldTu15::Impl::step_objects` (`call_actual`), around the
native physics frame. BE4A0 is called per object by B8F40/B9180 and
recursively for children; 10DD10 is its type-callback loop. 131470 is called
by 131AC0 in the campaign environment unit.

## Native route

Rewritten to the code standard on 2026-11-16 (claims doc, "Legibility
refactor backlog"); the machine-shaped closures of 2026-11-15 are gone. The
owners are now game logic plus a thin boundary adapter, as group H's world
leaves are.

**Logic** (`native_object_update` in
`src/physics/runtime/native_tu15_object_tick_systems.hpp`):

| Owner | System | What it does |
|---|---|---|
| B8F40 | `update_pass::run` | each player root that is active, awake and not left to the late pass updates (BE4A0); a root that moved updates its attachments (10E630); then every other such object in data array order, unless disabled |
| B9090 | `move_pass::run` | each root that moved, and every other moved object that is not disabled, moves (BE850; roots also run 10E710); then 102910 |
| B9180 | `late_pass::run` | every in-use object loses its `held` flag; each late object that is active, awake and not pending deletion has the mark cleared, updates, and moves if the update moved it; 102910; every object pending deletion is detached (BA3E0) and released (BA490); the tail C13B0 |
| BE4A0 | `object_update::run` | one object: clear `moved`; BDA50 for the parent flag, B37A0 in game modes 4..5; the timers (1D5790); type updates, damage (D8330) and node lights (C0910), OR-ed into `changed`; the children; the cluster record (1D6FB0); the countdown; otherwise sleep if nothing changed |
| 10DD10 | `type_updates::run` | the update callback of each definition in the object type's chain, OR-ed |

The game data are named layouts with `offsetof` assertions: `DataArray`,
`ObjectHeader` (with `header_flags`: active, awake, moved, late_update,
pending_delete, held), `ObjectData` (sibling, child, parent, type, countdown,
cluster, update and cluster flags, the node and timer offsets),
`ObjectGlobals`, `PlayerRoots`, `GameGlobals`, `ObjectTypeDefinition` (type
chain, update and move callbacks), `ClusterRecord` and `TimerBlock`. The
original's inlined data_array iteration is one function, `next_in_use`, which
returns how a search ended (`SearchEnd`). Lookups are `header_of`, `object_of`
and `datum_of`. Every access names its role; the logic has no registers,
flags, stack slots or coverage marks.

Values the original keeps in callee-saved registers across a child call are
named per system ("kept" structs: `RootWalk`, `ObjectScan`, `LateScan`,
`DeleteScan`, `Update`, `ChainWalk`). The logic hands one to each child call
and continues with what comes back, so a callee that does not preserve
EBX/EBP/ESI/EDI is followed as the original follows it (the harness's
stand-ins do this).

**Adapter** (`native_object_tick_detail` in
`src/physics/runtime/native_tu15_object_tick.hpp`). Per owner:

- a role -> instruction table (`instruction`), which also performs the
  register saves the original places just before an access;
- the original instruction's register effects for each access (`loaded`,
  `stored`), including partial-register writes;
- the Frame: stack arguments, the frame slots the original keeps (B8F40's
  root count in its PUSH ECX slot, BE4A0's argument slot rewritten with the
  header), child calls with the kept struct loaded into its registers, the
  flags of the deciding instruction (`TEST r8,imm`, `OR BL,AL`, `CMP r32`,
  `CMP word`, `ADD EDI,EAX`, or the previous child's), and the hand-back;
- the epilogue per exit: pops, AL, flags, RET n (or B9180's JMP C13B0);
- an edge table for coverage: an outcome is reached when one original
  instruction follows another among the owner's accesses. A scan's end comes
  from the logic's `Search` (B9180's quiet/skip outcomes share an edge and
  are told apart by the cleared flags).

The shared boundary framework is `boundary::Port` (the register file, flags,
cursor, ordered and recorded reads/stores, pushes/pops/RET, genuine child
calls) and `boundary::Memory` (typed role accesses). The verification
machinery (`Registers`, `Trace`, `Replay`, `admitted`, `execute`) is
unchanged. `Machine`, the old register-file base, stays only for the
group E and F closures that are not yet rewritten.

131470 follows the same split: logic `native_cluster_distance::cluster_distance::flood`
(`native_tu15_cluster_distance_flood_systems.hpp`: `StructureBsp`,
`ClusterPortal`, `Cluster`, `ClusterDistance`; the frame's locals reached as
addresses through `Frame::local` and read and written as memory with roles;
COMISS on the live MXCSR for both distance comparisons; the kept
`PortalWalk`), adapter `native_tu15_cluster_distance_flood.hpp` (the
0x454-byte frame, the REP STOSD clear in the live direction flag, XMM0/XMM7,
the 14DD70 boundary with its inline/dispatch route, and the exit registers).

Every original memory access still happens in program order at the same
address and width, through the unit's store helper; the stack pointer is
live, so a child's actual stack is consumed as the original consumes it.

| Closure | Children (route) |
|---|---|
| B8F40 | BE4A0, 10E630 (direct) |
| B9090 | BE850, 10E710, 102910 (direct) |
| B9180 | BE4A0, BE850, 102910, BA3E0, BA490 (direct); C13B0 (tail jump) |
| BE4A0 | 1D5790, 10DD10, C0910, BE4A0, 1D6FB0 (direct); BDA50, B37A0, D8330 (dispatch) |
| 10DD10 | each type callback `[comp+0x40]` (dispatch, indirect target) |
| 131470 | 14DD70 (the unit's dispatch, which reaches the native leaf through `s.required`) |

The cursor is published before each access and set to the original final
instruction at exit: the RET, or the JMP at B921A before C13B0's tail.

Admission is otherwise the translated body: TU1.5 image, no active trace,
observation-only stores, literal cursor or no instruction callbacks, and no
service site in the closure. `HALO2_CENSUS_NATIVE_BRIDGES` is honoured.
131470 additionally requires masked MXCSR exceptions and no active MMX.

### Switches

- `HALO2_REFERENCE_OBJECT_TICK=1` restores the five translated live-support
  bodies.
- `HALO2_REFERENCE_CLUSTER_DISTANCE=1` restores the translated 131470.
- `HALO2_VERIFY_OBJECT_TICK=1` / `HALO2_VERIFY_CLUSTER_DISTANCE=1` record and
  replay, so the children run once:
  1. For every admitted call, the native closure runs on the live bank with
     genuine children, recording its ordered accesses (kind, pc, address,
     width, value) and each child boundary (target, position, complete entry
     and exit register file, including XMM, x87 and MXCSR).
  2. The unmodified translated body then runs on a replay bank. Its reads are
     served from the record and must match in order, address and width. Its
     stores must match the record's pc, address and bytes. Each child call
     must have the same target, position and entry ABI, and the recorded
     results are applied.
  3. The final registers, flags, XMM, x87 and MXCSR must be identical.

  A summary line prints at exit: `Object tick verify: ...` /
  `Cluster distance verify: ...`.

  The replay reference is generated by the binder. It is the canonical body
  with only its `run_X(s)` / `dispatch(s,...)` child calls routed to
  `native_object_tick_detail::replay`. It sits in the unit prefix as an
  ordinary helper, which the partitioner inlines.

### Binding

`tools/native_object_tick_replacements.py` (marker `run_b8f40`) and
`tools/native_cluster_distance_replacements.py` (marker `run_131470`) are
called from `spatial_owner_bindings` in `tools/specialize_native_operands.py`.
The receipt key is `native_spatial_owners`. Both are build dependencies in
`cmake/NativeOperandSpecialization.cmake`.

Each canonical body is pinned (state type normalized). Every direct child the
closure enters must have its body in the unit. 131470 also requires its
dispatched 14DD70 route, with no direct body. Otherwise the translated route
is kept.

## Proof

`tools/validate_native_object_tick_closures_tu15.py` (CTest `native_tu15_object_tick`)
and `tools/validate_native_cluster_distance_flood_tu15.py` (CTest
`native_tu15_cluster_distance_flood`) are registered in
`cmake/NativeTu15ObjectTick.cmake`. Each extracts the pinned bodies and the
generated replay references, then compiles the harness:
`tests/native_tu15_object_tick_tests.cpp` or
`tests/native_tu15_cluster_distance_flood_tests.cpp`.

Every child is a stand-in whose effects are a hash of its complete entry ABI,
its return address and arguments, and the object flag bytes it can see:

- stores into the object table and data blocks;
- AL results;
- EBX/EBP/ESI/EDI clobbers;
- stack imbalance;
- flags and XMM;
- for 14DD70, special-float XMM0 results and MXCSR sticky bits.

Any entry difference therefore diverges.

The fixtures author complete object tables:

- headers, bit vectors and data blocks;
- acyclic child and sibling chains;
- type callback chains, including negative type bytes, empty and null
  callbacks;
- player roots, including zero and negative counts;
- 16-byte strides, invalid handles, and game-mode bytes around 4..5.

For 131470 they author cluster/portal graphs with negative cluster words, the
-1 word, and words beyond the frame vector (which alias the caller frame and
arguments). They also cover empty and negative portal counts, special limits,
both direction flags and every MXCSR rounding, DAZ and FTZ mode.

Both harnesses inject faults by removing one fixture page. A fault case must
produce the same exception, cursor and store prefix.

Each case compares:

- ordered stores (pc, address, preimage, bytes);
- the child call log;
- the whole bank;
- GP, EFLAGS, XMM, x87 status, MXCSR and cursor.

Results of the rewrite (2026-11-16, the unchanged harnesses; reports
`build/test-results/object-update-legibility-20261116/object-tick-rewrite.json`
and `cluster-distance-rewrite.json`). The equivalence figures are the same as
the machine-shaped closures' of 2026-11-15, because the reference bodies
decide them:

| | Object tick | Cluster distance |
|---|---|---|
| Release comparisons | 10,000 (2,000 per owner) | 8,000 (+ 8,000 inline) |
| Ordered stores | 808,046 | 845,366 |
| Genuine child calls | 175,202 | 75,875 |
| Fault cases | 1,088 | 2,108 |
| Census mode | 2,500 | 2,000 (+ 2,000 inline) |
| Verify mode (record/replay) | 2,500 | 2,000 (+ 2,000 inline) |
| ASan/UBSan | 2,000 + 400 verify | 1,600 + 400 verify, 1,600 + 400 inline |
| Branches | 55/59 (4 unreachable) | 17/17 |
| Mutants rejected | 39/39 | 30/30 |

The mutation screens now name the file of each defect (the logic or the
adapter). They carry the 2026-11-15 defects ported to the new code, plus
defects in the new parts: the kept-struct hand-back after a child call, the
search's register mirror, the access order inside the logic (B9180's flags
before identifier, `datum_of`'s size before data, the updating flag after the
root count's slot, the flood's start entry and next-entry reload), the
arithmetic word shift of a cluster index, and the prologue's argument
address.

The four unreachable branches are the `CMP EAX,-1` exits after a forward bit
scan (`a/b/c_scan_invalid`, `c_late_invalid`). The scan starts at a
non-negative index, proven by the preceding TEST/JL, so the found index can
never be -1.

Both harnesses also check these refusals: MapPack11, full callbacks without a
cursor, an active trace, an arbitrary observer, and (131470) unmasked MXCSR.
The binding checks are install, idempotence, a changed body, a missing child
body (B8F40 without 10E630), a direct 14DD70 route, and references in the
unit prefix without an unrouted child.

Source hashes are rechecked after each mutation screen. Before the shared
build, the bound production form was also compiled out of tree: the five
live-support partitions and the whole environment unit, with the build's
flags.

## Production results

### After the rewrite (2026-11-16)

Shared build 08:18, pinned as `build-perf-4k60/halo2_native.ou` (sha256
`88138e4f…`). Armory 01a, 300 frames: default, reference and verify all give
`5627a51badf47762` / `8a9b58b38bef`. The verify lines equal the 2026-11-15
figures below exactly (object tick 450/450/450/11,282/11,282 calls,
3,059,873 ordered accesses, 90,593 child boundaries; cluster distance 300
calls, 978,600 accesses, 51,300 boundaries). The census and the MP sweep were
stopped by memory pressure before any case finished; see the claims doc's
continuation ("Object update owners: rewrite to the code standard, and C0F60
restored"). The figures below are the machine-shaped closures' (2026-11-15).

The binary is `halo2_native.objtick` (sha256 `0468fdbf…`), built in the
shared `build-perf-4k60` tree. Its specialized units carry every binding
(receipts: `native_spatial_owners` adds `b8f40`, `b9090`, `b9180`, `be4a0` and
`10dd10` in live support, and `131470` in the environment unit).

### Armory parity

The run is 01a, deterministic `--campaign-reference-capture`, 300 frames at
1280x720. Each route below gives frame `5627a51badf47762` and trace
`8a9b58b38bef`:

- reference (`HALO2_REFERENCE_OBJECT_TICK=1 HALO2_REFERENCE_CLUSTER_DISTANCE=1`);
- native under the Khronos validation layer (0 messages);
- the reference again;
- verify (`HALO2_VERIFY_OBJECT_TICK=1 HALO2_VERIFY_CLUSTER_DISTANCE=1`);
- both census runs.

The verify run checked every call:

- `Object tick verify: B8F40 450, B9090 450, B9180 450, BE4A0 11282, 10DD10
  11282 calls; 3059873 ordered accesses (358246 stores) and 90593 child
  boundaries identical`.
- `Cluster distance verify: 300 calls; 978600 ordered accesses (235500 stores)
  and 51300 child boundaries identical`.

Live branch coverage is 36/59 and 12/15. The harness covers every reachable
branch.

### Multiplayer (Coagulation, `tools/perf_4k60/parity.py`, cases 4:90, 4:150 and 1:150 at 3840x2160)

Reference against native is identical in every case (`9c14a330…`,
`13fa114e…`, `05f31264…`, unchanged from the BD080 checkpoint), and the
reference is deterministic. Reference against verify is identical too. The
`--keep-lines` verify run checked every call in each case:

| Case | B8F40/B9090/B9180 | BE4A0 | 10DD10 | Ordered accesses | Stores | Child boundaries |
|---|---|---|---|---|---|---|
| 4:90 | 90 each | 2,627 | 1,752 | 384,640 | 57,741 | 14,568 |
| 4:150 | 150 each | 4,059 | 3,319 | 682,141 | 102,178 | 25,856 |
| 1:150 | 150 each | 2,466 | 1,924 | 544,167 | 65,808 | 15,141 |

All were identical. 131470 does not run in multiplayer. The tool reports
FAILED only because the kept verify line exists in the candidate role alone,
as with the BD080 and PVS verify runs.

### Census (same binary, Armory, 300 frames, `HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1 HALO2_CENSUS_NATIVE_BRIDGES=1`)

| Run | Translated instructions/frame |
|---|---|
| Reference switches on | 421,426 |
| Native | 378,225 (−43,201, −10.25%) |

Removed per frame: B9180 15,065, B8F40 8,320, 131470 7,376, B9090 7,265, BE4A0
2,928 and 10DD10 2,245, all now 0. No routine increased.

The native top entries are now:

| Routine | Instructions/frame | Status |
|---|---|---|
| 136EA0 | 17.4k | render `Frame` bridge, native in production |
| 135970 | 16.7k | render `Frame` bridge, native in production |
| 304350 | 10.4k | translated |
| 2847D0 | 9.1k | translated |
| 1F5FE0 | 8.2k | translated |
| 296AD0 | 7.5k | node rebuild/codec owner |
| 166C00 | 7.0k | render `Frame` bridge, native in production |
| 1362A0 | 6.7k | translated |
| 134D30 | 5.7k | translated |
| BF0C0 | 5.3k | node rebuild owner |

### Campaign maps (B9180 sweep note)

The sweep's early stop (backing error at 0x48C33C reported in B9180) was
observed on `halo2_native.nbsweep`. That binary predates these closures: it
holds no `Object tick verify` string. So the stop belongs to the translated
route, not to this port.

On this binary, with `HALO2_REFERENCE_SYNC_RENDER=1` (the async campaign
render currently stops 03a with "Scene-copy resources belong to another
renderer thread" under xvfb):

- **03a.** Native, reference and the pre-change baseline
  (`halo2_native.otbase`) all stop identically at tick 927. The stop is in
  the AI root (`1d0670 at 211c1b: live world owner missing at 211c1b ->
  2aba90`), before any B9180 fault.
- **05a.** Native and reference both complete 1,500 frames.

The 0x48C33C error did not reproduce. These closures make exactly the
original accesses, so a genuine B9180 access fault would surface the same way
on both routes, with the same cursor.

### Sampled profile (same binary, Armory, 1904x1018 Original, `HALO2_REFERENCE_SYNC_RENDER=1`, 120 frames at 500 Hz, two interleaved pairs)

The reference route sets `HALO2_REFERENCE_OBJECT_TICK=1
HALO2_REFERENCE_CLUSTER_DISTANCE=1`. Values are inclusive ms/frame.

| Run | Frame | `Simulation::step` | `PhysicsLiveWorldTu15::step_objects` |
|---|---|---|---|
| reference 1 | 44.0 | 6.92 | 6.20 |
| native 1 | 44.3 | 6.92 | 6.27 |
| reference 2 | 39.5 | 6.40 | 5.80 |
| native 2 | 75.2 (host contention: every scope doubled) | 14.17 | 12.83 |

At this resolution (one sample = 0.017 ms/frame) the six owners' own cost
cannot be told apart. The census puts it at 43.2k translated instructions per
frame. Pair 1 shows no regression.

Pair 2's native run was disturbed: every scope roughly doubled, including the
renderer, which these closures do not touch.

The per-owner inclusive scopes (`run_b8f40`, `run_be4a0`, ...) are not
comparable between routes. `native_cpu_profiler.c` keeps only the innermost
48 `backtrace` frames, and these owners sit above deep child stacks (type
callbacks, unit and AI updates). How much of a deep stack survives the cut
depends on the route's frame depth. For example, inclusive `run_b8f40` reads
1.58 ms on the reference route and 6.45 ms on the native route, while
`step_objects` is 6.20 against 6.27.

## Remaining work (ranked; native census, translated instructions/frame)

1. **Device update callbacks** in the scene device constructor unit. These
   are the per-type callbacks that 10DD10 reaches through
   `[474B90+type*4]+0x84 -> [comp+0x40]`:
   - 1179C0 (machine), 3.3k/frame;
   - 10B840 (machine, control, light fixture), 2.9k/frame.

   Each is about 400–470 instructions of mixed SSE (`cvtsi2ss`, `divss`,
   COMISS) and x87, with children:
   - 10C5D0, 10CA30, 10CD70, 1181F0, 121890, 1CE8D0, 1CEBD0, 118350 and
     1DA750;
   - BD080 (already native);
   - BDA50 and `selected()` dispatches.

   Port them with this machinery (`Machine`/`Registers`/`Trace`/`Replay`) and
   the leaf framework's x87 discipline (the 14DD70 precedent). The other
   callbacks are small: DF880 (biped/unit) 0.5k self, C9010 89, 10F3A0 70.
2. **Done** (`BE850_object_subtree.md`): the BE850 post-physics move subtree
   (BE850, BE610, C0F60, 10DD80, C0200).
3. **The rest of the BE4A0 subtree**. C0910, 1E15D0 and 1D5790 are done
   (`BE850_object_subtree.md`). Remaining:
   - D8330, 1.8k (dispatched; damage delivery unit);
   - 1D6FB0, 1.6k (cluster owner update with SSE and x87 FSQRT/FDIV). Its
     children are 1D9EC0 1.6k (phantom world), 1D7260 1.5k, 1D9080 and
     1D9100.
4. **Done** (`BE850_object_subtree.md`): 131470 runs the proven
   `kernel_14dd70` inline at its child boundary, with the dispatch round trip
   as the fallback (`HALO2_REFERENCE_CLUSTER_DISTANCE_CHILD=1`).
5. **Profiler depth.** `tools/native_cpu_profiler.c` keeps 48 frames, which
   truncates deep object-update stacks. Owner-level inclusive attribution
   needs a deeper window.
