# Weapon TU1.5 remaining-work checklist

Updated: 2026-10-19. Goal status: **active, incomplete**.

This is the operational checklist for the weapon goal. Read it together with
[the current goal state](../analysis/weapons-tu15-goal-20261013/goal-current.json)
when resuming. The [parity goal](WEAPONS_TU15_PARITY_GOAL.md) retains the detailed
contracts and historical findings. The current state points to the latest
published per-weapon matrix and any active validation; check that state before
starting another build. Historical reports are evidence, not a new task list.

**Completion rule:** all applicable gates for every scoped weapon must pass.
Check off a task only after its stated acceptance evidence exists. Update this
checklist, the affected matrix rows and current goal state as part of completing
that task. Record failures and their next action immediately; preserve unchecked
work across continuations. A component fixture, source build, Vulkan draw command
or one successful weapon sequence does not complete an entire weapon.

## Current position

- Xbox reference scope: **63 weapon paths, 1,110 authored occurrences, 38 maps,
  630 weapon gates**. The latest published matrix has **85 partial and 545
  unverified gates; zero complete gates and zero fully certified weapons**.
- Prepared MCC inventory: **43 cache files, 1,479 weapon rows, 66 distinct paths**.
  All63 Xbox reference paths are present. This is directory identity coverage.
- The previous four-frame BR/SMG/turret application result used the ISO cache;
  retain it as reference evidence. It is **not MCC runtime acceptance**.
- The first MCC-only Release build passed. Native HUD/shared-copy checks and
  one/four-player MCC simulations pass. Campaign `ai_place` fails at original
  `1E7F18`, missing backing `47D6C0`; source submission and application acceptance
  remain open. All owned source/build scratch is verified removed.

## Fixed boundaries

- Runtime testing uses **our prepared MCC cache**. No Xbox ISO or Vista cache
  fallback, reference-tag injection or supplied gameplay result is allowed.
- Xbox TU1.5 remains the behavior and visual authority. Inspect authenticated
  original source/assets/execution for discrepancies; do not guess or tune away
  a failed comparison.
- Future tag conversion belongs to MCC import. Exercise the actual delivered
  import/loader output and expose unsupported definitions honestly.
- Include campaign/AI, vehicles/seats, fixed turrets, objectives, unused variants,
  applicable species, both hands, split-screen and LAN. Mark a gate inapplicable
  only with evidence identifying its original owner and rule.
- Preserve unrelated work, original assets, existing reference evidence, frozen
  inputs and launcher-selected runtimes.

## Verified preparation

- [x] Enforce MCC cache headers and record exact runtime input hashes; reject
  real ISO and synthetic non-MCC/Vista-format header controls before runtime.
  [Receipt](../analysis/weapons-tu15-goal-20261013/mcc-cache-input-admission-v1.json).
- [x] Read all43 MCC cache directories and locate all63 reference weapon paths.
  [Inventory](../analysis/weapons-tu15-goal-20261013/mcc-authored-weapon-scope-v1.json).
- [x] Check syntax of the four affected C++ consumers and MCC helper scripts.
  Probe scratch is removed; this is not a runtime or parity result.
  [C++](../analysis/weapons-tu15-goal-20261013/mcc-weapon-test-syntax-v1.json),
  [helpers](../analysis/weapons-tu15-goal-20261013/mcc-test-helper-preflight-v2.json).

## Resume in this order

- [x] Finish the first MCC Release build and inspect each test outcome. Four
  MCC startup/simulation checks and four offline reference checks pass; campaign
  source submission fails and application acceptance does not run successfully.
  Preserve that failure and the immutable source/MCC hashes.
  [Results](../analysis/weapons-tu15-goal-20261013/weapon-mcc-native-release-validation-v1/release/tests.log).
- [x] Verify native MCC map/HUD startup and actual shared-copy behavior without
  an Xbox executable world constructor. The real authored HUD address and704
  overlapping-copy cases pass; no source return or tag address is supplied.
  [Native HUD receipt](../analysis/weapons-tu15-goal-20261013/weapon-mcc-native-release-validation-v1/release/evidence/weapon-campaign-native-hud.json).
- [ ] Resolve the actual MCC campaign `ai_place` failure: root `2097E0`, original
  instruction `1E7F18`, missing runtime backing `47D6C0`. Identify the original
  owner/initialization contract and reproduce it; do not inject the wanted result.
- [ ] Verify actual MCC campaign AI-held weapon submission against independent
  section/WORLD-pose/shader/lighting inputs, then the finite application path.
  Require the real expected owners and definitions in every submitted frame.
- [ ] Establish the MCC camera/scene case and expand coverage to loose BR/SMG,
  fixed-turret items and per-instance baked/dynamic lighting. Do not transplant
  the historical ISO dress-cap camera or datum assertions as MCC evidence.
- [ ] Recheck actual MCC first-person/dual-player sequences, sustained firing,
  reloads and authored sound events. The earlier ISO SMG firing case's zero
  authored sound submissions remains unresolved; nonzero callback PCM alone
  does not establish correct firing-sound identity, timing or ownership.
- [ ] Review the three additional MCC paths (`smg_silenced`, `gpmg`, Falcon
  `rpg`) for Xbox-scope applicability and actual ownership. Record the decision
  and evidence; neither silently omit them nor invent Xbox behavior for them.
- [ ] Continue the feature, visual and owner gates below across all63 reference
  weapon paths, fixing each reproduced discrepancy against its original owner.

## Required gates for every applicable weapon

Each item maps to the named gate in the per-weapon matrix. These boxes remain
open until **every applicable weapon/map/owner variant** passes that gate.

- [ ] `authored_contracts`: actual MCC import resolves each original definition,
  map variant, salted reference and TU1.5 balance operand; reject unsupported
  data rather than borrowing a foreign cache's datum or metadata.
- [ ] `trigger_resources`: complete barrel/trigger and burst/release scheduling;
  ammunition/reserve, chamber/reload/ready, interruption, battery, heat/venting,
  charge/overcharge, cancellation and their actual source publications.
- [ ] `aim_damage`: original aim/error/recoil, damage/shield/body/headshot/model
  regions and applicable melee/sword-lunge behavior through their actual owners.
- [ ] `projectiles`: actual creation/ownership, travel, guidance/rocket tracking,
  contact, attachment/sticking, ordinary/charged detonation, Needler combine and
  retirement; preserve source timing, precision and ordered effects.
- [ ] `inventory_lifetime`: real pickup/equip/switch, independent dual hands,
  reload arbitration, holster/cancel/drop, transfer and world retirement.
- [ ] `owning_game_rules`: real player, campaign AI, vehicle/seat, turret or
  objective owner; original spawning, game rules, BSP/map changes and scheduling.
  Reuse the working MCC native startup/audio path; implement lower-level hardware
  ownership only where a reproduced dependency actually requires it.
- [ ] `poses_models`: first/third-person and world models, all applicable species
  and hands, regions/LOD/permutations, complete animation/transition/interpolation,
  IK/attachments, scope/camera and owner changes.
- [ ] `materials_effects`: original textures/mips, shader inputs/program choices,
  lighting/colors/transparency/order, charge/fire/impact/detonation effects,
  particles/decals/glow/contrails and natural start/stop/drain.
- [ ] `hud_audio_publication`: ammo/battery/heat/charge/lock/scope/crosshair,
  firing/reload/charge/loop/stop/impact sound identity and timing, first/third-person
  perspective, split-screen isolation and actual LAN/remote event histories.
- [ ] `full_reference_sequence`: complete native MCC weapon sequences compared
  against substantiated Xbox TU1.5 visual/audio evidence, including interruption,
  adverse/edge cases and every applicable map/owner variant. CPU equality and
  GPU command submission cannot stand in for visible/audible comparison.

## Per-weapon completion

Every entry stays unchecked until all ten gates pass or have substantiated
inapplicability in its matrix row. Bounded existing progress remains in the
matrix; no entry is currently complete.

### Character weapons (8)

- [ ] `objects\characters\hunter\hunter_particle_cannon\hunter_particle_cannon`
- [ ] `objects\characters\prophet\weapons\gravity_cannon\gravity_cannon`
- [ ] `objects\characters\sentinel_aggressor\weapons\beam\sentinel_aggressor_beam`
- [ ] `objects\characters\sentinel_aggressor\weapons\beam_elim\sent_agg_beam_elim`
- [ ] `objects\characters\sentinel_aggressor\weapons\welder\sentinel_aggressor_welder`
- [ ] `objects\characters\sentinel_enforcer\weapons\beam\sentinel_enforcer_beam`
- [ ] `objects\characters\sentinel_enforcer\weapons\needler\sentinel_enforcer_needler`
- [ ] `objects\characters\sentinel_enforcer\weapons\rocket\sentinel_enforcer_rocket`

### Vehicle and seat weapons (20)

- [ ] `objects\vehicles\banshee\weapon\banshee_gun`
- [ ] `objects\vehicles\c_turret_ap\weapon\big_needler`
- [ ] `objects\vehicles\c_turret_ap\weapon\big_needler_handheld`
- [ ] `objects\vehicles\creep\plasma_turret\weapons\creep_turret`
- [ ] `objects\vehicles\ghost\weapon\ghost_gun`
- [ ] `objects\vehicles\h_turret_ap\weapon\h_turret_ap`
- [ ] `objects\vehicles\pelican\pelican_rear_gun\weapon\pelican_rear_gun`
- [ ] `objects\vehicles\pelican\pelican_rocket_pod\weapon\pelican_rocket_pod`
- [ ] `objects\vehicles\pelican\weapons\pelican_chin_gun`
- [ ] `objects\vehicles\phantom\plasma_turret\weapons\phantom_turret`
- [ ] `objects\vehicles\phantom\plasma_turret\weapons\phantom_turret_handheld`
- [ ] `objects\vehicles\scorpion\turrets\cannon\weapon\cannon_turret`
- [ ] `objects\vehicles\scorpion\turrets\cannon\weapon\cannon_turret_mp`
- [ ] `objects\vehicles\spectre\plasma_turret\weapon\plasma_turret`
- [ ] `objects\vehicles\warthog\turrets\chaingun\weapon\chaingun_turret`
- [ ] `objects\vehicles\warthog\turrets\gauss\weapon\gauss_turret`
- [ ] `objects\vehicles\warthog\weapon\warthog_horn`
- [ ] `objects\vehicles\wraith\turrets\minigun\minigun_turret`
- [ ] `objects\vehicles\wraith\turrets\mortar\mortar_turret`
- [ ] `objects\vehicles\wraith\turrets\mortar\mortar_turret_mp`

### Fixed turrets and their items (4)

- [ ] `objects\weapons\fixed\h_turret_mp\item\h_turret_mp_item`
- [ ] `objects\weapons\fixed\h_turret_mp\weapon\h_turret_mp_weapon`
- [ ] `objects\weapons\fixed\plasma_cannon\item\c_turret_mp_item`
- [ ] `objects\weapons\fixed\plasma_cannon\plasma_cannon`

### Melee and utility weapons (6)

- [ ] `objects\weapons\melee\energy_blade\energy_blade`
- [ ] `objects\weapons\melee\energy_blade\energy_blade_useless`
- [ ] `objects\weapons\melee\gravity_hammer\gravity_hammer`
- [ ] `objects\weapons\melee\jackal_shield\jackal_shield`
- [ ] `objects\weapons\melee\pike\pike`
- [ ] `objects\weapons\melee\unarmed\unarmed`

### Objective weapons (6)

- [ ] `objects\weapons\multiplayer\assault_bomb\assault_bomb`
- [ ] `objects\weapons\multiplayer\ball\ball`
- [ ] `objects\weapons\multiplayer\ball\head`
- [ ] `objects\weapons\multiplayer\ball\head_sp`
- [ ] `objects\weapons\multiplayer\ball\juggernaut_powerup`
- [ ] `objects\weapons\multiplayer\flag\flag`

### Handheld firearms (15)

- [ ] `objects\weapons\pistol\magnum\magnum`
- [ ] `objects\weapons\pistol\needler\needler`
- [ ] `objects\weapons\pistol\plasma_pistol\plasma_pistol`
- [ ] `objects\weapons\rifle\battle_rifle\battle_rifle`
- [ ] `objects\weapons\rifle\beam_rifle\beam_rifle`
- [ ] `objects\weapons\rifle\beam_rifle\beam_rifle_noplayer`
- [ ] `objects\weapons\rifle\brute_plasma_rifle\brute_plasma_rifle`
- [ ] `objects\weapons\rifle\covenant_carbine\covenant_carbine`
- [ ] `objects\weapons\rifle\plasma_rifle\plasma_rifle`
- [ ] `objects\weapons\rifle\shotgun\shotgun`
- [ ] `objects\weapons\rifle\smg\smg`
- [ ] `objects\weapons\rifle\sniper_rifle\sniper_rifle`
- [ ] `objects\weapons\support_high\flak_cannon\flak_cannon`
- [ ] `objects\weapons\support_high\rocket_launcher\rocket_launcher`
- [ ] `objects\weapons\support_low\brute_shot\brute_shot`

### Tower and Scarab weapons (4)

- [ ] `scenarios\objects\covenant\military\cov_guntower\cov_guntower_turret\weapon\cov_guntower_mortar`
- [ ] `scenarios\objects\covenant\military\scarab\weapons\aa_guns\scarab_aa_gun`
- [ ] `scenarios\objects\covenant\military\scarab\weapons\main_gun\scarab_main_gun`
- [ ] `scenarios\objects\covenant\military\scarab\weapons\main_gun\scarab_main_gun_handheld`

## Final integration and delivery

- [ ] Run coherent current-source MCC regression and real application coverage
  for all applicable player counts, local/remote owners and campaign/MP maps.
  Validate the actual launcher-selected/delivered runtime and converted content;
  isolated test artifacts do not establish delivery.
- [ ] Complete Xbox-reference visual/audio comparisons for the required full
  sequences; record remaining limits explicitly and leave affected gates open.
- [ ] Audit all63 matrix rows/630 gates and the three MCC applicability decisions.
  Require zero unresolved applicable gates and complete per-weapon evidence.
- [ ] Update this checklist, the matrix, goal state and goal contract together.
  Mark the persistent goal complete only when no required work remains.

## Storage cleanup is part of the goal

- [ ] For the active validation and each later run, use `tools/run_tests.py` for
  CMake and `tools/run_test.py`/`HALO2_TEST_SCRATCH` for probes and captures.
- [ ] After each run exits, verify removal of its owned build/source/scratch
  trees; retire unused one-off helpers and redundant rebuildable artifacts.
  Preserve small commands, hashes, reports and failure diagnostics.
- [ ] Before final completion, remove confirmed unused test/build outputs after
  checking processes, links, references and launcher runtime selection.
- [ ] Record removed paths/recovered bytes and every explicitly retained large
  artifact's path, size, hash and reason. Preserve required evidence and inputs.

These cleanup boxes describe ongoing/final obligations. Individual completed
run receipts do not check off cleanup for future runs.
