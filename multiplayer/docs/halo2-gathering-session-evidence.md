# Original Halo 2 searching and gathering session lifecycle

Research date: 2026-10-12. Scope: the retail executable's role chooser, gathering
session owner, advertisements, clocks and query scheduling. This supplements the
[candidate scorer](halo2-candidate-scoring-evidence.md),
[bandwidth inputs](halo2-bandwidth-input-evidence.md) and
[party averages](halo2-party-average-evidence.md). No product code, build,
deployment or earlier evidence note was changed.

The original scorer compares **one searching party against actual partially
populated gathering sessions**. The client chooses one role at a time. The
gathering role creates a separate game session, reserves the original party and
advertises its remaining capacity before a complete match exists. Its advertised
age starts at that game session's host creation. A central service can represent
these objects and reuse the recovered arithmetic; scoring arbitrary completed
groups, calling ticket age session age, or making every party simultaneously a
searcher and gatherer does not reproduce this lifecycle.

## Authority and reproducible controls

Executable: `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The runner uses `tools/oracle_game_variants.py` to map its original sections
unchanged. Fixtures supply configuration, party records, clocks and a stack.
Original instructions and callees are never replaced. Complete functions are
executed when their real callees are available. Explicitly labelled block or
entry controls stop before a platform/service call or begin after its return;
they do not manufacture a Live response or claim a complete platform flow.

```sh
.tools/python/bin/python .local/gathering-session-research-20261012/oracle_gathering_controls.py
```

There are **48 passing controls**, covering **2,527 unique original
instructions**, with every recorded instruction compared against the pinned
image. Artifacts under
[`.local/gathering-session-research-20261012`](../../.local/gathering-session-research-20261012):

| Artifact | Purpose |
| --- | --- |
| `oracle_gathering_controls.py`, `oracle-gathering.json` | Independent original-byte runner, all inputs and expected/actual outputs |
| `retail-timing-and-hopper-defaults.json` | Original timing defaults, all seven built-in hopper capacity/countdown tables |
| `original-function-spans.json`, `original-*.asm` | Inventory sizes, source status, byte hashes and original disassembly |
| `executed-original-instructions.txt` | Verified executed instruction addresses/bytes |
| `direct-host-birth-accesses.json` | Static direct `+0x78B4` accesses across the current function inventory |

Runner SHA-256:
`ad94f29e7afe6ea598c81f88fa3dabb35c0f4bbed7c92a94deed93b8985ed90b`.
Executed-instruction SHA-256:
`7043fd304d008d5dfcda02c289a01542a7aab78ae334143c180900d644c9b78b`.
Synthetic controls establish these client contracts, not the missing Live
service, original Internet conditions or historical downloaded overrides.

## The original party and the gathering game are different sessions

The owner has party session **A at `+0x30`** and game session **B at `+0x38`**.
`0x706F0` dispatches using A's mode (`session+0x4980`), separately from its
network state (`+0x741C`). In this note, modes6–13 mean the former. Network
states5–8 identify the hosting states checked by these routines.
[Owner layout](../../tools/reference/halo2-decompiled/include/unknown_058dd0.h#L11),
[dispatcher](../../tools/reference/halo2-decompiled/src/unknown_0706f0.cpp#L24),
`original-000706f0.asm`.

| Party A mode | Owner/function | Meaning established by its actions |
| --- | --- | --- |
| 6 | `0x70D60` | Choose search or gather after preparation/readiness |
| 7 | `0x70EA0` | Search for gathering sessions; select and request joining B |
| 8 | `0x71210` | Wait for B to become live; failed join marks candidate seen and resumes7 |
| 9 | `0x712D0` | Wait for all party players in B; then13; timeout/failure resumes7 |
| 10 | `0x71480` | Create a new hosted game B with the whole original party reserved |
| 11 | `0x71730` | Gather players in B, check countdown/readiness/teams, then12 or back to6 |
| 12 | `0x71D70` | Continue launch transition; hosting B switches B to mode2 |
| 13 | `0x71390` | Joined-party continuation; failed/disbanding B resumes7 |

[Search/join](../../tools/reference/halo2-decompiled/src/unknown_070ea0.cpp#L59),
[initial join](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1333),
[join/launch continuations](../../tools/reference/halo2-decompiled/src/unknown_071e00.cpp#L94),
[creation](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1505),
[gathering](../../tools/reference/halo2-decompiled/src/unknown_071730.cpp#L69).

Before choosing or creating a new role, `0x70B70` drains a previous B. A hosted B
is put into mode14 and leaves when its member count is at most1, or after its
mode14 clock reaches configuration `+0x1A8` (default10,000ms). Nonhosting B leaves
immediately. The dispatcher cleans an active search when A leaves modes7/8/9/13.
This does not support a party advertising a new gathering B while independently
retaining the previous searching role.
[Drain](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1462),
[search cleanup](../../tools/reference/halo2-decompiled/src/unknown_0706f0.cpp#L111),
`original-00070b70.asm`.

## Role choice and its real clocks

`0x70A20` clears the last search and gathering start ticks on entry.
`0x70D60` requires prepared player ranks (`state+0x954`); if still unavailable
after5,000ms it sets error/status18. With readiness it chooses searching when:

1. The previous search start `state+0xA7C` is zero; therefore first role is search.
2. Otherwise, elapsed time since that search start is **at least** elapsed time
   since the previous gather start `state+0xA68`.
3. Otherwise, derived host capacity `state+0x938` is NONE (`-1`).
4. Otherwise, an optional restricted-NAT policy asks for another search: flag
   `config+0x1AC`, NAT kind3, and shared counter below `config+0x1B0`. The retail
   defaults are flagfalse and limit5; this branch is inactive by default.

If none applies, it chooses gathering. The C++ local named `host` is misleading:
true calls **`0x70C00` → searching mode7**; false calls **`0x70C90` → mode10**.
Independent controls observe these actual entry addresses, including first,
older/newer/equal starts and unavailable capacity.
[Entry reset](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L473),
[chooser](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1660),
`original-00070d60.asm`, `oracle-gathering.json`.

Search entry records both wall seconds (`state+0x968`) and tick milliseconds
(`+0xA7C`). Gathering entry records its tick start (`+0xA68`), resets peak player
count (`+0xA6C`) to0, and starts the growth clock (`+0xA70`). These role-start
fields remain distinct from the advertised game birth and per-query clock.
[Role-entry owners](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L980),
`original-00070c00.asm`, `original-00070c90.asm`.

The search role returns to mode6 when queries exhaust, or when
`now_tick - state.A7C > config.198 + state.96C`. Default base timeout is240,000ms.
Thus four minutes is an **upper bound**, not a mandatory delay before gathering:
completed empty query cycles can exhaust earlier, after which the chooser can
select gathering. Query allocation failure/status errors instead follow the
search error path; they are not a guaranteed host fallback.
[Search completion/timeout](../../tools/reference/halo2-decompiled/src/unknown_070ea0.cpp#L117),
[query exhaustion](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L753).

### The launch-default timing cast is signed

`0x70190` draws `state+0x96C` and `+0x974` once during initialization. It reads
configuration `+0x19C` and `+0x194`, but **sign-extends their low16 bits before
multiplication**, then sign-extends the resulting16-bit value. Both defaults are
60,000; signed16(60000) is **−5,536**. Consequently these original defaults
produce a negative offset in the range−5,536…0ms, not a positive0…60-second
jitter. Seed0 produces search−1,307ms/gather−1,543ms; seed1234 produces
−3,955ms/−1,146ms. These are complete unchanged `0x70190` controls, not a
translation inferred from the C++ type names. A policy intending different
randomization must say it is custom. Later downloadable values remain unknown.
[Initializer](../../tools/reference/halo2-decompiled/src/network_session_client.cpp#L317),
[defaults](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L584),
`original-00070190.asm` at `0x70200/0x70209/0x7022A/0x70236`,
`oracle-gathering.json`.

## Gathering allocation, capacity and the advertised age

`0x71480` first chooses a map and variant, then calls actual
`network_session_host(B, mode2, …)` with the complete original party identities,
raw ranks and original party session ID. It reserves those players with timeout
NONE, copies the selected hopper, language, summary and quality-derived capacity
into B, publishes B's join ID/key/address to party A, and changes A to mode11.
It does not wait until a completed match has been assembled to create B.
[Creation](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1505),
[host/reservations](../../tools/reference/halo2-decompiled/src/unknown_059ad0.cpp#L3211),
`original-00071480.asm`, `original-00059bd0.asm`.

Original `0x72330` derives capacity from `0x7E100`'s quality level and hopper
geometry. For place hoppers it bounds the level by allowed player counts. For
team/clan hoppers it divides by required team count, checks minimum size, clamps
maximum size and multiplies back. Unknown bandwidth `(0,-1)` has level floor8;
it is not automatically capacity NONE. Complete original preparation controls
produce these capacities with the seven built-in launch hoppers:

| Built-in index/name | Capacity for fresh `(0,-1)` | Gather timeout base |
| --- | --- | --- |
| 0 Rumble Pit | 8 | 150,000ms |
| 1 Team Slayer | 8 | 150,000ms |
| 2 Head to Head | 2 | 120,000ms |
| 3 Team Skirmish | NONE | 300,000ms |
| 4 Minor Clan Match | 8 | 120,000ms |
| 5 Major Clan Match | NONE | 120,000ms |
| 6 Team Training | 8 | 150,000ms |

The two NONE cases cannot gather through this chooser with those fresh inputs.
This is not permission to fabricate measured quality or use simultaneous roles
to bypass the gate. A central allocation rule that removes the transport-capacity
restriction must be declared as a replacement policy.
[Preparation](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1204),
[quality selection](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp#L725),
[built-ins](../../tools/reference/halo2-decompiled/src/unknown_1936e0.cpp#L245),
`retail-timing-and-hopper-defaults.json`, `oracle-gathering.json`.

`0x59BD0` sets **B+0x78B4 = `time(NULL)`** at hosting birth. `0x73530` reads that
field and advertises **`time(NULL) - birth` in seconds**. This is neither the
party's queue age, the last role-start tick, mode age, nor first-advertisement
time. Complete `0x5A220` controls show that mode changes update B+0x4984 but
preserve +0x78B4; repeating the same mode does not reset its mode clock either.
An explicit original subtraction block preserves positive, zero and negative
wall-clock differences. The scorer's separate clamp handles negative ages.
[Host birth](../../tools/reference/halo2-decompiled/src/unknown_059ad0.cpp#L3275),
[mode clock](../../tools/reference/halo2-decompiled/src/unknown_059ad0.cpp#L586),
[description age](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1207).

A static direct-displacement scan across the current original function inventory
finds only the host write at0x59E05 and description read at0x73634. This narrows
the owner but does not prove that arbitrary pointer copies or untraced migration
paths cannot affect the record. The supported replacement contract is a stable
birth for a specific hosted gathering-session lifetime, reset on a new creation.
`direct-host-birth-accesses.json`.

## Advertisements describe available gathering capacity

`c_helper_b::v7` (`0x73360`) admits advertisements only when connected, owner
type6, party A is hosting in **mode11**, game B is hosting, its hopper exists,
and B's capacity does not exceed the hopper maximum. Complete controls reject
search mode7, setup10, launch12, disconnected/nonmatchmaking owner, absent B and
overcapacity B. Creating a game alone is insufficient to advertise it.
[Admission](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L425),
`original-00073360.asm`, `oracle-gathering.json`.

`0x73530 → 0x733F0` supplies Live public-filled from the aggregated summary player
count and public-open from **B.capacity4994 minus that summary count**; private
counts are zero in this producer. It also emits the selected hopper, aggregate
raw average, available space, mapped minimum, maximum, session age and add-party
mask. Detailed QoS payloads additionally carry quality, language, min/raw ranks,
group mask and clan IDs. The aggregation/admission masks have already been
recovered separately; a replacement must refresh them as accepted parties join
or leave, rather than retaining the founding party's values.
[Description/property producers](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1173),
[Live slot layout/create](../../tools/reference/halo2-decompiled/src/online_matchmaking.cpp#L30),
[attribute keys](../../tools/reference/halo2-decompiled/src/unknown_0b49a0.cpp#L146).

Advertisement creation retries are throttled by default10,000ms (`+0xC90`).
Updates occur immediately for changes to add-party mask, mapped minimum or
maximum; other changed fields wait default15,000ms (`+0xC94`). QoS description
refresh defaults to100ms (`+0xC98`) and listen budget32,768bits/s (`+0xC9C`).
Leaving eligibility unregisters/stops listening, then deletes the Live
advertisement, with deletion attempts throttled to3,000ms. These are different
tasks from searching; they are not a universal central polling period.
[Create](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L254),
[update](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L686),
[lifecycle](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1123),
[delete](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L711),
[QoS publication](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L951).

The Live advertisement ID/key in helper info identify the searchable/probed
record; its encoded description supplies B's actual join ID/key/address. The
client publishes the advertisement ID separately to A. A replacement can choose
its own ID scheme but must retain a stable mapping from the selected candidate
to the gathering game and must not conflate an old advertisement with a new B.
[Advertisement info](../../tools/reference/halo2-decompiled/src/online_matchmaking.cpp#L170),
[publication](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L324),
[description/join](../../tools/reference/halo2-decompiled/src/unknown_070ea0.cpp#L87).

## Query cadence and radius widening are response driven

The undecompiled original **`0x90840`** is the update owner. Its original bytes
call, in order:

```text
0x902B0: poll/consume the Live query task
0x905A0(kind0), then (kind1): poll/consume full and brief QoS work
0x91000: score and sort the candidate bank
0x90420(kind0), then (kind1): schedule needed QoS work
0x90F70: decide whether to widen/requery or finish
```

`src/stubs/lane_y.cpp` contains an empty reconstructed stub for this address;
that stub is not evidence of the retail behavior. `original-00090840.asm` is the
authority. `0x902B0` preserves returned order while copying up to50 brief
results, marks seen advertisements ineligible, and clears the new result's full
and brief readiness. Original `0x90420` can refresh full QoS after20,000ms
(`config+0x35C`); full-probe admission requires brief readiness. Already active
handles prevent another probe batch of the same kind.
`original-000902b0.asm`, `original-00090420.asm`,
[QoS scheduling](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L702).

`0x90B30` computes rank bounds and minimum score for the current radius, clears
the candidate/previous banks, and submits `0x8FA80`. Its **per-query clock starts
when submission returns a valid task index**, not at completed response time.
Failure to allocate/submit sets search state4. Two explicitly labelled original
post-call block controls verify successful task7 records the current tick while
NONE preserves the old clock and sets state4. The scorer's earlier5,000ms
selection wait uses this per-query clock.
[Query](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L196),
`original-00090b30.asm` at0x90C4B–0x90C73, `oracle-gathering.json`.

`0x90F70` can advance only when search state is nonzero, query task is NONE,
**both QoS handles are NONE**, and there is no selectable candidate. It adds
the current result count to its cumulative count. Below default150 results:

- Radius below maximum increments by1 and submits another query.
- At maximum, a casual/unranked search changes radius to NONE and submits an
  unbounded query; ranked/classified searches do not take this branch.
- With no radius change, exactly50 returned results permit another same-radius
  query; a shorter result set ends searching.
- At cumulative150 or more it ends, regardless of radius.

There is **no elapsed-time interval in this widening owner**. Inflight query,
kind0 QoS and kind1 QoS controls each preserve radius. Other controls verify
empty-cycle advance, ranked maximum exhaustion, casual unbounded pass, repeated
full page, partial unbounded exhaustion and149/150 boundary. They stop before
the real Live query entry if another query would be submitted.
[Widening](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L753),
`original-00090f70.asm`, `oracle-gathering.json`.

The query builder sends12 attributes: hopper, party average, optional minimum/
maximum and eight party-size categories. `XOnlineMatchSearch` uses procedure1
with maximum50 results. Its stored procedure, server ordering, pagination,
deduplication and timing are missing. Repeating a full50-result query is an
observed client action; it does not establish how the service avoids or allows
duplicates. A modern timed radius policy, reciprocal eligibility policy or
deterministic central result ordering is a declared replacement where these
original service contracts cannot be recovered.
[Query attributes](../../tools/reference/halo2-decompiled/src/unknown_0b49a0.cpp#L93),
[Live query boundary](../../tools/reference/halo2-decompiled/src/online_matchmaking.cpp#L109).

## Gather countdown and operational integration boundary

During gathering `0x71730` tracks actual B player count. Only a **new historical
peak** resets `state+0xA70`; departure, unchanged count or rejoin up to the old
peak does not. The executable block controls verify all three cases. Countdown
uses `0x1931D0(hopper, player_count)` against elapsed since that growth clock.
Invalid player counts return INT_MAX; maximum count returns0; other allowed
counts use the authored16-entry millisecond table. The complete default tables
are preserved in the artifact, rather than replaced by one guessed delay.
[Peak/countdown](../../tools/reference/halo2-decompiled/src/unknown_071730.cpp#L102),
[countdown getter](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp#L256),
`retail-timing-and-hopper-defaults.json`, `oracle-gathering.json`.

Gather timeout is **`hopper+0x59C + state+0x974`** compared with elapsed since
that same growth clock, using `<=`. The total gather-start clock `+0xA68` also
exists but serves separate work. Match launch additionally waits for the party
players/summary to agree, no conflicting group mask, all players present,
member readiness, and valid team balancing where applicable. A timeout can
return A to role-choice mode6 instead of launching an undersized/invalid match.
[Gather checks/end](../../tools/reference/halo2-decompiled/src/unknown_071730.cpp#L119),
`original-00071730.asm`.

A supported native/service bridge therefore needs a persistent gathering object
with founding party, actual accepted parties, stable birth, capacity, current
raw-rank/space/admission summary, join target and route-probe readiness. Search
state needs its own role start, query start, radius, bank, handles and cumulative
result count. Selection must reserve/join the chosen gathering object as one
party before updating its summary. Teardown must withdraw the advertisement
and drain the old object before creating a new gathering lifetime. These are
operational contracts recovered above, not a claim that a modern relay is XNet.

Still unresolved: the missing Live procedure1/search ordering and duplication
policy, historical downloaded timing/hopper overrides, equivalent NAT/member
reachability and direct-path QoS, full platform reservation/acknowledgment and
host-migration behavior, and whether a new central service should preserve
original host-capacity restrictions. Server-originated relay measurements can
be measured replacement inputs, but their transport semantics must be named.
The evidence supports implementing these explicit boundaries; it does not
support disguising an arbitrary central grouping policy as original Halo2.
