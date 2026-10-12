# Original Xbox multiplayer metadata and initial Slayer behavior

The disc's built-in **Slayer** is a free-for-all match with **25 points to win,
one eight-minute round, five-second ordinary respawns, and suicide point loss**.
Its spawn loadout is map-authored: ten shipped maps use an SMG, Midship and
Waterworks use a plasma rifle. Midship starts with plasma grenades; the other
maps start with frag grenades. These defaults come from the original executable
initializer and each scenario's starting-equipment records, not the campaign
starting profile or the serialized `sily` placeholder value.

This research establishes serialized metadata and selected original instruction
behavior. It does not establish a live match, Xbox System Link interoperability,
the full spawn selection algorithm, battery weapon behavior, complete pickups,
or complete multiplayer simulation parity.

## Pinned authorities and reproduction

- `assets/disc/default.xbe`: SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
- `tools/reference/halo2-decompiled`: revision
  `5f08630655e3ddc9ac45909090f11ae799fa0c27`.
- `tools/reference/assembly`: revision
  `fd14ef3c46135e9422966c6a64cd926fb0bd23fb`.
- Original caches: Xbox cache version 8, build `02.09.27.09809`; map SHA-256s,
  datums, tag names, and individual record file offsets are retained in the
  generated metadata report.

The versionable inspection/oracle tools contain no copyrighted map contents:

```bash
python tools/inspect_multiplayer.py --output analysis/multiplayer/authored-maps.json
python tools/inspect_multiplayer.py assets/disc/maps/mainmenu.map \
  --output analysis/multiplayer/authored-variants.json
.tools/python/bin/python tools/oracle_multiplayer.py \
  --output analysis/multiplayer/slayer-instruction-oracle.json
```

`inspect_multiplayer.py` reads the original caches without editing them. Tag
blocks use a count and metadata virtual pointer; the parser subtracts each
cache's derived metadata magic and bounds-checks every block. It resolves tag
references by the full datum, and localization by the referenced `unic` range
rather than treating repeated StringIDs across lists as globally unique.

`oracle_multiplayer.py` verifies the XBE hash, maps its original sections, and
executes these original routines in Unicorn:

| Original instructions | Verified result / explicit boundary |
| --- | --- |
| `0x19D220` | Slayer variant bytes. Locale/tag lookup is absent, so the name is empty; gameplay settings execute unchanged. |
| `0x2374F0` | All 112 parameter IDs read from those actual variant bytes through the retail callback table at `0x470828`. |
| `0x15A090` | Slayer accepts selector values `2`, `12`, `13`, `14`, rejecting the other single-selector cases `0..14`. |
| `0x2BC000` | Kill `+1`, suicide `-1`, betrayal `-1`, uncredited death no delta. The score-update service at `0x15B7C0` records requests; replication and win handling are outside that oracle. |
| `0x15CE70..0x15D151` | Countdown prefix at 30 Hz: ordinary death 150 ticks, suicide 300, betrayal victim 150, betrayer's subsequent death with the pending penalty 450. Execution stops before death markers, medal/event work and score dispatch. |
| `0x14F040` | Multiplayer appearance values `0,1,2,3` select authored representation slots `2,3,2,3`; supplied slot datums distinguish the selected records. |
| `0xFDAD0` | Weapon creation initializes an SMG magazine to 60 loaded/120 unloaded from total initial 180. Animation reset `0x1058B0` is skipped; magazine arithmetic executes unchanged. |
| `0x19EDA0` | Starting-weapon option 5 resolves `mulg` Runtime Weapons index 1, the authored SMG datum on all 21 supplied caches. Default option 0 preserves map equipment; none/random and other native options remain unsupported. |

The source identifies the enum conversion at
[`unknown_19d220.cpp:28`](../tools/reference/halo2-decompiled/src/unknown_19d220.cpp#L28):
UI variant category `0` means Slayer, but the actual engine/scenario enum is `2`.
`mulg` event types have a separate enumeration; do not reuse event-type numbers
as variant or scenario numbers.

## Player starts

Authority: Assembly
[`scnr.xml:1128`](../tools/reference/assembly/src/Assembly/Plugins/Halo2/scnr.xml#L1128),
`Player Starting Locations`, scenario root `+0x100`, stride `0x34`.

| Record field | Offset / encoding |
| --- | --- |
| Position | `+0x00`, three floats |
| Facing | `+0x0C`, radians float |
| Team designator | `+0x10`, signed 16-bit; Red `0` through Pink `7`, Neutral `8` |
| BSP ordinal | `+0x12`, signed 16-bit |
| Four game-type selectors | `+0x14`, `+0x16`, `+0x18`, `+0x1A`, signed 16-bit |
| Four corresponding spawn types | `+0x1C`, `+0x1E`, `+0x20`, `+0x22`, signed 16-bit; Both `0`, Initial only `1`, Respawn only `2` |
| Campaign player type | `+0x2C`, signed 16-bit; Chief `0`, Arbiter `1`, Chief multiplayer `2`, Elite multiplayer `3` in the schema |

Equipment mode matching is recovered in
[`unknown_157450.cpp:2023`](../tools/reference/halo2-decompiled/src/unknown_157450.cpp#L2023),
retail `0x15A090`. It returns the **first matching selector's index**. For Slayer,
match `2`, All `12`, or either exclusion selector `13`/`14`; `0` does not match
an active multiplayer engine. Both exclusion values reject CTF/Assault (`1`/`9`)
in the actual instructions, even though the schema describes `14` as excluding
Race. Preserve that executable behavior rather than generalizing the labels.
Without an active game engine the helper instead matches a `0` selector.

This helper is called by netgame equipment `0x15AE70` and starting equipment
`0x19EF40`. It is not a player-start eligibility predicate. The actual
player-start chooser `0x14EF00`, and title-update chooser `0x150640`, iterate
every `scnr +0x100` record. Their scoring helpers `0x23BA90` / `0x23DB90` read
only the record's position (`+0`, `+4`, `+8`); the four game/spawn selector
arrays do not participate. Initial versus respawn state selects the dynamic
spawn-zone bank (`0x23B170`, updated `0x23D270`) through player flag bit 3.
All 121 Dune starts have zero selector arrays and are valid inputs to this
chooser. The native runtime admits starts in its active BSP; its existing
distance ranking still does not reproduce the complete original spawn score.

All shipped multiplayer starts have BSP ordinal `0`, spawn-type arrays
`[0,0,0,0]`, and campaign-player-type field `0`. Requiring the multiplayer
campaign-player values `2`/`3` would reject **every** original start. FFA must
ignore a start's team designator: most original maps mark every valid start
Red `0`, while Midship and Headlong use Neutral `8`. Team modes need the actual
engine's team/start rules before claiming team-spawn parity.

| Map file | Authored starts | Player starts considered | Netgame equipment / Slayer-mode equipment | Default primary | Starting grenade flag |
| --- | ---: | ---: | ---: | --- | --- |
| `ascension.map` | 68 | 68 | 33 / 33 | SMG | `0`, frag |
| `beavercreek.map` | 87 | 87 | 41 / 41 | SMG | `0`, frag |
| `burial_mounds.map` | 158 | 158 | 41 / 41 | SMG | `0`, frag |
| `coagulation.map` | 123 | 123 | 61 / 61 | SMG | `0`, frag |
| `colossus.map` | 62 | 62 | 47 / 47 | SMG | `0`, frag |
| `cyclotron.map` | 112 | 112 | 40 / 40 | SMG | `0`, frag |
| `foundation.map` | 70 | 70 | 43 / 43 | SMG | `0`, frag |
| `headlong.map` | 183 | 183 | 82 / 82 | SMG | `0`, frag |
| `lockout.map` | 66 | 66 | 32 / 32 | SMG | `0`, frag |
| `midship.map` | 48 | 48 | 21 / 21 | Plasma rifle | `2`, plasma |
| `waterworks.map` | 204 | 204 | 80 / 80 | Plasma rifle | `0`, frag |
| `zanzibar.map` | 59 | 59 | 75 / 74 | SMG | `0`, frag |

Equipment counts include vehicle collections and null references; they are not
all weapon spawn counts. The generated reports retain each classification,
datum and position for filtering. The display names map `cyclotron` to Ivory
Tower and `beavercreek` to Beaver Creek in the existing frontend list.

The retail has considerably richer spawn influences than a distance heuristic:
[`unknown_23b500.cpp`](../tools/reference/halo2-decompiled/src/unknown_23b500.cpp),
`0x23AEA0`, `0x23B060`, `0x23B170`, `0x23B500`, `0x23B8E0`, `0x23BC40`, covers
scenario overrides, static zones, players, moving vehicles/projectiles, and
engine-specific influences. A native farthest-player choice is an explicitly
host-authored selection policy, even if it consumes the original legal starts.

## Variant defaults and parameter IDs

Authority: original initializer
[`unknown_19d220.cpp:32`](../tools/reference/halo2-decompiled/src/unknown_19d220.cpp#L32),
retail `0x19D220`; exact parameter mapping
[`unknown_236ae6.cpp:76`](../tools/reference/halo2-decompiled/src/unknown_236ae6.cpp#L76),
retail callback table `0x470828` and getter `0x2374F0`.

**Do not treat `sily +4` as the default gameplay value.** All 92 serialized
parameter tags in `mainmenu.map` have zero there. The built-in Slayer `goof`
entry (`mainmenu.map` file offset `0x038D3F04`) has an empty override block.
Retail first initializes the engine defaults, then applies the selected built-in
variant's `(parameter,value)` overrides; this sequence is visible in
[`unknown_1482e8.cpp:124`](../tools/reference/halo2-decompiled/src/unknown_1482e8.cpp#L124),
`0x1489B5`. Seed actual defaults before applying the overrides.

| UI parameter ID | Meaning | Original Slayer default | Variant storage |
| --- | --- | --- | --- |
| `0x00` | Round setting | `0`, one round | i32 `+0x4C` |
| `0x02` | Slayer score to win | `25` | i32 `+0x50` |
| `0x0A` | Round time limit | `480` seconds | i32 `+0x54` |
| `0x0B` | Rounds reset map | `1` | flags `+0x48`, bit 3 |
| `0x0C` | Resolve ties | `1` | flags `+0x48`, bit 4 |
| `0x0F` | Maximum players | `16` | i32 `+0x74` |
| `0x10` | Lives per round | `0`, unlimited | i32 `+0x7C` |
| `0x11` | Respawn time | `5` seconds | i32 `+0x80` |
| `0x12` | Suicide respawn penalty | `5` seconds | i32 `+0x84` |
| `0x13` | Shields | `0`, normal | i32 `+0x88` |
| `0x14` | Motion sensor | `1`, on | flags `+0x48`, bit 1 |
| `0x15` | Player active camo | `0`, off | flags `+0x48`, bit 2 |
| `0x16` | Team changing | `0`, off | flags `+0x48`, bit 6 |
| `0x17` | Team scoring | `0`, sum | i32 `+0xA4` |
| `0x18` | Friendly fire | `1`, on | flags `+0x48`, bit 7 |
| `0x19` | Team respawn modifier | `2`, None | i32 `+0xA8` |
| `0x1A` | Betrayal respawn penalty | `10` seconds | i32 `+0xAC` |
| `0x1C` | Slayer bonus points | `0`, off | engine flags `+0xF0`, bit 0 |
| `0x1D` | Slayer suicide point loss | `1`, on | engine flags `+0xF0`, bit 1 |
| `0x1E` | Slayer death point loss | `0`, off | engine flags `+0xF0`, bit 2 |
| `0x53` | Weapons on map | `0`, map default | i8 `+0xD4` |
| `0x54` | Overshields on map | `1`, on | flags `+0x48`, bit 8 |
| `0x55` | Camo on map | `1`, on | flags `+0x48`, bit 9 |
| `0x56` | Grenades on map | `1`, on | flags `+0x48`, bit 10 |
| `0x57` | Weapon respawn time | `0`, map default | i8 `+0xD5` |
| `0x58` | Starting grenades | `1`, on | flags `+0x48`, bit 11 |
| `0x59` | Primary starting weapon | `0`, map default | i8 `+0xD6` |
| `0x5A` | Max active players | `16` | i32 `+0x78` |
| `0x5B` | Teams enabled | `0`, off | flags `+0x48`, bit 0 |
| `0x5E` | Secondary starting weapon | `0`, map default | i8 `+0xD7` |

The initializer produces flags `+0x48 = 0xF9A` and Slayer engine flags
`+0xF0 = 2`. The parameter oracle exposes other IDs too; zero is not a universal
default policy. Team Slayer's authored override block contains score `50` and
teams enabled `1`; the remaining options inherit these initialized defaults.

## Starting weapons and grenades

Authority: Assembly `scnr.xml:1431`, Starting Equipment root `+0x128`, stride
`0x9C`; original executable **`0x19EF40`** (inventory/loadout) and **`0x15E970`**
(grenades). Both bodies are present in the XBE; inventory lists them as `todo`,
so the actual disassembly is the authority for control flow.

Starting-equipment flags are u32 `+0x00`: bit 0 No Grenades, bit 1 Plasma Grenades.
Mode selectors are signed shorts `+0x04..+0x0A`. Six collection references occur
at `+0x3C`, `+0x44`, `+0x4C`, `+0x54`, `+0x5C`, `+0x64`, with datum `+4` inside
each reference. Retail `0x19EF40` scans **the first five** collection slots,
finds up to two distinct weapon datums, then applies variant primary/secondary
choices. The sixth serialized reference exists but this routine does not scan
it. All twelve original maps have exactly one All-types `12` starting-equipment
record with one positive-weight item in the first collection and every other
reference null; hence their default secondary weapon is absent.

Collection layout: Assembly [`itmc.xml`](../tools/reference/assembly/src/Assembly/Plugins/Halo2/itmc.xml),
item block root `+0`, stride `0x10`; weight float `+0`, item reference `+4` (datum
`+8`), variant StringID `+0x0C`; collection spawn time signed short root `+8`.
Weighted choice is original `0x19F1A0`, recovered in
[`unknown_19eda0.cpp:124`](../tools/reference/halo2-decompiled/src/unknown_19eda0.cpp#L124).
A single item is selected directly; multiple items use the original integer
random draw and cumulative authored weights. Selecting the first item is only
faithful for the inspected single-item default collections.

Weapon choices are original `0x19EDA0`, recovered in
[`unknown_19eda0.cpp:38`](../tools/reference/halo2-decompiled/src/unknown_19eda0.cpp#L38):
`0` retains map default, `1` no weapon, `2` random while excluding the other
weapon, and `3..20` map through an explicit permutation of the `mulg` runtime
Weapons block (`+0x68`, stride `8`). The choices are not consecutive weapon-table
indices. Keep tag datum resolution instead of matching filename substrings.

`matg` references `mulg` at root `+0x168` (datum `+0x16C`). Its Runtime block is
`+8`, stride `0x568`; the schema also exposes default collections `+0x110` and
`+0x118` and grenade counts `+0x120`/`+0x124`. Those defaults are null/zero in the
inspected original multiplayer maps. The executed loadout path reads scenario
starting equipment instead. `scnr +0xF8` campaign profiles are absent on eleven
maps; Foundation has one profile with both weapon references null. These
campaign profiles cannot supply the multiplayer loadout.

Original `0x15E970` initializes frag count `2`, plasma `0`, with per-type maximum
`2`; the multiplayer globals byte-0 bit-3 state lowers that maximum to `1`.
The matching starting-equipment record's No Grenades bit zeroes both counts;
Plasma Grenades transfers frag to plasma. Variant Starting Grenades off zeroes
both afterward. With normal defaults this gives:

- Midship: plasma rifle, two plasma grenades, no frag grenades.
- Waterworks: plasma rifle, two frag grenades, no plasma grenades.
- Other ten maps: SMG, two frag grenades, no plasma grenades.

Weapon ammunition initialization is the weapon creation callback `0xFDAD0`.
Assembly `weap.xml:465` magazine
root `+0x2C0`, stride `0x5C`, distinguishes total initial `+6`, total maximum
`+8`, loaded maximum `+0xA`, and inventory maximum `+0xC`. The SMG has values
`180`, `240`, `60`, `180` respectively. The multiplayer loadout creation chain is
`0x15E970 -> 0x19EF40 -> 0xB7B40 -> 0x108A90`. Its type-2 handler list at
`g_468630[2]=0x467CD0` supplies creation callback `+0x2C=0xFDAD0`.
Original `0xFDB8D..0xFDBB3`, for ordinary nonnegative tag values, writes
`loaded=min(initialTotal, loadedMaximum)` and
`unloaded=min(initialTotal-loaded, inventoryMaximum)` to weapon runtime magazine
`+0x224 + index*0x10`, loaded `+8`, unloaded `+6`. **The starting SMG therefore
has 60 loaded and 120 reserve**, not 60+180. Total maximum `+8` is not read by
this initializer. The oracle runs the complete callback with 180/20/300 initial
rounds to verify normal creation, a partially loaded magazine, and the inventory
cap; it skips only the animation-reset service.
[Creation instructions](../analysis/multiplayer/ammunition-fdad0.txt),
[type dispatch](../analysis/multiplayer/ammunition-108a90.txt).

Plasma rifle magazine fields are all zero because
it uses battery/heat behavior, which the current magazine-only fire path cannot
represent. Both authored plasma-rifle maps need that behavior for full combat.

Player representation comes from `matg +0x138`, stride `0xBC`: hands datum `+4`,
biped datum `+0xB4`. Original multiplayer maps retain null campaign entries 0/1;
entry 2 references `masterchief_mp`, entry 3 `elite_mp`. The `mulg` Unit reference
is a different datum (campaign `masterchief` in inspected maps). Preserve the
profile's representation selection rather than using that Unit reference.
Original selection is
[`unknown_14b560.cpp:959`](../tools/reference/halo2-decompiled/src/unknown_14b560.cpp#L959),
`player_get_representation`, retail **`0x14F040`**: clamp the appearance enum,
then in multiplayer (`game_options.state == 2`) map appearance `0` to
representation `2`, and appearance `1` to representation `3`. Default Spartan
therefore selects authored entry `2`. `player_set_local_user` (`0x14F190`) and
player/unit assignment (`0x14CAD0`) repeat that conversion when binding the
first-person representation. Campaign appearance entries must not be used
directly for a multiplayer player.

## Netgame pickups, scoring and respawn

Authority: Assembly `scnr.xml:1310`, Netgame Equipment root `+0x120`, stride
`0x90`. Record flags `+0` contain Levitate bit 0 and Destroy Existing on New Spawn
bit 1; mode selectors signed shorts `+4..+0xA`; team index `+0xC`; spawn seconds
`+0xE`; empty respawn seconds `+0x10`; timer-start enum `+0x12` (pickup `0`, body
depletion `1`); classification byte `+0x14`; position `+0x40`; orientation
`+0x4C`; item/vehicle collection reference `+0x58`, datum `+0x5C`.

Retail respawn duration precedence is recovered in
[`unknown_157450.cpp:1344`](../tools/reference/halo2-decompiled/src/unknown_157450.cpp#L1344),
`0x15AC50`: nonzero placement Spawn Time, otherwise nonzero `itmc`/`vehc` Spawn
Time, otherwise **30 seconds**. Example: Lockout first equipment record file
offset `0x01D27E04` is the battle rifle with placement spawn time `45`, collection
spawn time `0`; its effective base duration is `45`, not `30`. Variant respawn
time overrides, empty/depletion scheduling, dropped weapons, collection
replacement and pickup physics require the other retail equipment routines
before an exact scheduling claim.

Slayer scoring is recovered in
[`unknown_072c70.cpp:623`](../tools/reference/halo2-decompiled/src/unknown_072c70.cpp#L623),
`0x2BC000`: a credited enemy kill adds one to the killer; a betrayal subtracts
one from the killer; a self-kill subtracts one only when suicide point loss is
on; death point loss, if enabled, independently subtracts one from the victim.
Bonus points follow additional weapon/medal logic when enabled. The default
bits enable only suicide loss. The original score service clamps accumulated
scores to `[-30000,30000]`; see
[`unknown_157450.cpp:2103`](../tools/reference/halo2-decompiled/src/unknown_157450.cpp#L2103),
retail `0x15B7C0`. Avoid awarding points to AI/workbench targets as if they were
match players; the retail handler consumes player indices.

Death countdown is recovered in
[`unknown_157450.cpp:3606`](../tools/reference/halo2-decompiled/src/unknown_157450.cpp#L3606),
`0x15CE70`. Normal independent respawns use
`max(one-second minimum, pending penalty ticks + base respawn ticks)`. Suicide
first adds its configured penalty to the victim. Betrayal adds the configured
penalty to the killer's pending penalty, consumed on that killer's next death.
The victim keeps an ordinary countdown. The original virtual relationship
test at **`0x24DD70` returns teams differ**; the recovered local variable named
`same_team` is misleading here. Follow the executable branch rather than that
name. At the normal 30 Hz rate and defaults, ordinary respawn is 150 ticks,
suicide 300, and a betrayer with the pending penalty respawns in 450 ticks.
Team inheritance/synchronized respawns, finite lives, round transitions and
blocking conditions need their additional engine state; the oracle covers
only the independent countdown prefix.

## Native implementation boundaries

At the start of this research, the existing loader kept only player-start
position/facing/BSP, `Simulation::respawn` forced team 1 and retained its equipped
workbench weapon, and the frontend passed `sily +4` as its parameter fallback.
These paths must be separated from real match membership, initialized variants,
map-authored starting equipment and match death/respawn state. This document
records the evidence for that transition; it is not a claim that every subsystem
described here is implemented.

An initial native FFA can consume the inspected legal starts and original tags,
initialize the retail Slayer parameters, and maintain host-authoritative player
scores/countdowns while clearly naming any provisional movement/combat/spawn
policy. Multiplayer pickups, grenades, energy weapons, dual wielding, vehicles,
HUD/announcer, end-of-round rules and team variants remain distinct coverage
requirements. A native UDP transport must be explicitly documented as a native
protocol: **no Xbox packet ABI is inferred by this research**. Original System
Link handshake, session membership, simulation replication and Xbox security
interoperability require independent recovered-ABI and live-peer evidence.
