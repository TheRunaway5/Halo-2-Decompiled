# Original death scheduling and corpse collection

This separate TU1.1 source gate establishes when ordinary stock SMG damage
releases player ownership, which death fields survive in the shared object
pool, and the bounded original corpse collection decisions. It does not prove
complete world startup, a marker pose, Havok death behavior, full world deletion
or live multiplayer parity. Earlier frozen suites and production are untouched.

## Authority and reproduction

- TU1.1 executable:
  `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
  `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- Original Lockout cache: `assets/disc/maps/lockout.map`, SHA-256
  `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
- New oracle: [oracle_mp_spawn_corpse_schedule.py](../tools/oracle_mp_spawn_corpse_schedule.py).
- Ignored evidence:
  `analysis/multiplayer/integration/spawn-source/corpse-schedule-report.json`
  and `tu11-corpse-schedule-instructions.txt`.
- The report pins all imported tools and the frozen datum, SMG producer,
  damage and lethal attribution reports. Executable instructions and cache
  metadata are mapped unchanged. No marker/head pose or instruction patch is
  supplied.

```sh
.tools/python/bin/python tools/oracle_mp_spawn_corpse_schedule.py \
  --output analysis/multiplayer/integration/spawn-source/corpse-schedule-report-repro.json
```

The suite contains **55 cases**: four damage/writer scheduling cases, four
garbage schedule cases, 24 dispatcher cases, three candidate/order cases, six
retention cases, nine pressure predicates, two complete final-object-update
caller cases and three player callback/storage release cases. The first case
includes all 25 original stock SMG hit calls; its fatal hit continues farther
than the previous frozen lethal suite.

## Actual lethal call order

The executed 25-hit sequence uses original player pools and host membership,
full projectile ownership, original first-impact event bytes and actual
Lockout damage/model metadata from the preceding frozen producer/attribution
suites. It runs `D9180` through `DA2B0`, original `D7E00`, result production,
`DAC40`, `1ED960`, `CB410`, `CBA60` and `CBDC0`.

In the fatal hit the source stages are, in this order:

1. `DA2B0` finds depleted body health and calls `D7E00`. That routine sets
   unit `+10A` bit 2 immediately. The player still owns that unit. `D7E00`
   completes before the damage accumulator receives its fatal flag.
2. The normalized result is produced. `DAC40` calls `1ED960` to read the
   previous four history entries and perform attribution/counters.
3. `1ED960` calls `15E4D0` before victim death statistics. The principal
   scheduling sequence preserves the earlier explicitly named writer adapter:
   it invokes original Slayer `2C2350` at this call site and omits the writer's
   marker/events/countdown effects. Separate cases below execute the real writer
   prefix instead.
4. `DAC40` invokes `CB410` with the same normalized result. `CB410` invokes
   `CBA60` to record the incoming hit and perform the incoming-owner counter
   tail, before its final death cleanup call.
5. `CB410` calls `CBDC0` synchronously when result `+4` bit 0 is set. It passes
   victim full object datum, signed short result `+40`, pointer to result `+18`,
   zero for the knocked flag, and result bit 10 for force. This is within the
   original damage call; it is not a later timer/queue-triggered cleanup.
6. `CBDC0` schedules garbage retention, saves player `+30`, releases `+2C`
   through original `14E240`, invokes `153A90`, then writes retained death
   fields. The new sequence ends at `CBF21`, immediately before reading the
   weapon-animation component through unit `+342`.

The unit is therefore dead and still owned during attribution and history
recording. `Unit.alive == false` alone is not an ownership transition. The
source order above is directly observed in the new trace and in original
[DA2B0](../analysis/map-pack-update/functions/000da2b0.c),
[DAC40](../analysis/map-pack-update/functions/000dac40.c),
[CB410](../analysis/map-pack-update/functions/000cb410.c) and
[CBDC0](../analysis/map-pack-update/functions/000cbdc0.c).

`139750` places the regular collector later in the same world update: early
players `14EAE0`, engine pre-world update `15D7C0`, object update `B7E00`,
world/physics phase `1C7660`, object phase `B7F50`, then final object phase
`B8040`. `B8040` clears garbage-global `+3` before calling `C0270`; the game tick
increments at `139868` after the later player/engine/world phases. This full
world caller is inspected, not executed as a complete game tick. Two bounded
cases execute complete `B8040 -> C0270` with explicit active-only header flags
and a named supplied `1019F0` movement/render queue leaf. Both an unexpired
corpse and a long-expired corpse remain allocated when normal pressure does not
request collection. See
[139750](../analysis/map-pack-update/functions/00139750.c) and
[B8040](../analysis/map-pack-update/functions/000b8040.c).

## Retained death fields and ownership

The complete executed prefix of `CBDC0` has these effects:

These bodies inherit the prior suite's explicit biped ragdoll-state byte
`+3DC = 3`, which takes original `E6050`'s no-transition branch. That byte is
not claimed as an output of an executed animation/death-state producer. Body
storage length `0x800` and active header registration are inherited bounded
fixtures, not complete generic world construction.

| Field | Source effect before `CBF21` |
| --- | --- |
| Unit `+10A` bit 2 | Already set by `D7E00`; body/header remain allocated |
| Unit `+134` bit 29 | Cleared while currently player-owned |
| Unit `+264` | Float zero while owned |
| Unit `+2BC` / `+2BE` | Byte/short zero while owned |
| Unit `+4` bit 14 | Garbage-list membership, set by `BC840` when admitted |
| Unit `+1C` | Full next object datum in the garbage linked list |
| Unit `+20` | Signed 32-bit garbage eligibility deadline |
| Player `+30` | Old full current-unit datum copied from `+2C` |
| Player `+2C` | `NONE` through complete original `14E240` |
| Unit `+13C` / `+140` | Current owner `NONE`, previous owner full player datum |
| Unit `+2E0` | Current game tick, written after ownership release |
| Unit `+134` bits 1, 3, 4 | Cleared by cleanup and complete `D22B0` |
| Unit `+2B8` | Float `0.25`, from original `D22B0(1.0)` |
| Unit `+148` | Zero |

The fatal hit occurs at tick 224 in the stock sequence. The retained unit has
`+2E0 = 224`, `+20 = 524`, previous full player `EC710003` and current owner
`FFFFFFFF`. Neither dead bit nor player detach frees its salted object header.
The direct cleanup fixtures at tick 200 similarly produce deadline 500.

Actor and other owner branches are source-defined but unexecuted here: unit
`+130` invokes `A89A0`; actor `+12C` copies actor `+30` into unit `+25A`, invokes
`1E6480`, then clears `+12C`. These fixtures have source-initialized `NONE`
references. The tail after `CBF21` remains outside execution. It reads weapon
animation component `+342`, stops weapons, clears `+134` bit 21, handles parent
`+14` and seat `+1FC`, releases/drop-handles weapon fields, processes carried
grenades, issues actions 13 and 9 according to `+1F5`/force, and interrupts
actions 1A/1B/16 through component `+346`. Those real animation/control/weapon
states and Havok outcomes are not invented. See original
[CBDC0](../analysis/map-pack-update/functions/000cbdc0.c),
[D22B0](../analysis/map-pack-update/functions/000d22b0.c),
[D0140](../analysis/map-pack-update/functions/000d0140.c) and
[CFF90](../analysis/map-pack-update/functions/000cff90.c).

## Garbage admission and deadline extension

`BC840` takes the full object in EAX and two stack arguments: enable byte and
signed tick interval. Garbage-global pointer is original `4E58C4`; its `+4`
is a signed 16-bit **active** garbage count and `+8` a full head datum.

On enable, a body whose `+4` bits 14 and 15 are both clear is pushed at the
head: body `+1C = oldHead`, global head = full object, bit 14 is set. It increments
the active count only if header `+2` bit 0 is set. Existing bit 14 prevents
duplicate insertion; bit 15 also prevents insertion. Regardless of insertion,
it computes wrapping 32-bit `gameTick + interval`, compares that value and the
old body `+20` as **signed** integers, and stores their signed maximum. Thus
another shorter request does not shorten the deadline.

On disable, only bit-14 bodies are unlinked by full datum from the list. It
clears body `+1C`, clears bit 14 and decrements the active count only if header
active bit 0 is set. It does not clear the deadline. The four executed flag
cases cover clear flags, already queued, bit-15-only and both bits; repeated
enable followed by disable checks list/count effects. Original `CBDC0` supplies
`signedShort(tickRate) * 10` for its interval, before player detach. See
[BC840](../analysis/map-pack-update/functions/000bc840.c).

Object activation/deactivation also maintains that active count for queued
bodies. `B8180` increments it when enabling a eligible inactive header; `B81F0`
decrements it when disabling an active header. Count is not total corpse count.
These functions are inspected here, not claimed as complete activation cases.
See [B8180](../analysis/map-pack-update/functions/000b8180.c) and
[B81F0](../analysis/map-pack-update/functions/000b81f0.c).

## Dispatcher and candidate removal predicates

`C0270` chooses its collection level from current state:

| State | Level |
| --- | --- |
| Garbage-global byte `+1` nonzero, byte `+2` nonzero | 0 |
| Byte `+1` nonzero, byte `+2` zero | 1 |
| No request; loop-heap remaining `<= 0xCCCC` or `0x800 - pool.count <= 0x66` | 3 |
| No preceding condition; signed active garbage count `>= 50` | 2 |
| Otherwise | No collection |

Request producers are not recovered in this suite; request bytes and loop-heap
pressure metadata are explicit varied inputs. The original dispatcher is fully
executed on retained/no-collection paths. The inherited supplied body arena is
not falsely presented as a complete initialized original loop heap.

Before each table step `C00B0` checks pressure again:

- Levels 0 and 1: true, critical byte false.
- Level 2: true only if signed active garbage count **> 30**; critical false.
- Level 3: true if remaining heap `< 0x19999` or `pool.capacity - pool.highWater
  < 0xCC`; critical if remaining `< 0x6666` or free high-water headers `< 0x33`.

Nine complete predicate cases exercise equal and one-below boundaries, counts
30/31 and negative signed count. Notice the outer level-3 choice uses **allocated
count**, whereas this inner gate uses **high water**. Source table at `446BB0`
first runs heap compaction for level 3, then garbage enumeration/consumption for
all levels, then another level-3 compaction and critical-only world collection
callbacks. Original heap compaction runs with no tracked loop-heap blocks in
this bounded supplied-arena fixture. Critical world collection is not established
by these cases. See [C0270](../analysis/map-pack-update/functions/000c0270.c),
[C00B0](../analysis/map-pack-update/functions/000c00b0.c) and raw table/listing.

`C0130` enumerates queued full object datums head to tail, up to 2048 candidates.
It omits a body with parent `+14 != NONE`; in mode 4 it also omits a body with
attached entity `+D4 != NONE`. It does not inspect deadlines. `C01B0` pops **one
candidate from the end** of this array per call. Three source cleanup prefixes
prove head order newest-to-oldest, consumption oldest-to-newest; supplied
retention true makes those three consumer calls return without deletion.

`C01B0` uses the following actual removal decision, in this order:

1. Initial admissibility is signed `gameTick >= body.deadline(+20)`.
2. Level 2 additionally requires header active bit 0; other levels do not.
3. Levels 1, 2 and 3 call original `BC970`; a true nearby/view retention result
   skips deletion. Level 0 bypasses `BC970`.
4. If admissible and not retained, invoke `BC840(false, NONE)` **before**
   `B92A0(fullObject, true)` and `B9350(fullObject, true)`.

Even level 0 does not bypass the deadline. The level/deadline/active/retention
matrix stops deletion cases at `B92A0` entry: the original list unlink has
already happened but world deletion and storage release have not. Nearby true
and false matrix cases explicitly supply `BC970`'s boolean; they do not prove
physical proximity or view direction. Source listing contains complete original
`C0130` and `C01B0`; these routines were not separately emitted as Ghidra files.

## Actual nearby/view retention boundary

Separate cases execute original `BC970` rather than a supplied return value.
It resolves the object's root parent chain, then requires root header active
bit 0, root body `+4` bit 8 set and bit 18 clear. Original `B9970/B9A10` must
find a world partition relation whose partition bit is active in options
`+1138`. Inactive header, missing flag 8, flag 18 and no partition return false
in the complete executed original routine without a pose.

With an explicit minimal partition relation, `BC970` walks source live players
and calls `B9C20(currentUnit, 04000095, 1)`. The case stops at this marker call,
without supplying a pose. Another case detaches every player through original
`14E240` and proves the complete routine returns false without a marker call.
The partition relation is a supplied world-state boundary, not a demonstrated
world-registration producer. Original Lockout model metadata is consulted by
the routine; using its radius does not prove marker/view retention. Its remaining
position, view and scale calculations must be supplied by source-derived live
world state before that predicate can be integrated. See
[BC970](../analysis/map-pack-update/functions/000bc970.c),
[B9970](../analysis/map-pack-update/functions/000b9970.c) and
[B9A10](../analysis/map-pack-update/functions/000b9a10.c).

## Player object-deleted callback and storage release

Actual `B92A0` first calls `B9430`, then its original global callback table
at `46FB04`: `BC770`, `C2520`, `1CD920`, **`154440`**, `16800D`, `15FE00`,
`20CFA0`, `17CE30`, followed by children handling. `B9350` later handles world,
parent and child removal, invokes per-type deletion callbacks through `10CD80`,
and only then calls `BD1F0`. This complete world deletion chain is inspected;
it is not replaced with an invented immediate free. See
[B92A0](../analysis/map-pack-update/functions/000b92a0.c),
[B9350](../analysis/map-pack-update/functions/000b9350.c),
[10CD80](../analysis/map-pack-update/functions/0010cd80.c) and raw callback table.

Three separate cases execute complete original `154440` and complete `BD1F0`
as explicitly separate kernels. `154440` iterates actual allocated players
for unit object type mask 3. A matching full current `+2C` is saved to `+30`,
detached through `14E240(NONE)` and followed by `153A90`. Matching full player
`+174` is cleared; matching full previous `+30` is cleared. Its final `189490`
nonplayer control notification is an explicit named supplied service.

Cases cover previous-only corpse, new current unit after respawn, and an
explicitly rebound still-current dead unit. A new current unit survives deleting
the old full corpse; old matching previous/watched fields clear. The isolated
source unlink/storage release clears the body pointer/header allocation, and
the next shared-pool allocation reuses the old index with a new source salt.
This proves callback/storage identity effects, not complete `B9350`, ragdoll,
weapons, parent detach or physics removal.

## Real death writer prefix before marker

Three source-generated fatal-hit cases execute original `15E4D0` through its
marker boundary: zero respawn seconds, five seconds, and direct self damage
with five-second suicide penalty. The self case's preceding 24 hits also use
self ownership, so original attribution supplies victim as the writer's attacker;
there is no substitution of incoming self for selected credited player.

Before `B9C20`, the writer stores death-relative tick `+1B4`, applies the source
suicide/relationship penalty, produces ordinary signed countdown `+164`, clears
penalty `+168` and sets engine history entry `+558 + slot*0x18` active byte 1.
Observed countdowns are 30, 150 and 150 ticks. The unit is already dead, but
player `+2C` and unit `+13C` remain bound; garbage insertion has not occurred.
Its marker is the exact current full unit and `04000095`, capacity 1.

The stop is before `15E7DC`'s original call executes. No marker output, position,
history short timers or later event/profile/Slayer effects are fabricated. The
inspected instructions after its return copy marker position into history `+4`,
`+8`, `+C`, then copy countdown low short into history `+10` and `+12` at
`15E834/15E83F`, before event handling and final Slayer callback. The current-unit
`NONE` branch instead uses original constants `(0, 0, 500)` and clears active;
that is an inspected branch, not the observed ordinary lethal call order. See
[15E4D0](../analysis/map-pack-update/functions/0015e4d0.c) and raw prefix listing.

## Native integration boundary

A shared object pool can retain original full unit, weapon and projectile
identity without asserting a complete original startup allocation sequence.
Before live corpse release, it must preserve at least salted header/type/body,
dead flag, full owner and previous owner, player current/previous/watched unit,
history records, garbage list/count/deadline, parent/entity and partition
references, and incoming-unit counters `+2EA/+2EC`. The death cleanup must
preserve the synchronous stage order above,
including original player binding/status callbacks and `+2E0` after detach.

The source does **not** justify freeing at death, freeing on respawn/leave,
or freeing every body after ten seconds. Full world deletion needs the real
global/per-type callbacks, corpse animation/control state, weapons/grenades,
partition/marker/view retention and world/physics lifetime. Until those producers
are available, retaining bodies with explicit limits is the supported boundary.

## Freeze hashes

| Artifact | SHA-256 |
| --- | --- |
| Oracle | `4ab8290691690a7bf92d3e8752521a249bb241e34515bbbc985f0a7628e7a68e` |
| Frozen report | `649a0761e442ff5698476a18d4fcef36ebe89d4431ecb8619de497a46e357f6d` |
| Instruction listing | `ea28b160b7f5bb3bb8d7508684fe279fa9042792852f9df0740d08060aee3626` |

Independent reproduction is byte-identical. Python compilation and source/doc
whitespace checks passed. Imported tools and all prior reports pinned by this
suite remain byte-identical.
