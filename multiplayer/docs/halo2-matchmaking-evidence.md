# Original Halo 2 matchmaking evidence

Research checked 10 October 2026. The user authorizes best-effort reproduction
from public information when original code is absent. Preserve recovered rules;
identify additional policy choices as approximations. This document describes
selection requirements, not a completed or playable matchmaking service.

## Evidence and version boundaries

The strongest references are Bungie's own contemporary articles and the
developers' GDC presentation. Max Hoberman's later explanations clarify design
intent. A later interview does not establish an undocumented original numeric
coefficient. Original playlists changed during service operation, so each
restored playlist needs a dated ruleset, rather than a mixture presented as one
original release.

Hoberman confirms that original Halo 2 unranked matchmaking also tracked skill;
its rating was hidden. [Hoberman interview, 2 December 2020](https://www.vice.com/en/article/why-players-blame-skill-based-matchmaking-for-losing-in-call-of-duty/)

Hoberman describes ranked play as intentionally allowing varied difficulty,
including easier opponents, harder opponents and close matches. These posts
give no fixed percentages, forced sequence, per-player win/loss quota, or
engagement-based outcome manipulation. Do not invent a one-third schedule.
[Variability, 20 November 2023](https://x.com/MaxHoberman/status/1726560295743824255),
[ranked mix](https://x.com/MaxHoberman/status/1726560304916754613)

He also advocates allowing different skill levels to play together in casual
unranked games. This is compatible with broad skill matching; it does not prove
that original casual matchmaking ignored skill.
[Hoberman, 20 November 2023](https://x.com/MaxHoberman/status/1726560313120841741)

The X pages were inaccessible to the research browser. Their text, author,
post ID and UTC timestamp were verified using X's own public syndication JSON
endpoint: `https://cdn.syndication.twimg.com/tweet-result?id=POST_ID&lang=en&token=1`.
News articles were used to locate the posts, not as the authority for these
requirements.

## Party and candidate selection

Chris Butcher and Barton House's **Recreating the LAN Party Online**, GDC 2005,
establishes these mechanisms:

| Mechanism | Primary slide |
|---|---|
| Match parties inside a selected hopper; weighted playlists and party composition restrictions | [28](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=28) |
| Each hopper has independent ranks; newcomers start at 1; party rank averages member ranks | [29](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=29) |
| Candidate scoring considers rank, required space, host quality, waiting time, latency and language | [38](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=38) |
| Search uses probe/full QoS, requires every party member to join, and widens rank range until timeout | [39](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=39) |
| Gather uses a bounded rank-prioritized join queue; waiting extends while progress continues | [40](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=40), [41](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=41) |
| Rank constraint enforced; a rating threshold rejects marginal candidates; remaining choice described as random | [48](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=48) |

The slides were visually checked, not accepted from OCR alone. Slide 48 is a
2005 description of shortcomings, not a complete universal specification.
The slides alone support random selection after eligibility filters but do not
supply exact coefficients. The executable investigation below now owns the
recovered scoring/ordering/selection behavior; its deterministic selector and
the slide wording remain an explicit build/service boundary.

Bungie's matchmaking overview describes automatic candidate evaluation,
retrying failed joins, and automatic host selection. Match quality includes
skill, party size, geography and connectivity. Team formation keeps party
members together. Map and variant selection follows developer criteria for
group size and composition. After configuration, a ten-second countdown starts
the game. The match host cannot end the match for everyone; host departure
triggers migration. Parties, gameplay hosts and their leaders therefore require
separate concepts. Postgame players can invite others into parties and reenter
matchmaking together.
[Bungie Matchmaking Overview, page dated 30 January 2007, archived 19 October 2015](https://web.archive.org/web/20151019104400/http://halo.bungie.net/stats/content.aspx?link=h2matchmaking)

The archived overview was retrieved as original HTML using the archive's `id_`
variant; the ordinary archive URL is supplied above for readers. This service
description does not establish ranked/casual playlist classifications or exact
variant probabilities.

## Published rank and party constraints

Published upward limits are maxima, not widening times.
[Bungie Stats Overview, page dated 30 January 2007, archived 2 December 2014](https://web.archive.org/web/20141202005503/http://halo.bungie.net/Stats/Content.aspx?link=h2statoverview)

| Rank | Extension |
|---|---:|
| 1 | +10 |
| 2 | +9 |
| 3 | +8 |
| 4 | +7 |
| 5 | +6 |
| 6 | +5 |
| 7–16 | +6 |
| 17–26 | +7 |
| 27–29 | +8 |
| 30 | +9 |
| 31 | +10 |
| 32 | +11 |
| 33 | +12 |
| 34 | +13 |
| 35 | +14 |
| **35, repeated in original** | Level 50 |

The duplicate **35 → 36+** correction and reciprocal interval interpretation
require declared approximations.

Documented effective-rank example: **12 + 1 → 12 + 6**. A generalized
highest-member-minus-range floor remains inferred.

Its party constraints:

| Playlist | Minimum significant party | Maximum party | Allowed difference |
|---|---:|---:|---:|
| Rumble Pit | N/A | N/A | N/A |
| Team Skirmish | 3 | 4 | 0 |
| Head to Head | N/A | N/A | N/A |
| Big Team Battle | 3 | 5 | 2 |
| Minor Clan Match | 3 | 4 | 0 |
| Major Clan Match | 6 | 8 | 2 |
| Training Ground | N/A | N/A | N/A |
| Double Team | 2 | 2 | 0 |
| Team Preview | 3 | 4 | 0 |

Difference means significant-party sizes, not team headcount. Skirmish
3-versus-4 is prohibited; pairs can oppose solos. BTB 3/4-versus-5 is permitted.
Classification is absent; average-plus-floor combines eras.

## Local recovered client evidence

`tools/reference/halo2-decompiled/src/online_session_search.cpp`,
`function_90b30` at retail address `0x90b30`, widens minimum/maximum search
values and clamps them to configured bounds. It also interpolates a minimum
composite candidate score using the expansion amount and configured values.
The [candidate controls](halo2-candidate-scoring-evidence.md) now establish its
arithmetic and input owners. Historical downloaded configuration, widening
cadence and missing service policy remain unresolved.

`tools/reference/halo2-decompiled/src/online_matchmaking.cpp` calls
`XOnlineMatchSearch` and parses service results. This is evidence of the client
search seam, not recovered Xbox Live server implementation. The coordinator
should preserve bounded widening without pretending that the reference files
contain the missing service.

## Implementation contract and explicit approximations

1. Keep manual discovery and automatic matchmaking as distinct actions.
2. Queue a party as one unit, in one versioned playlist. Store skill per player
   and playlist, including hidden casual skill. A screen name is presentation;
   verified private identity is the account key.
3. Define eligibility, party compatibility, team size, candidate selection and
   map/variant selection separately. Apply hard constraints before drawing an
   eligible match. Keep parties intact during team assignment.
4. Put initial skill window, expansion increments and timing, connection
   thresholds, reciprocal rank interpretation and high-rank typo correction in
   explicit policy. Label defaults as reconstruction tuning. Retain room for
   different dated casual/ranked configurations; do not claim the ranked table
   establishes every casual coefficient.
   [New unchanged-retail controls](halo2-casual-skill-evidence.md) establish
   shared retail-default XP updates for hidden and visible player hoppers and
   a final casual query without rank bounds. The final query retains composite
   candidate scoring. Native `unbounded_skill: true` is allowed only on a final
   casual search stage; elapsed timing and reciprocal eligibility are declared
   native policy, while measured QoS remains a separate adapter requirement.
5. Treat deterministic team search that minimizes mean-rank difference as a
   reasonable implementation of the documented fairness goal, **not** a
   recovered original team optimizer. Document tie handling and randomization.
6. Load versioned weighted map/variant entries once recovered. Bungie explicitly
   changed map weights and map ownership requirements, so equal weights and
   guessed rotations cannot be labeled original.
   [Bungie playlist update, 12 July 2005](https://www.bungie.net/en/Forums/Post/174550)
7. A proposal must not claim slot reservation, gameplay joining, measured
   connectivity or authoritative ranks until those interfaces exist. Never
   trust raw client-submitted ratings or match results in a public service.
   Original results were reported by each console and arbitrated by the service.
   [GDC slide 30](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=30)

A centralized coordinator is replacement service infrastructure. It can
reproduce matching decisions without recreating the original distributed
searching/gathering implementation. Declare this boundary rather than claiming
protocol compatibility or equivalent timing under real network conditions.

The [candidate-scoring investigation](halo2-candidate-scoring-evidence.md)
now recovers retail-default coefficients, integer party-average rounding,
complete sort/selection behavior and their input owners through238 unchanged
retail controls. Those inputs describe advertised gathering sessions, full QoS
and the original bandwidth estimator; current central ticket age and relay
quality percentages cannot silently replace them. GDC's random-pick wording
and the examined deterministic retail selector remain a build/service boundary.
Historical downloaded coefficients, central-service policy, multi-party
matching, exact team optimization and playlist-era rotations remain unresolved.
The [host-migration investigation](halo2-host-migration-evidence.md) establishes
the missing simulation authority handoff; it does not establish automatic
matchmaking backfill/rejoining policy. The GDC
presentation discusses searching/gathering parties and starting when full or
sufficiently populated after waiting, but does not supply a complete threshold
table. [GDC slides 37](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=37)
and [41](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=41)

Useful validation scenarios include mixed-skill parties, incompatible large
parties, fragmented queues, equal-size teams without splitting parties,
bounded ranked widening, the final casual unrestricted pass, hidden casual ratings,
weighted selection frequencies and cancellation before proposal commitment.
Use reproducible simulation seeds and record the policy version. A queue
simulation validates these rules; it does not prove OG service parity or
playable Internet matches.
