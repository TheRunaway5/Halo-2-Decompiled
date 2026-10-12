# Original ONLINE rank and playlist evidence

Research checked 11 October 2026. This is source research, not an implementation
or a claim that the native ONLINE mode has complete original matchmaking parity.
The exact original rank routines are present locally. Their retail executable
behavior takes precedence over simplified or inconsistent website examples.

## Implementable original rank mechanism

The unchanged retail executable was executed with synthetic inputs. All **30
controls passed**, including equal standings, per-pair truncation, signed
averaging, opponent filtering, team standings, XP bounds, configurable delta
bounds and strict rank hysteresis. Four original named variant presets were
also executed. No original routine or call was stubbed, and no native rank
implementation was imported. The executable SHA-256 was checked before use:
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
[Control script](../../.local/online-rank-research-20261011/oracle_rank_controls.py),
[complete original tables, preset buffers and results](../../.local/online-rank-research-20261011/oracle_rank_controls.json).

These controls establish the **retail executable defaults**, not every dated
Xbox Live configuration. The original online-file record names
`network_configuration.dat` and points its buffer at the configuration structure;
its initializer supplies defaults. A later downloaded configuration is a
separate versioned input. No captured Live configuration file was found in this
research. [Original configuration record and initializer](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L847).

### Pair exchange, ties and integer arithmetic

Original retail `0x746B0` takes opponent skill, own skill, own final standing and
opponent final standing. Actual register arguments are EBX, ESI, EDX and EAX,
respectively. Lower standing numbers win; `-1` is the no-standing sentinel.

1. Own valid standing wins if the opponent has no standing, or own standing is
   strictly smaller. Select the win exchange, own win percentage and own offset.
2. Own standing loses if the opponent is valid and own standing is absent or
   strictly larger. Select the loss exchange, own loss percentage and own offset.
3. Equal standings, including two absent standings, produce **zero exchange**.
4. Index exchange by `own_skill - opponent_skill + 127`. Multiply the signed
   16-bit exchange by the unsigned 8-bit own percentage, divide by 100 with
   **signed truncation toward zero**, then add the signed 16-bit own offset.

The executable uses integer multiplication and a signed divide-by-100 sequence,
not floating-point Elo or a new rating estimator. Offsets are signed shorts in
the executable despite the decompile's configuration declaration grouping their
storage as longs. [Original pair routine](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L340),
[executable controls](../../.local/online-rank-research-20261011/oracle_rank_controls.json).

### Player and clan aggregation

Original retail `0x74800`, stdcall `(collection, selected_player)`, compares the
selected player with each eligible opposing player. Opponents need the same
playlist key, active status, a valid team and skill, and no low-two-bit identity
flags. The selected player and same-team players are omitted. Team games use
team standings; FFA uses player standings. **Tied eligible opponents still count
in the divisor**, with a zero pair contribution.

It divides the signed integer sum by the number of eligible opponents with
`IDIV`, truncating toward zero. It then limits the change to keep resulting XP
between `0` and `0x3FFFFFFF`, and finally clamps the change to the configured
minimum/maximum. The retail defaults are `-200` and `+200`. With no opponents,
the initial sum stays zero. Original `0x74720` applies the analogous aggregation
to clan/team records. [Player aggregate](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L524),
[clan aggregate](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L463).

The result caller `0x741F0` uses player record `+0x98` for pairwise effective
skill, `+0x9A` for the previous rank passed to the hysteresis routine, and `+0x9C`
for XP. Preserve that separation when mixed-skill parties receive adjusted
skills; never overwrite the persistent rank with a matchmaking adjustment.
It builds the stat updates subsequently submitted through arbitration.
[Result caller](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1300),
[report caller](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1695),
[original arbitration wrapper](../../tools/reference/halo2-decompiled/src/unknown_092220.cpp#L166).

### Rank thresholds and hysteresis

Original `0x74970` takes XP in ESI and previous internal rank in EDI. It walks
configuration thresholds starting at index 1. If XP lies below threshold `i`,
the ordinary candidate is `i-1`. It retains the previous rank only when that
candidate equals `previous-1` **and** XP is strictly greater than the integer
midpoint of the candidate's lower and upper thresholds. At the midpoint itself,
it demotes. If XP crosses more than one rank, that one-step retention condition
does not apply. This is not half the total XP required for the promoted rank.
[Original hysteresis](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L872).

Example with original raw indices: previous 10, thresholds 2500/3000 for
candidate 9. XP 2751 retains 10; XP 2750 returns 9; XP 2499 returns 8. XP 3000
returns 10 without needing retention. These are executed original controls.

The default initializer `0x66330` produced these **raw threshold entries**:

```text
index 0..9:   0,100,200,400,600,900,1200,1600,2000,2500
index 10..19: 3000,3500,4000,4500,5000,5500,6000,6500,7000,7500
index 20..29: 8000,8500,9000,9500,10000,10500,11000,11500,12000,12500
index 30..39: 13000,13500,14000,14500,15000,15500,16000,16500,17000,17500
index 40..49: 18100,18900,20000,21500,23500,26100,29400,33500,38500,44500
index 50..127: 0x3FFFFFFF
```

These are internal indices, **not a verified display conversion**. The original
routine returns 0 at XP 0–99 and 49 at XP 44500; the contemporary website labels
those bands 1 and 50. Its factor tables sometimes use a level-0 row. Trace the
retail UI's presentation before imposing a universal `+1` conversion on every
rank input. [Initializer](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L697),
[Bungie launch-era overview, captured 9 November 2004](https://web.archive.org/web/20041109084613/http://www.bungie.net/Stats/page.aspx?section=FAQInfo&subsection=stats&page=statoverview).

### Retail default exchanges and percentages

Let `d = own_skill - opponent_skill`. For the initialized retail tables,
`win_exchange = clamp(100 - 10*d,20,180)` and
`loss_exchange = -clamp(100 + 10*d,20,180)` for valid skill indices 0–127.
Use the extracted tables when restoring a downloaded configuration rather than
assuming that expression applies to later service eras. All retail additive
offsets are zero. [Original table initialization](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L649).

```text
Own raw skill:    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15..127
Loss percentage: 0,5,10,20,35,45,55,60,65,70,75,80,85,90,95,100
Own raw skill:   0..40,41,42,43,44,45,46,47,48,49,50,51..127
Win percentage: 100,95,90,85,80,70,65,60,55,50,50,0
```

[Original percentage initialization](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp#L728).
The raw-domain behavior outside the public 1–50 range is part of the extracted
configuration, not a reason to admit invalid public rank values.

## Published later-era data and contradictions

The [Bungie overview captured 23 May 2006](https://web.archive.org/web/20060523122523/http://www.bungie.net/Stats/page.aspx?section=FAQInfo&subsection=stats&page=statoverview)
and [page dated 30 January 2007, captured 2 December 2014](https://web.archive.org/web/20141202005503/http://halo.bungie.net/Stats/Content.aspx?link=h2statoverview)
publish a different threshold scale. Minimum XP for displayed levels 1–50 is:

```text
1,100,200,300,400,500,600,700,800,900,
1000,1100,1200,1400,1600,1800,2000,2250,2500,2750,
3000,3250,3500,3750,4000,4250,4500,4750,5000,5250,
5500,5750,6000,6250,6500,6750,7000,7250,7500,7750,
8000,8250,8500,8750,9000,9250,9500,9750,10000,10250
```

Later higher-skill-win/lower-skill-loss magnitudes by absolute difference 0–15+
are `100,92,85,79,74,70,66,63,60,58,56,54,53,52,51,50`; the complementary
lower-skill-win/higher-skill-loss values are `200 - magnitude`. The later win
factor is 100% through published level 41, then
`95,90,85,80,70,65,60,55,50` for levels 42–50. These are documented facts,
not recovered downloaded configuration bytes.

Do not silently combine the later tables with retail defaults or repair these
source problems:

- The later loss table repeats **level 10** at both 40% and 50%, and **level 11**
  at both 45% and 55%. Other published loss rows are unambiguous: levels 1–9
  `0,2.5,5,7.5,10,15,20,27.5,35`; levels 12–28 start at57.5% and increase by
  2.5% each level; 29+ is100%. Exact rows10/11 require configuration evidence.
- Its unchanged explanation says mitigation ends at15, contradicting its own
  later table. Its summary also disagrees about whether zero-sum play starts
  at29,30 or31. The actual table, arithmetic and version must govern.
- Website examples round or simplify differently from the original executable.
  The launch-era four-player example gives `+67,+49,-18,-30`; running its raw
  skills10,7,8,5 and standings0,1,2,3 in the retail routine gives
  **`+66,+49,-17,-29`**. An unmitigated +10-skill control gives
  **`+66,+40,-40,-66`**, from signed integer averaging.
- The earlier published equal-level row puts110 in the lower-win column while
  the executable's equal-skill win table gives100. A tied *skill* comparison has
  no distinct lower-skill player. Equal *standing* is separately zero in code.

[Bungie's 15 April 2005 update](https://halo.bungie.org/bwu/index.html?item=21)
announces changes to advancement, high-rank matching and high-gap losses. That
confirms an era change; it does not provide a replacement configuration blob.

Bungie's overview also documents final-standing treatment of departures: teams
with any finisher rank above teams whose members all departed; scores order
within those groups. A departing player receives the team's result if teammates
finish. Party size adds no XP bonus, and guests cannot enter ranked playlists.
These service/game-result requirements have not all been exercised by the
synthetic calculation controls above.

## Dated actual playlist catalog

The [original Bungie catalog captured 6 April 2005](https://web.archive.org/web/20050406005054/http://www.bungie.net/Stats/page.aspx?section=FAQInfo&subsection=playlists&page=1)
labels its rules **updated 30 November 2004**. Preserve that source label and
capture separately; it is not a September 2005 catalog merely because an
approximate archive request happened to retrieve it.

Head to Head is ranked, exactly two players, FFA, solo parties only. Its original
entry order is:

| Order | Variant | Map |
|---:|---|---|
| 1 | Slayer Duel | Lockout |
| 2 | Slayer Duel | Ascension |
| 3 | Slayer Duel | Beaver Creek |
| 4 | Slayer Duel | Midship |
| 5 | Slayer Duel | Foundation |
| 6 | Rifle Duel | Lockout |
| 7 | Sword Duel | Lockout |
| 8 | Sword Duel | Midship |
| 9 | Sword Duel | Beaver Creek |

Team Slayer is ranked, 6–8 players, two teams, parties1–4; significant parties3–4
require equal significant-party size across teams. Its original ordered pool is:

```text
Team Slayer: Burial Mounds, Ivory Tower, Ascension, Colossus,
             Beaver Creek, Zanzibar, Midship, Foundation, Coagulation
Fiesta Team Slayer: Ascension, Beaver Creek
Team Rockets: Ivory Tower
Team Swords: Lockout, Beaver Creek
Team Shotguns: Midship
Team Phantoms: Lockout, Midship
```

Each comma-separated map is its own entry; the order above preserves all17 rows.
The catalog explicitly orders entries by relative weight, with ordinary Slayer
more likely than novelty variants, but publishes **no numeric weights**. Equal
weights, removing unsupported variants or reducing the pool to Lockout cannot
be called the original complete playlist. They are explicit subset/policy
choices until actual title-managed playlist data are recovered.

The same catalog identifies casual Team Training as unranked, 6–8 players, two
teams, guests allowed, using Team Skirmish plus Team Slayer. This includes
objective modes. Rumble Training says it uses the Rumble Pit games, yet another
line says Team Skirmish plus Team Slayer. Keep this original internal
contradiction unresolved; it does not authorize an invented casual rotation.

A second [catalog captured 12 December 2005](https://web.archive.org/web/20051212093137/http://www.bungie.net/Stats/page.aspx?section=FAQInfo&subsection=playlists&page=1)
is labeled **updated 2 December 2005**, has14 playlists and omits Head to Head.
It includes a different Team Slayer pool and casual training additions. It must
remain a separate revision. Bungie's [12 July 2005 update](https://www.bungie.net/en/Forums/Post/174550)
also changes ownership gates and Foundation weighting, proving that a single
undated synthetic pool cannot represent all original service operation.

## Original named presets and settings boundary

The original executable's `0x212F40` table contains **Slayer Duel0, Team Slayer2,
Rifle Duel7 and Sword Duel9**. It takes EDI=preset and ESI=variant buffer, calls
original `0x19D220`, copies the authored name and applies literal overrides.
All four complete 0x130-byte output buffers were captured with unchanged code.
[Source preset table and routine](../../tools/reference/halo2-decompiled/src/unknown_215b40.cpp#L380),
[original output buffers](../../.local/online-rank-research-20261011/oracle_rank_controls.json).

Slayer Duel has source engine2, score10, time480 and flags0xF9A. Team Slayer has
engine2, score25, time480 and flags0xF9B. Rifle/Sword Duel both use score10 and
have additional vehicle/weapon/shield overrides captured byte-for-byte. A generic
25-point Slayer preset therefore does not reproduce Slayer Duel.

These are executable **built-in presets**, not proof that a dated downloaded
playlist added no further overrides. Bungie's [December 2004 playlist announcement](https://bnetarchive.haloman30.com/News/Blog7d7a-2.html?dfilter=date012004)
states that playlist updates live in title-managed storage and includes a
specific change removing the Banshee from Head to Head Ascension. This is evidence
of map-specific playlist configuration beyond a name or generic default.

Native gameplay currently admits a limited set of Slayer rules. Preset execution
does not establish complete weapon behavior, every map's starting-weapon data,
objective modes, authenticated result arbitration, Internet play or complete
ranked/casual matchmaking. Do not publish a historical playlist until its whole
pool and settings are admitted, or give an explicitly restricted subset a
separate name and provenance.

## Reproduction and remaining evidence gates

Run from the repository root:

```sh
.tools/python/bin/python .local/online-rank-research-20261011/oracle_rank_controls.py \
  > .local/online-rank-research-20261011/oracle_rank_controls.json
```

Original HTML captures are stored beside those controls as
`bungie-stats-20041109084613.html`, `bungie-stats-20050407192947.html`,
`bungie-stats-20060523122523.html`, `bungie-stats-20141202005503.html`,
`bungie-playlists-20050406005054.html` and
`bungie-playlists-20051212093137.html`. Fetching used the archive's `id_` URLs to
read unmodified original HTML. Search/forum/wiki summaries were discovery aids;
the original archived page, local decompile and unchanged executable own the
claims above.

Remaining gates are the exact intended service era; downloaded network/rank
configuration; presentation of raw rank indices; complete dated TMS map/variant
settings and numeric weights; title-update comparison of these routines; final
standing construction for every mode/departure; trusted multi-console result
arbitration; and full admitted gameplay across each published pool. The retail
calculation mechanism is now recoverable without guessing, while those missing
inputs stay explicit.

## Follow-up: raw rank, displayed level and result-field provenance

This section resolves the presentation gate above from the **original Xbox
assets and unchanged retail instructions**. It supersedes only the earlier
statement that the raw-to-display conversion was unverified. The service-era,
downloaded-configuration and complete-playlist gates remain.

### Stored 0–49 represents displayed 1–50

The original `mainmenu.map` `wigl+0x158` block has exactly 50 signed-short
inclusive ranges: entries 0–48 are `[i,i]`; entry 49 is `[49,127]`. Original
`0x149EAD` searches these ranges without adding one. The roster producer
`0x25042D` passes player configuration byte `+0x7E` to `0x2B01A2`, which calls
that lookup. The original skin consumer selects the resulting `rank_icons_sm`
bitmap frame. Raw `-1` matches no range and hides the rank image. Raw 0 selects
frame 0; raw 48 selects frame 48; all raw 49–127 select frame 49.
[Range lookup](../../tools/reference/halo2-decompiled/src/unknown_1490ec.cpp#L359),
[item adapter](../../tools/reference/halo2-decompiled/src/unknown_2b01a2.cpp#L13),
[roster caller](../../tools/reference/halo2-decompiled/src/unknown_250155.cpp#L2179).

The literal Xbox `ui\global_bitmaps\rank_icons` frame 0 visibly prints **1**,
frame 1 prints **2**, and frames 0–42 print 1–43. Frames 43–49 contain the
original moon/eclipse/sun/comet/halo artwork for the top seven levels, rather
than printed numbers. Both original banks have 50 pages: large `rank_icons`
datum `E4FA02DA`, 28×26; small `rank_icons_sm` datum `E4E202C2`, 17×17. They use
format 11, flags `0x290`, linear BGRA rows with a 128-byte pitch. All **100**
decoded original Xbox pages are byte-identical in RGBA pixels to the supplied
Vista donor pages. This comparison checks actual pixels, not filenames or
matching dimensions. The Xbox map SHA-256 is
`1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
[Asset audit and per-page hashes](../../.local/online-rank-research-20261011/rank-asset-audit.json),
[large original page contact sheet](../../.local/online-rank-research-20261011/xbox-rank_icons-frames.png),
[small original page contact sheet](../../.local/online-rank-research-20261011/xbox-rank_icons_sm-frames.png).

Therefore a public numeric level **L in 1–50 corresponds to stored raw rank
L−1**. Convert public numeric inputs at their boundary; use the raw value for
the threshold lookup, percentage tables, playlist rank ceilings, persisted
rank and result arithmetic. Convert a valid raw public rank back with `+1`
when numeric level text is required; select the original artwork by the raw
frame/range. Preserve `-1` as unavailable. Do not subtract one again from values
read from original stat fields or session summaries. The original UI's final
range groups 49–127; that grouping does not certify public numeric levels above
50 or prove their service admissibility.

This resolves the earlier examples: XP 0–99 returns raw 0 and displays level 1;
XP 100 returns raw 1 and displays level 2; XP 44500 returns raw 49 and displays
the level-50 halo image. The original threshold/factor entries remain indexed
exactly as extracted above. A website's printed level labels do not shift the
configuration arrays inside the executable.

### Original stat, launch and retained result fields

Original `0x65530` obtains cached playlist **stat field 1** via `0x73A10` and
copies it verbatim to UI configuration byte `+0x7E`. For the live playlist it
copies stat field 0 to configuration XP `+0x8C`, field 1 to previous raw rank
`+0x8A`, and sets effective skill `+0x88` to `-1` pending matchmaking. The
maximum rank across available ranked playlists goes to byte `+0x7F`. None of
these stores add one. Original `0x73A10` returns the cached integer itself.
[Property writer](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L1412),
[stat getter](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L840).

Three full unchanged `0x65530 → 0x73A10` controls supply cached raw ranks
0, 1 and 49 with XP 0, 100 and 44500. They verify that the UI rank, maximum
rank and previous rank all remain exactly 0, 1 and 49, respectively; effective
skill is `-1` and XP remains unchanged. The fixture sets an excluded identity
flag to skip the unrelated recent-player cache write after these stores. It
does not replace the stat getter or the property producer.
[Executed property controls](../../.local/online-rank-research-20261011/oracle_rank_provenance.json).

Original `0x71FF0` supplies the matchmaking team's chosen properties: playlist
key `+0x84`, effective skill `+0x88`, previous raw rank `+0x8A`, XP `+0x8C`.
The real launch producer `0x6E910` copies the full 144-byte configuration from
network player `+0xA8` to launch player `+0x1C`. It then explicitly writes the
playlist key to launch `+0xA0`, summary XP `+0x288+4*i` to launch `+0xA8`, raw
summary rank `+0x248+4*i` to launch `+0xA6`, and the adjusted rank returned by
`0x63220` to launch `+0xA4`. These are separate stores, with no display
conversion. [Matchmaking properties](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1742),
[launch writer](../../tools/reference/halo2-decompiled/src/network_session_client.cpp#L78).

| Meaning | Configuration | Launch player | Game-player settings | Retained result player |
|---|---:|---:|---:|---:|
| Playlist key | +84 | +A0 | +C8 | +94 |
| Effective skill for XP comparison | +88 | +A4 | +CC | +98 |
| Previous persistent raw rank | +8A | +A6 | +CE | +9A |
| XP | +8C | +A8 | +D0 | +9C |

The game-player configuration begins at `+0x44`. Original `0x196470` copies
that complete 144-byte block verbatim to retained player `+0x10` when adding a
player's record. The retained array begins at collection `+0x384`, stride
`0xA4`, which is exactly the array consumed by `0x74800` and `0x741F0`.
The unchanged-copy control verifies the entire block, including distinct
effective and previous ranks. The separate session-options-to-game-player
allocation/assignment caller was not executed in this fixture; its unexecuted
boundary does not change the observed launch stores or retained-copy offsets.
[Retained record source](../../tools/reference/halo2-decompiled/src/unknown_1967d0.cpp#L990),
[copy routine](../../tools/reference/halo2-decompiled/src/unknown_1967d0.cpp#L1055).

The full original `0x741F0 → 0x73E00` result-batch control supplies effective
raw skill **15**, previous raw rank **0**, XP **0**, and a win over effective
raw skill 15. It produces **+100 XP** and writes new lower/upper raw rank **1**
in the update attributes, corresponding to displayed level 2. It does not use
previous raw rank 0 for the pairwise percentage/exchange, nor write displayed
2 as the persistent rank. This proves the result caller's field separation
through actual batch construction; final `XOnline` submission remains outside
the fixture. [Original result caller](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1300),
[original batch builder](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1065),
[executed results](../../.local/online-rank-research-20261011/oracle_rank_provenance.json).

### Party adjustment and executable-specific branch

For a non-absent raw summary rank, original `0x63220` obtains the highest raw
rank on the player's **machine** from summary machine associations. Its
`0x1931A0(highest, playlist)` walks backward from `highest` while the signed
byte `playlist+0x51C+[index−1]` is at least that highest rank. The effective
skill is the maximum of the player's raw rank and that returned index. No
numeric display conversion occurs. This machine association is the literal
scope of this helper; do not describe it as arbitrary accepted-party membership
without tracing the grouping producer. [Machine maximum](../../tools/reference/halo2-decompiled/src/network_session_membership.cpp#L305),
[summary adjustment](../../tools/reference/halo2-decompiled/src/network_session_membership.cpp#L669),
[ceiling inverse](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp#L236).

The retail instructions at `6323C..63242` reload and test the **playlist index**,
not the pointer returned by `192E60`: index 0 skips this adjustment. The
decompiled C++ currently tests `variant` there and differs from the executable.
Full unchanged launch controls verify index 0 leaves raw 0 effective 0, whereas
index 1 with an explicit **synthetic** ceiling table `min(i+5,127)` and a
same-machine raw-20 player raises raw 0/1 to effective 15 while preserving
previous raw 0/1 and XP. That table is a distinguishing fixture, **not a
recovered historical playlist rule**. Preserve the original branch when
targeting these exact retail routines; title-update/service-era behavior must
be established separately. [Executed instruction listing](../../.local/online-rank-research-20261011/rank-provenance-executed.txt),
[launch controls](../../.local/online-rank-research-20261011/oracle_rank_provenance.json).

### Reproduction and precise remaining limits

The follow-up has **150 passing controls**: 130 direct range inputs (`-1`, all
0–127 and 128), nine full roster cases, six complete launch cases, three full
cached-stat property cases, one exact 144-byte retained-copy case, and one
complete result-batch case. All executable
instructions remain original. Only the UI-global pointer accessor `148350` is
a seam for the relocated literal UI block; the roster fixture retains its
declared session/voice/text/widget-allocation inputs. The launch, retained copy
and result batch execute complete original routines with no replaced calls.

```sh
.tools/python/bin/python .local/online-rank-research-20261011/oracle_rank_provenance.py \
  > .local/online-rank-research-20261011/oracle_rank_provenance.json
python3 .local/online-rank-research-20261011/audit_rank_assets.py
```

The result-batch fixture uses inactive cached-user groups, so it makes no claim
about authenticated cached-user refresh, platform-clock calls or arbitration
submission. Session-options-to-game-player assignment, title-update comparison,
Live transport and final GPU presentation were not exercised. The raw/display
conversion and the distinct XP fields are resolved; the earlier downloaded
configuration, service-era and playlist-content gates remain unresolved.
