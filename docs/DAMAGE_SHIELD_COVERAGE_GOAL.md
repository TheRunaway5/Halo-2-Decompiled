# Damage, vitality and shield presentation coverage goal

Research date: 2026-10-11. Status: active coverage goal; implementation is incomplete.

Update 2026-10-12: the actual native multiplayer renderer now submits the
Spartan/Elite damaged-shield body and FP arms layers. The original model-owner
filter excludes held weapons; the native renderer now preserves that boundary. Real-controls
hits, expired feedback, owner isolation and Vulkan validation are covered in
[the native presentation delivery](SHIELD_LIVE_NATIVE_PRESENTATION.md). This
closes that declared native presentation seam; original world-model/queue
ownership, overshield binding and full damage coverage remain incomplete.

Implement every original damage producer and its admission, amount, shield/body,
aftermath and presentation consumers in the native port. Include ordinary shield
hit shimmer, depletion and recharge effects, first-person and world shaders, HUD
feedback, and overshields. Each supported branch must trace to original
executable instructions and owned cache assets. Unknown behavior remains an open
gate rather than a substitute inferred from a tag name or another game.

This document inventories evidence and orders the work. It does not claim that
all damage sources have already been identified or that a scalar fixture proves
live gameplay parity. The producer census below must be expanded whenever the
executable or authored reference graph exposes another source.

Completed systems must be connected as they land. The native multiplayer and
object/world source runtime now route direct damage and vitality through one
bank/CPU composition, including the ordinary object-update vitality call.
The current shared path selects 90 complete paired damage, vitality, delivery
and feedback bodies, including shield/body processing, material lookup, section
handling, effect requests and unit response selection. The live body-material
loop also consumes each player's damage-feedback state through that biped's
authored object functions. All 39 selected native checks pass. Full delivery,
natural source hits, shared host/source storage and shield draws remain open.
[Current implementation and exact boundaries](SOURCE_DAMAGE_DELIVERY_RUNTIME.md).

The source-enabled live body path now also consumes its actual retained owner
sphere. Its source world constructs and activates the original render-cache
pool over the same retained allocator/bank. Both changes have actual startup,
biped and Match callers, with an independent full-store original comparison
for cache startup. Complete visible shield admission/submission and shared
damage storage remain open. [Connected paths and validation](SHIELD_LIVE_OWNER_SPHERE.md).

The source-enabled world now also executes the original shield/death region
flag delivery and explicit damage-state selectors on its actual birthed and
Match bipeds. Independent original/native comparisons pass510 region cases and
3,530 deeper vitality cases, including the continuous2,801-phase charge/drain
history. Stock models retain their original fallback for absent authored states.
Fatal cleanup has progressed through its real source callers; its remaining
owner/physics boundaries and rendered shield submission are still open.
[Region integration and exact evidence](SOURCE_DAMAGE_REGION_LIVE_DELIVERY.md),
[fatal cleanup source handoff](DAMAGE_FATAL_CLEANUP_LIVE_DELIVERY_HANDOFF.md).

Confirmed-fatal live cleanup now also connects the original component
initializer, descriptor scale/pose updates, item/entity-state owners and action
interrupts. Its actual prepared Match reaches resource admission at
`170194→1703D0`; queued-resource/physical-cache startup remains required there
and for shield model delivery. All51 selected checks pass, including eight
fresh original/native owner captures. Death/rebirth and shield pixels remain
unfinished. [Connected cleanup and exact limits](SOURCE_FATAL_CLEANUP_LIVE_DELIVERY.md).

## Authority and current boundaries

The local [retail executable](../assets/disc/default.xbe), owned title-update
images, and original map/tag bytes are behavioral and asset authority. The
readable [damage reconstruction](../tools/reference/halo2-decompiled/src/damage.cpp)
and [function manifest](../tools/reference/halo2-decompiled/config/functions.csv)
are navigation aids: `D5BC0` and `D8B50` are marked matched; `D5DE0`, `D7B80`,
`D9110` and `D9640` remain marked todo in that manifest. A reconstructed routine
name or a successful isolated execution does not establish every caller or live
owner. [Retail disassembly generator](../tools/reference/halo2-decompiled/tools/disasm.py).

Tag schemas identify field offsets and reference categories. Their labels alone
do not prove the executable branch that consumes them. In particular, `tagRef`
starts with a group word: the datum is four bytes after the schema offset.
For example damage-info shield-damaged effect schema `+A4` contains its datum at
`+A8`. [Model schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/hlmt.xml).

The native implementation already has bounded ordinary armor, shield/body and
recharge arithmetic. `apply_unit_damage` explicitly lacks authoritative model-hit
selection and delegates admission, amount, reaction and ownership to callers.
`Simulation::damage` supplies current native admission and the shared adapter;
`PropSandbox::damage` also uses that adapter.
[Adapter](../include/halo2/unit_damage.hpp),
[implementation](../src/unit_damage.cpp), [live simulation](../src/simulation.cpp),
[prop owner](../src/prop_sandbox.cpp).

Existing [physics differential evidence](PHYSICS_PARITY.md),
[body-impact evidence](BODY_DAMAGE_RESEARCH.md),
[SMG/history evidence](MP_SMG_DAMAGE_HISTORY_RESEARCH.md),
[natural native PvP evidence](MP_NATURAL_PVP_DAMAGE_RESEARCH.md),
[grenade integration evidence](MP_NATIVE_GRENADE_SANDBOX_RESEARCH.md), and
[live prop evidence](MP_LIVE_PROP_SANDBOX_RESEARCH.md) establish distinct scoped
boundaries. Retain their supplied inputs and missing owners when reusing them.
Older broad progress summaries must not override the current source: grenade,
prop and world-impact integrations now exist, while full source contact and
effect admission remain separate gates.

## Damage producer census

| Category | Original owner or authored evidence | Current native boundary and required work |
| --- | --- | --- |
| Direct weapon/projectile impact | `FAA60` constructs the impact damage event and invokes `D7B80` with actual object, node and region entry; `FD7D0` handles impact/airburst/attached/super-detonation paths. `proj` has distinct impact, detonation, attached, super and boarding damage references. [Projectile source](../tools/reference/halo2-decompiled/src/projectiles.cpp), [schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/proj.xml). | `Simulation::trace` finds native closest terrain/prop/capsule contacts; ordinary SMG damage is integrated. Import and census every weapon/barrel/projectile graph, then recover its actual firing, charged/heat and collision-state producers. Cover direct and ballistic behavior, range/air/water scaling, penetration, ricochet, guidance, attachment, repeated impact and detonation without treating a generic shot as each weapon. [Content importer](../src/content.cpp), [simulation](../src/simulation.cpp), [material-response leaf](../src/projectile_material_response.cpp). |
| Melee, combos and lunge; carried-object melee | `weap` has player melee, three combo and lunge damage/response references plus reporting type. Original `EB080/EB700` own the action and impact timing; `D0830` is the impact/geometry owner. [Weapon schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/weap.xml), [source action/channel evidence](MP_MELEE_ACTION_SOURCE_RESEARCH.md). | A source-created first action/channel update is documented; that experiment never reaches contact or damage. Recover actual current-pose target admission and impact through `D0830`, including source-selected damage definition, back-facing/unsuspecting and sword branches where admitted. Verify flag, bomb and ball authored definitions rather than assigning generic melee damage. |
| Grenades and radial explosions | `D6C80` selects radius candidates and `D74E0` applies per-object radius handling; projectile `FD7D0` has distinct explosion branches. [Damage source](../tools/reference/halo2-decompiled/src/damage.cpp), [projectile source](../tools/reference/halo2-decompiled/src/projectiles.cpp). | Native frag/plasma throw, terrain contact, fuse and blast exist through `NativeGrenadeSandbox` and `Simulation::area_damage`. Native capsule/triangle nearest-point and obstruction services are explicit host geometry. Unit flight collision/sticking, original candidate traversal/filtering, stochastic responses, directional cones and acceleration/camouflage side effects remain independent. [Native grenade](../src/native_grenade.cpp), [area delivery](../src/simulation.cpp), [definition import](../src/prop_sandbox_definition.cpp). |
| Destructible objects and chained damage | `DAAA0/DA110` own section thresholds, delayed responses, model changes and side effects; `D9E70` emits a destroyed response's marker-based area damage. [Damage source](../tools/reference/halo2-decompiled/src/damage.cpp), [damage-section mageable object types, dynamic current poses, attached children, collision permutations, constraints, rider ejection and chain ordering. Prop importer, prop owner, live evidence. |
| Falling, excessive speed and distance death | Retail `E3700` uses actual velocity/support/vehicle protection and global maximum falling velocity to construct damage; `E40D0` calls the airborne owner. `matg` has separate falling, distance and velocity inputs. [Biped source](../tools/reference/halo2-decompiled/src/bipeds.cpp), [globals schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/matg.xml). | Complete paired falling, airborne, moving-support, position and request initialization/attribution bodies now compile, with TU1.1 biped phase adapters. Diagnostic original/native comparisons reach actual `D9180` delivery frames. Live physics admission, support lifecycle, player callbacks and complete damage delivery remain open. Kill-volume admission is separate. [Implementation and exact proof boundary](BIPED_FALLING_DAMAGE_RUNTIME.md). |
| Physics collision, crushing and vehicle contact | `1DA550` constructs contact damage from actual Havok contact/material state; `1DA9E0` maps speeds through authored limits; `1DA6D0` computes transferred collision force. Per-object `+6C..` collision fields and `matg` collision globals are authored inputs. [Collision source](../tools/reference/halo2-decompiled/src/unknown_1da550.cpp), [biped schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/bipd.xml), [vehicle schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/vehi.xml), [globals schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/matg.xml). | Native blocking and terrain/prop triangles do not produce original Havok impulse/contact ownership. Trace original contact admission, cooldown/history, owner and material into damage before enabling run-over, crush, moving machinery or transferred rigid-body damage. Census distinct producers rather than assuming these names all use one contact branch. |
| Vehicle destruction, riders and boarding | `DC230` transfers vehicle damage to a rider through seat/damage-info scalars; `D7B80` walks parent/child paths; `DA110` can eject or kill riders. `proj` has separate boarding damage. [Damage source](../tools/reference/halo2-decompiled/src/damage.cpp), [vehicle source](../tools/reference/halo2-decompiled/src/vehicles.cpp), [projectile schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/proj.xml). | Requires genuine vehicle, seat and rider owner state. Preserve direct seat scaling versus transferred damage, parent protection, occupant attribution and child cleanup. A childless unit/vehicle fixture does not close this category. |
| Scenario kill volumes and scripted damage | Original script roots `2A06A0` damage at a cutscene flag, `2A07C0` damage an object, `2A0730` walk an object list, and `2A0840` damage local player units. Scenario has explicit kill-index references to volume rows. [Script source](../tools/reference/halo2-decompiled/src/unknown_29f5b0.cpp), [scenario schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/scnr.xml). | Native scripts expose bounded immunity and health/shield queries, but not these complete damage roots. Current Match kill volumes test native body position over all rows and directly set death/body. Original TU1.1 `DEFC0→1EFEF0→11DB80` rotates one enabled row using actual tick/datum and tests the produced bounds center before authored `globals\\distance` damage. Replace this bypass only with the recovered request/admission owner. [Native script owner](../src/simulation.cpp), [Match loop](../src/multiplayer.cpp), [original kill-volume evidence](MP_SPARTAN_KILL_TRIGGER_RESEARCH.md). |
| Damage emitted by effects, items and equipment | Original effect part dispatch accepts `jpt!`, initializes an event, copies effect owner/location/direction/scale and calls `D6C80`. `eqip` has detonation damage and delay; `weap` has ready, overheated and detonation damage effects. [Effect source](../tools/reference/halo2-decompiled/src/unknown_175bd0.cpp), [effect schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/effe.xml), [equipment schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/eqip.xml), [weapon schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/weap.xml). | Follow actual owned reference graphs and original part scheduling; a visual effect callback must not accidentally omit gameplay damage or recursively execute it in each rendered view. Recover pickup/detonation/heat/event admission before claiming these sources. |
| Other authored damage families and rules | `jpt!` categories include flame, mounted weapon, vehicle, plasma, needle, shotgun and sniper, with EMP/unsuspecting side effects and owner/friend/player/shield flags. `matg` includes flaming-death and vehicle damage references. These are schema facts, not proof that every category has a unique producer. [Damage schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/jpt!.xml), [globals schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/matg.xml). | Complete the cache/executable caller census for campaign actors, mounted weapons, objective bomb explosions, flame/ongoing damage, needle super-detonation and special creature/infection branches. Record absent resident or externally shared tags explicitly; do not manufacture a missing family from the category enumeration. |

## Shared shield and health pipeline

Every producer must reach the original sequence applicable to its event:
definition/owner construction, admission and target selection, amount/scale,
shield routing, body/section routing, report construction, aftermath, death and
presentation. Original `D7B80` is the central dispatcher; separate radius and
section-marker paths feed it or its downstream services.
[Dispatcher and helpers](../tools/reference/halo2-decompiled/src/damage.cpp).

Required shared coverage:

- Initialization and maxima: `D5C90`, `D5D20/D5D80`, actor character vitality,
  difficulty/team scaling and player/variant traits. Preserve normalized
  fractions separately from absolute maxima. Native `VitalityDefinition` and
  importer already retain authored ordinary values; edited traits and complete
  actor/team owners require separate proof.
  [Original](../tools/reference/halo2-decompiled/src/damage.cpp),
  [native fields](../include/halo2/content.hpp), [import](../src/content.cpp).
- Amount and admission: `D72E0/D73C0`, `DB160` in the supported title-update
  image and corresponding retail amount logic; random upper-bound draw/stream,
  event scales, handicap/trait scales, owner/friend/player restrictions,
  protection, explosive-only rules and special kill admission. Current native
  `damage_amount` intentionally begins after random upper-bound selection.
  [Amount contract](../include/halo2/damage_amount.hpp),
  [live ordinary admission](../src/simulation.cpp),
  [original dispatcher](../tools/reference/halo2-decompiled/src/damage.cpp).
- Shield/body: `D9110`, `D9020/D8CB0/DB090` and `D5BC0`. Cover direct/radius,
  armor key order/missing/repeated keys, zero/negative products, shield-only and
  shield-skip flags, overflow, EMP, max-zero, protection minimum, lethal negative
  body fraction, requested normalized damage versus actual loss and strict
  stun thresholds. Explicit collision hit entries must own headshot/section
  selection. [Original](../tools/reference/halo2-decompiled/src/damage.cpp),
  [native kernels](../src/kernels.cpp), [body evidence](BODY_DAMAGE_RESEARCH.md).
- Sections and depletion: `DAAA0/DA110/DAE60`, `D6800/D6A70`, deferred responses,
  pending ticks, authored permutation/constraint/effect and child/rider side
  effects. Preserve callback/store order; dying must not bypass source ownership
  or silently discard a shield-depletion request.
  [Original](../tools/reference/halo2-decompiled/src/damage.cpp).
- Recharge and powerups: `D5DE0` has distinct body/shield stun semantics,
  actor/team scaling, mode gating, ordinary recharge, overshield charge and
  multiplayer drain/HUD-history adjustment. `D6790` restores health and `D6AF0`
  initiates overshield charging; `eqip` lists health and overshield powerups.
  Native `recharge` explicitly covers only the ordinary scalar branch.
  [Original](../tools/reference/halo2-decompiled/src/damage.cpp),
  [equipment schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/eqip.xml),
  [native contract](../include/halo2/kernels.hpp).
- Aftermath and authority: result `+44/+48` are normalized requested body/shield
  amounts, consumed by `D9640` and unit feedback. Preserve source damage owner,
  recent damage, impulse/stun, camouflage visibility, damage history, death,
  equipment drop, corpse lifetime and score callbacks across local, split-screen
  and native LAN. Existing damage/history/attribution tests cover bounded
  downstream branches; they do not recover Havok/posed hit finding.
  [Original report/aftermath](../tools/reference/halo2-decompiled/src/damage.cpp),
  [history](../src/damage_history.cpp), [Match hook](../src/multiplayer.cpp),
  [attribution evidence](MP_LETHAL_ATTRIBUTION_RESEARCH.md).

## Shield visuals are several distinct owners

1. **Ordinary impact shimmer.** Retail `D9640` consumes report shield amount
   `+48`; unless object damage flag bit11 selects its special behavior, an amount
   above the source epsilon accumulates into object `+FC` and `+F4`, capped at1,
   and resets recent-shield timer `+108`. Body damage similarly uses `+100/+F8`
   and timer `+109`. `D5DE0` updates these through `D8B50`; its special bit11
   branch is separate. This is not the shield-damaged threshold flag.
   [Producer and update](../tools/reference/halo2-decompiled/src/damage.cpp).
   **Instruction-level correction:** the reconstructed `D5DE0` tail incorrectly
   labels the fade-settings pointer as damage info. Original `D5DE0` saves info
   at stack `+14` before `PUSH EDI`, making it `+18` afterward; `D5E62` initializes
   the distinct current `+14` to null, `D5FDF` stores actor-vitality bounds there,
   and `D6598/D6614` load that slot into EDX for `D8B50`. Thus an ordinary player
   without actor bounds uses `D8B50` defaults (delay A0, decay A2, delay B2,
   decay B2), not `hlmt` body stun/recharge fields. Preserve the literal float,
   tick conversion and store order when recovering this chain.
   [Original caller listing](../analysis/systems/d5de0-research-disassembly.txt),
   [original fade listing](../analysis/damage-shields-goal-20261011/d8b50-retail.asm).
2. **Object-function shader inputs.** Original `BCC20` reads `15000557` from
   `+F4`, `13000556` from `+F8`, `0F000559` from current shield `+F0`, and
   `1800055A` from `+F0-1`; it clamps values to `[0,1]` and produces active state.
   Replay the complete producer/update/input chain with genuine per-unit state.
   [Resolver](../tools/reference/halo2-decompiled/src/unknown_0bb760.cpp),
   [retained original listing](../analysis/frontend/customization/000bcc20.asm).
3. **First-person and world shader admission.** Damage-info has separate damaged
   first-person and world shader references at datum offsets `+80/+88`. Recover
   the original overlay selector, pass admission, pose/material attachment,
   shader-function consumption and clocks. The current gameplay material loop
   now evaluates per-player health, shield and recent-damage inputs through
   the actual biped function bank. It consumes the host snapshot; generated
   source damage writes a separate retained bank. Original `4BCC0` and its
   collector consumers are implemented, but lack live geometry admission,
   compact node context and a complete shared transparent queue. Their draw
   and pixel integration remains open.
   [Authored model fields](../tools/reference/assembly/src/Assembly/Plugins/Halo2/hlmt.xml),
   [gameplay animation loop](../src/multiplayer_runtime.cpp),
   [menu-only input owner](../src/frontend_model.cpp),
   [original overlay ownership](SHIELD_OVERLAY_SOURCE_RESEARCH.md),
   [current live seam and next owners](SHIELD_LIVE_BODY_WIRING_HANDOFF.md).
4. **Threshold, depletion and recharge effects.** `D9110` emits damaged-effect
   requests only when the authored strict threshold crosses; shield depletion
   calls `D6A70`, which requests its effect before clearing `+F4`, setting the
   depleted flag and calling the object status callback. `D5DE0` independently
   requests the recharge effect when entering recharge. Retain original call
   attempts, including a `NONE` effect datum, separately from successful runtime
   allocation and visible output. [Original](../tools/reference/halo2-decompiled/src/damage.cpp).
5. **Overshield presentation.** Damage-info's separate first-person/world
   overshield shader references and damage-info count are now imported. The
   recovered selector independently admits current shield `>1` and carries
   `1-current_shield`; damage and overshield overlays can coexist. Live
   charge/drain ownership, material binding and scheduling remain open.
   [Model fields](../tools/reference/assembly/src/Assembly/Plugins/Halo2/hlmt.xml),
   [selector](../src/shield_overlay.cpp),
   [original vitality owner](../tools/reference/halo2-decompiled/src/damage.cpp).
6. **HUD and player feedback.** Shield meter/history/pulse is a separate
   `13A838/22B3CB` consumer. `jpt!` player responses also contain shielded/
   unshielded selection, screen flash, vibration, sound and camera impulse.
   Existing native HUD damage/recharge/overshield tests are useful for that
   consumer, but do not establish a body shimmer or player-response dispatcher.
   [HUD source evidence](MULTIPLAYER_HUD_RESEARCH.md),
   [damage-response schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/jpt!.xml),
   [HUD tests](../tests/hud_tests.cpp), [framebuffer tests](../tests/hud_render_tests.cpp).

**Completed foundation:** the biped and prop importers retain the five
authored damaged-shader and damaged/depleted/recharging effect references,
plus both overshield shaders and the original damage-info count.
`apply_unit_damage` returns source-ordered damaged/depleted attempts, preserving
`NONE` datums and both shield and body-death paths. A new original-instruction
fixture exports 25 cases and 31 ordered attempts from unchanged TU1.1
`DA710/DA2B0/D8070` using relocated original Lockout metadata. Threshold,
vitality and flag mutations are explicit synthetic inputs; effect allocation,
reaction/animation and spatial services are supplied. The native comparison
checks state/flags/stuns, exact request order/datums and the five actual Lockout
references. [Exporter](../tools/oracle_unit_damage_effects.py),
[comparison](../tests/unit_damage_effects_tests.cpp),
[report](../analysis/damage-shields-goal-20261011/unit-damage-effects-original.json),
[fixtures](../analysis/damage-shields-goal-20261011/unit-damage-effects.tsv).

The new `original_unit_damage_effects` and existing `retail_smg_damage` CTests
pass in `build-damage-shields-20261011`. This does not execute allocation,
attachment, recharge effect delivery or ordinary hit shimmer. Their live owners
and original visual comparison remain open. Reproduce the bounded foundation:

```sh
.tools/python/bin/python tools/oracle_unit_damage_effects.py \
  --output analysis/damage-shields-goal-20261011
cmake --build build-damage-shields-20261011 \
  --target unit_damage_effects_tests smg_damage_tests
ctest --test-dir build-damage-shields-20261011 \
  -R '^(original_unit_damage_effects|retail_smg_damage)$' --output-on-failure
```

The [evidence directory](../analysis/damage-shields-goal-20261011) also retains
configuration/build logs and scalar differential reports. Their selected
branches and external services must remain explicit; aggregate case counts do
not close the producer census or rendering gate.

**Recent-damage state increment:** the live unit now retains separate visible
and accumulated recent body/shield amounts plus the original signed timers.
Normalized reports carry new body/shield depletion bits; shield depletion
clears visible shield damage before report consumption. Ordinary recharge and
recent-damage fading execute in the native ordinary update phase before new
projectile hits, including fading on dead units. Players use original defaults;
campaign character vitality bounds retain their authored fade overrides.
Native snapshots and protocol30 UDP carry both channels unchanged. This native
transport format is separate from Xbox networking.
[State/API](../include/halo2/damage_feedback.hpp),
[kernel](../src/damage_feedback.cpp), [live phase](../src/simulation.cpp),
[snapshot](../src/multiplayer.cpp), [transport](../src/multiplayer_network.cpp).

The unchanged-retail oracle independently exports1600 report-prefix cases,
2400 complete fade cases,1600 recent-damage update-tail cases and1400 complete
vitality-input cases. These include signed timers, nearest-even delay conversion,
reset/depletion, special bit11, zero/epsilon, infinity/NaN and signed-zero
branches. Scalar output bits, timers and returned status matched in the native
comparison. Three actual original Lockout SMG report trajectories separately
exercise seven live ticks each in local and actual UDP production paths, using
explicit clear-world/contact and initial vitality fixtures.
[Exporter](../tools/oracle_damage_feedback.py),
[scalar comparison](../tests/damage_feedback_tests.cpp),
[live hit comparison](../tests/smg_damage_tests.cpp),
[retail report](../analysis/damage-shields-goal-20261011/damage-feedback-original.json),
[SMG report](../analysis/damage-shields-goal-20261011/smg-feedback-live-original.json).

Complete-body static comparison located TU1.1/u5 pairs `DA150/DB0A0` (fade),
`D73E0/D8330` (ordinary update), `DAC40/DBB90` (report consumer), and
`DAA90/DB9E0` (report producer), with matching instruction structure/internal
branches and no changed nonbranch immediates. Read-only scalar constants match;
mutable global bytes remain initial-image evidence, with no claim that their
live producers were replayed.
[Comparison tool](../tools/research_damage_feedback_updates.py),
[retained complete bodies and operand report](../analysis/damage-shields-goal-20261011/recent-damage-updates/report.json).

The authored shader-input seam now distinguishes recognized scalar/active
handler results from interpolation overrides. The original retail
`BAB40/108D90/BCC20/BA8C0/13BB90` chain is replayed using actual Lockout
Spartan/Elite records and identity curves; its fixture includes direct recent
shield input, both authored exports, missing exports, zero, fractional,
epsilon and out-of-range finite values. Renderer admission, selected plasma
passes, lighting-cache vitality externs, live first-person/world overlays and
original frame comparison remain open. The goal is active.
[Authored handler contract](../include/halo2/shader_animation.hpp),
[mapping implementation](../src/shader_animation.cpp),
[original mapping report](../analysis/damage-shields-goal-20261011/shield-function-inputs-original.json),
[remaining overlay gates](SHIELD_OVERLAY_SOURCE_RESEARCH.md).

Validation also passes the existing shader-animation regression suite and the
actual UDP full-roster test:16 original-start participants,4 local source
clocks,270 ticks, lost/reordered/duplicated/late snapshots,9 fragments bounded
to1400 bytes, and exact recent body/shield values/timers for every participant.
Initial feedback values in this transport test are explicit supplied state;
the SMG suite separately tests the real projectile/report producer.
[Full-roster test](../tests/multiplayer_fp_network_tests.cpp),
[state/alias/shader log](../analysis/damage-shields-goal-20261011/feedback-final-validation.log),
[UDP log](../analysis/damage-shields-goal-20261011/feedback-lan-validation.log).

The earlier full native build reached the final link and failed on then
in-progress loading declarations
`DesktopSession::begin_loading` and `DesktopSession::configure_loading`.
The later shield increment below builds the full executable successfully.
No updated launcher/runtime was installed. Compiler temporaries use this
isolated build's `tmp` directory because the shared `/tmp` tmpfs filled during
compilation. These are build-delivery limits, not original damage behavior.
[Build log](../analysis/damage-shields-goal-20261011/feedback-final-build.log).

Reproduce this increment:

```sh
.tools/python/bin/python tools/oracle_damage_feedback.py
.tools/python/bin/python tools/research_damage_feedback_updates.py \
  --output analysis/damage-shields-goal-20261011/recent-damage-updates
cmake --build build-damage-shields-20261011 \
  --target damage_feedback_tests smg_damage_tests unit_damage_effects_tests
ctest --test-dir build-damage-shields-20261011 \
  -R '^(original_damage_feedback|original_unit_damage_effects|retail_smg_damage)$' \
  --output-on-failure
```

## Shield selector, plasma pass and posed-input increment

`select_shield_overlays` now reproduces the original damaged/overshield
selector after explicitly supplied visibility, model-count and geometry-LOD
services. The unchanged original `4BCC0/B9CE0/3D3E0/4C450` fixture covers1,800
cases on real Lockout Spartan/Elite metadata:244 damage-only,65 overshield-only,
72 combined and1,419 rejected. It preserves first-person/world references,
independent overlay order and current-vitality scalars. The fixture explicitly
excludes previous-frame geometry history and stops before geometry finalization;
the native helper does not choose live visibility or geometry.
[Selector contract](../include/halo2/shield_overlay.hpp),
[original exporter](../tools/oracle_shield_overlay.py),
[comparison](../tests/shield_overlay_tests.cpp),
[original report](../analysis/damage-shields-goal-20261011/shield-overlay-original.json).

The actual plasma pass is now admitted as authored: layer15, implementation0,
original compiled vertex/pixel programs, RGB-only ONE+ONE blending, depth writes
off, two actual volume-noise textures and their original mip chains. Volume
binding admits literal PROJECT3D alongside the existing PROJECT2D operation;
unproved texture-coordinate rewriting remains rejected. All four actual
Spartan/Elite FP/world shaders retain separate animation banks. An independently
repeated unchanged-original `BAB40` input chain and `17550/17420/176A0/17960`
animation comparison covers240 cases/45,600 words bit for bit. Its prepared
static baseline is shared to isolate animation, so it does not independently
prove static material assembly or NV2A pixels.
[Pass research](SHIELD_PLASMA_PASS_RESEARCH.md),
[binder](../src/world_material.cpp),
[differential](../tools/differential_shield_material.py),
[material comparison](../tests/source_shield_material_tests.cpp).

`SourceShieldModelShaders` independently builds shield resident banks over the
copied original posed geometry before ordinary shader filtering. It preserves
the original part gate, chosen shader priority/command fields and the override
LOD's indexing of the original material's mask. An unchanged complete `41040`
prefix plus complete `1BF50` fixture covers1,533 cases, including93 actual
Lockout part contexts, and stops before scheduling. Four real body/arms models
pass48 view selections/288 part checks with329,940 posed vertices unchanged
apart from their selected material slots. Missing or out-of-range resident LOD
banks are reported as unresolved inputs rather than silently clamped.
[Resident bank](../include/halo2/source_shield_model_shaders.hpp),
[implementation](../src/source_shield_model_shaders.cpp),
[original exporter](../tools/oracle_shield_override_parts.py),
[comparison](../tests/source_shield_model_shaders_tests.cpp),
[original report](../analysis/damage-shields-goal-20261011/shield-override-parts-original.json).

Original `transparent_generic_offset` skin classes1..6 use the posed position
and **raw** posed normal for geometry, but the original decompressed local
position for noise DPH outputs `o9/o10.xyz`. Models now retain that local
position through posing. A payload-identity-scoped input policy carries it into
the production Vulkan vertex path without changing the authored programs or
normalizing the normal. Eighteen controlled cases match all1,152 CPU output
words against the six original compiled classes. Production GPU tests compare
3,456 words within2e-5 relative/absolute arithmetic tolerance, including18
missing-local-position and18 wrong-normal controls. Four actual body/arms
models separately retain their local positions through posing.
[Retained position](../include/halo2/cache.hpp),
[model decoding](../src/model.cpp),
[scoped adapter](../src/source_cpu_skin_inputs.cpp),
[production shader](../shaders/source_cpu_skin_outputs.glsl),
[CPU/GPU comparison](../tests/source_shield_vertex_tests.cpp).

The full renderer also renders all four real shield materials on their real
body/arm geometry with actual authored function inputs and textures. Controlled
bind channels plus a45-degree left-hand rotation and a bounds camera produce
deterministic640x480 captures; zero versus recent-damage input changes283,240
RGB bytes, and removing local-coordinate handling changes37,599 bytes. Vulkan
and synchronization validation are enabled with the locally installed layer.
These are isolated asset inspections, not natural gameplay or original-frame
comparisons. The existing reflection skin renderer regression also passes.
[Renderer inspection](../tests/source_shield_render_tests.cpp),
[captured artifacts](../build-damage-shields-20261011/shield-input-inspection/),
[validation log](../analysis/damage-shields-goal-20261011/shield-render-validation.log).

Live layer15 producer integration remains open. Original world shields enter
the shared ordinal transparent bank; deferred first-person collectors create a
separately drained suffix. The native ordinary geometry path groups by
material/section/part; a separate source queue and ordered callback consumer
now provide the explicit handoff described below. A selected FP
shader or a regular command from `402B0` does not prove execution: its caller
executes layers11/14/12/(13), while the main deferred route admits the shield
through view0/force0. A later111-case original-instruction trace resolves the
two-call suffix lifecycle: each call appends and rewrites the suffix start,
and the final drain executes only the second suffix before restoring its
starting count. Populated specialized FP regular commands still do not execute
layer15. Capacity/admission and supplied service boundaries are retained in
the trace report. Preserve that source scheduling boundary before wiring live
overlays. Neither the lighting-cache vitality byte nor an arbitrary post-model
draw is an established substitute.
[Exact scheduling and original traces](SHIELD_OVERLAY_SCHEDULING_RESEARCH.md).

`SourceTransparentQueue` now owns the persistent1,024-record bank. Ordinary
world reset preserves record storage; allocation clears priority and only the
plane-present low byte. Ordinary part and generic effect/location producers
write only their original fields and payload bytes. Every deferred FP producer
replaces the suffix start, drains retain count, and the enabled final FP drain
restores count to that suffix start. It uses the existing unchanged-original
ordinal sorter, including null callbacks and nonstable equal-key behavior.
[Queue contract](../include/halo2/source_transparent_queue.hpp),
[implementation](../src/source_transparent_queue.cpp),
[original instruction comparison](../tools/oracle_source_transparent_queue.py),
[native comparison](../tests/source_transparent_queue_tests.cpp).

`ModelView` accepts complete ordinary-world and deferred-FP drain snapshots
with a checked native sidecar at each absolute command ordinal. The renderer
consumes geometry callbacks and particle-location ranges in the original
ordinal order, preserves each geometry callback's contiguous stream, validates
source payload identities and requires shaderLOD0/layer15/implementation0.
Per-owner uniforms remain independent; cache flag800 selects the separate FP
projection. Supplying a bank suppresses that phase's old unordered layer15 and
particle submissions. Unsupported nonnull group callbacks remain explicit
errors. This consumer does not construct a live collector, select visibility,
or reconstruct source registration chronology.
[Completed service contract](../include/halo2/source_transparent_draw.hpp),
[renderer](../src/renderer.cpp),
[current live producer evidence and gaps](SHIELD_LIVE_PRODUCER_RESEARCH.md).

The persistent-bank comparison passes117 unchanged-original operation streams
with215,807 output words bit for bit, plus111 original FP caller traces. An
independent repetition produces identical fixture SHA-256 hashes. The full
renderer consumes all four authored shield materials through both ordered
world and FP banks with exactly the same RGB as their existing isolated
inspection, including occupied-prefix/null handling and a rejected missing
group service. The reflection renderer and same-material independent-owner
regression also pass; the latter covers one/four views, resize, storage growth,
retirement and recreation. The full native target builds and `--help` executes.
Vulkan and synchronization validation report no VUID/errors; the deprecated
enables-setting advisory is retained. Source, binary, fixture and image hashes
are retained in the validation manifest. After concurrent presentation-owner
changes, the selected targets and all four focused checks plus the independent
owner regression were rebuilt and rerun successfully against the retained
current source snapshot. These checks do not establish joined live collector admission,
first-person W-depth with atmosphere, mixed alpha callback pixels, particle
callback raster order, grouped callbacks or original framebuffer parity.
[Queue original report](../analysis/damage-shields-goal-20261011/transparent-queue-repeat.json),
[queue/consumer validation manifest](../analysis/damage-shields-goal-20261011/transparent-validation.json),
[retained verbose checks](../analysis/damage-shields-goal-20261011/transparent-current-render-verbose.log),
[independent-owner regression](../analysis/damage-shields-goal-20261011/transparent-current-owner-validation.log).

The native identity prerequisite now has one authoritative owner. `Simulation`
owns `NativeObjectLifetimes` before constructing the scene; scenario objects,
multiplayer units and inventory/world weapons, ordinary projectiles and held/
released grenades allocate from its recovered 2048-element, 12-byte header pool.
Full salted tokens use the original raw zero-based low16 slot. Scene placement
storage has a separate lookup, preventing its local vector index from aliasing
another category. Native reserve/publication/release records retain actual
call order, type and definition, including unsuccessful spawn rollback. These
records are integration bookkeeping, not an original event format, complete
constructor packet or proof of original successful allocation chronology.
[Owner contract](../include/halo2/native_object_lifetimes.hpp),
[implementation](../src/native_object_lifetimes.cpp),
[original shared-pool and lifetime evidence](SHIELD_OBJECT_REGISTRY_RESEARCH.md).

Campaign checkpoints copy allocator bytes, cursor/bitmap/count/salt, lifetime
sidecars and event order, then restore that owner before scene consumers.
Ordinary projectile expiry, contact, BSP retirement and script bulk deletion
release their full identities. Hidden retained scene objects and held-to-flight
grenades keep theirs. Prop-removal encoding, decoding and simulation validation
admit raw slot0 and reject index2048 and absent salt. Native LAN protocol33
excludes earlier peers whose independent scene handles are incompatible.
Multiplayer checkpoint restoration still requires its complete match owner;
campaign actors without native source datums are not reconstructed by this
increment. [Integration checks](../tests/native_object_lifetimes_tests.cpp),
[checkpoint and projectile cleanup checks](../tests/campaign_tests.cpp).

The next live gate remains the original registration and collector owner.
Native lifetime publication cannot supply the missing source body flags,
construction/activation location, live hierarchy service calls, spatial
membership or poses. The registration journal now implements the recovered
ordered attachment and detachment mutations described below; current inventory
selection alone still does not produce those source events.
Category append loops cannot substitute for original successful registration
chronology, full-row prefix swaps, collector clone/child order or particle-
location admission. The renderer's explicit drain input is available, but no
production collector is activated by the identity increment. All damage-source
and vitality/effect completion gates below remain required; the goal stays
active. [Current call sites and missing packets](SHIELD_OBJECT_REGISTRY_RESEARCH.md).

All 13 focused identity, allocator, multiplayer, checkpoint, scene-consumer and
grenade-delivery checks pass after the completed selected-target build. The
native app builds and its `--help` entry runs. The lifetime check uses actual
Lockout content for the 19-root census and grenade/world-pickup lifecycle; its
slot-zero prop-removal UDP case explicitly uses a controlled BLOC-only scenario
from an actual authored row. An initial 15-root assertion was corrected using
the raw authored census; that earlier failure remains retained. Source/binary/
asset hashes and passing logs are recorded in the
[lifetime validation manifest](../analysis/damage-shields-goal-20261011/lifetime-validation.json)
and [verbose checks](../analysis/damage-shields-goal-20261011/lifetime-validation-verbose.log).
Later concurrent frontend build additions and generated animation expansion
are retained separately; the claim applies to the tested source snapshot.
This increment performs no new original framebuffer or live shield collector
comparison. Installed launchers remain unchanged.

The registration journal now retains the actual parent-node byte alongside
full parent, child-head and sibling datums. Its explicit attachment event
implements retail `B950A..B9548` / TU1.1 `BA3FA..BA438`: prepend the child,
set header bit7, and reset its eight-byte location to `NONE/-1/active BSP`.
The caller must first supply real admission, deactivation, unregister and
backend removal. Its detach event implements complete TU1.1 `BA780→BF540`,
splicing by full datum while preserving all remaining sibling order, then
clearing parent, parent-node and header bit7. The original absent-target
branch leaves the child's sibling unchanged. Neither event supplies the
subsequent parent-node transforms, velocity, bounds, activation or physics
callbacks. Native stale-owner, cycle, chronology and free guards are explicit
input checks, not claimed original exception behavior.
[Journal contract](../include/halo2/source_object_spatial_pool.hpp),
[implementation](../src/source_object_spatial_pool.cpp),
[hierarchy source and integration boundaries](SHIELD_OBJECT_REGISTRY_RESEARCH.md#ordered-hierarchy-mutations).

All **260** unchanged-original streams, **8,196** operations and **788,535**
compared words match the production-linked native journal. Independently
repeating the original capture produces the same fixture SHA-256. Controls
cover persistent nested hierarchies, first/middle/tail splices in a 2,047-child
list, explicit unregister before attachment, actual node-byte truncation,
same-tick reparenting, free prerequisites and copied checkpoint restoration.
These are declared controlled inputs rather than original live construction
histories. All eight focused hierarchy/spatial/admission/contact checks pass;
the native app builds and its `--help` entry runs. The scoped build and passing
checks are recorded in the
[hierarchy validation manifest](../analysis/damage-shields-goal-20261011/hierarchy-validation.json),
[original comparison](../analysis/damage-shields-goal-20261011/hierarchy-repeat.json)
and [checks](../tests/source_object_hierarchy_tests.cpp). This closes the
topology mutation seam; it does not activate a production shield collector or
close the remaining damage, vitality and original visual gates.

The [packet-producer audit](SHIELD_REGISTRATION_PACKET_PRODUCERS.md) identifies
the next required owner: persistent source construction/body state keyed by
the shared full datum and checkpointed with the journal. It must retain actual
requests, header/body mutations, admitted poses and service outcomes before
extracting a registration packet. The fourteen authored Lockout power modules
provide the smallest no-audio cold branch. Lift/sound-scenery attachments,
biped `+348`, projectile generic flags, the three distinct location handoffs
and post-registration activation/physics remain separate evidenced services.
The four new original sound-scenery prefixes stop at real looping-sound
allocation; their captured rows are research expectations, never runtime
construction inputs. [Retained field evidence](../analysis/damage-shields-goal-20261011/overlay-pass/registration/packet-producers/manifest.json).

Fifteen distinct focused checks pass across selector, part bank, material
animation, feedback/damage, CPU/GPU input and full renderer regression groups.
All three original fixture generators were independently repeated with
identical fixture SHA-256 hashes. The full `halo2_native` target builds and its
`--help` entry executes successfully in the isolated build.
The installed launchers have not been changed. This increment closes bounded
selector, material-animation and posed-input gates; all damage producers,
special vitality owners, effect allocation and original visual comparisons in
the work order remain required. The goal is active.
[Native build log](../analysis/damage-shields-goal-20261011/shield-native-build.log).
[Complete scoped validation manifest](../analysis/damage-shields-goal-20261011/shield-validation.json).

Reproduce the shield checks with the original assets present:

```sh
.tools/python/bin/python tools/oracle_shield_overlay.py
.tools/python/bin/python tools/differential_shield_material.py \
  --binary build-damage-shields-20261011/source_shield_material_tests
.tools/python/bin/python tools/oracle_shield_override_parts.py
TMPDIR="$PWD/build-damage-shields-20261011/tmp" \
  cmake --build build-damage-shields-20261011 --target \
  source_shield_material_tests shield_overlay_tests source_shield_vertex_tests \
  source_shield_render_tests source_shield_model_shaders_tests
VK_LAYER_PATH="$PWD/.tools/validation/usr/share/vulkan/explicit_layer.d" \
LD_LIBRARY_PATH="$PWD/.tools/validation/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
  ctest --test-dir build-damage-shields-20261011 \
  -R '^(original_shield_overlay|original_shield_material|original_shield_override_parts|source_shield_vertex_cpu|source_shield_vertex_gpu|source_shield_renderer_gpu)$' \
  --output-on-failure
```

## Persistent constructor storage increment

The simulation now retains an actual finite body arena beside its one shared
header pool, including complete checkpoint copies. The existing source
allocation and append kernels operate on this storage, preserve physical
padding across relocation, and use genuine capacity failures. Raw storage
reservations leave header type zero until its real producer writes it;
unresolved native sidecars cannot publish a completed object. All thirteen
authenticated TU1.1/u5 base extents now import through a separate bounded
metadata provider. [Implementation and validation scope](SOURCE_OBJECT_BODY_STORAGE.md).

The unchanged TU1.1 allocation, append, release, pool and finite heap owners
produced 83 sequences / 1,996 full storage snapshots. Independent original
captures repeat byte-identically, including all selected extents, failed base
and component allocation, relocation, shared-identity reuse and bounded
persistent streams. This increment supplies mutable constructor storage; it
does not supply completed generic/type callbacks, flags/pose/activation
providers, registration packets or live shield collector admission. Those
remain required before this storage can carry live shield presentation.
[Original storage research](SHIELD_OBJECT_BODY_STORAGE_RESEARCH.md).

Nine focused checks pass with none skipped, then pass again after the full
native executable rebuild; its `--help` entry also succeeds. The installed
launchers have not been changed. The [validation manifest](../analysis/damage-shields-goal-20261011/body-storage-validation.json)
pins the exact bounded comparisons, source snapshots, binaries and logs.
The [next authored constructor handoff](SHIELD_COLD_POWER_MODULE_CONSTRUCTOR_HANDOFF.md)
identifies the real power-module request producer and mandatory pressure owner
before allocation. Body-heap capacity does not supply that separate pressure
environment. This is the next prerequisite for body/flag packet production;
all broader damage/vitality/visual gates below remain required.

## Scenario request producer increment

The native request producer now implements complete D6660 and binds its real
initializer, raw policy and bounded nearest-domain math owners. Selected-image
type-record offsets/stride and actual retained scnr/palette bytes produce fresh
C4 requests, including all fourteen authored Lockout power modules. Definition
NONE preserves the original failure writes. No object allocation, completed
constructor, activation or live shield collection follows from a request.
[Implementation, FP boundary and differential evidence](SOURCE_SCENARIO_CREATION_REQUEST.md).

Unchanged TU1.1/u5 executions agree on 623 cases each. The native comparison
matches 1,246 C4 outputs/defined returns, 97,324 ordered stores and 6,524 lower
call ABIs. All authored requests also match the actual cache-backed binding;
independent original captures repeat byte-identically. The next required binding
is actual current-world main/overflow state and services at148360 before allocation.
Body storage is a separate arena, and its capacity is not pressure admission.
The goal remains active, including all broader producer/vitality/visual gates.

## Main-heap producer and pressure increment

Complete TU1.1 `1482A0` now obtains real retained host pages through the recovered
`33785C` resource interface and preserves alignment metadata, global stores and
its distinct success/error EAX domains. The existing full-ABI `148190→22E4E0`
constructs the real main pool; allocation/free and pressure read that same bank.
Two unchanged-original sequences repeat byte-identically. The focused native
comparison matches326 operations,5,254 ordered stores and351,199,800 retained
bytes, including reuse, reset, all pressure levels and three real source page
exhaustions. Four related regression checks pass without skips.
[Implementation and validation](SOURCE_MAIN_HEAP_STORAGE.md).

Original pressure routines also execute unchanged through a host ELF32 shim:
all324 scalar outputs agree with the emulator, while hardware supplies the SSE
precision flags omitted by Unicorn in309 cases. This is host x86 evidence, not
Xbox hardware proof. The final full current-tree build is blocked by the separate
concurrent menu vertex catalog rejecting its generated entry; the isolated heap
build/comparison passes. No renderer workaround or quiet-pressure result was used.

Complete current-world startup, renderer/process-heap resources, overflow and
reclaim ownership remain required before authored power-module construction and
live shield registration. The goal remains active with the full damage producer,
health/shield and visual coverage gates below.

## Retained process-heap resource increment

Real Linux reservation/commit/release ownership now preserves one retained bank,
per-page accessibility and source writes on recommit. Nine complete paired
constructor/segment/descriptor/SEH/copy bodies compile957 original instructions.
The startup-shaped constructor reaches its required ordinal291 owner after real
reserve/commit and actual header/list stores. It does not install566F08 or supply
a lock/TLS/allocator result.

Unchanged TU1.1 comparison matches426 explicit standalone cases,6,997 ordered
stores,10 actual reserve/commit calls and24,576 retained committed heap bytes,
including full caller/SEH/FS state and all231 copy instructions. Independent
original captures repeat byte-identically across770 files. All10 related CTest
checks pass without skips; the refreshed full native build and CLI check pass.
Xbox kernel failures/clobbers, live startup synchronization,
later growth and broad damage/vitality/shield pixels remain open.
[Implementation and measured limits](SOURCE_PROCESS_HEAP_STORAGE.md).

## Process-heap initializer increment

The complete compiled constructor now joins its authenticated reserve/commit
frames and ordinal291 initializer to the same actual retained bank and shared
native lock owner. Unchanged TU1.1 comparison matches432 explicit cases,
9,711 ordered game stores,22 real resource calls,57,344 committed heap bytes and
56 separately executed compiled compatibility initializer stores. Independent
captures repeat byte-identically across780 files. Seven flags2 constructor cases
return their source-produced heap, including both source-selected lock layouts.
The earlier426-case boundary fixture also passes unchanged.

All14 related CTest checks pass together without skips. The refreshed full native
build and CLI check pass. The [initializer record](SOURCE_PROCESS_HEAP_INITIALIZER.md)
retains the exact tests, captured inputs, resources and evidence limits.

This is native platform/compatibility-source evidence, with original Xbox kernel
clobbers/failure semantics still open. The next full game owner is2D7F8E, whose
shutdown/DPC/timer/hardware setup precedes installation of566F08. No naked global
store, supplied TLS/callback result or ordinary renderer allocation is admitted.
The complete damage/vitality and shield pixel scope remains active.
[Implementation, comparison and limits](SOURCE_PROCESS_HEAP_INITIALIZER.md).

## Process-heap startup and shutdown registration increment

Complete source startup2D7F8E and setup2DC5FF/2DC5D2 now compile for both
authenticated images, adding169 original instructions. The selected-image loader
also retains the literal header. A persistent native shutdown registry owns the
actual source record and exact pinned compatibility list expressions under a
declared serialized native ABI; Xbox IRQL/interrupt/callback behavior remains
separate.

Two36-case original comparisons cover both images through their real first
boundaries and through actual shutdown registration to DPC initialization107:
528 ordered game stores and48 separately compiled primary list stores. A separate
24-case/288-operation registry comparison covers768 ordered stores, signed/equal
priorities, removal and actual duplicate topology. Each capture independently
repeats byte-identically. Full startup still requires107/113, actual hardware/
settings data and timer scheduling before the constructor and566F08 installation.
Ordinary renderer allocation and shield pixels remain open.
All18 related checks pass without skips; the full native build and CLI check pass.
The configured source snapshot and concurrent unrelated revisions are recorded.
[Implementation, evidence and limits](SOURCE_PROCESS_HEAP_STARTUP.md).

## Falling, moving-support and request increment

Six complete paired source bodies now compile557 original instructions for both
images, including original request initialization and attribution. TU1.1 biped
phase adapters join the actual existing velocity, buoyancy and animation owners.
The comparison retains the original strict threshold, immunity/parent gates,
support angular/linear motion, attached-node point transform, live BSP field and
partial-width attribution stores. Accepted diagnostic falling requests reach
the real `D9180` frame with authored `globals\distance` and a complete request.

Each original image executes137 declared standalone cases,22,390 instructions
and4,618 ordered stores, and repeats byte-identically. Every support, point and
request-leaf instruction executes. Both raw native comparisons and the TU1.1
join pass, including8 defined-input phase calls and24 absent-owner prefixes.
All24 related checks, the full native build and CLI help check pass. The SMG
fixture now waits for the real spawn-protection countdown before comparing its
unprotected original hit references; production scaling is unchanged.
These inputs do not prove a naturally admitted fall, support registration/sample
lifecycle, complete damage delivery, shield vitality update or visible shield
hit effect. Those owners remain required by this active goal.
[Implementation, validation and remaining gates](BIPED_FALLING_DAMAGE_RUNTIME.md).

## Work order and completion gates

The retained source-world path now constructs and activates the actual model
resource pools and cache, selects its physical budget from actual multiplayer
options, and joins original model/header admission to fatal cleanup. Queued
resources remain nonresident until real I/O and fixups complete. This selected
resource callback does not claim full retail stage lifecycle or final shield
submission. [Live connections and evidence](SOURCE_MODEL_RESOURCE_LIVE_DELIVERY.md).

Complete `A8620` now joins the actual fatal cleanup dispatcher and follows its
real session-byte gate. All 256 byte inputs match both original executables;
the actual two-player Match advances from `FB2FC → A8620` to `CFB80 → BA980`.
No active entity outcome is supplied. Cleanup, corpse detachment and death-view
selection remain unfinished. [Connected owner and evidence](SOURCE_FATAL_SESSION_GATE_DELIVERY.md).

The complete original vitality update, health restoration and overshield
activation now compile with their authenticated local owners:14 paired bodies,
976 instructions. Original/native comparisons pass3,584 cases per image,
including a continuous2,801-phase activation, charge, full drain and HUD-history
sequence. Real effect/section/actor/replication callees remain required, and the
live Simulation still uses its ordinary scalar update. The new module does not
close pickup admission, full damage delivery or shield pixels.
[Implementation, exact evidence and live gates](SOURCE_VITALITY_RUNTIME.md).

The shared direct dispatcher now has complete retail/TU1.1/TU1.5 correspondence,
including all immediate callee bodies and the first amount producer. Retained
kill-trigger and falling frames identify the next real state/callee boundaries;
they do not establish complete delivery or permit supplied callback outcomes.
[Dispatcher implementation handoff](DAMAGE_DISPATCH_RUNTIME_HANDOFF.md).

The latest startup increment joined the partial DPC/timer initializers107/113 and
stops before reading unowned hardware import322. A repeated36-case complete
startup capture contains372 source stores and408 compiled primary platform
stores; native dirty-record and original-prefix comparisons preserve every
untouched byte. Hardware/settings data, timer scheduling, heap installation and
shield pixels remain open.
[Initializer integration and evidence](SOURCE_PROCESS_STARTUP_RECORDS.md).

The falling increment now compiles those producer/request leaves and connects
the existing prephysics-defined call ABI. The real support-bank lifecycle,
player callbacks, complete delivery and live physics admission remain genuine
dependencies. Kill-volume death uses separate admission.
[Falling source handoff](FALLING_DAMAGE_PRODUCER_HANDOFF.md),
[current native integration](BIPED_FALLING_DAMAGE_RUNTIME.md).

1. Build a reproducible original-cache reference census: all local maps and
   shared-resource chains, every damage-bearing tag/field and executable caller,
   effect parts and script opcode references. Record hashes, image version,
   resolved/missing ownership and category; resident references are not a whole
   game inventory. Keep this document's rows open until that census is reviewed.
2. Close the shared result/presentation seam first: authored refs and ordered
   attempts, actual recent body/shield producer and decay state, `BCC20` inputs,
   and source-selected first-person/world overlay admission. Add only the next
   boundary whose original state and external services are known.
3. Carry each category through its genuine producer into the shared pipeline.
   Prioritize already-live SMG, frag/plasma and prop paths for continuous
   end-to-end verification, then implement recovered melee, environment,
   script/equipment, remaining weapons and vehicle paths without inventing the
   unavailable physical owners.
4. Recover recharge, health pickup, overshield charge/drain and variant/actor
   scaling at their owning call sites; compare ordinary and special paths
   without broadening an ordinary scalar fixture's claim.
5. For each boundary, execute unchanged original instructions and independently
   repeat the capture. Compare float bits, complete relevant flags/timers,
   ordered requests/callbacks, owner datums and RNG consumption. Declare supplied
   time, collision, allocation, actor, mode, resource and platform services.
6. Exercise real native controls in actual maps: target/cover admission,
   shield-only damage, break/overflow, health death, recharge, repeated hits,
   melee, grenade attachment/blast, chain destruction, fall/contact/volume and
   powerup cases. Include Spartan and Elite, shielded and unshielded actors,
   first-person and world views, all supported local viewport counts, and native
   LAN join/repeat/loss handling. No supplied target/health/pose/contact should be
   presented as natural gameplay evidence.
7. Capture shield visuals against an original executable or verified Xbox
   reference at controlled damage states and phases. Retain actual shaders,
   textures, materials, clock/input history, pixels and frame/viewport settings.
   Native screenshots alone demonstrate output; they do not prove original
   appearance. Rendering additional views must not apply gameplay damage,
   advance effect state or consume additional RNG.
8. Rebuild the delivered native artifact and run the relevant ordinary
   simulation, damage, attribution, death, grenade/prop, HUD, shader and LAN
   checks. Completion requires the reviewed census and all applicable live
   owner/visual gates; unresolved physical, runtime or platform branches remain
   named gaps even when isolated arithmetic and packaging pass.

No schedule is asserted. Advance by verified ownership boundaries and record
the first unavailable dependency where original execution cannot continue.

## TU1.5 player screen flashes, 2026-10-20

The missing native full-screen damage response now has a separate player-view
consumer. Authority is unchanged English TU1.5, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`:
`CC590 -> 156840 -> 1578A0 -> 156E40 -> 40880`. The oracle executes the
selection prefix, whole slot admission, whole per-user sampling and original
`134960` color packing; it stops before sound, rumble and camera services.

- `156840` selects the first response whose type differs from the current
  shield-positive boolean. Duration and intensity must both exceed `0.05`.
  Failing those gates does not continue to a later row.
- It overrides the authored flash type/priority to LIGHTEN/medium and selects
  ARGB `(1,0,.6,.8)` with shields, `(1,1,0,0)` without shields. Ordinary
  duration is `.2`/`.3` seconds at intensity `.3`. Authored duration above
  `.5` and actual signed seat `NONE` instead selects `.8` seconds, intensity
  `.6`, curve1. Report scale passes through original curve2.
- `1578A0` retains a higher-priority flash only while its remaining seconds
  exceed the incoming duration. Duration-to-ticks truncates; sampling uses
  the original transition tables. Repeated admitted hits replace/refresh the
  user slot. There is no hit-count threshold in this producer.
- `40880` follows `1977AB`'s HUD dispatch. Its LIGHTEN branch uses quantized
  premultiplied channels, ONE/INVSRCALPHA and RGB-only writes. `134960` uses
  x87 multiplication before integer rounding; a float-only packing substitute
  differs, for example at `.3f * 255`.

Campaign and multiplayer consume the existing admitted player-response
journal. Event-time shield, seat and flash scale survive native LAN protocol92;
each local viewport owns an independent timer. Late packet batches advance
the flash by authoritative event/tick ages; a stopped simulation clock does
not advance it.
The renderer schedules each flash after its HUD and before pause UI. A
nonzero global cinematic fade suppresses the damage flash. This preserves the
current native fade adapter; exact global-script fade admission remains its
separate source ownership boundary.

`tests/fixtures/player_screen_flash_tu15/` contains 1,320 original timed samples,
144 priority/truncation cases, eight multi-row selection cases and the two
needed original lookup tables. `player_screen_flash_tests` also loads every
authored Lockout damage response, including cache-interned blocks. The Vulkan
regression checks actual native response/UDP delivery, red and blue output,
HUD/pause ordering, two/three/four-view isolation and opaque fade suppression. Temporary
captures belong to `HALO2_TEST_SCRATCH`.

Reproduce original CPU evidence with:

```sh
python3 tools/run_test.py -- .tools/python/bin/python \
  tools/oracle_player_screen_flash_tu15.py \
  --output tests/fixtures/player_screen_flash_tu15
python3 tools/run_tests.py --target player_screen_flash_kernel_tests \
  --target player_screen_flash_tests \
  --filter '^original_tu15_player_screen_flash(_kernel)?$'
python3 tools/run_tests.py --target player_screen_flash_render_tests \
  --filter '^native_player_screen_flash_render$'
```

This closes the missing damage-flash consumer, not every full-screen effect.
The initial disc census spans 29 caches: 2,588 damage-tag appearances and 2,866
response rows meeting the two authored flash gates. All HUD screen-effect tag references
are NONE, including the Battle Rifle's mask-only widget; existing scope mask
handling remains separate. The only `egor` names in that census are blur and
desaturate. Existing cinematic fades, letterbox, crossfade, bloom and menu
filters retain their own evidence. Cinematic depth-of-field still lacks its
Vulkan scene/filter/composite integration, as recorded in
`CUTSCENE_TU15_COMPARISON_20261016.md`. Other generic flash blend modes are not
emitted by this damage wrapper and are not certified by its LIGHTEN test.
Full source-hit coverage, original-Xbox framebuffer comparison and installed
launcher delivery must not be inferred from these component checks.

### MCC flash-tag import census

Damage flashes have no separate bitmap/shader dependency: their definitions
are inline in `jpt!` Player Responses. `tools/mcc_player_screen_flashes.py`
now inventories that complete block, using the native unloaded-only shared
directory merge and exact same-address/group ownership for whole-tag aliases.
It rejects escaped pointers, missing alias backing and nonfinite flash fields.
Checking only nonzero-sized directory entries would miss, for example,
Cairo's Elite boarding-melee flash, which shares the Brute definition.

The selected prepared MCC cache
`assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b/` contains all 100
distinct flash-producing tag names across 43 caches: 4,298 damage-tag
appearances, 2,596 flash-producing appearances, 4,807 admitted authored flash
rows and 212 exact whole-tag aliases. All response fields match the installed
MCC source copy. Flash-producing definitions in all 30 matching original
Xbox reference caches also agree exactly (1,631 appearances / 3,020 flash
rows); additional MCC maps have no matching original-map claim here.

`player-screen-flashes.json` in that prepared cache contains every tag's full
salted identity and every flash field as exact integer/float bits. Its hash
and census are recorded in `provenance.json`. The fields were already present
in the copied map metadata; no original tag injection or map rewrite was
needed. `prepare_mcc_gameplay.py` now creates this checked inventory on each
future import. The human-readable names and bounded source/import comparison
receipts are in `analysis/player-screen-flash-tu15-20261020/`.

`mcc_all_player_screen_flashes` opens every inventory map through the actual
native `Cache` converter and loads every damage tag through
`load_player_screen_responses`, including aliases and interned blocks. It
compares all ten authored fields bit for bit and checks that no resident
damage tag was omitted. Reproduce with:

```sh
python3 tools/run_test.py -- python3 tools/mcc_player_screen_flashes.py \
  --mcc-root assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b/halo2 \
  --reference-maps assets/disc/maps \
  --output '@HALO2_TEST_SCRATCH@/flash-inventory.json'
python3 tools/run_tests.py --target player_screen_flash_mcc_tests \
  --filter '^mcc_all_player_screen_flashes$' \
  --cmake-arg=-DHALO2_MCC_PLAYER_FLASH_MAPS="$PWD/assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b/halo2/h2_maps_win64_dx11" \
  --cmake-arg=-DHALO2_MCC_PLAYER_FLASH_INVENTORY="$PWD/assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b/player-screen-flashes.json"
```

Validation on 2026-10-20 passed all four native suites (the two original
state/content suites, Vulkan/UDP rendering, and all 43 MCC caches), plus five
import boundary regressions. The controlled source hashes and CTest results
are in `build/test-results/player-screen-flash-complete/`; the bounded final
receipt and cleanup confirmation are under
`analysis/player-screen-flash-tu15-20261020/validation/`. The disposable build,
frozen validation copy and captures were removed. No large test artifacts or
new launcher runtime were retained.
