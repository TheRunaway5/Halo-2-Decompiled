# Full damage coverage: actual native connections and acceptance gates

2026-10-12. Research only. No production or test changes belong to this handoff.
The user requires complete Spartan/Elite shield visuals and all damage coverage,
with each finished system connected live and no guessed owners. The current
implementation does not satisfy that requirement. Complete paired instruction
bodies, diagnostic returns, natural host capsule hits and visible shield draws
are distinct kinds of evidence.

The highest-value immediate connection is **the existing 90-body paired damage
module into the retained TU1.5 world dispatcher**, followed by a single source
vitality/feedback authority used by gameplay and rendering. The paired module
already supports both executable images; the current convenience composition
hardcodes TU1.1. Source-enabled Match now selects a separate TU1.5 world when the
owned TU1.5 executable is selected. Neither shared host damage nor its vitality
readback is connected by the existence of that movement/world owner.
[Paired raw interface](../include/halo2/source_damage_delivery_runtime.hpp),
[TU1.1 convenience composition](../src/source_damage_runtime.cpp),
[actual world selection](../src/multiplayer_object_runtime.cpp),
[TU1.5 required-owner dispatcher](../src/physics_live_world_tu15.cpp).

## Fresh primary-source census

[The read-only census](../analysis/damage-shields-goal-20261012/damage-coverage-research/census.py)
reads 43 physical owned map files: the 30 disc caches, including campaign,
multiplayer, shared and title caches, and 13 owned DLC files including retained
old/new revisions. There are 42 distinct whole-file hashes. It retains all file
hashes, exact tag datums/names, byte offsets, 15 schema hashes, source block
counts and every direct reference to a resident `jpt!` from those schemas.
All 43 files parse, and no traversed block is outside its owning cache.
This does not mean all cache schemas or executable producers were traversed.
[Exact report](../analysis/damage-shields-goal-20261012/damage-coverage-research/owned-cache-damage-census.json).

The owned set has **164 distinct resident damage-definition names, 63 weapon
names and 43 projectile names**. Lockout has 96 local damage definitions, 33
weapon definitions and 29 projectile definitions. Its declared reference graph
has 299 direct `jpt!` references, including 5 effect-part references. Counting
references is not counting independently admitted damage mechanisms: a scenario
reference/prediction table can list a definition without executing it; a `jpt!`
can be player feedback with zero vitality damage. Names are identifiers, not
behavioral authority. Existing 96-definition damage diagnostics cover this
Lockout subset and cannot establish the 164-name owned union.
[Census data and exact float words](../analysis/damage-shields-goal-20261012/damage-coverage-research/owned-cache-damage-census.json),
[existing diagnostic scope](SOURCE_DAMAGE_DELIVERY_RUNTIME.md).

Nine weapon names in the owned set have multiple authored barrels, including
plasma pistol, carbine, rocket launcher, flak cannon and several vehicle weapons.
The current importer uses `barrels.offset` as one selected barrel, retains its
projectile's ordinary impact damage, and does not preserve every alternative
projectile damage reference in `WeaponDefinition`. Every barrel/trigger branch
requires its actual source selector. Adding all names to the weapon map would
not complete their producers.
[Importer](../src/content.cpp), [native definition](../include/halo2/content.hpp),
[per-cache barrel counts and references](../analysis/damage-shields-goal-20261012/damage-coverage-research/owned-cache-damage-census.json).

The fresh executable audit rechecks **60 original direct-dispatcher call edges
and 749 context instructions** against the unchanged retail, TU1.1 and TU1.5
images. Each image has 20 direct `E8` edges. The retained caller recovery has
0/3/4 unresolved caller memberships respectively; those remain unresolved.
It also exports 15 retail producer/navigation bodies, 4,144 instruction rows,
and scans 9 direct retail radius-owner edges. These static bodies are navigation
and raw-byte evidence, without a new all-body equivalence or dynamic admission
claim. Independent extraction repeats byte-for-byte.
[Audit tool](../analysis/damage-shields-goal-20261012/damage-coverage-research/verify_primary.py),
[primary report](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/report.json),
[independent repeat](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-repeat-v1/report.json).

The nine retail radius edges come from `CA0B0`, `CF040`, `D9E70`, `FC330`,
`104150`, `10B4D0`, `17A8A0`, `242660` and `2A06A0`. The report retains each
actual callsite and setup bytes. The four opaque owners must not acquire a
guessed meaning from their proximity to named reconstructed functions. Direct
edge scans do not discover all indirect producer dispatch or prove live caller
reachability. [Fresh raw call contexts](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/report.json).

## Concrete missing joins, in useful implementation order

1. **TU1.5 world to existing damage module.** Emit the authenticated 90-entry
   canonical/actual address map from the existing paired generator or its
   authenticated manifest. In `PhysicsLiveWorldTu15` route a real actual damage
   callee to its canonical entry and execute
   `source_run_damage_delivery_runtime(...,TitleUpdate15,...)` on that world's
   bank and full ABI. Pass lower calls to the actual existing world services.
   Preserve pending frames on missing services. Do not apply an address delta
   or call `source_run_damage_1_1`, which selects `MapPack11` unconditionally.
   Validate actual TU1.5 ordinary `BD360` vitality call and an original-produced
   admitted damage request, including all lower stops.
   [Authenticated generator](../tools/generate_source_damage_delivery_runtime.py),
   [module](../src/source_damage_delivery_runtime_generated.cpp),
   [current TU1.5 dispatcher](../src/physics_live_world_tu15.cpp).
2. **One vitality/feedback owner.** `Simulation::damage` currently mutates
   `Unit.vitality` with `apply_unit_damage`; ordinary update independently calls
   scalar `recharge` and `update_damage_feedback`. Original source routines
   mutate persistent raw body fields. `MultiplayerMatch::snapshot` publishes
   the host fields, and the renderer's function callback consumes that snapshot.
   Binding identical full object/player identities does not unify those banks.
   A source-authoritative path needs source damage/update once per owning tick,
   then read-only publication of the produced vitality, flags, signed stuns and
   recent-damage fields to all host/replica/HUD/material consumers. Do not run
   both scalar and source updates or copy host damage into source and call that
   original-produced output. Complete source admission remains required.
   [Live damage/update](../src/simulation.cpp),
   [snapshot owner](../src/multiplayer.cpp),
   [render consumer](../src/multiplayer_runtime.cpp),
   [scalar scope](../include/halo2/kernels.hpp).
3. **Actual weapon/projectile bodies before natural original hits.** Match
   source-enabled spawn genuinely constructs a biped, but allocates its starting
   weapon through `allocate_object(2) -> NativeObjectLifetimes::reserve_header`.
   That operation writes the header/type and records a native lifetime; it does
   not construct a raw weapon body or attach it to the source biped inventory.
   The carried weapon/ammo/inventory are host `Unit` sidecars. Complete original
   loadout construction/attachment must precede passing those handles to a raw
   weapon, projectile or melee producer. Source headers with body pointer zero
   cannot become successful source objects because a native weapon exists.
   [Actual spawn](../src/multiplayer.cpp),
   [allocation owner](../src/native_object_lifetimes.cpp),
   [known original loadout evidence](MP_MELEE_ACTION_SOURCE_RESEARCH.md).
4. **Kill volumes through their existing phase owner.** Current Match still
   scans every enabled-index row using native `Body.position` and writes dead/
   body directly. Original TU1.1 `DEFC0 -> 1EFEF0 -> 11DB80` selects one row from
   actual tick/full-datum arithmetic, respects the retained disable bank and
   tests source-produced bounds center `object+30`. Its positive branch builds
   the actual `globals\\distance` request through `D7C60/D7CD0/D9180`. This
   source phase is already compiled; let the real source phase own admission
   and publish its result, with complete damage/aftermath dependencies. Remove
   the host bypass only once the complete source path is usable. The current
   source-physics Match reads source motion after object phases, while its host
   kill postpass runs before them, so replacing only the position operand also
   changes the source order incorrectly.
   [Current Match order](../src/multiplayer.cpp),
   [actual original owner and bounds-center evidence](MP_SPARTAN_KILL_TRIGGER_RESEARCH.md),
   [compiled postphysics](../src/biped_postphysics_phase_generated.cpp).
5. **Authored effect damage on the gameplay scheduler.** Current firing damage
   references are used by `Simulation::shot` for rumble events. Retail `17A8A0`
   has a separate `jpt!` part branch which constructs a damage request from the
   effect's actual owner/location, point/direction and scale, then calls the
   radius owner. Lockout's five such parts include plasma-pistol charging and
   weapon overheating. Complete original event/part scheduling and gameplay
   dispatch are required; a renderer visiting an effect once per split-screen
   view cannot own damage execution. A rumble request is not area damage.
   [Live shot](../src/simulation.cpp),
   [fresh original part-owner listing](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/0017a8a0-retail.asm),
   [exact effect reference rows](../analysis/damage-shields-goal-20261012/damage-coverage-research/owned-cache-damage-census.json).

## Exact shared ABI and minimum real state

The complete dispatcher is retail `D7B80`, TU1.1 `D9180`, TU1.5 `DA0D0`.
At entry ESP `S`, its six DWORD arguments are request pointer at `S+04`, full
target at `+08`, signed low16 damage-info index at `+0C`, signed low16 opaque
index at `+10`, signed low16 model entry at `+14`, and opaque region/response
pointer/value at `+18`. The request is writable `0x88` bytes. Return is `RET18`;
normal exhausted-loop EAX is `FFFFFFFF`, not damage or Boolean success. The
dispatcher has original recursive child/rider calls, so one singleton scratch
request/result cannot safely stand in for its stack ownership.
[Full ABI and original listing](DAMAGE_DISPATCH_RUNTIME_HANDOFF.md).

`D7C60 -> D8BB0` uses EDX=request and one stack damage datum, `RET4`; it clears
exactly `0x88` bytes and initializes the original current-BSP/material/scale
inputs. `D7CD0 -> D8C20` has `(object,owner_output)`, `RET8`, and writes the
actual twelve-byte attribution container in source order. A native event needs
its genuine producer writes after these leaves; the constructor alone does
not establish impact flags, material, node, model entry, direction or scaling.
[Request/owner leaves](BIPED_FALLING_DAMAGE_RUNTIME.md),
[falling producer arguments](FALLING_DAMAGE_PRODUCER_HANDOFF.md).

The persistent body stores maxima at `+E4/+E8`, normalized body/shield at
`+EC/+F0`, visible recent shield/body at `+F4/+F8`, accumulated recent shield/
body at `+FC/+100`, signed shield/body stun words at `+104/+106`, signed recent
timers at `+108/+109`, and source damage-state bits at `+10A`. Do not convert
requested result amounts `+44/+48` into measured vitality loss: source feedback
consumes the requested normalized fields, including overkill/overflow. A new
host readback must preserve those distinctions, authored maxima and source
flags. [Original object field owner](../tools/reference/halo2-decompiled/src/damage.cpp),
[full source damage/vitality bodies](../src/source_damage_delivery_runtime_generated.cpp),
[feedback contract](../include/halo2/damage_feedback.hpp).

Actual prerequisites are source-constructed header/body/components and current
poses, genuine cache/tag/weapon/effect references, selected options/engine/
traits, actual player and actor records when reached, the owning source clock
and RNG, real parent/child/seat state and actual replication/effect/physics
services. Executable-global initial zero words are not initialized owners.
[Dispatcher state dependencies](DAMAGE_DISPATCH_RUNTIME_HANDOFF.md),
[vitality state dependencies](SOURCE_VITALITY_RUNTIME.md).

## Original producer seams and what their available evidence establishes

| Family | Exact owner/seam | Existing proof and remaining live prerequisite |
| --- | --- | --- |
| Direct projectile impact | Retail `FAA60`, TU1.1 `FEB40`, TU1.5 `FFA60`; its direct dispatcher edges are `FAE4E/FEF2E/FFE4E`. | Raw call setup carries actual contact object, node and model-entry values and normal. Native trace still selects capsules. Source current collision permutations, authoritative current world matrices, weapon/projectile body lifecycle and the original contact entry remain required. [Raw edges](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/report.json), [natural host limits](MP_NATURAL_PVP_DAMAGE_RESEARCH.md). |
| Melee impact | TU1.1 `EB080` begin, `EB700` ordinary progress/event gate, `D0830` contact owner. `D0830` enters dispatcher at `D1000`. | Genuine source-created SMG melee request/channel first update exists; impact progress is not reached. `EB700` calls `D0830(unit,semantic,component30,float(component31)*literal)`. Current-pose 5x5 source casts and real action schedule must admit contact. Generic instantaneous trace of a melee damage tag is not this path. [Exact ABI](MP_MELEE_ACTION_SOURCE_RESEARCH.md). |
| Radius candidates and per-object admission | Retail `D6C80` candidates, `D74E0` per-object; TU1.1 per-object direct dispatcher edge `D8AE0:D9080`. | Host grenade/prop area damage supplies native nearest points and obstruction casts. Original broadphase/candidate ordering, copied request, player-only and ignore-owner rules, directional/random/impulse paths need their actual owners. [Original radius listing](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/000d6c80-retail.asm), [live area owner](../src/simulation.cpp). |
| Excessive downward velocity | Retail `E3700`, TU1.1 `E4D30`, TU1.5 `E5C50`; EAX=full unit, plain RET. | Complete paired producer compiles and declared diagnostics reach dispatcher. Actual source options DWORD+8 equal2 returns early. Effective velocity uses original parent/physical/buoyancy/support owners and strict threshold. Stock request uses `globals\\distance`; `globals\\falling` is a distinct authored reference with unrecovered natural admission. [Exact gates](FALLING_DAMAGE_PRODUCER_HANDOFF.md). |
| Kill volume | TU1.1 `DEFC0 -> 1EFEF0 -> 11DB80 -> 11DA40`; TU1.5 `DFEE0 -> 1F47E0 -> 11ECE0 -> 11EBA0`. | Constructor-owned original volume admission exists under declared diagnostic placement. Live source current bounds, tick and disabled-volume bank plus complete downstream services are required. [Owner evidence](MP_SPARTAN_KILL_TRIGGER_RESEARCH.md). |
| Contact request construction | Retail `1DA550(object,contact_index,entry)`. | Raw body reads actual component contact impulse/material/key/point/normal and creates collision damage only on its real impulse gate. Blocking triangles/capsules do not provide this contact record. [Fresh instructions](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/001da550-retail.asm). |
| Contact speed and transferred force | Retail `1DA9E0` and `1DA6D0`; direct TU1.1 dispatcher callers `1DE3D0` and `1DE0C0`. | Actual physical component/body velocities, source contact impulse, authored limits, cooldown/history and owner are prerequisites. Independent newer physics work must prove this gameplay damage consumer is reached; contact/solver completion alone does not do that. [Fresh speed owner](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/001da9e0-retail.asm), [force owner](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/001da6d0-retail.asm). |
| Effects, section markers and script radius | Retail `17A8A0`, `D9E70`, `2A06A0` feed `D6C80`; other raw incoming owners remain opaque. | Actual scheduled effect/event/part, damaged section response/marker, or cutscene-flag location must own the request. Neither a named asset nor render traversal supplies admission. [Raw contexts](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/report.json). |
| Script object/list/local player damage | Retail `2A07C0`, `2A0730`, `2A0840`. | Real script argument types, list iteration, object current point/location and original mode gates are prerequisites. Current native script host has bounded vitality setters/queries, not complete invocation of these damage roots. [Raw listings](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/report.json), [native script host](../src/simulation.cpp). |

The existing direct-call census also exposes `D10F0`, `D1470`, `D1890`,
`DD5B0`, `F1420`, `11B190`, `11B6C0`, `14EFF0` and unresolved TU1.1 caller
boundaries. Each is an actual incoming edge, not permission to assign a guessed
family. Their full behavior and indirect callers remain part of the closure
audit. [Verified incoming census](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/report.json).

## Acceptance matrix for the complete stated scope

Each row needs its authored inventory, complete executable admission and shared
delivery, a naturally admitted native event, retained source-produced state,
and the corresponding live consumer. A fixture can establish an individual
kernel/branch without closing a row. These gates are acceptance criteria, not
claims that each named tag is a separate source mechanism.

| Required family | Completion evidence that is still needed |
| --- | --- |
| Every owned weapon/barrel/trigger direct impact | All 63 weapon names and their authored barrel/trigger selections are accounted for; actual weapon/projectile births, fire/charged/heat/distribution/medium/range paths admit source collision entries; real node/section/material and source damage banks reach body, HUD, history and presentation. Explicitly retain branches unavailable in a particular map. |
| Projectile state-machine alternatives | Each actual impact, detonation, attached detonation, super/attached-super detonation and boarding reference in the 43-name projectile census reaches its correct source producer; timers, contact, guidance/attachment and repeated hits are actual persistent source state. No single-impact adapter is relabelled as all alternatives. |
| Melee, combos, lunge and objective-item strikes | Normal and alternate authored damage/response choices, source action cooldowns/events, current-pose casts, target facing/unsuspecting/sword paths and flag/ball/bomb definitions are exercised through real retained items. |
| Frag and plasma grenades | Existing native Q lifecycle remains validated; natural unit contacts/sticking, authored timer/air/ground/attached variants, source candidate/order/filter/obstruction, radius scaling and shared damage/impulse/effect delivery all close. |
| Radial, cone and stochastic area damage | Real original candidate owners and per-target nearest/obstruction services admit every present authored branch; request copies, source RNG, parent/child traversal and requested-versus-actual amounts remain correct. |
| Destructible scenery/crates/machines and chains | Real current pose/sections/permutations, delayed response clocks, constraints, damage effects, state changes and chained requests execute in source order; genuine response ownership and recursion are preserved. |
| Fall-impact, excessive-speed and distance death | Distinct `globals\\falling` admission is recovered; source excessive-velocity and distance paths execute naturally with support/parent/protection and local-player callbacks. Scripted relocation or a supplied physical state does not establish a fall. |
| Scenario kill volumes | Actual source bounds center and clock select one enabled source row; boundary/disabled/parented rows and source mode gates are covered; ordinary dispatcher/aftermath/death/corpse/respawn and attribution complete. |
| Physics collision, run-over, crush and machinery | Real constructed/solved contact and retained impulse/history reach the original collision producers; authored speed/force/material/type limits and owner rules cause live damage. Labels alone do not establish distinct branches. |
| Vehicle damage, riders and boarding | Genuine vehicle/seat/rider/parent/child banks cover direct seat scaling, transferred damage, boarding variants, occupant ejection/death, destruction effects and attribution. |
| Effect-part, item and weapon self-damage | Actual event scheduling invokes `jpt!` gameplay parts once per owning update; ready/charging/overheat/reload/chamber/firing/misfire/empty/detonation references are accounted for; split-screen render count cannot duplicate damage. |
| Campaign actors, creatures and authored special sources | The 68 resident damage names absent from Lockout are accounted for using original consumers; character/difficulty/team/actor state, infection/creature/shield-zap and special weapon or object graphs are executed where present. A category/name is not substituted for a producer. |
| Scripted object/list/local-player/cutscene damage | Original typed arguments and real owners drive all four roots and their mode/location/list predicates; scripts read the same source-produced vitality afterward. |
| Objective bomb and map-specific authored sources | Authored objective/map event admission, ownership, arming/countdown and effect/request paths are traced and run; DLC/revision differences remain explicit. |
| Shared damage amount/admission | All present owner/friend/player, protection, hierarchy, shield-only/skip, EMP, random-upper-bound, trait/handicap and actor/difficulty branches execute with genuine state, preserving FP/RNG and strict comparisons. |
| Shared body/shield/sections | One source authority owns maxima/fractions, stun/depletion/threshold/overflow/overkill, materials/armor keys, head/section selection and pending responses; readback does not independently recompute damage. |
| Natural recharge, health and overshield pickups | Complete source update runs once per tick; actual pickup/equipment admission reaches health restoration/overshield activation; ordinary recharge, overcharge, multiplayer drain, actor/team scaling and history operate on the same source body and HUD bank. |
| Aftermath, death, corpse and rebirth | Source reaction/impulse/camouflage/history/credit/network and full death cleanup finish after every applicable producer; current/previous player ownership, equipment drop, corpse lifetime and actual rebirth are retained across repeated lives. |
| Spartan world and first-person shield visuals | Natural source hit updates actual body feedback/function inputs; original damaged/overshield shader selectors, geometry/node ownership, resources, collector/transparent queue and GPU submission render both views through hit/deplete/recharge/drain, with actual pixels compared to the original reference. |
| Elite world and first-person shield visuals | Same end-to-end requirements using Elite's own distinct authored damaged shaders/effects and geometry; the shared overshield references do not make ordinary Elite damage presentation identical to Spartan. |
| HUD, sound, flash, vibration and camera responses | Source response selectors, event scales and clocks reach actual per-player consumers under shielded/unshielded/lethal/recharge states; a body overlay draw does not establish player-response delivery. |
| Local, split-screen and native LAN authority | Identical authoritative source events mutate one bank once; snapshots/replicas/current-view owners consume produced results; damage cannot be view-dependent or duplicated and old full identities cannot redirect new owners. |
| Numerical and coverage closure | Every reached unsupported owner is resolved or a genuinely absent authored branch is demonstrated; known x87/NaN/estimate discrepancies have original evidence and are not forced to pass. Indirect producer dispatch and all cache-source differences are audited before reporting 100 percent. |

The Lockout multiplayer Spartan model has body/shield maxima 45/70, damaged
FP/world shaders `E9F4086F/E9F70872`, damaged effect NONE, depleted/recharge
effects `E95507D0/E95A07D5`, and overshield FP/world shaders
`E96307DE/E96607E1`. Elite has maxima 45/70, damaged FP/world shaders
`EA860901/EA870902`, damaged/depleted/recharge effects
`EA880903/EA8A0905/EA8C0907`, and overshield FP/world shaders
`EA8D0908/EA8E0909`. Those are raw metadata identities, not positive effect
allocations or rendered pixels. Spartan NONE damage-effect attempts must stay
distinct from Elite's real effect and from the ordinary shimmer shader.
[Fresh exact model bytes and reference offsets](../analysis/damage-shields-goal-20261012/damage-coverage-research/owned-cache-damage-census.json),
[visual-owner distinctions](DAMAGE_SHIELD_COVERAGE_GOAL.md).

This research executes no native gameplay test and produces no new screenshot.
Existing validated increments remain independently scoped. Current source
hashes and exact observations are frozen in the linked research manifest; other
chats continue editing this checkout. No percentage-complete result or complete
binary/source bundle is claimed.
[Research manifest](../analysis/damage-shields-goal-20261012/damage-coverage-research/manifest.json).
