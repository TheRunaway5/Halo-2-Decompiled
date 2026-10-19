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
- The latest MCC-only Release run (v9) passes nine of ten checks. Native actor
  placement and held-weapon source submission now pass:2,358 vertices, three
  source shader inputs and seven model rejection controls. The real application
  now completes public HS `ai_place`, loads objective strings, initializes the
  RTX 5080 renderer and runs frame-zero simulation. The node-map conversion
  clears the original `23644`/address `0x2` failure in the actual application.
  Both node-map prerequisites pass: 906 projected MCC node-map resources, all five
  complete original hinge headers, and canonical delivery with 112,779 pointer
  fields and 69,887,237 owned bytes verified. V9 also clears the `22D78` PRT
  failure after restoring all 95 fixed read-only inputs. It reaches actual
  turret-item, BR and SMG source models, then stops on material producer
  `17000578`: MCC's `primary_ammunition_tens`, whose Xbox ID is `1700056C`.
  Paired original/MCC string tables show that other built-ins also differ.
  Preserve authored function identities and bind actual MCC names to the
  original producers; the digit producer itself still needs implementation.
  Separately, the axis lookup's two read sites pass original index bounds and
  native 36-byte ownership checks. All 24 original bitmap bit-depth fields are
  restored and pass focused controls; full MCC revalidation remains required.
  Completed run/probe scratch is removed; four submitted application frames
  remain required.

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

- [ ] Finish the actual MCC application path through public HS `ai_place`.
  V6/v7 now place the actor through the public route. The former missing table
  word`209D04` at`2098A6` is cleared by all three original campaign-runtime CFG
  tables (72 bytes total); source branches and indices are unchanged. All56
  original selector/bounds cases pass. The objective-locale correction now
  clears actual MCC startup, and the RTX 5080 renderer initializes. Latest
  blocker: V9 clears both `23644` and the later PRT `22D78`/`46A680` failure.
  It reaches real turret-item, BR and SMG source models, then requests MCC
  `17000578` (`primary_ammunition_tens`). The original handler's ID is
  `1700056C`; the actual paired string tables show other built-in mismatches.
  Bind the correct authored namespace without changing alias/function identity,
  recover the complete original magazine-digit producer and test actual runtime
  magazine states. Never substitute zero or reference-tag data. The shared
  `48C754` policy retains its earlier renderer owner and writable value1.
  Require real expected owner/definition/constant/vertex bindings in all four
  submitted application frames; no frames have passed yet.
  [Original table](../analysis/weapons-tu15-goal-20261013/mcc-public-ai-place-table-frontier-v1.json),
  [table export](../analysis/weapons-tu15-goal-20261013/campaign-runtime-tables-export-v1.json),
  [locale diagnosis](../analysis/weapons-tu15-goal-20261013/mcc-pause-objective-locale-frontier-v1.json),
  [resource preflight](../analysis/weapons-tu15-goal-20261013/mcc-pause-objective-resource-preflight-v3.json),
  [original model-render frontier](../analysis/weapons-tu15-goal-20261013/mcc-model-render-23644-original-frontier-v2.json),
  [actual material frontier](../analysis/weapons-tu15-goal-20261013/mcc-weapon-material-17000578-original-frontier-v1.json),
  [paired StringIDs and handler](../analysis/weapons-tu15-goal-20261013/mcc-weapon-material-string-id-correspondence-v1.json).
- [ ] Complete the remaining independently bounded indexed inputs. Of the13
  original read sites, two axis sites are proven. The six swizzle reads still
  need actual caller dimension bounds and a table extent. The five bitmap
  reads have all24 supported input cells; malformed/foreign format admission
  and actual MCC consumer execution remain separate requirements.
- [ ] Establish the MCC camera/scene case and expand coverage to loose BR/SMG,
  fixed-turret items and per-instance baked/dynamic lighting. Do not transplant
  the historical ISO dress-cap camera or datum assertions as MCC evidence.
- [ ] Recheck actual MCC first-person/dual-player sequences, sustained firing,
  reloads and authored sound events. The earlier ISO SMG firing case's zero
  authored sound submissions remains unresolved; nonzero callback PCM alone
  does not establish correct firing-sound identity, timing or ownership.
- [ ] Qualify the Xbox `highplains2.map` variant against MCC `highplains.map`.
  Exact weapon/cache identity pairs match1,077 of1,110 reference contexts; all33
  mismatches are the Xbox Highplains2 context. Their scenario paths also differ,
  so a filename alias alone does not establish equivalence. Preserve both source
  identities and verify the required import/conversion and map-specific operands.
  [Context audit](../analysis/weapons-tu15-goal-20261013/mcc-weapon-context-coverage-v1.json),
  [original/MCC identity](../analysis/weapons-tu15-goal-20261013/mcc-highplains-counterpart-identity-v1.json).
- [ ] Continue the feature, visual and owner gates below across all63 reference
  weapon paths, fixing each reproduced discrepancy against its original owner.

## Completed and bounded milestones

- [x] Finish the MCC Release builds and inspect each actual outcome. V9 builds,
  passes nine checks and clears both original render-input failures. The actual
  application next fails on the MCC ammunition-tens producer before submitting
  a frame. Retain genuine failures and exact source/input hashes.
  [Latest results](../analysis/weapons-tu15-goal-20261013/weapon-mcc-native-release-validation-v9/release/tests.log).
- [x] Verify native MCC map/HUD startup and actual shared-copy behavior without
  an Xbox executable world constructor. The real authored HUD address and704
  overlapping-copy cases pass; no source return or tag address is supplied.
  [Native HUD receipt](../analysis/weapons-tu15-goal-20261013/weapon-mcc-native-release-validation-v1/release/evidence/weapon-campaign-native-hud.json).
- [x] Restore the exact original `47D6C0 = FFFFFFFF` operand and writable
  section ownership. The actual MCC rerun clears original `1E7F18` and reaches
  the later descriptor read. This does not complete actor or weapon acceptance.
- [x] Export and verify115 original AI descriptor interfaces:4,632 immutable
  bytes,920 writable cache bytes with original-2/0 inputs, and71 separate real
  mask constructors (213 source instructions). All constructor ABI/stores and
  cache/literal access rules pass in an isolated native composition. This is
  not yet MCC campaign acceptance. Probe binaries/scratch are removed.
  [Native composition](../analysis/weapons-tu15-goal-20261013/ai-descriptor-native-composition-v2.json),
  [source write ownership](../analysis/weapons-tu15-goal-20261013/ai-descriptor-source-ownership-v1.json).
- [x] Complete the authenticated AI behavior descriptor backing reached by MCC
  actor placement: root `2097E0`, instruction `1B0D0F`, descriptor `48AD60`,
  missing callback slot `48AD78`. Preserve real callback fields, source branches
  and constructor-owned mutable masks; no supplied callback/return value.
  [Original descriptor](../analysis/weapons-tu15-goal-20261013/mcc-campaign-48ad78-original-frontier-v1.json),
  [schema inventory](../analysis/weapons-tu15-goal-20261013/mcc-ai-descriptor-schema-inventory-v1.json).
  The actual MCC v4 actor/source case now passes this former failure.
- [x] Verify MCC campaign AI-held weapon source submission against independent
  section/WORLD-pose/shader/lighting inputs. The real Johnson actor owns its
  turret item;2,358 vertices, three source inputs and seven model rejection controls
  pass. This uses MCC and native startup, with no rendered-frame claim.
  [Source case](../analysis/weapons-tu15-goal-20261013/weapon-mcc-native-release-validation-v4/release/evidence/weapon-campaign-source-submission.json).
- [x] Verify cold world-memory composition and shared campaign-table ownership.
  V5 fails with overlapping regions before startup. The current factory differs
  from the deleted v5 build input and declares shared table words after their
  earlier complete owners. Validate that actual factory and preserve the
  existing arithmetic-table owner; do not infer the exact frozen-v5 cause from
  changed source. The actual current factory now passes all 18 original table
  words and preserves that complete 24-byte owner across 2,761 cold regions.
  This is cold composition evidence; MCC application acceptance stays open.
  Probe scratch is removed. Keep the known-passing v4 actor/source evidence.
  [Cold factory](../analysis/weapons-tu15-goal-20261013/campaign-table-native-cold-composition-v2.json).
- [x] Identify the actual model/resource owner behind the `23644` failure.
  The same MCC canonical allocation plan maps source section `91F94CDC` to
  section 0 of `ss_exterior_gun_hinge`. Its node map is `[3, 9]`; all five
  sections retain the same vertex counts and authored node maps in the paired
  Xbox cache. The Xbox header is 112 bytes and MCC is 120 bytes, with the
  type100/type104 node-map destination and count/pointer fields displaced.
  This establishes ownership/layout; native publication also passes in v8.
  [Owner](../analysis/weapons-tu15-goal-20261013/mcc-model-resource-owner-validation-v1/cmake/evidence/weapon-mcc-model-resource-owner.json),
  [paired headers](../analysis/weapons-tu15-goal-20261013/mcc-gun-hinge-paired-model-header-v1.json).
- [x] Validate the typed MCC node-map conversion through canonical metadata,
  actual resource I/O and the unchanged original publisher/render consumers.
  Preserve the raw MCC input, authored node bytes and all other resource bytes.
  Check fragmented reads, aliases, malformed owners and all five paired original
  headers; then rerun actual MCC rendering. Do not mount address2, skip the model
  or supply a source return value.
  The focused contract/actual-I/O checks now pass: 906 MCC node-map resources,
  five complete paired headers and 23,676,834 checks, including unchanged other
  resource bytes. The full MCC run now also requires the complete metadata
  delivery regression. V8 traverses the real publisher/`235E0` consumer past
  the former `23644` node-map failure. Full application frames remain open.
  [Focused result](../analysis/weapons-tu15-goal-20261013/mcc-model-node-map-validation-v1/cmake/evidence/mcc-model-node-map.json),
  [actual MCC delivery](../analysis/weapons-tu15-goal-20261013/weapon-mcc-native-release-validation-v8/release/evidence/weapon-mcc-runtime-delivery.json),
  [actual application](../analysis/weapons-tu15-goal-20261013/weapon-mcc-native-release-validation-v8/release/evidence/weapon-campaign-application.json).
- [x] Audit the current model-render module's fixed read-only program inputs
  against the authenticated original image and actual cold factory. All 95
  operand extents match. The live PRT input and ten further literal ranges
  preserve original bits and arithmetic; source results are not supplied.
  [Original inventory](../analysis/weapons-tu15-goal-20261013/model-render-original-readonly-operand-inventory-v2.json),
  [actual cold census](../analysis/weapons-tu15-goal-20261013/model-render-native-cold-operand-census-v3.json).
- [x] Verify the two axis-permutation indexed reads. The complete original
  `123210` routine returns only axes 0–2 in 1,000 executions, including ties,
  signed zeros, infinities and NaNs. `1211C0` combines that with a 0/1 sign
  choice into six table entries; the native factory matches all36 original
  read-only bytes. This does not establish the other indexed tables' bounds.
  [Original execution and native owner](../analysis/weapons-tu15-goal-20261013/model-render-axis-indexed-table-proof-v3.json).
- [x] Restore the 24 original bitmap bit-depth fields selected by the original
  decoder's `0..23` format switch. The real startup mounter fails before the
  fix at `45F290`, then passes all24 exact fields and six read-only, neighbor
  and atomic-overlap controls. Keep original decoder arithmetic and results.
  The next full MCC run also requires the current cold-world ownership test.
  [Original table and switch](../analysis/weapons-tu15-goal-20261013/model-render-bitmap-format-table-original-v1.json),
  [red](../analysis/weapons-tu15-goal-20261013/model-render-bitmap-table-native-red-v2.json),
  [green](../analysis/weapons-tu15-goal-20261013/model-render-bitmap-table-native-green-v1.json).
- [x] Review the three additional MCC paths (`smg_silenced`, `gpmg`, Falcon
  `rpg`) for Xbox-scope applicability and actual ownership. None appears in any
  of the 41 owned original map/shared/menu directories. MCC owns their metadata
  in both shared banks; the 25 multiplayer occurrences per path are nonresident
  aliases. Preserve these MCC additions outside the original 63-weapon catalog;
  their MCC gameplay remains unverified and no Xbox behavior is invented.
  [Applicability and ownership](../analysis/weapons-tu15-goal-20261013/mcc-additional-weapons-original-directory-audit-v1.json).

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
