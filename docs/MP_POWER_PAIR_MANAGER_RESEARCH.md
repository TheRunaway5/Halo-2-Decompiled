# Original power-module pair manager and retained agent ownership

The unchanged Lockout source composition now executes the actual pair filter,
manager, static callback and PVS island callback through retained contact-manager
and agent allocation, validation and island-bank commit. The native owner matches
109 independently captured original calls, 128,995,350 complete source buffer
bytes and 1,275 ordered stores. This constructs real retained pair storage; it
does not establish a solved contact, collision response, damage or explosion.

This is a separate extension of the immutable
[original broadphase freeze](MP_POWER_BROADPHASE_RESEARCH.md). Its source setup,
authored power-module identity/root, player/loadout construction, PVS and six
ordinary/physics/dirty/new-object phases are unchanged. The oracle asserts whole
prior broadphase and power schedule identity. No new guest fields, flags, pair
list, pose, contact, allocation return, filter success or solver output is
supplied. The original allocator operates its existing source-created backing
and free lists; no arena expansion is added.

## Authority and reproduction

The original TU1.1 instructions come from
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Authenticated owned u5English SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Unverified annotation names/comments are not evidence. The later report checks
47 complete routine bodies, branch topology, absolute memory operands, direct
calls, constructor vtables and the original agent-registration packet. This is
static correspondence, not execution of the 1.5 image or automatic relocation of
1.1 runtime storage.

The new files are `tools/oracle_mp_power_pair_manager_1_1.py`,
`tools/export_mp_power_pair_manager.py`,
`tools/research_mp_power_pair_manager_correspondence.py`,
`tools/audit_mp_power_pair_manager.py`,
`include/halo2/physics_pair_manager.hpp`, `src/physics_pair_manager.cpp` and
`tests/physics_pair_manager_tests.cpp`. Only their source/test CMake registration
is added. Scene, Simulation, rendering, releases and earlier evidence are untouched.

Ignored freeze: `analysis/multiplayer/sandbox-source/power-pair-manager/final/`.
The independent oracle runs capture 190 calls, 118 deduplicated complete retained
states, 2,316 reads and 1,591 stores. Reports and complete original instruction
files repeat byte-identically. Source-only exports also repeat identically; they
contain no native-derived expected values. `manifest.json` pins the final report,
export, complete correspondence, audit, new files, copied source dependencies,
fresh linked test artifacts and validation logs. The earlier missing-target
build log is retained; after configuring the updated CMake target, the actual
production library and new test executable were rebuilt successfully.

```sh
.tools/python/bin/python tools/oracle_mp_power_pair_manager_1_1.py --output analysis/multiplayer/sandbox-source/power-pair-manager/final/measured/report.json
.tools/python/bin/python tools/oracle_mp_power_pair_manager_1_1.py --output analysis/multiplayer/sandbox-source/power-pair-manager/final/repeat/report.json
.tools/python/bin/python tools/export_mp_power_pair_manager.py --report analysis/multiplayer/sandbox-source/power-pair-manager/final/measured/report.json --output analysis/multiplayer/sandbox-source/power-pair-manager/final/exported-owner
.tools/python/bin/python tools/export_mp_power_pair_manager.py --report analysis/multiplayer/sandbox-source/power-pair-manager/final/repeat/report.json --output analysis/multiplayer/sandbox-source/power-pair-manager/final/exported-repeat
.tools/python/bin/python tools/research_mp_power_pair_manager_correspondence.py --output analysis/multiplayer/sandbox-source/power-pair-manager/final/later-correspondence/report.json
.tools/python/bin/python tools/audit_mp_power_pair_manager.py
ctest --test-dir build-physics-body-state-20261008 -R '^original_physics_pair_manager$' --output-on-failure
```

Use a new output directory when extending an experiment. Frozen-write guards
reject different bytes at existing files.

## Actual source order and retained ownership

`378EB0` merges added/removed pairs using each handle's stored `+4` identity. It
skips equal pairs, dispatches removed rows through the selected handler's virtual
`+8`, and delivers added rows only after the actual filter's virtual `0` writes
and returns its output byte. Handler index is
`first_handle+8 * 8 + second_handle+8`; table entries and virtual targets come from
retained manager storage. No native accepted-pair list is provided.

`2E5F50` reads collidable `+1C` group/layer keys. Equal nonzero upper-16-bit groups
write zero at `2E5F78`; otherwise the original filter row
`filter+14+(left&31)*4` supplies the bit selected by `right&31`, written at
`2E5F99`. The executed rows write `01` because their actual stored row/key inputs
produce it. The native implementation computes that predicate and retains the
output address; it does not choose success.

The initial static insertion calls `314020`. Its actual other body is the BSP,
whose byte `+40` is nonzero, so this callback returns. Complete source-listed
nonstatic branches append the pair to the other island's `+80/+84/+88` bank and
retain the island in the static-list `+8/+C/+10` bank. Static removal `3140F0`
also returns for the executed static other body; its source-listed dynamic branch
uses `314180` to append the removed pair to island `+8C/+90/+94`, then retains the
island in the static list. Those positive dynamic branches are not newly measured.

The genuine loadout sequence additionally exercises `31C430/31C4B0` for handle
kind `2`, with actual body virtual `+1C→312010` and `+20→311F80`. These append and
remove the actual collidable in the entity's `+C0/+C4/+C8` bank. Original count,
backing growth, swap-with-last removal and their store order are retained. Actual
listener count `+40` is zero, so `311490/3114F0` return through their empty-list
branches. Positive listener callbacks remain required owners; no listener accepts
a pair by default.

The PVS body path calls `31D980`, which orders each pair via `379140`, allocates a
16-byte-row scratch bank via `313EF0`, installs the actual island secondary list
at `world manager+24`, and calls `378EB0`. Both measured added batches contain one
pair; empty batches also execute. The internal `379070` multiple-pair quicksort
is not measured or silently replaced. After delivery, `31D980` restores
`manager+24=manager+100`, copies remaining retained rows, allocates/grows the real
island bank `+68/+6C/+70`, commits rows and returns scratch storage through
`313F80`.

The PVS callback `31D360` first handles old-row ordering/matching and writes the
actual handles into its fresh 16-byte row. The actual other island is the static
BSP island, so the source `3131C0`, `31D210` and `3136B0` positive merge/wake
branches are skipped. Those remain required direct owners when their conditions
are encountered; no island readiness is supplied.

The real dispatcher then runs this constructor chain:

1. Body `+48` values select the stored contact-manager factory. Its virtual
   `+4→31F770` requests `0x120` bytes/class `0x1D` through the original allocator,
   writes the size word and calls `31F6C0`.
2. `31F6C0` installs VT `41B6B4`, constructs its `+10` constraint via `322590`,
   retains the factory context at manager `+8`, and binds the two bodies through
   `322060/3220D0`. Those increment actual body reference words, append the
   constraint to each body listener bank `+88/+8C/+90`, store constraint body
   pointers `+C/+10`, then the manager writes `+C=1`.
3. Actual wrapper getter `A0120` returns `0x17` (23). Actual BSP getter `A47B0`
   returns `0x14` (20), from its unchanged `mov eax,0x14`. The stored agent table
   index is `23*32+20`; its actual function is `300170`. The later mapping is
   anchored by the complete original `301040` registration packet, not a guessed
   equivalent constructor.
4. `300170` requests `0x40` bytes/class `0x1A`, writes its size word, runs
   `2EFB60` with the actual manager and installs VT `41959C`. `2EFB60` retains
   manager `+8`, initializes the empty bank `+C/+10/+14`, and writes the original
   `7F7FFFFF` vectors at `+30` then `+20` as two ordered 8-byte stores each.
5. `31D360` stores the real agent at row `+8`. Source motion getters return `4`
   for the canister and `5` for the player. The source world validator virtual
   `+4→27DA90→27E4A0` then executes; actual BSP byte `+40` admits its static-body
   branch. `27DA90` writes the original returned AL into its output byte. The
   caller consumes that byte and writes row `+C=0`, then advances its cursor by
   `0x10`. This gating word is not evidence of a solved contact.

Observed actual agent commits are `0601ADE0` for the canister pair and `0601AD60`
for the player pair. These are original measurement identities, never native
allocation constants or callback results. The native owner obtains new identities
from the actual small-block allocator's retained bank/freelist state.

`322590` owns exact constraint stores, including reference `+6=1`, vtables,
inline banks and raw constants `3E88D677`, `3F08D677`, `3F4D41B3`. Padding and
unowned fields survive. Older-body rebinding follows original listener removal,
reference decrement, signed resident-reference guard and destruction virtual call;
positive older-body retirement is not exercised by these fresh constructors.
Missing world notification/destructor classes remain hard provider boundaries.

## ABI, buffers and native use

All APIs require real `SourcePhysicsContactMemory` backing. The allocator adapter,
backend mode, virtual methods, free-list/class-table storage, filter rows, world,
island, body, shapes and dispatcher tables are inputs produced by earlier owners.
Missing backing or an unsupported virtual/direct owner throws at that boundary;
there is no captured-address provider, automatic contact or default successful
backend. Retained body/property/bank identities stay source-owned.

Temporary aliasing is explicit:

- `SourcePhysicsPairManagerScratch.entry_esp` is the actual original entry ESP.
  `378EB0` uses filter output at `entry+4` for the merge loop and `entry+14` for
  the remaining-added loop. Its callback entry is `entry-1C`.
- `31D360` uses its validator output at its own `entry+4`. This aliases an ABI
  argument byte and does not mutate the retained pair handle bank.
- `source_update_physics_island_pairs_1_1` takes the original frame at `31D9A7`.
  Its scratch descriptor is `frame+C`, and nested `378EB0` entry is `frame-1C`.
  The caller must provide this real temporary backing; it is not a body, agent
  or contact identity.

The exporter checks every observed read/store against a temporal source memory
map, imports virtual-table words only from original XBE bytes, and correlates
retained after buffers with complete original snapshots. Expected buffers are
original owning writes applied to original before state. Semantic stack outputs
compare exact instruction/address/written bytes; their original before bytes are
retained but excluded from the native before-byte comparison because internal
source ABI stores reuse them. Other internal stack/FS plumbing is source-only.
Original low-byte writes in collidable notification and sort ABI arguments preserve
their upper bytes. Required callback packets retain those literal words, including
source pointer upper bits; they are not normalized to a native boolean. Positive
listener delivery and multiple-pair sorting remain unmeasured required owners.

No x87 operation occurs in these represented pair owners; immediate float words
are stored as bits. Whole-frame/SEH/hardware register parity is not claimed.

Native integration may now supply actual static/PVS pair-delivery callbacks to
`source_insert_physics_world_broadphase_1_1`, invoking this manager and island
owner on the same real retained world/body/shape/bank state. It must establish
actual temporary storage/ABI and required backend/listener/retirement providers.
This completes the measured one-pair insertion/storage segment, not a complete
world step or solver.

## Validation and remaining live gate

The normal optimized build and ASan/UBSan build both match all 109 original calls,
full buffers and ordered stores. Every actually used new/body-state/disposal
source is instrumented in the sanitizer comparison, with leak detection enabled.
The separately configured production `halo2_systems` library and newly linked
CTest executable also pass. Prior manifest pins, whole source reports and
original schedule remain intact.

The next real owner is the scheduler that consumes these retained agent rows,
then actual collision/solver-manager execution. The stored agent VT `41959C`
has actual slots `+4→2FFCD0`, `+8→3010F0`, `+C→301170`; these concrete bodies
are not executed by this scope and their owning scheduler/admission remains open. No player-versus-canister
positive contact, penetration, impulse or damage callback is produced by this
scope. Required world stepping, concrete agent collision/terrain traversal,
contact-listener delivery, current body/pose scheduling and destruction/damage
remain open. Scene/Simulation still need an owned live bridge with these real
producers; replaying these measured pairs or replacing the authored convex shape
with an AABB/capsule would not close it. Platform/model/math and ordinary frame
admission boundaries inherited from earlier scopes remain unchanged.
