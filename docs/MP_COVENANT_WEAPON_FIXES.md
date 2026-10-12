# Needler, energy sword and shotgun runtime defects

The carried material callback previously supplied only ammunition decimal
digits; all other inputs became zero. This disabled the sword blade combiner
and the Needler's authored ammunition function. Native fired projectiles also
had no model submission path.

Carried first-person and body weapon material aliases now bind to the actual
primary or secondary weapon handle. The callback evaluates the recovered
`turned_on` predicate and magazine selectors using retained native weapon
fields and the actual authored magazine capacity. Each hand has independent
fields in local state and native UDP snapshots. World pickups/drops also retain
those exact words and apply the same authored animation masks to independent
per-owner renderer constant banks. This restores the Lockout world sword blade
without mutating the shared resident material. The on/off/on shader regression
checks both literal blade materials, and the UDP regression checks an offhand
drop, including preservation of a declared NaN payload wire control. Unsupported shader producers
remain separate.

Native projectile snapshots now retain full salted owner, definition, position,
velocity, birth forward/up and accumulated travel distance. The new scene
prepares the authored singleton/default-node projectile model, then submits
per-owner geometry, six-LOD selection, lighting and color constants. Shared
textures do not imply shared owner color state. Unrecovered model, animation,
region, PRT or color contracts are reported instead of substituted geometry.
The shard's transparent shader also consumes scalar extern37. The recovered
parent walk returns zero for a free type5 projectile with parent NONE; the
native flight bank uses that branch rather than its shooter's camouflage.
Six unchanged original scalar-dispatch executions across retail, TU1.1 and
TU1.5 check free and attached projectile controls in
`analysis/multiplayer/integration/free-projectile-camouflage-20261011.json`.

The Lockout Needler shard meets that bounded model contract. Its RGB function
uses the original `acceleration_range` selector on accumulated travel distance.
The selector and RGB interpolation match 1,022 independent executions of the
unchanged retail instructions, using the literal authored color function.
The birth-up fragment matches 1,216 independent TU1.1 cases, including ties,
signed zero and small-vector branches. These comparisons cover those numeric
fragments, not the complete original projectile birth or update chain.

`native_covenant_weapon_runtime_tests` uses actual SDL pickup and firing input,
the Vulkan renderer and SDL sound queue. It checks advancing authored attack
frames and changing submitted vertex positions, actual per-owner material
inputs, accepted authored sword strike sounds, and nonempty Needler shard
draws with live full owners. Its optional `idle` capture checks the carried
weapon before the attack; `ready` samples the actual authored ready clip;
`world` checks a live unpicked Lockout sword owner. The Sword ready probe
checks hilt-only materials, one delayed activation sound, authored particle
submissions, and the original blade-ready gate. `ignite` and `lit` capture
those later phases. Player placement is held by an explicit diagnostic
teleport during the held pickup input and after pickup, isolating this test
from the unfinished native mover. The probe excludes physical game controllers
through the documented SDL ignore hint and an explicit keyboard/mouse input
identity. Ready-only probes reject any unrequested attack. The full picked owner must
remain alive and selected through the attack. An earlier repeat that switched
back to the SMG is retained as a failed probe, not shotgun validation.
The SDL dummy device proves packet acceptance, not physical audible output.

Baseline rendered failures and subsequent captures are retained under
`build-unit-object-20261011`. The tested source capture is
`.local/unit-object-covenant-source-20261011`; its runtime build is
`build-unit-object-covenant-20261011`. The frozen weapon runtime uses native protocol 38; the shared UI runtime has
its own newer native protocol. Matching client/server builds are required.
Both carry the authoritative primary/secondary ammunition total for reload posing.

## Original ownership still open

`WeaponState::shader` retains recovered allocation/constructor, attachment,
holster, detach and ready-countdown fields. Native admission does not execute
the complete original item/power evolution.
The first bounded TU1.1 pickup probe completed the Needler attachment and
observed item flags changing from zero to `0D`. The initial sword probe stopped
at `16D2C3` on the absent `object looping sounds` pool. A separate continuation
now executes complete original `18A200`, `16D070` and `16D210` to construct,
bind and enable that pool. Only its virtual raw-storage leaf and game-arena
backing are new platform inputs; inherited fixture/resource/service boundaries
remain declared. The unchanged sword construction and 233ms pickup then
complete, with one original looping owner, item flags `0D`, weapon flags zero,
power zero, ready countdown21 and blocking countdown15. The actual ready owner
requests `E2FB0186` through `107B50`; this inherited effect boundary remains
unavailable, not a successful activation. Reports are
`analysis/multiplayer/integration/covenant-pickup-shader-state-20261011.json` and
`analysis/multiplayer/integration/sword-looping-pool-pickup-20261012.json`.
Native admission now applies the bounded original attachment/holster/detach
masks and runs the recovered ready countdown on selection. Its fixed effect
consumer owns the ready event described below. It still does not execute the
whole raw item/power lifecycle or the original blocking countdown15 owner.

Needler homing, attachment, supercombine, detonation, contrail and third-person
firing effects are separate missing owners. The normal local first-person firing
effect and sound now use the bounded owner described below. Sword lunge, complete melee admission and
power evolution and object looping sound lifecycle are also unfinished. The existing authored first-person attack
clock and sound event path are exercised, not replaced by new timing constants.
World pickup constants now have per-owner bindings; complete original item/power
producers and owner-specific animated bitmap residency remain open.

Full source Unit lifetime, live current/saved pose and blend banks, third-person
layers, admission/drop world publication and Havok remain open as described in
`MP_UNIT_OBJECT_COMPOSITION_PARITY.md`. These defect fixes are not a claim of
complete original weapons or game parity.

## Shotgun firing

Lockout's shotgun has literal rate bounds0/0, burst bounds1/1, recovery1.0 and
trigger behavior1. The native generic weapon loop previously returned early
for every nonpositive rate, so it could never prepare a shot. Unchanged original
`1062B0` admits the initial zero-rate shot and rejects the second burst shot;
`108010` produces mode2 and30 recovery ticks. The independent report contains
four admission controls and the recovery execution, without replaced services
or instruction patches:
`analysis/multiplayer/integration/shotgun-firing-gate-20261011.json`.

The native loop now treats the recovered zero-rate latch as zero interval and retains the single-trigger
release latch from original `1021D0`. The regression first failed on an admitted
zero-rate shot, then passed shell consumption,65 held ticks without repeating,
and release/repress. The actual Lockout SDL/Vulkan probe also fires, consumes
one shell, advances authored firing poses and publishes10 pellet impacts.
This fixes disabled firing; the generic floating recovery/reload and native
pellet/contact paths are not the complete original discrete shotgun owner.

## Sword activation and wall marks

The actual54-frame ready animation renders changing authored poses. Selection
starts with only the hilt: original `1040F0` clears weapon flag20 and sets the
signed16 ready countdown to21 from authored0.7 seconds at30Hz. Complete
unchanged `1021D0` executions then decrement that countdown and enable the blade
on the21st update. The native fragment compares byte-for-byte against this
independent executable timeline, including item flags and power bits.

Attached FP activation effect `E2FB0186` owns the sound and ignition particles;
the ready graph's frame1 type 2 event refers to a different world-camera effect
`E313019E`. The fixed event-sequence consumer now reads the actual delay,
duration, sound part and each particle system's marker. Its event-state words
match complete unchanged `17AD20` executions: the ready sound starts at15
updates, followed by authored emission and blade activation. The three authored
systems use the actual posed marker and existing original vertex programs for
classifications7/8. No replacement blade, glow geometry or invented sound delay
was added. The retained SMG path still passes600 birth/expiry lifetimes.

`analysis/multiplayer/integration/sword-ready-sequence-20261012.json` records the
original instructions and resource hashes. That event oracle explicitly leaves
void part playback, particle advance, attached update and retirement services
at declared boundaries. It proves scheduling, not complete effect execution.
The separate native SDL/Vulkan probe proves changing ready poses, hidden then
lit blade materials, exactly one accepted ready sound and actual ignition
particle draws. A full run submitted197 distinct ready poses and1172 particle
draws; render counts depend on presentation rate. Its early failed ignition
capture stopped before the first authored emission; invalidated runs with
unrequested attack input remain in the evidence directory.

The perpetual `BladeOn` widget loop, full ready interruption/blocking ownership,
original object looping sound lifetime and complete power evolution remain
open. The native UDP wire retains raw blade state and the emitted ready event;
this addition does not change protocol37 or establish original Xbox networking.

No synthetic wall scorch was added. Complete unchanged material selectors
`189E10` (TU1.1) and `18B1D0` (TU1.5) produced identical results for8,736 controls
per image using the actual sword material table. For action15, variant0 and
fallbackNONE, all182 authored Lockout materials return three visualNONE values.
This establishes the material-selector boundary, not an assertion that the
original game never leaves a sword mark. Other original melee/effect ownership
must be recovered before supplying a decal. Fixtures and pins are in
`analysis/multiplayer/integration/sword-material-visual-20261012-final/`.
The native generic impact consumer still assumes a projectile definition;
original melee-grid contact/publication is separate unfinished work.

## Reproduction

```sh
ctest --test-dir build-unit-object-covenant-20261011 --output-on-failure \
  -R '^(original_unit_object_composition|original_unit_weapon_routing|native_weapon_gameplay|original_weapon_projectile_basis|retail_weapon_projectile_color)$'
.tools/python/bin/python tools/export_weapon_projectile_basis_1_1.py
.tools/python/bin/python tools/export_weapon_projectile_color_retail.py
```

The rendered probe requires a display, Vulkan, the owned Lockout/shared caches,
an owned TU1.1 or TU1.5 update image. Stock Xbox HUD assets are sufficient;
imported Vista HUD assets are optional. The local validation layer
is at `.tools/validation`; logs record the actual GPU and layer configuration.
Original image and fixture hashes are recorded in
`tests/fixtures/weapon_projectile_basis_original.provenance.json` and
`tests/fixtures/weapon_projectile_color_original.provenance.json`.

## Needler ammunition geometry

The first-person luminous material has no ammunition animation overlays.
Its missing needles therefore required the original pose owner, independently
of the already recovered third-person luminous shader input. Original169965
selects action `0A000069` from the graph's overlay table. Lockout's Spartan
Needler graph `E3C50250` selects clip 0 for a single weapon and clip 1 for dual
wielding; both are authored type 2 channels with 33 frames and 55 graph nodes.
The native binding retains this selection. Original169728..169846 selects the
loaded-round frame, including the weapon-type 2 reload transition after tick 44
with the literal `3E4CCCCD` multiplier. The channel replaces only its authored
translation/scale components before shared hierarchy, camera, IK, skinning and
marker publication. It does not hide arbitrary mesh sections or assign a needle
per round. The original content keeps full-sized needles at higher ammunition
counts and retracts them as the magazine gets lower.

`tools/oracle_needler_ammunition_overlay_1_1.py` executes the unchanged lookup,
frame selector, `1CA950` sampler and codec6 translation/scale routines 293200 and
293350. Its 470 cases include every ordinary frame, signed short extremes,
both dual selections and reload/partial-reserve cases. The native regression
matches every float word in all 55 partial local poses. A deliberately nonidentity
input bank verifies preservation of untouched channels. The full/empty shared
pose changes 1,056 submitted weapon vertices. Sparse interpolation follows the
source's subtract/multiply/add order. Unicorn's rounded binary32 reciprocal is
an explicit software convention, not proof of physical Xbox RCPSS seeds.

The finite-ammunition 104AA0/104CF0 inventory sum includes reserve from all
matching carried owners, including inactive slots, and excludes their loaded
rounds. Each hand's authoritative result is replicated; clients do not infer
it from only visible weapons. Native inventory and actual UDP tests cover this
field. The recovered frame selector is not a complete original reload lifetime.
The actual SDL/Vulkan Needler probe fires all 30 rounds, observes 31 ammunition
frames, retracts the first authored needle scale from 1 to 0.446058, and completes
refill. The empty-magazine capture has no protruding needles. Its 30 fired shard
owners also reach the renderer. Original whole Unit/pose/world ownership remains
separate from this native integration.

## Ricochet spark textures

The tested Lockout SMG contacts already selected the authored animated atlas
`E1F3007E` (`effects\bitmaps\sparks\sparks_animated`). No replacement artwork
was introduced. The world-effects regression now checks that actual impact
draws retain this identity and the complete decoded source texel bank.
`tools/oracle_weapon_spark_materials.py` independently checks seven actual
spark materials: fiery/soft animated sparks, trails, frag sparks, clusters,
impact sparks and shrapnel. Original retail 17000/17170/189A0 produce the
program, constants and sampler states. Hash-pinned unchanged Xemu S3TC and
swizzle routines decode their literal cache payloads; A8Y8 uses the original
R,R,R,G component mapping. All 674,881 serialized bytes match, including both
bound stages and every authored mip texel. The fixtures remain separate from
the native decoder. These checks establish asset/binding/texel parity for the
listed particles, not original live effect selection or whole framebuffer parity.


## Needler normal firing effect and sound

The previous FP effect owner rejected the actual Needler firing definition
`E3B4023F`: it has two sequential events, shared camera mode3 systems and
physics0 movement controllers. The live prepared-shot callback also routed
only the SMG muzzle effect. Normal Needler firing now requests the literal
normal effect from the weapon's one authored barrel and firing entry, using
its actual picked weapon identity and retained effect handle. A closed loader
check requires this branch's barrel flags1, heat maximum0 and misfire threshold0;
other barrel contracts are not inferred from a weapon filename.

`tools/oracle_needler_firing_effects_1_1.py` runs the unchanged TU1.1 original
picked Needler through100 idle/held/released ticks. Native loaded rounds, rate
words and all14 prepared requests, including effect/scalar words, match that
independent fixture. The recovered rate update happens after the effect
consumes its previous value, preserving source subtract/multiply/add order.
The original allocation boundary explicitly returns unavailable in this oracle;
it does not prove successful original effect/world allocation or full generic
native trigger/reload parity.

`tools/oracle_weapon_fixed_events_1_1.py` executes complete unchanged
`17AD20`, `17AC00` and `179D10` bodies with literal Needler, Sword and SMG
metadata. The native fixed sequence matches all372 event states, start calls
and emission slices across18 timing controls, including zero and negative dt
and the original eight-iteration limit. Another36 actual Needler camera
controls match original `17AA90` admission. Particle allocation, part playback,
attached updates and retirement remain explicitly declared service boundaries.
The report pins both executable/cache bytes and every unchanged instruction
executed; no instruction is patched.

For the ordinary dry, single-user FP view, Needler event0 admits all four
shared-camera muzzle systems for its authored1/30 second duration. Event1
admits its camera1 world-coordinate smoke system for the authored0.3 seconds.
Each system keeps its own actual marker, emitters, particle definition,
movement controller and original material/bitmap bank. World-coordinate smoke
converts the posed FP marker through the current camera frame before birth;
camera-local muzzle systems keep their own coordinate mode. Existing independently
recovered physics0 controllers run after particle age/position/phase integration.
Refreshing a valid effect retains its particles and restarts the event sequence;
stale full handles allocate a new native owner.

The actual event0 sound part is `E3B50240`. Its event identity is retained and
played once when that event begins; equal absolute event start times do not
merge parts. The old generic shot-sound fallback is suppressed when this
admitted effect owns the local sound. The source-authored gain is1 for this
literal part. Nonlocal host requests retain the authored sound through the
existing effect sound adapter; their world particles remain unfinished.

The standalone regression performs600 Needler requests, including30 rapid
refreshes, and observes46,217 particle draws, exactly600 sounds and all four
exact authored bitmap identities. It exhausts neither the retained native pools
nor particle lifetimes. The existing600 SMG lifetimes and Sword delayed sound/
ignition regressions remain green. The actual SDL/Vulkan Needler run fires all
30 rounds, submits exactly30 nonempty owned firing PCM packets, shows authored
muzzle/smoke particles and30 live projectile owners, retracts its needles and
completes reload. Presentation draw counts vary with host frame timing.
A finite firing capture separately requires accepted SDL fire input, two shot
owners, visible particle submissions and exactly one sound per fired round.
An earlier capture that never admitted its requested fire input is retained
as a failed probe; its idle image is not firing validation.

The unchanged source fragments and native checks do not establish pixel/audio
identity to an original Xbox gameplay capture. SDL dummy audio proves accepted
PCM, not physical audible output. Full original allocation and attached/world
ownership, fluid/media transitions, multi-user camera/world placement and
render-independent emission scheduling remain open. Native LAN publication
still has one retained effect event per player, so offhand/coalesced histories
are not complete. Needler homing, impact/supercombine/contrail/detonation and
full source Unit/pose/Havok work remain separate unresolved owners.


## SMG sound regression caught during Needler integration

The stricter dual-SMG probe exposed16 firing PCM submissions for8 real fired
rounds. Literal SMG effect `E59E0429` already contains its one `E59F042A`
firing sound part, while the native projectile postpass also queued the generic
weapon sound. The live local effect now owns that sound for both SMG and Needler;
the generic fallback is suppressed when the actual effect callback is installed.
Remote host weapons use the same authored part through the existing sound
adapter. The local client fallback also recognizes SMG ownership. The SMG muzzle
definition is prepared from the actual SMG content record independently of the
match's starting weapon.

The regression counts ammunition decreases independently for both carried
SMGs and requires one sound per fired round. The standalone owner additionally
checks one authored sound part for each of its600 SMG requests. This closes the
duplicate local firing sound path; the native client offhand/coalesced event
history limitation above remains open.


## Needler projectile construction and flight color publication

`tools/oracle_needler_projectile_birth_1_1.py` executes unchanged original
`FC2E0` and its callees with the actual Lockout Needler projectile tag. Across
200 cases it records 400 original RNG writes and exact constructor output
words. The native constructor matches the timer reciprocal, arming reciprocal,
speed scale, angular velocity, guidance rate, speed and inherited velocity
addition. The timer branch retains the original PC53 arithmetic through the
tick-rate multiply before rounding to binary32; rounding the sampled seconds
first gives different results. The separate TU1.5 correspondence report maps
the complete constructor to `FD200`, plus both callees, with identical normalized
instruction bodies and constants. This is static TU1.5 correspondence and
independent TU1.1 execution, not full TU1.5 world execution.

The live multiplayer Needler now consumes the two constructor draws from its
retained simulation stream after successful native allocation. Its closed
loader admission checks the actual projectile flags, timer type, dry gravity
and equal initial/final speeds. The existing native direction and explicitly
absent inherited platform/parent/medium/speed override are inputs to this
branch. Original upper allocation, muzzle/distribution and whole-shot RNG
ordering are not established by the constructor fixture. The retained birth
fields do not yet advance under an original projectile update owner.

The native presentation adapter also omitted `traveled` from its projectile
publication, leaving the authored acceleration-range color input permanently
zero. The new regression fails against that old adapter, then passes with
actual simulation flight distance retained through both presentation and match
publication. Existing native UDP already carries this field. The independent
original projectile color fixture also passes; no substitute color or texture
was introduced.

`tools/oracle_needler_terrain_contact_1_1.py` additionally executes the original
contact dispatcher with one declared head-on, default-material terrain contact.
The original needle sticks and stops with contact flags `0x1a`. This probe
declares generic storage, collision and audiovisual service boundaries; it is
research evidence only. Native contact still retires the projectile. Original
material/angle selection, posed target attachment, guidance updates, fuse,
contrail, supercombine and detonation owners remain unfinished.

An inherited build cache initially retained the parent's shorter projectile
layout in some consumers after the override header was introduced. The actual
SDL test caught an exception in match publication. Every stale dependent was
invalidated and rebuilt; the validation recorder checks actual Ninja header
dependencies across all existing built objects. The strengthened match
publication regression exercises the affected path directly. The failed live
probe and debugger stack remain in the evidence directory.

## Tested Needler delivery

`Halo2-Weapons-Launcher` and `Halo2-Weapons-Title-Launcher` select the tested
`build-needler-projectile-20261012/halo2_native` delivery. The source parent is
`analysis/scoreboard-emblems/20261012/verified-source`; the explicit CMake
overrides in `.local/needler-projectile-overrides-20261012` apply the original
Needler constructor, flight-distance publication fix, prior SMG sound fix and
their stricter tests. The parent's firing/Sword owners match the preceding
independent MCC checkpoint. The accepted menu/scoreboard files retain their
exact hashes. Their preceding CPU/native Vulkan profile checks remain pinned,
and the current build also passes the 12-pane actual-asset PCR regression.
The source changes remain in the shared root. Other active chats continue
to select the ordinary shared launchers, so those are recorded independently
and are not substituted with this checkpoint.

All six SDL/Vulkan cases pass: Needler full ammunition cycle, Needler firing
capture, Sword ready sequence, Sword strike, Lockout world Sword capture and
dual SMG pickup/fire/reload/grenade/drop. Each reports zero Vulkan validation
errors. The final Needler full cycle accepted30 owned firing PCM packets for30
fired rounds and completed needle depletion/refill. The final dual-SMG run
accepted8 firing packets for8 real rounds; the exact delivered SDL shot count
depends on input/frame delivery, and the test compares each observed round
rather than assuming a fixed host timing count.

`build-unit-object-20261011/needler-projectile-validation-20261012.json` records
the immutable source/hash closure, original fixtures, launcher/runtime identities,
successful live commands and logs, and retained zero-distance/cache-layout
failures. It preserves the previous `needler-firing-validation-20261012.json`
checkpoint. All 200 constructor cases pass with both selected TU1.1 and TU1.5
native metadata; the original color/firing and SMG lifetime checks also pass.
The current 16-participant/four-clock native UDP regression passes. The explicit
original/world/GPU/audio limits above remain.
