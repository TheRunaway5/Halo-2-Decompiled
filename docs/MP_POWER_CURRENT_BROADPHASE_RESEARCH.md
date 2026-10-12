# Original power-module current broadphase and actual capsule bounds

The native current-broadphase owner now matches 38 complete unchanged original
calls and two composed island-schedule prefixes: 26 capsule bounds, eight current wrapper bounds, two island updates and
two backend updates. Optimized, ASan/UBSan with leak checking, repeated export,
and freshly linked production tests compare 47,336,392 source bytes and 534
ordered owning writes. Both real backend calls produce zero added/removed
pairs and nine endpoint-value writes. That is the measured initial world
result; the implementation does not choose a zero count or supply contacts.

This extension preserves [the retained-agent scheduler freeze](MP_POWER_AGENT_SCHEDULER_RESEARCH.md)
and its complete source preparation. All 547 direct prior pins and their
inherited checks remain intact. The oracle asserts entire prior scheduler
call/read/store/state identity and entire canister schedule identity. No pose,
velocity, current bounds, pair, contact, solver response, admission, clock,
allocator return or success flag is newly supplied.

## Authority and artifacts

Original TU1.1 executable is
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The authenticated owned u5English image has SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Original bytes and original execution own the findings; annotation names and
comments are not evidence.

New native files are `include/halo2/physics_current_broadphase.hpp`,
`src/physics_current_broadphase.cpp` and
`tests/physics_current_broadphase_tests.cpp`. CMake registers only this module
and `original_physics_current_broadphase`. Scene, Simulation, runtime,
renderers and releases are untouched.

Ignored immutable evidence is
`analysis/multiplayer/sandbox-source/power-current-broadphase/final/`.
Original and independent repeat report SHA-256:
`e94a689db0c1c0e53ab98be8d429c654ef958bc0377b47b39bcbcd1a4ac5f34f`.
Both direct original-only fixture files have SHA-256:
`65378f53e1bdf956795a6dfba923f3d814e84cdc7808551d6da099391c4873df`.
The full six-body later correspondence report has SHA-256:
`7fc356215532e834f83e1dbf61f48ff1f437f1247f08204a35c9c00d1f7597ef`.
`exported-schedule/` and its independent repeat retain the two original
composed prefixes, with their separate fixture hash in `audit.json`.
The final manifest pins evidence, tools, native source, test, document and
actual validation artifacts. `native-artifacts/` retains distinct optimized,
sanitizer and newly linked production binaries plus the actual linked systems
library. The archive filename collision diagnostic is retained; subsequent
copies use distinct names and their actual hashes are recorded.

The extension observes 40 complete original calls, 34 deduplicated full
states, 8,037 reads and 4,206 writes. It includes actual constructor/spawn/PVS
capsule and bounds calls, not just the two final islands. The original complete
`300870` calls are retained as source evidence. Two additional source-only
fixtures compose native `31E2C0 -> 31DB70 -> 31DEE0` and stop at the actual
`300870` entry, comparing its exact receiver, arguments, entry ESP and all
preceding state/stores. That owner is never returned as success. All 179 corresponding instruction files and both original reports
repeat byte-identically. The audit checks 15,360 original instruction rows and
3,808 observed read/write sites; 12 external tail sites are checked against the
separately pinned complete prior source execution.

## Complete native current owner

`source_update_physics_island_broadphase_1_1` implements complete `31DB70`.
Its arguments are the real island, backend, step, entry ESP and original FP
control inputs. The actual step argument is retained in the ABI; this routine
reads the owning world/body state rather than substituting a native elapsed
value.

The source sequence is:

1. Construct added then removed pair descriptors through original `313C90`,
   using actual `world+238` capacity (512 in this state).
2. Allocate aligned bounds bytes for `island+40 * 32`, then aligned handle
   bytes for `island+40 * 4`, through the actual `487638` scratch adapter.
   Its cursor and remaining-byte stores retain original order. Fallback
   virtual `+24/+28` is an explicit real allocator/retirement service.
3. Compute margin using original x87 `world+CC -> +8`, multiplied by the
   imported `46502C` word, then spill once at `31DC8D`. The measured result is
   `3C86594B`; it is not a native tuning constant.
4. Walk `island+3C/+40` body pointers in original ascending bank order. Each
   handle is body `+1C`, collidable is body `+C`, current transform is reached
   through collidable `+8`, then `+20`. Dispatch actual shape virtual `+18`
   with transform, margin and the next 32-byte bounds destination.
5. Dispatch backend virtual `+14` with handle pointer, bounds pointer, body
   count, added descriptor and removed descriptor. Actual target is `2E68D0`.
6. Append any source queued pairs from `island+74/+78` after real bank growth,
   then reset the source queued count. This positive branch is listed, not
   executed in these two admitted calls.
7. Release handle scratch then bounds scratch, call existing complete native
   `31D980` with actual added/removed pointers and counts, write `island+30=1`,
   then destroy removed and added descriptors in that order.

For entry ESP `E`, the main source frame is `E-5C`, added descriptor is
`E-34`, and removed descriptor is `E-20`. Bounds/handle pointer spill slots are
frame `+14/+10`; byte counts are frame `+24/+20`; margin is frame `+1C`.
Actual shape-call ESP is frame minus `10`, backend-call ESP frame minus `18`,
and `31D980`'s represented frame at `31D9A7` is frame minus `40`.
These are caller-owned temporary regions, never body or contact identities.
The generic older callback API has an ignored integer return slot for this
void owner; the test adapter returns an empty value solely for that C++ type.
The original parent discards that value. It is not an admission or success
response, and no source register result is replayed.

Unknown actual top-level shape/backend virtual targets require the typed
`required_owner` call with literal site, receiver, arguments and entry ESP.
Current wrapper dispatch supports the two measured resident underlying shape
classes only; any other underlying callback stops. No result is supplied by
default. Profiling-enabled source branches require an actual timestamp-counter
provider after the original preceding stores; profiling is disabled in the
measured state. Unmasked exceptions/FP status effects and Xbox hardware parity
remain outside this comparison.

## Backend endpoint and pair-delta ownership

`source_update_physics_broadphase_bodies_1_1` implements full `2E68D0` from its
980-row original body. It consumes actual current bounds and retained handles;
it does not compute collision geometry or solve a contact.

The source SSE sequence is add the backend min/max origin, multiply by scale
at `+30`, MINPS the imported upper vector, MAXPS the lower vector, then add
bias. All three vectors come from the original image `418C20/418C10/418C00`.
Use the float result bits shifted right seven; minimum is masked `FFFE`,
maximum masked `FFFF` then OR one. This is the original quantization, not a
native cast from a floating coordinate to an integer.

Backend `+40` points to 16-byte retained body rows. The handle's first word
selects its row. Row offsets `0/2/4/6` hold minimum Y/Z and maximum Y/Z
endpoint indices, `8/A` hold minimum/maximum X indices, and `C` holds the
retained handle or special tagged bucket code. Axis descriptors are backend
`+4C/+58/+64`; each four-byte endpoint has unsigned value at `+0` and body
index at `+2`.

For each input body, the full body performs X then Y then Z. Each axis moves
minimum left, maximum right, maximum left, then minimum right, using strict
source comparisons and original row-copy/index-repair/store order. Crossing
an opposing endpoint checks overlap against the other two current index ranges.
X uses the original packed 32-bit subtract/OR sign mask `80008000`; Y and Z
use original wrapped 16-bit subtract/OR sign tests. Qualifying crossings append
the retained own/other handles to the real added/removed descriptor through
actual source bank growth. X's tagged bucket branch appends/removes the body
index in its own original two-byte-row bank. No broadphase pair is equated to a
terrain contact or successful solver response.

The measured canister and player calls do not cross endpoints. Each writes
minimum, maximum, minimum for each axis, even when the value is unchanged.
Those nine real writes and full backend/endpoint/body/bank buffers match. Pair
crossing, tagged bucket mutation and growth are implemented from the complete
listed source, but their positive execution is unmeasured here. They are not
claimed as positive differential coverage or as live collision closure.

## Actual current shape bounds

The measured canister wrapper calls original `303E30` convex bounds, already
ported and independently verified, with its real authored convex data. The
actual player wrapper calls **`2E4670`**, not `2E22B0`. The latter is a distinct
listed support/normal callback reached later by the original narrowphase.

`source_physics_capsule_bounds_1_1` implements complete `2E4670` (54 original
rows). Radius is original x87 margin plus shape `+C`, spilled to float. Two
stored endpoints at shape `+10/+20` are transformed by actual current four-lane
matrix columns at `0/10/20/30`. Original arithmetic first forms
`z*column2 + translation`, then `x*column0 + y*column1`, then their sum.
The source writes maximum endpoints to output `+10`, minimum minus radius to
output `+0`, then reloads maximum and writes maximum plus radius. All four
lanes and the two eight-byte writes per MOVAPS are retained.

`source_physics_current_wrapper_bounds_1_1` implements actual `2E2380` with
convex/capsule dispatch. After underlying bounds, it follows wrapper `+34` to
its real body/world and uses actual motion velocity at `body+3C -> +40` times
world `+150`. It expands maximum with MAXPS and minimum with MINPS in original
order. It never substitutes authored roots for current matrices, invents
velocity, or inserts an approximate collider.

The 26 capsule calls include source spawn diagnostics, identity/current matrix
queries, PVS construction and the actual ordinary world path. The eight
wrapper calls include both canister and player plus the narrowphase's actual
relative transform and positive source margin `3D4985F0`. Native outputs are
compared to original instructions; no expected geometry is recomputed by the
exporter.

## Native fixtures and authenticated later correspondence

`C` retains original entry, EAX/ECX, observed return EAX, entry ESP, x87 control,
MXCSR and 16 argument words. `M/A` are literal original input/output spans.
`W/WS` retain site/address/before/written bytes. Source-owned descriptors,
bounds and meaningful frame slots remain compared even when they live on the
stack. Unrelated ABI pushes, SEH links and internal scratch temporaries remain
source-only; temporary before bytes are excluded where those ABI aliases own
the old contents. All heap before bytes, complete retained buffers and ordered
owning outputs are checked. The source-only exporter never calls native code.

| TU1.1 | Authenticated u5 | Complete body |
|---|---|---|
| 31DB70 | 321700 | current island bounds/backend/pairs |
| 2E68D0 | 2EA380 | retained endpoint and pair delta |
| 2E2380 | 2E5E80 | current wrapper bounds |
| 2E4670 | 2E8120 | actual capsule bounds |
| 300870 | 304350 | retained agent/welder, source-only here |
| 2EFC30 | 2F36F0 | current child-shape producer, next native gate |

All six complete instruction bodies and internal branch topology match after
explicit address relocation. Absolute operand bytes and direct calls are
retained separately. `300870`'s function-pointer operand is genuinely relocated;
its target correspondence was already independently anchored in the prior
scheduler freeze. This report is static authenticated correspondence, not u5
execution or a claim that 1.1 raw runtime addresses can be reused unchanged.

## Minimum remaining live bridge

Existing native `31E2C0` connects its real `31DB70` required-owner call
to this module in the two independently exported prefix comparisons, with the same actual source memory, allocator and temporary
frame ownership. This removes the earliest current-broadphase gap. The
following `31DEE0` still requires actual retained-agent virtual `+10`, target
`300870`. Neither source fixture bytes nor captured contact rows may be used
as live provider results.

The two genuine `300870` calls are retained in `audit.json`: canister agent
`0601ADE0` and player agent `0601AD60`, with real collidable, BSP, step and
output packet arguments. At `3008FB`, each first calls `2EFC30` with its actual
agent, own collidable, temporary shape packet, step and callback `2EF680`.
That complete source owner first executes relative-transform `2E0A60`, then
actual shape bounds. BSP virtual `+30` resolves to original `1F2420`, which
calls `2E5BC0 -> 3066E0 -> 305E40` against the real resident cache MOPP bytes.
The canister execution obtains actual child keys `20000813` and `2000081D`,
then reaches `1F3320` child-shape production, original `1C6C30` filtering and
`2EF680` real child-agent allocation. The actual source contact arises later
through those real owners; this current-broadphase module supplies none of it.

A production Scene bridge still needs those original child-shape/contact-manager
and solver owners, the complete outer world/integration schedule and genuine
retained body/world/shape storage. `SourcePropScene` currently consumes caller
root/lifetime and optional query state; it does not own these source physics
banks. This freeze changes no Scene/Simulation path and proves neither player
versus crate response nor canister damage/destruction, continuous settling,
full title startup, or an active live solver.

Reproduce with separate output directories for extensions:

```sh
.tools/python/bin/python tools/oracle_mp_power_current_broadphase_1_1.py --output analysis/multiplayer/sandbox-source/power-current-broadphase/final/measured/report.json
.tools/python/bin/python tools/oracle_mp_power_current_broadphase_1_1.py --output analysis/multiplayer/sandbox-source/power-current-broadphase/final/repeat/report.json
.tools/python/bin/python tools/export_mp_power_current_broadphase.py --report analysis/multiplayer/sandbox-source/power-current-broadphase/final/measured/report.json --output analysis/multiplayer/sandbox-source/power-current-broadphase/final/exported-owner
.tools/python/bin/python tools/export_mp_power_current_broadphase.py --report analysis/multiplayer/sandbox-source/power-current-broadphase/final/repeat/report.json --output analysis/multiplayer/sandbox-source/power-current-broadphase/final/exported-repeat
.tools/python/bin/python tools/export_mp_power_current_schedule.py --report analysis/multiplayer/sandbox-source/power-agent-scheduler/final/measured/report.json --output analysis/multiplayer/sandbox-source/power-current-broadphase/final/exported-schedule
.tools/python/bin/python tools/export_mp_power_current_schedule.py --report analysis/multiplayer/sandbox-source/power-agent-scheduler/final/repeat/report.json --output analysis/multiplayer/sandbox-source/power-current-broadphase/final/exported-schedule-repeat
.tools/python/bin/python tools/research_mp_power_current_broadphase_correspondence.py
.tools/python/bin/python tools/audit_mp_power_current_broadphase.py
ctest --test-dir build-physics-body-state-20261008 -R '^original_physics_current_broadphase$' --output-on-failure
```

Frozen writes reject differing bytes. All earlier freezes remain unchanged.
