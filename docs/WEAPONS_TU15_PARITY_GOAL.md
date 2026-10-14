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
