# Canister texture publication with actual native queue ownership

This separate TU1.1 observation advances the required canister decal through
original texture-request publication using real native mutex/semaphore objects.
It does **not** admit the decal or any particle. The ordinary effect path next
stops at the absent sound-system record; a separate original worker root stops
at the unopened file-token boundary before any IO.
[Tool](../tools/oracle_canister_async_queue_20261010.py),
[report](../analysis/multiplayer/integration/canister-async-queue-20261010/measured/report.json),
[freeze](../analysis/multiplayer/integration/canister-async-queue-20261010/frozen/manifest.json).
Production, shared CMake/runtime/shaders, releases and earlier freezes are unchanged.

## Actual constructor and wait boundaries

The original canister factory, player/PVS, texture/decal storage and protection
preparation retains the preceding immutable scope and its declared boundaries.
All inherited return-only services are removed before the added roots. A new
thin C transport compiles the **unchanged frozen** `NativeSynchronization`
implementation; its objects are actual recursive native mutexes and a bounded
counting semaphore. Its explicit native alias namespace is not Xbox handles.
[Frozen synchronization source](../analysis/multiplayer/integration/native-worker-threads-20261009/frozen/source/src/native_synchronization.cpp),
[transport](../analysis/multiplayer/integration/canister-async-queue-20261010/native/provider.cpp),
[preceding texture owner](MP_CANISTER_TEXTURE_CACHE_RESEARCH.md).

Original `1221B0` executes its 150×64-byte initialization and pointer stores.
XAPI entries `2DC83A(0,0,0)` twice and `2DC7B9(0,0,150,0)` use the actual native
owner. Only their real returned aliases enter original stores:

| Source slot | Actual native owner | Retained alias |
| --- | --- | --- |
| `4E7928` | free-list mutex | `F5100001` |
| `4E792C` | work-list mutex | `F5100002` |
| `4E7930` | semaphore, initial0/max150 | `F5100003` |

The prefix makes 2,709 nonstack stores. It preserves the source counter and
requests-added fields, initializes free head `4E9EB8=4E7938`, work head0 and
descriptor `[5098A0]={0,5018A0,8000}`. It stops **before** `2D8606` receives
`(0,4000,122350,0,0,0)`: no thread/TLS/lifetime result is supplied, worker slot
`4E7924` stays0, and the constructor is not marked returned. Following roots
form an explicitly bounded composition after this retained prefix; they do not
claim authored complete title startup or a returned platform constructor.
[Complete constructor](../analysis/multiplayer/integration/canister-async-queue-20261010/measured/instructions/001221b0.asm).

The original `2DC9C8 -> 2DC8B2 -> 2DC70A` wait wrappers now execute unchanged to
ordinal234 at `2DC8CF`, returning at `2DC8D5`. Actual arguments are
`(native_alias,UserMode1,alertable0,NULL infinite timeout)`. The adapter observes
real native ownership with a zero-duration native wait. **Only an actual
acquisition** is translated to `STATUS_SUCCESS=0`, after which original wrapper
return logic executes. A timeout/failure stops before a Source result or any
post-wait stores; no finite timeout is passed back as an infinite-wait outcome.
This bounded translation is supported by the pinned nxdk
`NtWaitForSingleObjectEx` acquisition contract and Cxbx's literal
`X_STATUS_SUCCESS=0`; it is not Xbox kernel/thread/TLS/scheduler equivalence.
Four waits acquire real ownership here. XAPI release adapters actually release
ownership/permits and preserve unconsumed EAX; semaphore previous-count output0
comes from the real release.
[Full wait wrapper](../analysis/multiplayer/integration/canister-async-queue-20261010/measured/instructions/002dc8b2.asm),
[pinned nxdk declaration](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h),
[pinned status definition](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/Cxbx-Reloaded-types.h).

## Exact publication and next stops

The same original required decal `E70B0596` requests texture page
`E2780000`, destination `80061000`, transfer `C00`, encoded source location
`890D4600`. Complete `19D8C0 -> 1A4870 -> 121FD0 -> 1220E0` leases the actual
free node, fills and publishes it, and releases one real semaphore permit.

The whole retained node at `4E7938` has priority2, full job0, ten-word task
`{file0,80061000,C00,090D4600,0,0,0,0,0,0}`, category3, callback `1A47D0`,
done address `7E42004F`, next0. Original counter and requests-added become1,
free head becomes `4E7978`, work head becomes `4E7938`. Done remains0.
`12ED10` returns pending1 and `180980` returns0; decal occupancy remains0.
This is publication/pending behavior, not successful texture/decal admission.
[Original publication](../analysis/multiplayer/integration/canister-async-queue-20261010/measured/instructions/001a4870.asm),
[queue publication](../analysis/multiplayer/integration/canister-async-queue-20261010/measured/instructions/001220e0.asm).

The ordinary effect update then reaches required sound `E70F059A` and faults
at `21F366`: `21F361` loads `[4ED950]=0`, then `21F366` reads byte `+78`.
The actual sound-system constructor/reset/cache/driver chain remains necessary.
No guessed disabled/initialized flag or returned sound request is supplied.
The separate original `122360` worker root runs synchronously on the real
native owning thread. It consumes the actual published permit, acquires/releases
the work mutex, stores `518000=4E7938`, and reaches callback `1A47D0(node+8)`.
It stops before the callback because the actual shared file bank still contains
token0. No read, callback result, done byte, unlink, free-list return, IO/GPU
completion or particle birth is supplied.
[Executed stop instructions](../analysis/multiplayer/integration/canister-async-queue-20261010/measured/instructions/executed.asm),
[complete worker](../analysis/multiplayer/integration/canister-async-queue-20261010/measured/instructions/00122360.asm),
[file-open ownership boundary](MP_ANIMATION_ASYNC_WORKER_RESEARCH.md).

The existing async suite proves actual file reads only with an explicitly
bound native file token/path. Original `19D820 -> 19DAD0 -> 19EE30 -> 1A4250`
path/open/token production is still required for a genuine source bank here.
The texture destination additionally overlaps the inherited **whole cache-file
mapping**, whose physical-memory overlay was an older declared fixture
boundary. This exact `C00` span intersects no tagged payload; calling it tagged
metadata would be inaccurate. Complete destination bytes and actual intersections
are retained. This scope performs no IO into that backing and does not remap a
buffer or change capacity to bypass its ownership.

## Reproduction and scope

Run from the repository with `PYTHONPATH=tools` and the bundled Python:

```sh
PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_async_queue_20261010.py --output analysis/multiplayer/integration/canister-async-queue-20261010/measured/report.json
PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_async_queue_20261010.py --output analysis/multiplayer/integration/canister-async-queue-20261010/repeated/report.json
```

The native compilation command, exact inputs/dependencies, repeated instruction
files, full queue/descriptor/file-bank/texture/decal/effect buffers, all source
stores and real native outcomes are frozen separately. This is a TU1.1 dynamic
composition; no new u5 execution is claimed. Authored required parts, class8,
collider controllers, whole-effect clock/birth/rendering and Xbox hardware
behavior remain explicit gates. No GPU is used.

The final composition retains 20 roots, 17 returns and the three exact stops,
88 observed source calls, 22,907 ordered nonstack stores and 89 instruction
files. The report plus all instruction files repeat byte-for-byte. It retains
three real synchronization objects, four actual acquisitions, one actual
semaphore publication and consumption, four real raw allocations/unmaps and
the preceding actually applied memory protection. Particle births remain0.

The preserved first observation exposed a Python return-observer bookkeeping
issue: a deliberately stopped constructor could be labelled returned when a
later explicit root reached the shared sentinel. The final observer discards
only those unreturned observation records between roots, retaining their IDs;
it changes no guest registers, fields, instructions or native operations.
The first report/tool/log remain under `first-observation/`.
