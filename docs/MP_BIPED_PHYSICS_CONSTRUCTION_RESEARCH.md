# Original allocated biped physics construction on Lockout

The separate **seven-case** suite reaches the original rigid-body constructor
from actual `B8A30` biped allocation, with source-produced animation, node
matrices, location and physical-component records. Three authored starts reach
the same missing platform context at **`318107: mov eax,fs:[0]`**. No live biped
constructor completes; no ordinary-world biped motion or contact is claimed.
The prior death-tail and placement suites remain frozen. [Oracle](../tools/oracle_mp_spawn_biped_physics.py),
[report](../analysis/multiplayer/integration/spawn-source/biped-physics/report.json).

## Authority and reproducibility

| Input/output | SHA-256 |
| --- | --- |
| TU1.1 original `default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Original `lockout.map` | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |
| Original `shared.map` | `084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af` |
| New oracle | `a2afef3a4762b991bf1cb974042b858b952b7d712583afd44960f766532abbcd` |
| New report | `928cf90743fd274461c594c7470708f5505b57fbb65b29c23b4bb69a597fea1c` |
| Canonical instruction manifest | `2966f122aa0ef461cdefd69e65aca7e306624c61059a7630faa341521db7dbad` |

Run `.tools/python/bin/python tools/oracle_mp_spawn_biped_physics.py`. The suite
checks seven earlier tool/report pins before and after execution, audits all
**10,051 executed original instruction addresses**, and writes **84 independent
listings / 9,492 listing rows**. Listing rows include unexecuted consumers and
reachable static branches; their per-listing execution counts are explicit.
The executable instructions are unchanged. [Oracle `main`](../tools/oracle_mp_spawn_biped_physics.py),
[report `frozen_pins`, `instruction_listings`](../analysis/multiplayer/integration/spawn-source/biped-physics/report.json).

## Original producers and admitted boundaries

Each creation descriptor is the exact output of the pinned TU1.1 normal player
setup in the earlier placement report, for actual Lockout authored starts
**0, 32 or 65**. Original `B8A30` allocates datum `00010002` and generic body
storage `010E2000`, then the original generic/unit/biped constructors grow its
logical size from **1,124 to 5,750 bytes**. The object heap supplies raw backing
and no-relocation growth; it does not supply the biped fields or pose. The
inherited object header pool has 64 entries and two excluded existing entries,
so this datum is a bounded fixture result, not the complete original startup
allocation sequence. [Earlier placement producer](../analysis/multiplayer/integration/spawn-source/placement-report.json),
[new allocation requests and complete raw body](../analysis/multiplayer/integration/spawn-source/biped-physics/report.json),
[B8A30](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/000b8a30.asm),
[BD170](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/000bd170.asm).

The new suite removes the earlier supplied-unavailable `1D0A60` animation
service. Original manager construction/bind, `C52C0` and the biped initializer
execute. Original `BDF10→BDF80→1CA950→280030→1E11B0` selects the actual animation
resource and `27FB10` samples it into source-owned node matrices. The source
manager is at body `+1426`, graph **`E9180793`**, packed animation **`0104FFFF`**.
No manager offset, clip, component field, root/node/head pose or contact is
written as a new fixture. [Constructor/sampler listings](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/000c52c0.asm),
[BDF80](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/000bdf80.asm),
[recorded original call order/raw manager](../analysis/multiplayer/integration/spawn-source/biped-physics/report.json).

The resource subsystem remains a declared boundary. Without resource delivery,
the unchanged constructor naturally reaches **`124DE8`**, dereferencing absent
resource-retirement pool **`4EB11C+38`**; it has not yet assigned a physical
component or valid location. In the five explicitly admitted resource cases,
`124F30` delivers only exact original cache bytes. Every request is for graph
resource **16**, descriptor **`802D19A4`**, Lockout file offset **`00BB4800`**,
size **30,948**, SHA-256
`d039d4c8457bc1f02905c2f83e0a6c4e6b956a0a01a47905e16d021a348d012c`.
Original resource scheduling, retirement/cache pools and I/O are unexecuted;
the original caller's opaque stack argument is recorded without assigning it a
meaning. No returned animation or pose is substituted. [Strict resource case and `resource_requests`](../analysis/multiplayer/integration/spawn-source/biped-physics/report.json),
[original resource consumer](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/00124dd0.asm),
[resource scheduler](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/00124f30.asm).

The actual Lockout BSP payload, source point classification and source cluster
registration execute. Active BSP/cluster backing and the requested live-physics
context byte `486560=1` are inherited startup inputs. Complete original world
initialization has not supplied them. [Location producer](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/0011d4a0.asm),
[point classifier](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/0014b9d0.asm),
[recorded startup boundaries](../analysis/multiplayer/integration/spawn-source/biped-physics/report.json).

## Runtime shape construction

The source tag iterator `1243C0` reads literal original cache index/instance
records and returns salted masterchief_mp datum **`E9F2086D`**. The bounded
original dispatcher fragment **`11DE40..11DED1`** selects its `bipd` branch,
loads the actual tag definition, adds **`264`**, and calls complete original
**`1C5CF0` with EDX=that physics-definition address**. The selected iterator
start/groupNONE and cache-table view are explicit inputs; unrelated whole-map
postprocessing is unexecuted. [Iterator](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001243c0.asm),
[dispatcher](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/0011de10.asm),
[shape fixup](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001c5cf0.asm).

The physical definition owns count/pointer blocks at `+20/+24` (stride `80`),
`+28/+2C` (stride `50`) and `+30/+34` (stride `80`). Original `1C5CF0` constructs
their embedded shapes: pair record `+20` gets vtable `45C670`, paired `+30`
gets `45C648` and an owner pointer back to `+20`; single record `+20` gets
capsule vtable **`45C6B0`**. It zeros embedded `+8`, stores ushort `80` at
embedded `+6`, and preserves authored radius/endpoints. [Exact stores](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001c5cf0.asm),
[base decompiled equivalent](../tools/reference/halo2-decompiled/src/unknown_1c22c0.cpp).

Lockout has three pair records and two capsule records, with no third-bank
records. The first capsule record is at **`802E9B50`**, its embedded shape at
**`802E9B70`**. The serialized vtable is **`00BF5950`**; only the original
constructor changes it to `0045C6B0`. The authored radius is bits `3E333333`;
standing endpoint Z values are `3F0CCCCD` and `3E333333`, crouched upper endpoint
Z is `3EA66666`. Their embedded fourth components are the authored radius.
The unchanged virtual getter then returns source shape type **7**. Without
this producer, actual creation fails at **`1DA942`**, reading serialized
vtable slot **`00BF5964`**. [Before/after literal records and omitted-fixup case](../analysis/multiplayer/integration/spawn-source/biped-physics/report.json),
[shape consumer](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001da6f0.asm).

## Source component pool and Havok storage

Full original **`1D2620`** invokes original allocator object `480D68` through
its original vtable `45A480` and method `11E150`. The source request is
**`1409B` bytes**; `11E150` calls `VirtualAlloc(0,size,101000,4)`. The suite
supplies raw mapped OS backing **`06200000`**, then original `16D070` constructs
the **512-entry / `A0`-byte** pool named **`havok components`**, with alignment
4. It installs global `526248`, and full `16D210`/`16D220` resets the pool.
The source name-derived salt is **`E168`**, and original `16D2C0` produces
first datum **`E1680000`**. Source component storage starts at **`06200050`**.
The earlier inherited 64-entry physical pool is not used by this source path.
[Pool initializer](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001d2620.asm),
[original OS allocator method](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/0011e150.asm),
[header initializer](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/0016d070.asm),
[source reset](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/0016d220.asm).

Original **`1C7050→1C6F50→1D2AA0→16D2C0→1D3340`** assigns that component to
biped `+B4` and stores owner datum `00010002` at component `+8`. `1D3340` owns
the initial NONE fields, counters and empty dynamic arrays. Its `+10` signed
count is produced by converting signed-short tick rate 30 to scalar float,
multiplying source constant `465198`, spilling float32, `fld/fistp` under
FPCW `027F`, then integer negation; the observed output is **−10**. After
the biped body-building prefix, component `+4` is **`00040207`**, `+70/+74`
are **0/0**, and `+78` is **`80000000`**. Those empty arrays are constructor
outputs before a rigid body has been appended, not valid world-body inputs.
[Component allocation](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001d2aa0.asm),
[exact component stores/conversion](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001d3340.asm),
[source body-building flags](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001da6f0.asm),
[retained records](../analysis/multiplayer/integration/spawn-source/biped-physics/report.json).

Original **`1482A0`** requests
`VirtualAlloc(0,106470,101000,4)`; only raw OS backing **`06000000`** is supplied.
The source aligns to `06001000` and writes its original allocation prefix.
Full **`148190`** constructs fixed-memory vtable **`45E77C`**, pool-memory
vtable **`45E7A8`**, and sets allocation mode **1**. The original singleton
install slice **`2DED5C..2DED67`** calls `148B00→2DD5B0` and installs the
original static game hkMemory **`480DB0` / vtable `45A8FC`** at `487638`.
This exact install slice excludes the earlier FS prologue and all remaining
Havok initialization. [OS backing/alignment](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001482a0.asm),
[memory constructors](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/00148190.asm),
[singleton install](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/002ded30.asm),
[base allocator model](../tools/reference/halo2-decompiled/src/unknown_146a20.cpp).

The actual body allocator **`1DAAC0`** asks original hkMemory virtual slot `+10`
for **`A0` bytes, class `28`**. Original **`148A10→22E400→22E1D0`** in allocation
mode1 returns **`06001030`**. The source writes ushort `A0` at allocation `+4`
and calls `318100` with ECX pointing to it; the full source construction-info
record is retained in the report. No Havok allocation return or class fields
are supplied. Without singleton setup, the same actual biped blocks at
**`1DAC99`** while dereferencing `487638=NULL`. [Body allocator](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001daac0.asm),
[game memory dispatch](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/00148a10.asm),
[fixed memory dispatch](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/0022e400.asm),
[raw allocator](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/0022e1d0.asm).

## Exact platform stop and retained outputs

Original `318100` pushes −1 and exception-handler address `37BEF3`, then
**`318107`** reads the original **FS-linked exception-registration head**.
The guest has no original platform FS context; the access resolves to unmapped
zero. No sentinel or backing record is supplied. Thus it stops before
`312410`, before the `41A950` rigid-body vtable store at `318128`, and before
the nested motion constructor. The allocation contains only source-written
size metadata, not a completed rigid body. Independently, full source new-map
physics entry **`1C5FF0`** reaches its real callback **`469290→2DED30`** and
stops at **`2DED37`** on the same FS read. It has not reached its original
world constructors, pool reset, live-flag store `1C6086`, or allocation-mode2
switch `1C60A1`. [Rigid-body prologue](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/00318100.asm),
[full Havok init prologue](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/002ded30.asm),
[new-map physics caller](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001c5ff0.asm),
[independent blocked cases](../analysis/multiplayer/integration/spawn-source/biped-physics/report.json).

| Authored start | Source root XYZ float bits | Source location leaf / cluster / BSP |
| --- | --- | --- |
| 0 | `40E11301 BE09D990 C10476D3` | 15 / 1 / 0 |
| 32 | `C10FDFCB 3F834A5C C1223342` | 2111 / 9 / 0 |
| 65 | `BE273FEF 413EC528 C1423332` | 2260 / 15 / 0 |

Each preserves authored root bits, state `+3DC=1`, adjustment counter `+3AA=0`,
source manager/pose and component `E1680000`. The only root stores are source
allocation clearing followed by the three authored-float stores; no collision
ray query occurs before this stop. This does not establish what a later
completed physics/world update would do to the root. All raw body/component,
construction-info, source stores, allocator arguments and the failing
instruction registers are retained. [Three source-created prefix cases](../analysis/multiplayer/integration/spawn-source/biped-physics/report.json).

## First-world consumers: identified, not executed on the partial body

The unchanged source scheduler has **`1397C0→B7E00`, `1397C5→1C7660`,
`1397CA→B7F50`, `1397CF→B8040`**. Ordinary `B7E00` skips headers with new-object
bit8; `B8040` clears that bit for admitted new/active/nondeleted objects and
then calls `BD360`. Original `BD360→10CDF0` dispatches the original biped
type table in order: unit callback **`C7E90`**, then biped callback **`DE960`**.
The literal original tables are in the report. These are static source
consumer findings, not a world tick executed with a completed live biped.
[Scheduler](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/00139750.asm),
[ordinary pass](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/000b7e00.asm),
[new-object pass](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/000b8040.asm),
[dispatch](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/0010cdf0.asm).

`DE960` rejects componentNONE. Its unparented movement route builds physics
input at **`DDBC0`**, then calls `1E9480` and `E3B20` before its
mode-specific actions and animation/camera tail. Crucially, `DDBC0` reads
**component `+70` → first body slot `+40` → body/motion state**, including
motion linear velocity at `+40/+44/+48`. Source `1D3A70` is the later producer
which grows the `60`-byte body-slot array and increments `+74`; it was never
reached after the blocked `318100`. Calling the movement consumer with this
empty partially constructed component would not prove ordinary motion.
[Biped update](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/000de960.asm),
[physical input consumer](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/000ddbc0.asm),
[body-slot producer](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001d3a70.asm).

Global physics stage **`1C7660`** requires a real world at `526234`, accesses
manager `526538`, and calls source **`317650`** for the world step, followed
by contact/collection consumers. Complete new-map initialization and the body
constructor must be recovered before those inputs exist. The report deliberately
records **zero completed live biped constructors, zero ordinary live-world
motion cases and zero source-generated live contacts**. It neither supplies a
world-step result nor treats partial construction as successful creation.
[Global physics stage](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/001c7660.asm),
[world-step listing](../analysis/multiplayer/integration/spawn-source/biped-physics/instructions/00317650.asm),
[report scope](../analysis/multiplayer/integration/spawn-source/biped-physics/report.json).

The next source gate is an independently established platform FS/thread/exception
context, followed by complete Havok map/world construction and body insertion.
Resource I/O, whole-world allocation order, active-world contacts and verified
Xbox floating-point behavior remain separate boundaries. No production or prior
frozen output was edited. [Oracle boundaries](../tools/oracle_mp_spawn_biped_physics.py).
