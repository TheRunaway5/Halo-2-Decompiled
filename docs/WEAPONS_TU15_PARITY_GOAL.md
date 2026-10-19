# Xbox TU1.5 weapon parity goal

Use [the remaining-work checklist](WEAPONS_TU15_GOAL_CHECKLIST.md) and
[current goal state](../analysis/weapons-tu15-goal-20261013/goal-current.json)
to resume this goal. The checklist is part of the completion contract: maintain
it and the per-weapon matrix after each verified milestone, preserve unresolved
items, and complete the cleanup obligations before declaring the goal complete.
The sections below retain the detailed requirements and historical evidence.

Status: **incomplete; work continuing**. The requested end state is feature and visual
parity for every weapon, including campaign/AI, vehicle/turret, objective and
otherwise unused authored variants. Passing a component test does not complete
a weapon. Original executable/disc data and verified original execution remain
the authority; unresolved owners are not replaced with tuning or artwork.

Runtime acceptance uses the project's prepared **MCC cache only**. Xbox ISO and
Vista caches are offline references and must not be substituted for runtime
inputs. Historical ISO-cache checks, including the v69 Cairo application run,
remain reference evidence and do not establish MCC acceptance. Future tag
conversion belongs to the MCC import path; validation must use its actual
output and expose unsupported data rather than inject reference tags.
`tools/validate_weapon_material_snapshot_tu15.py` requires an explicit
`--mcc-root`, validates version13 headers and records input hashes. MCC runtime
cases use compiled TU1.5 startup without an Xbox image constructor. The old
ISO-camera scenery fixture is excluded from this MCC acceptance route until an
MCC camera case is established.

The goal also requires removing unused test builds, compiler outputs and scratch
after each completed validation and before declaring the overall work complete.
Use `tools/run_tests.py` for CMake validation and `tools/run_test.py` with
`HALO2_TEST_SCRATCH` for one-off probes, emulator and capture work, as described
in `docs/TEST_STORAGE.md`. Keep small reports, commands, source hashes and
diagnostics. Retain large outputs only as required reference inputs, with their
paths, sizes, hashes and restoration/retention reason recorded. Preserve original
assets, existing reference evidence, frozen source snapshots, launcher-selected
runtimes and unrelated active work. Verify scratch removal before reporting a
validation complete. Cleanup is part of completion, not a separate optional task.

## Scope and completion requirements

`tools/audit_weapon_scope_tu15.py` currently finds **63 distinct weapon paths,
1,110 authored rows and 38 campaign/multiplayer maps**, including the owned
TU1.5 DLC maps. The complete per-map identities, root/trigger/magazine/barrel
bytes and original image/cache hashes are retained in
`analysis/weapons-tu15-goal-20261013/authored-scope.json`. Its per-weapon feature
and visual status remains unverified for all 63 definitions.

`weapon-completion-matrix-v75.json` expands those same63 rows into630 explicit
completion gates, including owner/applicability audits and complete reference
sequences. Bounded Hunter and Needler runtime evidence is attached to their
tested Old Mombasa and Lockout definitions; it certifies neither a whole gate
nor all authored variants. No weapon
is fully certified.

The separately retained MCC inventory reads all43 prepared caches and finds
1,479 weapon rows across66 distinct paths. All63 Xbox reference paths are
present. The additional MCC `smg_silenced`, `gpmg` and Falcon `rpg` paths have
now been checked against all 41 owned Xbox map/shared/menu directories: none is
present. Their definitions belong to both MCC shared banks, with nonresident
multiplayer aliases. They remain preserved MCC additions outside the original
63-weapon reference catalog, with gameplay unverified. The ownership and scope
decision is recorded in `mcc-additional-weapons-original-directory-audit-v1.json`.
Directory identities
are recorded in `mcc-authored-weapon-scope-v1.json`. These MCC inputs, rather than
the original map occurrences, govern new runtime acceptance.

Completion requires each applicable weapon contract to be implemented and
verified: trigger/barrel selection, burst/release scheduling, ammunition and
battery ownership, reload/chamber/ready and interruption, heat/venting/charge,
aim/error/recoil, damage and model-region/headshot admission, projectiles and
their contact/attachment/flight/detonation, special homing/lock-on/supercombine
and sword lunge/melee behavior, pickup/switch/dual/drop and world lifetime.
Vehicle/AI/objective weapons retain their actual owning actor/seat/game rules.

Visual verification includes actual world and first-person models/regions,
both playable species and applicable dual hands, current/saved poses and
blend/IK/camera ownership, materials/textures/mips/lighting, ammunition/heat/
power shader producers, HUD/scopes, ready/charge/fire/reload/empty/impact/
detonation effects and authored audio. Local, split-screen and LAN publication
must retain weapon identity and independent hand/event histories. Original
captures or independently executed source render inputs must substantiate
claims; finite poses or host Vulkan validation alone are insufficient.

## Current firing corrections

The native generic loop previously rejected several zero-rate weapons and did
not latch trigger styles4/5. The battle rifle repeated bursts while held. The
carbine read barrel0, although both authored trigger indices select barrel1.
The importer now follows the actual trigger reference, including the authored
secondary-only data reference of charge/autofire NPC weapons. Their separate
charge manager is still required.

`weapon_barrel_timing.hpp/cpp` implements complete TU1.5 `1071D0`, `108F30`,
`109030` and `108EA0` timing consumers, plus the ordinary barrel-update phase.
The live ordinary path uses discrete counter/burst/recovery ownership for
triggers0/1, identical-barrel trigger4 and ordinary unguided trigger5. The
positive-heat ordinary path now has the recovered energy phase below.
The SMG keeps its existing source path. The native adapter still owns upper
admission, reload transfer, projectile creation and unsupported alternate
target/zoom branches; this is not a complete original weapon manager.

`tools/oracle_weapon_barrel_timing_tu15.py` directly executes the unchanged
TU1.1 and TU1.5 complete timing owners and actual CRT callees. Both images
produce identical outputs for **5,271 cases over 48 distinct authored timing
contracts**, covering gate, both recovery phases and reset. All52 retained
barrel bytes match the native functions. These explicit object/tag/time/state
ABI controls supply no game-service results or instruction patches. Static
complete-body/callee correspondence and all executed original instructions
are retained beside the original report.

`tools/oracle_weapon_firing_cycles_1_1.py` independently executes original
constructor, input, weapon update and two projectile postpasses for magnum,
battle rifle, carbine, shotgun, sniper rifle, brute shot, Needler and unguided
rocket. The actual-asset native match regression matches **24 scenarios and
1,828 ticks and 132 prepared requests** of ammunition, counter, mode, countdown,
burst count, rate, recovery, effect identity and scalars. Held/released, rapid
pulses and short-burst release are included.
The original fixture declares raw storage and inherited animation/effect/
projectile service boundaries; comparison stops before reload/chamber admission.
It is TU1.1 full-update evidence and direct TU1.5 timing evidence, not a full
original TU1.5 world execution.

The first integration run caught automatic reload resetting the empty last
shot's recovery. The corrected native path defers it until idle barrel mode,
as original `103365` does. Its failed log is retained. Six focused regressions
pass, including existing Needler firing effects and projectile construction,
native gameplay/inventory and authored weapon actions.

## Heat, battery, error and preparation

`weapon_energy.hpp/cpp` implements the original heat/cooling/overheat/vent
phase, battery age stores, event4-based vent timing, and preparation misfire
and heat-detonation decisions with their shared random stream. Raw authored
operands remain data. Misfire depends on spent battery, not heat. A zero
detonation chance still consumes the original random value when heat passes
the authored threshold. Successful self-detonation and persistent overheat
effect allocation/lifetime remain required separate owners.

Complete static body correspondence identifies TU1.1 `1021D0`, `105F60`,
`107160`, `16A06A` and their TU1.5 owners `1030F0`, `106E80`, `108080`,
`16B44A`. Original execution of the declared heat fragment and complete
battery routine matches **42,592 heat and 23,900 battery cases** over 248
authored root contracts. Ordered zoom/action/animation/effect requests are
checked, with those services explicitly supplied at the ABI boundary. These
tests do not assert successful effects or a complete game tick. Another
**3,570 timing cases** execute original event4 lookup/conversion on 683
authored animation records plus declared boundary records; clip resolution is
an explicit supplied boundary. **784 preparation cases** compare misfire and
detonation decisions and the exact retained random word in both updates.

`oracle_weapon_energy_cycles_1_1.py` executes original constructor/input/full
update/two postpasses for plasma rifle, brute plasma rifle and beam rifle.
The actual-asset native regression matches **18 scenarios, 8,130 ticks and
450 non-NONE prepared requests** with both TU1.1 and TU1.5 data: heat, spent,
overheat/vent flags, barrel counters/recovery/rate, error/recoil, effect identity
and scalars, and every tick's shared random word. Held/released, pulses and
near-exhausted batteries cover registered and unregistered first-person views.
Both now execute original representation-based graph/timing lookup; an
unregistered view does not imply an absent weapon clip. Original projectile
and overheat-effect allocation return
NONE in this fixture; successful projectile/world and persistent FX ownership
are not established. Native pickup/ready occurs before the compared sequence;
placement and initial spent/seed controls are explicitly declared.

The ordinary native barrel path now completes error/recoil updates before its
projectile postpass, and samples the original direction table. Firing-effect
selection starts at original index0 and advances before selection, retaining
the alternating plasma muzzle bank. Each actual normal/misfire/empty reference
is registered separately for admitted native presentation. Unsupported visual
definitions retain their loader failure.

The native shader state and primary/secondary/dropped-world transport now keep
the independent heat word. Heat and age selectors match **288 cases** of
complete untouched retail, TU1.1 and TU1.5 shader-input execution, including
adjacent values, infinities, signed zero and NaN payloads. Raw state is preserved
through transport and source clamping occurs at shader consumption. Charge,
overheated remaps, HUD battery/heat producers and complete visual response
remain open. The current focused CPU run passes **22 tests**, including raw
primary/offhand/drop UDP round trips, the earlier weapon regressions and these
new original fixtures, the two existing carrier/source-color shader regressions,
the magazine consumers below, and source effect request/visibility checks.

## Magazine reload, transfer and transitions

`weapon_magazine.hpp/cpp` implements complete TU1.1 `106D40` / TU1.5 `107C60`
magazine transfer and `109C30` / `10AB50` mode transitions. The consumers retain
the literal 16-byte record, source signed-short stores and chamber conversion,
empty/full action selection, section0/section3 timing, and notification order.
The native ordinary single-magazine path now composes these with original
`106C30` / `107B50` reload admission, `106E70` / `107D90` chamber admission,
and `101E30` / `102D50` magazine update. It retains reload start/loop/end,
event-timed transfer, chamber-pending stores, and the source trigger gate for
shotgun interruption. Definitions requiring additional battery/parent/world
admission or more magazine owners remain outside this adapter.

The unchanged original consumers agree in both updates for **964 transfers
and 2,088 transitions over ten distinct authored magazine contracts**. The
native regression matches every retained byte and ordered service call,
including wrap/conversion and unavailable-animation timing controls. The
fixture explicitly supplies the external service results; it does not prove
their reserve/infinite-ammo or animation implementations. Complete static
correspondence also covers twelve magazine/reload owners and verifies their
jump-table targets. Complete original unit/own-model action arbitration and
its scheduling/interrupt composition remain pending.

`oracle_weapon_reload_cycles_1_1.py` executes constructor, input, explicit
reload command, complete weapon update and both projectile postpasses for
magnum, battle rifle, carbine, shotgun, sniper, brute shot, Needler and unguided
rocket. The native actual-asset regression matches **96 scenarios, 24,512
ticks and 344 effect requests** with both TU1.1 and TU1.5 data. Every retained
magazine byte, barrel chronology, effect identity/scalars and published ammo
matches. Partial/empty/redundant reload, held-trigger interruption, idle empty
weapons and last-round exhaustion are included, with both registered and
unregistered views. This fixture retains declared raw equipped storage and
the inherited unit/animation, unavailable projectile and effect boundaries;
it is not a complete world or model-owning unit execution.

The comparison exposed initial carry missing FB120/FB360's player-owned ammo
bit, shotgun preparation omitting magazine mode5, and interruption being
checked on a held latched trigger without a new request. It also established
the authored ordering of unused barrels: carbine barrel0 checks automatic
reload before its firing barrel1; the rocket's later unused barrel checks
after the last round has been consumed. The native phases now preserve that
order. Failed comparison and live-probe logs remain retained.

Complete shared-reserve consumers `104AA0` / `1059C0` and `104D30` / `105C50`
match **15,180 queries/spends over eleven authored reserve contracts** in
both updates. All four inventory slots, duplicate references, matching/full
definition identities, signed stores and the supplied infinite-ammunition
predicate are covered. The ordinary native adapter resolves those retained
inventory owners. Full gameplay production of the infinite-ammunition
predicate and integration into all other weapon managers remain separate.

Complete `16A06A` / `16B44A` timing sections match **25,515 cases** over 683
authored clips and declared controls. The original event4/event0 scans and
conversion execute; clip resolution is supplied for this numeric fixture.
Static complete-body and jump-table correspondence also covers `169F3B` /
`16B31B`, `11CF00` / `11E060`, `CEAB0` / `CFB40`, and their event/action
selectors. The source reads the owning player's representation at player+88
independently of local view registration. The native timing adapter now
resolves that representation's authored definition rather than requiring a
rendered local hand. Earlier fixtures that supplied an absent clip are retained
as bounded service-boundary evidence and do not establish remote timing.

The native effect adapter now returns NONE for a NONE effect, as complete
original `107B50` / `108A70` does, instead of retaining its old effect handle.
Six unchanged original calls with an invalid object control verify that this
early exit resolves no object or service; the source consumer regression also
checks this contract.

## Representation and dual graph timing

`oracle_weapon_represented_cycles_1_1.py` selects the actual matg representation
row and represented biped for Elite players. The native actual-asset comparison
matches **96 Elite reload scenarios, 24,512 ticks and 346 effect requests**, and
**18 Elite energy scenarios, 8,130 ticks and 450 effect requests**. Registered
and unregistered local views retain the same original graph lookup. Both suites
also pass with TU1.5 original data. Their existing raw equipped-storage and
unavailable unit/animation/effect/projectile service boundaries still apply.

Complete unchanged graph lookup and timing now agree for **28,512 queries over
all 33 Lockout weapon records**, both playable representations, single/dual mode,
27 activities, four timing sections and declared absent-render-model controls.
TU1.1 and TU1.5 execute the complete query and its graph/event/CRT callees with
no supplied results or instruction patches. The TU1.5 report retains 2,866
reached original instruction rows. Native lookup matches all queries, including
4,720 nonzero results. A separate actual-cache metadata control removes only
the magnum render models and confirms the same timings through the cache loader.
The native timing adapter returns zero for absent records or NONE graphs and
retains a valid graph independently of the render model or local view.

This comparison exposed a dual-action fallback error: the native loader used
the shallow named-action lookup and omitted valid fallback activities. Both
single and dual clips now use the recovered ordered class/type iterator. The
failing comparison is retained in `graph-timing-native-v1.log`; the corrected
native and direct TU1.5 reports are under `graph-timing-*`.

`oracle_weapon_dual_cycles_1_1.py` and the native actual-pickup regression match
**24 scenarios, 6,240 ticks and 830 effect requests** for paired magnums, Needlers
and plasma rifles, with both playable species. The compared owners are ordinary
magazine bytes/shared reserve, eight barrel chronology fields per hand, raw
heat/spent values, energy flags, shared random state and full effect identities/
scalars. Phase order is explicitly primary then secondary. Needler requests
also execute original FC2E0 in supplied generic storage to match its two extra
random writes; generic allocation still returns NONE. This is bounded weapon
phase composition, not proof of original unit routing, attachment, generic
allocation or full world scheduling. Provenance and earlier
failed-boundary comparisons are retained under `dual-cycles-*`.

The SMG adapter now supplies the actual shared-reserve query/spend consumers,
represented dual timing, complete barrel firing gate and magazine-effect calls
through `SmgRuntimeServices`. Original single reloads take 50 ticks; both species'
dual reloads take 110. The old native path used 50 for both. It also omitted the
other-hand gate and magazine-effect submission. **Eight original dual SMG
scenarios, 2,080 ticks and 548 effect requests** now match, including all 52
barrel bytes per hand, ordinary magazine state, shared reserves/seed and effect
ABI order. The comparison also passes with TU1.5 data. Raw constructor storage,
absent local-view registration and inherited external owners remain explicit;
this does not prove complete registered dual-hand feedback or unit arbitration.

The earlier prepared-shot fixture deliberately excluded magazine effects. Its
full original reports are preserved, and a separate pinned derivation now
checks all **170 ordered effect calls over 599 equipped ticks**, including the
165 prepared calls, in both local and UDP matches. The existing 22-case/2,220-tick
prepared fixture still passes. This restores the actual reload effe request
and chamber NONE call; additional generic effect allocation/lifetime remains
outside this SMG numeric adapter.

## Plasma impact spline

The first live plasma-rifle heat case reached a previously unimplemented
type10 spline in its authored impact-particle alpha. This is a particle curve,
not a heat value to suppress. The native evaluator now follows original
`13B390` / TU1.1 `13CB00` / TU1.5 `13DE70`: literal point-distance tests,
bit-level square-root estimate, RSQRTSS, exponent remap, cubic and range blend.
The original instruction ordering and float stores are retained.

`original_shader_spline2_host_x86.cpp` maps the pinned original image bytes
into a separate i386 test process and executes the complete original owner
and its actual CRT. No instructions or services are substituted. All three
images agree on **5,720 cases**, and the native function is bit-exact for those
cases on this host. Inputs include 18 distinct original Lockout emitter/sprite
curves and four declared branch controls. Original Xbox RSQRTSS estimate
precision remains unverified; host x86 execution does not establish Xbox
hardware or framebuffer equality. The live plasma-rifle sequence subsequently
reached firing, full overheat, authored vent poses and cooling.

## Holstered, dropped and interrupted weapon clocks

The native multiplayer adapter now advances ready, heat, cooling inhibitor,
blocking, magazine, firing-effect decay, rate/error and recovery clocks on
retained inactive and world pickup items. The previous adapter froze those
items. Ownerless error recovery uses the literal-one result of original
`D2940`; retained unregistered unit controls use their zero feedback timers.
Each actual barrel advances in authored order, including the carbine's
unused barrel0 and the rocket launcher's second barrel.

Before moving a selected weapon out of a hand, the adapter composes the
weapon-state leaves of original `E9A90`: `105750` conditionally transfers a
mode1 reload only when twice its remaining transfer timer is less than the
original transfer duration, `108230` cancels magazine/trigger state, and
`1042E0` forces every barrel into recovery and queries authored put-away timing.
Input clearing and the second cancellation precede the native inventory move.
The shared-ammunition query/spend runs while the original item is still in its
actual slot. The inventory move retains the resulting loaded and reserve counts.

`oracle_weapon_passive_cycles_1_1.py` executes the complete original update
and both postpasses for **90 declared ownership/state scenarios and 16,200
ticks** across twelve actual weapon definitions. `oracle_weapon_unequip_cycles_1_1.py`
executes the original clear caller and its weapon leaves, then compares
**180 single-hand scenarios and 10,800 ticks** using both actual player
representations. The native comparison uses actual pickup, switch and
replacement/drop transitions before comparing magazine bytes, every retained
barrel byte, trigger/input, clocks and shared seed. The latter fixture also
starts firing-effect intensity at one, exercising its decay. Both suites pass
with native TU1.1 and TU1.5 data.

These are bounded numeric/ownership comparisons. The original fixtures declare
raw object/player/time storage, unavailable own-model/unit-animation/world
services and failed allocations. Dropped reference cases explicitly apply the
post-detach owner/item/parent ABI after the original clear; they do not execute
the complete inventory/world release. The native bridge is disabled when the
source object/physics backend owns updates. Generic world effects, original
world scheduling/physics, complete own-model/action arbitration and the remaining
unit firing-feedback/aim-response producers remain open. Pending prepared projectiles and
non-idle passive trigger states outside the admitted clear path remain guarded.

`research_weapon_unequip_tu15.py` records complete-body and branch correspondence
for eleven clear/detach/feedback owners, including `E9A90` to TU1.5 `EA9B0`.
The original direct TU1.5 graph/timing execution also agrees with TU1.1 on
**29,568 queries, 5,344 nonzero**, now including `08000025` for all33 Lockout
weapon records, both representations, single/dual modes, absent render-model
controls and all four timing sections. Native retained put-away metadata is
checked against these results even without a registered view. A missing
action remains distinct from a present clip with a missing event.

## Shared registered hand feedback

The native multiplayer bridge now retains original unit+1FA/+1FB timers once
per unit, instead of private clocks that could not influence the opposite
weapon. Original C6C50 timer decay runs once before weapon updates, respecting
unit-definition flag11. Activities1..4 set the actual primary/secondary timer
only after the exact weapon/local-view/player/current-unit admission. Native
fresh pickups and removed hands remain unregistered until the real view
consumer binds them; removing a configured view clears admission without
changing the unit clocks. The pure SMG fixture interface retains its explicit
caller-supplied registration boundary.

The weapon update samples D2940's opposite-hand blend before its prepared-shot
notification. Projectile postpass samples the current shared timers after both
weapon updates. Both the SMG and ordinary firing-error/deviation paths use
these distinct phases. Retained inactive weapons use their real unit clocks;
ownerless weapons retain the original literal-one blend. The old observable
SMG timer packet mirrors the shared owner in the native adapter.

`oracle_weapon_hand_feedback_tu15.py` directly executes untouched TU1.1 and
TU1.5 registration/action, timer-decay and D2940 instructions. Both images and
the native kernel agree on **1,296 action/admission cases, 16,384 float blend
cases, 1,024 timer cases and all41 activity mappings**. Timer bytes and float
bits, including zero/negative/nonfinite clock controls, are exact for this
host execution. FP notifications and unrelated action requests remain declared
unavailable services in this isolated kernel fixture. Six complete static
owner bodies and their branch/operand mapping are retained separately.

`oracle_weapon_registered_dual_cycles_1_1.py` also executes the full weapon
input/update/postpasses with both exact weapon datums in local view0. It runs
the real E7EF0 action dispatcher, preserving reload admission. The native
comparison matches **32 scenarios, 8,320 ticks and 1,378 effect requests** for
magnum, Needler, plasma rifle and SMG, with both actual player representations.
It compares both magazine records, eight ordinary barrel chronology/error
fields, all52 SMG barrel bytes, heat/spent/flags, the shared unit timers, shared
seed and ordered effect ABI. The existing absent-view suites also assert that
no unit hand feedback is produced. Both registered and absent-view cases pass
with the corrected creation/view admission.

This is native hand-timer/error ownership and bounded original execution, not
complete firing feedback or visuals. D2310 is the existing camouflage
disruption owner in `source_camouflage.cpp`, not an aim-response owner. Full
unit/own-model/animation arbitration, natural world scheduling, remote
Xbox prediction/wire/event ownership, camera/IK/HUD effects and matched Xbox
TU1.5 framebuffers still require validation. Original fixtures retain declared
raw storage, unavailable rendering/physics/effect owners and failed generic
allocation. The source object/physics backend keeps ownership of its raw clocks.

## Weapon HUD ammunition and battery inputs

`weapon_hud.hpp/cpp` now follows the original56-byte per-weapon HUD status,
all ten weapon selectors48..57, weapon condition flags and unsigned cutoff
bytes. It preserves caller padding and signed16-bit loaded/shared-reserve
stores. Battery percentage and remaining fraction consume actual age; heat
and overheat flags consume the real published shader state. Zero-capacity
battery weapons are not declared empty solely because both ammunition words
are zero. The actual plasma rifles retain authored zero-capacity magazine
records; the beam rifle has no magazine record.

Seven complete static owners are mapped between TU1.1 and TU1.5, including
1046D0/1055F0,104AA0/1059C0,104CF0/105C10 and the HUD projection/input/mask/
visibility callers. The unchanged original images and native kernel agree on
**4,032 cases across all63 distinct authored weapon paths**, comparing all56
status bytes, ten scalar inputs, six weapon-type masks and nine visibility
queries. Original inventory ownership, duplicate/full-datum slots, signed and
overflowing reserve controls, charge bytes, trigger states and zero/negative
clock controls remain explicit fixture inputs. Infinite-ammo, acquisition and
appearance predicates remain supplied services; no positive world-target or
marker owner is claimed. The full static and executed instruction inventories
are in `hud-correspondence/report.json` and `hud-original.json`.

The native finite104CF0 helper now always includes the queried weapon's own
signed loaded/reserve stores, then adds matching different full weapon datums
only when an owning unit exists. It agrees with **834 independently executed
original inventory cases**. An older gameplay fixture omitted its declared
unit owner; it now checks both ownerless and owned behavior explicitly. The
failed original fixture log is retained in `hud-cpu-tests-v1.log`.

The real HUD consumes the existing loaded-plus-shared-reserve snapshot for
both hands, preserving individual gameplay reserve stores and the existing
wire layout. `original_weapon_hud_gameplay` passes **1,728 production ticks**
with both authored bipeds: actual dual SMG pickup, three/four consumed rounds,
independent reload reserves117/116, shared available293 and equal HUD reserve
counters changing240 to233. Actual source number-widget glyphs change for
both hands. Six actual plasma/brute-plasma/beam pickup/fire cases additionally
verify age/heat/percentage/availability publication. It also passes using the
owned TU1.5 executable. The native framebuffer in `hud-smg-live-v1` visibly
shows233 on both counters after that reload; this is native inspection rather
than a matched Xbox framebuffer comparison.

Charge/input/trigger/tracking/target-marker publication, complete scope/zoom
and warning animation ownership, and original Xbox pixels remain open. The
numeric kernel handles supplied charge/lock state; the production snapshot
bridge currently supplies only fields owned by its existing published state.

## Visual owners identified next

`audit_weapon_muzzle_admission` reads actual normal firing references across
all Lockout weapon barrels, including shared records owned by another tag.
Its report is `analysis/weapons-tu15-goal-20261013/muzzle-admission.json`.
SMG, Needler and several other definitions admit the existing fixed FP particle
loader. Most human muzzle effects currently require additional owners:
definition flags8/16 reach original `17D040` object/lighting-color production;
other definitions require real non-sound parts, particle controllers, vertex
classifications or cross-tag metadata ownership. Charged plasma and vehicle
effects can legitimately have no FP systems or direct sound references.
Admission failures are not bypassed to manufacture a passing muzzle.

## Charge and lock trigger reference groundwork

`research_weapon_trigger_tu15.py` maps twenty complete original owner bodies
between TU1.1 and TU1.5, including the whole1441-instruction weapon update
1021D0/1030F0, input1045E0/105500, barrel admission and charge/lock transition
helpers. All2,472 instruction rows and internal branches match after relocation;
all retained absolute operand bytes also match. The original instruction
listings, mapped calls and separately unverified external callees are retained
in `trigger-correspondence-v2/report.json`. The earlier nineteen-owner report
is retained. This is static reference groundwork,
not new native charge/lock execution or full-world validation. Actual charge,
alternate-barrel, target and tracking owners remain required.

## Complete trigger phase and native ordinary integration

`weapon_trigger.hpp/cpp` now implements the original12-byte trigger record,
five shared analog input bytes, charge admission/start/complete/release, lock
transitions and the complete102833/relocated per-trigger phase. The original
caller supplies real magazine/barrel admission, acquisition/tracking, unit
zoom/action and effect/world lifetime services. Unsupported service requests
are rejected rather than assigned invented game results. Caller padding,
signed timer interpretation and exact full effect/target identities are kept.

The unchanged TU1.1 and TU1.5 transition owners agree on **34,816 cases over
96 authored trigger/barrel-count operand contracts**, including signed8-bit
input timer rollover and the real CRT64-bit conversion. The complete original
per-trigger phase additionally agrees on **13,312 cases** over the96 contracts,
all six behaviors and retained mode branches. Source caller locals and raw
post-magazine storage are explicit ABI controls; barrel and target predicates
are supplied inputs, and effect allocation/stop, zoom/actions and destruction
remain captured unavailable services. This establishes the per-trigger
consumer and requests, not the independently owned world predicates or their
composition. Both images' reached instructions and services are retained in
`trigger-original.json` and `trigger-phase-original.json`.

The native kernel compares all **48,128 cases**, including the complete12-byte
trigger, two52-byte barrel stores, five analog bytes, target/marker stores and
**24,792 ordered requests**. Its test requires the exact case totals. The
current loader retains each authored trigger's consumed operands before
checking projectile barrels: seven barrel-less shield/objective paths also
have authored triggers. Their gameplay owners remain separately required.

The existing native ordinary path now calls this phase for behavior0,1 and
same-barrel4 with its real primary input and recovered admission bridge. It
updates retained trigger flags/ticks/effect words and mirrors the source latch
for reload admission. SMG retains its separate recovered Spew path; distinct
active barrels and charge now have the bounded plasma-pistol adapter below;
other autofire and targeting still need their complete native adapters.
No false target-query result or successful world effect is installed
to admit them.

`original_weapon_registered_trigger_cycles` runs actual pickups/local views
and compares both complete trigger records alongside the existing magazine,
barrel, heat/spent, timers, seed and effect ABI. It passes **32 scenarios,
8,320 ticks and1,378 requests** for magnum, Needler, plasma rifle and SMG with
both bipeds. A separate run also passes using the TU1.5 image. The fixture's
full original constructor/input/update/action dispatcher/postpasses execute
in TU1.1; the twenty-owner static correspondence includes TU1.5's whole weapon
update and its trigger helpers. This does not replace complete TU1.5 world,
camera, projectile, visual or network validation.

## Two-barrel plasma pistol and charge publication

The native adapter imports both actual plasma-pistol barrels, projectile and
effect banks, and executes the recovered charging/release, heat/age, preparation
and deferred dispatch phases. It retains the literal byte175 accumulator rather
than inferring charge progress from elapsed time. Preparation selects fire_2 or
misfire_2 for barrel1, also correcting the carbine's first-person firing action.
The first-person binding now resolves all36 required activity symbols.

`charged-correspondence-v2/report.json` verifies eight complete original
constructor/update/preparation/postpass/birth/speed owners, **3,783 instruction
rows**, between TU1.1 and TU1.5. Three relocated pointer operands resolve to
identical literal12-byte vectors. Other callees retain separate ownership.
`charged-cycles-v1/report.json` executes the unchanged original constructor,
input, full update and two postpasses for both bipeds and local/nonlocal view
registration. It covers tap, early release, charge edge, full/repeated charge
and nearly exhausted battery: **24 scenarios,5,164 ticks,136 effect requests
and44 projectile requests**. The native actual-pickup test compares both complete
52-byte barrels,12-byte trigger, six analog/charge bytes, heat/age, flags,
hand timers, RNG and effect/projectile request identity/direction. A separate
136-entry fixture checks the live stores at every effect callback, including
charge-start and battery-exhaustion ordering. It passes using either update's
actual constants. These fixtures declare unavailable effect/projectile allocation
and absent own-model services; they do not execute a complete original world.

`projectile-speed-v1.json` directly executes both updates' complete speed scalar
and difficulty-table consumers for **1,920 cases**. The native owner follows
proj flag0x400, full player datum presence, the literal player multiplier1.5,
and the actual MATG difficulty table10, including missing-table controls and
signed difficulty clamping. The first-person graph query expansion compares
**38,016 cases,5,776 nonzero results**, with complete original TU1.1 and TU1.5
queries and the native resolver, including both bipeds, dual and absent models.

Both hands now publish their actual input flags, trigger mode/ticks and charge
byte through native snapshots into the original weapon HUD projection. Native
protocol71 in the shared checkout and private0x803D in the isolated build carry
the added words; these are not Xbox wire formats. The actual native UDP regression
passes with full-roster fragmentation, reordered/duplicated/dropped/late packets
and nonzero charge publication. The selected birth barrel also determines the
snapshot projectile definition, fixing an SDL-discovered charged bolt being
published as its primary counterpart.

The current SDL flow passes for Spartan and Elite:32 charged-pose draws, the
authored fire_2 poses, one secondary firing PCM submission, battery/heat stores
and a live charged projectile with its actual full datum/definition. The bolt
has **no render model**. Its four authored attachments are a light, an effect,
an MGS2 light volume and looping sound. The MGS2 owner is now admitted below;
the other three attachments still need their live owners.
`charged-visual-owners-v1.json` retains those actual tag/attachment bytes.
Both bolt MGS2 definitions use runtime flag1 and8/6-sprite rows. The original
geometric radius sampling now executes in the native light-volume consumer.
`projectile-light-volume-original-v3.json` executes complete unchanged retail,
TU1.1 and TU1.5 sprite/aspect/projection/normalization owners on112 controlled
actual-asset cases. All three images and the native kernel produce identical
**1,456 sprite records**, including phase-dependent radii, positions, aspect
orientation and packed colors. Renderer-state entry and GPU submission remain
unavailable boundaries. A discovered30BF0 precision error is corrected: norm
products/sums/sqrt stay at x87 PC53 precision until the original float store,
then use SSE reciprocal/vector multiplies. The original64-case title regression
also matches all2,048 sprite records, markers, pixel-program words and base glow
texels exactly in `title-light-volume-after-radius-v1.json`. This verifies the
numeric consumer; the current gameplay integration is described below.
The v1 diagnostic omitted aspect-camera roots; v2 retained111 radius-X word
differences. The corrected v3 supplies the literal relocated camera roots and
matches every native sprite word after the30BF0 correction. Earlier diagnostics
remain retained and are not visual-parity evidence.
Live charge player-response publication,
other positive generic effect handles, misfire visuals, guidance/contacts, charged
dual/cancel/holster/drop and full own-model/camera arbitration remain open.
The retained first runtime failure incorrectly required bolt mesh submission;
the second exposed the genuine snapshot definition bug. Later native passes
do not certify those unresolved attachments or Xbox framebuffer parity.

`projectile-light-attachment-original-v1.json` executes complete unchanged
41980/B8CA0/B8D30/BE6D0/42850 attachment collection on both actual no-model
plasma projectile tags in TU1.1 and TU1.5. **768 cases** include controlled
completed root transforms, view types0/1/3, retained slot bytes and the shared
1024-record queue boundary. All192 successful112-byte records match exactly;
64 full-queue and512 non-world-view controls retain the original behavior.
The native callback token is explicitly normalized to retail4F010; native
object identities remain a separate namespace from Xbox pool handles.

Live primary and charged bolt MGS2 attachments now retain their actual native
projectile owners, completed free root matrices, proved one/white opacity and
color inputs, actual per-view2FD90 camera banks and physical viewport clips.
They enter the source transparent sorter with particle-location commands from
the completed particle center/depth producers. One original light-volume
command retains its ordered volume rows. Primary row1 uses its actual separate
bitmap at original frame0; charged rows use the original default glow texture.
Both bind the non-fuzzy4FE20 program. Fuzzy textures and unresolved marker,
function, fade, color and frame owners retain explicit admission failures.

`live-glow-four-view-original-v1.json` independently executes original sprite,
aspect, normalization and quad projection against the actual gameplay GPU
inputs: **108 charged records and9,006 submitted vertices in four views** match
retail, TU1.1 and TU1.5 exactly. After the primary texture correction,
`live-glow-primary-four-view-original-v1.json` matches **24 primary records and
2,016 vertices**, including ordered bitmap ranges and original frame0, in all
three images. The observer retains raw completed inputs and submitted triangles;
the original renderer-state, texture residency and push-buffer services are
declared hardware boundaries. The native four-view primary framebuffer and
lossless preview are retained in `glow-runtime-primary-four-view-v2`.
The first primary probe correctly rejected its unsupported texture and then
stopped before observing five attack poses; that failure remains retained.

This closes bounded live MGS2 ownership and render-input submission for the
admitted free native projectiles. Full world geometry/effect producer ordering,
source culling, gameplay sampler/framebuffer comparison and the other bolt
attachments remain open. When no complete source geometry command bank exists,
the renderer preserves its earlier geometry phase and sorts only the admitted
effect commands together. It does not claim full original1E370 composition.
The retained primary frame shows native split-screen composition; projected
bounds and GPU recording alone do not prove visible Xbox pixels.

The final textured-glow build repeats the independent comparison in
`live-glow-{single-view,elite-view,two-view,four-view}-original-v2.json` with
the actual submitted vertices and per-row bitmap ranges. Each matches all
three original images. Native charged probes pass for both playable species
and one/two/four views; the primary four-view probe also passes. Source view
and projection construction retains its466-case original fixture.
`charge-effect-lifetime-correspondence-v1/report.json` identifies the next
three complete original owners over185 instruction rows: TU1.1
178CD0/178C10/179080 correspond to TU1.5 17A0F0/17A030/17A4A0. Their pool salt,
retained particle-location flag, beam cleanup, repeat/stop flags and retirement
calls must be composed with actual positive effect allocations; static
correspondence does not provide those live owners.

The next execution checkpoint establishes that the pistol's actual charging
reference is **a damage/player-response effect, not a retained particle glow**.
Its flags are6, it contains one1/30-second event and one `jpt!` part, and it has
no particle or sound systems. `charge-effect-local-v1` and
`charge-effect-nonlocal-v1` construct and equip a real source-owned pistol,
advance60 neutral source ticks, then invoke the original108A70 callback with
the trigger's actual effect and zero control words. Each original call allocates
e6670002, runs initial event advancement and retires it before creation returns
NONE. The local call additionally executes the actual157D70 player-response
owner. Those are standalone callback inputs, not a full charge-to-render run.

Unchanged TU1.5 instruction execution matches **377 local and335 nonlocal
retained writes**, with all58,970,198 retained bytes agreeing in each case.
Direct execution of the owned original text in an isolated32-bit host process
also verifies the full GP/XMM/x87/MXCSR exit after the correction below. Linux
user-mode IF remains a declared platform difference; this path never reads it.
No original instruction is patched and no game-service return is supplied.
This is host x86 evidence, not physical Xbox execution or whole-boot provenance.

The original hardware comparison exposed a real native x87 state defect:
32-bit FISTP conversion retained an earlier C1 bit from trigonometric/math
work. The native helper now publishes the conversion's actual C1 result;
the combat and live-support generated modules and shared generator use it.
The corrected exit status0120 matches original hardware. Unicorn retains
different precision/control/status bits in this call; its raw mismatch remains
recorded, rather than being treated as complete FP evidence. The two corrected
objects also pass **914 unchanged original combat/bitmap/sandbox/primary cases**.
These checks use read-only sandbox-v4 libraries and matching headers with only
the recorded conversion correction. Combined current-checkout and installed
runtime validation of this new source correction remains pending. The previous
weapon launcher checkpoint is unchanged.

`charge-effect-authored-scope-v2.json` freshly verifies all38 map hashes and
actual trigger bytes. It inspects **108 charging references** across the full
weapon scope:36 use the sentinel damage/player-response wrapper,10 Hunter,
57 Wraith and5 Scarab. Complete1030F0/108DB0 source correspondence separates
**52 references with a possible idle charge route from56 without one**.
The56 are behavior1 Wraith mortar references; their charging reference does
not establish an emitted charge visual. The single Wraith graph reference
with behavior3 belongs to the Old Mombasa Covenant gun-tower mortar. Hunter,
Scarab and that gun tower still need their actual trigger/input/owning actor
composition. Raw graph records retain map relocations;80 raw-byte groups do
not imply80 semantic contracts. Actual player-response camera/controller
presentation for the pistol and sentinel wrapper remains required.

`wraith-charge-lifetime-observed-v1.json` adds a bounded positive graph
consumer. Source variant descriptor78 choice2 selects the actual Wraith;
the original netgame row55 factory creates it and its attached mortar_mp
weapon in Coagulation. Its actual parent chain and cross-tag trigger bytes
are authenticated against the retained source bank and owned cache. A player
is explicitly placed three metres above the vehicle's initial root and60
neutral source ticks run before the diagnostic callback. This manual108A70
request is not gameplay charge production by the behavior1 mortar.

The local case returns live instance e6670002. Its **29,352 unchanged original
instructions and1,071 retained writes** match the native allocation. The
subsequent original17A0F0 stop executes194 instructions and matches all five
retained writes. It clears bit0 on all four actual particle_system records
(0109->0108), marks the effect flags81->C5 and retains the effect for later
particle cleanup. It does not immediately free the source effect. The separate
case without a local view executes2,512 instructions, matches295 retained
writes and allocates then frees the instance before returning NONE.

All three cases match58,970,198 retained bytes and the complete exit ABI when
the owned original text executes directly on host x86, except for the declared
Linux user-mode IF bit. No game-service result or source instruction is
replaced. Unicorn's raw floating-point mismatch remains recorded for the two
allocation cases; its stop exit also matches directly. Cosmetic source RNG
can change execution counts between fresh captures; each report retains the
actual input bank. The earlier pistol local/no-local-view hardware regressions
also pass with the stricter cache, trigger and full-salt owner validation.

These diagnostics use the same read-only sandbox-v4 libraries, matching
headers and two corrected conversion objects as the previous checkpoint.
They add no installed runtime change. Actual particle drain and eventual
source-effect free, linked-object location cleanup, complete charge trigger
composition, lens/sound publication and native/Xbox rendering remain open.

## Native source particle-frame cleanup

`source_effect_particle_frame_tu15.hpp` and its generated C++ module recover
**38 complete TU1.5 bodies and2,016 instruction rows**, including179DF0,
177830,1778D0 and both actual particle metadata interfaces. They compile to
native arithmetic and static calls; missing downstream owners keep their
original ABI and fail. `PhysicsLiveWorldTu15::advance_effect_particles` now
exposes this original particle phase with an explicit frame delta. It remains
separate from the object/effect tick, as13AA28 is in the original frame.
The current root particle module and source-world translation units compile;
combined current-checkout and installed-runtime validation remain pending.

`particle-frame-native-comparison-v1.json` verifies two native frame calls
against untouched original execution: **5,063 ordered instructions,191 retained
writes,58,970,198 retained bytes per call and complete GP/XMM/x87/MXCSR exits**
match direct host x86 execution, with only the declared Linux IF difference.
The native replay executable also checks these outputs directly. Two focused
CTest registrations consume the owned captures when their directory is set.
The same two comparisons also pass with Release optimization (`-O3 -DNDEBUG`)
and the source runtime's strict floating-point flags.
This is native CPU cleanup evidence, not renderer or physical Xbox proof.

The actual source-world API passes the same full retained-bank comparison in
`particle-world-api-comparison-v1.json`. A fresh coupled Wraith case allocates
and stops its actual graph, advances two source particle frames, then naturally
frees the effect on the next object/effect tick. The interleaved control in
`particle-world-interleaved-comparison-v1.json` runs object ticks61 and62 before
their particle frames, and frees the effect on tick63. Both interleaved API
frame banks independently match original host x86 output.

These captures contain **four particle systems, four emitter records and four
location records, with zero live particle instances**. The first stopped frame
frees emitters/locations; the second frees systems and unlinks the effect's
particle chain;179CD0 then frees the effect. This validates empty-system
cleanup. Emission and drain with live particles remain required, as do linked
object cleanup, actual charge trigger production and presentation.

The full13A9B0 frame probe reaches a missing upstream audio bank:52D684 is
NULL at original22C59F. Owned startup row44B56C names its actual22C440
constructor; row44B574 names22C480 reset. Those owners and the full frame's
other prerequisites must be recovered before renderer-frame integration.
No replacement buffer or callback return is supplied to bypass that boundary.
The tested coherent source-world override contains only the new API/dispatch
hunks over read-only v4 source. Weapon launchers remain at the prior glow
checkpoint; all63 complete feature/visual contracts remain open.

## Native particle visibility and a live instance

The next module revision contains **47 complete bodies and2,388 instruction
rows**. Actual22C440 constructs52D684 through the original arena/checksum;
22C480 resets its ten DWORDs. These match untouched host x86 execution over
the actual source bank. The full13A9B0 probe now passes that initialization
and reaches owned D3D code, then the unsupplied Xbox physical-allocation import
800000A6. No fake video-mode cache, kernel return or replacement buffer is used.

The previous empty particle graphs were culled because their cluster21 bit in
4D1EE0 was clear. `publish_render_visibility` now publishes completed native
portal cluster entries into the actual allocated collector and executes whole
3F3E0. `begin_model_frame` performs literal2B630..2B63A's once-per-frame reset;
successive views union their bits. A declared diagnostic camera at the actual
biped origin produces clusters21,45,44 through the existing retail-source
camera/portal implementation. Whole TU1.5 3F3E0 matches58 ordered instructions,
three writes, the full bank and hardware exit. This verifies the TU1.5 consumer;
complete TU1.5 camera/175F90 traversal production remains a separate gate.

The longer actual source-world capture produces **one live64-byte particle on
the twelfth object/effect tick**. Stopping that graph through17A0F0 retains the
particle initially. Two source particle API frames drain the particle and then
the four systems/emitters/locations; the next actual object/effect tick73 frees
the effect. Both frames independently match untouched original instructions,
ordered writes and58,970,198 retained bytes; complete native GP/XMM/x87/MXCSR
exits match original host x86 with only Linux IF masked. The two frame calls
execute7,854 instructions and298 retained writes in total.

This live particle is removed by the original stale-draw rule in251A60. The
actual1772C0 draw owner stamps location+30; no draw submission was supplied in
this diagnostic. Rendered emission, ordinary particle aging and a rendered stop
sequence remain open. The MP Wraith charging callback is still a manual
diagnostic on a behavior1 trigger, not a production Wraith charge feature.

`particle-visibility-complete-evidence-v1.json` verifies nine current Release
kernel cases, both source API frame banks, the allocation/stop comparisons and
live pool counts. Two current root translation units compile against a saved
include snapshot. A combined current runtime and installed delivery remain
pending; the weapon launchers still select the prior glow checkpoint. No weapon
is marked fully feature-and-visually complete by this evidence.

## Particle draw admission, live motion and natural drain (October14)

The source particle modules now retain53 complete frame bodies (2,661 original
instruction rows) and18 complete draw/storage bodies (1,528 rows). Whole1770C0
and1772C0 submit four actual transparent-command records through1E4C0, with
callback176B30 and four genuine location+30 draw stamps of2. The declared
640x480,70-degree native camera is at the actual biped origin and aims at the
actual first particle location. It remains an explicit diagnostic input.

Whole2C40D0/2C2AD0 now own particle motion, including the exercised gravity/drag
program, and251650 owns natural particle removal and emitter accounting.
The original FST/FSTP float-store rounding flag exposed a lost x87 C1 bit;
the common lowerer now retains the hardware conversion flag, including its
single/double stores and64-bit integer spill. The actual natural-death case
matches the complete host x86 exit, including x87 status0222, with only Linux
IF masked. No expected FP status is supplied or masked to make it pass.

The fresh drawn-stop diagnostic creates44 live particles, invokes the actual
17A0F0 stop request at source tick76, and drains naturally over35 particle API
updates. The next original object/effect tick77 frees the effect. Every one of
those35 updates matches untouched TU1.5 code, original host x86 execution,
strict Release native instruction paths and ordered writes, full native CPU
exit state except Linux IF, and59,166,806 retained bytes. Together they execute
146,372 original instructions and6,011 retained writes. The source-world API
banks also match every hardware result. These updates retain one genuine
view's draw stamps; this is one draw submission followed by particle updates,
not35 rendered frames.

An original175F90 portal-producer comparison exposed missing CRT380FA0,
referenced by actual row46D990. Its six original stores initialize the empty
rectangle/bounds bank consumed by portal and object owners. The current source
world now calls that complete constructor. After actual219080/2190C0 mask
initialization and380FA0, whole175F90 executes30,910 original instructions and
produces exactly the native camera group, four cluster entries, sort indices
and four complete264-byte frustums. This is one declared camera/BSP comparison;
the original mask/bounds calls in that oracle are late diagnostic setup, not
proof of complete startup/camera/door chronology. Earlier uninitialized probes
remain retained.

`particle-draw-motion-complete-evidence-v1.json` verifies57 strict native owner
comparisons, all35 drawn-stop API frames, three additional active-motion API
frames, original draw submission and the initialized portal result. All three
current root translation units compile against a retained include snapshot.
The earlier Linux helper mapping failure is retained; its executable/host heap
now resides away from the actual08100000 source heap, with strict overlap
rejection and unchanged original game addresses.

The actual queued176B30 callback reaches original Xbox D3D code and faults at
404BCD reading address8 because the required device state is absent. No device
success return or game callback is supplied. Queue consumption, shader/vertex
production, GPU integration, repeated rendering, physical Xbox captures and
combined installed delivery remain open. The MP Wraith trigger is behavior1,
so its charging callback remains manually invoked diagnostic coverage. None
of the63 weapon paths is certified fully feature-and-visually complete here.

## Authored Hunter charge transition (October14)

The Old Mombasa `e2_cov_hunters0` start selects actual Hunter bipedFCC41A7C
and cannonFAD418A0. The original object factory creates that cannon as a child;
no replacement weapon body or actor is supplied. A real source player is born
at a declared nearby position, followed by60 neutral source ticks. Campaign
type1/mode1 and the options payload remain explicitly declared diagnostic
inputs, not proof of complete campaign startup.

The explicitly invoked whole108DB0 helper selects authored effectFADC18A8,
retains full instanceE6660001, and publishes trigger state1 with30 ticks. It
creates three actual particle systems, emitters and locations, with no live
64-byte particles yet. In the accepted capture, native execution matches all
471,154 original instruction addresses,918 ordered retained writes, complete
host x86 CPU exit except Linux IF, and58,970,198 retained bytes. The original
also executes the authored effect-part and sound-admission paths; audible
output remains unverified.

The older v4 diagnostic needed the actual219080/2190C0 portal-mask setup and
whole1D5DB0 unit-physics continuation, which current root already owns. Its
small private supplement preserves that older ABI. The trace comparison also
found the pool module reporting canonical TU1.1 PCs during TU1.5 execution.
The pool generator and generated source now use the authenticated selected
image map for every callback, including repeated instructions and startup
sites. The current pool source regenerates identically and compiles against a
retained root include snapshot. Failed intermediate probes remain preserved.

`hunter-charge-complete-evidence-v1.json` verifies the positive source graph
and complete transition comparison. The earlier newborn/no-player probe
returns NONE after immediate retirement in both original and native code;
its evidence remains separate. The accepted positive case still invokes the
helper manually and creates no AI actor. Actual AI firing input, complete
trigger scheduling, live Hunter particle emission/drain, rendering and Xbox
visual/audio comparison remain open. No weapon is certified complete here.

## Hunter neutral cleanup and cold actor ownership (October14)

The actual1030F0 weapon update, invoked over the retained neutral control
state, clears the explicit Hunter charge and calls the real effect stop owner.
Two179DF0 particle API updates drain its three empty system/emitter/location
graphs; the next object/effect tick61 releases the full effect datum. The
weapon update and both particle frames match untouched TU1.5 code, host x86
state and all58,970,198 retained bytes. This graph contains zero live particles;
it does not establish a held-fire or rendered Hunter sequence. See
`hunter-neutral-cleanup-evidence-v1.json` and its checkpoint.

The private v4 diagnostic now executes the whole1CFA40 AI storage initializer
and campaign1CFC90 new-map initializer. It creates a cold actor from the actual
Hunter characterFCC31A7B through1E7E40, then binds the existing authored Hunter
through1E9000. These are original owners, with no supplied actor record or
game-service return. The cold constructor matches1,105 instruction rows and815
retained writes; binding matches216 rows and7 writes. Both match complete CPU
exit except Linux IF and all58,970,198 retained bytes against host execution of
the unchanged Xbox executable. The constructor's336-instruction static
correspondence with base retail is retained separately.

`hunter-actor-transitions-evidence-v2.json` also verifies whole1E8410 birth
with explicitly declared squad NONE:2,851 instruction rows,887 retained writes,
complete hardware CPU exit and58,970,198 retained bytes. The original owner
admits the actor, binds the unit, assigns team3, adds the actor to the global
list and resolves navigation location19. This does not certify authored squad
birth or aggressive behavior. The emulator omits precision status on this
continuation; native execution matches the complete hardware x87/MXCSR state.

The production source-world dispatcher now registers28 whole AI owners
(2,353 instructions) for storage, new-map reset, cold actors, binding and birth
state. The corresponding private module capture repeats the actual Hunter
birth against its own untouched original input and hardware bank; see
`hunter-ai-module-birth-evidence-v1.json`. The current root module and world
dispatcher compile against one retained root header snapshot with no drift.
This registration does not change the world startup prefix or deliver a new
runtime, and the private v4 adapter remains explicit.

The next original AI frame initially faults reading the absent team bank.
Adding the actual1E7710 initializer and1E7760 new-map reset from startup row18
allows whole1D0670 to return through39,880 original instructions and361 retained
writes, without supplied services. Its unchanged code also matches host x86
retained memory/GP state. Native AI-frame lowering and complete hardware FP
comparison are ongoing; this original-only probe is not a native frame proof.

AI storage/new-map are source-executed prerequisites, not independently
certified boot sequences. Cold creation and binding do not certify the full
actor birth/admission, squad membership, perception, AI decisions or production
fire controls. Those owners and the complete visual/audio sequence remain
open, and the delivered runtime is unchanged. All63 weapon paths still require
their full completion gates.

## Authored squad and native AI input frames (October14)

The Hunter also completes whole1E8410 birth through actual scenario squad31,
`e2_cov_hunters0`, rather than the declared no-squad case. The source squad
pool contains all132 authored rows. Original20C030 links the full actor to
that squad, preserves the parent group and assigns its actual team. This birth
matches2,930 instructions,887 retained writes, complete hardware CPU exit
except Linux IF and58,970,198 retained bytes.

The source AI-frame module now contains129 complete routines and17,308
instructions, while registering only whole1D0670 with the world dispatcher.
Both a global/no-squad Hunter and the authored-squad Hunter execute actual
decisions and1EC640/C92C0 NPC unit-input production. Current-root standalone
Release replay, using current memory/snapshot owners and no frozen game
libraries or supplied game-service returns, matches both original paths:
39,880 instructions/361 writes and40,177 instructions/387 writes. Both match
full hardware CPU exit and all58,970,198 retained bytes. See
`hunter-ai-frame-complete-evidence-v3.json`. This is neutral input evidence;
hostile targeting, held fire and an actual charge shot remain unverified.

The world tick now calls this actual AI owner at the13AB0D position after
player input and before PVS/object physics. The original AI-enabled byte owns
admission. The new-player continuation also calls whole1D03A0 with its actual
full player datum, as14E798 does, to register campaign AI tracking. That
registration matches41 original instructions,11 retained writes, complete
hardware CPU exit and retained memory. The33-body actor/storage module also
repeats authored Hunter birth exactly with the registered player. See
`hunter-player-registration-evidence-v1.json`.

Current actor/world components compile against a retained root header snapshot
without drift. A separate fresh full runtime build uses the earlier frozen
28/124-body checkpoint; it does not include the later33/129-body updates or
player notification yet. The installed runtime remains unchanged. Full AI
storage/new-map boot composition, subsequent AI ticks, charge/projectile/effect
and native rendering/audio composition, and all63 weapon completion gates
remain open.

## Continued Hunter perception and tracking (October14)

The source-world diagnostic now captures actual1D0670 calls at their in-tick
position, then continues object physics and179DF0 particle updates. After its
explicit60-tick object-only warmup and authored actor birth, ten consecutive
AI/object/particle ticks complete. The cannon still has neutral analog input
and no charge; this is not a firing proof.

The native frame module has170 complete bodies/21,711 instructions. Perception
tick63 creates actual props/references, followed by tracking of the real player
at tick64 and eligibility updates at tick66. The original and native frames
match the ordered writes, full host CPU exit except IF and58,970,198 retained
bytes. Perception matches22,034 instructions/451 writes; tracking matches
23,336/296 and25,028/309. Original tick70 also completes its periodic squad and
group work; its native continuation is being recovered. See
`hunter-ai-frame-perception-evidence-v1.json` and
`hunter-ai-frame-tracking-evidence-v2.json` for bounded comparisons.

The diagnostic's old v4 object loop needed whole1164D0/116440 animation
continuations already owned in current root. They remain private supplements,
not claimed as new production code. Captured failed frames remain available
as authenticated original-only discovery inputs; only completed exact replays
are accepted as native proofs.

A coherent33/129-body full Release runtime build and eight selected regressions
pass, including actor construction/binding/control, the original AI frame,
trigger contracts, primary fire/reload and scheduled effects. This frozen
runtime predates the later perception/tracking module. Source/header changes
from independent root work are preserved; incompatible old/new memory ABIs
are not mixed. Installed launchers/runtime remain at the earlier delivered
checkpoint. Complete firing, visuals/audio and all63 completion rows stay open.

## Ninety exact AI consumer ticks and forward fixture (October14)

`hunter-ai-ticks-native-v11` completes90 actual AI/object/particle ticks after
the declared warmup and authored Hunter birth. Every one of its90 AI calls,
at source ticks60..149, matches untouched TU1.5 execution, independent host x86
CPU exit except IF, ordered writes and all58,970,198 retained non-stack bytes.
The same original inputs also pass current-root standalone native replay.
`hunter-90-ai-ticks-original-complete-v1/report.json` verifies all90 calls;
it does not independently replay the interleaved whole object/physics ticks.
The cannon stays neutral in this overhead-player fixture, so no charge/shot
or rendered weapon completion is claimed.

Current root has196 whole AI frame bodies/24,527 instructions, including the
periodic clump owner reached at tick90. The actual stored player is roughly
3.1 metres directly above the Hunter. A declared20-metre forward test point
was rejected by the original BSP membership data and preserved as a failed
fixture. A separate retained geometry probe establishes a valid10-metre point
along the authored Hunter facing in cluster7; the new forward fixture checks
membership again before creating its actual player body. No AI target,
weapon flag or game-service result is supplied.

That forward fixture reaches additional sight/visibility queries. Its original
tick64 completes49,310 instructions/401 retained writes and matches host x86
memory/GP state. Its full hardware FP exit is retained for native comparison;
new sight owners are being lowered. Hostile charge/fire, final renderer/audio
integration and every weapon's complete gates remain open.

## Outstanding goal work

All completion requirements above remain open unless the bounded evidence
explicitly covers them. Major known gaps include heat/battery/charge and
venting, rocket tracking, Needler guidance/stick/supercombine, sword lunge and
power/looping sounds, complete reload/chamber/interrupt owners, full source
Unit/inventory/pose/action/physics composition, nonlocal and coalesced effect
histories, and matched rendered sequences across all weapons and owners.
No weapon is marked fully feature-and-visually complete by this checkpoint.

The [complete sandbox goal](SANDBOX_TU15_PARITY_GOAL.md) now adds one passing
source-owned distant sword lunge: actual targeting, closing movement, posed
contact/damage and action completion across 180 ticks in a coherent native
build. Its October14 source-primary checkpoint also passes ordinary SMG fire
and manual reload:12 source trigger frames consume rounds60->53, then51
original reload-state frames restore60 rounds and spend seven reserve rounds.
The parallel native weapon update is bypassed and presented ammo is checked
against source storage every tick. Nine focused checks and the lunge/frag
regressions pass in its separate coherent v2 build. Complete weapon events,
first-person publication, dual/semi-automatic controls and original-world
coverage for every weapon remain required; this does not certify SMG parity.

Next coverage must include actual charge player-response publication, positive
Hunter/Scarab/gun-tower charge trigger and effect composition, full source frame
initialization and live-particle emission/drain after the original stop request, bolt attachments,
guidance/contact and charged dual/cancel/holster/drop composition, followed by
actual acquisition/tracking and its HUD publication, then complete
unit/own-model/camera arbitration, extending the bounded
inactive/world clock and numeric HUD evidence above. Initial energy-weapon carry remains guarded in the
native match; the current energy suites use actual pickups after an SMG start.

The current full native build succeeds in `build-weapons-tu15-20261013`, with a coherent
source snapshot at `.local/weapons-tu15-source-20261013`. Original resources
remain local symlinks. Shared dirty edits are preserved. Live SDL/Vulkan
verification passes all **thirteen current cases with zero Vulkan validation
errors** in `glow-runtime-suite-v2`, including charged plasma pistol, shotgun firing and Needler
empty/reload after the ordering corrections. The probe uses a finite one-tick-per-frame
clock, explicit mid-tick presentation fraction, private X11 display and confirmed
SDL X11 driver. The first Needler shot is released to observe its full attack
clip before held fire empties the magazine and reloads. Held shots restart that
clip every four source ticks. Earlier shared-display and incorrectly headless
probes remain retained, including their failures. These are native regression
observations with diagnostic placement and dummy audio, not Xbox framebuffer
or audible-hardware parity. The weapon-specific launchers deliver this tested
progress build; combined current-checkout validation remains pending.

Two additional Elite carbine/charged-pistol flows pass in `glow-runtime-elite-v2`.
The separate finite SMG SDL probe in `glow-smg-flow-v1` also passes with
zero Vulkan validation errors: actual pickup/switch, independent triggers,
dual reload, 308 posed draws across four clips, seven authored firing PCM
submissions for seven consumed rounds, and grenade/offhand release after the
reported reload action clears. The prior HUD checkpoint's retained dual-state framebuffer and lossless PNG preview
are in `hud-smg-live-v1`; both reserve counters show233. This is a native visual inspection;
matching Xbox camera/lighting/material/HUD evidence is still required.

The sixty-three current focused CTest checks are retained in
`glow-cpu-tests-v2.log`; the actual
UDP presentation regression also passes with TU1.5 data, including independent
clocks and reordered/duplicated/dropped/late packets. The earlier shared-reserve
AddressSanitizer/UndefinedBehaviorSanitizer run passed its recorded source.
The current isolated runtime and regression build log is `glow-delivery-build-v1.log`;
`smg-dual-complete-effects-build.log` records the complete-effect test update.
`Halo2-Weapons-Launcher` and
`Halo2-Weapons-Title-Launcher` now select `build-weapons-tu15-20261013`, verified
with both `--launcher-info` outputs. Their previous binaries are preserved
under `launcher-delivery`; source paths, launcher/runtime hashes and selection
proof are recorded in its report. A fresh launcher-to-native eight-frame smoke
passes with zero Vulkan validation errors in `launcher-delivery/glow-xbox-smoke-v1`.
This is delivery of the bounded progress
build, with full feature/visual parity still incomplete. The frozen source manifest and isolation hashes are refreshed
at each completed checkpoint.

The frozen source retains its tested Cache vector backend and damage API
outside this task's changes, while concurrent shared-checkout work is
preserved. `analysis/weapons-tu15-goal-20261013/source-isolation.json` records
that boundary. Its native wire protocol is private `0x803D`, to distinguish the
isolated payload from concurrent protocol work. Only weapon-specific launchers
were updated by this goal. Their dedicated CMake targets now default to the
original Xbox gameplay HUD and fonts; both `--launcher-info` outputs verify
that choice. Menu/pause assets keep their existing independent selection.
The earlier `hud-smoke` still selected imported gameplay artwork and is
retained as a superseded delivery observation; `glow-xbox-smoke-v1` verifies the
current corrected launchers/runtime. `hud-launchers-build.log` records their build.

## Hunter target admission and tracking progress (October15)

The ten-metre forward fixture completes90 AI/object/particle ticks. All90
captured AI calls independently match unchanged TU1.5 instructions, host
hardware CPU exit, ordered stores and58,970,198 retained non-stack bytes per
call (`hunter-forward-90-ai-ticks-original-v1/report.json`):3,233,512 instructions
and32,658 writes. Weapon input stays neutral, as it does in the original
consumer over these captured inputs. This does not verify the interleaved
object ticks, original boot or rendered weapons.

A separately declared five-metre position, checked against actual BSP membership
before birth, reaches original target admission at frame19/tick78. Original
code allocates one tracking record and admits its prop reference. The native
module now retains277 whole original bodies/32,289 instructions. Three
independent original/hardware comparisons pass, including this admission, the
previous forward visibility path and the original neutral baseline
(`hunter-ai-close-admission-evidence-v1.json`). Existing shared helper exports
are preserved. The next actual frame reaches267410; its recovery remains in
progress. No firing or visual-parity gate is complete from this evidence.

`hunter-ai-archives-v2.json` verifies the retained source/evidence archives.
`checkpoint-hunter-close-admission.json` records this checkpoint. Launchers
continue delivering the previous tested weapon build. All63 weapon rows and
630 completion gates remain in scope; full weapon parity remains incomplete.

## Original AI behavior-mask startup correction (October15)

The authentic `_cinit` owner2DBBAD walks the C++ initializer table in ascending
order. Its71 callbacks at table46DAD8..46DBF0 populate AI handler behavior
masks. All71 destinations remained zero in the diagnostic source world,
excluding combat behavior even after the Hunter admitted the player.

The root actor module now retains104 complete bodies/2,771 instructions,
including these71 original three-instruction callbacks. Root world startup
executes their actual published entries before AI storage construction.
`ai-behavior-mask-constructors-complete-v1/report.json` verifies all71 against
unchanged original code, independent host hardware and Release native code:
213 instructions,71 writes, exact CPU exit except IF and58,970,198 retained
non-stack bytes per callback. Authored source masks and every prior zero
destination are retained. The post-initialization actor birth remains exact
(2,930 instructions/887 retained writes) in
`hunter-behavior-initialized-birth-evidence-v1.json`. The actual root actor
and world components compile together in `ai-actor-root-components-v4`.
The first snapshot's missing included `.def` file is retained as a failed check.

The five-metre fixture before this correction completes90 exact AI calls in
`hunter-close-90-ai-ticks-original-v1/report.json` (3,206,763 instructions and
40,363 writes), but its weapon input stays neutral. Root frame code now has
285 complete bodies/32,716 instructions. With the corrected startup, the
fixture reaches additional behavior callbacks and the actual weapon input
consumer. Their recovery and independent replay are ongoing. No actual NPC
firing, renderer sequence, full CRT, installed-runtime or complete weapon
parity claim follows. `checkpoint-ai-behavior-masks.json` pins this boundary.

A static inventory of all417 published C++ initializer callbacks is retained in
`crt-constructor-table-audit-v1.json`. Seven entries lie outside the two
initially inspected executable sections; they remain unresolved. This
inventory identifies further startup evidence work and does not prove full
CRT execution. All63 weapon rows and630 completion gates remain in scope.

## Genuine Hunter charge and C1 correction (October15)

With the original behavior masks initialized, the five-metre Hunter fixture
begins an actual AI-controlled cannon charge at tick147. The trigger counter
advances from30 to1 and enters phase2 at tick177. The native object bank
contains its first `particle_cannon_component` projectile at tick178, datum
E2720003; subsequent source ticks create additional components. These are
native observations. Whole weapon discharge, projectile/impact comparison
against original code and the rendered sequence remain unverified.

Strict comparison initially rejected53 of90 AI calls for a lingering x87 C1
bit despite matching paths and retained writes. Controlled hardware probes
(`probe-x87-register-c1-v4.jsonl`) identify the reset at successful loads,
register stores, FXCH, FABS and FCHS. FCOMIP/FUCOMIP preserve the controlled
C1 bit for these finite inputs. The comparison-based first hypothesis was
rejected and reverted; its artifacts remain in
`rejected-x87-c1-candidates-v1.json`. Memory spills retain their actual
rounding C1 result.

The corrected native API and Release replay now match all90 original/hardware
AI calls, including complete CPU exits except IF and58,970,198 retained
non-stack bytes each:3,505,728 instructions and41,354 writes
(`hunter-ai-charge-c1-fixed-90-v1/report.json`). This verifies the first90
calls of a declared180-tick fixture; later discharge work remains open.
The376-body/40,832-instruction candidate retains the complete original
functions and exposes the aiming/definition helpers reached by the weapon
consumer. `checkpoint-hunter-ai-charge-c1.json` records the current boundary.
The installed runtime and the63-weapon completion scope remain unchanged.

## Exact Hunter discharge consumers and coherent runtime build (October15)

The fixture completes180 actual AI/object/particle ticks through source tick240.
Every AI call independently matches original code and hardware CPU exit,
trace, ordered writes and58,970,198 retained non-stack bytes:7,332,857
instructions and85,351 writes (`hunter-ai-discharge-180-original-v1`).
The root now contains105 actor/definition bodies (2,797 instructions) and
378 AI frame bodies (40,924 instructions), including the actual aiming and
definition owners reached by cannon discharge.

Seven complete1030F0 weapon updates at source ticks145,146,176,177,178,196
and210 also match unchanged original code, hardware and their actual native
API outputs:54,753 instructions and1,090 retained writes
(`hunter-weapon-update-original-v1/report.json`). These include charge
creation, the next trigger phase and component projectile creation. Their
actual indirect caller is10DD5B, with stack600EFF9C and an empty x87 stack.
The replay retains the captured caller status20, control27F and MXCSR1FA0;
only its return word is redirected to the diagnostic stop thunk.

The NPC discharge aim path also reaches11E660, which reads its actual BSP
cluster background-sound flag. The captured location selects cluster19,
palette5 and the original `old_mombasa_quiet` looping-sound definition. Its
whole26-instruction query passes original/hardware/native comparison. All71
behavior constructors and the corrected actor birth regressions pass with the
new actor module. No material or fog behavior is inferred from this query.

A fresh coherent13,340-file source snapshot is retained at
`.local/weapons-tu15-ai-source-20261015-v3`. Its combined Release build and
focused regressions are in progress in `build-weapons-tu15-ai-20261015-v3`.
The initial stale test-target selection is retained; the retry uses targets
verified from the configured Ninja graph. The installed runtime is unchanged.
Whole object physics, impact, audible playback, particle/render sequence and
physical Xbox framebuffer parity remain open. `checkpoint-hunter-discharge.json`
records this boundary. All63 weapon rows and630 gates remain in scope.

## Public campaign placement and scripted Hunter consumers (October15)

The coherent private runtimev3 built successfully and passed all eight focused AI/weapon regressions. Cairo public counted, authored and starting-location placement each retained actual actor/command/HS-thread bindings through checkpoint restoration and three ordinary AI/object ticks. Old Mombasa public counted placement now creates the authored Hunter and its cannon without the earlier manual lower-level actor-birth call.

Its first actual scripted AI consumer matches unchanged TU1.5 code, independent hardware and Release native execution:66,734 instructions,3,457 ordered writes and58,978,390 retained non-stack bytes, with full hardware CPU exit except Linux IF. The self-contained frame module includes the CRT classifier and its genuine indirect tail destination. Hardware checks covered16 FXAM encodings and16,384 count-one ROL value/flag cases; the empty physical-register FXAM path remains explicitly unavailable. All180 prior genuine Hunter discharge AI consumers still pass with this expanded module.

Root now contains460 complete AI-frame bodies/48,929 instructions and108 actor/shared-unit bodies/2,902 instructions. The actual object callerC7ED1 reached1164D0; its retained unit-extension query passed nine explicit component inputs. AdjacentC7EE1 reached116440/1E14F0;14 original/hardware/native comparisons covered its real0.5f input, NONE, duration boundaries, clamping, direction changes and retained timer states. The actor module also passed all81 prior constructor, background-sound and unit-query consumers. The public Hunter sequence completes one whole AI/object/particle tick and currently reaches missing AI callback1BB9A0 on its second tick. Full original B9180 discovery stops at the genuine NtWaitForSingleObjectEx kernel boundary, ordinal234; no successful wait was invented.

Verified lossless archives retain13,357 source files and47,420 evidence files, including rejected candidates. After independently hashing task-owned redundant banks against their archived manifest,250,696,262,478 bytes were freed; an archived168-byte caller bank was restored and its SHA-256 checked. The restore helper and exact historical validator sources are retained.

The updated matrix still contains all63 weapon paths,1,110 map occurrences and630 gates. Hunter owning rules, trigger resources, projectile creation and effect creation have bounded partial evidence; zero complete weapons are certified. Impact, full authored campaign behavior, audible playback, rendered sequences and physical Xbox comparison remain open. Current launchers independently select build-multiplayer/halo2_native; this goal has not redirected them to the private tested build. See analysis/weapons-tu15-goal-20261013/checkpoint-public-hunter.json and weapon-completion-matrix-v2.json.

## Original hardware SSE estimate boundary (October15)

Root now contains503 whole AI-frame bodies/51,336 instructions after187 strict regressions:180 prior discharge inputs and seven actual public-campaign inputs. The coherent private campaign sequence advances105 complete AI/object/particle ticks. Both it and the separately labeled lower-birth firing diagnostic next reach original116400 atC7EC8, which is being verified without substituting its return.

Actual campaign AI input105 exposed a Unicorn/host-SSE boundary. The original program was independently single-stepped on the host CPU:70,188 instruction events,2,793 retained store events, full CPU exit and58,978,390 non-stack bytes reproduce the ordinary unchanged-original hardware run. Instruction paths and store layouts match Unicorn, while numerical pose values differ. All53 observed RCPSS/RSQRTPS estimates differ from Unicorn; live host SSE primitives match every observed hardware estimate. Original Unicorn outputs remain retained and are not relabeled as hardware results. The native599-body candidate passes the observed hardware trace, ordered values, full exit except IF and retained bank using actual live CPU operators. The Xbox Pentium III estimate lookup remains unverified; this is host-CPU evidence.

The diagnostic also found that Capstone marks memory-destination MOVLPS/MOVHPS operands as reads. The test-only original-hardware observer now recognizes those actual store forms; seven sites/201 events were missing from its earlier store log. Earlier full CPU/bank observations remain valid. No original instruction bytes, game-function returns or game-service results were patched or substituted. The updated source and evidence archives verified3,125 source files and8,967 evidence files. Full all63-weapon feature, audio and visual parity remains incomplete. See checkpoint-public-hunter-estimates.json and hardware-store-observer-coverage-fix-v1.json.

## Public Hunter sequence and native particle captures (October15)

The public counted-placement fixture now completes180 actual AI/object/particle ticks. All180 AI calls match unchanged original code, independent host hardware and actual native CPU, ordered stores and retained memory:6,559,600 instructions and111,037 writes. Its source-API instruction trace is unavailable; a separate strict standalone replay verifies each original instruction path. Only input105 needs the independently observed host-hardware estimate reference. No Xbox Pentium III estimate lookup has been certified. This fixture submits14 sound requests and produces no firing events in its declared setup.

Root now contains631 whole AI-frame bodies/65,448 instructions after360 strict consumers:180 prior firing-diagnostic calls and180 public-campaign calls. The actor/shared-unit module contains156 bodies/9,263 instructions. Its111 strict consumers and actual nested114232→11DDF0 unit action pass; the latter compares37,407 original instructions,320 writes, full hardware CPU except IF and58,978,390 retained bytes. One attempted added standalone regression incorrectly selected1E8410, which belongs to live-support; the expected entry rejection and corrected module-owned regression list remain retained.

The separately labeled lower-birth firing diagnostic completes180 ticks and observes20 component projectile births and20 Hunter beam-hit/contact-effect events. Actual101DB8→178E30 hit-effect creation matches5,278 instructions,456 ordered writes, full hardware CPU except IF and its retained bank. These events do not establish full flight, contact selection or damage. With an explicit native camera and original observer/visibility consumers, the firing diagnostic produces2,436 particle draws across68 frames, with104 peak draws. Three authored charge/beam/hit effect resources are admitted; the declared view submits charge and beam particles. Four actual179DF0 particle calls at ticks140,171,190 and201 match original code/hardware/native trace, stores, full CPU except IF and59,174,998 retained non-stack bytes each:54,444 instructions and2,326 writes. The additional transparent storage explains the larger bank; an earlier verifier's hardcoded AI-bank size was corrected without weakening whole-bank comparison.

Nine native Vulkan capture triplets retain an authored static Old Mombasa BSP, its real water/view state and actual particle submissions. Eight samples change pixels when particles are submitted; all nine repeated particle captures match exactly, and the requested validation layer reports no errors. The layer is retained locally; no system package was changed. These frames omit the Hunter model, the complete native campaign render scheduler, audible playback and an OG Xbox framebuffer reference. They do not certify full visual parity.

A fresh13,515-file coherent root snapshot is building as runtimev4. The interrupted initial build log is retained; a supervised retry uses the same immutable snapshot. This goal has not changed the launchers. Their latest readback independently selects build-weapon-zoom-hud-20261014/halo2_native. All63 weapon paths,1,110 occurrences and630 gates remain in scope; zero complete weapons are certified. See weapon-completion-matrix-v3.json and checkpoint-public-hunter-rendering.json.

The coherent runtimev4 now builds and passes all eight focused AI/weapon checks. Its Cairo counted, authored and starting-location placement regressions each pass through three ticks/checkpoint restoration. A fresh180-tick public Hunter capture against these current coherent libraries also passes every unchanged-original/hardware/API/strict-native comparison. The original frozen snapshot contained one extra closing parenthesis in native_grenade.cpp; the shared root already held its correction. A declared CMake source overlay compiles that exact one-character fix while preserving the archived frozen source and every ABI header. This goal did not edit the shared grenade file or redirect either launcher.

A separate native Hunter model diagnostic completes180 ticks with28 effective rendered bones. Its real admission/collection path produces360 ordinary base draw units, selected LOD3, original lighting class2, and16/19 palette rows for the two selected regions. The authored cannon weapon's object-to-model reference is NONE; its visible geometry belongs to the Hunter body. Whole original model-builder and body GPU/reference comparisons remain open. The previously verified GPU samples contain particles and static BSP geometry only.

The second public retention pass verified15,657 source members and26,730 evidence members, including the complete immutable runtimev4 source and rejected discovery. After independently rehashing the archive and redundant native output banks,28,660,828,526 bytes were freed. Original input banks and hardware goldens remain in place; one native output bank was restored and its SHA-256 checked. Fresh runtimev4 evidence created afterward is tracked separately. See checkpoint-coherent-runtime-v4-verified.json and hunter-model-builder-evidence-v1.json.

## Storage cleanup at the user's request (October15)

Completed test runs no longer retain expanded full memory dumps, rebuildable fixture objects/executables, duplicate source staging trees, or disposable original-reference VM disks. The original reference session was stopped before its private disks were removed. Root source changes and the coherent builds outside the analysis folder remain intact.

Superseded full scratch archives are retired after their selected original captures are consolidated and every retained member is read back and checked against its previous SHA-256 receipt. The latest runtimev4 evidence archive, coherent v3/v4 source archives, reports, fixture sources, and current body/particle GPU captures remain. Historical manifests describe past runs; omitted full snapshots are deliberately discarded and must be recaptured if needed. Use analysis/weapons-tu15-goal-20261013/restore-retained-weapon-evidence-20261015.py for retained members. The older restore entry points forward to that helper after retirement.

The cleanup receipts are cleanup-retired-tests-20261015.json and cleanup-retired-archives-20261015.json in the goal analysis folder. Future runs should remove per-frame full banks after validation, retain selected inputs and proof receipts, and avoid accumulating complete historical scratch archives or duplicate source staging trees. Storage cleanup does not change any weapon completion gate.

## All scoped map combat postprocessors (October15)

The unchanged original TU1.5 routine165B30 and native combat-tag postprocessor now agree on ordered writes and complete cache output bytes for all38 scoped maps, covering all63 weapon paths and1,110 authored occurrences. The15 campaign maps correctly produce no multiplayer balance writes. All23 multiplayer maps reproduce41 writes each,943 in total, and pass actual native Cache weapon/damage imports and UDP rejection/admission for mismatched/matching data identities. The fixture supplies raw cache/global ABI inputs; preceding map activation is outside this comparison.

Two fixture corrections remain explicit. Campaign maps use a separate gameplay import owner and cannot be passed to the multiplayer content/LAN consumer. DLC maps use the owned disc shared-cache directory; their own directories do not contain mainmenu/shared caches. The balance test now accepts an explicit test resource path while preserving its old default. Both original TU1.1 and TU1.5 Lockout regressions also pass with that default. No production weapon behavior was changed by these test setup corrections.

Every map's full temporary cache copies are removed after comparison. Reports, ordered stores, and changed tag bytes are retained; the completed probe's rebuildable executable/object were removed. weapon-completion-matrix-v5.json preserves all630 gates, records bounded authored-contract evidence for every weapon, and also links the nine-sample native Hunter body/particle GPU diagnostic. No full gate or complete weapon is certified. Full campaign import/activation, weapon-specific histories, rendered/audio sequences and final Xbox reference comparison remain open. See all-map-balance-v3/report.json and checkpoint-all-map-balance.json in the goal analysis folder.

## Extended public Hunter owner sequence (October15)

The actual public placement has the authored Hunter position/facing, Covenant team3, and player team1. Both authored entry command scripts contain an initial220-tick sleep; their runtime progress has not been certified. The previous180-tick fixture did not cover a complete entry sequence. Extending the declared run to480ticks exposed the missing original1B35D0 method on public AI input186, source tick245.

Root now includes that complete nine-instruction method. It writes the current source tick to actor+1F4 through the actual object/state owner. All prior complete CFGs and every prior generated native function body remain byte-for-byte unchanged. The resulting module has632 complete routines/65,457 lowered instructions, plus the separately retained internal CRT entry37CE0C. The same actual API input passes unchanged-original/host-hardware/strict-native instruction and ordered-write checks:28,370 instructions,432 writes, full CPU except IF and58,978,390 non-stack bytes. Actual API stores/CPU/bank pass separately; its instruction trace is unavailable. An earlier comparison against a different captured input failed and remains labeled failed; the accepted comparison replays the exact current API input.

The public sequence now completes199ticks and next reaches originalD1878->18B4C0 in the object update. A private read-only observer captures the actual unresolved callee ABI, stack and full FP state immediately before the existing exception. Ordinary world source was not changed for that observer. The complete original56-instruction callee, including its actual BCDF0 call, executes70 instructions on this input and returns without non-stack stores. Host hardware reproduces the full exit/FP and58,978,390 retained non-stack bytes. Its native implementation is still pending.

A broader original object-update probe stops at an unsupplied NtWaitForSingleObjectEx import before reaching the contact callee and is explicitly incomplete. No kernel return was invented. Selected inputs/goldens and small proof receipts remain; redundant failed dumps were removed and identical private input copies were deduplicated. The full480-tick sequence, cannon firing, complete damage/contact scheduling, audio and Xbox framebuffer comparison remain open. See public-hunter-frame632-promotion-v1.json, hunter-public-186-api-evidence-v2.json, hunter-public-contact-original-v1 and checkpoint-public-hunter-extended.json in the goal analysis folder.

## Object hit-material query (October16)

The previously missingD1878->18B4C0 call resolves the target's global material index. The complete original query selects living/dead unit materials and falls back to the model's first damage-info indirect material. A dedicated source_object_material_tu15 owner now contains both complete routines,91 instructions, and the production world dispatch routes the actual call through it. The existing large physics-support module is unchanged.

The actual caller supplies a stack-local SHORT output. Non-stack comparisons alone would miss that observable result, so the strict replay now checks the caller's output word explicitly. The actual call returns material56 and matches unchanged original code, host hardware and native execution:70 instructions, full CPU/FP,58,978,390 non-stack bytes and the stack result. The sole declared input adaptation is the return-address sentinel used to stop the isolated original call; the actual incoming CPU/FP and other memory remain identical. Actual source API trace is unavailable and a separate same-input standalone consumer checks every instruction.

Six original/hardware/native cases pass: the actual living target and five explicitly declared disposable controls for dead-unit selection, model fallback, empty damage data, no model and a non-unit weapon. These controls are not additional actual gameplay histories. Every case compares the exact instruction path, full CPU/FP, non-stack bank and observable stack result. Their full temporary banks were deleted; the retained suite occupies about252KiB.

The public Hunter sequence now completes208ticks and next reaches204553->266D30 during the later object/AI notification phase. The complete480-tick sequence and the all-weapon feature/audio/visual gates remain open. Selected inputs/goldens remain and redundant full outputs were removed. See object-material-promotion-v1.json, hunter-object-material-api-evidence-v1.json and object-material-controls-v1/report.json in the goal analysis folder.

## Post-object AI notifications (October16)

The actual204553->266D30 callback now executes its complete264-instruction body in the existing AI owner. The captured path executes425 instructions and one retained-memory write. Unchanged original code, independent host hardware, same-input strict native replay and the actual source API agree on full CPU/FP and58,978,390 non-stack bytes. The only prior generated body change binds original26F859 directly to the recovered callee; every previous complete CFG is unchanged, and none of360 prior verified reference paths executes that callsite.

The following204589->2658B0 callback is also recovered. Its complete23-instruction body updates four actor fields using the original clock and incoming object identity. Original/hardware/native execution and the actual API agree on all four writes, full CPU/FP and the complete retained non-stack bank. Every prior generated native function body remains byte-for-byte unchanged for this addition. Root now contains634 complete AI routines/65,744 instructions, plus its separately retained internal CRT entry37CE0C.

The reusable private observer preserves original callsite and ABI information around nested AI calls and unresolved exceptions; it supplies no game-function return. The public sequence advances farther within source tick268 and next reachesE558D->2039B0 during the object postphase. Its completed-tick count remains208 and the requested480-tick sequence is unfinished. Both accepted API comparisons have separate same-input standalone instruction traces because actual API instruction traces are unavailable. Redundant failed whole-run dumps and duplicate outputs were removed; selected actual inputs/outputs and hardware goldens remain. All63 weapons and630 gates remain in scope, with zero complete weapons certified. See public-hunter-frame633-promotion-v1.json, public-hunter-frame634-promotion-v1.json and checkpoint-public-hunter-notifications.json in the goal analysis folder.

## Recorded-animation storage and public damage notification (October16)

The original startup row55 at44B68C identifies the pool queried by2039B0 as "recorded animations." The native world omitted both its constructor2035B0 and its map reset2035F0, leaving5021FC NULL. The earlier unresolved query capture is retained as defect evidence and is not an initialized campaign reference. The world now runs both original callbacks in their registered order after row54. A small generated module retains11 complete routines/377 instructions, including the constructor, map reset, deactivation and object query; it supplies no allocation result or query return.

Actual constructor, reset and nestedE558D query inputs independently match unchanged TU1.5, host x86, strict native replay and the actual source API. They execute293/84/15 instructions and44/9/0 retained writes respectively, with exact full CPU/FP except IF and complete non-stack banks. Four explicit disposable populated-record controls match the original query's identity, bitmap and excluded flag behavior; these are branch controls, not additional gameplay histories. Their large temporary banks were removed after verification.

The public counted Hunter sequence now completes218ticks. It next reaches actualCC9CD->1D1880 at source tick278. The older paired damage owner reports canonicalCB84D for this call; the actual selected return address isCC9D2. The new original callback replay completes12,500instructions and81 retained writes without patched instructions or supplied game services. Native recovery and the requested480-tick sequence remain in progress. All63 weapons/630 gates remain in scope and zero complete weapons are certified. See recorded-animation-promotion-v2.json, recorded-query-controls-v1/report.json and hunter-public-damage-notification-original-v1/report.json.

The native damage notification is now accepted. Ten added complete routines bring the AI owner to644 complete bodies/66,584 instructions, plus the separate internal CRT entry37CE0C. All prior complete CFGs are unchanged. Nine prior call sites now statically bind recovered original callees; none appears in the360 previously certified instruction traces. The accepted actual API input has its own unchanged-original, host-x86 and strict-native replay:12,500 instructions,81 ordered writes, full CPU/FP except IF and58,978,390 non-stack bytes match. A first cross-run comparison correctly rejected differing process-state input banks; it was replaced by the same-input accepted comparison and was not counted as a pass.

The completed object tick clears the source player's+2C unit binding toNONE and the former unit's+13C player field toNONE. The fixture previously kept delivering a command to its initial unit, stopping at219completed ticks. Its next version reads the actual full player identity and current unit binding for each input tick and delivers no unit command while unbound; it supplies no respawn, rebinding or health result. See public-hunter-frame644-promotion-v1.json, hunter-damage-notification-api-evidence-v2.json, hunter-player-binding-after-damage-v1.json and capture-public-hunter-sequence-v2.json. The refreshed weapon-completion-matrix-v9.json retains63 paths/1,110 occurrences/630 gates and zero complete weapons certified.

The updated input fixture reaches the next AI frame with one unbound-input tick, passing the previous stale-unit guard. It stops at actual26E5E4->26F260 at source tick279; the complete callback has87 instructions and two direct callees. The completed-tick count is219. Its selected incoming input is retained for unchanged-original replay. See checkpoint-public-hunter-damage-notification.json and hunter-public-post-damage-callback-input-v1.

The untouched26F260 callback executes51instructions with zero retained writes. Unicorn and ordinary host execution disagree only on SF, aside from platform IF: Unicorn exitsEFLAGS=C2; host exits242. Independent ptrace execution confirms the same51-instruction path, exact full hardware CPU/FP and58,978,390 non-stack bytes. The last flag-setting instruction is original26F2C3 FCOMIP. The existing native comparison backend already clears SF/OF/AF, consistent with the [Intel instruction specification](https://cdrdv2-public.intel.com/868137/325462-089-sdm-vol-1-2abcd-3abcd-4.pdf). No original instruction or Unicorn output was patched. This separately retained observed hardware reference is accepted for this explicit comparison boundary; physical Xbox execution remains unverified. The candidate now contains646 complete AI routines/66,691 instructions, adding only complete26F260 and1ECCC0; native replay and live API checks are compiling. See checkpoint-public-hunter-post-damage-comparison.json and hunter-post-damage-comparison-hardware-v1/report.json.

The51-instruction26F260 path now passes both strict native replay and the actual internal native call against the unchanged host instruction trace, full hardware CPU/FP and retained bank. A private observer reads original ABI snapshots around internal native bodies and skips an entry already observed by its public caller. Its647 guards add no source instruction, source memory mutation or supplied return. The native comparison backend required no behavior change. Root now contains646 complete AI routines/66,691 instructions, plus internal CRT entry37CE0C. The public sequence reaches223completed ticks before actual26D049->266490. See public-hunter-frame646-promotion-v1.json and hunter-post-damage-callback-api-evidence-v2.json.

The entire next actual AI input224 has also completed in unchanged TU1.5 code:192,111instructions and9,675 retained writes. Ordinary host execution matches its GP/XMM/flags/depth and complete58,978,390-byte non-stack bank. Its raw x87/MXCSR sticky flags differ from Unicorn; those outputs remain separate and the original hardware state remains the native golden. This larger input recovers23 additional whole routines, including target and animation paths, bringing the candidate to669 complete routines/68,311 instructions. All prior complete CFGs are unchanged;32 prior call sites bind recovered original callees, and none is reached by the360 earlier certified traces. Native whole-frame and public continuation builds remain in progress. No full weapon gate is certified by these component results.

The complete actual AI224 call now passes unchanged original code, independent host execution, strict native instruction/store replay and actual API CPU/FP/bank comparison. Root contains669 complete routines/68,311 instructions. Its public sequence completes225ticks and next reaches actual275307->275370 at source tick285. The whole AI226 input has independently completed24,177instructions and353 writes; the672-routine candidate adds complete26C2A0,26C350 and275370. Its three prior call-binding changes are absent from the360 earlier certified traces. Native builds are in progress; these later routines are not yet promoted.

## Compact completed-capture retention (October16)

To prevent full memory dumps from growing back after the requested cleanup,42 completed captures now share source-address-aligned64KiB compressed chunks. Their2,523,907,596 raw bytes occupy about17MiB including manifests. Every full file is reconstructed and hashed before its raw copy is retired. The existing restore-retained-weapon-evidence-20261015.py helper restores each member with its original byte digest. Two material-query banks were restored through that helper and the strict native/hardware comparison was rerun, including the observable stack result; it passed. The restored banks and successful test's temporary output were then retired again. Active newer AI inputs and hardware flag-boundary references remain raw. See bank-chunk-retention-summary-v1.json, bank-chunk-restoration-native-check-v1.json and retained-bank-chunks-v1/README.md in the goal analysis folder.

## Animation and object-phase continuation (October16)

The accepted AI226 delta was merged into the independently advanced root, preserving helper1C9C90 and its surrounding work. The merged source passed both complete AI224 andAI226 replays. The next actual1D5897->1D63C0 animation callback passes165 original instructions, three ordered writes, full host CPU/FP except IF and58,978,390 non-stack bytes against both strict native replay and the actual live call. Its reference also recovers1C9C90 independently, matching the preserved implementation. Root now contains675 complete routines/68,496 instructions, plus internal CRT entry37CE0C. The existing object postphase production owner remains in place. See public-hunter-frame672-merged-promotion-v1.json and public-hunter-frame675-promotion-v1.json.

The complete actual object postphase at source tick301 now matches51,317 original instructions,3,110 writes, full hardware CPU/FP and retained non-stack memory. A private166-routine/14,516-instruction probe supplies the separate exact instruction trace; the actual source API supplies its CPU/store/bank comparison. Original Unicorn and host numerical outputs differ at observed CPU estimate boundaries; both remain retained. Independent hardware stepping confirms the original instruction path and supplies the hardware golden. This private code probe is not a replacement production owner. See hunter-postphase242-api-evidence-v1.json.

Observed public continuations have reached359ticks and, with bounded failure capture,361ticks. Repeat runs reach the next11750D->218020 callback at different ticks; fixed-ordinal discovery is not assumed reproducible. Fixturev3 retains one incoming selected root bank/CPU/partial-store snapshot in host memory, discards each successful snapshot and writes only the actual first failing input. It changes no game inputs or tick scheduling. Its captured object-update input362 completes8,909 original instructions and132 writes; the selected unit callback independently completes153instructions and eight writes. The678-routine candidate is compiling. The full480-tick sequence and all63-weapon feature/audio/visual gates remain open. See capture-public-hunter-sequence-v3.json and hunter-public-failing-objects-input-v1.

## Completed bounded480-tick public sequence (October16)

The actual11750D->218020 callback now passes153 original instructions and eight writes against both hardware/native replay and the live call. Its complete containing object update also passes8,909 instructions,132 ordered writes, full hardware CPU/FP except IF and58,978,390 non-stack bytes. Fixturev4 captures one complete object-update before/after pair when that real nested callback executes, retaining only one pending snapshot in memory. Its source input/tick loop is byte-for-byte identical to fixturev2; no state, target or return is forced. The separate private object-update probe preserves the production owner and its callbacks.

The public counted-placement sequence now completes all480 requested AI/object/particle ticks, with25 sound submissions. The source player becomes unbound after219ticks and remains unbound for261ticks. This nearby-player setup produces no weapon-effect events, effect-damage calls or projectile detonations; it does not certify public cannon firing. Root contains678 complete AI/shared routines/68,574 instructions, plus internal CRT entry37CE0C. Full weapon gates remain open, with zero complete weapons certified. The next work is a source-evidenced ranged setup and complete firing/visual/audio evidence. See public-hunter-frame678-promotion-v1.json, hunter-triggered-objects-api-evidence-v1.json and weapon-completion-matrix-v13.json.

## Ranged Hunter setup and storage follow-up (October17)

The original character weapon selector1ED530 is now pinned in full: actor+54 selects the character tag, char+CC/+D0 selectsCC-byte weapon rows, row+8 matches the weapon, and char+8 supplies parent inheritance. Original consumers read the firing limits at+C/+10 and select combat bounds at14/18,24/28 or2C/30. The authored Hunter cannon limits are2.25..20; normal combat bounds are2..10. See hunter-ai-weapon-ranges-v2.json.

A declared15-unit forward spawn at the earlier+3 height was outside the BSP and was rejected before player birth. A read-only BSP survey found the same forward point at+1 height in cluster7. That public run completes103ticks and reaches a new actual211C1B->2AFA50 call on AI input104, while the player remains source-bound. Its one incoming root capture is retained for unchanged-original replay. No trigger, health, target or service return is supplied. See hunter-public-ranged-run-v2 and capture-public-hunter-ranged-v2.json.

The latest cleanup stores35 more completed banks,2,103,256,330 raw bytes, in the shared verified chunk store and removes rebuildable hardware scratch. The goal folder was29,058,433,024 allocated bytes at that checkpoint, down358,501,502,976 bytes from the original baseline; subsequent active inputs are tracked separately. The run wrapper now propagates the capture exit code after writing receipts, so a rejected setup is recorded as failed. See storage-cleanup-latest-20261017.json. Full weapon parity remains unfinished.

The first actual211C1B descriptor callback on the ranged input now completes138 unchanged original instructions and eight writes, with exact host CPU/FP and retained-bank agreement. The full AI104 replay reaches its10-million-instruction safety limit and remains explicitly incomplete; its large disposable outputs were removed after retaining the report and a compact trace summary. The selected actual input and small completed callback reference remain.

The actual ranged bank has game+1120 equal to0. Original211B10 only checks a future script sleep tick when that byte is set; complete original191DF0 sets it after successful11E8E0 BSP admission. The current declared source-world fixture has not executed that game-start owner. This is a missing lifecycle requirement, not a reason to supply a ready byte. Natural ranged firing requires recovering startup/activation as well as the reached callback. See hunter-ranged-game-ready-audit-v1.json and source-game-start-191df0-v1.json.

The candidate callback delta preserves every previous goal body exactly and separately preserves independently added MCC hooks and descriptor routines in the shared root. Raw generation had also changed identity-literal formatting; those unrelated body rewrites were excluded. The merged private candidate has681 complete routines plus one internal CRT entry. Its strict and coherent fixture builds are tracked by ai-frame-root-probe-v48-job.json and hunter-public-ranged-build-v3-job.json; it is not promoted until its comparisons pass.

The actual internal2AFA50 invocation now passes the same138-instruction/eight-write comparison against unchanged original code, host CPU/FP, ordered stores and58,978,390 non-stack bytes. A private read-only observer was necessary because the recovered internal static call bypasses the world's external-owner observer. Its guard is absent from production. The verified delta is promoted; root now has681 complete routines/68,746 instructions plus internal CRT37CE0C, with every prior root body and independent MCC hook preserved exactly. See public-hunter-frame681-promotion-v1.json and hunter-ranged-builtin-api-evidence-v2.json.

The next actual2AFA89->27FA20 call has an independently completed original/host reference:6,831 instructions, zero non-stack writes, exact CPU/FP and retained memory. Its observed name operand is generic_player_fired and its numeric input is31; no additional behavior is inferred from that name. Native recovery of this next routine remains pending. weapon-completion-matrix-v14.json records the lifecycle gap and preserves all63 paths/1,110 occurrences/630 gates with zero complete weapons certified.

## Game-start lifecycle and level-handle construction (October17)

A declared native191DF0 bridge now captures the delivered game-options record before player, actor or object birth: source tick0, requested BSP0, selected BSPNONE and ready byte0. Its EAX input aliases the existing game+8 options record; this is an explicit native caller input, not a captured retail caller or a complete campaign boot. Original13A780 invokes54 nonzero new-map callbacks across68 registered rows. The first unchanged-original attempt faults at row13's1A59D0 because the4FAFBC/4FAFB8 level-handle pools are null. No callback was skipped or supplied.

The existing complete50-instruction1A5410 constructor is now bound into the production source world before clock storage. Its actual invocation passes all815 original instruction addresses,213 ordered source writes, full hardware CPU/FP except IF and57,073,750 non-stack bytes. The reference executes every original XBE instruction and supplies only the already defined native raw-pool allocation boundary: real anonymous host mmap pages, a fresh10000000 source-token namespace and explicit GP/FP preservation. An independent32-bit host run allocates its own real pages and agrees exactly. This tests the declared native platform ABI; Xbox kernel address selection and clobber behavior remain unverified. No pool header, ready byte or game-service result is installed by the adapter. The source prepares both real pool records itself.

The original game-start attempt with those constructed pools passes the former null access and stops later at the unsupplied MmSetAddressProtect import800000B6, reached through126730/2DB4CA after6,142 instructions. It remains an incomplete original-only lifecycle reference. The native game-start owner, its full callback sequence, ready-state publication and natural ranged firing are still open. See source-game-start-callback-table-v1.json, hunter-game-start-original-v1/report.json, hunter-game-start-original-v2/report.json, hunter-level-handles-actual-api-evidence-v1.json and level-handles-constructor-promotion-v1.json. The production source is separately compiled against current root headers; that check does not certify a linked game or complete startup.

## Cold activation and complete level-handle new-map owner (October17)

The existing complete original activation module is now reused in a private cold-start fixture. This fixture omits the earlier manual initialize_map and supplies an independent exact1118-byte options copy at60080000, matching the original caller's pointer ABI. It executes191DF0 and its registered new-map loop; it supplies no ready flag or callback return. The first actual unresolved callback is13A7BA->1A59D0, registered row13, before physics/player/actor/object birth.

That complete callback now has a separate native new-map owner:11 complete routines/899 instructions. Unchanged original code, ordinary host x86 execution, strict native replay and the actual native call agree on4,417 instructions,308 ordered non-stack writes, full CPU/FP except IF and58,986,582 retained bytes. This path needs no supplied kernel or game service. The production route and CMake source are integrated; both current-root world compilation and the new owner's actual CMake object build pass. The existing constructor owner is preserved. See level-handles-new-map-promotion-v1.json, hunter-level-handles-new-map-api-evidence-v1.json and level-handles-new-map-cmake-object-v1.json.

The private cold map-start now reaches the next registered resource-cache callback126730 at row16. Whole game-start, BSP admission, ready-state publication and natural ranged firing remain incomplete. A separate original-only discovery run with actual host-backed pages and two successful mprotect operations advances29,252 instructions to the unsupplied system-time import80000080. Its host page/cache policy is explicit; Xbox kernel clobbers and GPU cache coherency remain unverified. It is not a completed whole-game reference.

One cleanup assumption was corrected: retained build manifests also pin reused private object files. Eleven deleted objects were restored as verified hardlinks to exact duplicates; two world objects were rebuilt with their original commands and exact digests. All producer object paths are authenticated again. Keep every object named by a retained linked_files_sha256 manifest and compact completed banks instead. See level-handles-object-restoration-v1.json. weapon-completion-matrix-v16.json retains63 paths/1,110 occurrences/630 gates and zero complete weapons certified.

## Resource-cache and row24 startup comparisons (October17)

The actual registered row16 callback126730 now completes5,107 unchanged original instructions and4,734 ordered non-stack writes. Its strict default reference stops at the unsupplied800000B6 import. The separately selected native platform mode performs the two real host mprotect requests, preserving the declared GP/FP/flags ABI and consuming the original stack. An independent32-bit host run performs its own real page operations. Original, host hardware, strict native production-body replay and the actual cold-start call agree on full CPU/FP except IF and58,986,582 retained bytes. Xbox kernel clobbers and GPU cache coherency remain unverified. See hunter-resource-cache-api-evidence-v1.json.

The current production activation owner already contains the complete callback. Its body differs from the separately recovered closure only in four explicit complete-callee dispatches; the production owner independently passes the full same-input replay. No duplicate production resource-cache module was added. Private entry observers preserve the source instructions and permit capture of internal static calls. See resource-cache-root-body-correspondence-v2.json and resource-cache-root-owner-probe-v1/build.json.

The current production row24 callback11E550, including its complete219F90 tail, also passes unchanged original code, independent host hardware, strict native replay and the actual cold-start call:16,223 instructions,1,436 ordered writes, full CPU/FP except IF and58,986,582 retained bytes. The expanded private cold startup reaches registered row29's10FA90 next. These are complete callback comparisons, not complete game-start, BSP admission, ready-state publication or natural Hunter firing. See hunter-map-row24-api-evidence-v1.json.

Completed banks continue to move into the verified shared chunk store; producer binaries, pinned objects and original assets remain. Batchv7 compacts18 more banks. All63 weapon paths,1,110 occurrences and630 completion gates remain in scope, with zero full weapons certified.

## Complete reached Hunter script target (October17)

The actual2AFA89->27FA20 call now passes6,831 original instructions, zero non-stack writes, exact host hardware CPU/FP except IF and58,978,390 non-stack bytes. The complete six-routine/491-instruction closure retains all untaken CFG paths; four routines are new to the production AI owner:27FA20,29B4A0,32A604 and32D9E0. The two existing closure routines and every prior root body remain byte-for-byte unchanged, including independent MCC hooks. Root now contains685 complete routines/68,921 unique original instruction addresses, plus the separate internal CRT entry37CE0C. The preceding2AFA50 callback regression also passes unchanged. See hunter-ranged-script-target-api-evidence-v1.json and public-hunter-frame685-promotion-v1.json.

A private read-only observer captures the actual nested return. The ranged diagnostic optionally stops immediately after that completed capture, before resuming the surrounding AI frame. This prevents the already identified unready startup from producing an unbounded script run; it supplies no state, instruction or result. The source player is still bound at the103-tick checkpoint. Whole AI104, complete startup/readiness, natural Hunter cannon firing, visible/audio comparison and all map variants remain open. weapon-completion-matrix-v17.json preserves63 paths/1,110 occurrences/630 gates with zero full weapons certified.

## Cold shader, renderer and view storage (October17)

The isolated cold fixture now reuses the current original shader-function owner. Its actual10FA30 constructor matches75 original instructions/four writes against untouched XBE execution and independent32-bit host hardware. Its own complete native instruction trace, ordered stores, CPU/FP and58,589,270 non-stack bytes agree. The registered10FA90 new-map reset separately passes36 instructions/33 writes, strict native replay, the actual source call and58,986,582 retained bytes. No bank pointer or ready result is supplied. See hunter-shader-function-startup-api-evidence-v1.json and hunter-shader-function-map-api-evidence-v1.json.

The fixture also reuses the current CPU-only renderer interface composition and complete20E30/20E90 owners. This composition retains actual original allocation/CRC calls but does not execute or certify the preceding GPU initialization. With these required fields present, complete registered row31 callback22570 passes27,460 original instructions and10,556 writes, including511 actual20E30 calls. Original x86, independent hardware, strict native replay and the actual call agree on CPU/FP except IF and58,986,582 non-stack bytes. See hunter-renderer-map-api-evidence-v1.json.

Actual row36 constructor171E10 passes74 original instructions/three writes. Actual row43 constructor228260 passes294 instructions/219 writes, including the original zero-fill. Both provide complete actual native instruction traces and exact original/hardware CPU/FP and58,978,390-byte comparisons. The complete row36 new-map callback171F40 and its two complete helpers pass601 instructions/326 writes, strict replay and the actual source call. See hunter-row36-constructor-api-evidence-v1.json, hunter-row43-constructor-api-evidence-v1.json and hunter-row36-map-api-evidence-v1.json.

The cold fixture reaches row44 and stops at the already recovered128900 callee, which is absent from its older activation snapshot. A separately pinned shared activation probe with newer owners instead reaches2260E8. Original2260E0 loads52D674 immediately before that failed null-context write. Original224EC0 allocates/publishes that2AD8-byte sound context at224F15, before its387877 SDK factory call. The current source world explicitly omits that full audio-device startup after its CPU storage prefix. This is a remaining device/lifecycle requirement; allocating a replacement context or supplying success would not prove it. See next-audio-context-startup-audit-v1.json and shared-current-activation-check-v1/run.json. Full audio/device startup, BSP admission, readiness, natural Hunter firing and visual/audio parity remain open.

Private build toolv14 now records exact preprocessed input, source, compiler and semantic-flag admission before reusing objects across additional header search paths. Its first follow-up reused11 objects through those checks. Immutable duplicate producer objects are hardlinked while preserving all paths/digests;88 replacements freed77,205,504 bytes. See producer-object-hardlink-retention-v2.json. weapon-completion-matrix-v18.json still records63 weapon paths/1,110 occurrences/630 gates and zero full weapons certified.

## Complete audio-initializer discovery (October17)

A private optional call now attempts the complete original224EC0 initializer after the existing CPU sound storage prefix. The first run reaches3,127 original instruction labels and the actual37DA9A audio-settings query through2E101C. It stops because no UserSettings resource is bound. Its older native-exit destructor then throws during unwinding and aborts, masking the first diagnostic. The retained partial trace/input remains explicitly incomplete; it is not an original/hardware/native comparison or a successful device.

The follow-up binds the actual local256-byte EEPROM resource through the existing settings owner. Only its96-byte non-key UserSettings section is read or retained; keys are not copied. Native outputs contain the real query type4 and that section's actual AudioFlags. The current production noexcept exit guard is reused in the private fixture so the first error survives cleanup. This run exits normally with the expected diagnostic failure rather than aborting.

It reaches3,132 original instruction labels and stops at the encoded HalBootSMCVideoMode data import80000164. The actual trace identifies the helper entry33C29D and its33C2A2 dereference; neighboring byte33C29C is not that function entry. Its source audio context is published by the original initializer, while its device-initialized byte remains0. No boot-mode value, original reference mode, device success or game-service result is supplied. The partial-after bank may include native resource unwinding and is not a completed output golden. The next required input is real native ownership of that platform mode and the following device/driver prerequisites. See audio-startup-discovery-evidence-v1.json and hunter-audio-startup-discovery-v2/audio-startup/constructor/failure.txt. weapon-completion-matrix-v19.json keeps the entire63-weapon scope open.

## Observed platform input and native APU resource composition (October17)

An isolated cold xemu boot captures an actual SMBus transaction and its kernel publication using hardware breakpoints, with no guest code, memory, register or service-result patches. Its actual prior/status/response inputs are7/0/1; the observed publication is1. This is the owned Complex4627/xemu platform, not physical retail or original title-execution proof. Only the20-byte H2SMCR1 transaction input record enters native code. The observed publication remains separate test evidence. Temporary HDD/EEPROM copies and owned processes were retired; parent disks and platform files remain unchanged. See actual-smc-platform-input-v2/capture/report.json and job.json.

The new explicit input owner authenticates the original33C29D getter and41B784 encoded data import, evaluates the already recovered kernel expression, and mounts immutable native import data. It supplies no mode default, native SMBus transport, device result or Xbox kernel pointer. The focused current-root test checks the actual publication, unchanged original code/import, immutable ownership and rejection of foreign/malformed inputs. Root World's explicit binding API also passes a current-root syntax check without input drift. See boot-smc-input-root-test-v1/build.json and boot-smc-world-root-syntax-v1/report.json.

The complete private224EC0 attempt now executes that original getter and reaches3,702 source instruction labels. Reusing the existing native SDL/reset owner completes the whole cold APU reset and reaches4,083 labels. Adding the existing whole hardware-storage/DMA bodies and native page owner reaches91,577 labels, including the normal38E996 return and16 retained native allocations totaling339,968 capacity bytes. The default actual SDL stereo callback device opens; no dummy driver is selected. Logical native DMA addresses are not host PCI/DMA addresses. These are native composition results, not a full original CPU/FP comparison or PCM proof. No duplicate production reset/storage implementation was added. See audio-startup-discovery-evidence-v2.json through v4.json.

The next required owner is38E8C9's interrupt/frontend/scheduler configuration. Source audio initialization remains0; full device, interrupt, DSP, voices, PCM, cold game/BSP startup, readiness and every weapon's visual/audio sequence remain open. weapon-completion-matrix-v20.json preserves63 paths/1,110 occurrences/630 gates and zero full weapons certified.

Storage retention now compacts every completed capture immediately, verifying complete reconstruction before removing raw banks. Batchesv12 through v14 retain these six new banks in the shared chunk store. Pinned producer binaries/objects and original assets are preserved. The latest measured total is recorded in storage-cleanup-current-20261017.json, down from387.6GB to about31GB. This is the folder's allocated-size reduction; Btrfs shared extents prevent equating it with exclusively freed physical disk space.

## Complete original CPU comparison of the reached audio prefix (October17)

A read-only boundary observer now retains the actual38ECA3->38E8C9 CPU and source bank before resource unwind. It records four raw SDK allocations, one held SDK lock,16 native page allocations, one completed reset, control generation1 and16 completed native control commands with no reset fault. It supplies no return value or replacement source state. The original audio context still has initialized0. See hunter-audio-startup-discovery-v6/audio-startup/constructor/frontier-resources.json.

The untouched TU1.5 original224EC0 prefix, started from the exact captured input, matches all91,577 instruction labels,88,421 ordered non-stack writes including preimages, every captured GP/XMM/FP/flags byte, and all60,327,674 retained source bytes. Independent real host allocations/exclusion implement the declared native resource ABI. The EEPROM query executes separately compiled pinned compatibility-primary code against only the actual non-key UserSettings section. Offline reset MMIO inputs express the existing native control owner's cold graph; this does not execute or prove an original Xbox kernel or physical MCPX device. No native expected output, game result, source-code patch or byte mask is inserted. See audio-prefix-original-comparison-v1/report.json and v2/report.json.

All103 uninterrupted original CPU intervals also pass independent host i386 execution, with defined CPU/FP state and complete retained interval pages equal. The host observes91,553 single steps plus24 REP terminal-site events corresponding to the91,577 source labels. Its exact88,421 ordered non-stack writes have the same SHA256 as both the original emulator and actual native stream. The write-report reproduces byte-identically. Full interval inputs total1,900,544 bytes; large banks are not copied once per call. See audio-prefix-hardware-store-comparison-v1.json and audio-prefix-native-composition-evidence-v1.json.

This completes the reached CPU-prefix correspondence under its declared native ABI. It does not complete224EC0, the device or any weapon. The prior FECTL bit3 primary-source investigation remains a specific missing device-effect contract; accepting88 as an inert field would guess that behavior. Interrupt/frontend/scheduler, DSP/PCM, full cold map/BSP/game readiness and the entire63-weapon feature/visual scope remain required. weapon-completion-matrix-v21.json retains all630 gates and zero full weapons certified. Batchv15 compacts the actual before/frontier/partial-after banks and both original frontier banks, preserving exact reconstruction and every pinned producer path.

## Source Needler contact flow and unit impact feedback (October17)

The retained multiplayer source world now has a bounded actual Needler flow independent of the unresolved complete audio startup. Authored starting choice12 selects the real Lockout Needler/projectile definitions. Native roster activation, seed and held-fire controls are declared inputs; the weapon/object/projectile instructions own their actual results. Across240 ticks,30 loaded rounds become19,11 projectile identities are observed and all11 retire. The parallel native firing counter stays0. This first diagnostic uses an ordinary source velocity getter; the later two-player diagnostic uses the existing read-only BC200 query with separate scratch. No Xbox framebuffer, audible output or complete game startup is claimed.

The two-player diagnostic selects two actual authored spawn positions before birth, using a host ray only for scene selection. It aims through ordinary controls and supplies no target, trajectory, contact or damage outcome. Original source objects attach11 needles to the opposing unit; source shield damage begins at tick118. The last completed pre-reaction row retains shield0.7642855644226074. At tick165, unit impact feedback reaches actual E0DDD->1F5620, which the source dispatcher omitted. No positive contact-effect callback was observed before this stop. See needler-source-flow-evidence-v1.json.

The new complete source_damage_feedback_tu15 owner retains all72 original1F5620 instructions and all eight required lower callees. The actual reached branch executes19 instructions and two ordered non-stack writes. Untouched original code, independent host i386, strict current-root replay and the actual composed API agree on CPU/FP except host IF and58,986,582 complete non-stack bytes. The original caller return is the sole standalone input adaptation. The earlier cross-run bank pairing correctly failed and remains recorded; final acceptance replays the completed call's exact input with no memory masking. Generation reproduces byte-identically, the production source matches the tested module, and current root World syntax passes. The untaken branches are retained but not dynamically certified. See damage-feedback-promotion-v1.json and needler-feedback-actual-api-evidence-v2.json.

Production World dispatch now owns this callback. The private source flow advances to actual E0DE6->E3E20, the next missing unit-reaction helper, still at tick165. Its genuine incoming CPU/bank is retained in source-needler-next-feedback-capture-v1/captured-entry. No source-ready flag, callee result or damage decision is supplied. The installed runtime is unchanged. Complete Needler guidance/contact/fuse/detonation/visual/audio ownership and all other weapons remain required. weapon-completion-matrix-v22.json preserves63 paths/1,110 rows/630 gates and zero full weapons certified.

Failed/unexecuted build scratch is removed. The earlier target run and its producer were also retired after every observed tick, projectile and failure matched the stronger selected-entry capture exactly. Logs/source inputs and all required producer paths remain. Batchv16 retains the12 new memory banks through verified reconstruction, including the next unresolved call, without keeping duplicate raw banks on disk.

## Complete reached Needler unit reaction (October17)

The source feedback owner now retains complete E3E20, reached by original E0DE6 with the unit in EAX and raw DWORD flag1 on the stack. Its119 original instruction sites join the unchanged72-instruction1F5620 body. Generation reproduces byte-identically. Actual lower physics owners execute their own source bodies; no game or platform return is supplied. See damage-feedback-promotion-v2.json.

The actual two-player Needler diagnostic completes all240 ticks. It consumes11 rounds, attaches11 source needles, begins shield damage at tick118, records opponent death at tick165 and retires every projectile. Parallel native shots remain0. The completed E3E20 call's exact input passes untouched TU1.5 execution, independent host i386, strict native replay and the composed source API:11,493 instruction labels,1,088 ordered non-stack writes, complete CPU/FP except host IF, and58,986,582 retained non-stack bytes. The standalone return sentinel is the sole input adaptation. See needler-reaction-actual-api-evidence-v1.json and needler-source-flow-evidence-v2.json.

Shared physics previously reported canonical TU1.1 observer PCs even when selecting TU1.5 arithmetic and addresses. The explicit selected-image observation API now reports authenticated TU1.5 instruction/store PCs. Whole CFGs from both original executables authenticate477 routines/35,694 static sites. Arithmetic, memory operations and required-owner call ABI remain unchanged. The existing canonical observer API still passes all50 retained world/x87 cases across both versions. Every observed tick row also matches the earlier completed Needler flow exactly. tools/fix_physics_world_observation_tu15.py reproduces this layer idempotently while preserving the separate precision fixes. See world-observation-legacy-evidence-v1.json.

Production contains the tested feedback and observation modules. The current-root feedback module compiles and its1F5620 regression passes; current World syntax passes. The complete reaction/API comparison uses the pinned coherent runtime ABI. A full current-root composed runtime and the installed launcher runtime were not executed or updated here. No positive projectile-contact effect callback was observed. Complete guidance/contact/fuse/detonation comparisons, rendered and audible sequences, full cold startup, untaken branches and all scoped variants remain required. weapon-completion-matrix-v24.json retains63 paths/1,110 occurrences/38 maps/630 gates with zero full weapons certified.

Superseded replay binaries and unexecuted variants are retired after validation. Completed banks move directly into verified shared chunks; required producer objects, sources and original references remain. The current allocated folder size is recorded in storage-cleanup-current-20261017.json.

A read-only contact trace records889 projectile updates and11 actual100C1C->178E30 requests. Every observed request has effect datum FFFFFFFF and returns instance FFFFFFFF, explaining why this listener reports no positive allocation. All240 gameplay rows remain unchanged. This does not establish why the descriptor is null or certify other visual producers. The next comparison must capture that actual incoming request and execute the untouched original callee before judging its intent; no replacement effect is supplied. See needler-contact-effect-trace-evidence-v1.json.


## Authored Needler contact effect and visual admission (October 17)

The actual tick118 contact call is now compared against untouched TU1.5 code, independent host i386, the complete current-root owner and the composed source API. All34 original instruction labels, CPU/FP state except host IF, and58,986,582 retained non-stack bytes agree; the original performs no non-stack writes and returns FFFFFFFF. The real100C1C caller copies projectile definition+EC into the request's effect field. All36 scoped resident Needler projectile references contain the authored null value there, with no unresolved resident definitions. This contact field requires no replacement effect. Other firing, attachment and detonation producers remain required. See needler-contact-reference-provenance-v1.json and needler-contact-effect-actual-api-evidence-v2.json.

Completed source ticks retain a read-only8656-byte contrail snapshot in38 ranges:10 active Needler trails and38 points at tick118,11 distinct trail identities over the sequence, and all trails/points retired by tick239. Binding the actual admitted players to local views exposes positive firing, energy, standard-detonation and super-detonation effect records without changing any of the240 gameplay rows. The reached179DF0 particle phase matches571 unchanged original instruction labels, full CPU/FP except host IF and complete retained non-stack memory in host/native/API comparisons. All three particle-system pools are empty, so this is not emission or rendered-output proof. See needler-visual-input-evidence-v1.json, needler-visual-input-evidence-v2.json and needler-local-particle-actual-api-evidence-v1.json.

The renderer-storage diagnostic completes but its draw owner is inactive; prepare_storage returns before calling its source constructor. The fixture still needs production firing-effect resource preloads, actual camera/render inputs and live emission. Independent sparse trail snapshots also differ despite identical reported gameplay/pool identities; their geometry equivalence is not established. No framebuffer, audible sequence or physical Xbox comparison is claimed. weapon-completion-matrix-v25.json preserves63 paths/1,110 occurrences/38 maps/630 gates and zero fully certified weapons.

The user-requested cleanup now retires redundant archive containers after every retained member is reconstructed and authenticated in a shared compressed content store. Original archive hashes remain historical receipts; container bytes are intentionally discarded, while exact member bytes remain recoverable. restore-retained-weapon-evidence-20261015.py handles both the shared memory-bank store and migrated archive members. Completed raw banks are compacted after comparison, and byte-identical immutable producer files share storage while retaining their required paths. See storage-cleanup-current-20261017.json for the latest measured size and verification audit.


A subsequent diagnostic performs the production authored firing-bank resource preloads before advancing particles. All30 distinct authored inputs load without errors; the draw owner becomes active and source transparent storage is constructed. All240 gameplay rows still match. The exact captured179DF0 phase again passes571 unchanged original instruction labels, host i386 CPU/FP, strict current-root replay and the composed API over59,183,190 retained non-stack bytes. All three particle pools remain empty. Resource admission alone therefore does not establish emission; original effect-event admission with actual observer/camera inputs is the next required path. This test does not establish the storage constructor's original CPU correspondence, a complete current-root runtime, geometry or rendered parity. See needler-particle-resource-admission-evidence-v1.json and weapon-completion-matrix-v26.json. Batchv21 compacts all six new large banks immediately after comparison.


## Live Needler particle hierarchy and camera packet limits (October17)

The earlier empty-system sample at tick180 missed short-lived systems. A full240-tick census, repeated with the same verdict and identities, finds159 Needler system observations while the52-byte location and64-byte particle pools are empty after each phase. Capturing the first firing phase at tick80 shows the actual original cause:177913/17791A tests the render cluster union, which is zero in the headless fixture;17791E calls178290 and deletes the location chains. The original call, independent host i386, strict current-root replay and composed API agree on8,415 instruction labels,425 ordered writes and59,183,190 retained non-stack bytes. This is a missing diagnostic view-publication path, not evidence that retail Needler effects are absent.

The diagnostic now runs the actual157FE0 unit camera scheduler, complete observer adoption/motion/world owners and authored portal/frustum visibility. It publishes those calculated results through the normal native view path after the shared particle phase, preserving the previous frame's union for the next phase. No visibility bit or game result is forced. Live emission is observed. The hierarchy is systems84 -> locations52 -> emitters76 -> particles64; the earlier loose emitter terminology for the52-byte pool is superseded by this verified layout.

That view path exposed a production defect:245400 read zero packet limits from unconstructed56D324/56D328 and produced a1-degree normal FOV after observer repair. Original CRT rows46DC90/46DC94 reference complete382360/382380, each MOVSS/MOVSS/RET. Their six source instruction sites now join the camera owner, and World executes the original initializers before camera use. The two isolated constructor bridges derive inputs from a real native bank; they are not captures of a nested retail or native constructor caller. Both match untouched original code, independent host i386 and strict native replay on trace, ordered writes, full CPU/FP except host IF and59,183,190 retained non-stack bytes. All19 prior camera routines retain identical arithmetic and calls after literal fixed-image address simplification. Generation reproduces byte-identically, the promoted module equals the tested source and current World syntax passes. See camera-packet-limits-promotion-v1.json.

The actual240-tick diagnostic now retains authored70-degree unzoomed FOV for both living players; its gameplay rows remain exactly unchanged. At the captured tick80, every occupied full-identity chain is verified:6 Needler firing systems,10 locations,14 emitters and12 particles. Every live captured particle belongs to that real Needler firing effect. The particle pool reaches46 live rows during the full sequence. The reached179DF0 call passes16,227 original instruction labels,482 ordered writes, complete retained non-stack memory and independent host CPU/FP versus strict native/composed API state. Emulated FP status differs from host in this arithmetic path; the independent host defines the full CPU/FP comparison, with only IF excluded. See needler-live-particle-evidence-v1.json, needler-particle-lineage-evidence-v2.json and needler-camera-limits-particle-api-evidence-v1.json.

This proves bounded live emission and the camera-constant fix. World and first-person draw submission, vertex/material/shader comparisons, rendered/audio sequences, dead-player views, all camera modes, complete cold startup and every scoped weapon/map variant remain required. The complete current-root application and installed launcher were not executed or updated here. weapon-completion-matrix-v27.json preserves63 paths/1,110 occurrences/38 maps/630 gates with zero fully certified weapons. Completed large banks are compacted immediately after comparison; required producer paths remain retained.

The shared World received independent edits after the first syntax receipt. The repeated current-source check passes with the camera initializer calls intact; camera-packet-limits-promotion-v2.json pins that source and syntax receipt. needler-live-particle-evidence-v2.json includes the reproducible hierarchy verifier. weapon-completion-matrix-v28.json preserves the complete scope. Batchv22 retires26 new raw banks after reconstruction checks; current producer inputs remain authenticated.

## Completed Needler draw queues and live resource admission (October17)

The diagnostic now submits actual world and first-person particle draws with source cameras, view constants, projection, fog and depth inputs. The earlier diagnostic read an uninitialized raw portal bank as all closed. It now uses the same native authored portal initializer as multiplayer: Lockout has41 portals and no machine associations requiring live owners. No visibility or portal bit is forced, and production portal code is unchanged. Earlier emission/hierarchy evidence remains bounded evidence; its containing-cluster-only visibility does not establish draw coverage.

Multiplayer previously preloaded authored firing resources but did not admit newly live impact, detonation or child-system resources after the shared particle phase. Production now reads the actual effect and retained-system pools after that phase, preloads their real definitions, prepares required storage and registers the existing source-instance duplicate suppression. The same completed world queues produce zero submissions with the earlier firing-only resource set and1774 with live resources admitted:193 standard detonation,1481 super detonation and100 Master Chief shield depletion draws. The sequence also submits231 first-person firing draws. The pure resource-consumer comparison leaves source memory unchanged at the checked positive queue. All240 gameplay rows remain identical to the previous completed flow, with11 rounds consumed and no supplied game outcomes or parallel native shots.

Actual tick175/view0 world producer1770C0 passes4858 unchanged original instruction labels,96 ordered non-stack writes, independent host i386 CPU/FP and59183190 complete retained bytes against strict native replay and the composed API. This is an actual native bridge with stack argument0, not a captured nested retail caller. The actual API provides ordered stores but no instruction trace; the separate strict trace is checked. Every retained draw record and its real shader selection is authenticated. The shared vertex datum has distinct classification7/8 programs,51/63 instructions; both byte strings match the original Lockout map exactly. See needler-world-queue-api-evidence-v1.json and needler-completed-draws-evidence-v2.json.

The tested resource consumer matches current root byte-for-byte, and current-root multiplayer syntax passes. See live-particle-resource-admission-promotion-v1.json. The complete current-root application and installed runtime were not executed here. Vertex execution, geometry/rasterization, complete shader/material presentation, matched Xbox TU1.5 framebuffer/audio sequences, cold startup and all scoped variants remain required. Independent auxiliary visual geometry varies across runs, so this evidence makes no cross-run visual equality claim. weapon-completion-matrix-v29.json retains63 paths/1110 occurrences/38 maps/630 gates and zero fully certified weapons. Batchv23 compacts all six new large comparison banks after exact reconstruction checks.

The remaining1.085GB projectile-glow source archive is also retired after authenticating its original checkpoint hash, its6497-file source manifest, every6498 regular member including that manifest, and all four external-resource symlink targets. Exact regular-file bytes share the existing compressed content store; the external link metadata is retained without following or materializing those links. A real README restore through the public helper passes its original SHA256, then its temporary copy is removed. See projectile-glow-archive-restore-check-v1.json. The current required179 linked producer inputs remain present and unchanged; the general deduplication audit reports851 existing local pinned paths unchanged and no new mismatches. The latest measured folder allocation remains in storage-cleanup-current-20261017.json.

## Actual Needler world draws on current native graphics (October17)

The selected completed tick175/view0 source bank now authenticates the material owner of every one of its28 world draws. Full salted system/location identities, live occupancy, each location's parent chain and the original effect/event/system reference resolve four actual particle definitions: small fiery smoke, fiery smoke, Needler shard debris and the directional additive burst. Material selection is not inferred from a shared shader name. The bank reconstructs and hashes directly in memory from the retained chunks; no new full bank is written. See tools/prepare_captured_needler_gpu_tu15.py and needler-captured-gpu-input-v1.json.

The graphics probe imports those exact original material definitions and unchanged captured3360-byte vertex/32-byte depth records. Current ParticleRenderer records their native vertex, geometry and fragment pipelines against isolated640x480 color/depth attachments with the actual640x240 view scissor. Its84 recorded draw observations across the full, repeated and effect-isolated submissions preserve every descriptor-backed vertex/depth input byte, viewport/scissor and authored program selection. Standard detonation's four draws change52 pixels; super detonation's24 draws change1118 pixels; the full set changes1170 RGB pixels. The repeated complete RGBA readback is exact. Real Khronos validation reports zero errors and warnings on the RTX5080.

The tested C++ particle renderer matches current root. All12 current GLSL input files are snapshotted, the two exercised program byte identities match the original Lockout map, and all eight required shader modules compile and pass SPIR-V validation without root drift. Repeating the actual draw set with these freshly compiled current shaders preserves the previous coherent readbacks byte-for-byte. See needler-native-graphics-evidence-v1.json, needler-current-particle-shaders-v2/build.json and needler-captured-gpu-run-v3/report.json. The reusable runner authenticates its binary, linked inputs, source draw file, map, shader modules and real validation-layer library before and after execution.

This is one isolated world view at one source tick. It does not establish original NV2A arithmetic/rasterization, first-person graphics, BSP/body occlusion, full transparent ordering, glow/contrail composition, postprocessing, a complete current-root application, installed launcher delivery or matched Xbox framebuffer/audio sequences. The launcher currently selects the separate MCC test runtime and reports native MCC gameplay incomplete; it was inspected but not changed or certified here. Complete cold startup and all63 weapon paths/1110 occurrences/38 maps/630 gates remain required. weapon-completion-matrix-v30.json retains zero fully certified weapons.

## Native world and first-person particle sequence (October17)

A read-only extension of the same actual240-tick source flow now retains every draw's authored particle definition. The existing source render-state reader verifies the full salted system/location identities and parent chain, and the actual cache consumer confirms the selected template, pass, vertex program and classification. Source-memory checkpoints remain unchanged around all actual view/pass observations at ticks80 and175; the dead opposing player has no invented tick175 view. All240 gameplay rows and the living-view schedule match the preceding completed run exactly. All26 supporting objects retain exact compiler/preprocessing admission. No new full source bank is written.

This run retains1990 actual draws from nine authored particle definitions:231 first-person firing,252 standard detonation,1407 super detonation and100 shield-depletion world draws. Differences in auxiliary visual geometry/counts between independent runs remain explicitly unverified; the deterministic gameplay rows do not imply cross-run particle equality. See needler-source-particle-sequence-input-v1.json.

The current particle renderer and current compiled GLSL execute every one of the810 real view/pass samples on the native Vulkan graphics stages. All3980 descriptor-backed complete vertex/depth input observations match the captured bytes. All810 repeated RGBA readbacks are exact;691 source-empty samples remain exactly empty,65 world samples and44 first-person samples produce pixels. First-person firing peaks at23104 changed pixels at tick86/view0; world particles peak at3140 at tick174/view0. Real Khronos validation reports zero errors and warnings. Ten populated world samples at ticks230..239 produce no changed pixels; their expected original visibility is not certified. No geometry, alpha or material is changed to produce a positive result.

The refactored decoder also reproduces the preceding single-frame world readbacks byte-for-byte. Five peak/baseline PPM files and compact per-sample results retain the sequence without storing hundreds of framebuffer copies. See needler-native-particle-sequence-evidence-v1.json, needler-particle-sequence-gpu-run-v1/report.json and tools/verify_captured_particle_sequence_gpu_tu15.py. Immutable snapshots retain both producer source versions.

This verifies isolated native particle execution over time. It does not compare every new source call with untouched original code, establish NV2A arithmetic/rasterization, compose complete BSP/weapon/body/hand/transparent/glow/contrail/postprocess frames, verify audible output or certify a matched Xbox TU1.5 sequence. Complete cold startup, a complete current-root application, installed delivery and every scoped weapon/map/owner variant remain required. weapon-completion-matrix-v31.json retains all63 paths/1110 occurrences/38 maps/630 gates and zero fully certified weapons.


## Actual Needler contact update and native resource ownership (October18)

The first-damage tick118 now retains the complete actual FDEB0 caller through FDDE0/FDE67, its source bank and quiescent native resource ownership. The240 gameplay rows still match the preceding run. The original call reaches FFA60 contact handling and two178E30 effect requests. A synchronization-only replay returned but left the async-worker semaphore signaled, unlike the native owner. That result is retained as a rejected contract comparison.

The native World already declares126139 ->1A3B70 as synchronous animation I/O. A small reference provider now restores the actual29 completed read jobs and authenticates the same private resident and three shared resource snapshots. It uses the existing coherent SourceRuntimeResources implementation to validate the real jmad descriptor and read15872 sector bytes. The payload also matches an independent direct read of authenticated mainmenu.map. Only after the destination write succeeds does it retain the next real job and publish I/O completion. After contact, all30 read jobs and synchronization ownership match the actual native capture. Negative controls reject a different catalog, an invalid read namespace and an unavailable authored descriptor without returning a job or payload.

Under this explicitly declared native I/O contract,58124 original executable instructions return with all59183190 retained non-stack bytes identical to the native call. All1045 ordered source write records agree. Their22 canonical damage-observer labels are compared through a fresh complete90-routine/8976-site two-image CFG audit; neither source arithmetic nor recorded labels are changed. Only actual canonical memory-write sites are translated, since other current-image modules can overlap old read-instruction addresses numerically. Two separately recorded native I/O writes are checked against the actual sector bytes and completion cell. See needler-projectile-native-resource-evidence-v1.json, needler-native-animation-resource-controls-v1.json and damage-delivery-canonical-observation-audit-v2.json.

GP/XMM/flags/FS, empty x87 depth, control word and MXCSR match, while emulated x87 status is0220 versus native0020. Independent host FP and a complete native instruction trace remain required. The original lower asynchronous submission/worker body is outside the native synchronous I/O contract; no Xbox scheduling or kernel-clobber correspondence is claimed. This is an actual coherent source fixture, not a complete current-root application or installed delivery. Full projectile branches, rendered/audio comparison, complete cold startup and all63 weapon paths remain incomplete. weapon-completion-matrix-v32.json retains630 gates and zero fully certified weapons.


## Independent host and complete contact-path observations (October18)

Independent host i386 now executes the same first-damage contact call with unchanged original code bytes and real native animation-resource backing. The ordinary run delivers the native I/O boundary through an execute-protected entry and an alternate signal stack. The original caller frame and authored descriptor are checked; the independently restored cache owner reads actual sectors, retains its actual next job only after the destination write and then publishes completion. The lower asynchronous worker body is explicitly outside this native contract. An authenticated memfd supplies the60MB input bank without expanding a disk copy.

The independent host exit matches all native GP/XMM/flags/FS/depth/control/MXCSR and x87 status except Linux IF. Both actual host and native x87 status are0020; the emulator's0220 is a model difference. The complete59183190 retained non-stack bytes agree. A second host run single-steps original instructions without opcode patches or substituted CPU estimates and records every source and declared I/O store.

The tracing observer was corrected for two real decoding details: this Capstone version labels0F17 MOVHPS's memory destination as a read, and a zero-count REP touches no operand. The original opcode determines write admission. REP callback counts also differ: the emulator emits an additional zero-count exit callback after each completed repeated operation. Host records58094 callbacks versus58124 from emulation; collapsing only contiguous callbacks of the same original REP gives57483 matching architectural observations. Every actual store remains compared separately. All1047 host write records agree, including the15872-byte cache delivery and completion cell. The rejected observer results remain diagnostic evidence.

A read-only native diagnostic now records every instruction callback and its actual address domain. The Damage owner deliberately retains canonical older-image observer labels; nested external owners recover their own domain. After the complete90-routine mapping is applied only where explicitly marked, all58124 native callbacks equal the original trace. All1045 native source writes agree, and the two native I/O writes are separately authenticated. This new240-tick flow preserves every gameplay row from the previous capture. See needler-projectile-actual-api-evidence-v1.json, needler-projectile-update-host-trace-v4/report.json and needler-projectile-instruction-host-source-retention-v1.json.

This closes CPU/FP, memory, instruction-path and ordered-write comparison for this captured contact path under the declared native resource ABI. Untaken projectile branches, original asynchronous scheduling, a complete current-root runtime and matched Xbox TU1.5 rendered/audio sequences remain required. The coherent fixture does not certify installed delivery or every map/weapon variant. weapon-completion-matrix-v33.json retains63 paths/1110 occurrences/38 maps/630 gates and zero fully certified weapons. Completed full banks are compacted immediately after comparison and verification continues directly from the retained chunks.


## Natural Needler super-detonation and target death (October18)

The existing240-tick flow now supports reusable read-only capture selection by source tick and full projectile identity. Tick selection alone first captured E29E002F's76-instruction attached-projectile update, rather than the needle that retires. The baseline source rows identify E2940025 as the actual retiring identity at165. Selecting that identity changes no game control, contact, damage, effect or retirement result; all240 gameplay rows remain identical. The superseded tick-only executable was retired after verifying no external producer dependencies, with exact metadata and the negative-selection observation retained.

The selected FDEB0 call changes the actual target body's normalized health from1 to-6.622222900390625 and shield to0. It creates three real full-identity effects: Master Chief shield depletion, Needler energy and the authored Needler super-detonation. The projectile publishes body flag100 and state1 while its full object-pool identity still exists at call exit. Subsequent source cleanup removes it by the end of tick165. This explicitly distinguishes the captured call from physical pool release; the full flow later retires all11 needles.

The port's declared English interface-language input is now carried into the reference at the same two authenticated XGetLanguage callers. Its raw1 comes from the pinned compiled native World configuration; no console EEPROM query status is supplied or certified. This call also reaches the existing animation, geometry and texture native I/O contracts. Each actual source descriptor/tracking/page expression must match before the real cache owner reads sectors, writes the destination, retains a fresh completed job and publishes completion. Four reads deliver1310720 bytes: one animation, one geometry block and two texture blocks. Each payload independently matches the authenticated original map/shared file. Read ownership grows from30 to34 actual jobs, and synchronization ownership agrees. An unavailable source-file range is rejected before a job or payload is returned.

Independent ordinary and single-stepped host i386 executions match all native CPU/FP state except Linux IF and all59183190 retained non-stack bytes. All365032 architectural instruction observations agree. Native records368309 callbacks and host368058 because completed REP observations differ; raw traces remain retained. All13094 native source write records agree, with201 original128-bit MOVAPS stores represented as the native writer's documented two ordered8-byte records. The eight actual I/O writes are checked separately. MOVLPS and MMX MOVQ store opcodes correct further Capstone read-only annotations. Observation domains also prevent actual-image B9D1C/B9D35 writes from being remapped as older-image instructions. The two remaining shared numeric sites are actual comparisons and authenticated canonical stores; ambiguous writes are rejected.

Unicorn follows an additional CRT floating-point-control path:368334 callbacks and different FP/control state and stored values. Native instead matches actual host execution, including complete retained memory and ordered writes. No game arithmetic or instruction result was changed to force emulator agreement. See needler-super-detonation-actual-api-evidence-v1.json, needler-super-detonation-host-trace-v2/report.json, needler-super-detonation-source-retention-v1.json and needler-weapon-resource-bounds-control-v1.json.

This verifies the captured natural super-detonation/death path under the declared native locale and cache I/O contracts. Standard-detonation and other untaken branches, original async worker scheduling, console locale discovery, complete current-root runtime/delivery and matched Xbox TU1.5 framebuffer/audio remain required. weapon-completion-matrix-v34.json preserves all63 weapon paths/1110 authored occurrences/38 maps/630 gates, with zero weapons fully certified. Completed full banks are compacted after comparison and the same verifier reads their authenticated chunks directly.

## Natural Needler standard detonation and retained evidence (October18)

The same source flow reaches the first authored standard-detonation effect at tick175. Read-only capture selects the actual E29E002F projectile and FDEB0 call. All240 gameplay rows remain unchanged. The call creates the real Needler energy and standard-detonation effects and changes the already-dead target body's health from-6.622222900390625 to-6.65333366394043. Projectile state changes from0 to1 while its full pool identity remains at call exit; its body flag100 remains clear. The end-of-tick live projectile list subsequently excludes it. The published player Unit retains cached health, which is not another raw corpse-body vitality capture.

Ordinary and single-stepped host i386 executions match native CPU/FP except Linux IF and all59183190 retained non-stack bytes. All47217 architectural instruction observations and1338 ordered source stores agree. Native records47923 callbacks and host47894; their raw REP observations remain separate. Native and host x87 status0120 differs from emulation0100. This call performs no additional resource reads, and actual read ownership remains34 jobs with matching synchronization state. See needler-standard-detonation-actual-api-evidence-v2.json and needler-standard-detonation-source-retention-v1.json. Batchv30 verifies exact reconstruction and removes all six completed full-size banks; the verifier passes directly from retained chunks.

weapon-completion-matrix-v35.json retains all63 weapon paths/1110 authored occurrences/38 maps/630 gates with zero fully certified weapons. Untaken branches, original asynchronous workers, complete current-root runtime/delivery, all variants and matched Xbox TU1.5 visual/audio sequences remain required.

An isolated original-software reference boot now uses a private HDD overlay and EEPROM copy. Direct X11 NVIDIA graphics avoids the failed Xvfb llvmpipe/NVIDIA Vulkan display-sharing combination. The complete loaded3570764-byte text section exactly matches assets/disc/default.xbe, SHA70c5fbef9836295e4895149eba14c961da5f9fe6168c39c2a0f28315b2910d7b; it does not match TU1.5. The emulator was shut down normally and its redundant text dump removed after comparison. No reference frames are certified as TU1.5. See xemu-retail-reference-v1/loaded-retail-base-identification-v1.json. Core dumps are disabled for further launches. The earlier failed launch left a root-owned19840524-byte system coredump outside the goal folder; unprivileged removal and noninteractive sudo are unavailable, so that exact file is recorded for cleanup without claiming its removal.

## Live original TU1.5 Needler reference and firing interval (October18)

The existing xbox-tu15-reference-disc-v1 image and normally played Lockout session are reused after the image SHA and all81 packaged file hashes are checked. No new disc image or complete source/build copy is created. A private reflink of the small session disk retains its existing read-only backing; a private EEPROM and Xvfb display isolate changes. The original OpenGL llvmpipe backend runs successfully with core dumps disabled. On restore and after normal menu/game transitions, the complete live3611948-byte text section matches the authenticated TU1.5 SHA d9c6fd7e7475862b1f19c2da80d7f2f6dc41da53619991002e9546e5f9e629fe. This supersedes the base-disc attempt as the current version-verified original-software reference.

Normal controller menu inputs select Split Screen, Lockout and the Needler starting weapon. No guest code, memory, register, camera, clock, target or weapon result is supplied. Read-only compressed censuses identify the actual held full-datum Needler, player and every live object. A normal right-trigger hold advances source ticks429 to436: loaded ammunition30 to28, reserve60 unchanged and two new full-identity authored Needler shard projectiles. The real first-person Needler and full Lockout scene are retained as window readbacks. Their exact GPU/source-tick boundary, palettes, native pixel equality and audio have not been joined or compared.

A separate restore of that normally played idle state observes the first two ammunition decrements with an actual guest hardware watchpoint. The unchanged original stores occur at ticks430 and434, each reporting following PC1082C4. Native source flow first decrements at80 and84, matching this four-tick interval and29/28 loaded counts with60 reserve rounds. This proves one interval; it does not establish complete sustained/released firing, reload or visual parity. See needler-original-live-cadence-v1/report.json and needler-original-reference-evidence-v1.json. The latter verifier authenticates93 retained inputs, including exact diagnostic sources, normal-input records, compressed actual memory, the shared original image and the required private idle-state template. The standard-detonation native CPU/FP/memory/store verifier repeats identically from retained chunks.

The original emulator exits normally, its private display terminates, the original source session disk remains byte-identical, and the retained private template is read-only for future copied sessions. Only the needed small session disk is reflinked; the original multi-gigabyte disc and base disk are reused. weapon-completion-matrix-v36.json preserves all63 paths/1110 authored occurrences/38 maps/630 gates with zero fully certified weapons. Matched original/native full graphics, poses/materials/effects/audio, complete current-root runtime/delivery and every scoped weapon/map variant remain required.

## Original Needler and hand GPU geometry owners (October18)

The retained normally played idle state now supplies actual original first-person GPU inputs. tools/capture_xbox_tu15_weapon_graphics.py uses a temporary reflink, the existing original image and shared backing disk, a private display, hardware breakpoints and the observed rendering-thread context. No guest code, memory, register or game result is set. The complete live TU1.5 text is authenticated before and after capture. The executable's real xemu path preserves the rendering-thread name required by the existing context bridge; its AppRun symlink changes that name while resolving to the same bytes. All temporary session disks are removed after their processes exit and the source template remains byte-identical.

The initial132B0-to132B0 capture contains335 UI draws, all with only three or four vertices, and no authored first-person index sequence. It is rejected as a geometry reference. Its unnecessary62771364-byte container is intentionally removed after the negative action inventory, hashes and zero current producer dependencies are verified; metadata remains in retire-rejected-ui-only-capture-v1.json.

The retained positive capture begins at an actual405080 indexed submission whose1058 original indices match the authored first Needler part. It ends at a subsequent matching submission after the source clock advances434 to436. needler-original-graphics-v4/original-weapon_capture.rdc retains98949022 bytes. The normal original software supplies all inputs. This is a partial rendering interval with interleaved emulator UI activity, not an independently certified whole-frame boundary.

Replay identifies eleven actual GPU draws belonging to five authored geometry parts: Needler luminous needles, dull casing and gun body, plus both Chief hand sections. Every ordered index is re-read from the original geometry resource and compared. All57 consumed non-generic attribute observations match original resource bytes for every referenced vertex, including interleaved positions, bone bytes, weights, UVs and packed basis fields. Relative offsets are checked against the independently matched complete source-stream anchors. Inactive stale buffer bindings are excluded; actual generic shader inputs remain explicit runtime observations. Original part material references are also re-read from the cache, without inferring the active pass or shader from those names. See needler-original-draw-owner-evidence-v2.json and needler-original-graphics-evidence-v1.json.

The authored190-index Needler core part is not observed in this capture. Its expected visibility in other states remains unresolved; no rendering change is inferred from its absence. Active shader/pass selection, original WORLD-node/palette correlation, native poses and shader arithmetic, matched native color/depth output, complete scene composition, audio and physical Xbox precision remain unverified. The source capture, actual uniforms, shader source, ordered index bytes and vertex data are retained for those comparisons. No native graphics parity is claimed from original-only replay. weapon-completion-matrix-v37.json retains all63 paths/1110 authored occurrences/38 maps/630 gates with zero fully certified weapons.

## Original live first-person palettes and required cleanup (October18)

The v6 capture observes eight completed actual13A2E0 palettes and matching168DED
WORLD-node records at ticks430,432,434 and436: Chief hands and the Needler at each
tick. The effective WORLD pointer is observed inside the original owner and its
bytes equal the captured input. The current native `source_model_palette` consumes
those original inputs, literal inverse binds, selected sections and resident node
maps. All2640 matrix float words, base counts and defined region ranges match the
actual original returned palettes exactly. Unused header slots are excluded.
This verifies native palette generation from original poses; it does not verify
native animation/pose production. See needler-live-palette-comparison-v2/report.json.

The same capture again authenticates all ordered geometry indices and57 consumed
attribute observations across eleven GPU draws and five authored parts. Its actual
shader programs expose rigid and indirect palette addressing, including the
observed address-register variants. Every one of1536 consumed matrix words matches
exactly one of the observed original CPU palettes and its native result, at tick434
or436. The comparison uses the actual reflected uniform capacity and referenced
vertex bone bytes. Two rejected verifier assumptions about uniform capacity and
address temporary names are retained as diagnostics; neither was a game or native
palette mismatch. See needler-original-native-palette-evidence-v1.json. Whole-frame
boundaries, native poses, shader arithmetic, color/depth output, complete scene and
audio remain unverified. No weapon or complete gate is certified by this result.

Cleanup is a required part of the goal. The prior pass removed1440 completed test
files and reduced combined artifact-folder allocation by11.55GB while retaining
exact required inputs. New probes compile and execute only in runner-owned scratch,
which is removed afterward. The superseded small palette executable is also
retired into exact compressed storage. New original GPU JSON observations are
losslessly reduced from30472827 to1629251 bytes; their original hashes and complete
reconstruction are retained. Required GPU inputs, small comparison fixtures,
commands and exact diagnostic sources remain; no new capture container, complete
source tree or persistent build tree is made. Paths, sizes and cleanup checks are
recorded with the evidence. See docs/TEST_STORAGE.md and goal-completion-contract-20261018.json.

The subsequent original GPU post-vertex export retains1.8MB of required outputs
for the same authenticated capture, without another capture container. Two actual
hand draws provide1530 unique vertices. Current production scalar and SIMD skin
functions and their effective weights agree, but6541 of18360 compared basis and
eye-relative position words differ from this OpenGL host replay, with maximum
absolute difference1.9073486328125e-6. A single retained vertex reproduces four
differences. Both failed comparisons, exact sources and small fixtures are kept.

Single-variable probes followed by combinations identify reciprocal-multiply
UNORM weights and right-associated DOT3 as a complete explanation of all13770
captured basis words. Right-associated DPH explains all4590 eye-relative position
words. This sample cannot distinguish division from reciprocal multiplication
for its SNORM position fetch. These are captured-host arithmetic observations,
not physical NV2A rules or corrected production skinning. The production kernel
is unchanged and its original-GPU precision comparison remains failing. See
needler-live-skin-comparison-v1/report.json, needler-skin-rounding-diagnostic-v1.json
and needler-skin-position-diagnostic-v1.json. Matrixv39 retains all630 gates and
zero fully certified weapons. Native poses/import, full frames and physical Xbox
precision remain separate requirements.

## Original live first-person calls and CPU precision boundary (October18)

The original TU1.5 software now supplies actual entry/exit CPU state and physical
RAM for complete `168DED` model-preparation calls. Hardware breakpoints observe
the naturally reached call and return. The real CR3 page tables, virtual aliases,
segments, raw floating-point registers and complete executable text are retained
and authenticated. The pinned xemu debugger hardcodes its tag-word reply to zero;
the independently observed monitor FTW and physical registers establish the empty
x87 stack. Raw debugger observations remain unchanged in the evidence.

The idle call at tick435 and a separate call after normal right-trigger input
both return the actual37-node Chief hands and17-node Needler. A hardware
ammunition watchpoint observes the real30-to29 decrement, with60 reserve rounds,
before the firing case is selected. Each original call independently replays
exactly through5722 instructions and935 ordered stores. The existing compiled
native owners, given only the captured entry state and memory, match all54 nodes,
model identities, the complete instruction sequence and4608 written bytes in
each case. No expected pose or gameplay service return is supplied. This checks
model preparation from original cached animation state; it does not certify the
entire native animation history or current-root application.

The only raw CPU difference for those two calls is x87 control-word bit6:
original emulation023F versus the host's027F. Intel253665-021 Volume1 Figure8-6
identifies this bit as reserved. A real FLDCW/FNSTCW probe verifies the host's
normalization, and negative controls reject changes to all sampled nonreserved
fields. The raw discrepancy remains in the failed strict reports; separate
evidence compares the checked defined fields while recording that exception.
See first-person-original-call-v1-native-evidence-v1.json and
first-person-original-fire-call-v2-native-evidence-v1.json.

An earlier complete `168CA1` update is captured after another normal first shot,
at tick438. It executes the actual169C46 and16A4DD registered update owners and
changes1897 bytes of the33584-byte first-person state bank. Independent unchanged
original execution matches its27216 instructions,5029 store records, CPU state,
owned outputs and entire state bank. The native adapter initially lacked a route
to121890; adding the already compiled scene-device owner follows the production
dispatch and supplies no fabricated result. The completed native call follows
the same instruction sequence but differs from the emulator's transforms.

The unchanged original bytes also run independently on the host i386 CPU under
strict retained-memory and single-step observation. Native execution matches all
27216 instruction observations,21748 written bytes,148 reciprocal-square-root
estimate lanes, checked CPU/FP fields and the entire33584-byte state bank. Native
records5030 stores versus5029 original host records because one store is split;
ordered byte writes match exactly. Original source instructions and data are
unchanged. Linux segment/stop routing is a declared test boundary.

All148 estimate inputs and instruction identities agree between original
emulation and host/native execution, while every estimate output differs. The
pinned xemu source computes single-precision square root followed by division;
that expression reproduces every observed emulator estimate. The first is
input4E7FFC00: emulator38000100 versus host38000000. The first differing store
follows at2961E1. This leaves604 differing state bytes and raw x87 status4200
versus4000. Those failures remain explicit. The production arithmetic is
unchanged; neither this AMD host nor the emulator establishes the original Xbox
CPU's estimate/FP rules. See first-person-original-update-fire-v1-host-v1/report.json
and first-person-original-update-fire-v1-cpu-boundary-v1/report.json.

All private binaries, compiler objects, working disks and raw RAM dumps from
these runs are removed, including failed attempts. Six required original64MiB
RAM observations are retained as authenticated shared compressed contents with
exact reconstruction checks. Small reports, traces, commands, source snapshots,
raw CPU observations and node/state outputs remain. Identical immutable evidence
copies share storage without changing their paths or bytes. The original disc,
read-only session template, launcher-selected runtime and unrelated active tests
are preserved. See original-pose-reference-retention-v1.json and
post-original-pose-storage-audit-v1.json for exact retained paths and sizes.

Matrixv40 retains all63 paths,1110 occurrences,38 maps and630 gates, with zero
fully certified weapons. Physical Xbox precision, complete native state origin,
all transitions/species/hands/regions, current-root runtime integration, final
frames/audio and the remaining weapon scope are still required. Removal of unused
test artifacts remains mandatory after validation and before goal completion.

## Original plasma-pistol input and SMG firing observations (October18)

Normal controller input and the original menus supply a solo Lockout plasma
pistol. The live object/tag census authenticates its identity rather than
inferring it from the image. At tick97, a complete original `1030F0` call routes
the initial primary input into the weapon's secondary input state. Independent
original execution matches705 instructions,92 ordered stores,303 written bytes,
the complete5750-byte unit,774-byte weapon and33584-byte first-person state bank.
This is initial input routing; it does not prove full charge, charge effects,
charged-bolt allocation or release. The full-charge capture attempts produced no
valid case and remain recorded as failed observations.

The existing compiled native owners match those instructions, stores and object
outputs. Their strict comparison against original emulation fails on raw x87
control027F versus023F and status4020 versus4000. Unchanged original bytes executed
independently on the host CPU match the native instructions, all303 written
bytes and all checked CPU/FP fields. The emulator's precision-status difference
remains open. No production arithmetic or expected gameplay return was supplied.
See plasma-original-charge-start-v1-replay-v1/report.json,
plasma-original-charge-start-v1-native-v1/report.json and
plasma-original-charge-start-v1-host-v1/report.json.

A separate normally played solo Lockout session supplies an SMG. Its actual
definition is3850044422, not the battle rifle originally inferred from a menu
image. The complete original `1030F0` firing call at tick166 consumes60 to59
loaded rounds and returns normally. Independent replay executes43941 original
XBE instructions and7915 stores, then stops at the real kernel entry80020F5A.
The actual PE export table identifies ordinal234; the pinned primary NXDK export
declaration names NtWaitForSingleObjectEx. The source call at2E037F uses the
original import41B738, encoded800000EA and resolved live to80020F5A. No wait result
is supplied. Full native comparison was not executed because independent
original full-call verification did not pass.

A diagnostic reconstructs that independent prefix from actual entry RAM and all
7915 ordered writes, checking every overwritten byte. The original mapped kernel
code is authenticated against both live entry/exit RAM observations. It executes
59 additional kernel instructions and27 stores, then stops before CLI at8001436F
inside80014368. This is a reconstructed independent prefix, not an actual live
kernel-entry capture or a native interrupt owner. It leaves interrupt, pending
IRQ/PIC, scheduler and wait-object ownership unresolved. Earlier diagnostic
harness failures remain separate from this successful boundary identification.
See smg-original-fire-v1-replay-v1/report.json,
smg-original-fire-v1-kernel-boundary-v2/kernel-api-name.json and
smg-original-fire-v1-kernel-prefix-v3/report.json.

All working emulator disks, raw RAM dumps, compiler products and scratch from
these observations are removed, including failures. Four required original64MiB
RAM observations remain as exact shared compressed inputs. The new normally
played reference VM is also retained as authenticated compressed contents:
355336542 logical disk bytes reference139123180 compressed chunk bytes. A
compressed-only restore authenticates the entire TU1.5 executable text and the
same live SMG before the redundant raw disk is removed. The snapshot identifier
`tu15-plasma-reference` is a legacy label; its actual held weapon is the SMG.
Restoration creates working disks only under owned test scratch. The unused
4113088-byte OCR download is removed while its exact public download recipe,
hash and menu diagnostics remain. Shared chunk sizes overlap other retained
references and do not imply exclusive physical space reclaimed.

Matrixv41 preserves all63 paths,1110 occurrences,38 maps and630 gates with zero
fully certified weapons. The two new trigger/resource gates are partial, with
the complete native lifecycles, current-root runtime integration and full
original visual/audio sequences still required. The completion contract still
requires cleanup after each validation and before the overall goal can finish.
See original-weapon-reference-retention-v1.json,
post-original-weapon-storage-audit-v1.json and
checkpoint-original-weapon-20261018.json for hashes, retained paths and sizes.

## SMG kernel continuation and next ownership boundary (October18)

Further diagnostic execution explicitly admits guest CLI/POPFD flag changes,
using the captured CPU flags. It supplies no device, IRQ/PIC, asynchronous
interrupt, scheduler, kernel or gameplay result. The authenticated original
NtWaitForSingleObjectEx path returns through282 instructions and80 stores.
The next original NtReleaseMutant path returns through182 instructions and63
stores. These are uninterrupted independent executions from reconstructed
prefix state, not actual live kernel-entry/exit observations or native kernel
implementations. The original kernel bytes remain unchanged and are checked
against both original full-call RAM observations before execution.

The complete weapon replay with those checked kernel sites proceeds to
NtReleaseSemaphore at80013C0B, ordinal222. It remains a failed full-call
comparison. A separate independent prefix then executes312 original kernel
instructions and98 stores, reaching80018DD1, `mov ebp, cr0`, inside80018DC0.
Control-register/context-switch ownership remains unavailable; execution stops
before the read. The dispatch path receives no fabricated return, worker
completion, clock value or interrupt state. No complete native firing comparison
was executed. The original actual firing60-to59 observation remains valid,
while its independent full-call verification remains open.

The later runs reuse the same retained original RAM. They add small reports,
source copies and traces, with no new large reference inputs or retained test
builds. Matrixv42 keeps all63 paths,1110 occurrences,38 maps and630 gates and
zero fully certified weapons. The audited24 owned scratch paths and the prior
publisher's scratch are removed. This boundary refinement completes no weapon
or gate and preserves the mandatory cleanup completion requirement. See
original-smg-kernel-continuation-v1.json,
post-original-kernel-storage-audit-v1.json and
checkpoint-original-kernel-20261018.json.

## Original worker handoff and actual native resource read (October18)

Independent execution now supports the captured physical64MiB RAM together with
the actual CR0/CR3/CR4 paging state. This uses architectural page translation
rather than preinstalled virtual aliases. Against the earlier original alias
replays, the plasma-pistol call matches705 instructions,92 stores and303 ordered
source-write bytes. The Needler firing model-preparation call matches5722
instructions,935 stores and4608 bytes. Checked CPU/FP fields and control
registers match. Physical addresses are normalized explicitly for this
comparison; the ordinary native comparison rejects unnormalized physical
traces. Source instructions and source inputs remain unchanged.

The original SMG semaphore-release continuation executes408 kernel instructions
and120 stores using its actual control-register state. It switches the source
stack and resumes another waiting continuation at2E0385, rather than returning
to the semaphore caller at2E02D6. No thread context or scheduler return is
invented. The resumed original worker reaches NtSetInformationFile, ordinal226;
its153-instruction original path returns normally. The subsequent NtReadFile,
ordinal219, executes902 kernel instructions and341 stores, then stops before
OUT8002400D, port01F6, byteA0. No port output, device input, IRQ delivery or
successful I/O result is supplied. This does not establish native dispatch or a
complete independent firing call.

The actual read requests47616 bytes at offset79681536, using observed file
handle64 and buffer82953000. The original full-call exit buffer matches that
exact range of owned assets/disc/maps/shared.map. Its complete file SHA256 is
084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af;
the range SHA256 is3aa31d135f406b2197b5049d59be3b843b5183b85b36e093ec652273ce05405b.
The raw buffer changes47383 bytes from the independently reconstructed request
entry. Two identical ten-word task candidates are observed; their content alone
does not identify the originating queue node or original handle registration.

The current production source_async_jobs.cpp/source_bitmap_async_read.cpp read
owner is freshly compiled in disposable scratch, with its headers authenticated.
It receives the observed ten-word task, original entry buffer, an explicit
shared.map file binding and a declared chunk limit equal to the observed
request size. Native execution receives no expected output bytes or original
kernel result. Its real file seek/read produces all47616 original exit-buffer
bytes exactly. Its ten-word task update also matches one original full-call
exit observation: cursor79729152 and processed47616. The second original task
copy remains unchanged, and is retained as a separate observation. This proves
the bounded read/update with declared procedure bindings; it does not install
native queue/worker/device ownership or establish the original I/O timeline.

These runs reuse retained RAM and original assets. No new large reference
inputs are retained. Compiler products, raw buffers and all known41 scratch
paths are removed, including failure runs; the checkpoint publisher's scratch
is checked separately after exit. Exact duplicate immutable evidence copies
share storage while retaining every historical pathname and hash. The small
primary ABI declarations, commands, failures and source snapshots remain.
Matrixv43 preserves63 paths,1110 occurrences,38 maps and630 gates, with zero
fully certified weapons. Real native dispatch/worker binding, resource
allocation/readiness, device/IRQ completion, full firing sequences and the
remaining visual/audio scope remain open. See original-weapon-io-progress-v1.json,
original-weapon-cpu-paging-equivalence-v1/report.json,
smg-original-file-read-native-v2/report.json and
post-original-io-storage-audit-v1.json.

## Original sound worker and production cache-read boundary (October18)

The original SMG request now has a verified queue identity: node4ECE38,
job156808, task4ECE40, callback1AAA80 and actual chunk limit40000. Its free/work
mutex slots and semaphore slot contain4,8 and16. Original entry RAM plus the
checked producer prefix establishes the queued input before the real semaphore
notification; the consumer prefix identifies the current node directly. This
does not infer queue identity from matching task contents.

The current native worker runs that input on a real native thread with actual
mutexes, semaphore acquisition, file seek/read, done publication, unlink and
free-list return. Original source handle tokens are explicitly bound to actual
native synchronization objects. The native synchronization initialization is a
declared adaptation, not a copy of original kernel objects or scheduling state.
Native teardown requests stop and joins before borrowed bank/file lifetimes end.
Its9632-byte queue bank, current-node slot, complete40-byte task,47616-byte
resource buffer and caller completion byte match the actual original full-call
exit exactly. Eight source stores are recorded with authenticated TU1.5 site
correspondence. Constructor priority, Xbox TLS/scheduler/device behavior and the
full firing call are not certified by this component.

The main-thread original instruction trace identifies the request caller as
21F4BD in the sound-cache load owner21F420. Its complete call/return trace has no
unmatched returns. This call is outside the existing texture/geometry/animation
native read handlers. The generated support owner deliberately dispatches1A3B70
to an external owner at that exact site.

The playable world's dispatch now includes source_sound_cache_io_tu15 for this
sound request. It checks the source return frame, sound chunk, occupied and
salted cache/page records, allocated destination, encoded resource offset,
rounded byte count, caller done address, category5 and priority4. The already
allocated source block receives real bytes from the world's bound resource
snapshots. An actual retained native completed-read receipt precedes the done
byte. Missing/failing I/O or receipt ownership cannot publish completion. This
uses the world's existing synchronous native resource policy; it does not install
the experimental thread composition into the world or claim original async
queue/job timing. Zero-size/unsupported resource requests remain unavailable.

A separately declared original prefix stops at the actual1A3B70 entry after
43849 instructions and7866 stores. It remains a failed full-call comparison;
only the declared entry boundary is reached and used as the input. The new
production I/O owner independently matches all47616 original resource bytes and
the original done byte. Nine invalid-call, missing-owner and failed-I/O controls
reject without completion. The focused CMake Release and ASAN/UBSAN checks pass.
The current live-world translation unit also compiles with its recorded build
flags. A complete application relink, installed-runtime firing capture, original
audio/PCM output and final frames remain unverified.

Compiler products, CMake builds, full RAM copies and working buffers are removed
after these checks. No new large reference inputs are retained. Small reports,
commands, failures and source snapshots remain; duplicate immutable source
copies share storage. Matrixv44 keeps63 weapon paths,1110 authored occurrences,
38 maps and630 gates with zero fully certified weapons. The mandatory cleanup
requirement remains in effect. See original-sound-worker-production-progress-v1.json,
smg-original-worker-native-v1/report.json, smg-native-sound-cache-io-v2/report.json,
smg-sound-cache-io-sanitizers-v2/report.json and
post-original-sound-worker-storage-audit-v1.json.

## Full application link and sound resource scope (October18)

A private Release build links the complete application and the normal source
weapon-cycle probe. All three selected CTests pass: original sound-cache I/O,
primary weapon cycle and original sound submission events. The actual linked
application also completes120 headless ticks with one and four local players.
The executable, local runtime dependencies and all build products are removed
after validation. This is finite headless integration, not input-driven rendering,
installed-runtime delivery, full firing parity or audible output.

The retained submission report reveals an unresolved admission boundary:
all18 observed sound requests returnFFFFFFFF while their driver is0. Observing
those request boundaries does not establish driver acceptance, exercise the new
sound-cache I/O handler in that world history, or produce PCM. The bounded I/O
owner retains its separate actual-original-entry comparison. The whole checkout
also changes during compilation because unrelated frontend, launcher, model and
HUD work is active. Before/after hashes and source observations record this;
the build is not described as one immutable source snapshot.

A read-only audit covers all347922 nonempty sound chunk records across the38
scoped maps,73842 unique file ranges. Every raw and512-byte rounded request fits
its bound file; no zero sizes, unaligned offsets or requests over the native
size limit occur. This covers literal gestalt metadata, not weapon dependency
closure, permutation selection, runtime admission, decoding or playback. See
sound-cache-sector-scope-v1/report.json,
weapon-current-runtime-and-sound-sector-progress-v1.json and
checkpoint-weapon-runtime-20261018.json.

## Original sound metadata query (October18)

Native execution from the unchanged captured SMG1030F0 entry exposes a missing
129833->1296E0 query. Independent original execution reaches that exact entry
after34777 instructions and6767 stores, without a supplied game-service result.
This is a separately declared prefix; its full-call comparison remains failed.
The complete1296E0 CFG contains27 instructions and no source writes or external
calls. A directly compiled port now retains those instructions and the actual
caller ABI. The playable world routes the original query to this owner.

The original captured input and48 explicitly modified branch inputs produce
identical GP values, defined flags, instruction traces, XMM state, x87 status,
MXCSR and nonreserved control bits in original/native execution. Together they
execute all27 original instructions. The initial strict comparison remains as
failed evidence: native FLDCW normalizes reserved x87 control bit6, producing
027F instead of the independent emulator's023F. The existing real host
FLDCW/FNSTCW probe documents that exact normalization; the corrected comparison
preserves both raw values and compares every other control bit. No production
arithmetic or original input is adjusted to remove that representation difference.

Release and ASAN/UBSAN component CTests pass; the current world translation unit
compiles with the added dispatcher. A diagnostic using existing build-multiplayer
archives plus freshly compiled query and I/O owners advances to the next missing
129846->127B20 function. It does not finish firing, reach the I/O owner in that
composition, or certify the current whole application. The complete application
build above predates this additional query port; relink/runtime validation of
the updated composition remains required.

All owned scratch for passing and failed checks is removed. No new large
artifacts are retained; original reference inputs remain preserved. Matrixv46
keeps63 paths,1110 occurrences,38 maps and630 gates, with zero fully certified
weapons. See sound-query-original-native-progress-v1.json,
smg-sound-query-differential-v2/report.json,
checkpoint-sound-query-20261018.json and
post-sound-query-storage-audit-v1.json.

## Original scalar sound query and class getter (October18)

The next original firing call,129846->127B20, selects a scalar from the actual
request, sound parameter record or sound class. Its complete23-instruction body
calls the complete17-instruction228220 class getter. Both bodies now compile
directly from the unchanged authenticated TU1.5 instructions. The world admits
the127B20 root; the class getter remains inside that composition, with existing
other class-getter routing preserved.

Independent original execution reaches127B20 after34802 instructions and6768
stores. This deliberately stopped prefix remains a failed full-firing comparison.
The new native composition matches31 cases: the unchanged actual input plus30
explicit byte controls. All40 original instructions execute across those cases.
Every GP/XMM value, defined arithmetic flag, ordered source write and instruction
trace matches. The controls include signed class/parameter indices, positive and
negative zero, infinity, quiet and signaling NaN payloads. They do not represent
additional authored gameplay observations. The actual absent-context branch
faults at228239 in both executions; no successful scalar or context is supplied.
Raw undefined flag bits and the previously measured reserved x87 control-bit
normalization remain visible in the comparison evidence.

All three sound component CTests pass in both Release and ASAN/UBSAN builds:
scalar/class query, cache metadata query and actual cache-resource I/O. The
current world translation unit also compiles with the new dispatcher. The
existing-library firing composition advances to12985C->12A810. At that boundary
its reported GP, XMM and defined arithmetic flags equal independent original
execution; raw x87 status differs by0020 and raw control by0040. These differences
are retained, not masked into a whole-CPU pass. Native trace observations also
include canonical damage-module PCs that need a qualified address mapping.
Neither complete firing nor audio admission/playback is established.

A fresh full application build with all three sound components and the primary
weapon/sound-submission checks is live at checkpointv47. Its specific CMake
process, execution sessions, private build and two waiting follow-ups are recorded
in checkpoint-sound-scalar-20261018.json. The follow-ups use the newly linked
application and libraries; their temporary executable/link-input/RAM copies are
owned by cleanup wrappers. No kept-artifacts option is used. This running work
is not reported as passed or deleted. All eight completed validation scratch
paths and the checkpoint publisher's scratch are removed; no new large evidence
artifacts are retained. Matrixv47 preserves63 paths,1110 rows,38 maps and630
gates, with zero fully certified weapons. See
sound-scalar-original-native-progress-v1.json,
smg-sound-scalar-differential-v1/report.json and
post-sound-scalar-storage-audit-v1.json.

## Original sound listener selection (October18)

The complete12A810 listener selector and its12A930,2287B0,253070 and2531E0
callees now compile from the authenticated TU1.5 image. This closed composition
contains312 original instructions and supplies no external service outcomes.
The world routes the measured12A810 root while preserving other existing owners.

Across55 cases,54 successful native executions match unchanged original i386
instructions executed on the host CPU: every GP/XMM value, defined arithmetic
flag, raw x87 status/control, MXCSR, instruction trace and ordered source write.
All312 instructions execute. The actual absent uncached-pointer fault is also
preserved independently in the native and emulator adapters. Declared listener,
distance, range and pair-byte controls include a separately identified pointer
rebinding fixture; they are not additional captured gameplay or resource binding.
Nine raw emulator precision-flag disagreements remain in a failed comparison
report. Original host execution agrees with the native status in those cases;
the emulator failure is not relabeled as a pass.

All four focused sound CTests pass in Release and ASAN/UBSAN, and the current
world translation unit compiles. The existing-library firing diagnostic advances
to129888->127CF0 after35015 native instruction observations. Its GP, XMM, flags
and MXCSR equal an independent35029-instruction original prefix at that boundary.
Raw x87 status/control and canonical damage observer PC differences remain
explicit; this is not a full firing or whole-CPU comparison.

The previous full application build terminated with unresolved
MultiplayerRuntimeAuthority definitions. Its failure log is preserved, and its
build plus both waiting follow-ups have been removed. Current CMake source
registration has since changed. A fresh application build including listener
selection is live at checkpointv48, with actual process/session identities in
checkpoint-sound-listener-20261018.json. It is not yet a successful link or runtime
proof. Completed validation scratch and the checkpoint publisher's scratch are
removed. No new large binaries, raw RAM or captures are retained.

Matrixv48 retains63 paths,1110 rows,38 maps and630 gates, with zero fully
certified weapons. See sound-listener-original-native-progress-v1.json,
smg-sound-listener-production-v1/report.json,
current-root-sound-metadata-runtime-build-v1/terminal-cleanup-observation.json
and post-sound-listener-storage-audit-v1.json. The next sound-gain composition
is under separate investigation and has not been admitted to production.

## Original gain interpolation and RNG scalar (October18)

The127CF0 gain composition and127D70 random scalar are now wired into the
world. Eight complete original bodies contain386 instructions. Actual host CPU
execution verifies127 successful cases and five real unmapped-read faults.
The gain composition exercises all353 instructions, including three faulting
instructions. The random scalar exercises32 of33: SHR ECX,16 bounds ECX to65535,
then TEST clears OF and leaves SF zero, so JGE always bypasses127DAA. That
original instruction remains in the port; no input or branch is fabricated.
The RNG update writes the original retained bank and uses its actual seed.

The independent emulator's failed raw-state comparisons remain preserved.
In particular, a faulting ADD reports different flags there; the actual host CPU
fault context agrees with the native state. Successful calls match original
host GP/XMM values, defined flags, raw x87 status/control, MXCSR, traces and
ordered writes. Faults compare the last retired instruction's defined flags.
Two declared readonly-data controls exercise redundant gain clamps; they are
diagnostic input overlays and are never supplied to production.

All six sound component CTests pass in Release and ASAN/UBSAN. The current world
translation unit also compiles, and its source hashes are rechecked after the
checkpoint. The native existing-library composition reaches128B9A->2290C0 after
35622 instruction observations; GP, XMM, flags and MXCSR equal the independent
35636-instruction original prefix. This is still a failed full firing call,
with raw FP-status and canonical observer-PC differences recorded.

The previous listener application build completed successfully, passed all six
selected checks and ran120 ticks with both one and four local players. Fifteen
source files changed across that shared-checkout build; its before/after hashes
are retained rather than claiming an immutable snapshot. That build predates
these two additional ports, which still require a full application relink.
All18 retained sound submissions have driver0 and resultFFFFFFFF; successful
submission checks do not establish admission or PCM playback.

All completed build, probe and application scratch is removed. Small reports,
commands and source snapshots remain; no new large binaries, raw RAM or frames
are retained. Matrixv49 preserves63 paths,1110 rows,38 maps and630 gates, with
zero fully certified weapons. See sound-parameters-original-native-progress-v1.json,
checkpoint-sound-parameters-20261018.json and
post-sound-parameters-storage-audit-v1.json. The next2290C0 composition has been
recovered and syntax checked separately; its behavior is not yet verified.

## Original sound properties and direct distance call (October18)

The2290C0 sound-property composition is now wired into the world. Its five
complete bodies contain189 instructions, including the original18FF57->21FEB0
tail call. Sixty-one cases match unchanged original host execution across188
instructions: GP/XMM values, defined flags, raw FP state, traces and ordered
writes all agree. The remaining229110 count-recheck guard stays in the port.
It is not forced with an invented asynchronous memory change. Controlled tag,
extra-info and context records reuse retained stack pages and are never supplied
as production data. Reference entry state comes from uninterrupted original
execution starting at the retained live SMG call.

The next caller,128BD3->12A930, uses the already verified distance body directly.
The world now admits that measured entry as well as12A810. Original replay can
select this later invocation by its actual stack return address; it changes no
instruction, return value or input bank. Firing then advances to128C21->127E00:
35846 native instruction observations versus a35860-instruction original prefix,
with equal GP, XMM, flags and MXCSR. Raw FP and canonical observer PC differences
remain explicit. Neither execution is a verified full firing call.

All seven sound component CTests pass in Release and ASAN/UBSAN. Current world
syntax and source hashes pass, and completed build/probe scratch plus publisher
scratch are removed. No new large artifacts are retained. A full application
relink with the gain, random-scalar and property ports remains required; the
previous passing application build predates them. Sound admission, PCM playback,
rendered output and whole-weapon certification remain open.

Matrixv50 retains63 paths,1110 rows,38 maps and630 gates, with zero fully
certified weapons. See sound-properties-original-native-progress-v1.json,
checkpoint-sound-properties-20261018.json and
post-sound-properties-storage-audit-v1.json. The next127E00 body is recovered
separately; it returns a live x87 value and still needs behavioral validation.

## Original live x87 sound result (October18)

The complete34-instruction127E00 signed-short interpolation is now wired into
the world. Thirty-four cases match unchanged original host execution, including
the exact80-bit live ST(0) result, stack depth/TOP, raw x87 status/control, MXCSR,
GP/XMM values, defined flags, traces and ordered writes. Controls cover24/53/64-bit
precision, four rounding modes, signed bounds, RNG edge seeds and empty-stack
status/TOP values. The actual caller supplies the RNG address. No substitute
seed or float32 return is introduced. Thirty-three original instructions execute;
SHR EAX,16 followed by TEST makes JGE bypass127E4B. That instruction remains in
the generated port. Raw emulator precision/C1 disagreements remain failed
comparisons; original host execution agrees with native state.

All eight focused sound CTests pass in Release and ASAN/UBSAN. Current world
syntax is checked again after a separate build-partition change in the shared
CMake file. Existing-library firing reaches128DCF->21F960 after36011 native
observations, versus a36025-instruction original prefix, with equal GP/XMM,
flags and MXCSR. Raw FP and canonical observer PC differences remain open.

A fresh application build containing all four additional sound ports is live at
checkpointv51. Its exact CMake process, private build and waiting application/
firing follow-ups are recorded in checkpoint-sound-random-short-20261018.json.
The firing adapter is a retained source snapshot. All completed validation scratch
and publisher scratch are removed; only this active validation is preserved.
No new large binaries, raw banks or frames are retained as evidence.

Matrixv51 preserves63 paths,1110 rows,38 maps and630 gates, with zero fully
certified weapons. See sound-random-short-original-native-progress-v1.json and
post-sound-random-short-storage-audit-v1.json. The next21F960 range-selection
candidate matches68 original host cases across all106 instructions; it has not
yet been admitted to production.

## Original range, permutation and chunk binding (October18)

The21F960 range selector,21FA80 permutation composition and12B1C0 source binding
are now wired into the world. Their eleven complete bodies contain527 original
instructions. Actual host execution verifies151 successful cases and six real
unmapped-read faults across526 instructions. The remaining21FC5A instruction is
bypassed by the source SHR/TEST/JGE invariant and stays in the generated port.
Preferred/nearest interval selection, cached choices, persistent used masks,
weighted retries, forced choices, the original count32 shift behavior and chunk
reference-byte updates match unchanged original execution. Controlled inputs
remain separate from captured gameplay; no replacement records are supplied
to production.

All eleven focused sound CTests pass in Release and ASAN/UBSAN. Current world
syntax and source hashes pass. The previous full application build, including
the live x87 return, passed ten selected checks and completed120 ticks with both
one and four local players. Nine source inventory changes across the shared
build are recorded. Its build and both application/firing follow-ups are removed.
That build predates these three new ports, which still require a full relink.
All18 sound submissions remain unadmitted; no PCM or rendered-output proof is
claimed from these checks.

The full38-map metadata audit finds a maximum of32 permutations and12 referenced
count32 definitions, with no invalid sound/range/class references. Those12
definitions are marine_shortmove sounds; this is a general sound-domain audit,
not a weapon dependency-closure claim. Existing-library firing now reaches
128E5D->128970 after36388 native observations, versus a36402-instruction original
prefix, with equal GP/XMM, flags and MXCSR. Full firing and raw FP/observer-PC
parity remain open.

All completed validation scratch and publisher scratch are removed. Small
reports, commands, hashes and source evidence remain; no new large artifacts
are retained. Matrixv52 preserves63 paths,1110 rows,38 maps and630 gates, with
zero fully certified weapons. See sound-selection-original-native-progress-v1.json,
checkpoint-sound-selection-20261018.json and
post-sound-selection-storage-audit-v1.json. The next128970 body and its
21F260/1293F0 callees are recovered separately and remain unported.

## Original chunk readiness and captured firing return (October18)

Original128970 readiness,1293F0 source pinning,21F260 cache-state selection and
21F3B0 cache-entry retention are now generated from authenticated TU1.5 bytes.
The four bodies contain187 instructions. Unchanged original host execution
matches114 cases across180 instructions:101 completed calls and13 explicit
allocation/scheduling stops. The seven unexecuted instructions follow those
external callees; no replacement return is supplied to force their continuation.
Release and ASAN/UBSAN checks pass. World dispatch uses the existing allocator
and actual native resource-read owner. Its existing16E940 handle-lookup owner
is also included in the firing diagnostic's dispatch.

The existing-library replay now returns from the captured original1030F0 SMG
firing call after51698 native instruction observations. All three retained
spans, totaling40594 bytes of unit, weapon and complete first-person state,
match the original capture. GP/XMM, flags and MXCSR also match. Raw x87 control
still differs by reserved bit0x40 and status by precision flag0x20; neither is
hidden by this bounded result. The captured plasma charge-start call also
returns with matching owned spans. These are finite captured calls, not a
complete firing/reload/playthrough or rendered-output certification.

The SMG continuation performs the actual47616-byte sound-resource read and
records the native completed receipt. This uses the declared synchronous native
policy; it does not certify original Xbox queue timing or request identity.
The independent original allocator-entry prefix matches GP/XMM, flags and
MXCSR before this resource adaptation. Its raw FP and canonical observer-PC
differences remain recorded. The fresh full application relink, all six selected
checks and finite120-tick runs with one and four players pass. The fresh-library
firing replay also returns with the same40594 owned bytes and captured GP/XMM,
flags and MXCSR equal. Its complete47616-byte resource buffer matches both the
original worker buffer and the authored shared.map range. SHA-256 proof is
retained without another raw buffer copy. The selected source inventory records
13 changes during the shared build; finalized port hashes remain equal.

The passing firing replay starts from retained original initialized banks.
Native live-world sound storage still stops before original224EC0 device
activation, leaving its constructor's unactivated state and no driver. That
420-instruction activation body is recovered, with its real SDK calls retained
as the next audio boundary. Passing storage, submission-observation or captured
firing checks does not establish live sound admission, DSP/PCM output or full
weapon parity.

All23 completed private validation directories and publisher scratch are removed.
Small evidence remains in sound-chunk-readiness-original-native-progress-v1.json,
fresh-native-sound-read-buffer-proof-v1.json and
post-sound-chunk-readiness-storage-audit-v1.json. Matrixv53 preserves63 paths,
1110 authored rows,38 maps and630 gates, with zero fully certified weapons.

## Original host firing prefix and current first-person update (October18)

Unchanged original1030F0 bytes now execute on the host through the real
21F3B0 allocator boundary. The current native prefix matches all36550 instruction
observations, all7151 ordered writes, GP/XMM, defined flags, raw x87 control and
status, empty x87 stack and MXCSR. Seven explicit damage-owner observer ranges
use independently authenticated correspondence across90 complete routines and
8976 static instruction sites. Other observers keep their actual PCs; source
arithmetic and public observer records are unchanged. The new diagnostic stop
does not supply a callee return, and its full-call result remains failing.
Default complete SMG firing still returns with40594 captured owned bytes equal.

The previously observed14 extra emulator steps and two extra writes come from
the CRT control-word guards at32CCFC and332133. The original host and native
control word is027F; the independent emulator retains023F and takes the extra
save/restore paths. Canonical observer renaming changes addresses, not instruction
counts. The raw precision-status difference also matches unchanged original host
execution. The initial failed emulator-trace assumption remains retained beside
the stronger host comparison; native math was not adjusted to fit it. This is
source-execution evidence on the host, with no physical Xbox certification.

The current native168CA1 first-person firing update returns and matches the
retained unchanged original host across27216 observations,21748 ordered write
bytes,148 actual host estimate lanes and its complete33584-byte state. The
original ammunition watchpoint records the Needler's30-to29 decrement, and the
original map identifies its17-node weapon model and37-node Master Chief model.
The emulator-captured state still differs by604 bytes. That discrepancy remains
explicit; no rendered framebuffer, physical Xbox, live audio activation or
DSP/PCM parity is claimed from these CPU comparisons.

All recorded private probe directories and publisher scratch are removed. No new
large binary, RAM bank or capture is retained. Small reports, traces, source hashes
and correspondence remain in original-host-weapon-validation-progress-v1.json,
checkpoint-original-host-weapon-validation-20261018.json and
post-original-host-weapon-storage-audit-v1.json. Matrixv54 retains all63 weapons,
1110 authored rows,38 maps and630 gates, with zero fully certified weapons.


## Original weapon material inputs and plasma-rifle gauge (October18)

The live held/body and world weapon material callbacks now resolve original
one/zero builtins and authored object functions through actual bound weapon
inputs. Enabled zero, age gates, authored curves and optional primary-trigger
state retain their source ownership; an unavailable producer stays unresolved.
Whole unchanged TU1.5 handlers substantiate two builtins and25 actual authored
heat/ammunition cases. Nine additional native cases retain actual heat gates.

The38-map metadata audit finds192 authored one references across five weapon
paths. The concrete Lockout plasma-rifle gauge comparison executes1789 original
instructions through the unchanged original shader consumers and callback.
All188 shader words match native output; the previous callback differs at
word19,9b6666ff instead of original/native9d6666ff. Its test prepares the actual
pass with the runtime explicit zero camouflage value. The initial test omitted
that required context and failed; its diagnostics remain retained.
The188 unchanged-original words are also retained as a small mandatory native
regression fixture, so matching only the previous callback is insufficient.

Release application validation, all six selected checks, finite120-tick one/four
player simulations and four sanitizer checks pass from one disposable coherent
source snapshot. A separate diagnostic serialization build uses the same copied
headers/libraries and records the complete gauge banks. All recorded private
build/probe directories are removed; no new large artifact is retained. The
focused actual SDL pickup/held-gauge probe passes on a private X11 display,
observing the corrected one input on live plasma-rifle gauge materials. Its
fixture delivers placement through original B95D0, rather than changing only
the simulation position overwritten by source physics readback.

The separate complete firing probe picks up the actual weapon at tick42 and
submits two attack poses, but fails its combined shot/sound observation gate.
No published shot_tick transition is observed after the real SDL fire input.
That failure remains preserved and unresolved; the held-gauge result does not
certify firing or sound. Source readback also retains the legacy last_shot
publication field; do not infer actual shot count solely from that field.

Metadata references do not prove all those model variants render correctly.
Original callback registration, framebuffer, physical Xbox and full audio
acceptance remain open. Matrixv55 retains all63 weapons and630 gates, with zero
fully certified weapons. See weapon-material-input-original-native-progress-v1.json,
checkpoint-weapon-material-input-20261018.json and
post-weapon-material-input-storage-audit-v1.json in the goal analysis folder.

## Source-authoritative shot publication, October18

The source equipment readback now copies the literal preparation tick at weapon
offset0x248 into the existing primary/offhand shot publication. OriginalTU1.5
0x108607 stores the game clock tick loaded through0x51DF54; no tick adjustment
is applied. The previous SDL plasma-rifle traces contained four actual source
preparations in each placement mode, with52 sampled publication mismatches.
After the correction, all64 sampled publications match the actual source banks.
Both continued source placement and natural movement after pickup were checked.

The four focused material/heat/charge checks pass. The two complete firing
checks remain failed: four published shots each produce zero observed firing
sounds. This validates timestamp readback only. Device activation, DSP/output,
projectiles, complete rendered sequences and the full weapon scope remain open.
The evidence is in `checkpoint-weapon-shot-publication-20261018.json`; private
build/source trees are removed and only small reports and diagnostics remain.

## Source plasma-bolt creation observations, October18

Read-only entry/return observations of original B9B70 confirm four successful
plasma-bolt constructions in each of three live SDL trials. The original
614-instruction body correspondence and every retained listing byte were
reverified against the authenticated original images. Each returned object has
its actual full generation, projectile type, definition, unattached parent and
initial velocity. Continued placement and natural post-pickup movement agree.
Empty tick snapshots therefore do not prove failed source projectile creation.

The third trial additionally observes original BA490 retirement requests and
retains the existing source contact diagnostics. All four bolts complete two
source contact effects about0.47 units from their birth position and retire
in the same source tick through original B92B5->BA490. This scene's empty
projectile snapshots are explained by those contacts. The next flight trial
must use a clear firing lane. Exact callers and raw state are in
`weapon-projectile-retirement-analysis-v2.json`. Complete flight/contact/
retirement and original-render acceptance remain open; zero firing sounds still
fail the complete firing checks. No generated binary, source copy or raw bank
is retained. See `checkpoint-weapon-projectile-evidence-20261018.json`.

## Source plasma-bolt flight in a clear authored scene, October18

The actual SDL pickup/fire fixture now supports an authored spawn selected by
host clearance rays. Those rays only choose scene inputs; original B95D0 owns
placement and original projectile routines still own all movement and contacts.
All four constructed bolt handles are observed in sustained flight, with
monotonic travel read directly from their actual source banks. Each also submits
its authored glow with valid vertices into the real gameplay viewport. All four
complete13 source ticks of flight, contact effects and retirement in this scene. Exact
counts, selected spawn, source hashes and failure diagnostics are preserved in
`checkpoint-weapon-clear-flight-20261018.json`.

The four focused material/heat/charge checks pass. The complete firing check
still fails on zero firing sounds. This sequence does not yet compare its glow
inputs with independent original execution or certify an Xbox framebuffer,
complete projectile lifecycle, whole weapon or other map variants. Temporary
build/source trees are removed; no new large reference inputs are retained.

## Original comparison of actual plasma-bolt glow inputs, October19

All52 actual single-view gameplay glow records, over four complete source
object generations, match the unchanged TU1.5 sprite/projector consumers:
3744 XYZW/RGBA/UV vertices and every bitmap range are bit-identical. Retail
and TU1.1 consumers independently agree under the declared same-input ABI.
The bounded retained input JSON is222739 bytes and reproduces the
original comparison report byte-for-byte after the private build is removed.
See `checkpoint-weapon-live-glow-original-20261019.json`.

The full firing test still fails on zero firing sounds and retains its original
nonzero verdict. Hardware entry/submission boundaries are explicit in the
original report. Framebuffer, complete producer/lifecycle, other views/species/
variants and whole-weapon acceptance remain open. Raw captures, binaries and
private source/build trees are removed; the small required input fixture and
comparison/failure reports are retained.

## Source species and split-screen glow comparison, October19

The five new cohorts cover two/four Spartan views and one/two/four Elite views.
Together with the preceding single Spartan view, both species cover all three
local-view layouts. Each verifies actual source biped tags against the selected authored
representation and observes all four source bolts in every gameplay view.
All648 captured glow records and
46656 vertices per original image, including
colors/UVs/bitmap ranges, match unchanged TU1.5 consumers exactly. The retained
four-view fixture independently reproduces the original report after build
cleanup. See `checkpoint-weapon-glow-species-view-20261019.json`.

The first batch preserved two duplicate-keyboard fixture failures and a parent
report-directory label error. The corrected fixture uses the existing standalone
input assignment with one keyboard owner and neutral absent controllers.
The four focused checks pass; all complete firing tests still fail on
zero firing sounds. Framebuffer, complete producer/lifecycle
and remaining weapon/variant acceptance remain open. Only bounded required
input fixtures and small reports remain; private binaries/raw captures/source
copies/build trees are removed.

## Live dual-SMG histories and HUD storage, October19

Both actual source species now complete the SDL map pickup, carried switch,
dual pickup, independent trigger fire, reload, grenade throw and offhand-drop
sequence. All745 hand identity/ammunition/preparation-tick publications match
their original source banks. The two datums submit616 animated weapon poses
across seven clips per species. See `checkpoint-weapon-dual-source-history-20261019.json`.

The first run in each species stopped at original25AA73 reading NULL+82.
The unchanged original reproduces that exact fault independently. Actual
25A71C/1976CF allocation and1976FE new-map initialization now own the missing
50EECC HUD storage;92 new original instructions retain the complete map reset.
The existing63 routine bodies are unchanged after identity-address normalization.
The corrected live runs retain a stable nonzero HUD binding through drop.

The four focused checks pass. Both complete tests remain failed on seven
rounds and zero authored firing-sound submissions each. Full framebuffer/audio,
all maps/weapon pairs, interruption and drop-world lifetime acceptance remain
open. Private builds/source/scratch are removed; small reports retain the
original failures and corrected observations. No large new captures or binaries
are retained. Full certification remains zero of63 weapons.

## Detached weapon resource readback, October19

The real drop sequence redistributes reserves from116 while held to53 in the
detached original object. The native world item retained its former held state.
Held and loose weapons now share the same source-bank readback for both barrels,
triggers, magazines, preparation clocks and shader fields. Both species match
all2130 detached identity/inventory/root/ammunition publications, including1024
neutral headless ticks each. Both objects remain live in that window; no cleanup
timer or complete retirement policy is inferred. See
`checkpoint-weapon-dual-drop-lifetime-20261019.json`.

Completed source node banks differ from the local pickup idle sampler in these
scenes. This is a completed-tick matrix comparison, not an interpolated GPU or
framebuffer proof. Loose-weapon source pose publication/rendering and LAN pose
ownership are the next required visual work. The fresh application passes one-
and four-player finite checks; four focused checks pass. Both complete live
tests still fail on seven rounds and zero firing sounds. Private source/build/
scratch trees are removed, including the cancelled fixture setup; only small
reports and source-node evidence remain. Full certification stays zero of63.

## Loose weapon original poses and native transport, October19

Loose weapons now publish original WORLD nodes, participate in native LAN pose
ownership, and use original accepted geometry, LOD/regions, rendered nodes and
cached lighting in local source-owned views. Attach/drop transitions cut the
pose interpolation across inventory ownership changes. Native LAN protocol90
rejects older decoders that cannot admit loose owners. Both species match the
completed-tick nodes and original selected-region palette matrices in actual
dual-SMG scenarios. See `checkpoint-weapon-loose-source-pose-20261019.json` for
exact counts, source hashes and retained failures.

The first Elite LAN attempts exposed an existing explosion-location decoder
restriction to scale1. Original17BA50 preserves all52 WORLD transform bytes;
the live Elite scene contains scale1.5. The decoder now retains finite source
scale using the existing firing-location bound. Both species admit with zero
packet rejections, and every observed source explosion transform survives
byte-for-byte. All five focused checks and both finite application checks pass.
Both full live tests still fail on seven rounds and zero firing sounds each.
Endpoint matrices do not certify intermediate interpolation, split views,
composed LAN LOD/lighting or an Xbox framebuffer.

The all-map metadata audit also finds94 of1110 authored rows with world models
excluded by the current first-person-record filter, spanning27 weapon paths.
Unchanged original3D3D0 returns one world model for all667 authored model
chains, including those94 rows, without reading a first-person bank. The443
model-NONE rows are excluded before that original call. These campaign variants
still require actual owner/render checks; a model count does not authorize
native pickup or equip behavior. Keep the complete63-path/630-gate scope open.
All validation source/build/scratch trees are removed after terminal runners;
only small reports remain. Full weapon certification stays zero.

## Campaign source weapon units, October19

The original campaign collector separately builds actor-held weapon units, but
the old drawing loop submitted actor bodies and native pickup previews. The new
campaign weapon scene prepares authored world resources independently of FP or
pickup eligibility, and submits only completed original main units. It retains
each unit's section order/LOD, completed WORLD skin, packed colors and cached
lighting; shader state comes from that actual weapon's source fields and
magazines. Unknown producers and resource owners remain explicit boundaries.
Overrides stay separate. BSP material rebases and restored/discontinuous pose
histories reset without advancing simulation.

The actual Cairo ai_place scenario now submits its original attached weapon.
It is the authored h_turret_mp_item, whose FP record bank is empty; this is an
actual source-owned world-only positive, not a synthetic pickup eligibility case.
Geometry matches independently decoded authored sections skinned by the source
WORLD bank, material identities/LODs match, cached lighting words match, and
three actual source magazine/builtin shader inputs pass per submitted unit.
Source RNG is unchanged; absent original admission emits no stale draw. Exact
counts and source hashes are in
`checkpoint-weapon-campaign-source-submission-20261019.json`. Four focused and
two finite application checks pass, together with the armed CPU scenario.
The application checks here are multiplayer headless checks; composed campaign
GPU submission, final pixels, other map/owner variants and full audio remain open.

The earlier94-row result describes the multiplayer body_guns filter, not94
proved missing campaign pixel cases. Campaign selects its own FP record; the
new world scene has no FP residency prerequisite. Keep original owner eligibility
and full visual acceptance separate. Temporary source/build/probe outputs are
removed after terminal runners; small diagnostics and evidence remain. Full
certification is still zero of63 weapons.

## Actual campaign application frontier, October19

Two freshly built actual Cairo application runs place the original Johnson
squad and initialize the RTX5080, then fail before their first frame on a
scenery lighting-owner guard. Seven companion checks pass in each attempt;
zero weapon inputs or Vulkan weapon commands submit. A read-only per-object
vkCmdDraw observer is implemented, but has no positive application case yet.
The smaller model-only probe does not reproduce this failure and stays red.

The full application identifies dress_cap F9D6179E, placement583, class1,
no override and a full range. Its original instance146 selects baked bucket4
with matching checksum336987148. The scene caches its model bank using only
definition/variant; the first matching placement152 has no BSP0 baked record,
so a dynamic bank is reused for the later baked placement. The actual source
packet and untouched cache records establish this ownership mismatch. See
`checkpoint-weapon-campaign-application-frontier-20261019.json`.

The correction still requires class1 material capability and original per-section
incident/radiance streams, selected per original unit/current BSP instead of
the first placement's lighting mode. The guard remains strict; scenery is not
omitted. GPU/framebuffer and full audio acceptance remain open, with zero of63
weapons fully certified. All owned builds/source/scratch are removed after
terminal runs; only small commands, hashes, reports and diagnostics remain.

## Per-instance campaign scenery lighting and first weapon GPU submission, October19

The shared campaign scene bank now prepares class1 and class2 shader capability
independently of the first placement's lighting mode, sharing immutable geometry.
Each actual original unit selects its own lighting class and packed scenery
reference against the current BSP. Class1 consumes its checksum-checked authored
section offsets, world-space incident directions and RGB vertex streams before
WORLD posing. Overrides, unsupported classes and partial ranges still reject.

The captured-camera CPU regression reaches the exact earlier dress-cap packet
at placement583/reference146 despite constructing its resources from placement152.
It verifies3978 baked vertices,87 dynamic vertices and12 rejection controls.
Authored emissive/transparent passes keep their independent implementations;
selected shader payloads are compared against independently prepared originals.
The smaller CPU collection has58 model units/20 particle commands, versus61/21
in the application, so complete scene-state equivalence is not claimed.

Eight companion checks pass. The actual Cairo application now submits frame0
with12 source weapon units and17 Vulkan weapon draws. The original held
world-only turret item submits834 vertices in two actual draws (438 and396),
with its full source identity, selected shaders, owner constants, pipeline,
descriptor and vertex-buffer bindings observed after vkCmdDraw. The required
four-frame application still fails on its next update at original root1CC8D0,
call32AB71 ->32ABD8. No return is supplied and this failure remains red.

See `checkpoint-weapon-campaign-scenery-lighting-20261019.json`. These command
observations do not prove visible samples, Xbox framebuffer parity or complete
audio. All63 weapons/630 gates remain in scope with zero fully certified.
Both disposable validation builds/source trees and application scratch are
verified removed; only small reports, hashes and diagnostics are retained.

## Full bounded Cairo application passes, October19

The shared original32A990 CRT copy entry now reaches its existing complete
Process owner instead of an incomplete private light-portal copy closure.
No instruction or return is substituted. All704 declared overlap/alignment/size
cases pass through the live world, comparing the complete512-byte operand
buffer and returned destination. Independent Unicorn execution of untouched
TU1.5 bytes passes the same704 cases, including36 executions of32ABD8, with
original return stack, callee-saved registers and direction flag checked.
The original comparison is bounded; it does not establish whole native ABI
identity. The earlier cold-constructor fixture failed before reaching copy;
that evidence remains separate from the passing explicit-reference case.

All10 checks pass in the fresh snapshot, including the scenery regression,
actual actor/weapon source submission and four actual Cairo application frames.
Each frame submits12 original source weapon units and17 Vulkan weapon draws.
Eight retained target turret-item commands across four frames have matching
full owner/shader identities and observed owner constants, pipeline, descriptor
and vertex-buffer bindings. The application exits0 in282.49sec without timeout.
These are functional observations under the retained validation compiler flags,
not a performance benchmark, visible-sample comparison or Xbox framebuffer proof.

See `checkpoint-weapon-campaign-copy-dispatch-20261019.json`. The current renderer
changed in another workspace task after the validated snapshot; that drift is
recorded, and unrelated edits remain intact. Full63-weapon/630-gate visual/audio,
map/owner variants and complete sequences remain open; zero weapons are fully
certified. All owned generated source/build/application/probe/publisher scratch
is verified removed, with no new large artifacts retained.

## Expanded campaign BR/SMG/turret world coverage, October19

The exact retained-camera CPU scene now independently verifies all12 admitted
weapon units/4104 vertices:1 attached turret item (834 vertices),4 loose battle
rifles (1464),7 loose SMGs (1806). Ordered source sections, completed WORLD
poses, authored part/shader LODs, cached lighting bytes and written original
color channels match. The application collector retains every weapon command
and identity instead of filtering its report to the turret item, and requires
all three definitions in each submitted frame of the existing four-frame check.

Nine companion checks pass. The fresh application submits only frame0 before
its300-second timeout. That frame retains12 weapon inputs and17 Vulkan draws:
2 turret-item,8 battle-rifle and7 SMG commands. Each owner/vertex count and
observed GPU shader/LOD pair agrees with its independent CPU scene result.
The full four-frame check remains failed; per-definition coverage is true only
for the one submitted frame. The previous four-frame checkpoint67 remains a
separate result against its earlier source snapshot. Current main.cpp changed
after this validation snapshot; its drift is recorded and preserved.

See `checkpoint-weapon-campaign-scene-weapons-20261019.json`. Startup/renderer/
update timing and shipping Release optimization are the next investigation;
no frame requirement is reduced. This is not matched Xbox framebuffer/audio
or complete weapon certification. All63 paths/630 gates remain open, with zero
fully certified. Four recorded source/build/application/publisher scratch paths
are removed and no new large artifacts are retained.

## Release four-frame BR/SMG/turret coverage, October19

The fresh optimized Release snapshot passes all10 checks. Its actual Cairo
application completes four frames in92.70sec, retaining48 original weapon
inputs and68 actual Vulkan weapon commands. Every frame contains all three
definitions:1 attached turret item,4 loose BRs and7 loose SMGs. The independent
CPU case still matches12 source units/4104 vertices, authored shader LODs,
WORLD nodes, cached lighting bytes and written original colors. First-frame
owner/vertex counts and GPU shader/LOD choices agree with that CPU selection.

Opt-in monotonic trace stages separate startup from frame work. Resources reach
renderer construction at14.73sec; renderer initialization takes69.52sec and
finishes at84.25sec. This functional run uses-O2 with the existing strict FP
options; it is not a controlled before/after performance or gameplay FPS
benchmark. The earlier-O0 timeout remains preserved as a separate failed run.

See `checkpoint-weapon-campaign-scene-weapons-release-20261019.json`. Current
renderer edits and separately prepared cold-HUD changes differ from the
validated snapshot and remain explicit. Original197741 writes the selected
HUD tag address to51DF94; the pinned retail loader supplies four zero bytes,
but native startup storage lacked that destination. The exporter now adds
only those four writable bytes, preserving every other generated byte. The
original routine still produces the real tag address. A cold-world/HUD/copy
regression is prepared but not yet run; no cold-HUD acceptance is claimed.

All63 paths/630 gates, original matched visual/audio and complete weapon/owner/
map sequences remain required; zero weapons are fully certified. All recorded
validation/probe/export/publisher scratch paths are removed and no new large
artifacts are retained.

## MCC native startup and remaining-work checklist, October19

The operational checklist now owns the resume order, all63 weapon entries,
ten gates per weapon, MCC-only runtime boundary and cleanup obligations. It is
linked from the goal contract, current state and matrix; it must be updated at
each verified milestone. All630 full gates remain incomplete (85 partial,
545 unverified); no weapon is certified.

The first native MCC Release build succeeds. Cairo's actual source-selected
HUD address and704 overlapping-copy cases pass without an Xbox executable
constructor or reference cache metadata. Both one/four-player MCC CLI simulation
checks pass. Four offline reference checks also pass; two read original ISO
metadata and are explicitly excluded from MCC acceptance and future snapshot
selection. Campaign ai_place fails at original1E7F18 because47D6C0 lacks native
backing. The dependent application has no source report and does not launch.
A fixture setup requirement now prevents that secondary missing-file attempt.

Pinned original code/data shows47D6C0 is the literalFFFFFFFF read by1E7F18 and
stored into actor+418 by1E7F1E. Native memory now declares exactly those four
bytes with the original writable section ownership. The second MCC Release
run clears that original frontier and next fails at1B0D0F reading descriptor
48AD60+18 (48AD78). Five checks pass; source submission fails and the dependent
application is correctly skipped. Authenticated original descriptor metadata
shows the real slot is zero, with normal source branch ownership; completing
its descriptor backing remains open. No tag pointer, actor result or return
value is supplied. See checkpoint-weapon-mcc-actor-operand-20261019.json.
All owned source/build/probe/publisher scratch is verified removed; no new
large artifacts are retained.

## MCC actor and held-weapon submission, October19

Complete original descriptor interfaces now supply4,632 immutable bytes and920
source-writable cache bytes for115 indexed records. The71 real CRT mask
constructors remain unchanged and own their separate destinations. Their213
instructions, ABI/stores, original metadata bytes and access controls pass both
an isolated native probe and the fresh MCC Release fixture. The initial
readonly-cache probe is superseded by the original write-ownership evidence.

Seven of eight checks pass. Actual MCC actor placement and held turret-item
CPU submission now pass:2,358 vertices, three shader inputs and seven model
main-unit rejection controls. The original lighting/section/WORLD and shader
selection comparisons remain required; this is not a framebuffer result.
The actual CLI application reaches public HS placement and fails at2098A6 on
an absent209D04 dispatch word before rendering. Source metadata and branches
are unchanged; all three original campaign-runtime tables (72 bytes) are now
declared, with56 unmodified-original selector/bounds cases and syntax passing.
Actual MCC application revalidation remains open. See
checkpoint-weapon-mcc-ai-descriptors-20261019.json and the operational checklist.

All63 weapon paths/630 full gates remain incomplete, with zero fully certified.
All recorded owned source/build/application/probe/publisher scratch is verified
removed; no new large artifacts are retained. Current root code may differ from
the hashed validation snapshot; final current-source and delivered-runtime
acceptance remain separate requirements.

## MCC campaign tables, scope decisions and remaining-work checklist, October19

The operational checklist is attached to the goal contract and current state.
It contains all 63 weapon entries, all ten aggregate gate names, MCC-only
runtime boundaries, the immediate validation order and ongoing/final cleanup.
The matrix still has 85 partial and 545 unverified gates, with zero complete
weapons. Preparation checkboxes do not certify weapon gates.

The v6 immutable MCC Release run builds and passes seven of eight checks:
actual actor/held-weapon source submission, descriptor composition, HUD/704
copies, both player-count CLI simulations and retained arithmetic comparisons.
The three original bounded campaign dispatch tables now let the real application
complete public HS actor placement. Cold composition independently verifies all
18 words and preserves the existing 24-byte arithmetic-table owner. The earlier
v5 overlap is retained as a failed run; its deleted private source is not
reconstructed from later shared-root edits.

The actual application then stops before rendering on objective locale loading.
The pause reader used decoded locale offsets to seek the compressed physical
MCC file. All five objective offsets and strings are valid in independently
decoded MCC data; all five physical reads fail. The consumer now uses the
ordinary MCC resource owner. Its syntax passes, and the production resource
reader matches all five strings and rejects two invalid extents. A full
application rerun is still required; no MCC frames or weapon GPU commands have
passed. See `mcc-pause-objective-locale-frontier-v1.json` and
`mcc-pause-objective-resource-preflight-v3.json`.

All 41 owned Xbox map/shared/menu directories have also been checked for the
three additional MCC paths. None is present; their metadata belongs to both MCC
shared banks, with 25 nonresident multiplayer aliases per path. Preserve those
MCC additions outside the original 63-weapon reference catalog, with their
MCC gameplay unverified. All new owned build/source/probe trees are removed;
small reports, commands, source/input hashes and diagnostics remain.

The following v7 MCC Release validation also passes seven of eight checks and
clears the objective-locale failure in the actual application. The RTX 5080
renderer initializes; frame-zero simulation finishes. Before the first frame
submits, original model-render root `23670` reaches `23644` and dereferences
address `0x2`. The authenticated `235E0` callee reads a byte from ECX+EDX, with
actual ECX2/EDX0. Its section/resource pointer chain and imported backing require
diagnosis; address2 must not receive invented static backing. The corrected
known-entry instruction evidence is
`mcc-model-render-23644-original-frontier-v2.json`; the earlier misaligned
23630 window is explicitly superseded. Source/HUD/descriptor and both MCC CLI
cases remain positive, with zero submitted application frames and zero full
weapon gates. The v7 private source/build and both instruction-probe trees
are removed. The checklist preserves this exact next action.

The v8 MCC Release run passes nine of ten checks. The typed node-map projection
preserves all 906 authored maps and five paired original headers. Complete
canonical delivery verifies 112,779 pointer fields and 69,887,237 owned bytes.
The actual unchanged publisher/render consumer clears `23644`; the application
then stops at original `22D78` on missing read-only input `46A680`. Its four
original bytes and 34 further fixed operand extents are restored. A fresh cold
factory verifies all 95 fixed read-only operands, including exact access and
ownership, without supplying arithmetic results. The separate 13 indexed read
sites still require bounded table evidence. The duplicate `48C754` declaration
now validates its earlier renderer-cache-pool owner and preserves writable
value1. Actual MCC application revalidation remains next, with four submitted
frames and full visual/audio gates still open. Checkpoint75 retains the v8
failure and preparation evidence; all completed owned scratch is removed and
no new large artifacts are retained.
