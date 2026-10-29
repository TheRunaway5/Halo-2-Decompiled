# Playlist and hopper source evidence

Research date: 2026-10-28. TU1.5 controls, recovered literal tables and MCC
scenario metadata support the implementation below. **No authenticated final-2010
downloaded hopper file or complete final-era numeric weighting was recovered.**
Removing development labels does not turn an authored rotation into that missing
service configuration. This note changes no live service or catalog.

## Executed TU1.5 selection contract

The actual TU1.5 selector starts at `198EC0`. The bounded unchanged-byte replay
starts at `198EDB`, after the file-read call, with an explicitly supplied
synthetic read result and complete block. It executes the real map predicate
`1A5E30`, weighting, RNG, ordered selection, 304-byte copy and return. It does
not execute file access, a real downloaded catalog, original upstream RNG
ownership/call ordering or a whole matchmaking session.

The authority is the U5 English executable SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
[Oracle](../../tools/oracle_hopper_selection.py),
[independent expected outputs](../../tests/fixtures/hopper_selection_tu15_original.json),
[execution receipt](evidence/playlist-hoppers-20261028/selection-original.json).

The executed contract is:

1. Preserve row order and duplicate rows. A block holds up to 100 rows; a row
   carries map ID at `+4`, signed weight at `+248` and complete settings at
   `+24C`, within stride `37C`. The block count is at `+44`, rows start at `+48`.
2. An absent installed-map ID has effective weight zero. Each available row's
   signed stored weight clamps to **1..1000**, including zero/negative inputs.
   The original predicate searches supplied map metadata; it does not trust a
   map filename supplied by the weighted row.
3. With no available weight, return failure without changing RNG or outputs.
   The supplied failed-read path likewise preserves those values.
4. Otherwise consume one 32-bit step from the random owner field `+4`:
   `seed = seed * 1664525 + 1013904223 (mod 2^32)`; use `seed >> 16`.
5. For safe total `T <= 32766`, choose `1 + ((high16 * T) >> 16)` and take the
   **first** row whose cumulative weight is at least that choice. Copy that
   row's map and **all 304 settings bytes**. Do not independently redraw a
   mode, map or option subset afterward.
6. The original converts `T+1` to signed 16 bits. At total 32767 and above the
   replay exposes undesirable overflow behavior, including first-row selection.
   An authored modern catalog should reject unsafe totals before selection;
   this is an explicit admission restriction, not a claim that the source
   handles arbitrary positive sums correctly.

The run covers **456 cases**, including 420 safe-total cases and 36 overflow
cases, with 150 distinct executed original instructions and no instruction
patches or substituted callees. It includes absent maps, duplicate IDs, signed
clamps, empty/read-failed blocks, seed wrap, selected output preservation and
exact settings-copy hashes. Synthetic settings use all byte values by a fixed
pattern; they are not a claimed historical variant buffer.

For frequencies, original integer scaling has a 1/65536 draw grid. For cumulative
interval `[A,B)` in total `T`, its exact number of high16 draws is
`ceil(B*65536/T)-ceil(A*65536/T)`. Tests can enumerate the high16 domain instead
of using a flaky random sampling tolerance. Duplicate map/mode rows contribute
their separate intervals; filtering changes the denominator.

The independent reconstructed reading aids are
[selector and builder](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp)
and [LCG helper](../../tools/reference/halo2-decompiled/include/unknown_0259d0.h).
Their retail annotations are locators; the actual U5 execution above supplies
the current instruction evidence. The TU1.5 storage ABI and instruction order
differ from retail; whole-function byte equivalence is not asserted.

## Exact table data versus historical service data

All five recovered retail tables occur byte-for-byte, uniquely, in TU1.5:

| Retail locator | TU1.5 byte location | Rows | Weight sum |
| --- | --- | ---: | ---: |
| 444FC0 | 44F120 | 14 | 64 |
| 445068 | 44F1C8 | 24 | 67 |
| 445188 | 44F2E8 | 12 | 32 |
| 445218 | 44F378 | 5 | 20 |
| 445258 | 44F3B8 | 6 | 38 |

The [literal-table receipt](evidence/playlist-hoppers-20261028/literal-tables.json)
retains all 61 `(preset,map,weight)` triples and hashes from both authenticated
images. This is proof of table bytes, **not** proof that each updated fallback
caller activates them, nor that the 2010 service delivered those rows.

The existing complete unchanged retail fallback constructor independently
produced seven active hoppers, 116 total rows and complete per-row settings.
Its constructor clears flags `10/20` at settings `+48` and the `+58` value;
substituting a same-named generic preset loses that information. This is a
separate launch-era profile, with no claim of final-era activation.
[Prior complete-builder evidence and limitations](online-historical-config-followup-20261013.md),
[retained full original output](../../.local/online-completion-20261013/research/retail-complete-hopper-builder.json).

Bungie's released May 9, 2007 announcement identifies unranked Rumble Pit,
Team Training, Team SWAT and Big Team Battle, plus ranked Double Team, Team
Slayer, Team Skirmish, Team Snipers and Team Hardcore. It gives no complete
numeric map/variant weights. The April 13 proposal describes 2v2 doubles,
3–4 players per small team, 5–6 per BTB team and a Slayer-biased mixed BTB pool;
proposal bounds must remain distinct from the actual downloaded final settings.
[Released announcement](https://bnetarchive.haloman30.com/News/Bloge04e.html?dfilter=date112007&mode=&page=7&tag=),
[April proposal](https://bnetarchive.haloman30.com/News/content2559.html?cid=12362&type=topnews).

User-requested Action Sack/Fiesta/Infection and restored clan queues can coexist
with that dated nine-playlist lineup, with their own explicit provenance.
The preserved December 2005 Team ActionSack publication supplies many named
map/variant combinations but no numeric weights or complete options. Names
alone cannot safely construct those objective variants.
[Documentary dated catalogs](historical-catalog-evidence-20261013.json).

## Permanent recovered data and option identifiers

[recovered-hopper-rows-v1.json](../master_server/data/recovered-hopper-rows-v1.json)
is a self-contained 236,277-byte metadata file: all 16 original fallback headers
(seven active), their 116 ordered records with complete raw 304-byte settings,
all 61 literal table triples, all 28 original executable presets, seven native
default buffers and the 39 original built-ins. It pins the source fixture,
original constructor reports, constructed file and executable/mainmenu hashes.
Production catalog generation need not import ignored `.local` files. Raw
metadata is not a map/artwork/executable payload. Its provenance keeps retail
construction, the retail/TU1.1 built-in fixture, and U5 byte-presence evidence
separate rather than labeling any of them a final-service download.

The reconstructed online headers explicitly copy the **retail launch default
rank map** at `+51C..+59B` from those retained original headers. Their previous
zero-filled map was invalid under the original signed-byte validator and is
now rejected by Rust during catalog admission. The 128-byte map has SHA-256
`e429ca18cb9a49e9a1348dbe68d315fa8a8b4a64d51dd0704a451e271d463cef`.
Each generated frequency-manifest entry identifies the exact source header,
executable and table bytes. This is an explicit reconstructed-profile choice;
dated published map/mode membership does not recover downloaded rank maps.
The selected header feeds Rust's search and advertisement bounds. Persistent
XP configuration remains separately identified by `rank_configuration`.

The 39 built-in names, frontend engine IDs and authored writes were independently
checked against the pinned original mainmenu's `goof` rows. The original `sily`
records supply the option IDs and serialized choices below; their actual tag
names/file offsets and localized labels are in the
[option receipt](evidence/playlist-hoppers-20261028/option-parameters.json).
That receipt now covers all 112 IDs: 92 actual UI records and 102 non-null
descriptor functions independently executed in both retail and TU1.5. U5's
actual getter `23DEE0` reads table `47CF40`; all 112 descriptor contracts match
retail, with 477 distinct executed instructions per image and no patches or
substituted callees. Missing UI meanings remain null rather than being inferred
from a neighboring field.

| Meaning | Parameter ID | Serialized choice |
| --- | ---: | --- |
| Shield type | 19 | 0 normal; 1 none; 2 overshields |
| Motion sensor | 20 | 0 off; 1 on |
| Primary weapon | 89 | 3 BR; 4 magnum; 10 sniper; 14 beam rifle |
| Secondary weapon | 94 | 1 none; otherwise same weapon IDs |
| Starting grenades | 88 | 0 off; 1 on |
| Map grenades | 86 | 0 off; 1 on |
| Map weapon set | 83 | 1 none; 11 rifles; 12 sniping |
| Overshield / camouflage pickup | 84 / 85 | 0 off; 1 on |
| Primary / secondary light vehicle | 76 / 77 | 7 none |
| Primary heavy vehicle | 78 | 4 none |
| Banshee | 79 | 4 off |
| Primary / secondary turret | 81 / 82 | 6 none |

The actual original Snipers built-in writes, in order, are
`20=0,89=10,94=4,83=12,76=7,77=7,78=4,80=4,79=4,2=15`.
The secondary-heavy `80=4` is directly present in that authored `goof` row;
it has no separate original `sily` record in this inspected menu. The Snipers
buffer is free-for-all, so a new team version must also set the actual team
option (`91=1`) and its explicitly chosen team score. Such an authored version
does not establish every original Team Snipers download setting.

There are no Team SWAT or Team Hardcore complete buffers among those 39
built-ins. Verified parameter choices permit an explicit authored SWAT setup
(no shields, BR/magnum, no motion sensor/grenades/vehicles), but do not by
themselves prove the final-service variant. The December 2005 primary Hardcore
description establishes nondefault starting weapons and no motion sensor; its
rows include multiple Pro CTF, Slayer, Oddball, King, Snipers and Assault
variants. It does not give the full option buffers. A single BR-start Slayer
row is therefore an authored approximation, not a restored complete Hardcore
hopper. The December 2005 Snipers description establishes a ranked two-team
sniper playlist but likewise omits full buffers and numeric weights.
[Preserved primary catalog and exact source identity](../../.local/online-completion-20261013/research/bungie-public-catalog-2005-12-02.json),
[public primary capture](https://web.archive.org/web/20051212093137/http://www.bungie.net/Stats/page.aspx?section=FAQInfo&subsection=playlists&page=1).

For authored objective equivalents, the same original UI records establish:

| Meaning | Parameter ID | Serialized choices |
| --- | ---: | --- |
| CTF flag placement | 50 | 0 per team; 1 single; 2 neutral |
| Flag touch return / flag at home to score | 52 / 53 | 0 off; 1 on |
| Flag reset time | 54 | 5,10,15,30,45,60 seconds |
| Bomb placement | 59 | 0 per team; 1 single; 2 neutral |
| Bomb reset time | 61 | 5,10,15,20,30,45,60 seconds |
| Bomb arm time | 63 | 5,10,15 seconds |
| Bomb fuse field (no UI record) | 95 | Original 16-bit descriptor at +10A; native normalization admits 0..15 |
| Bomb touch return | 92 | 0 off; 1 on |
| Sticky arming | 97 | 0 off; 1 on |
| Oddball count | 43 | 1,2,3 |
| Moving hill | 40 | 0 off; 30,60,120,180,300 seconds |
| Uncontested hill / team time multiplier | 38 / 39 | 0 off; 1 on |
| Flag / bomb / ball carrier vehicle operation | 57 / 66 / 46 | 0 off; 1 on |

The original `sily` title for ID61 is **Bomb Reset Time**, despite the reading
aid's older “Assault Detonation Time” name. It writes the32-bit `+F4` reset
field; ID63 writes the16-bit `+108` arm field. ID95 is a real descriptor for
the neighboring `+10A` field, but has no original menu record/title/choices.
Its planted-bomb countdown role is visible in the compiled original TU1.5
plant owner: `24BFC0` reads game-options `+246` (variant base `+13C`, hence
variant `+10A`) and `24BFCA` writes the objective countdown at `+114`.
The source countdown owner consumes that value. A zero setting has a special
original plant fallback of10; it must not be advertised as immediate detonation.
This is a source inspection of existing original-derived native owners, separate
from the newly executed descriptor controls above.
[Plant/countdown owner](../../src/gameplay/multiplayer/objective_native/plant.hpp),
[countdown consumption](../../src/gameplay/multiplayer/objective_native/object_tick.hpp).
No separate disarm-time UI parameter was identified. Carrier vehicle operation
is also distinct from a global vehicle-weapon enable flag; no such separate UI
flag was identified. Do not substitute reset time for either operation.

December2005 Team Skirmish has53 objective rows and **no Slayer rows**.
Its mixed predecessor retail fallback must not silently replace that dated
published membership. The self-contained
[published membership data](../master_server/data/published-hopper-membership-v1.json)
retains every source row and its ordered map list separately from unknown
numeric weights and variant buffers.

## MCC map identities

The read-only [MCC identity receipt](evidence/playlist-hoppers-20261028/mcc-map-identities.json)
joins authored menu/scenario paths to the prepared cache files. The current
runtime root was read in place; no cache bytes were changed. Important base
joins are:

| Source map ID | MCC key | Display name |
| ---: | --- | --- |
| 10 | cyclotron | Ivory Tower |
| 20 | midship | Midship |
| 30 | zanzibar | Zanzibar |
| 40 | waterworks | Waterworks |
| 50 | lockout | Lockout |
| 60 | burial_mounds | Burial Mounds |
| 70 | colossus | Colossus |
| 80 | ascension | Ascension |
| 100 | beavercreek | Beaver Creek |
| 110 | coagulation | Coagulation |
| 800 | headlong | Headlong |
| 1402 | highplains | Tombstone |
| 1400 | derelict | Desolation |

The last two IDs/names come from each MCC scenario's own `scnr+398` level-data
record. The inspected prepared mainmenu still contains blank-title placeholder
rows 1239/1240 for those paths, so those placeholders must not supply identity.
MCC Tombstone's scenario leaf is **highplains**, not the old Xbox revision leaf
`highplains2`. An alias from that old filename is not an MCC asset binding.
Map support must also retain the actual authored team-count/engine metadata;
an installed map does not by itself prove all modes or complete match outcomes.

All oracle/probe invocations used `tools/run_test.py`. Private scratch was
removed; only source, the 883,738-byte expected-output fixture and small JSON
receipts are retained. No binaries, native runtime, original XBE/map copies,
captures, credentials or player data were retained or changed.
