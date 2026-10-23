# Equipment behavior and shading

Scope: the eleven equipment families resident in all 21 owned Xbox multiplayer
maps, using OG Xbox TU1.5 behavior and prepared MCC rendering assets. This is
an equipment acceptance matrix, not a whole-game or physical-Xbox framebuffer
parity certificate. The integrated acceptance run remains open below.

## Inventory and acceptance matrix

The original census contains 231 equipment definitions/models: seven ammo
families, both grenade pickups, overshield and active camouflage per map.
`analysis/equipment-20261022/assets.json` retains the original asset census.
The material regression checks 840 authored pickup shader LODs and 84
Spartan/Elite world/first-person overshield bindings against both original and
prepared MCC caches. It rejects absent original counterparts. MCC-only unused
residents, including its additional health-pack tag, are outside this original
multiplayer inventory.

| Equipment | Model/shader bindings | Behavior and inventory | Respawn | GPU model |
| --- | --- | --- | --- | --- |
| Battle rifle ammo | Pass | Matching-weapon run pending | Placed-row run pending | Pass |
| Pistol ammo | Pass | Matching-weapon run pending | Placed-row run pending | Pass |
| Needler ammo | Pass | Matching-weapon run pending | No authored multiplayer row | Pass |
| Shotgun ammo | Pass | Matching-weapon run pending | Placed-row run pending | Pass |
| SMG ammo | Pass | Matching-weapon run pending | Placed-row run pending | Pass |
| Sniper rifle ammo | Pass | Matching-weapon run pending | Placed-row run pending | Pass |
| Rocket launcher ammo | Pass | Matching-weapon run pending | Placed-row run pending | Pass |
| Fragmentation grenade pickup | Pass | Actual contact, cap and retained-item control pass | Pass | Pass |
| Plasma grenade pickup | Pass | Actual contact and inventory delta pass | Pass | Pass |
| Overshield | Pass | Actual pickup, charge, drain and expiry pass | All-map recheck running | Pass |
| Active camouflage | Pass | Actual pickup, timer, fade and expiry pass | All-map recheck running | Pass |

There are 16 placed powerup rows and 18 collection choices across the21 maps.
Coagulation's two mixed rows remain genuine collection permutations. The test
uses declared session seeds until each original choice has actually appeared;
it does not turn either row into two guaranteed spawns. The all-map cohort
passed before the newly recovered renderer initialization and is being repeated
with that initialization. Scenario respawn zero uses the original collection
delay, then the 30-second fallback. A zero field does not disable respawn.

Ammo type 0 and zero powerup duration do not make an ammo box inert. Original
player contact tries the weapon magazine transfer owner before powerup
handling. The matching-weapon regression follows the actual magazine reference,
fires and reloads a real weapon, verifies authored transfer quantities and
item deletion, fills the reserve through contacts, and checks that a full
reserve retains another box. It checks the six placed families on actual
Headlong/Turf rows and uses the original item constructor for the unused
Needler resident. Its application results are never assigned.

## Overshield implementation

The runtime previously prepared only damaged-shield banks and skipped the
overshield selector. Preparation now includes the distinct authored layer 15
overshield pass and both hlmt shader references. Body and arms submit the
selected overlays in original order, with their own resolved camouflage
inputs before authored animation. Existing damaged-shield banks remain separate.

Two original BAB40/animation comparisons each cover 240 cases. The ordinary
fixture compares 45,600 input/program/constant words; the full-camouflage
19CA0-before-17420 fixture compares 60,000 words. Native baseline words isolate
animation, so these are not independent static material assembly or Xbox pixel
proof. The model regression covers eight species/view banks, 28 geometry banks,
96 view selections and 1,976,040 unchanged posed vertices.

The shield GPU regression now passes with Vulkan validation, including ordered
world/FP callback equivalence, input response, local noise coordinates and
restoration after another owner's camouflage input. Original first-person
overshield program behavior is retained at full camouflage; no replacement
alpha rule is introduced.

The integrated contact path also reached an undeclared overshield player
response at TU1.5 48C778. Its five initialized writable words are now backed by
the exact original image data. The existing camouflage response owner remains
independent. `analysis/equipment-20261022/pickup-response-data.json` retains the
pinned image hash and exact response bytes.

## Camouflage implementation

The renderer now consumes the recovered original override pipeline:

- Positive original primary admits the clone, including values below the first
  fade-byte increment. Activation flags do not substitute for that scalar.
- The clone's selected global draw shader owns its initial mask and extern 26.
  The replaced armor shader's No Active Camo bit cannot omit cloned body parts.
- Original fade-byte selection chooses the authored layer 18 implementation.
  Target 19 is a current-frame four-tap scene copy, refreshed before each group.
- The authored layer 1 depth prepass and layer 18 blend/color constants are used.
  Ordinary body drawing is suppressed while the replacement is active.
- Conditional layer 19 uses the existing exact D24S8 color alias, depth EQUAL,
  blue-only destination-color multiplication and actual fog externs. Packed
  depth is preserved; the blue channel affects the source stencil byte.
- Per-owner and per-view bindings cover bodies, held weapons, hands and FP guns.
  Shield overlays follow the camouflage group. Expiry returns ordinary drawing.

Multiple completed color-alias phases can share one unsubmitted command/fence.
Another command, an unfinished phase, or ordinary weather reuse still fails its
ownership checks. The first phase's rollback state survives subsequent phases.
The GPU regression exercises two simultaneous fogged views and deterministic
reuse, in addition to partial/full camouflage, species, FP/world, reset and
unaffected-view controls. Full camouflage must remove opaque armor remnants.

Original fixtures now retain only the banks consumed by the regressions:
5,040 binder-state cases,420 input cases through deduplicated actual banks, and
64 complete pass returns out of 79 original pass-service cases. Fifteen explicit
original owner stops remain stops. Full raw oracle banks/listings are generated
only in disposable scratch. The compact fixtures total 881,223 bytes and are
required reference inputs; their hashes are in
`analysis/equipment-20261022/camouflage-fixtures.json`.

The live test exposed a null rasterizer pointer during original camouflage
model admission. Complete original 22570 new-map initialization now publishes
4922A0 from the resident matg Rasterizer Data block. Its reached cold debug/query
storage is backed by verified original zero-filled extents; no GPU-ready result
or shader pointer is preset. The original callback performs publication and
resets. Source evidence is retained in
`analysis/equipment-20261022/renderer-new-map-backing.json`.

The source contracts remain in `MP_CAMOUFLAGE_OVERLAY_SOURCE_RESEARCH.md`,
`MP_CAMOUFLAGE_PASS_SERVICES_SOURCE_RESEARCH.md`,
`MP_CAMOUFLAGE_PASS_SOURCE_RESEARCH.md` and
`MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md`.

## Integrated acceptance and validation limits

Pending: `native_mcc_equipment_ammo`, the final all-map recheck, and
`native_mcc_equipment_render`. The live renderer check requires actual source
pickups, both species, world/FP draws, partial and full camouflage, simultaneous
overshield layers, and expiry without a stale clone. Its callbacks observe
actual Vulkan camouflage draws, not a manually assigned activation result.

Nine additional camouflage/shield/scene-copy CTests pass. Their GPU cases use
the Vulkan validation layer. Both original camouflage bank regressions and all 11
pickup model GPU families also pass. Component/model evidence remains distinct
from the pending integrated runtime check. No physical-Xbox blend rounding,
physical-controller session, cross-machine match, or full map playthrough is
certified by these tests.

## Reproduction and storage

Use `tools/run_tests.py` for CMake validation and `tools/run_test.py` for any
one-off probe/oracle, as required by `TEST_STORAGE.md`. Relevant targets are
`equipment_material_tests`, `equipment_render_tests`,
`equipment_runtime_tu15_tests`, `equipment_ammo_runtime_tests`,
`equipment_live_render_tests`, `camouflage_render_tests`,
`source_shield_render_tests`, `source_shield_material_tests`,
`source_shield_model_shaders_tests`, `camouflage_state_material_tests` and
`camouflage_pass_bank_tests`.

Set CMake `HALO2_MCC_EQUIPMENT_MAPS` to the prepared MCC map directory and
`HALO2_MCC_EQUIPMENT_MAP` to its Headlong cache. GPU tests require a display,
Vulkan and the validation layer. Use `--test-jobs 1` for GPU validation. Reports
and command receipts are retained under `build/test-results/equipment-continuation-*`;
raw captures and private builds remain owned scratch and must be removed before
closing acceptance. Large artifact retention is not requested.
