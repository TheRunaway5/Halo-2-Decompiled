# Original power-module retained-agent schedule and contact delivery

The native retained contact-output delivery now matches the actual unchanged
Lockout canister/world execution. Twelve complete original calls and four exact
required-owner prefixes match 18,939,074 source buffer bytes and 108 ordered
stores, in optimized, ASan/UBSan and newly linked production tests. The complete
original execution independently repeats 406 calls, 26 retained states, 8,918
reads and 4,922 writes. No new contact, count, success, pose, velocity, clock,
allocator result, world admission or solver response is supplied.

This is an independent extension of [the pair-manager freeze](MP_POWER_PAIR_MANAGER_RESEARCH.md).
Its authored power-module datum/root, source-created body/shape/world/player,
PVS and six selected phases are unchanged. The oracle asserts entire prior
pair-manager calls/reads/stores/states and canister phase/call/store/callback/state
identity. All 224 direct prior pins and their inherited checks remain intact.
The original transient canister-to-BSP contact is an original execution result;
this native delivery module does not yet produce live collision geometry.

## Authority and reproduction

Original TU1.1 XBE:
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Authenticated owned u5English SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Original instruction bytes are authority; annotation labels/comments are not.

New files are `include/halo2/physics_agent_scheduler.hpp`,
`src/physics_agent_scheduler.cpp`, `tests/physics_agent_scheduler_tests.cpp`,
`tools/oracle_mp_power_agent_scheduler_1_1.py`,
`tools/export_mp_power_agent_scheduler.py`,
`tools/research_mp_power_agent_scheduler_correspondence.py` and
`tools/audit_mp_power_agent_scheduler.py`. CMake adds only the module and focused
test. Scene, Simulation, gameplay, renderer and releases are untouched.

Ignored immutable evidence is
`analysis/multiplayer/sandbox-source/power-agent-scheduler/final/`.
The original report SHA-256 is
`8a5984c9aa78e1209886fcf6623b518bd60510a14e78ea1964606210defa7141`.
The independent source-only fixture SHA-256 is
`99ee291014691e3521ea80f9fee721439559560b6373c191b5dd5f2f64a177e7`.
The final authenticated correspondence report is `later-complete/complete-report.json`,
SHA-256 `b1015d0675f633c87aa12969abc8ae7df9ffd0fbe9f26aa917f6986eb91bf0aa`.
The earlier 26-body candidate investigation remains separately retained in
`later-correspondence/`; the final report includes the two actual relocated
function-pointer targets and checks their complete bodies.

```sh
.tools/python/bin/python tools/oracle_mp_power_agent_scheduler_1_1.py --output analysis/multiplayer/sandbox-source/power-agent-scheduler/final/measured/report.json
.tools/python/bin/python tools/oracle_mp_power_agent_scheduler_1_1.py --output analysis/multiplayer/sandbox-source/power-agent-scheduler/final/repeat/report.json
.tools/python/bin/python tools/export_mp_power_agent_scheduler.py --report analysis/multiplayer/sandbox-source/power-agent-scheduler/final/measured/report.json --output analysis/multiplayer/sandbox-source/power-agent-scheduler/final/exported-owner
.tools/python/bin/python tools/export_mp_power_agent_scheduler.py --report analysis/multiplayer/sandbox-source/power-agent-scheduler/final/repeat/report.json --output analysis/multiplayer/sandbox-source/power-agent-scheduler/final/exported-repeat
.tools/python/bin/python tools/research_mp_power_agent_scheduler_correspondence.py
.tools/python/bin/python tools/audit_mp_power_agent_scheduler.py
ctest --test-dir build-physics-body-state-20261008 -R '^original_physics_agent_scheduler$' --output-on-failure
```

Use a new output directory for an extension. Frozen writes reject changed bytes.
The two failed observer drafts are retained: the initial observer treated
`31ED30`'s world as ECX, while its original cdecl ABI passes world in argument 0.
The corrected observer reads that actual argument. These were observer errors;
no guest state/result was changed. The first audit also retained its diagnostic
for external tail targets omitted from direct-call function listings. The final
audit checks those 62 sites against the pinned full prior original execution.

## Actual unchanged owner sequence

`1C7660` constructs the real step from its unchanged inherited clock, then calls
`317650(world, step)`. That complete wrapper calls `316200`, then `316440`.
The former walks actual active islands in ascending source order and calls
`31E2C0(island, world+C4, world+140)`. The latter owns subsequent integration,
island/body retirement and end-of-step callbacks. This extension does not port
those complete outer world/frame owners or invent repeated ticks.

`31E2C0` first calls `27F0F0`, which writes the current island to `5660BC` and
snapshots the actual heap-byte total from `148800` at `5660B8`. The getter sums
source heap records reached through `480DA4/480DA8`; no count is supplied.
Next comes `31DB70` current bounds/broadphase/pair update. Stored `world+23E`
selects `31DEE0` when nonzero or `31E190` otherwise. Finally `27F110` clears
`5660BC`, reads the actual heap total again, and sets `island+AE` and `5660B0`
when that total changed. These are source accounting writes, not a simulated
thread lock or a chosen collision admission.

The complete native `source_schedule_physics_island_agents_1_1` retains this
order. Its actual current-update `31DB70` provider is required; both measured
islands stop there in the native differential. It never supplies the provider's
result. The source original continues naturally through the real backend and
retained agents.

`31DEE0` constructs a 64-row scratch output bank with original `31D5F0`.
The 64 is the literal source capacity request at `31DF2D`, not an emitted count.
Rows at `island+68/+6C` are 16 bytes: handles at `+0/+4`, retained agent at `+8`,
and signed threshold state at `+C`. The original loop visits rows in ascending
bank order and skips null agents. It resets the real output descriptor count,
sets actual `world+254` control fields and invokes the retained agent's virtual
`+10`; the actual target is `300870`, not its unused `+4/+8` wrappers.

For nonnegative row state, the source x87 threshold is the signed state times
`world+248`, plus `world+244`, with the original float spill. The emitted distances
and control result determine the source increment/refinement or decrement path;
positive refinement calls real `31CA20`. Negative state uses source word
`C47A0000`; the `-2` branch reads original `465048`. Native `31DEE0` preserves
those operations and required callbacks. Both measured native cases stop at
actual `300870` with exact receiver, four arguments, owning ESP and all preceding
stores; no native agent result is supplied. Refinement/decrement/negative-state,
empty-row and enabled-profiling branches are source-listed, not newly executed
native positive coverage. Alternate `31E190` remains a required owner.

The source then invokes contact-manager virtual `+C→31F550` for each retained
agent. Actual canister manager `0601AFA0` receives one emitted row; actual player
manager `0601B220` receives zero. These addresses are measured identities, not
allocation constants for live use. Original output capacity is 64 in both cases.

## Complete native output/contact refresh segment

`source_construct_physics_agent_output_bank_1_1` and its destructor implement
complete `31D5F0/31D690`: actual adapter `487638`, aligned 48-byte-row scratch
allocation, descriptor/capacity/initial pointer stores, retirement and real
small-block free. Scratch fallback virtual `+24/+28` requires an actual provider.
No new arena or successful allocator callback is added.

`source_deliver_physics_agent_output_1_1` implements complete `31F550`:

1. Follow the fourth argument's real descriptor and emitted count. Each emitted
   48-byte row contains an unsigned index at `+20`. The manager's byte map at
   `+30` selects its retained 32-byte point/normal destination at `+44`.
2. Copy the two actual aligned 16-byte packets in original MOVAPS order. Observe
   their two 8-byte writes per instruction exactly as original execution does.
3. Decrement manager `+C`; when zero, reload `min(bodyA+5C,bodyB+5C)+1` using the
   original unsigned 16-bit fields. No fixed callback delay is invented.
4. Construct the actual five-word packet: manager, collidable A, collidable B,
   **the original output-packet pointer**, and current entity. The third
   argument's step pointer is retained in the ABI but is not substituted into
   that output-pointer field.
5. Dispatch world listeners via `31E9D0`, then nonempty A/B entity listeners via
   `31C3C0`. They visit actual retained listener banks in descending order and
   compact null entries with the original ordered shifts. World sets packet
   `+10=0`; each entity sets packet `+10` to its actual entity pointer.

The actual `27E760` callback is complete `RET4`, implemented as that observed
void leaf. Unknown listener classes require their actual provider; none returns
contact or success by default.

The actual object listener `27E3A0` follows each collidable's parent chain,
requires its original kind `+18==1`, resolves its source body, reads property
keys `2001/2002`, selects the actual physical component from `526248` and tests
its source `+4` bit. Native `source_deliver_physics_object_contact_listener_1_1`
then calls complete `1D4160` with the real component, own/counterpart datums and
own/counterpart slot words. There is no reconstructed body or encoded-key alias.

`source_refresh_physics_entity_contact_bank_1_1` implements `1D4160`'s emitted-row
and retained-contact scan. It matches low-16 index, counterpart datum and signed
slot bytes `+45/+46`, checks the original first collidable/body, and invokes the
existing verified native `1D2F10` contact refresh. Only AL is stored into the
original temporary flag word; the upper 24 bits come from actual caller backing.
The measured false word is `64E39500`, not a synthesized zero word. The source
low byte owns the predicate. Explicit original x87 control/MXCSR inputs are
required. Existing positive character, flipped-normal, wake and impulse branches
retain the contact-refresh module's hard boundaries.

These complete native paths consume actual emitted rows and existing contacts.
They do not allocate an invented contact or replace the missing narrowphase.
The canister's current point/normal/distance and coefficient writes match the
original full source buffers and store order through this complete delivery.

## ABI, native fixtures and later update

The caller must back real 32-bit source memory, allocator/runtime bank storage,
original definition/constants and temporary ABI regions. Original entry ESP is
explicit; it is never an object identity. `31DEE0` descriptor is entry minus
`20h`, output packet entry minus `2Ch`, and its agent/manager call entry is minus
`50h`. `31F550` listener packet is entry minus `14h`, cdecl world/entity listener
entry minus `30h`. Their virtual listener entry is a further minus `14h`;
`27E3A0`'s `1D4160` entry is another minus `2Ch`. The measured captures retain
these exact alias chains and caller words.

Fixture `C` records retain original entry, EAX, ECX, observed returned EAX,
entry ESP, x87/MXCSR and 16 raw stack words. `M/A` contain full original-derived
input/output spans. `W/WS` retain original site/address/before/written bytes;
only semantic temporary before-byte comparison is excluded where unrelated ABI
aliasing owns those old bytes. Other internal pushes/SEH plumbing remain
source-only. `B` records the actual next unavailable callee's instruction,
entry, receiver, entry ESP, cdecl flag and exact arguments. Native tests throw
at that owner without writes or supplied return, and compare the strict
original preceding state/store prefix. Four such cases are distinguished from
the twelve complete calls; they are not positive solver comparisons.

The 186 direct-call function listings retain 16,111 original instruction rows.
All 5,428 observed read/write sites are byte-audited; 62 external tail sites use
the separately pinned full prior original execution. Original/repeat reports,
all corresponding instruction files and both source-only exports repeat exactly.
Expected bytes come only from original reads, writes and retained buffers.

The final later report checks 28 complete bodies, branch topology, direct calls,
absolute operands and an actual initializer-vtable anchor. Key mappings:

| TU1.1 | Authenticated u5 | Owner |
|---|---|---|
| 317650 | 31B1A0 | outer step wrapper |
| 31E2C0 | 321E50 | island schedule |
| 31DB70 | 321700 | current broadphase update |
| 31DEE0 / 31E190 | 321A70 / 321D20 | retained agent schedulers |
| 31D5F0 / 31D690 | 321180 / 321220 | output bank lifetime |
| 300870 | 304350 | actual retained agent virtual |
| 2E68D0 | 2EA380 | actual current broadphase backend |
| 31F550 | 3230C0 | retained contact output delivery |
| 31E9D0 / 31C3C0 | 322560 / 31FF50 | world/entity listeners |
| 27E3A0 | 281E10 | physical object listener |
| 1D4160 / 1D2F10 | 1D8A20 / 1D77D0 | contact scan/refresh |

Two absolute operands contain relocated function pointers rather than equal raw
bytes: `469290→46E350` points to complete `2DED30→2E2800`, and
`4693AC→46E46C` to complete `2FFE20→3038C0`. Their complete bodies are explicitly
checked. The observed `27E760→2E4D60` one-instruction callback is anchored through
actual complete initializer `1C5FF0→1CB220`'s stored BSP listener vtable, rather
than choosing an arbitrary matching RET4. This is static correspondence, not
execution of the 1.5 image or automatic relocation of 1.1 runtime storage.

## Minimum remaining live bridge

The existing source body/factory/slot/world/broadphase/pair-manager providers can
retain the real canister and its agents. This module can be installed for their
output-bank lifetime and real emitted-row/contact-bank delivery. It cannot turn
an absent producer into live collision.

The next required owner is complete `31DB70→2E68D0`: current source shape bounds,
real unsorted body/handle storage, updated sweep endpoints, genuine added/removed
pairs and delivery to existing native `31D980`. The source capture executes
those real calls; the native scheduler stops before them. Next is actual retained
`300870`, its real shape/BSP child agents, contact-manager allocation/geometry and
solver delivery (`31F1B0` and its actual lower chain). The source already enters
these naturally, creates/refreshes/removes the transient canister/BSP contact,
and retains exact original packets. Their output must be genuinely produced in
native ownership, never replayed from these fixtures.

Complete title/device/frame startup, natural repeated clock ownership,
player-versus-canister positive contact, full continuous settling, damage,
destruction/explosion, all shape classes and Xbox reciprocal/FP hardware parity
remain unproved. The inherited unchanged 12-byte tick/rate/dt fixture and declared
compatibility/tag/resource/platform backing are retained. No new PVS or
activation masks, world admissions, contact results or hit inputs are added.
