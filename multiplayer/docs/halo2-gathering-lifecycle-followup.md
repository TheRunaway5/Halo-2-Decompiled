# Original Halo 2 lifecycle boundaries: countdowns, failures and seen IDs

Research date: 2026-10-12. Scope: the seven built-in hopper limits/countdowns,
restricted-NAT role counter, signed tick comparisons, search-task errors and
completed QoS failures. This supplements the
[gathering lifecycle note](halo2-gathering-session-evidence.md). No product,
build, deployment or earlier research file was changed.

The original searching lifecycle has separate task, search and matchmaking
states. Failed QoS candidates become ineligible and seen; a failed task does
not directly become search state4. The original seen-ID capacity is **150**,
which contradicts the reconstructed source's25 for this owner.

## Authority, controls and fixture

Executable: `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

```sh
.tools/python/bin/python .local/gathering-lifecycle-research-20261012/oracle_lifecycle_controls.py
```

**423 passing controls**, **3,439 unique original instruction addresses**.
Every recorded instruction is compared against the pinned executable. Original
instructions and callees are never replaced. Synthetic records and the game's
own clock override supply inputs. Explicit original blocks are labelled where
platform calls, allocator execution or later launch checks are outside scope.
These controls establish client contracts, not original Live responses, Xbox
network behavior or later downloaded configuration.

Artifacts under
[`.local/gathering-lifecycle-research-20261012`](../../.local/gathering-lifecycle-research-20261012):

| Artifact | Purpose |
| --- | --- |
| `oracle_lifecycle_controls.py`, `oracle-lifecycle.json` | Independent runner and every expected/actual control |
| `retail-lifecycle-fixture.json` | Schema1, pinned executable, seven hopper tables, comparison boundaries, NAT block inputs, task/search states, QoS outcomes and seen-bank contract |
| `original-function-spans.json`, `original-*.asm` | Original function sizes, byte hashes, reconstructed-source status and disassembly |
| `executed-original-instructions.txt` | Verified executed instruction addresses and bytes |

Runner SHA-256:
`2654ebcb57098ee427e21bca5234b21ada88e18d01ab087c486e8e50749e5187`.
**Full fixture SHA-256**:
`2d7abcf721107cb15401e098d7d394c40c58d67e45e55a657919fc2b39af9bcf`.
Instruction-trace SHA-256:
`4f43b410a41cb8a8b655d8a7333966c52438e6d05204ccf38213467ee86b9e38`.

## All seven built-in hopper countdowns

Complete original `0x193C70` constructs each hopper. Complete `0x1932C0` and
`0x193300` return its minimum/maximum **total player counts**. Complete
`0x1931D0` returns `INT_MAX` outside those bounds,0 at maximum, otherwise
`hopper.points[player_count-1]`. All counts0 through17 were independently
executed for each built-in. The timeout is the constructor-produced field
`hopper+0x59C`, read directly by the gathering owner; the three listed functions
are not a separate timeout getter.
[Constructors](../../tools/reference/halo2-decompiled/src/unknown_1936e0.cpp#L245),
[geometry](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp#L61),
[countdown](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp#L256),
`original-00193c70.asm`, `original-001931d0.asm`.

| Index / built-in | Minimum…maximum | Gather timeout base | Countdown milliseconds by eligible total player count |
| --- | --- | --- | --- |
| 0 Rumble Pit | 4…8 | 150,000 | 4:30,000; 5:22,500; 6:15,000; 7:7,500; 8:0 |
| 1 Team Slayer | 6…8 | 150,000 | 6/7:30,000; 8:0 |
| 2 Head to Head | 2…2 | 120,000 | 2:0 |
| 3 Team Skirmish | 10…16 | 300,000 | 10/11:45,000; 12/13:30,000; 14/15:15,000; 16:0 |
| 4 Minor Clan Match | 6…8 | 120,000 | 6/7/8:0 |
| 5 Major Clan Match | 12…16 | 120,000 | 12…16:0 |
| 6 Team Training | 4…8 | 150,000 | 4/5:30,000; 6/7:15,000; 8:0 |

These are executable launch defaults, not proof of the later public playlist
catalog or downloaded values. Countdown0 means the countdown gate can pass;
separate member, readiness and team checks still control launch.
`retail-lifecycle-fixture.json` fields `hoppers` and `countdown_gate`.

Original `0x719E9..0x71A05` calls the real countdown getter, rejects `INT_MAX`
explicitly, and passes the countdown gate when **elapsed growth time >=
countdown**. A below-minimum count does not pass even if elapsed equals
`INT_MAX`. Original `0x71C7A..0x71C95` expires gathering when **elapsed growth
time >= hopper.timeout + state.gather_jitter**. All seven boundaries have
controls one millisecond below, at and above the threshold. The previously
recovered growth clock resets only on a new historical player-count peak.
`original-00071730.asm`, `oracle-lifecycle.json`,
[growth owner](../../tools/reference/halo2-decompiled/src/unknown_071730.cpp#L69),
[jitter evidence](halo2-gathering-session-evidence.md#the-launch-default-timing-cast-is-signed).

## Restricted-NAT counter uses the old shared value

With the optional policy enabled and the actual NAT getter returning3, the
original chooser compares **the old value** of shared global `0x55E6FC` against
`config+0x1B0` after incrementing the stored value. With default limit5, eligible
calls produce five searches (old values0…4), then gathering (old value5) and
reset to0. It is shared across owners, not a per-party counter. NAT values
other than3 do not increment or reset it. The earlier first-search, role-age
and capacity-NONE gates bypass the NAT branch; policy-disabled gathering also
bypasses it. The policy flag is false in the launch defaults.
[Chooser](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1660),
`original-00070d60.asm` at0x70E33–0x70E5A, `oracle-lifecycle.json`.

Controls execute that exact original block with the NAT return value as an
explicit input and stop at the real search/gather entry. An eight-call sequence
alternating two synthetic owners proves the shared counter behavior. The
complete original offline NAT getter returns0. The connected classifier calls
Xbox `XOnlineGetNatType`; that platform call is not emulated or stubbed. The
controls therefore do not classify a modern relay/NAT or establish equivalence
between a replacement NAT enum and Xbox kind3.
[NAT getter](../../tools/reference/halo2-decompiled/src/online_tasks.cpp#L686),
`original-0006b550.asm`, fixture `nat_return_block`.

## Tick arithmetic and comparisons retain signed32-bit behavior

Original `0x75890` subtracts `now-start` modulo2^32 and exposes the result as a
signed32-bit long. The gathering growth calculation uses the same literal
subtraction. Neither extends timestamps to64 bits nor special-cases zero.
Controls include `now=0x100,start=0xFFFFFF00 → 512`,
`0x80000010-0x7FFFFFF0 → 32`, and `0x80000000-0 → INT_MIN`.
[Elapsed helper](../../tools/reference/halo2-decompiled/src/unknown_075870.cpp#L33),
`original-00075890.asm`, `original-00071730.asm` at0x7188A–0x718A8.

The chooser compares these signed elapsed values with `>=`; normal short
intervals across tick wrap preserve the expected older role. Intervals at or
beyond half the32-bit cycle can become negative. Full original `0x70EA0`
controls confirm search timeout uses **strict `>`**, including a wrap crossing:
240,000ms at zero jitter stays searching;240,001ms returns party mode6.
The negative half-cycle result does not exceed the positive timeout. Gathering
timeout and countdown use inclusive `>=`, as above. A replacement that uses
unbounded elapsed integers must explicitly decide whether it preserves this
retail wrap behavior.
[Chooser](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1680),
[search timeout](../../tools/reference/halo2-decompiled/src/unknown_070ea0.cpp#L136),
`oracle-lifecycle.json`, fixture `tick_subtraction`, `gather_timeout_gate`.

## Task status4 is distinct from search state4

Complete original `0x6B5D0` and `0x902B0` run against a genuine engine task
pool. Task handles areNULL so cleanup uses the original pool-release routine
without calling Xbox task close. The finished-success control has logonNONE,
so the original results parser returns zero results through its own gate; it
does not supply a simulated successful Live response.
[Task poll](../../tools/reference/halo2-decompiled/src/online_tasks.cpp#L38),
[task layout](../../tools/reference/halo2-decompiled/include/online_tasks.h#L24),
`original-000902b0.asm`, fixture `query_tasks`.

| Task flags / record | Original poll status | Query completion owner |
| --- | --- | --- |
| started1 | 0 | Keeps task pending |
| running2 | 1 | Keeps task pending |
| finished4 | 2 | Parses results, then releases task |
| finished+failed0x24 | 3 | Releases task |
| valid record, flags0 | 4 | Releases task |
| invalid salt / missing record | 5 | Drops search's task handle; no valid record is released |

The completion owner leaves `search.state` unchanged in each case. By
contrast, full original `0x90B30 → 0x8FA80` with logonNONE returns taskNONE,
sets **search.state4**, and preserves the previous query-start clock. A valid
submission starts that clock, as established in the earlier note. Full
`0x70EA0` with empty results and an active search maps state0 to party mode6,
continues state1, and maps state4/5 (or another nonzero/non1 value) to
matchmaking state15. These three state domains must remain separate.
[Submission](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L196),
[Live wrapper](../../tools/reference/halo2-decompiled/src/online_matchmaking.cpp#L117),
[search role gate](../../tools/reference/halo2-decompiled/src/unknown_070ea0.cpp#L122),
`original-00090b30.asm`, fixture `search_owner`.

The lower-level radius owner `0x90F70` checks only state!=0, not state==1.
Do not infer a standalone error-state guard there: actual dispatcher order,
submission and role-owner checks jointly determine the flow. A task completion
failure can leave state1 and allow a subsequent query; that is different from
a submission failure detected by the role owner.
[Radius owner](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L753),
`original-000706f0.asm`, earlier gathering-lifecycle evidence.

## Completed QoS failures reject candidates

Complete original `0x905A0` controls use real QoS pool records and real target
status/extraction/description decoder callees. For normal full-function
controls one target is complete and another remains pending, avoiding the
unavailable active Xbox release call. Target flags and results are synthetic
inputs to the unchanged client, not measured network outcomes.
[QoS status/extraction](../../tools/reference/halo2-decompiled/src/unknown_07b4c0.cpp#L202),
[completion owner](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L382),
`original-000905a0.asm`, fixture `qos_targets`.

| Original target outcome | Status | Candidate effect |
| --- | --- | --- |
| Complete, not contacted | 3 | Ineligible and seen |
| Complete, target disabled | 4 | Ineligible and seen |
| Complete/contacted, no description data | 5 | Ineligible and seen |
| Complete/contacted, invalid description data | 5 | Ineligible and seen |
| Complete/contacted, valid encoded description | 5 | Copies details; records current tick; sets readiness for that probe kind |
| Partial-complete/contacted with data | 2 | Does not decode or set readiness in this owner |
| Probes transmitted, still pending | 1 | Does not set readiness |

A failed refresh leaves previously cached full-readiness/details bits set, but
sets eligibility false. Those bits alone must never admit selection. Successful
kind1 is brief-ready, not full-ready. Successful kind0 sets full-ready. On
successful decode the target index becomesNONE; borrowed payload size/pointer
are cleared after decoding. Rejection alone does not clear that target index.
[Rejection](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L360),
[readiness and cleanup](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L407),
`oracle-lifecycle.json`.

When the complete batch is observed, the owner clears outstanding indices of
still-eligible matching-kind entries, calls real QoS release, then clears the
search's handle. The active-batch control stops at the genuine release entry;
it does not execute/stub XNet. The disabled-subsystem control executes the full
function: default status3 rejects the target and the handle becomesNONE.
[Batch cleanup](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L434),
[release](../../tools/reference/halo2-decompiled/src/unknown_07b4c0.cpp#L165),
`oracle-lifecycle.json`.

There is a separate original scheduler oddity: matched `0x90420` tests the
returned QoS handle **against zero**, while matched `0x7B4C0` returnsNONE(-1)
when disabled or allocation/lookup fails. The fully executed disabled path
therefore storesNONE, records a scheduled count and leaves search.state1;
only a literal zero return takes its state5 branch. Do not label every failed
QoS allocation as retail state5. Active lookup/platform-failure execution
remains unavailable here.
[Scheduler](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L702),
[lookup](../../tools/reference/halo2-decompiled/src/unknown_07b4c0.cpp#L85),
`original-00090420.asm` at0x90561–0x90581,
`original-0007b4c0.asm`, `oracle-lifecycle.json`.

## Seen-ID capacity, overflow and lifetime

Original `0x90880` at **0x9091F reads absolute0x4CE1F8**, initialized by
original `0x66330` to150. With network configuration base0x4CE040, this is
**offset0x1B8**, also the cumulative-result limit. The executable stores150
in `search+0x98` and allocates150×8=1,200 bytes. The reconstructed TODO source
instead names `value1f8` (=25 at absolute0x4CE238); that is a decompilation
error for this owner. The independent original producer-block control verifies
capacity and allocation size. Prefer byte addresses over the wrong field name.
`original-00090880.asm` at0x9091F–0x90936,
[configuration defaults](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L594),
[contradicted reconstruction](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L827),
fixture `seen_bank`.

Complete `0x912D0` first marks a fresh candidate seen and ineligible, then
appends its8-byte advertisement ID only if absent and count<capacity. It is
append-only: no eviction, rotation or replacement. Controls fill all150 slots,
reject ID151 locally without retaining it, preserve IDs1…150, and confirm a
duplicate does not increase count. A future query can therefore rediscover an
overflow ID unless another gate rejects it. An already-seen candidate makes
the helper return immediately; importers are responsible for consistent
eligibility when restoring seen candidates.
[Seen lookup/rejection](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L342),
`original-00091290.asm`, `original-000912d0.asm`, `oracle-lifecycle.json`.

Full `0x90B30` clears both candidate/previous banks while preserving seen
count and IDs, including on a genuine offline submission failure. Thus the
bank persists across radius and same-radius query retries. New successful
search initialization resets the count at0x90A4F; the original tail is executed
in a labelled block control. The dispatcher calls cleanup when party mode
leaves7/8/9/13; cleanup frees the seen storage and clears the pointer/active
flag. Entering mode7 alone is not the reset instruction: the new initialization
owns the fresh bank. No allocator/free implementation was substituted to test
the full cleanup path.
[Query bank clearing](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L231),
[new initialization](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L854),
[cleanup](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L304),
[dispatcher gate](../../tools/reference/halo2-decompiled/src/unknown_0706f0.cpp#L111),
`original-000706f0.asm` at0x7090C–0x7092D,
`original-00090c80.asm`, fixture `seen_bank`.

The supported replacement contract is a bounded, stable-ID rejection bank
owned by one initialized search lifetime, retained across its queries and
discarded before a new initialization. Choice of native advertisement IDs,
relay probing and a transport-specific failure adapter remain replacement
implementation responsibilities; these controls do not make them Xbox Live.
