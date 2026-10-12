# Imported multiplayer weapon effects and live loadouts

This freeze inventories the original weapon/effect/model metadata in all 21 imported multiplayer maps. It retains 696 resident weapon rows covering 36 distinct weapon paths, 822 referenced effect records and 756 model records. Twenty maps contain 33 weapon tags; Turf contains 36, including additional Phantom turret and Scarab main/AA weapons. These are authored definitions, not a claim that players can select or fire every imported weapon.

The [literal report](../analysis/multiplayer/integration/spawn-source/weapon-effects-inventory/measured/report.json), [readable 36-weapon index](../analysis/multiplayer/integration/spawn-source/weapon-effects-inventory/measured/authored-weapons.md) and [derived summary](../analysis/multiplayer/integration/spawn-source/weapon-effects-inventory/measured/summary.json) retain each map independently. Datums are map-specific. No animation clip name selects, substitutes or implies an effect.

## Actual live weapon admission

The isolated [CPU observer](../tools/inventory_mp_live_loadouts.cpp) links the immutable `build-kbm-20261008-201923` Cache/Match libraries and their matching headers. It opens the actual maps with the authenticated original u5 gameplay XBE, calls `load_content(0,true)`, resolves starting options0/5, constructs an ordinary native Slayer match, joins one player and delivers only held fire for90 ticks. It assigns no root, health, ammunition, pose, readiness, contact or weapon fields. This is a native loadout/fire smoke test; it neither executes the original whole engine nor renders a muzzle flash.

The [42 cases](../analysis/multiplayer/integration/spawn-source/weapon-effects-inventory/measured/live-loadouts.jsonl) produce:

| Cases | Actual result |
| --- | --- |
| Option0 on19 maps | Authored scenario Slayer collection selects SMG; match admitted |
| Option0 on Midship/Waterworks | Authored collection selects plasma rifle; match explicitly rejects battery/heat combat |
| Option5 on all21 maps | Actual `mulg` runtime weapon[1] selects SMG; match admitted |
| All40 admitted cases | 90 native ticks;45 actual `Simulation::shot` timestamp changes; ammunition60→15; same live full unit datum |

The importer admits 27 weapon definitions per ordinary map and30 in Turf. Six sword/objective weapon tags have no barrels and are skipped. The remaining imported non-SMG weapons are data-only for the ordinary current player loadout: variant admission accepts only0/5, and there is no live carried-weapon pickup/switch consumer selecting the other definitions. Their presence in `LevelContent.weapons` does not establish supported combat. Generic non-SMG simulation code is not equivalent to the original weapon managers.

The [source audit](../analysis/multiplayer/integration/spawn-source/weapon-effects-inventory/source-audit.json) and copied observed source/diffs distinguish the execution checkpoint from current work. The starting selector and capacity admission remain unchanged. Current work adds literal first-barrel firing references, strict SMG scalar admission, an optional prepared-effect callback, and separately configured world-item records. At this observation the live runtime does not configure the generic weapon-effect owner. These additions are not present in the older executable used for the40 smoke cases; no new live renderer result is claimed here.

## Literal references, locations and model markers

Each resident weapon record retains its complete31C-byte root, attachment block, all magazine/trigger/barrel rows and both authored first-person records. Every record has file offset, original metadata address and raw bytes. In particular:

| Metadata | Literal retained fields |
| --- | --- |
| Weapon+38 | Own `hlmt` reference and its actual `mode` reference |
| Weapon+94 | All18-byte attachment rows: type reference and marker StringID |
| Weapon+140/+148 | Ready effect/damage references |
| Weapon+2A8 | All10-byte FP model/animation records, including NONE models |
| Weapon+2C0 | All5C-byte magazines, including reload+34/+38 and chamber+44/+48 references |
| Weapon+2C8 | All40-byte triggers, including charging+30/+34 and charging-damage+38/+3C references |
| Weapon+2D0 | AllEC-byte barrels, flags, optional marker+30, projectile reference and illumination-recovery+A0 word |
| Barrel+E4 | All34-byte firing rows: signed-short count bounds, normal/misfire/empty effect and damage references |
| Effect+C/+14 | Actual location StringIDs and38-byte events; exact delay/duration words, parts and systems |
| System+8/+C/+E/+10/+12/+16 | Location, coordinate system, environment, disposition, camera mode and flags; allB8-byte emitter rows |
| Model+48/+58 | All60-byte nodes and allC-byte marker groups/24-byte marker rows |

The Assembly schema pins provide descriptive layout labels. The raw cache bytes and original instructions provide authority. Schema labels alone do not establish unexecuted behavior. Camera-mode integers are retained literally:0 independent,1 first-person,2 third-person,3 both according to that schema. The prior executed stock-SMG effect owner independently proves its actual camera-specific records; this inventory does not execute all other weapons' camera gating, effects or particle births.

Normal-reference absence is explicit for `energy_blade`, `energy_blade_useless`, `flag`, `ball`, `juggernaut_powerup`, `assault_bomb` and `warthog_horn`. The first six have no barrels; the horn has two barrels with no firing-effect rows. This means no authored **normal firing reference** in those rows. Ready effects or attachments, including sword visuals and bomb lights, remain separately recorded. Direct sound references are also distinct: for example the Banshee secondary normal firing entry references a sound, rather than an `effe`. No generic flash is substituted for an absent/direct-sound reference.

The plasma pistol charging slot contains a `jpt!` tag reference; the report preserves its actual group and datum rather than converting its descriptive schema slot into a visual effect. Normal, misfire, empty, charging, reload, chamber and attachment references remain separate phases. Native `SmgDefinition.preparation_effect` is a boolean derived from barrel+A0 illumination recovery; it is not an authored effect-tag reference.

Some vehicle weapons have no own world render model. The source106070 parent-selection boundary matters there: this inventory records NONE and does not manufacture a parent vehicle marker, root transform or attachment location. Retained marker translations/quaternions are local authored rows, not world-space poses. Actual source current nodes, representation/FP mapping, parent ownership and visibility are required to place an effect.

## Stock SMG and prepared-effect admission

All21 authored SMG roots have **weapon+154=`00000000` and weapon+25C=`00000000`**. The [21-row scalar assertion](../analysis/multiplayer/integration/spawn-source/weapon-effects-inventory/measured/summary.json) retains each actual datum and both words. These exactly meet the new source107160 prepared-effect scalar/misfire admission. The older smoke executable remains unchanged; this separate raw-cache assertion validates the new input gate.

Lockout SMG normal reference is actual `effe E59E0429`, `effects\\objects\\weapons\\rifle\\smg\\fire_bullet`, from weapon `E57B0406`. Its location is StringID `0F0000DB`, `primary_trigger`. The authored camera modes select one first-person system and three third-person systems, in literal order1,2,2,2. Names are labels; the references and camera fields are the evidence. The event has zero delay and duration words `3D088889/3D088889`; these do not establish particle lifetime.

The distinct Lockout world/FP primary-trigger marker rows differ by one Z ULP: world `3CD8BD6B`, FP `3CD8BD6C`; X=`3DCE498E`, Y=0. Both use node0, region/permutationFF, quaternion words0,0,0,`BF800000`, and marker scalar word0. The tool retains these rows literally, without a scale fallback or world→FP substitution.

Original execution and native consumer boundaries remain in the immutable [SMG effect owner](MP_SMG_EFFECT_OWNER_RESEARCH.md) and [local effect owner](MP_SMG_LOCAL_EFFECT_OWNER_RESEARCH.md) freezes. Generic event/particle construction, current local/world marker transforms, visibility, persistent lifetimes and rendering must be composed with their real owners. An authored tag or shot counter alone does not supply those owners.

## Complete original owning instruction evidence

The [eight complete TU1.1 listings](../analysis/multiplayer/integration/spawn-source/weapon-effects-inventory/measured/instructions/report.json) contain1090 unchanged instructions. This extension is static evidence, separate from the previously executed stock-SMG owner:

| Owner | Exact reference read/call evidence |
| --- | --- |
|1040F0 |104251 reads weapon+144;10425F calls107B50 (ready) |
|106C30 |106D15 reads magazine+38;106D20 calls107B50 (reload) |
|106E70 |106ED6 reads magazine+48;106EE2 calls107B50 (chamber) |
|107E90 |107EF8 reads trigger+34;107F01 calls107B50 (charge) |
|107160 |Complete firing selection/prepared callback body;107B30 calls107B50 |
|1099F0 |First barrel/first effect empty-reference path at entry+18 |
|106070 |Actual weapon/retained-parent object selection |
|107B50 |NONE→NONE; `effe` refresh/create, `snd!` sound; other groups→NONE |

Whole-body TU1.5 correspondence for106070/107B50 and the generic request/visibility owners is already independently frozen in [SMG effect owner u5 correspondence](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/u5-correspondence/report.json). The new ready/reload/chamber/charge listings here are TU1.1 evidence; no unverified TU1.5 address is inferred.

## Reproduction and immutable audit

[inventory_mp_weapon_effects.py](../tools/inventory_mp_weapon_effects.py) reads assets without modification. [summarize_mp_weapon_effect_inventory.py](../tools/summarize_mp_weapon_effect_inventory.py) verifies all21 SMG scalar pairs,42 native loadout results,40 exact fire/ammunition results and36 weapon identities before producing the index. Full command arguments are in [reproduction](../analysis/multiplayer/integration/spawn-source/weapon-effects-inventory/REPRODUCE.md) and [the recorded native argv](../analysis/multiplayer/integration/spawn-source/weapon-effects-inventory/live-loadout-argv.json). The CPU probe uses no SDL window, controller, GPU or shared CMake change.

Every measured output, eight listings, summary and native stdout/stderr reproduces byte-identically:14 files. The [repeat audit](../analysis/multiplayer/integration/spawn-source/weapon-effects-inventory/repeat-audit.json) and [manifest](../analysis/multiplayer/integration/spawn-source/weapon-effects-inventory/manifest.json) pin tools, assets, original executables, checkpoint libraries/manifests, observed current source and relevant prior evidence. Original cache hashes also remain identical across the two full reads. No prior freeze or shared production file was edited.

Literal report SHA-256: `2c9b5688d88eb8aeb2df58ff396729cf02d832c6b3c2bbbd99757f96837a0443`. Native42-case output: `c26e86fed68306576ae1f83cbf63631b669e336c478db2c493e8a24b5e8d3f7c`. Summary: `3a54f9a18e870cd261c7463752295f6217b5035ee3a5a21a59ba7751243544a3`.
