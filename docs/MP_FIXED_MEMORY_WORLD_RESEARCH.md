# Original fixed allocator, CRT scratch backing and new-map insertion

The `2010` allocation failure was caused by an **omitted original CRT scratch
registration**, rather than evidence that the fixed arena should be enlarged.
Original `37D8C0` registers the executable's separate `480DD0/4000` static
buffer with adapter `480DB0`. With that producer, unchanged `1C5FF0` passes
complete world construction, mode transition and BSP body insertion. A separate
case with the original source-initialized empty object-header pool executes
**all of `1C5FF0` through its return**. Ordinary motion/contact and the complete
authored startup remain unproved. Production and previous freezes are unchanged.
[Oracle](../tools/oracle_mp_spawn_fixed_memory_world.py),
[report](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/report.json),
[preceding whole-context freeze](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md).

## Authority and immutable evidence

The original TU1.1 XBE is
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The actual Lockout cache is SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The oracle reads the preceding frozen native compatibility generator executable
unchanged; it never calls its build function. A new real Wine32 worker remains
alive while each original-code case executes. Its full produced KPCR, ETHREAD,
backed host stack and process record are transferred exactly as in the previous
freeze. Native snapshots and runtime pins are separate per-run evidence.
No instruction, mode, capacity, scratch field or allocation return is patched.
[Complete compatibility producer and scope](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md),
[read-only reuse and byte audits](../tools/oracle_mp_spawn_fixed_memory_world.py).

| Frozen evidence | SHA-256 |
| --- | --- |
| New oracle | `d04c6f66b32cbbdf039a1c0f4bfa91748b6fe184c77b035d0e5ae64b7ae40d61` |
| Semantic report | `93c2f875b646ee1e298059237d1ba9f5e064153834d8188376bb101b9a7df75d` |
| Canonical original instruction manifest | `7d4b6782df14d85573bda342aec4ae1b65cd81eaf462f53847c70cf865bce478` |
| Executed original instruction listing | `273d51d70a6a097156f83bb89dc02801e46f792828e8d455fad823d161735f16` |
| Canonical run native-evidence pins | `0b206ddab23ae3c755f32fb4e672075c2d743dd7f5964691806082c83490953a` |

The three cases verify **8,399 unique executed original instruction addresses**.
The 200 routine listings contain 16,385 instruction rows; the separate executed
listing contains exactly 8,399 rows. All original bytes are checked against the
XBE and each emulator before publication. Twenty-six earlier immutable inputs
are audited before and after the run.
[Original listings and manifest](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/report.json),
[complete executed byte listing](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/executed-original-instructions.asm).

Run from the repository root:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_fixed_memory_world.py \
  --output analysis/multiplayer/integration/spawn-source/fixed-memory-world/reproduction/report.json
```

The canonical report and original instruction manifest are independently
reproduced byte-identically. The newly created worker's raw host snapshot may
vary. Earlier tools, documents, reports and the native producer executable are
asserted against their prior pins before and after execution.
[Per-run native pins](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/run-native-evidence-pins.json).

## The actual memory objects and modes

`1482A0` requests raw `VirtualAlloc(0,106470,101000,4)` backing. It aligns
`480D90=(raw+1000)&FFFFF000`, with original callback/offset metadata at aligned
address minus8/minus4. This is retained prior raw-memory service input, not an
independently recovered Xbox kernel result. `148190` then constructs these
separate objects:

| Source object | Original construction |
| --- | --- |
| Fixed allocator at aligned+0 | vtable45E77C, refcount1, backing aligned+30, capacity15000, allocation count0 |
| Pool allocator at aligned+15040 | original22E4E0, vtable45E7A8, class lists/tables/counters, internal4000 scratch registration |
| Adapter480DB0 | original executable static object, vtable45A8FC; separate CRT buffer producer below |
| Later fallback record at aligned+105410 | original1482F0, backing from518434, capacity452E8 adjusted for alignment |

The fixed allocator's `+18` is its backing pointer, `+1C` its byte capacity and
`+20` its signed allocation count. `22E1D0` keeps sorted `(offset,size)` records
at `backing+capacity-(index+1)*8`, growing that registry downward. Allocations
search aligned gaps and then the remaining tail, reserving another8-byte record.
`22E400` supplies alignment16 to `22E1D0`; memory-class values are passed through.
`22E380` removes an exactly matching registered pointer and compacts the records;
`22E420` routes the size/class free interface to that pointer-only operation.
The snapshots contain every original allocation/free entry, return and registry.
[Original fixed constructor](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/00148190.asm),
[fixed allocator and gap checks](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/0022e1d0.asm),
[alignment wrapper](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/0022e400.asm),
[original pointer release](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/0022e380.asm),
[pool construction](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/0022e4e0.asm).

`148A10` explicitly routes mode1 to the fixed allocator alone. For other modes,
it tries pool480DA0, then fallback480DA4, then the allocator obtained by1485A0.
There is no mode1 expansion or fallback in this routine. Source `148190` sets
mode1; source `1C60A1` sets mode2 only **after `1C66A0` returns**, subject to
original518448. Allocation releases use the pointer's owning range, rather than
current allocation mode alone.
[Original mode router](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/00148a10.asm),
[original release router](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/00148a70.asm),
[original new-map order](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/001c5ff0.asm).

The failing case has 46 fixed records, 80,056 allocated bytes and 368 bytes of
existing registry. The next registry begins at offset85,640. Its largest aligned
hole is **3,496 bytes**; the other sizeable hole is1,872 aligned bytes. Neither
admits the requested `2010` (8,208) bytes. `22E1D0` returnsNULL and original
`2DD621` attempts its first store through that NULL. This is a precise result
for the incomplete input preparation, not evidence of ordinary Xbox exhaustion.
[Negative case and full registry](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/report.json),
[original failing consumer](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/002dd5f0.asm).

## Recovered CRT producer and temporary allocation lifetime

The original `_rtinit` table word468624 points to32C7A6, whose first call is
32CFFC, the CRT lock initializer. `_cinit` walks its C initializers before its
C++ table: C word468CC4 selects3274E4, the exit-array producer, and C++
word468948 selects37D8C0. The oracle executes those required original producers,
the preceding eight original Havok registrations, and the full37D8C0 constructor
in their recovered relative order. Other runtime/TLS/C/C++ initializers remain
outside this bounded preparation.
[Original runtime table loop](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/002d8155.asm),
[original runtime lock caller](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/0032c7a6.asm),
[original C-before-C++ loop](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/002d80fd.asm),
[exact table-word bytes](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/report.json).

`32CFFC` constructs required static CRT locks using their original records and
the already pinned compatibility initializer; it returns1. `3274E4` requests80
bytes through original malloc/process-heap instructions, initializes its exit
array and returns0. `37D8C0` calls original
`2DD590(this=480DB0,backing=480DD0,size=4000)`. The executed stores are:

| Instruction | Original output |
| --- | --- |
| 2DD598 | adapter+08=480DD0 |
| 2DD59B | adapter+0C=4000 |
| 3274D9 via327544/32750C/327464 | destructor37FEE0 written to the source heap-backed exit array |

The backing is the original executable's static 16KiB range. No new OS allocation
or arena enlargement is requested for it. The full original atexit path and
compatibility acquire/release execute; no success is substituted. The callback
is retained but is not invoked in these new-map cases. Its original destructor
37FEE0 changes the adapter vtable to its base45A8D0.
[Complete constructor](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/0037d8c0.asm),
[actual backing registration](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/002dd590.asm),
[actual exit-array initializer](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/003274e4.asm),
[actual atexit writer](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/00327464.asm),
[registered destructor](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/0037fee0.asm).

Original313C90 computes temporary byte demand `(count*8+F)&~F`. At313CD7 it
compares that demand with adapter+0C. With the omitted constructor, available
bytes are0, so the2000 demand calls virtual+24 (`2DD5F0`), which requests2010
from fixed memory. With the producer, available bytes are4000: the unchanged
branch uses480DD0 directly, advances+08 by2000 and decreases+0C by2000.
The recorded actual maximum scratch pointer is **482DE0**, and minimum remaining
bytes are1FF0. Original temporary release restores480DD0/4000 before `1C66A0`
returns. Consequently **no2010 request occurs**
in either recovered-producer case. The report includes all59 adapter-record
stores; no lifetime reset is performed by the fixture.
[Original temporary constructor](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/00313c90.asm),
[original temporary release](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/00313e50.asm),
[all exact original scratch stores](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/report.json).

## New-map cases and bounded completion

| Case | Executed outcome |
| --- | --- |
| Missing original scratch producer | core world06005470 retained; source2010 allocation returnsNULL; stop2DD621; mode1 |
| Recovered CRT scratch, inherited fixture objects | 1C66A0 returns; mode2; BSP insertion returns; 1C6370 stops1DA942 on inherited object's unfixed serialized shape |
| Recovered CRT scratch, original empty object pool | complete1C5FF0 returns, including BSP insertion, listener registration and complete1C6370; mode2 |

All cases use actual Lockout BSP bytes and bounds, unchanged1C6270 before1C66A0,
and original constructors. Shape060051F0, wrapper06005220 and static
body06005250 precede world06005470. In both recovered-producer cases, insertion
changes **static body+08 fromNULL to06005470**. The source's final67 fixed records
total82,868 bytes with536 registry bytes, still within original15000 capacity.
Mode2's first pool allocation runs unchanged22E950 and returns0601A380; the
fixed capacity remains untouched.
[Exact source calls, returned stages and retained objects](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/report.json),
[original BSP world insertion](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/001c7320.asm),
[original pool allocation](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/0022e950.asm).

The complete case obtains its header pool from original producers. It executes
125280's original physical/game-arena prefix and its1252BF tail, preserving the
actual frame across the **excluded217090 savegame/thread-I/O call**. Original
source produces the game-arena allocator. B76C2..B76E0 uses that allocator to
construct a2048-by-12 object-header pool;16D210 activates/resets it. All membership
and salted initialization come from those instructions. The pool is empty,
with next saltE26F. It replaces inherited preconstructed fixture membership;
there is no manual bitmap/flag edit to bypass the later walk. Earlier other-object
initializers and the later dynamic body-heap setup remain excluded.
[Original arena producer](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/00125280.asm),
[actual arena allocator](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/00125e50.asm),
[original pool caller](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/000b76c2.asm),
[original pool creation](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/0016cff0.asm),
[earlier independently frozen pool contract](MP_DATUM_OWNERSHIP_RESEARCH.md).

After complete1C6370, source1C60DD installs component pool526248 into509998 and
sets50999C/5099A0 toNONE. The source routine returns. The empty-object input is
an explicit bounded case, not a claim that actual Lockout has no authored objects.
The selected arena/BSP-table/object-pool preparation order does not establish
the complete authored startup allocation sequence.
[Original final scheduling stores](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/001c5ff0.asm),
[original remaining-object walk](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/001c6370.asm).

## Remaining boundaries

The fallback record1482F0 exists after full1C5FF0, but its source backing global
518434 is stillNULL in these bounded cases. No fallback allocation is requested.
The exact original backing producer is133760: it requests
`VirtualAlloc(0,452E8,101000,4)`, writes518434 and constructs two1360F0 objects.
That producer's OS allocation and subsequent consumers remain separate work;
its NULL backing is not claimed adequate for later fallback use.
[Original fallback construction](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/001482f0.asm),
[original backing producer listing](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/instructions/00133760.asm).

This extension completes one actual **1C5FF0 routine** with stated produced
inputs, distinct from direct BSP/core constructors. It does not complete whole
XAPI/title/map initialization, ordinary live biped physical construction or any
Havok motion/contact stage. Inherited-object shape/pose state is never supplied
to force its failed path. A live allocated unit should next be produced through
its original object/tag/animation/physical-component constructors against the
retained world; complete dynamic body-heap, authored allocation order and active
world services remain explicit gates. The preceding whole compatibility context
is still compatibility-produced, not original Xbox hardware context.
[Precise scope and all service boundaries](../analysis/multiplayer/integration/spawn-source/fixed-memory-world/report.json),
[prior allocated-biped evidence](MP_BIPED_PHYSICS_CONSTRUCTION_RESEARCH.md).
