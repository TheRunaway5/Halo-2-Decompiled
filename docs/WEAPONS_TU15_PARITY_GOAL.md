# Xbox TU1.5 weapon parity goal

Status: **incomplete; work continuing**. The requested end state is feature and visual
parity for every weapon, including campaign/AI, vehicle/turret, objective and
otherwise unused authored variants. Passing a component test does not complete
a weapon. Original executable/disc data and verified original execution remain
the authority; unresolved owners are not replaced with tuning or artwork.

## Scope and completion requirements

`tools/audit_weapon_scope_tu15.py` currently finds **63 distinct weapon paths,
1,110 authored rows and 38 campaign/multiplayer maps**, including the owned
TU1.5 DLC maps. The complete per-map identities, root/trigger/magazine/barrel
bytes and original image/cache hashes are retained in
`analysis/weapons-tu15-goal-20261013/authored-scope.json`. Its per-weapon feature
and visual status remains unverified for all 63 definitions.

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

`research_weapon_trigger_tu15.py` maps nineteen complete original owner bodies
between TU1.1 and TU1.5, including the whole1441-instruction weapon update
1021D0/1030F0, input1045E0/105500, barrel admission and charge/lock transition
helpers. All2,449 instruction rows and internal branches match after relocation;
all retained absolute operand bytes also match. The original instruction
listings, mapped calls and separately unverified external callees are retained
in `trigger-correspondence/report.json`. This is static reference groundwork,
not new native charge/lock execution or full-world validation. Actual charge,
alternate-barrel, target and tracking owners remain required.

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
build. It also retains a failing original-weapon input regression: ordinary
primary fire currently leaves the source unit trigger neutral and its SMG at
60 rounds. The parallel native weapon adapter is not original-world weapon
ownership; composing that input/state/event path remains required.

Next coverage must include complete unit/own-model/aim-response arbitration
and charge/tracking/target-marker publication, extending the bounded
inactive/world clock and numeric HUD evidence above. Initial energy-weapon carry remains guarded in the
native match; the current energy suites use actual pickups after an SMG start.

The current full native build succeeds in `build-weapons-tu15-20261013`, with a coherent
source snapshot at `.local/weapons-tu15-source-20261013`. Original resources
remain local symlinks. Shared dirty edits are preserved. Live SDL/Vulkan
verification passes all **twelve current cases with zero Vulkan validation
errors** in `hud-runtime-suite-v1`, including shotgun firing and Needler
empty/reload after the ordering corrections. The probe uses a finite one-tick-per-frame
clock, explicit mid-tick presentation fraction, private X11 display and confirmed
SDL X11 driver. The first Needler shot is released to observe its full attack
clip before held fire empties the magazine and reloads. Held shots restart that
clip every four source ticks. Earlier shared-display and incorrectly headless
probes remain retained, including their failures. These are native regression
observations with diagnostic placement and dummy audio, not Xbox framebuffer
or audible-hardware parity. The weapon-specific launchers deliver this tested
progress build; combined current-checkout validation remains pending.

The separate finite SMG SDL probe in `hud-smg-flow-v1` also passes with
zero Vulkan validation errors: actual pickup/switch, independent triggers,
dual reload, 308 posed draws across four clips, seven authored firing PCM
submissions for seven consumed rounds, and grenade/offhand release after the
reported reload action clears. The current retained dual-state framebuffer and lossless PNG preview
are in `hud-smg-live-v1`; both reserve counters show233. This is a native visual inspection;
matching Xbox camera/lighting/material/HUD evidence is still required.

The forty-eight current focused CTest checks are retained in
`hud-cpu-tests-v2.log`; the actual
UDP presentation regression also passes with TU1.5 data, including independent
clocks and reordered/duplicated/dropped/late packets. The earlier shared-reserve
AddressSanitizer/UndefinedBehaviorSanitizer run passed its recorded source.
The current isolated runtime and regression build log is `hud-delivery-build-v2.log`;
`smg-dual-complete-effects-build.log` records the complete-effect test update.
`Halo2-Weapons-Launcher` and
`Halo2-Weapons-Title-Launcher` now select `build-weapons-tu15-20261013`, verified
with both `--launcher-info` outputs. Their previous binaries are preserved
under `launcher-delivery`; source paths, launcher/runtime hashes and selection
proof are recorded in its report. A fresh launcher-to-native eight-frame smoke
passes with zero Vulkan validation errors in `launcher-delivery/hud-xbox-smoke`.
This is delivery of the bounded progress
build, with full feature/visual parity still incomplete. The frozen source manifest and isolation hashes are refreshed
at each completed checkpoint.

The frozen source retains its tested Cache vector backend and damage API
outside this task's changes, while concurrent shared-checkout work is
preserved. `analysis/weapons-tu15-goal-20261013/source-isolation.json` records
that boundary. Its native wire protocol is private `0x803C`, to distinguish the
isolated payload from concurrent protocol work. Only weapon-specific launchers
were updated by this goal. Their dedicated CMake targets now default to the
original Xbox gameplay HUD and fonts; both `--launcher-info` outputs verify
that choice. Menu/pause assets keep their existing independent selection.
The earlier `hud-smoke` still selected imported gameplay artwork and is
retained as a superseded delivery observation; `hud-xbox-smoke` verifies the
corrected launchers. `hud-launchers-build.log` records their build.
