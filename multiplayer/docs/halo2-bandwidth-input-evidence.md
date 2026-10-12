# Original Halo 2 bandwidth inputs and unavailable-quality behavior

Research date: 2026-10-12. Scope: the original retail client bandwidth estimator,
its sample producers, party quality selection, and the complete candidate
description/QoS/scoring/selection path. This note changes no native/backend code,
build, launcher or deployment. It supplements the
[candidate scoring evidence](halo2-candidate-scoring-evidence.md).

**The original client permits selection with legitimately unavailable bandwidth
quality.** A fresh local machine advertises first quality **0** and second quality
**NONE (`-1`)** when no successful sign-in QoS cache or usable estimator sample
exists. The detailed codec preserves those values; validation does not require
positive quality. Completed full candidate QoS plus a valid description can make
that candidate selectable. Unavailable bandwidth is therefore not a reason to
invent a bandwidth estimate, reinterpret relay percentages, or mark brief QoS as
full QoS. This exact path passes unchanged-original-byte controls described below.

## Authority and reproducibility

Executable: `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The oracle uses the existing `tools/oracle_game_variants.py` loader and supplies
synthetic data, configuration and a stack. Original instructions and callees
remain unchanged. No platform callback, XNet implementation or native
implementation is replaced. Reconstructed C++ names are reading aids; original
disassembly and executed bytes are authority for the bounded claims here.

```sh
.tools/python/bin/python .local/bandwidth-input-research-20261012/oracle_bandwidth_controls.py
```

There are **100 passing controls** and **3,244 unique executed original
instructions**, each compared with the pinned image. Artifacts are under
`.local/bandwidth-input-research-20261012/`:

| Artifact | Purpose |
| --- | --- |
| `oracle_bandwidth_controls.py` | Independent original-byte fixture runner |
| `oracle-bandwidth.json` | Every input, expected value and actual value |
| `retail-defaults.json` | Original initializer's estimator/party thresholds and full level table |
| `executed-original-instructions.txt` | Executed addresses and original bytes |
| `original-function-spans.json` | Function inventory, source status, sizes and byte hashes |
| `original-*.asm` | Original producer/caller/consumer disassembly |

Oracle SHA-256:
`32c6925c36a0110c15bbf93695d6ad289df7bb34912ff7d51cb8e82096de1219`.
Result SHA-256:
`a0045bffc6adc4bdd86355bff87cd2710e68b6baefc2bc59b5e3661d1bcb378b`.
Executed-instruction artifact SHA-256:
`06594182dec23937ec150c10b7f3eb842a62797f340c5890b6a5cd815ce353e9`.
These controls do not execute the lost Live service or establish a modern
transport's equivalent measurements.

## Configuration provenance and availability

The original initializer `0x8D9F0` supplies observer configuration `0x4CF4E0`
at `0x8DB3B → 0x75970`; it supplies estimator configuration `0x4CE050` at
`0x8DCD3 → 0x7F020`. These are network configuration base `0x4CE040` plus
`0x14A0` and `0x10`, respectively. Original `0x66330` initializes the values
below. Downloaded historical overrides remain unresolved, as documented in the
candidate evidence. The table records executable defaults, not every later
playlist/service configuration.
[Defaults](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L512),
`original-0008d9f0.asm`, `retail-defaults.json`.

| Estimator relative offset | Retail default | Use |
| --- | --- | --- |
| `+0x00` | float `0.25` | Fraction trimmed from **each** end of a sample set |
| `+0x04` | `2` | Minimum paired observer sample count |
| `+0x08` | `20480` bits/s | Reserve subtracted from observer second value |
| `+0x0C` | `51200` bits/s | Minimum non-NONE published estimate |
| `+0x10` | `300000` ms | Minimum collection window |
| `+0x14` | float `0.5` | Strict upper bound for acceptable period ratio |
| `+0x18` | `5` | Strict excess-count threshold before growth |
| `+0x1C` | `65536` bits/s | Strict upstream QoS excess test |
| `+0x20` | `32768` bits/s | Growth step per count above threshold |

`0x7F020` starts with the estimator disabled, history unloaded, published
estimate `g_4CF968 = -1`, measured flag `g_4CF964 = false` and published level
`g_4CF96C = 16`. Successful original Live logon enables it in `0x6C2A0` and
recalculates. Original logoff disables it. Discord authentication is a replacement
identity mechanism; it does not supply original XNet capacity measurements.
[Initialization](../../tools/reference/halo2-decompiled/src/unknown_07f190.cpp#L76),
[logon gate](../../tools/reference/halo2-decompiled/src/online_tasks.cpp#L784),
[logoff](../../tools/reference/halo2-decompiled/src/unknown_06b590.cpp#L394).

The estimate is published **before** the observer's operational budget is
clamped. A non-NONE observer budget is bounded by observer configuration
`+0x114 = 10240` and `1048576` bits/s. An unavailable estimate gives the observer
budget `1048576` and observer flag `+0x4E01 = true`, while the advertised estimate
stays `-1` and estimator measured flag stays false. The observer flag must not
be interpreted as proof that this fallback was measured. A control publishes
`2000000` while the operational budget is capped at `1048576`.
[Publication/budget branch](../../tools/reference/halo2-decompiled/src/unknown_07f190.cpp#L281),
[observer minimum](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L427),
`oracle-bandwidth.json`.

## Sign-in QoS owners and exact units

`0x2BB04E` starts `qos_service_lookup()` after successful logon.
`0x7B5E0` invokes **`XNetQosServiceLookup(0, 0, &qos)`**. Its endpoint selection,
probing implementation and Internet measurement behavior belong to the original
platform/service and are not recovered by this wrapper. The result is distinct
from a candidate-session probe or a relay health percentage.
[Sign-in task](../../tools/reference/halo2-decompiled/src/unknown_2b116a.cpp#L5267),
[service lookup](../../tools/reference/halo2-decompiled/src/unknown_07b4c0.cpp#L136).

`0x7B7B0` accepts a contacted target with complete or partially complete flags,
then copies the XNet fields into this 32-byte result:

| Result offset | Field/units |
| --- | --- |
| `+0x00`, `+0x04` | Probes sent/received, counts |
| `+0x08`, `+0x0C` | Minimum/median RTT, **milliseconds** |
| `+0x10` | Upstream, **bits per second** |
| `+0x14` | Downstream, **bits per second** |
| `+0x18`, `+0x1C` | Optional payload size/pointer |

There is no positive-bandwidth or positive-RTT validation here. Success and
measurement values are separate. A zero bandwidth field is copied unchanged;
the wrapper does not replace it with a fabricated capacity.
[Extraction](../../tools/reference/halo2-decompiled/src/unknown_07b4c0.cpp#L240).

`0x2BB0E9 → 0x19ADF6` forwards a successful sign-in result and NAT kind to two
owners, in this order:

1. **`0x7F0D0` consumes upstream `+0x10`**, appending it to the estimator's scalar
   history, recalculating, checking the excess threshold, and persisting history.
2. **`0x64870` caches all 32 bytes**, NAT kind and the resulting estimate/level.
   The downstream cache is `g_4CD868 + 0x78`.

Local member-property update `0x658C0` starts the first quality field at0, copies
the cached downstream only when cache-valid, and sets the second field directly
from `g_4CF968`. Thus a fresh unavailable pair is `(0, -1)`. In member properties
the fields are `+0x68/+0x6C`; in the complete 0x10C-byte member entry they are
`+0x90/+0x94`. NAT kind is entry `+0x98`, and reachability mask is `+0x9C`.
The original whole forwarding/cache path is executed in the oracle, including a
successful all-zero-bps result that leaves `(0,-1)` unavailable.
[Interface initialization](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L115),
[Forwarding](../../tools/reference/halo2-decompiled/src/unknown_2b116a.cpp#L5098),
[two owners](../../tools/reference/halo2-decompiled/src/unknown_19987f.cpp#L1230),
[cache](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L361),
[local properties](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L1128),
`original-0019adf6.asm`, `oracle-bandwidth.json`.

Do not confuse the configured **candidate-probe traffic budget** with a measured
bandwidth result. `0x7B4C0` uses defaults `16384` bits/s for kind0 and `4096`
bits/s for kind1 when its budget argument is NONE. Its probe-count arguments are
8 and0 respectively; the platform's interpretation of the latter is outside
this research. Neither constant is the advertised machine quality.
[Candidate lookup wrapper](../../tools/reference/halo2-decompiled/src/unknown_07b4c0.cpp#L55).

## Gameplay observer history is not a clean upload/download pair

The decompile's estimator locals named `sent` and `received` must not determine
native field names. Their actual producers have different ownership:

- `0x76670` adds sent packet bytes to active channel `+0x4A4`.
  `0x76720 → 0x79660` adds received bytes to channel `+0x4A8`.
- `0x795B0` and `0x79560` calculate those rates as
  `bytes * 8000 / elapsed_ms`, integer arithmetic. Independent controls give
  `1000 sent bytes / 1000ms = 8000 bits/s` and
  `2000 received bytes / 1000ms = 16000 bits/s`. Nonpositive elapsed time gives0.
- `0x79260` totals **received rates** across active channels and calls `0x77F90`
  when there are no loss-penalized channels. That updates observer **`+0x4E08`**
  as a maximum. It also raises already-known `+0x4E0C` toward a larger received
  aggregate by a configured arithmetic shift.
- When flag `+0x4F3E` is set, `0x79260` separately totals **sent rates** and
  initializes or smooths observer **`+0x4E0C`** toward that total. This is a mixed
  owner, not an independent raw downstream or upstream test.

[Packet owners](../../tools/reference/halo2-decompiled/src/unknown_075870.cpp#L586),
[receive accumulator](../../tools/reference/halo2-decompiled/src/unknown_075870.cpp#L1830),
[rates](../../tools/reference/halo2-decompiled/src/unknown_075870.cpp#L1540),
[aggregate owner](../../tools/reference/halo2-decompiled/src/unknown_075870.cpp#L1860),
[maximum/smoothing](../../tools/reference/halo2-decompiled/src/unknown_075870.cpp#L1044),
`original-00079260.asm`.

`0x77F90` also counts elapsed periods of the predicate
`qualifying_channels > active_channels * configured_share`. A qualifying channel
is not rate-limited and either not budget-limited or meets the received-rate
threshold. Defaults are share0.75, threshold30720 bits/s, period60000ms and
smoothing shift3. This period ratio is **not packet loss**, relay quality or a
fraction of bandwidth used. `0x75C80` returns the two observer values and
`true_period_count / total_period_count`; it fails if either value is NONE or
there are no counted periods.
[Qualification](../../tools/reference/halo2-decompiled/src/unknown_075870.cpp#L1880),
[period owner](../../tools/reference/halo2-decompiled/src/unknown_075870.cpp#L1044),
[defaults](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L464),
`original-00075c80.asm`.

The 27-long history at `g_4CF8F0`, persisted as `g_51099C`, contains scalar
count/values at `[0]/[1…8]`, paired count at `[9]`, paired first values `[10…17]`,
paired second values `[18…25]`, and excess count `[26]`. Both histories are
eight-sample FIFOs. `0x7F2A0` collects a pair only after300000ms and when the
period ratio is strictly below0.5 **or both values are at least the prior
published estimate**. It then clears excess count, persists and recalculates.
It resets the observer and clears pending collection even on a rejected or
unavailable sample. `0x7F070` starts/ends collection around its original game
mode/flag lifecycle; replacing it with an arbitrary periodic timer is an adapter
decision, not established lifecycle parity.
[Scalar/history owner](../../tools/reference/halo2-decompiled/src/unknown_07f190.cpp#L129),
[pair collection](../../tools/reference/halo2-decompiled/src/unknown_07f190.cpp#L158),
[lifecycle](../../tools/reference/halo2-decompiled/src/unknown_07f190.cpp#L200).

## Exact estimator and party quality selection

`0x7F190` drops `trunc(f32(count * 0.25))` samples from each sorted end, then
integer-divides the remaining sum. With two samples it trims none; with four it
averages the middle two. `0x7F410`, when enabled, uses this logic:

```text
A = B = U = 0
if paired_count >= 2:
    A = trimmed_mean(observer_first_history)
    B = trimmed_mean(observer_second_history)
if upstream_qos_count > 0:
    U = trimmed_mean(upstream_qos_history)

estimate = NONE
measured = false
if A > 0:
    estimate = max(min(A, B - 20480), 51200)
    measured = true
    if U > estimate and excess_count > 5:
        estimate = min(U, estimate + (excess_count - 5) * 32768)
else if U > 0:
    estimate = max(U, 51200)
```

`0x7F0D0` increments excess count only when the recalculated estimate is marked
measured and the new upstream sample is **strictly greater than estimate+65536**.
The scalar fallback is available but is not marked measured. The controls cover
reserve/minimum, two-sample gate, trimming, scalar fallback, disabled behavior,
FIFO rollover, strict excess and growth boundaries.
[Mean](../../tools/reference/halo2-decompiled/src/unknown_07f190.cpp#L38),
[estimator](../../tools/reference/halo2-decompiled/src/unknown_07f190.cpp#L224),
[excess owner](../../tools/reference/halo2-decompiled/src/unknown_07f190.cpp#L147).

Party selector `0x7E100` starts with the current member's **actual pair**, without
requiring that member to pass the alternative-machine thresholds. It considers
members in order, replacing the best only when first≥131072, second≥131072,
reachability mask equals `(1 << member_count)-1`, and second≥best_second+10240.
Config flag `+0x1AC` optionally excludes NAT kind3; retail defaultfalse does not.
It derives a level from the estimate threshold table and floors it to8. This
level is a capacity value, not player skill or a measured-bps fallback. With
current `(0,-1)`, output remains that pair and level8. With current NONE and no
eligible member, selection is NONE, pair `(0,0)`, level8.
[Selector](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp#L725),
[threshold defaults](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L565).

The collection is the session's **member/machine** collection, not a list of
individual player ranks. `0x72330` obtains both searching quality and the
capacity level through this selector. `0x73530` advertises the selected pair at
description `+0x50/+0x54`; `0x70EA0 → 0x90880` supplies the searching pair.
[Search preparation](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1204),
[description producer](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1173),
[search caller](../../tools/reference/halo2-decompiled/src/unknown_070ea0.cpp#L77).

## Full selection with missing quality, and the native bridge boundary

`0x7DB10/0x7DF00` encode/decode both quality values as raw32-bit data.
`0x7DA60` validates counts/type/version ranges, but has **no quality-field
presence/range check**. `0x905A0` sets description-present `+0xA4` and readiness
`+0x74+kind` only after extracting a successful target result with nonempty
payload and decoding it successfully. Full readiness is kind0; a kind1 response
can contain valid details while remaining ineligible for final selection.
[Validation/decoder](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp#L539),
[encoder](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp#L803),
[completion](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L382).

The oracle executes the entire encoder → QoS extraction → decoder → completion
→ scorer → original full sort → selector with candidate pair `(0,-1)`, own pair
`(0,-1)`, median RTT50ms, matching rank, exact space, age0 and neutral language.
It retains the pair exactly, marks full readiness, scores240 and selects at
threshold200. It also executes `(0,0)` and `(-1,-1)` codec controls, without
claiming those are the normal fresh local representation. With own pair
`(300000,300000)` and candidate `(0,-1)`, the literal original quality predicate
adds100, producing340. Suppressing the adjustment because candidate quality is
unknown would change retail arithmetic. A kind1-only result does not select;
missing QoS payload invalidates the candidate.
[Scoring](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L593),
[selection](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L663),
`oracle-bandwidth.json`.

The synthetic QoS pool contains one completed target and another genuinely
pending target. This permits the full original completed-target consumer to run
without entering XNet handle release; no callee is stubbed. It establishes the
client's data/readiness behavior, not a successful physical-network probe.

Supported implementation seams are therefore:

- Represent unavailable advertised bandwidth explicitly as first0/secondNONE.
  Keep the estimator publication separate from operational transport budgets.
- Apply the recovered scorer to faithfully owned values and preserve its literal
  missing-value arithmetic. Keep full readiness independent of bandwidth
  availability but dependent on genuine candidate-response/QoS completion.
- If implementing measurement later, capture the original-shaped upstream QoS
  history, downstream cache and observer first/second owners, their units,
  lifecycle and configuration. Party quality selection also needs actual member
  reachability/NAT semantics; Discord identity and a relay status percentage
  do not provide those inputs.

Still unresolved are the original service-QoS measurement implementation,
historical downloaded settings, equivalent modern NAT classification,
equivalent member reachability, and transport-level reproduction of the
observer's probing/budget/smoothing lifecycle. A central relay can measure its
own paths, but this research does not establish relay round-trip time as equal
to the original searching-party-to-advertised-session median RTT. Such a route
adapter needs measured inputs and explicit replacement-service provenance.
None of these unknowns justifies fabricating the two bandwidth values, and the
verified missing-quality path allows them to remain unavailable.
