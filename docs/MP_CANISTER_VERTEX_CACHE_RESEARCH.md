# Original canister decal vertex-cache owner

This separate CPU experiment closes the previously stopped `43880` vertex-cache
constructor and whole `17EC90`/`17ED50` decal construction/reset using genuine
host address-space storage. It does **not** admit the complete canister effect:
the first effect update next stops in the absent texture-cache owner, before any
particle is born. Production, runtime, shaders, previous evidence and releases
are unchanged.

## Original allocation protocol

The untouched TU1.1 caller `43880` invokes `33785C(0,12,0x101000,4)`.
`33785C` passes five arguments through thunk `417C34`, whose original value is
encoded ordinal184 (`800000B8`): `BaseAddress*`, `ZeroBits=0`, `RegionSize*`,
allocation type, protection. On signed nonnegative NTSTATUS it returns the
updated base; on negative status it calls `2D81D4` and returns NULL. The caller
tests NULL, calls `GetLastError` on failure, then reaches its first object store.
No allocator return is supplied at the wrapper or caller.
[Complete wrapper](../analysis/multiplayer/integration/canister-vertex-cache-20261010/final-measured/instructions/0033785c.asm),
[complete caller](../analysis/multiplayer/integration/canister-vertex-cache-20261010/final-measured/instructions/00043880.asm).

Pinned nxdk declarations identify `PAGE_READWRITE=4`, `MEM_COMMIT=0x1000`,
`MEM_TOP_DOWN=0x100000`, and `MEM_NOZERO=0x800000`; this request has no NOZERO bit.
Pinned Cxbx `VMManager::XbAllocateVirtualMemory` treats a NULL base as reservation,
rounds the requested size to4KiB, chooses a64KiB-aligned top-down base, attempts
real host allocation/commit, and only after success writes updated base/size and
returns status0. Its page-table, physical-page and complete VMManager execution
remain excluded here. These are compatibility ABI evidence, not independently
verified Xbox hardware defaults.
[Pinned declarations](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h),
[pinned implementation](../analysis/multiplayer/integration/spawn-source/local-map-protection/primary-source/VMManager.cpp).
The compatibility address-domain definitions are retained from the same
[pinned upstream source](https://raw.githubusercontent.com/Cxbx-Reloaded/Cxbx-Reloaded/585c49a50af1255ab155099e06f24505f9c5a800/src/common/AddressRanges.h).

## Explicit native platform adapter

The new oracle supplies only the original ordinal184 boundary. For this exact
request it searches the pinned compatibility user-address domain top-down at
64KiB boundaries, excludes actually mapped guest ranges, then calls Linux
`mmap` with `MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE` and real read/write
permissions. It requires the returned host address to equal the requested
address and shares that exact storage with `Unicorn.mem_map_ptr`. Actual EEXIST
can retry lower; other allocation failures stop without a fabricated Source
failure/success. This is a named Linux allocation adaptation; its address-choice
algorithm does not claim to execute Cxbx's complete VMA manager.
[Adapter](../tools/oracle_canister_vertex_cache_20261010.py),
[local Linux headers/manual and hashes](../analysis/multiplayer/integration/canister-vertex-cache-20261010/primary/files.json).

The observed request rounds12→4096 bytes. The top candidate is7FFE0000, but444
candidate pages overlap genuine existing guest cache backing. The actual host
and guest allocation is **7E420000**, with observed `/proc/self/maps` permissions
`rw-p`; no failed host allocation occurred. The complete4096-byte Linux-produced
initial page is zero, and every retained stage checks host bytes against guest
bytes. Only after actual mapping succeeds does the adapter write the original
API's base/size outputs and translate success to0. The unchanged wrapper returns
7E420000 with stack delta20. Cleanup actually unmaps guest storage and `munmap`
returns0. Full platform ABI registers, pointer outputs, mapped ranges, complete
raw page and cleanup outcome are recorded.
[Measured report](../analysis/multiplayer/integration/canister-vertex-cache-20261010/final-measured/report.json).

## Source-owned retained outputs

The unchanged bounded factory/player/PVS preparation correlates with the prior
canister freeze. All inherited return-only services are removed before the new
roots. The original effect and mask constructors/resets execute, followed by
whole `17EC90` and `17ED50` with no guest instruction patches or supplied engine
state. `43880` returns naturally and produces:

| Owner | Actual source-produced output |
| --- | --- |
| `510C34` | 7E420000;12-byte object words `1,03131000,0` |
| Arena tail |14000 bytes at unmasked source address83131000; source stores masked GPU offset03131000 |
| `510C38` |82F0D444, actual613C-byte page allocator |
| `13E8E0` | actual name pointer457A50; capacity500, shift6, entry count400; callbacks43810/43840, NULL third callback; actual allocator82D48288 |
| `4CFCB8` |400 DWORDs set toFFFFFFFF by unchanged43880 |
| `4F1F20` |82EF974C, actual100CC-byte decal-pool allocation;1024×64 records |
| `4F1F1C` |82F09818, actual3C2C-byte decal context |

`17ED50` zeros the3C2C-byte context, then fills its first3800 bytes withFFFFFFFF,
writes context+3800=FFFFFFFF and+3804/+3808=0, sets pool+29=1, and executes
original `16D220`. Every stage retains complete613C-byte allocator,100CC-byte
pool,3C2C-byte context,14000-byte source arena tail,1000-byte lookup table, arena
fields, raw mapped page, all effect pools, original calls and ordered non-stack
stores. The stored03131000 is preserved as a GPU offset; it is not dereferenced
as a host pointer.
[Constructor](../analysis/multiplayer/integration/canister-vertex-cache-20261010/final-measured/instructions/0017ec90.asm),
[reset](../analysis/multiplayer/integration/canister-vertex-cache-20261010/final-measured/instructions/0017ed50.asm),
[allocator constructor](../analysis/multiplayer/integration/canister-vertex-cache-20261010/final-measured/instructions/0013e8e0.asm).

## Exact next dependency

Actual source canister E26F0000 retains the separately declared diagnostic
`D7E00` death root. Seven `D73E0` updates return. Source effect E6650000 and its
location remain allocated with actual player-produced PVS `FFDF0000`.
The first `1788B0` update reaches authored event1's first required decalE70B0596,
then `180980` requests its texture. `12ED10` calls `13EAE0` at12ED43 with literal
arguments **`(allocator0,sizeB00,priority5)`**, returning to12ED48. `13EAFA` faults
reading NULL+34 because original `[4EDA34]` is NULL. The new decal vertex
allocator `[510C38]` is valid; these are different owners.
[Texture request](../analysis/multiplayer/integration/canister-vertex-cache-20261010/final-measured/instructions/0012ed10.asm),
[page request](../analysis/multiplayer/integration/canister-vertex-cache-20261010/final-measured/instructions/0013eae0.asm).

Actual `12D840` is the static writer of4EDA34. Existing independent
[grenade texture research](MP_GRENADE_ACTION_SOURCE_RESEARCH.md) already executes
that constructor and stops in `12D950` at a distinct MmSetAddressProtect request;
its graphics storage/capacity/admission is not imported as success into this
experiment. No required decal, light, sound, object, particle controller or
class8 system is discarded to advance this effect. Particle pools remain empty;
whole-effect birth, collider1/class8, natural lifetime and rendering remain open.

## Reproduction and bounds

Run each command once into a new output directory from the repository root:

```sh
PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_vertex_cache_20261010.py --output analysis/multiplayer/integration/canister-vertex-cache-20261010/final-measured/report.json
PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_vertex_cache_20261010.py --output analysis/multiplayer/integration/canister-vertex-cache-20261010/final-repeated/report.json
.tools/python/bin/python tools/freeze_canister_vertex_cache_20261010.py
```

The selected original root sequence has16 roots,15 returns and one exact stop,
49 observed calls,18,335 non-stack stores and54 instruction files. Complete
unchanged source bodies and executed/store instructions are checked against both
the original XBE and guest bytes. The first observation is retained separately;
the expanded final observation adds complete buffers and ABI observations only.
Independent repeat and pin audit are recorded in the new frozen manifest.

The original TU1.1 XBE, Lockout/shared cache, recorded complete compatibility
thread context, prior source factory/resource/default-variant/clock inputs and
preparation service boundaries retain their earlier pins. Selected roots are
not full title startup, a natural full game frame, natural bullet damage or a
hardware/Xbox VM result. No native/GPU behavior is claimed.
