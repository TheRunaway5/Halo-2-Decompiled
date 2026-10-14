# Action Sack and Fiesta: original owner follow-up

Research checked 2026-10-13 for the requested2010 default and an unranked
Action Sack allowing couch guests. This changes no product code or enabled
playlist. Original weapon selection is recoverable without guessing; a complete
2010 Action Sack rotation and all required live gameplay are separate inputs.
The [mode/clan audit](native-online-mode-clan-audit-20261013.md) records normal
engine admission and presentation boundaries.

## New unchanged-original controls

The new [runner](../../.local/native-online-mode-clan-audit-20261013/oracle_fiesta_choices.py)
executes the **whole starting-weapon resolver** and **whole map-weapon mapper**
in the owned retail,TU1.1 andTU1.5 executables. Every executed instruction is
checked against the pinned image. The inputs are the actual18-entry Lockout
runtime weapon reference bytes plus declared global/tag-directory bindings,
seed values and options words. No instruction, original callee, game-service
return, RNG result or native implementation is substituted.

Each image passes21 direct-choice controls,768 random/exclusion/retry controls,
and323 Random Set controls compared with the same original mapper's derived
fixed-set result: **1,435 complete original calls per image,4,305 total**.
All three images produce identical selected definitions and complete actual
shared-seed store histories. The corpus covers every first-trial nibble,
every unique excluded runtime reference, NONE and both duplicate rocket
references, and all17 derived map weapon sets.
[Full original results](../../.local/native-online-mode-clan-audit-20261013/fiesta-original.json),
[retail executed instructions](../../.local/native-online-mode-clan-audit-20261013/retail-fiesta-executed.asm),
[TU1.1 instructions](../../.local/native-online-mode-clan-audit-20261013/tu11-fiesta-executed.asm),
[TU1.5 instructions](../../.local/native-online-mode-clan-audit-20261013/tu15-fiesta-executed.asm).
The runner also retains each exact416-byte starting-weapon code/switch-table/
alignment span before the independently checked next loadout-function prefix;
its hashes and scope are recorded per image in the same result JSON.

| Owner | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Starting-weapon choice | `19EDA0` | `1A2700` | `1A89B0` |
| Map-weapon set mapping | `15A640` | `15BC80` | `15D060` |

Pinned executable SHA-256 values are
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`,
and`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
respectively. Original loadout allocation, weapon gameplay, packet delivery,
2010 title-managed playlists and rendered frames are outside these controls.

Reproduce from the root:

```sh
.tools/python/bin/python .local/native-online-mode-clan-audit-20261013/oracle_fiesta_choices.py
```

## Random starting weapons: exact choices and RNG consumption

Serialized parameter89 (`0x59`) selects primary equipment in variant byteD6;
parameter94 (`0x5E`) selects secondary inD7. Their runtime option bytes are212
and213. The original resolver's option0 retains its supplied map default;
option1 returnsNONE; option2 draws Random. Direct options3–20 use a fixed
permutation into authored `mulg` runtime weapon references.
[Original decompile](../../tools/reference/halo2-decompiled/src/unknown_19eda0.cpp#L38),
[original field and full loadout evidence](../../docs/MP_INVENTORY_RESEARCH.md#initial-loadout-and-map-placements),
[fresh source `sily` records](../../.local/native-online-mode-clan-audit-20261013/fiesta-original.json).

One Random trial does:

```text
shared_seed = (shared_seed * 1664525 + 1013904223) modulo 2^32
choice = 3 + (shared_seed >> 28)
definition = runtime_weapon[permutation[choice - 3]]
retry when definition == NONE,
        or definition == excluded_definition and excluded_definition != NONE
```

The seed is the shared gameplay seed pointer at retail`4E7408`, TU1.1`4EE9D8`,
TU1.5`4F3CD8`. Retry stores are real draws from that same stream; do not use a
per-player, per-couch, wall-clock, cryptographic or separately reseeded generator.
Do not deduplicate or filter this pool before sampling.
[Actual executed code and stores](../../.local/native-online-mode-clan-audit-20261013/fiesta-original.json),
[source RNG helper](../../tools/reference/halo2-decompiled/include/unknown_0259d0.h#L140).

The16 trial IDs are decimal3–18. Lockout's actual mapping is:

| Choice | Runtime reference | Definition |
| ---: | ---: | --- |
|3|3|Battle Rifle|
|4|0|Magnum|
|5|1|SMG|
|6|6|Plasma Pistol|
|7|7|Plasma Rifle|
|8|5|Rocket Launcher|
|9|2|Shotgun|
|10|4|Sniper Rifle|
|11|12|Brute Shot|
|12|9|Needler|
|13|10|Carbine|
|14|13|Beam Rifle|
|15|11|NONE: retry|
|16|15|Same Rocket Launcher datum as choice8|
|17|14|Energy Sword|
|18|8|Brute Plasma Rifle|

The resulting nonempty pool has14 distinct definitions, but it is **not a
uniform draw over those14 weapons**. The two rocket trial positions remain
distinct; NONE and excluded results retry. Explicit choice19 selects runtime16,
Sentinel Beam, but **Random never generates19**. Explicit choice20 selects
runtime17,NONE in this cache. Runtime references are authored map data, and
their complete original per-map paths/datums must be preserved.
[Fresh original references and trials](../../.local/native-online-mode-clan-audit-20261013/fiesta-original.json).

Original loadout order is also significant. When primary option is2, resolve
secondary first with excludedNONE, then resolve primary excluding that secondary
definition. For other primary options, resolve primary first, then secondary
excluding primary. The full source loader first resolves scenario collections,
then these choices, then allocates primary and secondary in that order; actual
duplicate carried definitions are removed by the inventory owner. Choosing two
independent random values or always drawing primary first changes the stream
and loadout.
[Complete original TU1.1 loader](../../analysis/multiplayer/integration/inventory-source/instructions/001a28a0.asm),
[earlier executed choice controls](../../analysis/multiplayer/integration/inventory-source/choices.json),
[original inventory owners](../../docs/MP_INVENTORY_RESEARCH.md).

## Map Random Set is a different owner

Parameter83 (`0x53`, variantD4/runtime option210) is Weapons on Map.
Original `sily` choice19 is **Random Set**. The whole original mapper derives:

```text
value = ((options_word_10 XOR options_word_14) * 1664525 + 1013904223)
        modulo 2^32
selected_fixed_set = 2 + ((value >> 16) modulo 17)
```

It then runs the ordinary fixed-set mapping for the original weapon class.
This calculation does **not** write the shared gameplay seed. Every tested row
with the same options words selects the same underlying standard set2–18;
it is not a new Random starting-weapon draw per pickup/respawn. The source
first reverse-resolves the supplied definition into the runtime reference
array, preserving the first match of duplicate definitions. Unknown definitions
remain unchanged through its original early path.
[Whole original map-mapper corpus](../../.local/native-online-mode-clan-audit-20261013/fiesta-original.json),
[original TU1.1 mapper](../../analysis/multiplayer/integration/inventory-source/netgame/instructions/0015bc80.asm),
[original reverse lookup](../../analysis/multiplayer/integration/inventory-source/netgame/instructions/001601e0.asm).

These controls identify exact arithmetic over the two declared options words.
They do not reconstruct their complete lifecycle/seed-generation owner or
establish a source-faithful new-match sequence by choosing convenient values.

## Authored Fiesta is not a complete later playlist

Fresh retail `goof` row33 is named `Fiesta` plus the authored built-in glyph,
frontend engine1/source engine3 (**Oddball**), file offset`038D4198`. Its complete
override list is:

| Parameter | Value | Original setting |
| ---: | ---: | --- |
|3|120|Oddball score to win round: two minutes|
|89|2|Random primary starting weapon|
|94|1|No secondary starting weapon|
|83|19|Random map weapon set|

The original `goof` buffer and unchanged default/override controls own this
claim. This row is not evidence for a service-authored Fiesta Slayer, Team
Fiesta or Balltacular rule buffer. A matching word in a later website is not a
complete variant definition.
[Fresh metadata](../../.local/native-online-mode-clan-audit-20261013/fiesta-original.json),
[unchanged original row33 buffer](../../analysis/multiplayer/game-variants/original.json),
[source mapping](../../src/source_game_variant.cpp#L54).

Bungie's own2December2005 update announces Team ActionSack as an unranked
addition and explicitly points to the separate playlist page for detailed
rules. That establishes the feature's provenance, not a2010 complete pool,
numeric weights or downloaded variant bytes. The2009 article about bringing
Action Sack back discusses Halo3 and must not be used as a Halo2 rule table.
[Original Bungie2December2005 article, preserved mirror](https://www.halo.bungie.org/bwu/index.html?item=52),
[Bungie22May2009 article for the cross-game distinction](https://hbo.bungie.org/bwu/index.html?item=229).

The retained original playlist page, labeled2December2005 and captured
12December2005, supplies20 ActionSack variants/68 variant-map pairings. It
specifies unranked6–8 players,two teams,party1–8 and guests allowed. Its two
Fiesta-named rows are Phantom Fiesta on Foundation,Gemini,Lockout,Turf and
SWAT Fiesta on Ascension,Beaver Creek,Gemini,Ivory Tower. Those row names and
maps are primary evidence. They provide no complete preset buffer, every
editable setting, numeric weights or proof that the same pool survived to2010.
[Original capture, ActionSack lines1710–1802](../../.local/online-rank-research-20261011/bungie-playlists-20051212093137.html#L1710),
[dated original archived page](https://web.archive.org/web/20051212093137/http://www.bungie.net/Stats/page.aspx?section=FAQInfo&subsection=playlists&page=1).

The2010 Action Sack map/variant inventory and any exact TMS overrides remain
separate evidence requirements. An explicitly described native unranked party
playlist may use proven original random mechanics, but cannot claim a restored
complete2010 rotation without that dated primary catalog and runtime coverage.

## Native integration and weapon coverage gates

The current upper rules adapter only admits starting options0/5 and does not
admit secondary-equipment parameter94 or map-weapon-set parameter83 changes.
The ordinary native spawn path selects one rules weapon and initially fills
slot0; it does not call the Random resolver per successful player spawn.
The source-owned TU1.5 world can supply actual equipment through its existing
initializer/bridge, which should be reused with its genuine shared state,
not bypassed with a second cosmetic random inventory.
[Rules](../../src/multiplayer.cpp#L74),
[default starting-weapon adapter](../../src/multiplayer.cpp#L1764),
[native/source spawn join](../../src/multiplayer.cpp#L1011),
[source equipment bridge](../../src/multiplayer_object_runtime.cpp#L792),
[equipment synchronization](../../src/multiplayer.cpp#L1088).

Native map pickup rows currently admit single-item collections and default
source mapping. The source scene likewise registers the original single
definition. Full Random Set requires the original class mapping and selected
definition/variant to reach allocation, world art, inventory, replication,
pickup prompts and respawn while retaining original row indices and cadence.
Filtering to loaded/working weapon definitions changes the requested game.
[Native pickup owner](../../src/multiplayer_inventory.cpp#L45),
[world scene admission](../../src/source_netgame_weapon_scene.cpp#L28),
[original row owner and timing](../../docs/MP_INVENTORY_RESEARCH.md#initial-loadout-and-map-placements).

Required Random starting-weapon cases and their known bounded evidence/gaps:

| Weapons | Existing native evidence | Still required before claiming complete Fiesta coverage |
| --- | --- | --- |
|SMG|Recovered input/update/ammo and first-person owners|Complete independent hand/world/network/presentation sequences and all applicable editable-rule flow|
|Magnum,Battle Rifle,Carbine|Actual authored trigger/barrel timing and bounded ammo cycles; burst/selected barrel corrections|Complete reload/chamber/interruption, aim/error/damage/model regions and matched world/FP/audio/remote sequences|
|Shotgun,Sniper Rifle|Actual authored bounded firing cycles|Complete distribution/scopes/reload/chamber/contact/damage and presentation proof|
|Rocket Launcher|Bounded unguided firing/ammo cycles|Actual tracking/lock-on branches, projectile/impact/detonation and full action/presentation owners|
|Brute Shot|Bounded firing/ammo cycles|Original projectile bounce/contact/detonation, complete reload and presentation; current native projectile path ends at first contact|
|Needler|Recovered firing effects and projectile-birth owner|Guidance, sticking, supercombine and full contact/effect lifetime/presentation|
|Plasma Pistol|New heat/age core appears in current source|Actual charge/alternate trigger, homing, heat/vent effect lifecycle, battery/FP/world/network composition|
|Plasma Rifle,Brute Plasma Rifle,Beam Rifle|New heat/age core appears in current source|Complete cooling/overheat/vent services, battery/action/HUD/shader/dual where applicable and network tests|
|Energy Sword|Melee-only tag loading and native generic melee action|Original target/lunge/power/looping-sound, action/physics and matched presentation owners|

This table reports concrete owner gaps; it does not mark an entire weapon
complete because an ammo/timing comparison passes. The separate active
TU1.5 weapon goal explicitly covers all63 authored weapon paths and every
applicable gameplay/presentation owner. Source files are changing in that
work, so this research does not overwrite its progress or substitute an older
claim that all energy arithmetic is absent.
[Current weapon goal and explicit evidence bounds](../../docs/WEAPONS_TU15_PARITY_GOAL.md),
[current heat/age services](../../include/halo2/weapon_energy.hpp),
[live numeric heat path and missing persistent effect owner](../../src/simulation.cpp#L1095),
[first-contact projectile boundary](../../src/simulation.cpp#L795),
[sword action boundary](../../src/simulation.cpp#L980).

The match constructor still rejects an energy starting weapon, even while newer
numeric energy owners exist. Removing that guard without the reached services
and complete tests would hide the boundary. The original built-in Fiesta also
needs Oddball's actual objective lifecycle; the normal game-mode admission and
the Slayer-only Vista postgame variant bridge remain separate requirements.
[Starting weapon guard](../../src/multiplayer.cpp#L277),
[normal engine gate](../../src/multiplayer.cpp#L159),
[PCR bridge](../../src/vista_pcr_publication.cpp#L17).

Casual guest admission and guest-stat exclusion are server identity policies
already documented for couch play. Keep all couch seats together and retain
those restrictions when enabling any proven Action Sack games; their existence
does not waive the original weapon/engine/settings gates.
[Couch ONLINE contract](online-couch-players.md).
