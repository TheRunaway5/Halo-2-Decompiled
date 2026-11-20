# Physics world leaves: 1F5FE0 / 2E4550 / 303BA0 / 2E47B0

Group H (`REMAINING_TRANSLATED_20261115.md`). There are native kernels for
four genuinely translated leaves of the physics world unit
(`physics_world_runtime_generated.cpp`). None has an executed child.

**Status (2026-11-16): 1F5FE0, 2E4550, 303BA0, 2E47B0, 303EE0, 303C70,
303D00 and 304350 proven and bound** (the 304350 family is described at the
end). **30CF40, 304130 (304350's children) and 2F33B0 (2F36F0's child):
proven and bound; 1F76A0: proven and bound; 2F8630 and 2E4FE0 (the GSK
agent's closest point and swept pair) and 1F7C10 (the BSP child shape):
proven and bound; 1F6D10 (the structure's queryAabb): proven and bound
(2026-11-17); 2F36F0 (the collection agent's child agent update) and E8D80
(the vehicle update unit's object flag update): proven and bound
(2026-11-17)** (sections at the end). **Status of the leaves (2026-11-15, rewrite): proven and bound.** The first port was
machine-shaped (register-named locals, raw offsets and an instruction address
on every access) and was unbound for that reason. It has been rewritten to the
code standard (`REMAINING_TRANSLATED_20261115.md`, "Code standard"): the logic
is readable systems code, and the calling convention lives in a separate thin
adapter. The unchanged equivalence harness passes, and the binder is
registered again (live results below).

| Owner | Canonical body | Role | Census before (instr/frame, calls) |
|---|---|---|---|
| 1F5FE0 | `run_1f16f0` | collision surface polygon: walks a BSP surface's edge ring and appends each (optionally transformed, scaled) vertex to the polygon shape in ECX | 8,245 (33.3 calls) |
| 2E4550 | `run_2e0a60` | relative transform: inverse (transposed rotation) of `[ESP+4]` composed with `[ESP+8]`, then the translation | 2,615 (19.2) |
| 303BA0 | `run_300100` | copy a 0x7E-byte contact record | 925 (31.9) |
| 2E47B0 | `run_2e0cc0` | rotate a vector by a transposed rotation | 600 (17.1) |

Callers (static): 1F5FE0 from 1F7C10 and 1F67C0 (the BSP collision agent);
2E4550 from 2E4FE0, 2F8630, 2F36F0, 1F6D10, 2F8B60, 2E5320 and 2F8300; 303BA0
from 303EE0 (under 304350); 2E47B0 from 304350, 313BB0 and 309060.

## Native route

Two files:

- **Logic:** `src/physics/runtime/native_tu15_world_leaves_systems.hpp`
  (`halo2::native_world_leaves`). Named layouts with `static_assert`ed
  offsets: `CollisionBsp` (tag blocks: surfaces +2C, edges +34, vertices
  +3C), `BspSurface` (plane, first edge, flags, breakable surface, material),
  `BspEdge` (start/end vertex, forward/reverse edge, left/right surface, each
  indexed by side), `BspVertex`, `RealMatrix4x3` (scale, forward, left, up,
  position), `HavokTransform` (three rotation columns, translation),
  `PolygonShapeHeader`/`PolygonVertex` and `ContactRecord`. The systems are:
  - `surface_polygon::build` (1F5FE0): a plain loop around the surface's edge
    ring, taking from each edge the vertex on the side facing the surface,
    optionally transforming it into world space (`transform_corner`) and
    appending it (`append_corner`);
  - `havok_rotation::multiply_inverse` (2E4550, `hkTransform::setMulInverseMul`)
    and `havok_rotation::rotate_inverse` (2E47B0,
    `hkVector4::setRotatedInverseDir`), both built on one `rotate_inverse`
    (R^T v = (x terms + y terms) + z terms over the transposed columns);
  - `contact_record::copy` (303BA0), field by field.

  Every memory access goes through a `Memory` object and names its role (an
  `Access` enum per system). Arguments and the locals the original keeps in
  its frame come from a `Frame` object. Single-precision arithmetic uses
  explicit-destination SSE (`sse::multiply/add/subtract`, `ordered_equal` is
  UCOMISS), so the first operand keeps the NaN payload and MXCSR behaves as the
  original. Comments mark each place where the operation or access order is
  the original's and matters. Values the original leaves in scratch registers
  come back in each system's `Residue`, for the adapter only.
- **Boundary adapter:** `src/physics/runtime/native_tu15_world_leaves.hpp`
  (`halo2::native_world_leaves_detail`). Per leaf:
  - a role -> instruction table (the cursor and store-observer PCs);
  - a `Frame` that reads the stack arguments and performs the register saves
    and restores the original schedules around them (1F5FE0 saves EBX/EBP
    before reading the BSP argument, ESI before keeping the first edge, and
    restores ESI/EBP after reading the shape key);
  - a `Schedule` that may run frame traffic tied to one access (2E4550
    restores EDI between loading and storing the translation) and reads branch
    coverage from the roles;
  - the exit: result registers (including 303BA0's `MOV DL`/`MOV CL`
    partial writes), the flags of the deciding instruction (1F5FE0 `ADD
    ESP`, 2E4550 the last `ADD EAX`/`DEC EDI` pair, 2E47B0 `AND ESP`), the
    scratch XMM registers and the RET.

  Admission, the reference and verify switches and the record/replay
  verifier are unchanged.

Every original memory access happens in program order at the same address
and width: pushes, pops, the return address, 1F5FE0's reuse of its argument 1
and 2 stack slots (surface address, first edge) and its three local spills of
a transformed corner. MOVAPS reads are one 16-byte read and MOVAPS stores the
unit's two ordered 8-byte stores. Every store goes through the unit's store
helper.

For 2E4550 and 2E47B0 the binder also gives the translated fallback the same
explicit-destination packed operations (the 2E44D0/2E46E0 precedent).
Without instruction callbacks the compiler may otherwise commute `mulps` or
`addps` operands, which changes the NaN payload when both operands are NaNs.

Admission: the TU1.5 image, TU1.5 observation PCs
(`selected_observation_pcs`), masked MXCSR exceptions, and either no
instruction callbacks or a literal cursor with no active trace and
observation-only stores. No service site lies in any leaf.
`HALO2_CENSUS_NATIVE_BRIDGES` is honoured.

### Switches

- `HALO2_REFERENCE_PHYSICS_WORLD_LEAVES=1` restores the translated bodies.
- `HALO2_VERIFY_PHYSICS_WORLD_LEAVES=1` records and replays. For every
  admitted call:
  1. The native leaf runs on the live bank and records its ordered accesses
     (kind, pc, address, width, bytes).
  2. The unmodified translated body then runs on a replay bank. Its reads
     are served from the record and must match in order, address and width.
     Its stores must match in pc, address and bytes.
  3. The final GP registers, flags, XMM, x87 and MXCSR must be identical.

  At exit it prints `Physics world leaves verify: 1F5FE0 N, 2E4550 N,
  303BA0 N, 2E47B0 N calls; ...`.

### Binding

`tools/native_world_leaves_replacements.py` is called from
`spatial_owner_bindings` in `tools/specialize_native_operands.py` (marker
`static void run_1f16f0(ObservedWorldState`), with receipt key
`native_spatial_owners`. It is also listed in
`cmake/NativeOperandSpecialization.cmake`. Each canonical body is pinned
(state type normalized). 1F5FE0's three `selected()` operands must resolve to
the TU1.5 literals the leaf reads: 486570→48B880, 465030→46A0D0 and
45D238→4620D0. Otherwise the translated route is kept. The physics contact
unit carries another `run_1f16f0` with a different state type, and it is not
bound.

## Proof

`tools/validate_native_world_leaves_tu15.py` extracts the pinned bodies (in
the binder's reference form) and their operand cases. It compiles
`tests/native_tu15_world_leaves_tests.cpp` against them. Run it through
`tools/run_test.py`:

```
python3 tools/run_test.py -- python3 -B tools/validate_native_world_leaves_tu15.py \
    --report build/test-results/physics-world-leaves-20261115/native-tu15-world-leaves-rewrite.json --cases 40000
```

CTest: `native_tu15_world_leaves` (`cmake/NativeTu15WorldLeaves.cmake`,
default 20,000 cases, two mutant jobs).

Fixtures:

- **1F5FE0.** A complete BSP: the surface table, 12-byte edge rings of 1 to
  16 edges with random sides, and vertex rows. Surface indices are small,
  negative, or carry upper garbage, where side 1 is impossible. Vertex
  indices are sometimes out of range (0xFFFF). The optional scaled transform
  has scale exactly 1.0 or not.
- **2E4550, 303BA0, 2E47B0.** Operand blocks that are aliased (in place,
  over the transform, overlapping rows, the transform composed with
  itself), unaligned, or escape the fixture. 2E4550 also gets rows at
  0xFFFFFFC0, where the last `ADD EAX,0x10` carries into the `DEC`.
- **All four.** Special floats (NaN payloads, SNaN, infinities, denormals,
  signed zeros), every MXCSR rounding mode with and without DAZ/FTZ, set
  sticky flags, random register files, and fault injection with one
  fixture page removed.

Each case compares the ordered store events (pc, address, preimage, bytes),
the whole bank, the GP registers, EFLAGS, XMM, x87, MXCSR and the cursor. A
fault case must give the same exception, cursor and store prefix.

Results of the rewrite (2026-11-15, report
`build/test-results/physics-world-leaves-20261115/native-tu15-world-leaves-rewrite.json`).
The harness (`tests/native_tu15_world_leaves_tests.cpp`) and the comparison
logic are unchanged; the same seeds give the same case and store counts as
the machine-shaped kernels:

| | |
|---|---|
| Release comparisons | 40,000 (10,000 per owner), 805,844 ordered stores, 3,918 fault cases |
| Census mode | 10,000 |
| Verify mode (record/replay) | 10,000 |
| ASan/UBSan | 8,000 + 1,000 verify |
| Branches | 11/11 |
| Mutants rejected | 34/34 |

The harness also checks these refusals: MapPack11, canonical observation
PCs, full callbacks without a cursor, an active trace and unmasked MXCSR.
The binding checks are install, idempotence, a changed body and a changed
operand mapping. The source hashes are rechecked after the screen.

The mutation screen is the only part of the validator that changed: its
anchors are source text, so they now point into the logic (`SYSTEMS`) or the
adapter (`HEADER`), one mutated copy of one header per mutant. The defect
classes are those of the first screen, plus four:

- **1F5FE0 logic:** the scale test, side selection and its sign extension,
  the vertex and edge strides, the w tag, the position operand order, the
  vertex store order, the scaled source, the material store width, the flags
  byte field, the local point read and the count increment.
- **1F5FE0 adapter:** the surface-index reload slot, the exit flags, the
  first-edge slot, the cursor, the RET bytes, the save order of EBX/EBP and
  the transform's XMM6 residue.
- **2E4550:** the row count, the transpose, the sum order, the subtraction,
  the translation source (logic); the DEC carry, the stack alignment and the
  EDI restore (adapter).
- **303BA0:** a vector row (logic); CL's upper bytes and the RET (adapter).
- **2E47B0:** a broadcast lane and an operand order (logic); the flags
  (adapter).

One first-run mutant survived and was replaced: an EDI restore that reads
the slot without assigning it. The adapter does not model EDI as the
original's loop counter, so it keeps the saved value either way; the
replacement drops the restore, which misaligns the ESI and EBP pops.

The first, machine-shaped port's results are kept in
`native-tu15-world-leaves.json` beside it (30/30 mutants).

## 303EE0 contact record append

303EE0 (canonical `run_300400`, 1.0k instructions/frame, called from 304350)
is `hkArray<ContactRecord>::expandOne`: it appends one 0x80-byte record and
returns its address. The record is a copy of a frame local whose state byte
(+7C) and byte +7D are cleared; the original leaves the local's other bytes
uninitialized, so whatever the stack holds there is copied as found. A full
array (size == capacity & 0x7FFFFFFF) first grows through 2E3250
(`hkArrayUtil::_reserveMore`, canonical `run_2df780`).

- **Logic:** `contact_array::expand_one` in
  `native_tu15_world_leaves_systems.hpp` (`HavokArray`: data, size,
  capacity and flags).
- **Adapter:** `kernel_303ee0` in `native_tu15_world_leaves.hpp`. It is the
  first group H parent with children, and it sets the pattern for 304350:
  - 2E3250 is entered as a genuine child through the unit's own route
    (`Port::call_child`), with the original's registers at the call (EAX =
    ECX = the full size, the CMP's flags) and the exact stack (two cdecl
    arguments, the return address, `ADD ESP,8` after). Every register comes
    back.
  - 303BA0 runs its native kernel inline at its own boundary: ECX, the
    argument push (which the original makes before it stores the new size)
    and the return address.
  - The exit flags are `SHL EAX,7`'s. OF survives from the `INC EAX` that
    counted the record, because the unit's shift leaves OF for counts above
    one, as the hardware leaves it undefined.
- **Verify mode** records the child boundary (target, position in the
  accesses, entry and exit registers). The binder's reference copy
  (`native_world_leaves_reference_300400`) routes the translated body's
  2E3250 call to `replay_child`, which requires the same target, position and
  entry registers and applies the recorded exit. 303BA0 stays the translated
  body there, with the native route bypassed.
- **Binder:** `tools/native_contact_array_replacements.py` (pin, the two
  return-address operands 300439→303F19 and 300456→303F36, and both child
  bodies present), registered in `spatial_owner_bindings` and the cmake
  helper list. The receipt key is `native_spatial_owners`, with entry
  `303ee0`.
- **Proof:** `tools/validate_native_contact_array_tu15.py` with
  `tests/native_tu15_contact_array_tests.cpp` (CTest
  `native_tu15_contact_array`). The 2E3250 child is a deterministic stand-in
  in both runs. It moves the array, doubles the capacity, changes scratch
  registers, flags and XMM1, and leaves its entry EAX/ECX/EDX/flags in XMM2,
  so a wrong register state at the call reaches the comparison. Fixtures: room,
  full and over-full arrays, the capacity flag bit, a record ending at a page
  boundary, random stack contents, special floats, every MXCSR mode and one
  removed page.

Results (report
`build/test-results/physics-world-leaves-20261115/native-tu15-contact-array.json`):

| | |
|---|---|
| Release comparisons | 40,000, 1,075,898 ordered stores, 1,238 fault cases, 6,250 growth calls |
| Census mode | 10,000 |
| Verify mode (record/replay with the child boundary) | 10,000 |
| ASan/UBSan | 8,000 + 1,000 verify |
| Branches | 2/2 (room, growth) |
| Mutants rejected | 13/13 |

The mutants cover the capacity mask, the full comparison, the record
stride, the size increment, the result offset and the cleared state (logic),
plus the argument push order, the child's entry EAX and flags, the cdecl
cleanup, the exit flags, the local's address and ECX at exit (adapter). Two
child-entry mutants first survived: the stand-in's register results are
overwritten before 303EE0 returns. The stand-in now also leaves its entry
state in XMM2, and both are rejected.

## Live verification (2026-11-15, all five bound)

Binary: `build-perf-4k60/halo2_native` (21:24, receipt `native_spatial_owners`
1f5fe0/2e4550/303ba0/2e47b0/303ee0), run from a private copy. Results are in
`build/test-results/physics-world-leaves-20261115/live/`.

- **Armory verify** (01a, 300 frames, `HALO2_VERIFY_PHYSICS_WORLD_LEAVES=1`):
  - calls: 1F5FE0 9,992, 2E4550 5,768, 2E47B0 5,141, 303EE0 9,571;
  - 2,077,251 ordered accesses (795,120 stores) and 1,209 child boundaries
    identical;
  - coverage `1fff`: every branch was reached live, including transformed and
    scaled corners and the growth path;
  - frame `5627a51badf47762`, trace `8a9b58b38bef`.

  303BA0's own bridge counts 0 calls. Its only live caller is 303EE0, which
  runs it inline, so its accesses are verified inside 303EE0's calls.
- **Armory frame parity:** native and reference
  (`HALO2_REFERENCE_PHYSICS_WORLD_LEAVES=1`) both give `5627a51badf47762` /
  `8a9b58b38bef`.
- **Census** (same binary, `HALO2_CENSUS_NATIVE_BRIDGES=1`, reference switch on,
  then off):
  - total 290,558 -> 277,131 instructions/frame (-13,427);
  - `physics_world_runtime_generated` 71,658 -> 58,236;
  - 1F5FE0 8,245, 2E4550 2,615, 303BA0 925, 2E47B0 600 and 303EE0 1,037 all go
    to 0;
  - files: `census-reference.txt`, `census-native.txt`, `census-summary.txt`.
- **Multiplayer verify sweep** (`h_sweep.py`, scripted fire, grenade, jump,
  use, crouch and move, 600 ticks, verify mode):
  - cases: coagulation, headlong, zanzibar, containment and burial_mounds at
    1p and 4p; ascension, lockout, midship and turf at 2p; campaign 03a and
    05a at 600 frames;
  - 713,089 verified calls (1F5FE0 225,658, 2E4550 163,572, 2E47B0 141,842,
    303EE0 182,017), 39.8M ordered accesses (16.1M stores) and 20,516 child
    boundaries identical;
  - 0 refusals, 0 differences.

  Every MP case exits 0. The two campaign cases exit 1 with "Scene-copy
  resources belong to another renderer thread", a pre-existing renderer
  error: the earlier node-rebuild sweep (sweep3) failed 03a the same way, and
  03a fails identically on this binary with
  `HALO2_REFERENCE_PHYSICS_WORLD_LEAVES=1` (`sweep-reference/`).
- **Multiplayer frame parity** (`tools/perf_4k60/parity.py`, coagulation,
  `--motion`, 3840x2160, cases 4:90, 4:150 firing, 1:150 firing): reference
  (switch on), its repeat and the native candidate are byte-identical.
  `PASSED` (`mp-parity-coagulation.json`).

## 303C70 / 303D00 local shape buffers

303C70 (canonical `run_3001d0`) and 303D00 (`run_300260`) construct and
destroy an `hkLocalArray` of 256-byte shape buffers on the thread memory's
stack (the singleton at [48C938]: next free byte +8, bytes left +C, a mark at
+14 whose meaning is not established). A block that does not fit comes from
the virtual `allocate_stack()` (+24); the destructor pops the block, or gives
it back through `deallocate_stack()` (+28) when it starts at the mark or there
is no mark, then frees the array's storage through `deallocate()` (+14, class
12) if the array owns it. Each opens an MSVC exception frame (handler 37ED68)
whose unwind state marks the array alive.

- **Logic:** `local_buffers::construct` and `local_buffers::destroy`
  (`ThreadMemory`, `LocalArray`, `ThreadMemoryFunction`).
- **Adapter:** `kernel_303c70`/`kernel_303d00`, with the shared
  `open_exception_frame` and `call_dispatch` helpers. The three virtual calls
  are genuine children entered through the unit's `dispatch` with the
  original's canonical site; their registers and flags at the call are the
  original's (303C70: EAX bytes, ECX the thread memory, EDX its vtable, the
  CMP's flags; 303D00: CMP block,mark or TEST mark,mark, and SHL's flags for
  `deallocate()`, whose class and size are pushed before the data is read).
- **Binder:** `tools/native_local_buffers_replacements.py` (two pins, the
  return-address, handler and thread-memory operands, the dispatch sites),
  registered in `spatial_owner_bindings` (marker `run_3001d0`); receipt
  entries `303c70`, `303d00`. Each verification reference routes its dispatch
  calls to `replay_child`.
- **Proof:** `tools/validate_native_local_buffers_tu15.py` with
  `tests/native_tu15_local_buffers_tests.cpp` (CTest
  `native_tu15_local_buffers`). The thread memory's virtual functions are
  stand-ins that log their complete entry state (registers, flags, x87, an
  XMM digest, the canonical site) into the fixture, so a wrong register at a
  call reaches the store stream.

| | |
|---|---|
| Release comparisons | 40,000, 1,186,399 ordered stores, 3,732 faults, 27,046 dispatch children |
| Census / verify | 10,000 / 10,000 |
| ASan/UBSan | 8,000 + 1,000 verify |
| Branches | 8/8 (bump, allocate; empty, pop, release at the mark, release without a mark; owned, borrowed) |
| Mutants rejected | 23/23 |

## 304350 welded collection agent

304350 (canonical `run_300870`, 10.4k instructions/frame in the Armory) is the
collision agent of a body whose shape wraps a collection of child shapes
(`hkpCollisionAgent::processCollision`, thiscall, RET 10). It keeps one child
agent per child shape and welds the children's contacts:

1. 2F36F0 updates the child agents (it receives the collection's body and the
   function 2F3140);
2. per child with an agent (not hkpNullAgent, which 30BAD0 returns), the
   shape comes from the collection's `getChildShape()` into a shape buffer
   (303C70's array); the game's weld predicate ([46E46C]) decides from the two
   shape types whether the child agent's closest contact is gathered into a
   local `hkInplaceArray<ContactRecord,4>` (303EE0, growing through 2E3250) or
   the child agent processes its collision directly;
3. the records are sorted by distance (a selection sort through the x87
   stack: four candidates at a time while four remain, then one at a time),
   and each record with a contact is welded (304130) against the records after
   it; the weld returns the record count it keeps;
4. every point each record's child agent already holds is welded too, in the
   welds' form (position with the distance in w, the normal rotated into body
   B's frame by 2E47B0);
5. each record is emitted: 30CD00 for the kept records with a contact, then
   30CF40 with the distance less the bias at [46A180].

It also writes the profiler's begin/end timer events when the stream has room
(RDTSC through the unit's service), and gives back the buffers (303D00) and
the records' storage.

- **Logic:** `collection_agent::process_collision` and its phases (`gather`,
  `record_contact`, `sort_and_weld`, `swap_records`, `weld_child_points`,
  `emit`, `timer_event`), with named layouts (`CdBody`, `ShapeHeader`,
  `Agent`, `ChildEntry`, `ChildAgent`, `ChildPoint`, `ProcessInput`, and the
  meanings of `ContactRecord`'s fields here). The arguments and the frame's
  locals are a `Locals` table of addresses; every access of the original,
  including the loop counters it keeps in its frame and re-reads (the logic
  uses the values read back), is a role-tagged memory access. x87 values are
  `long double` through `x87::load/store/subtract/greater`, which convert a
  memory operand as the translated FLD does (a signalling NaN is quieted
  before FSUB/FCOM).
- **Adapter:** `kernel_304350` and `CollectionAgentAbi`, one object that is
  both the schedule and the frame:
  - role -> instruction (the grouped search's four lanes and the state's two
    paths have their own PCs);
  - register mirroring at each role: the register each original load writes
    and the register arithmetic between two accesses; EBX, ESI and EDI are
    mirrored at every write (EBX carries the welds' record count, and ESI at
    the end depends on which phases ran);
  - the frame traffic interleaved with the logic's accesses (the unwind-state
    stores, the arguments pushed before a store, the `this` reloads);
  - the x87 stack kept exactly as the translated instructions keep it (FLD
    pushes, FCOM records the status, FSTP pops, FXCH);
  - the calls: 2F36F0, 304130 (two sites), 30CD00 and 30CF40 through
    `call_child`, the seven virtual and function-pointer calls through
    `call_dispatch` with their canonical sites; 303C70, 303D00, 303EE0, 2E47B0
    (two sites) and 30BAD0 run inline. After each genuine child the adapter
    requires the callee-saved registers and the stack cleanup the ABI
    promises, and throws otherwise instead of diverging.
- **Verify mode** (shared with the other leaves) now also carries the host's
  x87 exception flags in the register file (the state's x87 status reads
  them, and an FCOM records them), and records the RDTSC service's results
  so the replay is served the same time stamps.
- **Binder:** `tools/native_collection_agent_replacements.py`: the pin, 27
  operands (return addresses, handler, timer names, profiler stream,
  predicate, bias, thread memory, candidate filter), the seven dispatch
  sites, the translated children present, and the inline children's pins
  (the local buffers, contact array and world leaves binders' own checks, and
  30BAD0's pin and operand). The reference routes the translated children
  and dispatch calls to `replay_child` and runs the inline 303C70/303D00/303EE0
  as their own routed references. Registered after its children's binders
  (marker `run_300870`); receipt entry `304350`.
- **Proof:** `tools/validate_native_collection_agent_tu15.py` with
  `tests/native_tu15_collection_agent_tests.cpp` (CTest
  `native_tu15_collection_agent`, 8,000 cases). Every translated child and
  virtual target is a stand-in that logs its complete entry state into the
  fixture and reads its stack arguments; the weld changes the count it
  returns and sometimes the records' size; 2E3250 moves the records to an
  owned block. Fixtures: 0-15 children, null and real child agents,
  predicates that gather or process, contacts found or not, distances with
  ties, NaNs, signed zeros, denormals and infinities, child agents with 0-4
  points (or a negative count), the profiler stream with and without room,
  buffers that fit the stack or not, x87 depths 0-7 (the deepest overflow),
  every MXCSR mode, and one fixture page removed.

| | |
|---|---|
| Release comparisons | 20,000, 24,991,033 ordered stores, 1,546 faults, 667,038 child calls, 54,659 gathered records |
| Census / verify | 5,000 / 5,000 |
| ASan/UBSan | 4,000 + 1,000 verify |
| Branches | 19/19 |
| Mutants rejected | 35/35 (each screened in release, then verify mode) |

Two grouping mutants first survived the release-only screen: they change only
which instruction reads each candidate distance (and add one re-read of the
array's data), which no store, register or cursor shows. The screen now also
runs each mutant in verify mode, whose replay checks every read's order,
address and width; both are rejected there.

After the shared-header changes the earlier proofs were re-run with unchanged
figures: world leaves 40,000 / 805,844 stores / 34 mutants, contact array
40,000 / 1,075,898 stores / 13 mutants.

## Live verification of 303C70 / 303D00 / 304350 (2026-11-16)

Binary: `build-perf-4k60/halo2_native` (20:16, receipt `native_spatial_owners`
1f5fe0/2e4550/303ba0/2e47b0/303ee0/303c70/303d00/304350), run from a private
copy (sha256 prefix `09255dee3c05de3d`; the copy needs `eos_sdk_worker`
beside it). Results are in
`build/test-results/physics-world-leaves-20261115/live-304350/`.

- **Armory verify** (01a, 300 frames, `HALO2_VERIFY_PHYSICS_WORLD_LEAVES=1`):
  - 304350 727 calls (every call of the run), 1F5FE0 9,992, 2E4550 5,768,
    2E47B0 1,048 (its other calls now run inside 304350);
  - 4,241,333 ordered accesses (1,755,560 stores) and 65,188 child boundaries
    identical;
  - frame `5627a51badf47762`, trace `8a9b58b38bef`.

  303C70, 303D00 and 303EE0 count 0 calls of their own: in the Armory their
  only caller is 304350, which runs them inline, so their accesses are
  verified inside its calls. Live coverage reached every 304350 branch except
  the profiler event (the stream has no room) and the null agent, and the
  local buffers' bump, empty, pop and borrowed paths.
- **Armory frame parity:** native and reference
  (`HALO2_REFERENCE_PHYSICS_WORLD_LEAVES=1`) both give `5627a51badf47762` /
  `8a9b58b38bef`.
- **Census** (same binary, reference switch on, then off):
  - total 263,027 -> 238,971 instructions/frame (-24,056; the earlier leaves
    are in both numbers);
  - `physics_world_runtime_generated` 69,533 -> 45,477;
  - 304350 10,361, 303C70 114, 303D00 94 and 30BAD0 64 go to 0 (with
    1F5FE0, 2E4550, 303BA0, 2E47B0 and 303EE0, already 0);
  - 304350's children stay translated and unchanged: 2F36F0 1,592, 304130
    2,291 (30CD00 and 30CF40 likewise);
  - files: `census-reference.txt`, `census-native.txt`, `census-summary.txt`.
- **Multiplayer verify sweep** (`h_sweep.py`, scripted fire, grenade, jump,
  use, crouch and move, 600 ticks, verify mode; same cases as before):
  - 480,387 verified calls (304350 42,097, 1F5FE0 225,617, 2E4550 163,538,
    2E47B0 49,135), 84,282,081 ordered accesses (36,862,610 stores) and
    1,289,492 child boundaries identical;
  - 0 refusals, 0 differences; every MP case exits 0.

  The campaign cases 03a and 05a exit 1 with the pre-existing "Scene-copy
  resources belong to another renderer thread" (see above), after their
  verify lines (304350 85 and 119 calls identical).
- **Multiplayer frame parity** (`tools/perf_4k60/parity.py`, coagulation,
  `--motion`, 3840x2160, cases 4:90, 4:150 firing, 1:150 firing): reference
  (switch on), its repeat and the native candidate are byte-identical.
  `PASSED` (`mp-parity-coagulation.json`).

## 30CF40 manifold report and 304130 contact weld

Two of 304350's translated children (2026-11-16, night). Each is bound at its
own body, so 304350's native calls through `call_child` reach them through
the bound body like any other caller; 304350's adapter and proof are
unchanged.

**30CF40** (canonical `run_309450`, cdecl, 2,958 instructions/frame in the
Armory) re-checks the contact points a convex child agent keeps between steps
(its manifold: the count at agent+18 and the 0x30-byte points from agent+30)
against the two bodies' current transforms. A point holds a point in A's frame
(w: its weight), one in B's frame (w's low word: its id) and the normal. It
stays when the distance along the normal is less than the input's tolerance,
greater than the separation 304350 passes, and the drift across the normal is
less than the input settings' drift tolerance (squared on the x87 stack). A
point that stays is appended to the output's points at weight * A + (1 -
weight) * B with the distance in w; one that fails is removed through the
contact manager's virtual removal (vtable +8, its id), and the manifold's last
point moves into its place. The output's array grows first through 2E31E0
(`hkArrayUtil::_reserve`, to twice its capacity or to what is needed).

- **Logic:** `manifold_report::verify_and_report` (`CachedPoint`,
  `OutputPoint`, `ReportInput`, `Locals`; the world transform and the two
  horizontal sums in the original's operand order). The three tests are
  ordered x87 comparisons evaluated in order: equal values and NaNs remove a
  point (TEST AH,5/JP and TEST AH,41/JNZ).
- **Adapter:** `kernel_30cf40`/`ReportAbi`. EBX, ESI and EDI are pushed between
  the first reads; the x87 stack is kept as the translated instructions keep
  it (FLD, FLD ST(0) with its own cursor, FMUL, FSTP m32, FSTP ST(0), FLD and
  FCOMP with FNSTSW/TEST); XMM0..XMM5 at the removal call and at the exit come
  from the point's check (`publish_check`); 2E31E0 is a genuine child through
  the unit's route and the removal goes through the unit's dispatch with its
  canonical site 30965A.

**304130** (canonical `run_300650`, thiscall, RET 14, 2,291 instructions/frame)
welds 304350's records [first, n) against one contact: a record whose state is
1 is tested by 303800 (thiscall on the agent, RET 10, through the unit's
dispatch: the verdict for the record's second contact against the contact,
with the input's parameter). A rejected record is swapped with the last of the
n, n shrinks in its argument slot, and the record moved in is tested next.

- **Logic:** `contact_weld::weld` and `swap_with_last`. The swap is the
  original's field-by-field exchange: the record's vectors held in XMM0..XMM5,
  its floats at +20/+24 through the x87 stack (an SNaN comes back quieted), its
  words and bytes in the frame; the last record copied over it; the held
  fields written into the last record through the field addresses the
  original spills to its frame and reads back. The order matters when the
  record is the last one.
- **Adapter:** `kernel_304130`/`WeldAbi`: the test's three arguments pushed
  before `this` is re-read, EAX = the second contact, EDX = the verdict,
  CMP [record+7C],1's flags; XMM0..XMM6 after a swap; the exit registers of
  each path (EDX's upper bytes are the spilled +78 address).

Both translated routes (and the references) use explicit-destination SSE
(`ordered`, as the world leaves binder does): without instruction callbacks
the compiler may commute `addps`/`mulps`/`addss` operands, which changes the
NaN payload when both operands are NaNs. The first -O2 harness build showed
exactly that on the reference side (a default NaN where the original keeps a
payload).

- **Binder:** `tools/native_manifold_replacements.py` (pins; operands
  3094A7→30CF97, 30965D→30D14D, 418A60→41C500 for 30CF40, 2FFD60→303800 and
  3006AB→30418B for 304130; dispatch sites 30965A and 3006A6; 2E31E0's body
  present), registered in `spatial_owner_bindings` (marker `run_309450`) and
  the cmake helper list; receipt entries `30cf40`, `304130`. Bridges
  `bridge_30cf40` and `bridge_304130`; verify summary entries 30CF40 and
  304130.
- **Proof:** `tools/validate_native_manifold_tu15.py` with
  `tests/native_tu15_manifold_tests.cpp` (CTest `native_tu15_manifold`). The
  children are stand-ins that log their complete entry state (registers,
  flags, x87, an XMM digest, the site) into the fixture; the removal sometimes
  changes the count, a cached point or the output's size, and 303800's
  stand-in sometimes answers through another hkBool, moves the records' data,
  or rewrites 304130's n in its argument slot (all re-read by the original).
  Fixtures: points that coincide, move along or across the normal, random and
  special floats, outputs with and without room (borrowed storage too), zero
  and negative counts; records with state 1 or not, SNaNs at +20/+24, first
  past n or negative, records on the stack below the frame; x87 depths 0-7,
  an active MMX state, every MXCSR mode and one page removed.

| | 30CF40 | 304130 |
|---|---|---|
| Release comparisons | 20,000, 2,318,556 ordered stores, 3,622 faults, 61,346 child calls | 20,000, 1,278,733 ordered stores, 3,034 faults, 28,891 child calls |
| Census / verify | 5,000 / 5,000 | 5,000 / 5,000 |
| ASan/UBSan | 4,000 + 1,000 verify | 4,000 + 1,000 verify |
| Branches | 8/8 (room, grow doubled, grow needed, no points, kept, far, separated, drifted) | 5/5 (none, untested, kept, swapped, swapped with itself) |
| Mutants rejected | 54/54 together (31 for 30CF40, 23 for 304130) | |

Report: `build/test-results/physics-world-leaves-20261115/native-tu15-manifold.json`.
Four first-run mutants were equivalent (unobservable flags or registers that
the original overwrites before any observation: TEST AH,41's flags, the
capacity's AND, DEC [EAX+4]'s flags, FNSTSW's EAX upper half) and were
replaced by observable ones; two more for 304130 (the loop CMP's operand
order and AL's held state byte) led to the n-rewriting stand-in and an EDX
mutant.

### Live verification of 30CF40 / 304130 (2026-11-16)

Binary: `build-perf-4k60/halo2_native` (21:16, receipt `native_spatial_owners`
with `30cf40` and `304130`), run from a private copy (sha256 prefix in
`binary.txt`). Results are in
`build/test-results/physics-world-leaves-20261115/live-manifold/`.

- **Armory verify** (01a, 300 frames): 30CF40 10,203 and 304130 3,772 calls,
  with 304350 727, 1F5FE0 9,992, 2E4550 5,768 and 2E47B0 1,048; 5,175,936
  ordered accesses (2,014,576 stores) and 71,649 child boundaries identical;
  frame `5627a51badf47762`, trace `8a9b58b38bef`. Live coverage: 30CF40 room,
  no points, kept, far, separated; 304130 none, untested, kept, swapped.
- **Armory frame parity:** native and reference both give `5627a51badf47762` /
  `8a9b58b38bef`.
- **Census** (same binary, reference switch on, then off): 263,035 ->
  233,732 per frame; `physics_world_runtime_generated` 69,533 -> 40,228;
  30CF40 2,958 and 304130 2,291 go to 0 (the earlier group H owners are 0 in
  the native run too). Files `census-reference.txt`, `census-native.txt`,
  `census-summary.txt`.
- **Multiplayer verify sweep** (`h_sweep.py`, one worker; the same 16 cases):
  750,511 verified calls (30CF40 184,102, 304130 85,803, 304350 42,097, 1F5FE0
  225,686, 2E4550 163,623, 2E47B0 49,200), 95,791,024 ordered accesses
  (40,519,359 stores) and 1,342,225 child boundaries identical; 0 refusals,
  0 differences. Live coverage adds 30CF40's drift test and 304130's swap of
  the last record with itself; only 30CF40's growth stays unreached live
  (the harness covers both growth paths).
  - mp ascension 2p exited 1 after printing its verify line and the shutdown
    handler (`xvfb-run: kill: No such process` in the tail). Re-run alone on
    the same binary it exits 0 with the same counts (30CF40 17,354, 304130
    5,660), and with the reference switch it exits 0 too
    (`ascension-rerun/`, `ascension-reference/`). The campaign cases 03a and
    05a exit 1 with the pre-existing renderer-thread error, after identical
    verify lines.
- **Multiplayer frame parity** (`tools/perf_4k60/parity.py`, coagulation,
  `--motion`, 3840x2160): reference, its repeat and the native candidate are
  byte-identical, `PASSED` (`mp-parity-coagulation.json`).

## 2F33B0 key sort

2F33B0 (canonical `run_2ef8f0`, cdecl, 1,526 instructions/frame) is
`hkAlgorithm::quickSortRecursive` over unsigned 32-bit keys (2F36F0 sorts the
shape keys of the child shapes near a body with it): Hoare's partition around
the middle key (an arithmetic halving of low + high), recursion on the lower
part and a loop (the original's tail call) on the upper part. Indices are
signed and keys unsigned, as the original compares them. After a swap the
original re-reads both bounds from its argument slots, and the loop writes
its new lower bound back to its slot. The fourth argument (the comparator
object) is only passed down.

- **Logic:** `key_sort::sort`.
- **Adapter:** `kernel_2f33b0`/`SortAbi`: PUSH ECX (a slot never read), EBX,
  ESI and EDI pushed between the argument reads; the registers each partition
  leaves (ECX the pivot, EAX i, EDI j, EDX low, EBX high) and CMP EDX,EAX;
  CMP EDI,EBX before the tail loop and at the exit. The recursion runs the
  kernel inline at its own boundary (four arguments, the return address
  2F3414, ADD ESP,10), so it is verified within the outer call.
- **Binder:** `tools/native_key_sort_replacements.py` (pin, operand
  2EF954→2F3414, the only call is its own recursion); the reference is the
  renamed body, whose recursion re-enters the bound body with the native route
  bypassed, so verify mode replays the whole translated recursion.
- **Proof:** `tools/validate_native_key_sort_tu15.py` with
  `tests/native_tu15_key_sort_tests.cpp` (CTest `native_tu15_key_sort`).

Results (report `build/test-results/physics-world-leaves-20261115/native-tu15-key-sort.json`):

| | |
|---|---|
| Release comparisons | 20,000, 587,125 ordered stores, 5,809 faults |
| Census / verify | 5,000 / 5,000 |
| ASan/UBSan | 4,000 + 1,000 verify |
| Branches | 5/5 (scan up, scan down, swap, recursion, tail loop) |
| Mutants rejected | 17/17 |

Fixtures: 0-40 keys (random, sorted, reversed, few distinct values, all
equal, top bits set), bounds inside the array, empty or inverted ranges,
bounds below or past the keys' page (faults), random register files, x87
depths, an active MMX state (the sort has no x87 instruction), every MXCSR
mode and one page removed. Two first-run mutants were equivalent and were
replaced: SAR versus SHR of low + high (the index only addresses a key, and
times four the sign bit leaves the 32-bit address unchanged) and EDX = j
before the tail loop (the next partition sets EDX before any observation).

### Live verification of 2F33B0 (2026-11-16)

Binary: `build-perf-4k60/halo2_native` (21:49, receipt `native_spatial_owners`
with `2f33b0` after `30cf40`/`304130`), run from a private copy. Results are in
`build/test-results/physics-world-leaves-20261115/live-key-sort/`.

- **Armory verify:** 2F33B0 771 calls identical (with 30CF40 10,203, 304130
  3,772, 304350 727 and the leaves); 5,371,172 ordered accesses (2,075,768
  stores) and 71,649 child boundaries identical; frame `5627a51badf47762`,
  trace `8a9b58b38bef`. All five 2F33B0 branches were reached live.
- **Armory frame parity:** native and reference both give `5627a51badf47762` /
  `8a9b58b38bef`.
- **Census** (same binary, reference switch on, then off): 257,338 ->
  226,521 per frame; `physics_world_runtime_generated` 69,499 -> 38,669;
  2F33B0 1,526 goes to 0. (Another session's natives landed between the two
  builds, so the reference totals of the two binaries differ.)
- **Multiplayer verify sweep** (the same 16 cases, one worker): 779,777
  verified calls (2F33B0 29,528, 30CF40 184,102, 304130 85,803), 98,708,856
  ordered accesses (41,455,153 stores) and 1,342,225 child boundaries
  identical, 0 refusals, 0 differences; every MP case exits 0 (ascension
  included), the campaign cases as before.
- **Multiplayer frame parity** (coagulation, `--motion`, 3840x2160): `PASSED`.

## 1F76A0 shape key filter

1F76A0 (canonical `run_1f2db0`, 1,551 instructions/frame; called by 1F6D10,
1F26B0 and 1F7C10) answers whether the child shape a shape key names may still
collide. The key packs its kind (bits 29-31), a secondary index (16-28) and an
index (0-15):

1. an instanced geometry surface: it collides unless it is breakable and
   185AA0 reports its breakable surface gone (the instance at structure+144,
   0x58 bytes, its signed definition index at +34; the definition at
   structure+13C, 0xC8 bytes, its surfaces at +9C);
2. a collision BSP surface (structure+2C): it collides unless it is breakable
   and its bit is clear in the breakable state bit vector ([4F9B50]+1, the
   8-dword set chosen by the signed word at 474C24);
3. an object region: the handle at [52BE70][index] (-1: no object), its
   header in the object array ([4ECBD0]+44, 12 bytes; flag bit 4 excludes
   it), and the region's permutation byte (signed, at the object's +11A
   offset plus the region in bits 0-4) must equal the key's (bits 5-12);
4. always collides.

- **Logic:** `shape_key_filter::may_collide` (`Key`, `SurfaceRecord`,
  `ObjectHeader`, the globals by name).
- **Adapter:** `kernel_1f76a0`/`FilterAbi`. The jump table at 1F77AC is the
  adapter's: the four TU1.5 targets select the cases; any other entry goes
  through the unit's `canonical_branch()` so the translated route's message is
  thrown ("Unbound TU1.5 contact dispatch target" for an unknown value,
  "Required original solver dispatch target unavailable" for 0 or another
  table's label). 185AA0 is a genuine child through the unit's dispatch
  (EAX = the breakable index, canonical site 1F2E0D, AL back). Each path's
  registers and flags are mirrored at its loads; the three exits (1F770A,
  1F7754, 1F77A5/1F77A7) are the original's.
- **Binder:** `tools/native_shape_key_filter_replacements.py` (pin; the nine
  operands; the unit's `canonical_branch()` must map the four targets to the
  body's labels; the one dispatch site), registered in `spatial_owner_bindings`
  (marker `run_1f2db0`) and the cmake helper list; receipt entry `1f76a0`.
- **Proof:** `tools/validate_native_shape_key_filter_tu15.py` with
  `tests/native_tu15_shape_key_filter_tests.cpp` (CTest
  `native_tu15_shape_key_filter`). The validator extracts the unit's
  `canonical_branch()` and `sign_extend()` for the harness. Fixtures: every
  kind 0-7 (the jump table's neighbours hold 0, another table's target, a
  canonical label or a random value), breakable or solid surfaces, signed
  definition indices and sets, missing and excluded objects, region bytes
  that equal the key's permutation (half of them 80..FF, which the signed byte
  never equals), one page removed.

| | |
|---|---|
| Release comparisons | 20,000, 85,947 ordered stores, 5,306 faults |
| Census / verify | 5,000 / 5,000 |
| ASan/UBSan | 4,000 + 1,000 verify |
| Branches | 6/6 (solid surface, breakable call, breakable bit, object missing or excluded, permutation compared, always) |
| Mutants rejected | 20/20 |

The coverage word had six bits left, so each case's deciding read is one
branch. Three first-run mutants were replaced: the skipped
`canonical_branch()` call for a zero entry (it returns 0 there, so the message
is the same), CMP ECX,EAX's operand order (its flags survive only when the two
are equal) and the unsigned region byte (it needed fixtures whose byte equals
the key's permutation, now added).

### Live verification of 1F76A0 (2026-11-16)

Binary: `build-perf-4k60/halo2_native` (22:12, built by another session's
build after 1F76A0 was registered; receipt `native_spatial_owners` with
`1f76a0`), run from a private copy. Results are in `build/test-results/physics-world-leaves-20261115/live-shape-key-filter/`.

- **Armory verify:** 1F76A0 21,371 calls identical, with every other group H
  owner (30CF40 10,203, 304130 3,772, 2F33B0 771, 304350 727, ...); 5,586,807
  ordered accesses (2,124,068 stores) and 71,649 child boundaries identical;
  frame `5627a51badf47762`, trace `8a9b58b38bef`. The Armory reaches the
  solid-surface, object and always cases (no breakable surface there).
- **Armory frame parity:** native and reference both give `5627a51badf47762` /
  `8a9b58b38bef`.
- **Census** (same binary, reference switch on, then off): 257,337 ->
  224,958 per frame; `physics_world_runtime_generated` 69,499 -> 37,118;
  1F76A0 1,551 goes to 0.
- **Multiplayer verify sweep** (the same 16 cases, one worker): 1,217,136
  verified calls (1F76A0 437,237), 102,786,007 ordered
  accesses (42,362,196 stores) and 1,342,225 child boundaries identical,
  0 refusals, 0 differences; non-zero exits: sp:03a_oldmombasa:600 1, sp:05a_deltaapproach:600 1 (the campaign
  cases' pre-existing renderer-thread error). Live 1F76A0 cases: solid surface, breakable bit, permutation compared, always.
- **Multiplayer frame parity** (coagulation, `--motion`, 3840x2160): `PASSED`.

## 2F8630 closest point and 2E4FE0 swept pair

Two owners of Havok's GSK agent (2026-11-16, late). The agent finds the closest
points between the convex shapes of two bodies and keeps, between steps, a
simplex cache (agent+C) and a separating plane (agent+20: a normal, and in w
the distance by which the bodies are known to be apart).

**2F8630** (canonical `run_2f4bd0`, thiscall, RET 14; 2,367 instructions/frame
in the Armory, 31.9 calls) is the agent's closest-point query; its result is
an hkBool through its first argument:

1. while the plane's distance is greater than the bound at [46A0E8], it is
   reduced by how far the bodies may have closed along its normal (B's centre
   change less A's along the normal, plus both bodies' movement bounds, the w
   of each motion's first centre); the reduced distance is kept in the agent
   (FST), and while the unrounded value stays greater there is no contact;
2. otherwise the search: B's transform in A's frame (2E4550), the closest
   features on the cache (30E930; non-zero: no contact), the closest points
   (30D350, into the output) and the normal in world space (2E4760);
3. the surfaces' distance is the cores' distance less both convex radii,
   rounded; when it is less than the input's tolerance, A's point moves onto
   A's surface and B's point (A's point moved back by the distance) goes into
   B's frame: a contact;
4. otherwise, unless agent+18 is set or the settings' first byte is clear, the
   plane is cached: the world normal, with the distance less the tolerance
   times [46A190].

Each stage writes a profiler timer event (names 41CD3C and 41CD34) when the
stream has room (RDTSC through the unit's service).

**2E4FE0** (canonical `run_2e14e0`, thiscall, RET 14; 2,648 instructions/frame,
31.9 calls) wraps 2F8630 for the game's swept shapes (getType() 0x17: a convex
shape with a sweep at +10 and a squared-speed threshold at +20):

1. each swept shape's sweep is set: when its owner's squared speed (the owner
   of the root CdBody, +20; its object at +3C, the velocity at +40) is greater
   than the threshold and the game's predicate 283380 agrees, the velocity
   rotated into the body's frame and scaled by the input's step time (+10);
   otherwise zero. Flags record which shapes swept (bit 0 A, bit 1 B);
2. when the flags differ from the previous step's (agent+F0), the settings'
   first byte is cleared for this query (no plane is cached);
3. when they differ or a shape swept, the agent's cache is reset: B's
   transform in A's frame (2E4550), the simplex cache set up again (309150
   when B's shape is of type 5, 309060 otherwise) and the plane's distance -1;
4. 2F8630 runs; the flags are kept in the agent and the settings' byte is
   restored.

- **Logic:** `closest_point::find` and `swept_pair::closest_point` in
  `native_tu15_world_leaves_systems.hpp`, with the shared layouts in
  `convex_pair` (`BodyMotion`, `ConvexShape`, `SweptShape`, `CollisionInput`,
  `GskAgent`, `ClosestPointOutput`), `havok_rotation::rotate` (2E4760,
  hkVector4::setRotatedDir), the HK_TIMER_BEGIN macro as `monitor::timer_event`
  (any owner's roles) and the x87 operations FADD, FSUBR, FMUL m32, FCHS and the
  below test (`x87::add`, `subtract_from`, `multiply`, `negate`, `less`).
- **Adapters:** `kernel_2f8630`/`ClosestPointAbi`, `kernel_2e4fe0`/
  `SweptPairAbi` and the inline `kernel_2e4760` in `native_tu15_world_leaves.hpp`;
  `X87Mirror` keeps the state's x87 stack as the translated instructions keep
  it, and `genuine_child` requires the callee-saved registers and the stack
  cleanup after each genuine child. 2E4550 and 2E4760 run inline at their own
  boundaries; 30E930, 30D350, 283380, 309060 and 2F8630 (under 2E4FE0) are
  genuine children through the unit's route; the shapes' getType() (sites
  2E1510, 2E162C, 2E1792) and 309150 (site 2E17AC) go through the unit's
  dispatch. XMM registers are published where the original computes them
  (after each speed and sweep, at the plane's and a contact's exits).
- **Binder:** `tools/native_convex_pair_replacements.py` (pins; operands; the
  four dispatch sites; the translated children present; 2E4550's pin as the
  world leaves binder checks it and 2E4760's own pin), registered in
  `spatial_owner_bindings` (marker `run_2f4bd0`) and the cmake helper list;
  receipt entries `2f8630`, `2e4fe0`. Both translated routes, their references
  and 2E4760's translated body (which the reference runs) get explicit-
  destination SSE. Verify summary entries 2F8630 and 2E4FE0.
- **Proof:** `tools/validate_native_convex_pair_tu15.py` with
  `tests/native_tu15_convex_pair_tests.cpp` (CTest `native_tu15_convex_pair`).
  The translated children and virtual targets are stand-ins that log their
  complete entry state; 30E930 answers apart or not, 30D350 writes one of the
  case's planned outputs (cores' distances around the tolerance plus the
  radii), 283380 answers from the owner, the cache set-ups write the cache,
  and 2F8630's stand-in under 2E4FE0 reads the settings' byte and both sweeps.
  Fixtures: planes not trusted, staying apart or closing (and one whose
  reduced distance is greater than the bound only before FST rounds it), the
  profiler stream with and without room and no RDTSC service, plane caching
  allowed or not; swept shapes on either body with 0-3 parent levels, speeds
  around the threshold, the predicate's answer, the previous flags, B's shape
  of type 5 or not, an unbound getType() target; motion and owner addresses
  with varied low bits (and 64K carries); special floats, x87 depths 0-7, an
  active MMX state, every MXCSR mode and one page removed.

| | 2F8630 | 2E4FE0 |
|---|---|---|
| Release comparisons | 20,000, 1,102,535 ordered stores, 3,155 faults, 19,458 child calls | 20,000, 2,636,729 ordered stores, 2,817 faults, 96,056 child calls |
| Census / verify | 5,000 / 5,000 | 5,000 / 5,000 |
| ASan/UBSan | 4,000 + 1,000 verify | 4,000 + 1,000 verify |
| Branches | 10/10 (plane not trusted, apart, closed; features apart; contact; plane cached; not cached by the agent or the settings; timer written, stream full) | 15/15 (per body: not swept, parent chain, slow, refused, swept; flags changed, reset after a sweep, cache kept, type 5, other) |
| Mutants rejected | 65/65 together (35 for 2F8630 with the inline 2E4760, 30 for 2E4FE0), all in release mode | |

Report: `build/test-results/physics-world-leaves-20261115/native-tu15-convex-pair.json`.

The coverage bits are in word 1 (2F8630 64-73, 2E4FE0 74-88). Six first-run
mutants were equivalent: flags or registers the original overwrites before
any observation (FNSTSW/TEST flags followed by a CMP; EDX's LEA before a MOV
EDX; 2E4760's AND ESP flags before 2F8630's FCOMP/TEST; the TEST AH,41 flags of
B's speed, the same as TEST AH,45's for an FCOM; the x87 stack value an FSTP
pops while the store comes from the logic). They were replaced by observable
ones. Three exposed weak fixtures and were kept: the FST-rounding mutant (a
plane distance one float ulp from the bound was added) and two that need
addresses with varied low bits (aligned motions made `ADD EAX,20` and
`ADD ECX,40` set the same flags, and `+40` never carried past the low word
FNSTSW replaces). After those fixture changes the operand-order mutant of
2E4760's z product survived once (NaN operands with two payloads had become
too rare); a fixture kind with NaN payloads in both operands of every
rotation product was added, and it is rejected.

## 1F7C10 BSP child shape

1F7C10 (canonical `run_1f3320`, thiscall, RET 8; 1,162 instructions/frame in
the Armory, 33.5 calls) is the structure collision shape's
getChildShape(key, buffer): it builds the child shape a key names into the
caller's buffer and returns it. 1F76A0 decides first; a key that may no longer
collide gives an empty shape in the buffer (vtable 462180, one reference, no
user data). Otherwise, by the key's kind (bits 29-31):

- 1: a surface of the structure's collision BSP ([4ECC10]): its polygon
  (1F5FE0; no transform, owner -1);
- 2: an instance: 2E88B0 builds the shape in the buffer around the Havok
  object at +40 of the instance definition's collision data (definition
  +B4); the reference the build took is given back
  (hkReferencedObject::removeReference: at zero the object is destroyed,
  unless 283450 reports it static, which keeps one reference); then 184E20
  places the shape (its +10) at the instance;
- 5: a surface of the instance definition's collision BSP (1F67C0);
- any other kind: 1F7330.

Kinds 1 and 2 without a buffer give 0 (kind 2 still gives the reference back).

- **Logic:** `bsp_child_shape::child_shape` and `instance_shape`
  (`ReferencedObject`; the structure's globals and the instance and definition
  layouts are the shape key filter's).
- **Adapter:** `kernel_1f7c10`/`ChildShapeAbi`. 1F76A0 and 1F5FE0 are entered
  through the unit's route, so they run their own bound native bodies (the
  30CF40/304130 precedent); 283450 is a genuine translated child; 2E88B0,
  184E20 (two sites), the destructor (CALL [vtable]), 1F67C0 and 1F7330 go
  through the unit's dispatch with their canonical sites. The key's decoding
  (SHR/AND into ECX, EDX and EAX), the CMP chain over the kind, and the pushes
  (EDI once a key is accepted, EBX on kind 2's path) are mirrored; each exit
  pops them back.
- **Binder:** `tools/native_bsp_child_shape_replacements.py` (pin; the 16
  operands; the six dispatch sites; the three translated children present),
  registered in `spatial_owner_bindings` (marker `run_1f3320`) and the cmake
  helper list; receipt entry `1f7c10`. Bridge `bridge_1f7c10`; verify summary
  entry 1F7C10.
- **Proof:** `tools/validate_native_bsp_child_shape_tu15.py` with
  `tests/native_tu15_bsp_child_shape_tests.cpp` (CTest
  `native_tu15_bsp_child_shape`; it extracts the unit's `sign_extend()` for the
  translated body). Every child is a stand-in that logs its complete entry
  state; 2E88B0's sometimes takes a reference on the object. Fixtures: keys of
  every kind 0-7 (indices in and out of the instance table), the filter's
  answer, buffers present or not, signed definition indices (-1 wraps below
  the table), reference counts that reach zero (static or destroyed), stay
  above it or wrap from zero, random register files, x87 depths, an active MMX
  state, every MXCSR mode and one page removed.

| | |
|---|---|
| Release comparisons | 20,000, 817,494 ordered stores, 1,422 faults, 35,900 child calls |
| Census / verify | 5,000 / 5,000 |
| ASan/UBSan | 4,000 + 1,000 verify |
| Branches | 11/11 |
| Mutants rejected | 36/36 |

Report: `build/test-results/physics-world-leaves-20261115/native-tu15-bsp-child-shape.json`.

Three first-run mutants were equivalent (TEST AL,AL's flags after 1F76A0,
overwritten by the key's decoding or by TEST EAX,EAX; DEC WORD's flags,
overwritten by CMP WORD; XOR EAX,EAX after TEST EAX,EAX of a zero buffer) and
were replaced by observable ones; a fourth (the unsigned definition index)
had failed to compile and was corrected.

### Live verification of 2F8630 / 2E4FE0 / 1F7C10 (2026-11-16)

Two binaries, each run from a private copy: the 23:35 build (receipt
`native_spatial_owners` with `2f8630`, `2e4fe0`; results in
`build/test-results/physics-world-leaves-20261115/live-convex-pair/`) and the
23:51 build (with `1f7c10` too; results in `live-bsp-child-shape/`).

- **Armory verify:** 2F8630 9,571, 2E4FE0 9,571 and 1F7C10 10,051 calls
  identical, with every other group H owner; 6,760,393 ordered accesses
  (2,572,270 stores) and 124,434 child boundaries identical (23:51 binary).
  2E4550's own count falls to 3,783: its calls from 2E4FE0 and 2F8630 now run
  inline. The Armory reaches 2F8630's untrusted, apart, closing, contact and
  cached-plane paths; 2E4FE0's body A is a swept shape that never sweeps
  there.
- **Armory frame parity:** native and reference give `5627a51badf47762` /
  `8a9b58b38bef` on both binaries.
- **Census** (reference switch on, then off): 257,336 -> 219,782 per frame on
  the 23:35 binary (2E4FE0 2,648 and 2F8630 2,367 go to 0; 2E4760 216 -> 57,
  its other caller is translated); 251,137 -> 212,420 on the 23:51 binary
  (1F7C10 1,162 goes to 0); `physics_world_runtime_generated` 69,499 ->
  30,783.
- MP verify sweep (the same 16 cases, one worker): 1,745,433 verified calls
  (2F8630 182,017, 2E4FE0 182,017, 1F7C10 226,140), 127,024,389 ordered
  accesses (52,036,830 stores) and 2,465,787 child boundaries identical,
  0 refusals, 0 differences; every MP case exits 0. The campaign 03a/05a
  exits are the pre-existing "Scene-copy resources belong to another renderer
  thread", after identical verify lines. Live coverage adds 2F8630's plane
  kept out of the cache by the agent and by the settings; 2E4FE0's swept body
  A (with a parent chain), changed flags, the reset after a sweep and the
  309060 cache set-up; 1F7C10's instance path. Not reached live (the harness
  covers them): 30E930 reporting apart, a written timer event (the stream has
  no room), body B swept, the predicate refusing, the type-5 cache,
  1F7C10's rejected keys, missing buffers and released references.
- MP frame parity on coagulation with motion (3840x2160; 4:90, 4:150 firing,
  1:150 firing): reference, its repeat and the native candidate are
  byte-identical, `PASSED` (`mp-parity-coagulation.json`).

## 1F6D10 structure AABB query

1F6D10 (canonical `run_1f2420`, thiscall, RET 8; 1,552 instructions/frame in
the Armory, 2.4 calls) is the structure collision shape's
queryAabb(aabb, hits): the keys of the child shapes that may overlap an AABB,
appended to an hkArray<hkpShapeKey>.

1. The structure's own MOPP is queried (2E9670, canonical 2E5BC0), and the
   keys that may no longer collide are removed (1F6FA0, canonical 1F26B0,
   which runs the key filter 1F76A0).
2. The keys are then walked. An instance key (kind 2) whose definition has at
   most 0x2000 collision surfaces is expanded into the keys of its surfaces:
   - the instance's matrix becomes an hkTransform in the frame, and the AABB
     a box (identity rotation, the AABB's centre, half its size as the
     extents) made relative to it by 2E4550;
   - the instance key is removed (the later keys move down; the size is
     re-read as the bound for every key moved);
   - the definition's MOPP shape (+50 of its collision data) appends the keys
     of its surfaces in that box (queryObb, vtable +2C, tolerance 1e-4);
   - each new key k becomes an instance surface key (kind 5:
     ((k | FFFFA000) << 16) | the instance), which 1F76A0 keeps or removes.

- **Logic:** `structure_aabb_query::query_aabb`, `expand_instance`,
  `build_box` and `remove_key` (`KeyArray`, `Aabb`; the structure, instance
  and definition layouts are the shape key filter's).
- **Adapter:** `kernel_1f6d10`/`AabbQueryAbi`. 2E9670, 1F6FA0 and 1F76A0 are
  entered through the unit's route (1F76A0 runs its own bound body); 2E4550
  runs inline; the definition's queryObb goes through the dispatch (canonical
  site 1F262E). The frame reuses several slots in turn; the matrix copy's
  interleaved loads and stores keep their own instructions. Coverage bits
  100-108 (word 1).
- **Binder:** `tools/native_structure_aabb_query_replacements.py` (pin; eight
  operands; the dispatch site; the three translated children present; 2E4550's
  pin as the world leaves binder checks it), registered in
  `spatial_owner_bindings` (marker `run_1f2420`) and the cmake helper list;
  receipt entry `1f6d10`. Bridge `bridge_1f6d10`; verify summary entry 1F6D10.
- **Proof:** `tools/validate_native_structure_aabb_query_tu15.py` with
  `tests/native_tu15_structure_aabb_query_tests.cpp` (CTest
  `native_tu15_structure_aabb_query` in `cmake/NativeTu15WorldLeaves.cmake`).
  Every child is a stand-in that logs its complete entry state: 2E9670
  appends keys of every kind, 1F6FA0 sometimes drops the last key, 1F76A0
  answers from the key (and rarely makes the size negative), the queryObb
  reads the transform and extents and appends surface keys. Fixtures: keys of
  every kind (instance indices in and out of the table), surface counts
  around 0x2000 and negative, signed definition indices, collision data near
  the top of the address space, matrices and AABBs with special floats, NaN
  payloads in both operands of the box's MULPS and ADDPS (kind 12), a hits
  header inside its own key data (kind 13; see below), an instance table that
  straddles a missing page (kind 14), random register files, x87 depths, an
  active MMX state, every MXCSR mode and one page removed.

| | |
|---|---|
| Release comparisons | 20,000, 3,958,312 ordered stores, 4,446 faults, 107,883 child calls |
| Census / verify | 5,000 / 5,000 |
| ASan/UBSan | 4,000 + 1,000 verify |
| Branches | 9/9 (empty, other kind, too many surfaces, expanded, expansion's shift, no new keys, accepted, rejected, rejection's shift) |
| Mutants rejected | 43/43 (41 in release mode, 2 in verify mode) |

Report: `build/test-results/physics-world-leaves-20261115/native-tu15-structure-aabb-query.json`.

The last first-run survivor, `query-removal-bound` (the removal's bound kept
instead of re-read), needed a fixture in which a key moved down overwrites
the size the removal re-reads. Kind 13 now also places the hits' header one
word below its key data: the size is key 0, an instance key, and the
capacity is key 1 (negative, or a small count). Removing key 0 moves the
capacity into the size; the original stops (or moves that many keys), while
the mutant keeps moving keys until it leaves the fixture. The new random
draws then left `query-half-order` (MULPS's operand order) without a case
where both operands are NaNs, so kind 12 also gives the half factor and the
AABB NaN payloads. Both mutants are rejected.

### Live verification of 1F6D10 (2026-11-17)

Binary: the shared build of 2026-11-17 00:59 (receipt `native_spatial_owners`
with `1f6d10`), run from the private copy `build-perf-4k60/halo2_native.h8`
(sha256 prefix `7660814c6fed9c53`). Results are in
`build/test-results/physics-world-leaves-20261115/live-structure-aabb-query/`.

- **Armory verify:** 1F6D10 727 calls identical, with every other group H
  owner (1F76A0 21,371, 30CF40 10,203, 1F7C10 10,051, 1F5FE0 9,992, ...);
  7,015,928 ordered accesses (2,679,311 stores) and 130,290 child boundaries
  identical. 2E4550's own count falls to 2,092: 1F6D10's calls run it inline.
  Live 1F6D10 paths: other kinds, expanded, the expansion's shift, no new
  keys, accepted.
- **Armory frame parity:** verify, native and reference all give
  `5627a51badf47762` / `8a9b58b38bef`.
- **Census** (reference switch on, then off): 251,133 -> 210,865 per frame;
  `physics_world_runtime_generated` 69,499 -> 29,231; 1F6D10 1,552 goes to 0.
- **MP verify sweep** (the same 16 cases, two workers): 1,786,401 verified
  calls (1F6D10 48,710), 129,617,499 ordered accesses (53,106,552 stores)
  and 2,585,439 child boundaries identical, 0 refusals, 0 differences; every
  MP case exits 0. The campaign 03a/05a exits are the pre-existing
  "Scene-copy resources belong to another renderer thread", after identical
  verify lines. Live coverage adds the empty query. Not reached live (the
  harness covers them): too many surfaces, rejected keys and the rejection's
  shift.
- **MP frame parity** on coagulation with motion (3840x2160): reference, its
  repeat and the native candidate are byte-identical, `PASSED`
  (`mp-parity-coagulation.json`).

## 2F36F0 child agent update

2F36F0 (canonical `run_2efc30`, thiscall, RET 10; 1,592 instructions/frame in
the Armory) is the first step of 304350's collision agent: it keeps the
agent's child agents, one per child shape of the collection below body B
that lies near body A. Body B's shape is a bounding-volume tree over the
collection's children; the agent's entries are 304350's `ChildEntry` (shape
key, a word not written here, child agent).

1. Body A's AABB in body B's frame: body A's transform relative to body B's
   (2E4550), then the shape's getAabb() with a tolerance of one and a half
   times the input's (the half is rounded to a float by FSTP before the
   input's tolerance is added).
2. While the game enables it ([46E468]), nothing more happens unless a face
   of the AABB moved by more than half the tolerance since the last update
   (CMPLTPS against the AABB the agent keeps at +20/+30; the maximum corner
   first).
3. The agent keeps the new AABB, and the tree's queryAabb() returns the keys
   of the overlapped children into a local hkInplaceArray<hkpShapeKey,128>.
4. Entries and keys are matched. An entry whose key was not returned has its
   agent cleaned up (vtable +14) and is removed. A key without an entry gets
   one: its child shape is built (getChildShape into a local buffer, the child
   CdBody's parent being body B); when the input's collision filter enables
   the pair, the function 304350 passes (2F3140) creates the agent, otherwise
   the entry gets the null agent (30BAD0). [48CBB8] chooses the matching:
   - set: in the query's order, editing the agent's array in place (an entry
     that does not have the next expected key is searched among all keys; a
     new key's entry is inserted at the key's own index, growing the array
     through 2E31E0 when full);
   - clear: the keys are sorted (2F33B0) and merged, unsigned, with the
     entries into a local hkLocalArray (2F3430/2F34D0); the agent's array then
     receives the merged entries, its storage replaced by one of exactly their
     count (thread memory allocate/deallocate) when it is too small.
5. The keys' storage goes back to the thread memory when the query grew it
   out of the inline buffer.

- **Logic:** `child_agent_update::update`, `aabb_moved`/`face_test`,
  `match_in_query_order`, `remove_entry`, `merge_sorted`, `release` and
  `grown_capacity` (`Agent`, `Input`, the virtual slots by name; 304350's
  `CdBody`, `ChildEntry` and `ShapeHeader`; 1F6D10's `Aabb` and `KeyArray`);
  `sse::less` (CMPLTPS) joins the explicit-destination SSE operations.
- **Adapter:** `kernel_2f36f0`/`ChildAgentAbi`. The frame (exception frame,
  handler 37E656, unwind state at T+3DC set to 0, 1, 0, -1 as the locals come
  to life) reuses several slots in turn; the x87 stack is mirrored for the
  tolerance (FLD, FMUL, FSTP, FLD, FADD, FSTP); XMM0-5 are published where the
  original leaves them. 2E31E0, 2F3430, 2F34D0 and 2F33B0 (bound at its own
  body) are entered through the unit's route; getAabb, queryAabb, the
  clean-ups (three sites), getChildShape, isCollisionEnabled and the creation
  function (three sites each) and the thread memory's allocate/deallocate go
  through the dispatch with their 17 canonical sites; 2E4550 and 30BAD0 run
  inline. Every pushed argument is recorded and checked against the logic's
  at the call. Coverage bits 109-127 (word 1).
- **Binder:** `tools/native_child_agents_replacements.py` (pin; 32 operands;
  the 17 sites; the four translated children present; 2E4550's pin as the
  world leaves binder checks it and 30BAD0's as the collection agent binder
  pins it), registered in `spatial_owner_bindings` (marker `run_2efc30`) and
  the cmake helper list; receipt entry `2f36f0`. Bridge `bridge_2f36f0`;
  verify summary entry 2F36F0.
- **Proof:** `tools/validate_native_child_agents_tu15.py` with
  `tests/native_tu15_child_agents_tests.cpp` (CTest
  `native_tu15_child_agents`). Every child and virtual target is a stand-in
  that logs its complete entry state; getAabb writes the kept AABB, a moved
  corner or a random box; queryAabb appends the planned keys and grows the
  storage past 128; the reserve moves the storage; the sort sorts unsigned;
  the creation function makes agents. Fixtures: the cache on or off, kept or
  moved; either matching; 0-12 entries with ascending (or shuffled) keys and
  real, null or null-agent agents; keys in the entries' order with some
  removed and some new, shuffled or duplicated; the filter's verdicts; full,
  owned or borrowed entry storage; more than 128 keys; special tolerances
  and half factors; an unbound virtual target; entries straddling a missing
  page; random register files, x87 depths, an active MMX state, every MXCSR
  mode and one page removed.

| | |
|---|---|
| Release comparisons | 20,000, 16,355,129 ordered stores, 4,448 faults, 509,146 child calls |
| Census / verify | 5,000 / 5,000 |
| ASan/UBSan | 4,000 + 1,000 verify |
| Branches | 19/19 (cache kept, moved; query order; expected key, found, removed, removal's shift, inserted, grown, moved up; created, null agent; sorted, kept, merged new, gone, after the entries; storage replaced; keys' storage freed) |
| Mutants rejected | 51/51 (all in release mode; 6 by the adapter's argument checks) |

Report: `build/test-results/physics-world-leaves-20261115/native-tu15-child-agents.json`.

First-run survivors: three logic mutants changed only an argument the
adapter pushed from its own register mirror (the deallocations' byte counts,
the sort's upper bound). The adapter now records every pushed argument and
checks it against the logic's at the call, which rejects them. Two adapter
mutants were equivalent (a TEST width whose flags a later instruction
overwrites, a CMP in the removal whose flags are dead) and were replaced by
observable ones (the cache test's AL, the end of the keys in EAX at a
clean-up).

## E8D80 object flag update

E8D80 (canonical `run_e7e60` in `physics_vehicle_update_generated.cpp`,
stdcall, RET 4; 2,204 instructions/frame in the Armory) is an entry of the
vehicle update unit. An object (its handle's low word indexes the object
array, [4ECBD0]+44) has 60 flags: the bits of two dwords at +4 of a block it
locates by a signed word offset at +346. Each flag's definition (the table at
473D28) may have an update function (+4). For each set flag with one, the
function receives the handle and the flag's index (stdcall) and answers in
AL whether the flag stays set; a flag answered false is cleared, its word
re-read first (the update may change it). E8D80 returns whether any update
answered true. The flags' meaning is not established.

- **Logic:** `object_flag_update::update` (the object header is the shape key
  filter's).
- **Adapter:** `kernel_e8d80`/`FlagUpdateAbi`. The unit publishes canonical
  instructions (E7E60..E7EE6), so the adapter does; the update functions are
  genuine children through the unit's dispatch (canonical site E7EBE, return
  address E8DE0). The coverage word was widened to three: E8D80's bits are
  128-131 (word 2).
- **Binder:** `tools/native_object_flag_update_replacements.py` (pin, state
  type `VehicleState`; three operands; the site), registered in
  `spatial_owner_bindings` (marker `run_e7e60(VehicleState`) and the cmake
  helper list; receipt entry `e8d80`. Bridge `bridge_e8d80`; verify summary
  entry E8D80 (under `HALO2_VERIFY_PHYSICS_WORLD_LEAVES`).
- **Proof:** `tools/validate_native_object_flag_update_tu15.py` with
  `tests/native_tu15_object_flag_update_tests.cpp` (CTest
  `native_tu15_object_flag_update`). The update stand-in logs its entry state,
  sometimes changes another flag of the object, and answers from its inputs.
  Fixtures: handles with upper garbage and bit 15 of the index, flag blocks
  before or after the object, flag words of every density, definitions with
  an update, none or an unbound target, a block straddling a missing page,
  random register files, x87 depths, an active MMX state, every MXCSR mode and
  one page removed.

| | |
|---|---|
| Release comparisons | 20,000, 8,512,305 ordered stores, 3,308 faults, 378,999 child calls |
| Census / verify | 5,000 / 5,000 |
| ASan/UBSan | 4,000 + 1,000 verify |
| Branches | 4/4 (flag clear, no update, kept, cleared) |
| Mutants rejected | 22/22 (all in release mode) |

Report: `build/test-results/physics-world-leaves-20261115/native-tu15-object-flag-update.json`.

First-run survivors: the handle's index mask needed handles with bit 15 set
(the fixture now has a second run of headers at index 8000); four adapter
mutants were equivalent (the last CMP EBP,3C compares equal operands, EDI's
inversion and the shift count are dead before any observation, and TEST AL's
width is overwritten) and were replaced (the loop's last INC/CMP skipped,
EDX's headers, ESI's word offset, AL's decision).

## 1D7260 body motion snapshot

1D7260 (canonical `run_1d29a0`; EAX = the body's index, [ESP+4] = the havok
component; RET 4; 1,494 instructions/frame in the Armory) refreshes the
snapshot at the start of one body slot of a havok component (the component's
0x60-byte slots at +70): the position (written into the frame by 1D8B20,
which reads the motion's +B0 or asks the component's frame 1D8B90 when the
component's flag 0x40 is set), the linear velocity (motion +40) and the
angular velocity (motion +50); a body whose byte at +40 is set gets the zero
vector ([474D04]) for both. The slot is located before the position query;
the body is read from the slot array as it is after it. The words are copied
through the frame's locals in the original order (MOVSS keeps every bit).

- **Logic:** `body_motion_snapshot::refresh` (`BodySlot`, `Locals`).
- **Adapter:** `kernel_1d7260`/`SnapshotAbi`; 1D8B20 is a genuine child
  through the unit's route (ECX = the component, EAX = the index, EDI = the
  output, flags of ADD EBX,ESI). Exit: EAX/ECX/EDX the angular velocity, XMM0
  a moving body's angular z, flags of ADD ESP,24. Coverage bits 132-133.
- **Binder:** `tools/native_body_motion_snapshot_replacements.py` (pin; two
  operands; no dispatch site; the child present); receipt entry `1d7260`.
  Bridge `bridge_1d7260`; verify summary entry 1D7260.
- **Proof:** `tools/validate_native_body_motion_snapshot_tu15.py` with
  `tests/native_tu15_body_motion_snapshot_tests.cpp` (CTest
  `native_tu15_body_motion_snapshot`). The 1D8B20 stand-in logs its entry
  state, writes the position, and sometimes moves the slot array, flips the
  motionless byte or replaces the motion. Fixtures: indices inside, below and
  wrapping; the slot inside the frame (kind 13), the zero vector on the
  locals (12), the motion's velocities and the body's motion pointer on the
  locals (11), a straddled page (14), one page removed (15).

| | |
|---|---|
| Release comparisons | 20,000, 748,112 ordered stores, 1,913 faults, 19,251 child calls |
| Census / verify | 5,000 / 5,000 |
| ASan/UBSan | 4,000 + 1,000 verify |
| Branches | 2/2 (moving, motionless) |
| Mutants rejected | 34/34 (30 release, 4 verify) |

Report: `build/test-results/physics-world-leaves-20261115/native-tu15-body-motion-snapshot.json`.

## 2E4760 at its own body

2E4760 (`run_2e0c70`) ran native only inline in 2F8630. It is now also bound
at its own body by the world leaves binder (`bridge_2e4760`, verify summary
entry 2E4760); the binder's reference keeps explicit-destination SSE, as the
convex pair binder already gave it. The world leaves harness has a fifth
owner (the transform owners' fixture; kind 13 now fills the operands with NaN
payloads so both operands of a product are NaNs). Re-run: 40,000 comparisons,
666,706 ordered stores, 3,959 faults, census 10,000, verify 10,000, ASan/UBSan
8,000 + 1,000, 11/11 branches, 41/41 mutants (7 new for 2E4760).
