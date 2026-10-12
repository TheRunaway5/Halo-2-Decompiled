# Remaining live damage joins

2026-10-12. Research-only audit of the current root source. No production or
test changes, damage delivery, contact result or successful lower service was
supplied. Existing historical handoffs are navigation; the code observations
below were rechecked against the current files.

## Completed source work already connected

Two high-priority claims in
[the earlier coverage handoff](DAMAGE_COMPLETE_LIVE_COVERAGE_HANDOFF.md) are stale:

- TU1.5 already routes the paired damage module on the retained world bank.
  `PhysicsLiveWorldTu15::Impl::required` selects actual addresses from
  `physics_live_entries_tu15` and invokes `source_run_damage_delivery_runtime`
  with the selected image, full ABI, actual bank and recursive services.
  A fresh mechanical check found all 90 generated entries represented at their
  authenticated TU1.5 address: 86 use `Module::Damage`; four already have earlier
  ownership (`B8250 -> B9390` World; `B9C20 -> BAD60`, `BBCB0 -> BCDF0`, and
  `177A10 -> 178E30` Phantom). This checks routing, not natural damage admission
  or completion of every lower service.
  [Dispatcher](../src/physics_live_world_tu15.cpp#L379),
  [entries](../src/physics_live_entries_tu15.hpp#L665),
  [generator](../tools/generate_physics_live_entries_tu15.py#L22).
- TU1.5 initial equipment now uses the bound source player/unit and original
  `B1A70 -> B1C30 -> 1613B0`, then reads retained equipment and actual weapon
  bodies. Match invokes it after source biped birth and publishes the result
  using `synchronize_source_equipment`. The older header-only starting-weapon
  limitation still applies to the native fallback, not this TU1.5 branch.
  [Source producer/readback](../src/physics_live_world_tu15.cpp#L490),
  [spawn join](../src/multiplayer.cpp#L953).

The same-bank source world is optional: `--mp-source-object-world SAVE_DIR`
selects it. `MultiplayerWorld` otherwise returns no source object world.
[CLI](../src/main.cpp#L197), [selection](../src/multiplayer_world.cpp#L27).
Do not describe ordinary demo hits as raw source-produced damage solely because
these complete modules are available.

## Smallest actual unwired seams

| Priority | Current behavior | Required connection and verification |
| --- | --- | --- |
| 1 | `apply_unit_damage` already returns ordered shield-depleted and threshold-damaged effect requests, including NONE attempts. `Simulation::damage` consumes normalized amounts and flags but discards `result.effects()`. | Deliver each admitted request once through an authoritative gameplay event owner, preserving its exact species definition, full unit identity, source order and default-owner versus incoming-owner distinction. Verify natural repeated hits, break, overflow, body-only death, NONE Spartan threshold attempt and both species in first-person/world views. This existing scalar seam supports native presentation; it does not close the complete raw dispatcher. [Requests](../src/unit_damage.cpp#L35), [owner contract](../include/halo2/unit_damage.hpp#L4), [discarding caller](../src/simulation.cpp#L660). |
| 2 | `Simulation::step_players` calls scalar ordinary `recharge`, discards its Boolean result and schedules no authored recharge effect. The source raw ordinary vitality body is already available as canonical `D73E0` / actual `D8330`. | For native presentation, retain the actual transition from non-recharging to recharging and deliver the authored effect exactly once, with sound/attachment/clock consumers. For final source authority, use the original produced flags/effect request after its once-per-tick update, rather than independently calculating or copying a host transition into raw memory. Verify break, full stun delay, interruption by a new hit, resumed recharge, completion and dead-unit rejection. [Current update](../src/simulation.cpp#L1108), [scalar scope](../include/halo2/kernels.hpp#L89), [complete source interface](../include/halo2/source_vitality_runtime.hpp#L5). |
| 3 | Source-motion Match reads source motion and equipment after source object phases, but publishes vitality and damage feedback from independent host `Unit` fields. Host damage, scalar recharge and feedback fade still mutate those fields. | Introduce one read-only vitality/feedback publication over full salted object identity after the true source phase; preserve maxima, fractions, flags, signed stuns/timers, requested normalized report amounts and recent damage. Disable parallel host mutation only when genuine source producers plus their lower aftermath succeed. Test one admitted source hit and ordinary recharge over consecutive real ticks, then compare Match/HUD/shader/replica fields to the same source body. [Host mutation](../src/simulation.cpp#L636), [host update](../src/simulation.cpp#L1112), [source motion-only publication](../src/multiplayer.cpp#L1345), [snapshot](../src/multiplayer.cpp#L1516). |
| 4 | Melee animation reaches its authored impact tick, but `OrdinaryClock` explicitly discards `advance_source_melee_animation`'s impact return. No damage or contact request follows. | Execute complete original `D0830` contact/selection at the admitted ordinary `EB700` event, using actual weapon, current source pose and its 5 by 5 casts. Preserve full contact/material/node/model-entry/parent/owner context and real damage/effect selection. A generic immediate ray, fixed range or damage tag alone is not this producer. Verify real input, no damage before authored impact, a positive current-pose contact and miss, source recovery/combo and dual/alternate/objective-item paths. [Discarded event](../src/native_melee_animation.cpp#L34), [original ABI/evidence](MP_MELEE_ACTION_SOURCE_RESEARCH.md#L116). |
| 5 | Host Match scans every imported kill-trigger row against `Body.position`, then directly sets dead/body. The source phase has its own row selection and bounds center. | Let the original postphysics phase own row admission and build its `globals\\distance` request; read the genuine fatal result and aftermath into Match. Only then remove the host bypass. Verify enabled/disabled and boundary rows, one-row scheduling, actual bounds center, momentum, credit, corpse and respawn. Replacing the host point operand alone is insufficient. [Bypass](../src/multiplayer.cpp#L1354), [original retained producer](MP_SPARTAN_KILL_TRIGGER_RESEARCH.md), [source fatal stimulus remains distinct](../src/multiplayer_object_runtime.cpp#L702). |

`finish_death` currently calls `kill_biped` only after the host has confirmed a
fatal result; the source constructor of that fatal record is an explicit native
stimulus, not proof that the original incoming damage producer admitted it.
[Caller](../src/multiplayer.cpp#L516),
[contract](../include/halo2/multiplayer_object_runtime.hpp#L52).

## Remaining producer blockers

These cannot be completed by attaching the existing scalar damage kernel to a
new generic contact:

- **Direct weapons and regions:** native `trace` still targets unit capsules.
  The complete dispatcher accepts genuine node/model-entry/region state, but
  the live hit producer has not produced it. Headshots, section responses and
  current permutations require the original posed collision path. Weapon
  importer selection still retains the first authored barrel and one impact
  damage reference; all authored triggers/barrels and alternate projectile
  references need their actual selectors.
  [Current trace](../src/simulation.cpp#L746),
  [request contract](../include/halo2/source_direct_damage_runtime.hpp#L5),
  [owned census](../analysis/damage-shields-goal-20261012/damage-coverage-research/owned-cache-damage-census.json).
- **Grenades and area damage:** the current source-derived action, marker,
  fuse and native terrain bounce deliver a real host area-damage callback.
  Native grenade collision explicitly excludes unit shapes; attaching/sticking
  cannot be enabled by relabeling terrain. Area candidates use native capsule
  nearest points and ray obstruction; original candidate order, hierarchy,
  cone/random/impulse and complete per-object admission remain separate.
  [Contact limitation](../src/native_grenade.cpp#L283),
  [detonation callback](../src/native_grenade.cpp#L313),
  [area consumer](../src/simulation.cpp#L687).
- **Falling:** complete paired excessive-velocity and airborne bodies now exist
  and TU1.1 phase routing includes them; the old claim they are unimplemented is
  stale. Their actual parent/support/buoyancy/physical state, player callback
  and complete damage continuation must be reached naturally. Distinct
  `globals\\falling` impact admission remains unrecovered; excessive velocity
  uses `globals\\distance`, not an invented fall-height formula.
  [Current complete bodies](../include/halo2/biped_falling_damage.hpp#L5),
  [current phase join](../src/biped_phase_runtime.cpp#L12),
  [original exact gates](FALLING_DAMAGE_PRODUCER_HANDOFF.md).
- **Effect/item/weapon self-damage:** firing event damage references currently
  become rumble requests. Original effect `jpt!` parts need their genuine
  gameplay scheduler and owner/location/direction/scale before radius dispatch.
  A renderer visiting each split-screen view must not execute gameplay damage.
  [Current firing/chamber events](../src/simulation.cpp#L834),
  [original part body](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/0017a8a0-retail.asm).
- **Physics collision, vehicles, props, script and campaign:** source contact
  impulse/history and real seat/parent/rider banks are prerequisites for their
  incoming damage producers. `PropSandbox::damage` remains a separate scalar
  state with bounded authored response scheduling. Script vitality setters are
  not the four original typed damage roots. Resident assets outside Lockout do
  not establish executed producers.
  [Prop mutation](../src/prop_sandbox.cpp#L105),
  [script host](../src/simulation.cpp#L1487),
  [exact producer inventory and acceptance gates](DAMAGE_COMPLETE_LIVE_COVERAGE_HANDOFF.md#L184).

Source-physics command delivery currently writes crouch/jump plus look/movement
only. Fire, grenade and melee remain the host action paths; complete original
player command/action roots are not invoked by forwarding these native
`Controls` through the current source command block.
[Exact delivered fields](../src/multiplayer.cpp#L1321).

## Verification performed and next acceptance

This audit re-read the current caller/callee joins above and mechanically checked
the 90 generated damage-entry selections against the live actual-address table.
It did not run or claim a new gameplay or original execution comparison.

Existing evidence that can be reused with its limits:

- `source_direct_damage_runtime_tests` checks declared raw banks and pending
  services against unchanged original instructions; it does not produce a
  natural hit. [Scope and retained failures](SOURCE_DAMAGE_DELIVERY_RUNTIME.md#L48).
- `physics_live_world_tu15_tests` creates a genuine bound player/biped and
  original equipment, then drives 180 movement/world ticks. It currently
  asserts movement and equipment, not a damaging hit or source vitality
  publication. [Test](../tests/physics_live_world_tu15_tests.cpp#L187).
- `multiplayer_grenade_delivery_tests` exercises real SDL/UDP Q and natural
  native terrain/fuse/blast damage, with its explicit native collision limits.
  [Test](../tests/multiplayer_grenade_delivery_tests.cpp#L77).
- `native_melee_gameplay_tests` currently assigns positions and clears map
  collision; it is a useful contact-integration failure fixture, not natural
  gameplay proof. The production event currently discards impact, so this test
  cannot establish damage completion. [Test](../tests/native_melee_gameplay_tests.cpp#L10).
- `multiplayer_kill_trigger_momentum_tests` starts from an explicit debug
  teleport and verifies the host kill postpass; it does not invoke the complete
  original selector/request path. [Test](../tests/multiplayer_kill_trigger_momentum_tests.cpp#L47).

All asset-bearing new checks must use the intentionally retained backup rather
than restoring or rewriting root `assets/`:
`/home/eric/.local/share/halo2-native/cache-reset-backups/20261012T201413Z/assets`.
Pin the selected executable image per check: TU1.1 at
`multiplayer-map-pack/demos/update/Updates/default.xbe.03` under that directory;
owned English TU1.5 at
`/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`.
