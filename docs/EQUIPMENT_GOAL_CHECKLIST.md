# Equipment behavior and shading

Requested scope: all equipment works and uses its authored shading, including
overshield and active camouflage. This checklist remains open until ordinary
pickups, state changes, expiry/respawn and rendering pass together with prepared
MCC assets. An asset census or explicit powerup grant is not that acceptance.

## Authored inventory

`tools/audit_equipment_assets.py` reads all 21 owned Xbox multiplayer caches.
There are 231 equipment definitions/models: seven ammo types, both grenade
types, overshield and active camouflage per map. Type 0 is the actual authored
value on the ammo definitions, rather than an invented powerup category.
Collection permutations remain separate choices on their original scenario row;
Coagulation's mixed collections must not become two guaranteed powerup spawns.
The bounded census is `analysis/equipment-20261022/assets.json`.

| Equipment | Authored model and shader LOD binding | Actual pickup/application | Ordinary respawn | GPU appearance |
| --- | --- | --- | --- | --- |
| Battle rifle ammo | Pass | Open | Open | Open |
| Pistol ammo | Pass | Open | Open | Open |
| Needler ammo | Pass | Open | Open | Open |
| Shotgun ammo | Pass | Open | Open | Open |
| SMG ammo | Pass | Open | Open | Open |
| Sniper rifle ammo | Pass | Open | Open | Open |
| Rocket launcher ammo | Pass | Open | Open | Open |
| Fragmentation grenade pickup | Pass | Headlong contact/cap control passed | Open | Open |
| Plasma grenade pickup | Pass | Headlong contact passed | Open | Open |
| Overshield | Pass | Headlong pickup/charge/drain passed | Open | Authored overlay inspection passed |
| Active camouflage | Pass | Headlong pickup/timer/fade passed | Open | Biped refraction/stencil path open |

The all-map material regression passes 231 real models, 840 authored pickup
shader LODs and 84 Spartan/Elite first-person/world overshield bindings. This
checks actual resident resource binding; it does not certify the pickup models'
live lighting, pose, transparent ordering or framebuffer appearance.
The same cohort passes against the prepared MCC cache. That comparison matches
the original per-map equipment tag paths and rejects missing counterparts.
Unused MCC-only residents, such as its additional health-pack tag, are outside
this original multiplayer cohort and are not certified by the result.

## Overshield correction

The native renderer prepared only damaged-shield banks and explicitly skipped
`ShieldOverlayKind::Overshield`. The shield override binder also rejected the
distinct authored `plasma/overshield` pass. The new regression first failed at
that actual binder, as retained in `overshield-red.json`.

The binder now admits the original single layer-15 implementation, including
its exact scalar-37 alpha remap, volume textures and local-coordinate vertex
program. Runtime preparation includes both authored shield references; body and
arms submit each selected overlay in original selector order. Per-player banks
receive their own resolved camouflage value before authored animation. The
ordinary body collector recognizes the already-submitted overshield override.

The original retail `BAB40` input and shader animation owners independently
match all 240 new cases / 45,600 input and animated program/constant words.
As in the existing damaged-shield differential, native baseline words isolate
animation; this is not independent static material assembly or pixel proof.
The fixture is `tests/fixtures/overshield_material_original.tsv` and its receipt
is `analysis/equipment-20261022/overshield-material-original.json`.

The expanded model regression covers eight damaged/overshield body/arms banks,
28 authored geometry banks, 96 view selections and 1,976,040 unchanged posed
vertices. GPU and actual pickup results must be recorded separately below.

The subsequent GPU inspection passes all eight damaged/overshield species/view
cases: deterministic RGB, shield-input response, retained local noise
coordinates, ordered world/FP callback equivalence and restoration after a
different owner's camouflage input. It measures 455,536 input-response and
59,325 local-coordinate control RGB bytes. This explicitly runs **without the
unavailable Vulkan validation layer**, using a fresh current material binder
and a read-only frozen renderer dependency set whose renderer ABI headers and
renderer source match the current checkout. This is an inspection harness,
not a rendered pickup in an ordinary played match or an Xbox pixel comparison.

An additional original19CA0-before-17420 comparison matches 60,000 words for
240 full-camouflage overshield cases. Both original shader variants receive the
remap. In this GPU inspection the first-person shader's RGB remains unchanged;
the authored program is preserved rather than forcing an invented alpha rule.
The outer camouflage clone/composition remains a separate missing consumer.

The native Headlong contact cohort passes both grenade types and both powerups.
It records actual deleted grenade generations and matching inventory deltas;
the frag contact consumes two neighbors first and reaches its authored maximum
of four, correctly retaining the selected remaining item. Overshield reaches
2.9925923 after its three-layer charge and then drains to 2.9703693; camouflage
reaches primary 0.2583334 with timer 1319 in the declared sample. The initial
current-build powerup/plasma results and the corrected frozen-engine contact
composition are retained separately. No application result is assigned.

The focused full build completed. Seven of eleven selected CTests passed;
two existing camouflage material checks lack their original saved reports,
and two GPU checks require the missing validation layer. The explicit
non-validation GPU inspection above supplies RGB evidence but does not convert
those unavailable checks into passes. Runtime routing passes syntax compilation;
no launcher-selected executable or full played-match acceptance is claimed.

## Camouflage boundary

Existing native timer/fade/disruption and weapon scalar-37 consumers remain
separate from the missing biped override pipeline. Original `4BCC0` selects
the global camouflage shader when primary is positive; `133AB0` creates its
clone and suppresses ordinary collection. The authored layer-18 and layer-19
passes require their current scene-copy, color/depth alias, stencil, fog and
view bindings. The current gameplay renderer has no submitted biped camouflage
replacement. Giving an opaque body a transparency value would not implement
those owners.

The original contracts and remaining boundaries are recorded in
`MP_CAMOUFLAGE_OVERLAY_SOURCE_RESEARCH.md`,
`MP_CAMOUFLAGE_PASS_SERVICES_SOURCE_RESEARCH.md` and
`MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md`. No variant admission or
camouflage completion claim changes here.

## Reproduction

Run all generated binaries and captures with the disposable workflows in
`TEST_STORAGE.md`. Focused targets include `equipment_material_tests`,
`equipment_runtime_tu15_tests`, `source_shield_model_shaders_tests`,
`source_shield_material_tests` and `source_shield_render_tests`.

`native_mcc_equipment_pickups` takes `HALO2_MCC_EQUIPMENT_MAP`, a prepared MCC
Headlong map containing both powerups and both grenade types. Its only placement
stimulus moves a genuine source player to an existing authored pickup through
original `B95D0`. Ordinary frames must consume the item and produce actual
inventory/timer/fade/charge/drain changes. It assigns no application result.
This contact fixture does not establish player traversal to the pickup,
respawn scheduling, multiplayer replication or visual acceptance.

The original material CTest is `original_equipment_materials`; its MCC
counterpart is `mcc_equipment_materials`, using the prepared map directory in
`HALO2_MCC_EQUIPMENT_MAPS`. `original_overshield_material` checks the new frozen
original animation fixture through the existing shield material executable.
