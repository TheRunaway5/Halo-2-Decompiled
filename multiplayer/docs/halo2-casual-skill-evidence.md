# Original Halo 2 hidden casual skill

Research checked 11 October 2026. This supplements
[the matchmaking evidence](halo2-matchmaking-evidence.md) and
[the recovered rank mechanism](online-rank-playlist-evidence.md).
It establishes a supported **retail-default hidden-rank update path**, rather
than treating original casual skill updates as entirely unknown. Exact later
Xbox Live configuration bytes and complete historical casual rotations remain
unrecovered. No product code was changed by this research.

## Findings

Original Halo 2 keeps a playlist's skill/XP separately from whether its rank is
publicly displayed. The retail result producer applies the same player exchange
and rank routines to hidden and visible player hoppers. Its public-property
producer withholds hidden hopper ranks while still carrying their current
playlist XP and raw rank. These are executed original instructions, not an
inference from the word "unranked". [Executed controls](../../.local/casual-skill-research-20261011/oracle_casual_dispatch.json),
[original result producer](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1301),
[original public-property producer](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L1413).

There is also a concrete casual search difference: after reaching its configured
bounded expansion, an unranked search can make a further query with **both rank
bounds removed**. Ranked and clan searches do not take that branch. Composite
candidate-quality scoring remains active, including its rank-difference term. It would therefore be incorrect to reproduce
original casual matching merely by hiding an otherwise permanently bounded
ranked search. [Original retry](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L754),
[query construction](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L197),
[executed query-boundary controls](../../.local/casual-skill-research-20261011/oracle_casual_dispatch.json).

## First-person and contemporary published evidence

Hoberman's direct interview statement: “even the unranked playlist tracked and
matched you based on your skill.” [2 December 2020 interview](https://www.vice.com/en/article/why-players-blame-skill-based-matchmaking-for-losing-in-call-of-duty/).

Bungie's 2 December 2005 Weekly Update announces the transition of Team Snipers
to ranked play: its existing hidden rank would become visible, and no stats
were wiped. That is contemporary evidence of persistent skill in an unranked
playlist and continuity when its visibility classification changed. It does
not publish that playlist's numeric rating configuration. The original Bungie
URL now returns 404; HBO preserves the attributed Bungie update and original
link. [Bungie Weekly Update, preserved copy](https://www.halo.bungie.org/bwu/index.html?item=52),
[original publication URL](https://www.bungie.net/News/content.aspx?story=weeklyupdatedec02&type=topnews).

Chris Butcher and Barton House's original GDC 2005 presentation describes:

| Primary slide | What it supports |
|---|---|
| [29: matchmaking system](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=29) | Separate ranks in each hopper, newcomer rank 1, party rank from member averages, rank as a principal matching input, automatic team formation. |
| [30: results](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=30) | Each console reports results and the service arbitrates them; clients' asserted ratings/results are not authoritative by themselves. |
| [38: party search](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=38) | Query by hopper/rank range; evaluate skill match, available space, host quality, wait, latency and language. |
| [48: improvements](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=48) | Rank eligibility and a quality threshold precede the remaining random choice. It is an account of the implementation's shortcomings, not a published coefficient table. |

All four slide images were fetched from their original presentation archive and
visually inspected. They are preserved under
[the research artifacts](../../.local/casual-skill-research-20261011/).
The slides do not establish separate numeric casual exchange tables or an exact
casual retry cadence.

Hoberman's own 20 November 2023 posts describe intentional variety in ranked
opposition and criticize the repeated demand for perfectly even games. His
unranked-design post advocates enabling players across skill levels to enjoy
casual play together. These statements support avoiding a forced 50/50 outcome
schedule; they do not specify proportions, performance manipulation, or new
casual rating coefficients. [Variety](https://x.com/MaxHoberman/status/1726560295743824255),
[ranked mixture](https://x.com/MaxHoberman/status/1726560304916754613),
[unranked design](https://x.com/MaxHoberman/status/1726560313120841741).
The X pages did not render through the research browser. X's own public
syndication endpoint returned each post's matching ID, author, full text and
UTC timestamp; the source JSON is preserved in the research artifact directory.
For reproduction, use
`https://cdn.syndication.twimg.com/tweet-result?id=POST_ID&lang=en&token=1`.

## Original client ownership and gates

The following retail functions were checked against the unchanged original
`assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Complete function spans and their byte hashes are included in the control
report; the corresponding disassemblies are preserved alongside it.

1. **Classification and validation.** Hopper types 1/3 use the hidden player
   class; types 2/4 use the displayed player class; type 5 is clan. Retail
   `0x193470` returns true for 2/4/5, and `0x193490` for 1/3. This classification
   is independently visible in `0x65530`'s display stores and `0x66050`'s guest
   admission gate. Generic hopper validation accepts all five types, valid
   dimensions and a 128-entry rank-range table. A hidden hopper does not fail
   validation simply because its rank is hidden.
   [Classification](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp#L213),
   [validation](../../tools/reference/halo2-decompiled/src/unknown_193560.cpp#L33),
   [display/guest callers](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L1413).
2. **Stats reads.** Retail `0x74AA0` queues six statistics for every available
   hopper. Only type 5 additionally requires clan identity. The statistics
   namespace is the hopper's `field_8`; it is not the visibility type. This
   provides a source-supported way to retain a playlist's statistics while
   changing whether its rank is displayed.
   [Read/cache owner](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1457).
3. **Reporting caller.** Retail `0x73CA0` validates the hopper, the round-key
   fields and the local machine's presence. For a valid result, it calls
   `0x741F0` without a ranked-only gate and submits the generated operations
   through `online_round_report`. The caller was checked statically from
   original bytes; final Live submission was not emulated.
   [Caller](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1696).
4. **XP/level producer.** Retail `0x741F0` branches only on type 5 versus every
   other valid hopper. Types 1–4 all use player aggregation `0x74800`, then
   hysteresis `0x74970`; type 5 uses clan aggregation `0x74720`. Active players
   still need a valid team, proper identity flags, matching playlist key and
   initialized XP/raw rank. Hidden status does not skip those updates.
   [Result producer](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1301).
5. **Numeric configuration.** The pair exchange `0x746B0` reads one global
   `g_network_configuration` table set. It receives skill and standings, with
   no ranked/casual parameter or per-hopper coefficient branch. Retail
   `0x66330` initializes those tables; the online-file owner exposes the same
   structure to `network_configuration.dat`. Thus recovered retail defaults
   support both hidden and displayed hoppers. A subsequently downloaded
   configuration can replace them and must remain a separate version.
   [Pair exchange](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L340),
   [default tables and file owner](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L649).
6. **Display is separate.** Retail `0x65530` reads display levels only for 2/4/5,
   leaving 1/3 absent from the displayed maximum and selected-playlist level.
   Its following current-hopper branch still loads XP and raw rank for any
   valid hopper. Hiding the public value must not zero private skill or stop
   updating it.
   [Public-property owner](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L1413).

The XP in these routines is the original per-playlist skill accumulator.
Skill/raw rank, prior rank for hysteresis and accumulated XP occupy separate
fields. Preserve that separation instead of substituting games played or a
new rating estimator. Raw rank versus displayed badge conversion remains
governed by the original UI analysis in the rank evidence document.

## Casual widening and the unrestricted pass

Retail `0x90880` stores `flag14 = type==2 || type==4 || type==5`. It derives
bounded search limits from the hopper's rank-range table and initializes the
expansion counter to 0. `0x90F70` increments that counter until its configured
maximum. When the maximum has been reached, only a search with `flag14==false`
sets the counter to `NONE` and retries. `0x90B30` treats `NONE` as an explicit
instruction to disable the minimum and maximum rank filters. It retains the
ordinary minimum composite candidate-quality score from the network
configuration. Retail `0x91000` still computes its rank-difference contribution
to the composite score, and `0x91400` requires that score to meet the retained
minimum. Removing rank eligibility bounds therefore does not remove all skill
preference. The value 100 seen in the control is a score, not a latency or
standalone QoS measurement.
[Candidate scoring](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L593),
[candidate selection](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L663).
[Initialization](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L821),
[retry](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L754),
[query](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L197).

This is a **fallback after unsuccessful bounded searching**, not evidence that
unranked games always ignore skill. The candidate budget, full 50-result batches,
QoS completion, joining failures and subsequent searching/gathering states also
affect continuation. A central replacement coordinator does not have those
original service-query rounds, so mapping this rule onto elapsed time requires
an explicitly declared native policy. The complete historical timing is not
proved by these controls.

These controls prove **one searching party's outgoing query**. They do not
execute a prospective gathering party's admission decision or prove that both
parties simultaneously remove their own bounds. A central coordinator that
requires reciprocal eligibility or waits for both parties to reach the final
stage adds an explicitly declared native policy; that symmetric rule is not
recovered from this asymmetric client query branch.

## Executed checks and limits

Run:

```sh
.tools/python/bin/python tools/oracle_online_casual.py \
  .local/casual-skill-oracle > .local/casual-skill-controls.json
```

All 12 controls passed:

The source-controlled [portable oracle](../../tools/oracle_online_casual.py)
was independently rerun and reproduces every research control value, function
span hash and executed-instruction hash. Its tool-file hash differs from the
initial research script because its root and output paths are portable.

| Controls | Observed original behavior |
|---|---|
| 4 result-production controls, types 1–4 | Full `0x741F0` and `0x73E00` produce byte-identical batches for the same synthetic standings/skills: 42 operations, +100/-100 exchange, the same XP/level changes. |
| 4 public-property controls, types 1–4 | Cached XP 5500/raw rank 15 remains available in all four classes. Public rank is absent for 1/3 and present for 2/4. |
| 4 search controls | Before the synthetic maximum, hidden and displayed searches both expand bounded filters. At the maximum, hidden search retries without either rank filter and retains quality threshold 100; displayed search ends that search round. |

The synthetic widening maximum 3 is a fixture input, **not a recovered original
playlist coefficient**. Search execution stops at the real `online_match_search`
wrapper before invoking the unavailable Live service. No original executable
instruction or call is replaced. The public-property fixture uses an excluded
identity sentinel solely to bypass an unrelated recent-player-cache write after
the observed stores. Results use separate eligible synthetic identities.
There are no Discord credentials or real user identities in these artifacts.
[Control source](../../.local/casual-skill-research-20261011/oracle_casual_dispatch.py),
[report](../../.local/casual-skill-research-20261011/oracle_casual_dispatch.json),
[executed original instructions](../../.local/casual-skill-research-20261011/retail-casual-executed.txt).

## Supported implementation path and remaining uncertainty

A native casual playlist can explicitly select the recovered
`retail-launch-2004` skill policy, update private per-playlist XP/raw rank from
validated results through the same recovered player arithmetic, and keep public
level/XP absent from casual UI/API results. Keeping the same stable playlist
statistics key when only its displayed/unranked classification changes is
supported by both the client seam and the documented Team Snipers transition.
These are recovery-backed rules, not new coefficients.

Release20261011T230633Z deploys those hidden updates for the casual Slayer and
casual Team Slayer lab playlists. Real HTTP/WebSocket tests verify unanimous
commit, duplicate-confirmation safety, changed skill in the next search,
independent ranked skill, persistence and retention when the same playlist
becomes visible. Two native casual clients and four native clients in accepted
parties completed actual gameplay, voice, postgame and requeue. External Discord
identity endpoints were fixtures; physical WAN validation remains open.

A native search policy can also support the recovered final unrestricted-rank
pass while retaining eligibility, whole-party/team constraints and measured
connection checks. Its central-service timing must be named and versioned as
native tuning until original query cadence and downloaded rank-range data are
recovered. The shared XP algorithm does not justify reusing every ranked
selection limit in casual play.

The deployed lab final stage removes the rank eligibility limit after30seconds;
that elapsed timer and reciprocal interpretation are explicitly native tuning.
It preserves whole-party, compatibility, map and measured-connection constraints.
The original composite candidate-quality formula remains a separate gap; the
native QoS score is not presented as that recovered formula.

Still unresolved: exact downloaded configurations for each historical era;
complete casual playlist map/variant weights/settings; all guest/splitscreen
admission and result behavior; original search/gather cadence under real Xbox
Live responses; dated candidate scoring/host-quality configuration; and
end-to-end original-service arbitration. Retail synthetic execution proves the
bounded mechanisms above, not full historical casual matchmaking or Internet
match parity.
