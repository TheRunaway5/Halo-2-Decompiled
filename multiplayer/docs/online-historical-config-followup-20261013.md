# Historical ONLINE configuration follow-up

Research date: 2026-10-13. Target requested by the user: the final Xbox Live era,
with Action Sack restored for party games. This note resolves later executable
ranking defaults and complete retail fallback pools. It does **not** identify a
recovered final-2010 downloaded playlist or network configuration. No gameplay,
server, queue catalog or deployed configuration is changed by this research.

## Newly recovered TU1.5 ranking policy

The genuine supplied U5 English executable initializes a distinct later ranking
policy. This resolves the ambiguous percentage rows in the public later stats
page **for this executable's defaults**; it supersedes the earlier research gate
that no later executable table had been compared. Downloads can still replace
these defaults. The authority is
`/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`,
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
[Complete original controls](../../.local/online-completion-20261013/research/title-update-1.5-defaults.json),
[original initializer stores](../../.local/online-completion-20261013/research/title-update-1.5-defaults-initializer.asm).

Complete unchanged `67400` initializes the configuration. Complete unchanged
`784D0`, `78620`, `78540`, and `78790` execute pair exchange, player aggregation,
clan aggregation, and rank hysteresis. No original call or instruction is
replaced. The tests supply synthetic result records; they import no replacement
rating implementation. All executed instruction bytes are checked against the
pinned XBE. There are **8,284 passing TU1.5 controls**, including every raw public
rank pair 0–49 for win/loss/tie, absent standings, threshold boundaries, signed
averaging, guest/playlist exclusions, team standings, XP clamps, and clan
aggregation. These are bounded calculation controls, not service arbitration or
an authenticated live match.
[Instruction trace](../../.local/online-completion-20261013/research/title-update-1.5-defaults-executed-original.txt),
[31 distinguishing controls and literal tables](../../.local/online-completion-20261013/research/title-update-1.5-rank-original.json).

The versioned policy data is ready for an explicit new configuration ID such as
`title-update-1.5-defaults`; preserve the existing `retail-launch-2004` policy.
[Policy data](../../.local/online-completion-20261013/research/title-update-1.5-rank-policy.json).
Do not relabel this as a captured 2010 service configuration or migrate existing
nonzero player XP without an explicit rule.

Exact raw XP thresholds 0–49 are:

```text
0,100,200,300,400,500,600,700,800,900,
1000,1100,1200,1400,1600,1800,2000,2250,2500,2750,
3000,3250,3500,3750,4000,4250,4500,4750,5000,5250,
5500,5750,6000,6250,6500,6750,7000,7250,7500,7750,
8000,8250,8500,8750,9000,9250,9500,9750,10000,10250
```

Entries 50–127 remain `0x3FFFFFFF`. Raw0 is displayed level1 under the original
asset ranges established in [the prior rank evidence](online-rank-playlist-evidence.md).
The original initialized first entry is zero, not the public website's printed
one-XP minimum. [Initializer `67B77`–`67C31`](../../.local/online-completion-20261013/research/title-update-1.5-defaults-initializer.asm).

Exact integer loss percentages at raw0–29 are:

```text
0,2,5,7,10,15,20,27,35,40,45,50,55,57,60,
62,65,67,70,72,75,77,80,82,85,87,90,92,95,97
```

Raw30–127 is100%. The source stores bytes: there is no2.5% arithmetic. In public
numeric terms, level10/raw9 uses40%, level11/raw10 uses45%, and level31/raw30
is the first unmitigated row. The public later table's duplicate level10/11
rows and summary disagreement are therefore unnecessary inputs for this policy.
[Literal byte stores `67C33`–`67D16`](../../.local/online-completion-20261013/research/title-update-1.5-defaults-initializer.asm),
[prior published contradictions](online-rank-playlist-evidence.md).

The high-skill-win magnitude for raw difference0–15+ is exactly:
`100,92,85,79,74,70,66,63,60,58,56,54,53,52,51,50`.
The opposite direction uses200 minus that magnitude. Loss exchange is the
negative complementary direction. Own win factors remain100% through raw40,
then95,90,85,80,70,65,60,55,50,50 for raw41–50; raw51+ has0%. All additive
offsets are zero and delta bounds are−200/+200. Signed division and the strict
one-rank demotion midpoint remain in original code.
[Exact tables and addresses](../../.local/online-completion-20261013/research/title-update-1.5-defaults.json).

For example, raw15 losing to raw15 gives−62; raw41 defeating raw40 gives87.
For previous raw13, XP1301 retains13 while XP1300 demotes to12. In a team game,
team standing still governs each member's exchange. These are executed original
outputs, not rounded web examples.
[Compact controls](../../.local/online-completion-20261013/research/title-update-1.5-rank-original.json).

The same complete calculation controls were executed in retail and the supplied
U1–U5 English executables. Retail/U1/U2/U3 share the launch table bytes. U4/U5
share the later table bytes; the normalized later table digest is
`4cc9e51eb2ff12dbca19f355a474ecff73123acf4ec88a7086e5b3803ae71008`.
This is executable-version evidence, not proof of a download's deployment date.
[Six-version hashes and controls](../../.local/online-completion-20261013/research/summary.json).

## Exact retail fallback rotations and original file format

The original retail executable contains recoverable numeric map weights and full
variant buffers; the older research correctly said the *published dated web
catalogs* lacked numeric weights, but that does not exclude the executable
fallback. Original `193010` selects five literal map/preset/weight tables.
Complete `1940D0` constructs seven valid fallback hoppers and nine zero entries.
The seven active blocks contain116 entries, using61 distinct table rows across
the five tables and the28 named original preset constructors.
[Original builder reading aid](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp#L650),
[complete unchanged builder execution](../../.local/online-completion-20261013/research/retail-complete-hopper-builder.json),
[all16 direct block controls and28 complete preset buffers](../../.local/online-completion-20261013/research/retail-fallback-pools.json).

| Source table | Used block indices | Entries | Exact examples |
|---|---|---:|---|
| `444FC0` | 0,13 | 14 | Rumble Slayer on map10 weight16; map50 weight14 |
| `445068` | 1,4,6,14 | 24 | 1 Flag CTF on map30 weight10; Classic CTF on map70 weight6 |
| `445188` | 3,5,15 | 12 | 1 Flag CTF on map800 weight8; Assault on map800 weight4 |
| `445218` | 11,12 | 5 | Team Slayer on map10 weight8; map40 weight4; map50 weight6 |
| `445258` | 2 | 6 | Slayer Duel on map50 weight16; map80 weight12 |

Only block indices0–6 are active in the fully constructed launch fallback file;
indices11–15 having a table selector does not prove those hoppers are active.
Duplicate source rows, including repeated map/preset entries, remain separate.
`193010` clears variant flags0x10/0x20 and sets the `+58` value tozero after
running the full preset constructor. Copying generic presets alone misses these
builder changes. The raw buffer artifacts preserve them.
[Complete original construction](../../.local/online-completion-20261013/research/retail-complete-hopper-builder.json).

The literal filename is `multiplayer_hopper.dat`, through pointer `46DD5C`.
The file has16 headers of0x614 bytes, then16 blocks of0x15CB8 bytes. Each block
has map count at+44 and100 records at+48, stride0x37C. A record's map ID is+4,
weight+248 and complete0x130-byte variant buffer+24C. Full file size is1,453,248
bytes. The reconstructed fallback artifact has absent localized UI globals,
which leaves localized names/descriptions blank; it is not an original cached
TMS download. No original source/assets were modified.
[File read/write owner](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp#L404),
[full original builder trace](../../.local/online-completion-20261013/research/retail-complete-hopper-builder-executed.txt).

Original weighted selection `192EB0` ignores maps absent from the installed
map registry, clamps available weights to1–1000, chooses over their cumulative
sum using original RNG, and copies the full selected variant. This establishes
selection mechanics; it does not recover final-era numeric weights or authorize
silently dropping unsupported entries from a claimed complete dated rotation.
[Original selector](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp#L579).

## Final-era catalog evidence and its limits

Bungie's actual **9 May 2007** release announcement lists nine playlists after
consolidation. Its publication is later and stronger than the earlier proposed
13 April update. It identifies casual Rumble Pit, Team Training, Team SWAT and
Big Team Battle; ranked Double Team, Team Slayer, Team Skirmish, Team Snipers and
Team Hardcore. Tombstone/Desolation are required for five. The complete release
announcement was read from the preserved Bungie blog; the current Bungie post
corroborates release and reset.
[Bungie release announcement](https://bnetarchive.haloman30.com/News/Bloge04e.html?dfilter=date112007&mode=&page=7&tag=),
[current official release post](https://www.bungie.net/en/Forums/Post/58025564),
[machine-readable dated identities](../../.local/online-completion-20261013/research/bungie-published-late-catalog-2007-05-09.json),
[versioned documentary catalog bundle](historical-catalog-evidence-20261013.json).

The 13 April proposal specified2v2 Double Team,3–4 per team for the small team
queues, and5–6 per team for casual BTB. It also explained population-driven
consolidation. Those are dated proposal bounds, not proof of every later final
TMS setting. The actual May announcement supplies names/visibility and release
status but no complete rotations or numeric weights.
[Bungie's proposal](https://bnetarchive.haloman30.com/News/content2559.html?cid=12362&type=topnews).

No final-2010 `multiplayer_hopper.dat` or `network_configuration.dat` was found
in the inspected supplied source/import trees or retrieved in this pass. The
archive availability API, asked for 14 April 2010 for the old official playlist
URL, returned its nearest capture as **14 March 2007**, before the consolidation.
Direct Wayback/CDX reads then returned429. The returned capture must not be
renamed 2010. The mirror's earlier HTML is available but not evidence of a later
download's exact bytes. [Recorded source status](../../.local/online-completion-20261013/research/public-source-status.json).

Two older complete public factual manifests were extracted from already retained
unmodified original Bungie HTML:9 playlists/89 explicit rows labeled30 Nov2004;
14 playlists/369 explicit rows,503 map/variant pairs labeled2 Dec2005. The
extractor preserves original misspellings, group rows, source metadata
contradictions and missing numeric weights/settings. It handles malformed source
cell endings without silently losing rows. These manifests aid coverage audits;
they cannot replace final-era TMS inputs.
[2004 manifest](../../.local/online-completion-20261013/research/bungie-public-catalog-2004-11-30.json),
[2005 manifest](../../.local/online-completion-20261013/research/bungie-public-catalog-2005-12-02.json),
[original catalog citations](online-rank-playlist-evidence.md#dated-actual-playlist-catalog).

## Restored Action Sack and Fiesta

The 2 Dec 2005 published **Team ActionSack** is casual,6–8 players, two teams,
parties1–8. It has20 published rows and68 map/variant pairs. The restored queue
can cite this dated catalog, independently from the requested final-era default.
Its novelty rows include Phantom Fiesta and SWAT Fiesta. Their published maps
are preserved in the manifest, while numeric weights and full downloaded
variant settings remain absent.
[Original public catalog](https://web.archive.org/web/20051212093137/http://www.bungie.net/Stats/page.aspx?section=FAQInfo&subsection=playlists&page=1),
[complete ActionSack rows](../../.local/online-completion-20261013/research/bungie-public-catalog-2005-12-02.json).

The supplied original `mainmenu.map` does contain an authored **Fiesta** variant
(index33 among39 built-ins). Its source engine is Oddball, with120 score,
primary starting weapon parameter89=2 (`Random`), secondary94=1 (`None`), and
weapons-on-map83=19 (`Random Set`). It is not proof of an exact named Team Fiesta
Slayer, Phantom Fiesta or SWAT Fiesta buffer. Original parameter89 and83 are
separate owners and must not be confused. The unchanged retail/TU1.1 variant
oracle already captures this authored buffer and descriptor semantics.
[Source asset/control report](../../analysis/multiplayer/game-variants/original.json),
[original variant oracle](../../tools/oracle_game_variants.py).

Party1–8 in the historical casual catalog also needs a deliberate team-layout
contract. Keeping every accepted party intact cannot fit a party of8 on a
four-player team. Restricting that queue to party4 is a native restriction;
claiming the public maximum8 requires the original casual splitting/assignment
owner. In all cases, the user-requested four couch players should remain a
supported whole team against others, and untethered guests remain excluded from
persistent personal statistics. The historical web metadata alone does not
settle modern party admission or the user-authorized guest privacy contract.

## Quit penalties and clan rank presentation

Original retained quit standings are already recovered across retail/TU1.1/U5:
retain allocated departed players and their original PCR statistics; rank FFA
quitters below finishers; use actual team standings for team players, and place
any team with a finisher above wholly departed teams. A quitter whose teammates
win receives that team result. Normal pairwise XP applies; neither original
retained result producer nor full player/clan XP routines show an additional
fixed quit subtraction. The new TU1.5 controls preserve that arithmetic owner.
[Original departure evidence and unchanged controls](halo2-nonhost-departure-evidence.md#retention-placement-and-final-publication),
[original result caller](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1300).

Bungie's 20 May 2005 answer says ordinary quitting is not cheating and cannot be
banned merely to force play. Earlier 22 April language discusses abusive repeated
quitting through feedback among misconduct. Neither provides an automatic
counter, cooldown duration or fixed XP fine. Therefore a modern escalating quit
cooldown could be an explicitly new policy, but must not be called a recovered
original penalty or imported from later Halo games.
[Bungie 20 May 2005](https://halo.bungie.org/bwu/index.html?item=26),
[Bungie 22 April 2005](https://halo.bungie.org/bwu/index.html?item=22).

Clan result subject ownership is already exact: type5 uses selected clan rank
and XP, with one clan-owned update, rather than personal rank. The original rank
artwork/raw range mapping is also recovered. Original final clan UI widget
binding and the later downloadable clan catalog remain different questions;
clan restoration was not present in the May2007 nine-playlist announcement.
Do not substitute a player's top personal rank for a selected clan's rank.
[Batched clan ownership and controls](halo2-clan-matchmaking-evidence.md#clan-owned-postgame-xp-and-rank),
[original raw-to-artwork mapping](online-rank-playlist-evidence.md#stored-049-represents-displayed-150).
Bungie specifically changed friends' top-rank presentation to highest non-clan
rank in the 15 April 2005 update; that reinforces the distinction between friend
profile rank and a selected clan rank.
[Bungie 15 April 2005](https://halo.bungie.org/bwu/index.html?item=21).

## Reproduction and implementation boundaries

From the repository root:

```sh
.tools/python/bin/python .local/online-completion-20261013/research/oracle_historical_config.py
.tools/python/bin/python .local/online-completion-20261013/research/oracle_retail_complete_hoppers.py
python3 .local/online-completion-20261013/research/extract_public_catalogs.py
```

The third command uses BeautifulSoup to read already retained original HTML;
no credentials or live Discord identities enter the research artifacts.
[Artifact checksums](../../.local/online-completion-20261013/research/manifest.json).

Actionable now: add the independently versioned exact TU1.5 ranking data and
compare native arithmetic to the provided controls; retain quitters and apply
source standings/normal XP; preserve late-era names/visibility as dated evidence;
use whole original buffers for any supported retail fallback queue; expose
restored Action Sack with its actual recovery provenance. Completing every
final-era rotation still requires actual dated TMS map/variant/options/weights
and admitted gameplay for the complete pool. Source defaults or a working
Slayer subset must not conceal missing objective, vehicle or weapon behavior.
