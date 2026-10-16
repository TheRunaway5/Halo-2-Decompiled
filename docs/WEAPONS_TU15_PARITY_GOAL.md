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

`weapon-completion-matrix-v1.json` expands those same63 rows into630 explicit
completion gates, including owner/applicability audits and complete reference
sequences. Bounded Hunter evidence is attached only to its tested Old Mombasa
datum; it certifies neither a whole gate nor all authored variants. No weapon
is fully certified.

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
