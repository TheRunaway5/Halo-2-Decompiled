# Original Halo 2 candidate scoring, ordering and selection

Research date: 2026-10-12. Scope: original retail client code and its configuration
defaults, with primary public statements used to identify intent and limits.
No native/backend/build/deployment code or earlier evidence note was changed.

The retail executable contains a recoverable composite scoring formula. It scores
potential **gathering sessions for one searching party**, sorts them, then chooses
the first sufficiently rated session with completed full QoS. This is a more
specific operation than assembling an entire match in a central queue. Reusing
the arithmetic is supported; treating current central ticket age, relay quality
percentages or arbitrary groups as equivalent inputs is not.

The formula, complete 50-entry sort, admission checks and selection have been
executed from unchanged retail instructions against synthetic records. There are
**238 passing controls**. This establishes bounded retail behavior, not the
missing Xbox Live search service, downloadable settings from a particular later
date, native transport equivalence or a new live deployment.

## Executable authority and reproducibility

The executable is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The oracle uses the existing `tools/oracle_game_variants.py` loader, maps its
original sections unchanged, and supplies data fixtures and a stack. It imports
no native matchmaking implementation. No original instruction, allocator,
callback or Live function is replaced. Threshold controls stop **before** the
real Live query entry `0x8FA80`; they do not manufacture a service response.

Run from the repository root:

```sh
.tools/python/bin/python .local/candidate-scoring-research-20261012/oracle_candidate_controls.py
```

Artifacts under `.local/candidate-scoring-research-20261012/`:

| Artifact | Purpose |
| --- | --- |
| `oracle_candidate_controls.py` | Independent original-byte fixture runner |
| `oracle-candidate.json` | Inputs, expected/actual outputs and all 238 controls |
| `executed-original-instructions.txt` | 1,958 original instruction addresses/bytes; each checked against the pinned image |
| `retail-defaults.json` | Values produced by original `0x66330`, including the entire language matrix |
| `original-function-spans.json` | Exact inventory sizes, source status and instruction hashes |
| `original-*.asm` | Original disassembly, including undecompiled producer/caller seams |
| `primary-source-manifest.json`, `gdc-38.*`, `gdc-48.*` | Retrieved primary presentation slides and their hashes |

Oracle SHA-256:
`c4ee42635f9398f877566dee702ba050dff4186dbae4556234d96ccae2a1d3ac`.
Results SHA-256:
`fe07e4b35e030f91fac0cbb7d956777c51ebf68f09f29512f496754fe5f18dbd`.
Executed-instruction artifact SHA-256:
`9a8a631cabdb2b9f67d9b43011fa77d7902c36658eb369d649d3379dc0df4ed4`.
Some reconstructed C++ routines remain `todo` in the function inventory. The
original instructions and these bounded controls are authoritative for the
specific claims below; C++ names are reading aids.

## Inputs and their owners

Search initialization `0x90880` receives the selected hopper, searching player
count, average/maximum/minimum party ranks, two party quality values, optional
common clan and session language. Caller `0x70EA0` supplies these from the actual
party state. The party rank calculation `0x72540` divides summed available ranks
by their count using integer division. Candidate aggregation `0x7E210` similarly
uses the advertised ranks of rated players; it does not use the adjusted
team-balancing rank array for the candidate average. Clan hoppers inherit the
clan-owned rank path documented in
[clan evidence](halo2-clan-matchmaking-evidence.md).
[Search initializer](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L821),
[caller](../../tools/reference/halo2-decompiled/src/unknown_070ea0.cpp#L62),
[party rank calculation](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1090),
[candidate aggregation](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp#L1102).

Detailed descriptions are copied at candidate offset `+0xA8` by `0x905A0` after
`0x7E080` decodes a successful QoS payload. Their producer is `0x73530`.

| Scorer input | Detailed candidate offset | Producer/units |
| --- | --- | --- |
| Candidate average rank | `+0x120` | Description `+0x78`, aggregation `0x7E210` |
| Remaining player space | `+0x10C` | Description `+0x64`: capacity minus current player count |
| Waiting value | `+0x11C` | Description `+0x74`: `time(NULL) - session.time78b4`, **seconds** |
| Candidate quality first/second | `+0xF8`, `+0xFC` | Description `+0x50/+0x54`, selected party machine via `0x7E100` |
| Candidate language | `+0xB8` | Description `+0x10`, game/session language index |
| Measured latency | `+0x90` | QoS result at `+0x84`, `rtt_median` at `+0x0C`, **milliseconds** |
| Full QoS readiness | byte `+0x74` | Successfully extracted and decoded **kind-0** QoS result |

The waiting value is the advertised session wall-clock age. Session creation
initializes `time78b4 = time(NULL)`; it is not established as the age of an
arbitrary central ticket. The candidate quality pair also differs from current
relay measurements: the first originates in the sign-in QoS **downstream bits/s**
field, through `0x2BB0E9 → 0x19ADF6 → 0x64870 → 0x658C0`. The second is
`g_4CF968`, Halo 2's separate bandwidth estimate. `0x7F410` derives that estimate
from trimmed historical send/receive samples, reserve/minimum configuration and
an upstream QoS sample when historical estimates are unavailable. `0x7E100` may
choose a better eligible machine in the party, rather than simply returning the
current machine's pair. These are not two raw upload/download measurements to a
modern relay.
[Description producer](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1173),
[QoS extraction](../../tools/reference/halo2-decompiled/src/unknown_07b4c0.cpp#L240),
[sign-in QoS forwarding](../../tools/reference/halo2-decompiled/src/unknown_2b116a.cpp#L5098),
[interface cache](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L361),
[local member properties](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L1126),
[bandwidth estimate](../../tools/reference/halo2-decompiled/src/unknown_07f190.cpp#L225),
[party quality selection](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp#L725),
[session creation](../../tools/reference/halo2-decompiled/src/unknown_059ad0.cpp#L3275).

Before a detailed description arrives, the seven Live result attributes supply
rank at `+0x5C`, free space at `+0x60`, age at `+0x6C` and admission values at
`+0x64/+0x68/+0x70`. The scorer can rank these brief results but cannot yet add
description-dependent quality/language terms. `0x8FB30` preserves the service's
result order while parsing its seven integers; undecompiled `0x902B0` copies each
0x68-byte result into a 0x204-byte candidate and clears its two QoS readiness
flags. No service-side query ordering is recovered here.
[Live result parser](../../tools/reference/halo2-decompiled/src/online_matchmaking.cpp#L66),
[attribute conversion](../../tools/reference/halo2-decompiled/src/unknown_0b49a0.cpp#L78),
[brief/detailed scorer branches](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L606).

## Exact retail-default score

Original `0x91000` resets each candidate's score to zero. Invalid/ineligible
candidates remain zero. For an admitted candidate, operations occur in this
order. `trunc` means conversion toward zero; `f32` means the individual SSE
single-precision operations, not algebraically equivalent double arithmetic.

```text
d = abs(candidate_average_rank - searching_party_average_rank)

score = 0
if d <= radius:
    score += rank_high - trunc((rank_high-rank_low)/(radius+1)) * d

if remaining_space == searching_party_player_count:
    score += exact_space_bonus

bounded_age = clamp(advertised_session_age_seconds, 0, maximum_age)
score = trunc(score + bounded_age * age_coefficient_double)

if detailed_description_available:
    if (candidate_first < first_threshold OR candidate_second < second_threshold)
       AND own_first >= first_threshold AND own_second >= second_threshold:
        score += quality_adjustment
    score += language_matrix[own_language][candidate_language]

if full_qos_ready:
    t = clamp(f32(f32(median_rtt_ms) * f32(0.01)), 0, 9)
    index = trunc(t)
    if index < 9:
        score += trunc(f32(f32(curve[index]) +
                 f32(f32(t-f32(index)) *
                     f32(f32(curve[index+1])-f32(curve[index])))))
    else:
        score += curve[9]
```

Configuration base is `0x4CE040`. Original `0x66330` produces these defaults:

| Offset(s) | Default | Observed meaning |
| --- | --- | --- |
| `+0x1F0/+0x1F4/+0x1F8` | `4 / 100 / 25` | Rank radius/high/low coefficients |
| `+0x340` | `50` | Exact remaining-space bonus |
| `+0x344/+0x348` | `180 / 2.0` (double) | Age clamp in seconds/coefficient |
| `+0xDC/+0xD8` | `262144 / 262144` | First/second quality thresholds |
| `+0x350` | `100` | Quality adjustment |
| `+0x1C0…+0x1E4` | `100,80,50,0,-100,-200,-200,-200,-200,-200` | Latency curve at 100-ms intervals |
| `+0x1FC` | Nine by nine table | Language adjustments, below |

The default rank contribution at distance `0,1,2,3,4` is
`100,85,70,55,40`; at distance `>4` it is zero. The configured low value25 is
**not** the contribution at distance4: integer division happens first, and its
denominator is5. This term is still evaluated for casual/unranked searches.
Unbounded admission does not remove it.

Default age180 or greater contributes360; negative age contributes0. With a
synthetic fractional coefficient the conversion happens before quality/language
and RTT are added. The original binary's RTT contribution at499ms is **−198**,
where ideal real-number interpolation followed by a single truncation would
yield−199. Retain the SSE operation order if exact arithmetic parity is intended.

The quality predicate above awards **+100 when the candidate's pair is below a
threshold and the searching party's pair meets both thresholds**. This direction
is verified in original instructions at `0x91164–0x91194` and controls. Do not
silently reverse it to match an intuitive host preference.
[Scorer](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L593),
[default initializer](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L575),
`original-00091000.asm`, `retail-defaults.json`, `oracle-candidate.json`.

The full default language table can be represented without inventing names for
unverified platform indices. Let `special = {1,6,7}`:

- Same index: `+100` for a special index, otherwise0.
- Different indices: `−100` if either is special, otherwise`−50`.

This reproduces all81 cells, each exercised against original scoring bytes.
`0x11CA80` maps platform language values1…8 to game indices0…7, value9/default
to0; the9×9 configuration still includes index8. The slide and this research do
not justify substituting country, region, Discord locale or ping for that index.
[Language mapping](../../tools/reference/halo2-decompiled/src/unknown_11c9c0.cpp#L78),
[matrix initializer](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L618).

## Admission, ordering and the selection threshold

Admission `0x90D90` precedes scoring. It requires the advertised add-mask bit for
the searching party's player count, rejects the cached local XNet address and
enforces enabled rank bounds. These bounds use the advertised maximum and
**mapped/floored minimum**, not a newly chosen average-distance rule. With
details present it additionally requires protocol kind4, lower version≥`0x2651`
and upper version≤`0x2651`; a clan search rejects an advertised clan equal to its
own. The literal version predicates are recorded even though inverted lower/upper
fixtures can satisfy them. Without details, version/clan checks wait for the
description. The search client's rank predicates concern its own outgoing
search; this does not establish the current central service's reciprocal party
eligibility rule.
[Admission](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L536),
[minimum producer](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1226),
[separate clan admission/host acceptance](halo2-clan-matchmaking-evidence.md).

`0x91000 → 0x13DA70` sorts **all50 slots by descending score**. The original
comparator `0x90F10` returns true for a smaller score, but the generic sort moves
that result toward the end. Among fully QoS-ready entries with equal score,
lower median RTT wins, then lower rank difference, then less remaining space.
The actual comparator tests the **first operand's** readiness before comparing
RTT; with a ready/unready pair it can return true in both directions. Two such
controls pass. A universal tuple sort is therefore not a proven replacement for
every intermediate state, and completely tied entries have no established stable
order guarantee. Identical candidate/snapshot bytes cause the scorer to return
without recalculating, even if a configuration coefficient changed.
[Comparator and sort call](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L504),
[original generic sort reconstruction](../../tools/reference/halo2-decompiled/src/unknown_13dcd0.cpp#L54).

`0x91400` scans that array and retains the **first** valid, eligible, fully ready
entry whose score≥`search.minimum_score`. Selection succeeds immediately if
every eligible entry is fully ready. Otherwise it requires:

```text
ready_count >= floor(eligible_count / 4)
AND elapsed_since_this_query_start_ms > 5000
```

A qualifying ready entry is required even when the floored quarter is0.
The5,000-ms boundary is strict:5,000 waits;5,001 succeeds in the control.

## Portable core and expanded differential checks

`master_server/session_candidates.py` now ports the recovered admission,
composite arithmetic, exact 50-record sort, byte-cache, radius threshold and
readiness selection. It preserves the asymmetric mixed-readiness comparator,
the original eight-record short sort, individual float32 RTT operations and
the strict query-delay boundary. It keeps the complete internal candidate
bytes so opaque changes invalidate the cache and unchanged sorted bytes bypass
rescoring. This is an internal representation, not a new public wire protocol.

The independent original-byte runner is now portable:

```sh
.tools/python/bin/python tools/oracle_session_candidates.py .local/session-candidates-oracle
python3 -m unittest multiplayer.master_server.tests.test_session_candidates -v
```

The runner imports no replacement implementation. Its outputs contain **264
complete input/previous/output banks, 27 admission cases, 124 selections and100
threshold cases**. They include every integer RTT from0 through1000ms,96 seeded
mixed-readiness/invalid/brief/detailed banks, opaque-byte changes, negative
configured coefficients, cached rescoring and tick wrap. All complete output
banks match byte for byte. The expanded runner also passes339 scalar assertions
and verifies1,987 executed original instructions against the pinned image.
These counts overlap; they are not339 additional independent banks.

Evidence is under `.local/session-candidates-port-20261012/original-final/`.
The checked-in compressed fixture is
`master_server/tests/fixtures/retail-session-candidates.json.gz`.

| Artifact | SHA-256 |
| --- | --- |
| `tools/oracle_session_candidates.py` | `29133faa0b9a6a08868a3477f1f3a16bebc8e4c0660f8f2d11e9f4cf0a05e59e` |
| Compressed differential fixture | `20ef2f951fdac45ebec6501c95dd3b372c706392961f89b36b51bf1acb8f649a` |
| Executed original instructions | `a673673dfe0ad5bfe5e2f3833ea592e4a4e741127ded6afbe4660cf30acf8845` |

The central queue does **not yet call this candidate core**. Its native grouping,
host choice and timing remain separately documented approximations. Live
integration still needs actual gathering-session advertisements and age,
title-language ownership, and candidate route QoS with decoded descriptions.
The [bandwidth-input controls](halo2-bandwidth-input-evidence.md) establish
legitimate unavailable values `(0,-1)`; they do not supply missing route RTT or
turn the relay percentage into original QoS.
`0x90B30` resets the start timestamp after each actual query succeeds, so this
clock is per query attempt. It is separate from candidate session age.

`0x90B30` computes the threshold from current expansion radius`r` and maximum
radius`R`, using float32 arithmetic and conversion toward zero:

```text
if r != NONE and R > 0:
    minimum_score = trunc(f32(200 + f32(f32(r)/f32(R)) * f32(100-200)))
else:
    minimum_score = 100
```

For `R=6`, original prefix controls give `200,183,166,150,133,116,100`.
The final casual `NONE` range removes min/max rank admission and retains100.
These are radius-driven thresholds, not a recovered schedule measured in seconds.
The existing native30-second final casual stage remains a separately documented
replacement-service timing policy.
[Selection](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L663),
[query/range/threshold builder](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L197),
[casual expansion evidence](halo2-casual-skill-evidence.md#casual-widening-and-the-unrestricted-pass).

## Public statements and configuration provenance

Butcher and House's GDC2005 presentation describes party-host-driven searching,
Live hopper/rank queries and scoring by rank, space, host quality, waiting time,
latency and language. It is primary evidence for those dimensions. Slide38's
host-quality wording does not explain the retail predicate's direction, so this
note preserves the measured predicate rather than rewriting it.
[GDC2005 slide38](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=38).

Slide48 describes a threshold followed by an otherwise random choice. In the
examined retail local path, the complete sort has a fixed middle pivot and the
selector takes the first qualifying result. Twelve repeated original selector
calls choose the same synthetic host. Caller `0x70EA0` immediately uses that
selected identifier/address to start joining. **No RNG shortlist is recovered in
this path.** The slide might describe another build, service result selection or
a higher-level characterization; this research cannot choose among those
possibilities. Do not turn the slide into guessed random weights or claim that
the whole historical service was deterministic.
[GDC2005 slide48](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=48),
[join consumer](../../tools/reference/halo2-decompiled/src/unknown_070ea0.cpp#L84).

These coefficients are **retail executable defaults**. `g_477058` points its
download buffer at `g_network_configuration` with size`0x1730`; its definition
names `network_configuration.dat` and format version`0x23`. Original logon path
`0x6BFF0` queues this request through `0x80F70`. The pending receive path
`0x8E240 → 0xB4450` downloads to that buffer through Xbox title storage. Its
kind0 path is version-prefixed, normally `9809/network_configuration.dat` because
`0x2651 = 9809`. `0x81230` checks the declared size, definition version and CRC.
Original validation controls accept a synthetic correctly sized/versioned/CRC
file and reject old/new versions, wrong size and corruption. They do not recover
the content of any actual downloadable override.
[Definition and buffer](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L854),
[request queue](../../tools/reference/halo2-decompiled/src/unknown_080f30.cpp#L101),
[pending transfer](../../tools/reference/halo2-decompiled/src/unknown_08e1e0.cpp#L211),
[storage path](../../tools/reference/halo2-decompiled/src/unknown_0b3d30.cpp#L110),
[payload validation](../../tools/reference/halo2-decompiled/src/unknown_080f30.cpp#L39).

No historical override file was found in the checked local assets. This closes
the **retail-default coefficient** gap left in the earlier casual note, while
later/dated Live configuration and service query behavior remain unresolved.

## Supported next implementation boundary

An explicitly versioned retail-default session-candidate policy can reproduce
the formula, constants, table, admission ordering, readiness gate and threshold
arithmetic using correctly sourced inputs. Party/hopper rank, player count,
capacity, clan ownership and game language are available from authoritative game
state. A central implementation still needs to define actual candidate gathering
sessions and their creation times, obtain comparable median RTT/full-probe
success, and supply the original quality inputs or declare a different transport
policy. Current relay QoS percentages do not fill those fields.

Complete match assembly, reciprocal rank constraints, choosing a match host from
central participants, relay routing and modern privacy/identity behavior can
remain replacement-service responsibilities, with their differences stated.
This evidence does not authorize fabricated bandwidth estimates, historical
download settings, universal stable tie ordering, an RNG shortlist or a claim of
one-to-one Live interoperability. All fixture identities in these artifacts are
synthetic; no Discord identities, tokens or credentials are recorded.
