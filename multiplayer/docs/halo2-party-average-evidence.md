# Original Halo 2 party search rank: integer raw-rank aggregation

Research date: 2026-10-12. This supplements the
[candidate evidence](halo2-candidate-scoring-evidence.md) and
[clan evidence](halo2-clan-matchmaking-evidence.md). It changes no coordinator,
backend, native code, build or deployment.

**Search rank is an integer average of the unadjusted selected-hopper raw ranks.**
Original `0x72540` finishes with signed `IDIV` at `0x726D1`. For nonnegative
available ranks this floors the mean: `[1,2] → 1`, `[2,3,8] → 4`,
`[49,48] → 48`. Retaining a rational mean changes the original search input.
Neither capped display rank nor the optional adjusted rank is the source of this
average. These are raw inputs; a displayed-level conversion must stay separate.
[Original helper reconstruction](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1092),
`original-00072540.asm`.

## Independent unchanged-byte controls

Run from the repository root:

```sh
.tools/python/bin/python .local/party-average-research-20261012/oracle_party_average.py
```

The runner executes the **complete original `0x72540` and `0x7E210`**, including
original identity lookup, validation and arithmetic callees, against synthetic
session/cache/summary records. No instruction or callee is patched or replaced.
There are **18 passing controls** and **2,773 unique executed original
instructions**, all compared with the pinned executable. The setup uses original
`0x66330` defaults and `0x193C70` built-in hopper initialization. One adjustment
table is explicitly synthetic to distinguish raw from adjusted values; it is not
a recovered historical configuration claim.

Executable SHA-256:
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Oracle SHA-256:
`16bd9272db4f0d5299b741c536f15ab9e5a0e1ab7ed6e93ff316b5ac709619df`.
Result SHA-256:
`056c8a724bf478bd476f89945b868488acb9461159a202417d7c2454633dcb49`.
Executed-instruction artifact SHA-256:
`fa18dbd19bc1f999ce0dc451c14b969cf36e9c5c3387041e6a8f23164070536e`.

Artifacts under `.local/party-average-research-20261012/` are
`oracle_party_average.py`, `oracle-party-average.json`,
`executed-original-instructions.txt`, `original-function-spans.json` and
`original-*.asm`. JSON top-level fields are `xbe_sha256`, `control_count`,
`unique_original_instructions`, `trace_sha256`, `oracle_sha256`, `scope` and
`controls`. Each control contains `name`, `actual`, `expected` and `inputs`.
Available-rank cases named `integer_mean_(...)` have `inputs.ranks` and an output
array `[mode, ready, minimum, maximum, average]`. Tuple values serialize as arrays.

## Source ownership, missing ranks and guests

`0x72540` reads the searching session from `state.owner.session_a`. Its rank
array is `state+0x878`, XP array `+0x8B8`, selected hopper `+0x18`, player count
`+0x7B4` and identity summary `+0x630`. Original `0x63190` matches the full
12-byte user identity to find the summary index. When all readiness gates pass,
the helper sums entries whose rank is not NONE (`-1`), counts those entries,
and writes minimum `+0x95C`, maximum `+0x958` and integer average `+0x960`.
`0x70EA0` passes the resulting average, maximum and minimum into `0x90880`.
[Identity lookup](../../tools/reference/halo2-decompiled/src/network_session_membership.cpp#L291),
[collection and division](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1092),
[search caller](../../tools/reference/halo2-decompiled/src/unknown_070ea0.cpp#L77).

The readiness gates matter; this is not a general rule to silently discard
missing ordinary players:

- For a nonguest, a cached rank **or XP** equal to NONE triggers recovery from
  the player's properties. Recovery requires the selected hopper to match and
  both property raw rank and XP to be available. Failure leaves `ready=false`
  and does not produce a new average.
- Guest identity flag bits1/2 force that slot's cached rank/XP to NONE and
  exclude it. `[10, guest,20]` averages15 even if the guest's initial cache
  contained49. If no rated entries remain, the helper changes mode to1.
- Player-count/identity/map-readiness failures have their separate mode6 path.
  An unavailable member map-state control confirms no new average is written.

The values404 in blocked control outputs are observation sentinels supplied by
the fixture, not original defaults. An incomplete or stale nonguest record
remaining at404 proves the helper did not calculate an average.
[Recovery/guest gates](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1097),
[map/rated-count gates](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1138).

The property source is **raw rank at complete player `+0x132`**, equivalent to
properties `+0x8A`; XP is complete player `+0x134` / properties `+0x8C`.
The nearby adjusted field at complete player `+0x130` / properties `+0x88` is
not read by this helper. An independent recovery control with raw7 and adjusted60
copies7, yielding average12 from `[10,7,20]`.
`0x65530` publishes selected-hopper cached raw rank/XP at `+0x8A/+0x8C` and
initializes `+0x88` separately. Later gathered match publication `0x71FF0` also
writes raw, XP and adjusted values to these distinct property fields.
[Selected-hopper publisher](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L1444),
[gathered publication](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1741),
`original-00072540.asm`, `original-00071ff0.asm`.

## Clan and gathered-candidate boundary

`0x72540` has no type-5 aggregation branch. The clan-specific owner is upstream:
`0x74AA0` requests clan identity for the selected type-5 hopper namespace, and
the selected-hopper publisher exposes those clan-subject results through the
same raw-rank/XP properties. A complete original helper control with the original
Minor Clan Match hopper and repeated clan raw15/XP5500 returns15 for all three
members, despite adjacent adjusted60 values. This establishes generic
aggregation of the already-published clan values; it does not turn personal
playlist ranks into clan rank.
[Clan statistic-subject chain](halo2-clan-matchmaking-evidence.md#clan-rank-is-the-statistic-subject-generic-averaging-still-occurs),
[query owner](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1462).

An intentionally inconsistent cached clan fixture `[15,16,16]` still produces15
through generic integer division. That does not recover a retired-service
conflict rule for disagreeing copies of one clan statistic. A replacement
authoritative clan record can provide a coherent snapshot with that trust choice
stated explicitly.

Gathered candidate aggregation `0x7E210` also uses signed integer division of
**raw first-array values**, not its separate adjusted array. A synthetic
monotone adjustment table yields raw `[1,2,8]`, adjusted `[7,7,8]`, and average3.
It does not yield adjusted average7. The candidate description producer reads
that raw aggregate for its advertised average.
[Gathered raw/adjusted owners](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp#L1134),
[description output](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1220),
`original-0007e210.asm`.

The two helpers have different missing-value gates. `0x7E210` excludes guest
flags from the rated count but does not independently skip a nonguest raw NONE
when accumulating its generic average. An intentionally incomplete gathered
fixture `[10,-1,20]` therefore yields9. Searching `0x72540` separately refuses
readiness for the equivalent unavailable nonguest until recovery. Do not use the
gathered arithmetic alone as permission to admit incomplete parties. These
controls establish bounded client behavior, not missing Live service behavior,
native runtime equivalence or a deployment check.

## Native service correction and portable regression fixture

The coordinator now retains the configured XP policy's private raw rank0–127
separately from its capped visible level. Its party search average uses integer
division over those unadjusted ranks. The inferred mixed-party floor remains a
separate native team-balancing operation. Backend-authored snapshots supply
every ordinary queued player's rank and XP; unknown/guest rank recovery is not
silently inferred or enabled by the generic average alone. Clan queues continue
to use their authoritative clan-owned snapshot.

The original runner has been promoted to `tools/oracle_party_search_ranks.py`.
It adds raw `[80,100] → 90`, `[127,126] → 126` and a16-player112–127 average119,
for **21 passing original controls** and the same2,773 executed instructions:

```sh
.tools/python/bin/python tools/oracle_party_search_ranks.py .local/party-search-ranks-oracle
python3 -m unittest multiplayer.master_server.tests.test_matchmaking -v
```

Portable runner SHA-256:
`30c4b0f21fa0c72a445e780ec6b7d9688d994689f5ac9d1f374d0563ec148506`.
Checked-in fixture `master_server/tests/fixtures/retail-party-search-ranks.json`
SHA-256: `9c28c0924bced7b9fbe79778bb82bf71fce5798d029544d55db42606bdaaf496`.
The fixture is produced without importing the replacement implementation.

An authenticated HTTP regression seeds raw80 and100 from the trusted service
store. Both still display level50 in ranked menus; casual menus expose no level.
Both ranked and casual searches retain the distinct private raw values and
reject the otherwise falsely identical candidate pair under their configured
bounds. Neither response exposes raw skill, identity handles or credentials.
Separate tests reject mixed legacy/raw units and verify unadjusted integer
party averages against the available-rank original controls. These tests do not
establish equivalence of the remaining central grouping/host policy or the
missing original service.
