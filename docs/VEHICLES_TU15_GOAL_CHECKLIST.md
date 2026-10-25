# All vehicles and variants: TU1.5 acceptance

The requested goal is **open**. Every original vehicle and variant must work
through native gameplay, presentation and audio owners, survive the original
damage/destruction lifecycle, and be validated in the game delivered by the
launcher. A successful cache audit, component oracle or startup smoke does not
complete a vehicle row.

Current delivery (refreshed October24): the launcher selects qualified runtime
`c5078a5b547040a6bc298f1cc239e0d9`, SHA-256
`83ec9c2a057cced5d38cfedc5eab0005b20c26845800df5852e51667a1eb4d7c`.
The [seventh delivery](../analysis/vehicles-tu15-goal-20261023/launcher-delivery-v7/report.json)
builds, qualifies and publishes the exact restored production source from the
latest374-check epoch. All thirteen submitted-frame configurations, four
audio scenarios and frontend checks pass. Both launchers select this runtime;
its114-file package retains304,558,061 bytes. It includes the native flying
selector and effect-readback ownership repair. The newer prepared original
audio reader remains checkout work beyond this frozen delivery. Current
normal-input Banshee entry/movement/primary firing now passes; additional
trigger and full flight-envelope checks remain required.

**Current in-game red:** [Scorpion coaxial fire](../analysis/vehicles-tu15-goal-20261023/delivery-installed-scorpion-coax-v2/report.json)
reaches its actual authored input1 projectile, then the selected game exits
with `Required original runtime backing unavailable at0x90c605c8` before the
action completes. The observer now reads the actual typed barrel bank; its
earlier presentation-view lookup failure is retained separately. A native
failure stack trace is running. This is a game failure despite the passing
headless secondary-fire component case; coaxial acceptance remains red.

The [current installed Banshee game](../analysis/vehicles-tu15-goal-20261023/delivery-flying-installed-banshee-v1/report.json)
passes real entry and primary firing:17 projectile generations,1.6482 world
units,1,527 submitted frames, nonzero game PCM and normal window close/exit0.
Its flying-selector counter advances from0 to2 during actual vehicle birth.
During firing it records42 native difficulty,21 material and864 model calls.
The inspected X11 frame shows the occupied aircraft in the base passage;
ascent/open-air flight and original output equality remain open. Its [cleanup](../analysis/vehicles-tu15-goal-20261023/installed-flying-banshee-inspection-and-cleanup-v1.json)
verifies removal of383,759,170 dependency bytes,36,887,040 raw PCM bytes and
the private build/sink. The [source reconciliation](../analysis/vehicles-tu15-goal-20261023/launcher-flying-source-reconciliation-v1.json)
confirms exact restored production sources in the selected launcher package.
The [longer normal-input takeoff](../analysis/vehicles-tu15-goal-20261023/delivery-installed-banshee-takeoff-v1/report.json)
uses the retained0.3-radian departure input and300 ticks of throttle, without
jump. It passes the existing greater-than10 horizontal/2 climb gates:
19.4476 horizontal units and14.2038 climb, with45 projectile generations,
1,785 submitted frames, nonzero game PCM and normal close/exit0. The native
flying selector is observed at birth; firing records200 difficulty,100
material and3,668 model calls. Its [inspected-frame/cleanup receipt](../analysis/vehicles-tu15-goal-20261023/installed-banshee-takeoff-inspection-and-cleanup-v1.json)
retains the actual exterior X11 frame and verifies removal of the private
build, dependencies, raw PCM and sink. This is bounded takeoff evidence;
complete flight controls, original output equality and lifecycle remain open.

The preceding selected runtime was`8cfb6e3ddda845bc876f4150741bb646`, SHA-256
`a3faf9355e04a1a9b84d8aa8759df15dcef0cd670147f98e5b06a08207234320`.
This task [built, qualified and published](../analysis/vehicles-tu15-goal-20261023/launcher-delivery-v5/report.json)
`e4be62bdf877497eb824c828e920c7f2`, including the original maximum-four
grenade codec. Its build, four audio scenarios, twelve rendered configurations
and frontend checks passed. Another task subsequently published the current
selection. The [latest reconciliation](../analysis/vehicles-tu15-goal-20261023/launcher-current-selection-reconciliation-v3.json)
verifies its retained vehicle kernels, Random binding, vehicle table projection
and unchanged grenade encode/decode statements. Protocol104 retains the codec.
This reconciliation does not qualify all newer unrelated changes against this
task's earlier374-check source epoch.

The [current installed Ghost test](../analysis/vehicles-tu15-goal-20261023/delivery-ordinary-installed-ghost-v7/report.json)
passes real entry, driving and firing:20 authored projectile generations,
7.5297 world units of motion and1039 submitted frames, followed by normal X11
window close with exit0. The actual game's stream supplies nonzero PCM.
During the action interval, read-only counters record42 native difficulty,
21 material and966 model calls; the current-difficulty wrapper remains zero.
The required452,252-byte X11 frame was inspected. Its [cleanup receipt](../analysis/vehicles-tu15-goal-20261023/installed-ghost-v7-inspection-and-cleanup-v1.json)
verifies removal of the owned observer/build,385,488,688 dependency bytes,
38,123,520 raw PCM bytes and private sink. The previous900-frame installed
attempt expired before seat admission; its failure is retained. No original
framebuffer or waveform equality is claimed.

The [installed Scorpion test](../analysis/vehicles-tu15-goal-20261023/delivery-ordinary-installed-scorpion-v1/report.json)
also passes real entry, driving and cannon fire:4.7203 world units, one authored
cannon projectile,859 frames and normal window close/exit0. The actual player's
four-fragmentation-grenade inventory crosses the former wire limit. Native
parameter/material/model activity and nonzero game PCM are recorded. The
501,473-byte frame is inspected; its [cleanup receipt](../analysis/vehicles-tu15-goal-20261023/installed-scorpion-inspection-and-cleanup-v1.json)
verifies removal of the observer dependencies, private sink and35,927,040 PCM
bytes. Coaxial firing and the complete destruction lifecycle remain separate.
The previous borrowed observer build has now been retired by its owner.

The [joined native regression](../analysis/vehicles-tu15-goal-20261023/vehicle-joined-full-regression-v2/report.json)
passes all374 declared checks without skips:327 constructor contexts,
30 destruction cases and seventeen weapon/boost/ownership checks. Normal-input
candidate runs also pass the Ghost, both Warthogs, Spectre, both tanks, Banshee
and both stationary turret families. These are bounded checks; all nine gates
for every original context remain open where complete evidence is absent.
The [completed build-session cleanup](../analysis/vehicles-tu15-goal-20261023/vehicle-session-final-cleanup-v1.json)
verifies removal of its temporary source and build. The303,819,622-byte e4be
launcher package is explicitly retained as a qualified delivery.

The following sections preserve earlier results and their source epochs;
pending language in those historical entries is superseded only by the exact
later evidence linked above or below.

The preceding selected qualified runtime was `70d5f714743e494cabeada6de3e9b3e9`.
Its [source reconciliation](../analysis/vehicles-tu15-goal-20261023/launcher-source-reconciliation-v1.json)
confirms that it includes all nine newer vehicle difficulty/material source
changes byte-for-byte, plus24 other delivered source changes with no conflicts.
This task's [v3 qualification](../analysis/vehicles-tu15-goal-20261023/launcher-delivery-v3/report.json)
passed its build, four audio scenarios,12 submitted-frame configurations and
frontend check; publication correctly preserved the other task's newer
selection. The selected package independently passes those qualification gates
and retains314,475,104 bytes. Its binary SHA-256 is
`833a11d9bea55d2b65227084185169a9ee8e4723812d5d0b4213da4513c6f973`.

The [current installed Ghost run](../analysis/vehicles-tu15-goal-20261023/delivery-ordinary-installed-ghost-v4/report.json)
passes real entry/drive/fire input,900 submitted frames, natural shutdown and
nonzero audio from the actual process. Read-only samples of named diagnostic
counters observe40 native difficulty calls,20 native material calls and955
native model calls during the actual action interval. The current-difficulty
wrapper counter remains zero; this run does not claim that route executed.
The required450,745-byte rendered PNG shows the seated driver, Ghost, reticle,
HUD and terrain. Its owned383,345,454-byte observer dependencies and private
build, sink and41,909,760 raw PCM bytes were removed. Earlier [installed proof](../analysis/vehicles-tu15-goal-20261023/delivery-ordinary-installed-ghost-v1/report.json)
and its [cleanup receipt](../analysis/vehicles-tu15-goal-20261023/delivery-installed-frame-inspection-and-cleanup-v1.json)
remain scoped to the preceding delivery. Neither run establishes full parity.

Actual match startup still rejects Random light, heavy and turret settings.
The [three-family reproduction](../analysis/vehicles-tu15-goal-20261023/random-vehicle-admission-red-v1/report.json)
records these failures. The [unchanged original selector reference](../analysis/vehicles-tu15-goal-20261023/random-vehicle-original-v1/report.json)
passes288 cases across three original maps, including60 reroll cases and up
to six draws. Its required compressed reference is8,480 bytes. Native selector
qualification and per-birth template/presentation integration remain open.

The [expanded original reference](../analysis/vehicles-tu15-goal-20261023/all-vehicle-choice-original-v2/report.json)
now covers2,583 inputs: every signed-byte setting for all three families on
three maps, plus288 Random inputs. Every one of208 instructions executes.
The required compressed reference is32,892 bytes. The preceding expanded v1
has an explicitly recorded case-metadata issue and is superseded by v2.
The [native selector comparison](../analysis/vehicles-tu15-goal-20261023/all-vehicle-choice-native-v1/report.json)
passes5,166 comparisons against the unchanged CPU reference and retained
literal controls, including ordered reads/stores, full GP/flags/FP state and
complete native/literal memory banks. It observes393 native RNG draws. The
[negative control](../analysis/vehicles-tu15-goal-20261023/random-vehicle-native-negative-v1/report.json)
rejects a five-choice-to-four-choice range mutation. The [scoped native routes](../analysis/vehicles-tu15-goal-20261023/all-vehicle-choice-native-routes-v1/admission.json)
preserve all literal bodies, and the [installed-route comparison](../analysis/vehicles-tu15-goal-20261023/all-vehicle-choice-installed-v1/report.json)
passes with native counter assertions. Missing-page faults and executable
performance are outside this selector component result.

The current implementation prepares cold definition/variant candidates from
the original runtime references without consuming RNG.15D4F0 and the native
selectors retain actual per-birth choice. Scene publication then binds the
completed full generation to the matching authored model variant; bounds,
seats, child placements, rendering and network placement metadata use that
same template. Continuation binding includes the immutable candidate catalogue.
The [five-unit compilation check](../analysis/vehicles-tu15-goal-20261023/random-vehicle-syntax-v1.json)
passes. Actual240-tick Random light/heavy/turret match checks and the native
application build are running in the [owned session](../analysis/vehicles-tu15-goal-20261023/random-vehicle-session-v1/session.json).
Its source is `.local/test-scratch/halo2-test-jyah8vzp/source`; this active
temporary build is not a retained delivery. The first build stopped at the
movie packager's data-alias guard. An [identical owned15,418-byte data copy](../analysis/vehicles-tu15-goal-20261023/random-vehicle-private-data-v1.json)
preserves the original inputs and permits the next build. Live behavior,
rendered Random births and launcher delivery remain unverified.
The [regeneration diagnostic](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-regeneration-diagnostic-v1.json)
confirms exact regeneration of all three installed selector owners. Whole-file
regeneration differs in unrelated existing x87/body owners and is not promoted.
The [component cleanup receipt](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-component-cleanup-v1.json)
confirms removal of the completed compiler/oracle owners; the active gameplay
build remains owned by its running session.

The [installed flying/secondary-heavy diagnostic](../analysis/vehicles-tu15-goal-20261023/vehicle-flying-secondary-heavy-admission-v1.json)
reproduces a real Flying Vehicles=Off crash at row53 after actual frame9.
The120-frame secondary-heavy=Scorpion diagnostic exits normally; it does not
prove the selected definition or its presentation. The source now projects
option79 only onto classification4 and option80 only onto classification5,
following15CEF0's actual option+20C/+20D reads.15CD30's two enabled branches
both use runtime reference4 with variant0. Off prepares no flying template;
the original source owner must retain NONE. The [scoped compilation check](../analysis/vehicles-tu15-goal-20261023/vehicle-flying-secondary-heavy-syntax-v1.json)
passes, and explicit Off/On/secondary-heavy actual-world checks are prepared.
These changes are newer than the currently running three-family build and
still require a declared source refresh and live validation before delivery.

The rebuilt [three-family live result](../analysis/vehicles-tu15-goal-20261023/random-vehicle-runtime-validation-v2/report.json)
passes all three240-tick checks. Both authored rows per family produce actual
source generations bound to the matching definition and model variant. Native
selector counters record2 light,2 heavy and9 turret draws, with no supplied
RNG, admission or source results. The initial preparation's packaging failure
remains separately recorded. This result proves live births and template
binding, with rendered input/audio/destruction checks still separate.
The [declared source refresh](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-runtime-refresh-v1/prepared.json)
updates only content projection, its actual-world test, the generator guard
and private test registrations. The next [six-case build/run](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-runtime-validation-v3/report.json)
adds Flying Off, Flying On and secondary heavy, preserving the three Random
checks. All six pass in21.72 seconds. Off retains NONE for both actual source
rows; On and secondary-heavy bind every affected row to its actual authored
definition/variant. This result still excludes rendered input, host-output
audio and destruction acceptance. The Random light-vehicle real-input game
run is now underway; no updated launcher delivery is claimed.

That [rendered Random-light run](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-light-game-v1/report.json)
fails before play: its cold resource union exposes an unresolved Mongoose glass
shader. The [literal table census](../analysis/vehicles-tu15-goal-20261023/vehicle-runtime-table-census-v1.json)
identifies the underlying TU1.5 mismatch. Original Coagulation, Headlong and
Zanzibar use11 references with NONE at6/8/9. MCC uses14, substitutes Mongoose
at6 and appends Creep, Falcon and Gravity Throne. The preceding six live
checks establish actual births/binding; they do not establish table parity.

The new guarded projection matches every full group/salt/path in the known
14-entry tuple before restoring reference6 and count11 in the native cache.
Random retains its original five-way draw and retries NONE; the candidate
pool never selects or filters the gameplay RNG. Unknown/custom tuples remain
unchanged. The actual-world checks now independently verify the retained
source table against the authenticated11-entry identity tuple. The [scoped
refresh](../analysis/vehicles-tu15-goal-20261023/vehicle-table-runtime-refresh-v1/prepared.json)
proves that removing this projection restores all three previous loader/header
preimages exactly. Its [seven-check build/run](../analysis/vehicles-tu15-goal-20261023/vehicle-table-runtime-validation-v4/report.json)
is pending, including full-bank mutation/rejection controls and the six actual
world cases. Three preceding standalone compiler setup failures are retained;
the checks are now linked through the actual native cache target. The failed
game's private observer/sink and2,196,480 raw PCM bytes were removed, preserving
the default sink; it retained no image or runtime.
The seven checks now all pass in32.19 seconds, with the independent original
identity-table assertion active in every world case. The rendered Random-light
retry is running against that exact source-bound binary. Its [cleanup receipt](../analysis/vehicles-tu15-goal-20261023/vehicle-table-failure-cleanup-v1.json)
confirms removal of all four completed failure owners and357,530,344 temporary
dependency bytes. The current gameplay source/build owner remains active.

The [rendered Random-light retry](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-light-game-v2/report.json)
passes900 frames and natural shutdown. Normal XTest walk/entry/fire input
boards a randomly born Warthog chaingun seat and observes23 projectile
generations, nonzero PCM from the actual game process and native selector
counters at14 calls/four draws. The action interval additionally records48
native difficulty,24 material and819 model lookups. This gunner-seat check
does not claim chassis driving; its displacement is zero. The [inspection and
cleanup receipt](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-light-inspection-and-cleanup-v1.json)
retains the required475,030-byte PNG and confirms removal of the observer
dependencies, private sink and73,605,120 raw PCM bytes. The frame shows the
gunner, Warthog rear, HUD and map; original-pixel/audio equality remains open.
The same-source Random-heavy real-input run is now active.

That [heavy run](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-heavy-game-v1/report.json)
crashes during the normal approach to a randomly born Scorpion, before entry:
`Invalid native grenade counts`. The native snapshot codec accepts only0..2.
The [original grenade authority](../analysis/vehicles-tu15-goal-20261023/original-grenade-count-authority-v1.json)
pins both authored maximums to4 in all three original maps. This is a genuine
transport range defect; the failed run did not yet retain the rejected counts.
The replacement preserves both0..4 counts and selection in one byte, rejects
invalid data and increments native protocol102 to103 so old peers refuse the
changed layout. Gameplay inventory, pickup and limit logic are unchanged.
The [exhaustive codec check](../analysis/vehicles-tu15-goal-20261023/native-grenade-wire-v1/report.json)
passes75 inventories,181 reserved wire bytes,502 invalid count inputs and four
invalid selections. The [scoped integration](../analysis/vehicles-tu15-goal-20261023/vehicle-grenade-wire-refresh-v1/prepared.json)
authenticates both complete old network/protocol preimages and retains all
other owners. Its [eight-check rebuilt validation](../analysis/vehicles-tu15-goal-20261023/vehicle-grenade-wire-validation-v5/report.json)
is pending. The subsequent real approach must confirm the actual carried
counts and absence of the crash before it is considered resolved in game.
All eight rebuilt checks pass in21.46 seconds. The protocol103 heavy-vehicle
real-input retry now records the actual carried grenade counts in its neutral
LAN observations. The [failure/codec cleanup receipt](../analysis/vehicles-tu15-goal-20261023/vehicle-grenade-failure-cleanup-v1.json)
confirms removal of both completed owners,357,539,528 private dependency bytes
and56,217,600 raw PCM bytes, preserving the default sink. The active candidate
remains unpublished pending in-game checks and launcher qualification.
The [heavy retry](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-heavy-game-v2/report.json)
passes normal Scorpion approach/entry/drive/fire,4.62206 world units of motion,
one cannon projectile,900 frames, actual host-output audio and natural
shutdown. Actual decoded observations include three frag grenades, proving
the former two-grenade transport limit was crossed without the crash. The
action interval observes two native difficulty, one material and805 model
lookups; native selector execution is separately observed in the same process.
The [inspection and cleanup receipt](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-heavy-inspection-and-cleanup-v1.json)
retains the required498,731-byte PNG and confirms removal of the temporary
observer dependencies, sink and42,378,240 raw PCM bytes. Original visual/audio
equivalence and complete Scorpion behavior remain open. The Random stationary
turret real-input checks use this same protocol103 source epoch.

The [Coagulation stationary-turret retry](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-turret-game-v2/report.json)
and [Zanzibar check](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-zanzibar-turret-game-v1/report.json)
each submit900 frames and shut down naturally but fail to board. The input
probe walks into lower map geometry below the elevated authored seat. Both
maps provide empty authored navigation routes for these approaches; neither
result establishes a turret gameplay failure or successful turret acceptance.
The [cleanup receipt](../analysis/vehicles-tu15-goal-20261023/stationary-turret-failure-cleanup-v1.json)
verifies all three completed probe owners, private dependencies, audio sinks
and162,562,560 raw PCM bytes were removed. Three required diagnostic PNGs
total625,762 bytes and show the player still on foot facing obstructing map
geometry. The input observer now has a capsule-clear collision-floor route
fallback; its in-game behavior is still unverified.

The [collision-route component check](../analysis/vehicles-tu15-goal-20261023/stationary-turret-collision-routes-v2/report.json)
finds129-waypoint Coagulation and233-waypoint Zanzibar paths in0.59/0.80
seconds using the actual static collision floors and player capsule. Its
compiler owner was removed; no game behavior is inferred. The next two game
runs reach the Coagulation ramp. [v3](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-turret-game-v3/report.json)
stops before a required jump; [v4](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-turret-game-v4/report.json)
uses normal Jump input and progresses up the approach but exhausts its900-frame
budget before boarding. Both exit naturally and retain failed acceptance.
Their [v3](../analysis/vehicles-tu15-goal-20261023/stationary-turret-route-v3-cleanup-v1.json)
and [v4 cleanup receipts](../analysis/vehicles-tu15-goal-20261023/stationary-turret-route-v4-cleanup-v1.json)
verify removal of private builds, dependencies, sinks and PCM. Their required
images total987,455 bytes. The longer2100-frame retry is running with the
same gameplay binary; only the input probe's route and budget have changed.
The [2100-frame Coagulation retry](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-turret-game-v5/report.json)
also fails boarding: the approximate static route leads below the platform.
Its [cleanup receipt](../analysis/vehicles-tu15-goal-20261023/stationary-turret-route-v5-cleanup-v1.json)
retains the required365,367-byte diagnostic image and verifies temporary
dependency/PCM/sink removal. The [Ascension check](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-ascension-turret-game-v1/report.json)
has no collision route to its elevated turret and its straight-line fallback
stops at map geometry. It exits naturally after2100 frames but remains failed.
Its [cleanup receipt](../analysis/vehicles-tu15-goal-20261023/stationary-turret-ascension-cleanup-v1.json)
records the required image and removal of the owned observer, PCM and sink.
The [footprint-support route refinement](../analysis/vehicles-tu15-goal-20261023/stationary-turret-collision-routes-v3/report.json)
passes its geometry-only checks; neither pathfinding component result proves
in-game traversal. No additional stationary-turret acceptance is claimed.

The [current complete native regression](../analysis/vehicles-tu15-goal-20261023/vehicle-candidate-full-regression-v1/source-binding.json)
completed against this same frozen candidate:327 constructor contexts,
30 destruction cases, nine primary weapons, two secondary weapons, Ghost
boost and five ownership/BV checks. Its private registrations are restored
after the run. The preceding focused repeat passes the eight actual-world,
table and grenade-wire checks; its separate legacy physics fixture is skipped
because that declared input path is absent and is not counted as a pass.
The [completed374-check result](../analysis/vehicles-tu15-goal-20261023/vehicle-candidate-full-regression-v1/report.json)
passes372 checks, including all327 constructors, all30 destruction/continuation
cases, both secondary weapons and Ghost boost. Warthog MG fails its live
projectile/contact assertion; Spectre fails target damage. Both remain red.
This primary-firing fixture still uses private B9B70/BA490 entry/return
observations to capture short-lived bullets and correct muzzle parallax;
those scopes were removed for the ordinary application build. The hypothesis
that this explains the failures is unverified until focused reruns pass.

The [joined candidate](../analysis/vehicles-tu15-goal-20261023/vehicle-joined-delivery-inputs-v1/prepared.json)
adds25 independent production files from the currently selected qualified
delivery, preserving every vehicle change. This is private source integration,
not acceptance or launcher publication. Its [staging cleanup](../analysis/vehicles-tu15-goal-20261023/selected-delivery-input-cleanup-v1.json)
verifies removal of the separate1,959,062-byte staging owner. Two read-only
diagnostic scopes are restored in the private test build with complete
production preimage hashes. A configure attempt exposed three newly registered
test sources absent from the older snapshot; these are now frozen explicitly.
The [next native build](../analysis/vehicles-tu15-goal-20261023/vehicle-joined-native-validation-v7/report.json)
is active. Focused firing, the complete regression and ordinary production
restoration must pass before launcher delivery. No new runtime is selected.
That joined native build now passes all eight checks, with unchanged frozen
inputs. The [focused firing rerun](../analysis/vehicles-tu15-goal-20261023/vehicle-joined-firing-regressions-v1/report.json)
passes both failures: Warthog MG records55 full projectile constructors and
one source kill; Spectre records41 and one source kill. Neither vitality nor
damage results are supplied. The read-only constructor observations also
restore the fixture's muzzle input reference. This is a fixture diagnostic
result on the joined epoch, not a claim that production gameplay was changed
to manufacture the outcome. The [complete joined repeat](../analysis/vehicles-tu15-goal-20261023/vehicle-joined-full-regression-v2/source-binding.json)
is now running with every declared source report required to be retained,
hash checked and passed. Its two temporary scopes must be removed and their
complete production preimages restored before launcher qualification.

The [joined-epoch ordinary Spectre game](../analysis/vehicles-tu15-goal-20261023/vehicle-spectre-ordinary-game-v1/report.json)
passes real walk/entry/fire input,900 frames, natural shutdown and actual
process audio. It boards the gunner at tick871 and fires through ticks961..1021,
observing16 generations of the actual weapon's authored projectile definition.
Resident projectiles and complete native contact events are distinct evidence;
unrelated definitions are excluded. The action interval records40 native
difficulty,20 material and787 model calls; the current-difficulty wrapper is
zero. Chassis displacement is zero in this gunner-seat check. The [inspection
and cleanup receipt](../analysis/vehicles-tu15-goal-20261023/spectre-ordinary-inspection-and-cleanup-v1.json)
retains a required482,646-byte PNG and verifies removal of357,489,992 temporary
dependency bytes,44,098,560 raw PCM bytes and the private sink. The image shows
the seated Spectre gunner, turret/chassis, reticle, HUD and terrain. Original
pixels, waveforms and full Spectre lifecycle parity remain open. The candidate
still contains two inactive diagnostic scopes that must be removed for delivery.

The [ordinary Gauss Warthog game](../analysis/vehicles-tu15-goal-20261023/vehicle-gauss-ordinary-game-v1/report.json)
also passes real gunner entry/fire,900 frames, natural shutdown and actual
process output. Its one authored projectile generation coincides with two
native difficulty, one material and714 model calls. Its [inspection/cleanup](../analysis/vehicles-tu15-goal-20261023/gauss-ordinary-inspection-and-cleanup-v1.json)
retains a required474,231-byte PNG and verifies removal of357,489,992 temporary
dependency bytes,39,697,920 raw PCM bytes and the private sink. This gunner
check claims no chassis motion or original-pixel/waveform equality.
The first ordinary Wraith approach fails before boarding: the player is within
the authored2-unit entry range but the input probe faces its walking point.
The retry targets the actual entry marker and holds normal Use input. No
gameplay admission or state is supplied. Subsequent probes freeze their exact
input worker and check the imported helper hashes before/after execution.
The marker-targeting retry also stops at the Wraith's front corner. The next
probe first walks around the authored hull radius to the entry's outer side,
then approaches the real marker. That [ordinary Wraith run](../analysis/vehicles-tu15-goal-20261023/vehicle-wraith-ordinary-game-v3/report.json)
passes entry/drive/fire,6.26922 world units of motion, one authored cannon
projectile,900 frames, actual process output and natural shutdown. Its native
action counts are two difficulty, one material and775 model calls.255 retained
observations carry four frag grenades, exercising the original maximum over
protocol103. Its [inspection and cleanup](../analysis/vehicles-tu15-goal-20261023/wraith-ordinary-inspection-and-cleanup-v1.json)
retains a required488,512-byte PNG and verifies removal of357,489,992 temporary
dependency bytes,54,773,760 raw PCM bytes and the sink. The [two failed approach
cleanup receipt](../analysis/vehicles-tu15-goal-20261023/wraith-approach-failures-cleanup-v1.json)
retains844,352 required image bytes and confirms removal of their owners/PCM.
The image shows the Wraith driver/cannon, HUD, terrain and four-grenade count;
original pixel/waveform equality and full Wraith lifecycle remain open.

The [joined complete cohort](../analysis/vehicles-tu15-goal-20261023/vehicle-joined-full-regression-v2/report.json)
now passes374/374 in1817.19 seconds, without skips. Every declared source
report is retained, hash checked and passed; the frozen source is unchanged
and the incoming private CMake registrations are restored. This result covers
327 constructors,30 destruction cases, nine primary weapons, two secondary
weapons, Ghost boost and five ownership/BV checks. Original pixels, waveforms,
every authored lifecycle and complete playability remain separate gates.

The [stationary-human ordinary game retry](../analysis/vehicles-tu15-goal-20261023/vehicle-choice-turret-game-v6/report.json)
passes normal walking/jumping onto the Coagulation platform, entry,90 neutral
ticks, firing20 authored projectile generations,2100 frames, actual process
audio and natural shutdown. The input probe now advances across floor
waypoints while airborne and jumps only after actual movement stalls;
gameplay state/poses/admission are never supplied. The action interval records
40 native difficulty,20 material and743 model calls; native Random selection
is separately observed in that same process. The [inspection/cleanup receipt](../analysis/vehicles-tu15-goal-20261023/stationary-human-ordinary-inspection-and-cleanup-v1.json)
retains the required rendered PNG and verifies removal of its observer, PCM
and sink. It shows the seated turret gunner on the roof with HUD and terrain.
The Covenant stationary variant is now being checked through normal input.
That [Covenant stationary game](../analysis/vehicles-tu15-goal-20261023/vehicle-covenant-turret-ordinary-game-v1/report.json)
now passes normal entry/fire, eight authored projectile generations,2100
frames, actual process audio and natural shutdown. Position is unchanged
during firing. The action interval records16 native difficulty, eight material
and881 model calls. The [inspection and cleanup](../analysis/vehicles-tu15-goal-20261023/stationary-covenant-ordinary-inspection-and-cleanup-v1.json)
retains a required427,942-byte PNG and verifies removal of357,489,992 temporary
dependency bytes,50,979,840 raw PCM bytes and the sink. Original waveforms and
the known missing Classic Phantom AP flyby resource remain open.

The first ordinary Banshee probe exhausts its900 frames before reaching the
parked aircraft. It supplies no admission and remains failed; the longer
2100-frame retry uses a collision-floor walking route where available.
The [longer ordinary Banshee check](../analysis/vehicles-tu15-goal-20261023/vehicle-banshee-ordinary-game-v2/report.json)
passes normal approach/entry,18 authored projectile generations,2100 frames,
actual process audio and natural shutdown. The60-tick action moves1.67616
world units; the final observed aircraft is airborne at height15.4553.
This does not claim a full flight envelope. Its native action counts are40
difficulty,20 material and846 model calls. The [inspection/cleanup](../analysis/vehicles-tu15-goal-20261023/banshee-ordinary-inspection-and-cleanup-v1.json)
retains a required677,717-byte PNG and verifies removal of357,489,992 temporary
dependency bytes,49,006,080 raw PCM bytes and the sink. The [failed first
approach cleanup](../analysis/vehicles-tu15-goal-20261023/banshee-first-approach-cleanup-v1.json)
retains its required384,189-byte diagnostic image and confirms removal.

The [production restoration](../analysis/vehicles-tu15-goal-20261023/vehicle-joined-production-restoration-v1/prepared.json)
removes both B9B70/BA490 scopes and restores the complete14e3af8e... native
support preimage and canonical CMake hash065719bc.... The first launcher
invocation stops before building/publishing because its selected-package
client-config path is absent. The preserved7fefd88c... configuration is then
verified at hashb02a15cc... and reused. The [production launcher build](../analysis/vehicles-tu15-goal-20261023/launcher-delivery-v5/validation/report.json)
passes configure, build and routing tests at two compiler/Rust jobs. All four
repeated audio scenarios pass. Submitted-frame and frontend qualification are
running; the currently selected runtime remainsac5fb37c....

An earlier [366-check checkpoint](../analysis/vehicles-tu15-goal-20261023/current-native-summary-v3.json)
passes every constructor, destruction, primary damage, secondary input, native
boost and strict BV check plus both portal ownership regressions. CTest returned
zero for all366 cases. Its source-bound wrapper initially rejected the newer
CTest summary spelling; the [bounded parser repair](../analysis/vehicles-tu15-goal-20261023/current-native-validation-v3/summary-parser-repair.json)
checks every numbered passed-test row and preserves the original report. No
test assertion, result or threshold changed.

The rendered [ten-input cohort](../analysis/vehicles-tu15-goal-20261023/current-rendered-firing-v1/report.json)
passes nine cases with actual SDL input, native projectiles, authored visual
submission and matching Classic PCM voice counts. Wraith fails during contrail
rendering. Its [recovered inputs](VEHICLE_CONTRAIL_SNAPSHOT_TU15.md) now have
independent red/green and negative-control checks: two live CRT policy words
and the consumed transition lookup rows. The initial16-row export was invalid:
it included mutable placement state. The full cold-world ownership guard
rejected it before promotion; the corrected extent contains only authored
types1..7. The [fresh delivery snapshot](../analysis/vehicles-tu15-goal-20261023/delivery-snapshot-v1/refresh.json)
includes155 changed files and preserves all31 original variant-map hashes in the
current selected cache. Its [371-check build/run](../analysis/vehicles-tu15-goal-20261023/delivery-native-validation-v1/report.json)
built successfully but was stopped after the cold-world conflict. A corrected
combined-source run must pass before the rendered reruns and launcher
promotion. Full vehicle acceptance remains open.

The [four-check rebuilt ownership checkpoint](../analysis/vehicles-tu15-goal-20261023/delivery-ownership-validation-v1/report.json)
passes world/branch ownership, contrail snapshots and the actual shared material
bank. Curve export now preserves seven separate1,024-byte row owners. This
checkpoint deliberately excludes the full vehicle cohort and cannot qualify
launcher delivery by itself.

The corrected-data [full371-case run](../analysis/vehicles-tu15-goal-20261023/delivery-native-validation-v3/report.json)
finishes370/371: all327 constructor contexts,26 destruction cases, nine primary
damage paths, both additional-input cases, Ghost boost, strict native BV and all
four ownership checks pass. The Wraith-minigun fixture asks the multiplayer
variant for a child it does not author. The retained original default variant
declares a mortar and two miniguns; the current destruction fixture constructs
that named variant through the real initializer/constructor and observes the
actual native damage-record/model owners. The next coherent source freeze
must integrate that fixture and newer native damage kernels before delivery.
Rendered startup additionally requires the narrowly measured compile-policy
candidate described in [the contrail/startup investigation](VEHICLE_CONTRAIL_SNAPSHOT_TU15.md).

The [next coherent snapshot](../analysis/vehicles-tu15-goal-20261023/delivery-snapshot-v2/refresh.json)
includes70 changed files and retains all31 variant-map hashes. Its30 destruction
cases additionally include the campaign Sentinel emitter, default Wraith mortar
and authored guntower no-damage-information gate. The [native damage binding](../analysis/vehicles-tu15-goal-20261023/delivery-native-damage-source-bindings-v1.json)
matches all six current native record/model module/header hashes to their
existing unchanged-original qualification. The new374-check cohort must qualify
this combined source; preparation and those prior component proofs are not a
completed runtime result.

The rebuilt production-default [Wraith firing/audio run](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-rendered-v6/report.json)
passes90 real Vulkan frames, one native mortar projectile, authored trails and
one Classic voice submission. Its own live Pulse stream supplies956,415 nonzero
PCM samples after explicit routing to the private monitor. The monitor and
46,794,240 raw PCM bytes are removed, preserving the desktop default. This is
actual host-output evidence; physical-speaker audibility and original waveform
or pixel comparisons remain open. The preceding failed monitor run and silent
SDL routing diagnostic remain recorded in the startup investigation.

The [completed374-case cohort](../analysis/vehicles-tu15-goal-20261023/delivery-native-summary-v4.json)
passes373 cases:327 constructors,29 destruction cases, nine primary target-damage
paths, both additional-input cases, native Ghost boost, strict BV and four
ownership checks. The guntower no-damage-information check fails because its
required dispatcher/model lookup is not observed. A fixture-only caller filter
still fails and is preserved as a separate result; filtering unrelated periodic
lookups alone cannot establish the missing dispatcher evidence.

The shared source's [qualified paired native lookup integration](../analysis/destruction-goal-20261022/model-lookup-paired-native-v1/admission.json)
also routes the direct-damage and vitality modules' additional copies through
the same native kernel. Its existing192 original comparisons and four live
controls include the guntower negative case. [Exact source bindings](../analysis/vehicles-tu15-goal-20261023/delivery-paired-model-source-bindings-v1.json)
authenticate those two modules and their three generation helpers before the
[scoped integration](../analysis/vehicles-tu15-goal-20261023/delivery-native-stage-v6/refresh.json).
The [fresh full-cohort run](../analysis/vehicles-tu15-goal-20261023/delivery-native-validation-v5/report.json)
must pass before launcher qualification. Prior passes remain evidence for their
own source snapshots.

That run is deliberately stopped after the guntower fails again. The missing
route is in the actual-address live-support copy, beyond the two paired module
copies. The [narrow installation](../analysis/vehicles-tu15-goal-20261023/delivery-native-stage-v7/refresh.json)
uses the shared authenticated installer: it checks the complete old lookup-body
hash and adds only its native route/include, preserving every other frozen body.
The [focused rebuild](../analysis/vehicles-tu15-goal-20261023/delivery-guntower-native-v2/report.json)
must pass the actual dispatcher rejection before another complete cohort run.

That focused rebuild passes: two full-generation target requests return zero
from the native lookup with original caller return `DA180`, retain the authored
zero damage-information count and leave vitality, regions, generation and death
state unchanged. Exact complete-world continuation passes. The [independent route audit](../analysis/vehicles-tu15-goal-20261023/delivery-live-model-original-route-audit-v1.json)
authenticates all24 original instructions and preservation of2,026 existing
literal bodies. The [route closure](../analysis/vehicles-tu15-goal-20261023/delivery-native-model-route-closure-v1.json)
checks all four TU1.5 compositions before future cohort builds and delivery.
The [generator-only refresh](../analysis/vehicles-tu15-goal-20261023/delivery-native-stage-v8/refresh.json)
preserves the installer on regeneration; the [new complete run](../analysis/vehicles-tu15-goal-20261023/delivery-native-validation-v6/report.json)
must still qualify every declared case together.

The [complete374-case result](../analysis/vehicles-tu15-goal-20261023/delivery-native-summary-v6.json)
passes with every retained case's hash/freshness and the frozen source binding
verified:327 constructors,30 destruction cases, nine primary target-damage
paths, two additional inputs, native boost, strict BV and four ownership checks.
This is the current combined native checkpoint, without a full-parity claim.

The [fresh rendered sweep](../analysis/vehicles-tu15-goal-20261023/delivery-rendered-firing-v2/report.json)
completes all ten cases and verifies nonzero PCM from each actual game process.
Seven pass; Banshee, machine-gun Warthog and the human stationary turret fail
the firing-only audio ownership assertion on extra vehicle-owned submissions.
All ten retain actual projectile and Classic firing-voice counts. These three
remain failures while private diagnostics record the extra sound identities
without removing assertions. The [image inspection/cleanup receipt](../analysis/vehicles-tu15-goal-20261023/delivery-rendered-inspection-and-cleanup-v2.json)
records all ten inspected final frames, their visibility limits, exact retained
image sizes, private sink removal and raw-output cleanup. Original pixels,
physical speakers and full audio lifecycle parity remain open.

The [Banshee diagnostic](../analysis/vehicles-tu15-goal-20261023/delivery-banshee-audio-rejections-v1/report.json)
identifies the unmatched voices as Classic left/right engine startup/loop
packets with `Ambient` kind and the actual vehicle owner. The [Warthog diagnostic](../analysis/vehicles-tu15-goal-20261023/delivery-warthog-audio-rejections-v1/report.json)
identifies Classic chain-gun ammo-rattle loop packets. The original firing-only
assertion misclassified these attached-loop packets as unmatched firing cues.
The repaired observer still checks real PCM voices, Classic resources and live
full emitter generations, records loop packets separately, and retains the
exact source firing-cue/native firing-voice equality and local firing-view gate.

All three [targeted reruns](../analysis/vehicles-tu15-goal-20261023/delivery-typed-loop-audio-v1/report.json)
pass with actual host output. Their private fixture changes only that observer;
every other frozen assertion/input and every borrowed gameplay/render library
remains unchanged. The [combined ten-case acceptance](../analysis/vehicles-tu15-goal-20261023/delivery-rendered-combined-acceptance-v1.json)
joins those results with the seven unchanged passes. The previous failures are
preserved. Typed engine/spin-loop observations are not a full lifecycle or
waveform comparison.

The [production preparation](../analysis/vehicles-tu15-goal-20261023/delivery-canonical-v1/prepared.json)
removes the private constructor/deletion observers and appended registrations.
Its [restoration preflight](../analysis/vehicles-tu15-goal-20261023/delivery-native-canonical-restoration-preflight-v1.json)
proves every other original byte is retained while keeping the qualified native
model-lookup route. The launcher build/qualification is recorded under
[`launcher-delivery-v1`](../analysis/vehicles-tu15-goal-20261023/launcher-delivery-v1/).
Preparation alone has not yet promoted a launcher runtime.

That launcher attempt builds successfully but rejects promotion when Lockout
reaches the missing sound-scenery angle owner `18B580`. The [scoped audio refresh](../analysis/vehicles-tu15-goal-20261023/delivery-audio-refresh-v1/prepared.json)
admits the independently recovered complete routine and exact readonly angle
constant, preserving every unrelated frozen source. Its [focused original and ownership tests](../analysis/vehicles-tu15-goal-20261023/delivery-audio-native-v1/report.json)
pass. The [actual Lockout rerun](../analysis/vehicles-tu15-goal-20261023/delivery-lockout-audio-v2/report.json)
passes533 source ticks, three reload packets,49 ambient packets and24 matching
source/client SMG impulses. An earlier rerun used an incorrect map directory
and skipped; it remains recorded. The [second launcher qualification](../analysis/vehicles-tu15-goal-20261023/launcher-delivery-v2/)
must complete before delivery can be claimed.

Two further semantic native kernels implement complete original `1F1950` and
`1F19B0` difficulty parameter selection. Their [unchanged-original qualification](../analysis/vehicles-tu15-goal-20261023/native-difficulty-v5/report.json)
passes528 declared inputs, including40 original read faults, all33 static
instructions, ordered reads, complete GP/flags/SSE state and unchanged memory.
The [negative control](../analysis/vehicles-tu15-goal-20261023/native-difficulty-negative-v2/report.json)
rejects a deliberate lower-clamp error. [Four scoped source routes](../analysis/vehicles-tu15-goal-20261023/native-difficulty-routes-v1/admission.json)
preserve every retained literal body and its original observation API. Rebuilt
live gameplay and launcher admission of these newer kernels remain required;
the in-progress launcher snapshot predates them.

The [current-source difficulty result](../analysis/vehicles-tu15-goal-20261023/delivery-difficulty-summary-v1.json)
passes35 checks: all30 destruction scenarios and five ownership/original-data
checks. Every destruction case records positive whole-run native parameter
activity;25 cases also record positive explicit firing intervals. The other
fixture branches retain their broader counter scope. All retained reports and
the unchanged production source manifest are authenticated. This is additional
evidence for this source epoch; it does not rerun all327 constructor contexts.

The [bounded Banshee ascent/firing check](../analysis/vehicles-tu15-goal-20261023/delivery-banshee-flight-v2/report.json)
passes180 frames,110 actual forward/jump input ticks, a0.6-radian upward look,
1.172 world units of rise and41 full live projectile generations with matching
Classic firing voices and actual host PCM. The first run's mouse event preceded
capture, and its construction counter required a private hook absent from the
production build. The repaired observer intersects completed native frame data
with full live source datum ownership and retains every firing/audio assertion.
The [inspection/cleanup receipt](../analysis/vehicles-tu15-goal-20261023/delivery-banshee-flight-inspection-and-cleanup-v1.json)
preserves both results, verifies private storage/audio cleanup and records the
partly occluded final views. Full flight handling and original comparison remain
open.

The [next scoped source join](../analysis/vehicles-tu15-goal-20261023/delivery-material-refresh-v1/prepared.json)
also admits the shared hardware-qualified native material-damage owner. Removing
only the difficulty additions yields that owner's exact qualified module hash;
removing the material additions yields the current35-check module unchanged.
Both source preimages are preserved. Its live joined validation and subsequent
launcher rebuild remain required.

The [ordinary Ghost game](../analysis/vehicles-tu15-goal-20261023/delivery-ordinary-ghost-v5/report.json)
passes with actual XTest keyboard/mouse input and independent native LAN state:
walk around the vehicle hull, source driver-seat admission at tick883, drive and
fire at ticks973..1034,21 projectile generations and7.959 world units of motion.
The ordinary executable naturally completes900 rendered frames and its own
stream supplies nonzero PCM. The [frame inspection](../analysis/vehicles-tu15-goal-20261023/delivery-ordinary-ghost-frame-inspection-v5.json)
records the visible vehicle/driver/HUD. No host pose, velocity or admission is
written. Its temporary build, private sink and raw outputs are removed. This
qualifies that snapshot's bounded input/output path; original comparison and
launcher-installed acceptance remain separate gates.

The [four-family original reference](../analysis/vehicles-tu15-goal-20261023/four-vehicle-choice-original-v1/report.json)
adds the complete flying selector15CD30:3,444 inputs cover all signed-byte
choices and384 Random inputs across three maps, with all266 instructions
observed. Its required compressed authority fixture is42,816 bytes. The
[extended semantic kernel](../analysis/vehicles-tu15-goal-20261023/four-vehicle-choice-native-v2/report.json)
passes6,888 native/literal comparisons of complete GP/flags/FP state, ordered
accesses/stores and full memory banks. Both authored enabled flying branches
retain Banshee reference4 and variant0; the hidden Random branch retains its
own draw. The [light-range](../analysis/vehicles-tu15-goal-20261023/four-vehicle-choice-negative-light-v2/report.json)
and [flying-reference](../analysis/vehicles-tu15-goal-20261023/four-vehicle-choice-negative-flying-v2/report.json)
negative controls reject their deliberate mutations. The [four installed source routes](../analysis/vehicles-tu15-goal-20261023/four-vehicle-choice-installed-v2/report.json)
pass the same6,888 comparisons with native-call assertions. A separate flying
counter permits actual-process evidence in the next Banshee game. Current-game
validation and launcher delivery of the added flying kernel remain pending.

The [fresh source epoch](../analysis/vehicles-tu15-goal-20261023/vehicle-next-epoch-native-v1/report.json)
builds the native application and passes all eight declared checks, including
six240-tick authored match cases. Its production source remains unchanged.
The [scoped flying refresh](../analysis/vehicles-tu15-goal-20261023/vehicle-flying-epoch-preparation-v1/prepared.json)
then changes only the qualified selector/header/generator owners and declared
test registrations. The [nine-check run](../analysis/vehicles-tu15-goal-20261023/vehicle-flying-epoch-native-v1/report.json)
passes, including seven240-tick actual match cases. Flying On/Off each record
two flying-specific native calls; Flying Random records four calls and two
draws. Source hashes remain unchanged. The [normal-input Banshee game](../analysis/vehicles-tu15-goal-20261023/vehicle-flying-banshee-ordinary-game-v1/report.json)
passes actual entry,18 authored projectile generations,1.6739 units of movement,
1,159 rendered frames, actual game PCM and normal window close/exit0. The
flying-specific counter advances from0 at tick1 to2 by seat admission, then
stays2 during firing: this selector owns birth, rather than per-shot behavior.
The required340,287-byte frame shows the occupied aircraft in a cave passage;
this does not establish the complete flight envelope. Its [cleanup](../analysis/vehicles-tu15-goal-20261023/vehicle-flying-banshee-inspection-and-cleanup-v1.json)
verifies removal of359,259,344 dependency bytes,39,667,200 raw PCM bytes and
the private build/sink. Full regression and launcher delivery for this source
epoch remain pending. Two read-only constructor scopes are now installed only
inside the owned test copy for the primary-firing regression; they must be
removed before delivery. The active owned source/build is
`.local/test-scratch/halo2-test-aagykchy`; it is temporary investigation storage.

The [first full regression build](../analysis/vehicles-tu15-goal-20261023/vehicle-flying-full-regression-v1/report.json)
fails before CTest at the world-memory executable: the low-level projectile
reader references effect readback archived only in the presentation library.
The [ownership split](../analysis/vehicles-tu15-goal-20261023/effect-readback-ownership-split-v1.json)
preserves every complete readback/classification and lens-opacity body while
moving pool readback to the physics library. A focused link then exposes its
missing binary-reader dependency. The [second scoped refresh](../analysis/vehicles-tu15-goal-20261023/vehicle-effect-readback-owner-preparation-v2/prepared.json)
declares the actual cache-library dependency. The [focused world-memory/lens
checks](../analysis/vehicles-tu15-goal-20261023/effect-readback-owner-focused-v2/report.json)
now both pass, with no skip. The [repaired source's nine-check run](../analysis/vehicles-tu15-goal-20261023/vehicle-effect-readback-owner-native-v1/report.json)
and [complete374-check regression](../analysis/vehicles-tu15-goal-20261023/vehicle-flying-full-regression-v2/report.json)
pass. All327 constructor contexts,30 destruction cases and17 remaining checks
pass without skips; source hashes stay unchanged and private CMake registration
is restored. Its [production restoration](../analysis/vehicles-tu15-goal-20261023/vehicle-flying-production-restoration-v1/prepared.json)
removes both complete diagnostic scopes and restores canonical CMake bytes.
The [sixth launcher build/qualification](../analysis/vehicles-tu15-goal-20261023/launcher-delivery-v6/report.json)
fails during packaging because the checkout bundler now requires a dedicated
EOS worker absent from this qualified frozen epoch. Its [diagnosis](../analysis/vehicles-tu15-goal-20261023/launcher-delivery-v6-packaging-diagnosis.json)
pins both tool/source epochs. The [seventh qualification](../analysis/vehicles-tu15-goal-20261023/launcher-delivery-v7/report.json)
uses the matching frozen bundler. It passes all four audio cases, thirteen
rendered configurations and frontend checks, then publishes both launchers.
Its delivered source manifest exactly matches the restored production manifest.

The [AP sound import contract](../analysis/vehicles-tu15-goal-20261023/classic-phantom-import-contract-v1.json)
authenticates31 original campaign/multiplayer contexts and finds one complete
normalized contract: literal flags0x1c02,44.1kHz mono Xbox ADPCM, authored
playback/scale/class/pitch/permutation metadata and six encoded resource hashes.
It retains each map's literal indices and resource locations separately.
This establishes import inputs; a native resource owner and live admission
remain required. Its [temporary owner is removed](../analysis/vehicles-tu15-goal-20261023/classic-phantom-import-contract-cleanup-v1.json).
The [prepared original data bank](../analysis/vehicles-tu15-goal-20261023/classic-phantom-prepared-bank-v1.json)
at `assets/imported/tu15-vehicle-sound-20261024-v1` retains90,160 bytes: a
manifest and78,336 bytes of authenticated encoded audio plus exact original
sector tails. Its six declared payloads total76,860 bytes. It includes no
executable and changes no selected cache/runtime. The [standalone native decode](../analysis/vehicles-tu15-goal-20261023/classic-phantom-prepared-bank-native-v1/report.json)
reads this prepared bank without reading the original map/resource files and
reproduces all six earlier original-resource PCM hashes exactly. Its [temporary
decoder/build/PCM copies are removed](../analysis/vehicles-tu15-goal-20261023/classic-phantom-prepared-bank-native-v1/cleanup.json).
The [native C++ bank owner](../analysis/vehicles-tu15-goal-20261023/classic-phantom-native-bank-owner-v3/report.json)
now directly reads the admitted data asset, reproduces all six original PCM
hashes, rejects ten malformed/integrity inputs and enforces two pitch/permutation
bounds. It returns immutable original records and sector data; its literal
ordinals still require proper runtime relocation. Its [private compiler/PCM and
malformed-bank copies are removed](../analysis/vehicles-tu15-goal-20261023/classic-phantom-native-bank-owner-cleanup-v1.json).
This reader is current source work beyond the frozen flying candidate. Native
tag binding and live playback acceptance remain open.
The [actual canonical metadata inspection](../analysis/vehicles-tu15-goal-20261023/classic-phantom-canonical-metadata-v3/report.json)
loads Coagulation and Delta Approach through the real native Cache/delivery
planner. The [paired original delta](../analysis/vehicles-tu15-goal-20261023/classic-phantom-native-import-delta-v1.json)
confirms that the nearby Classic sound's playback gain is-3dB versus original
AP-12dB; its native codec and encoded extents also differ. The remastered donor
has additional pitch, gain, duration and permutation-name differences. These
results rule out closing AP parity by renaming or replacing payload bytes alone.
The [expanded native inspection](../analysis/vehicles-tu15-goal-20261023/classic-phantom-all-canonical-metadata-v3/report.json)
passes all31 map contexts. Complete AP projectile hashes and full-salt typed
references are retained. Every AP projectile selects the remastered donor;
the distinct Phantom projectile selects the separate Classic donor. This is
reference ownership evidence, not live import or playback admission.

The [second original contract](../analysis/vehicles-tu15-goal-20261023/classic-phantom-import-contract-v2.json)
also recovers the actual initial permutation-used byte. All31 original maps
contain00; the earlier contract omitted this owner. A [V2 prepared bank](../analysis/vehicles-tu15-goal-20261023/classic-phantom-prepared-bank-v2.json)
retains that authenticated byte with the unchanged exact resource sectors.
The [extended native reader check](../analysis/vehicles-tu15-goal-20261023/classic-phantom-native-bank-owner-v4/report.json)
passes all six exact original PCM comparisons, twelve malformed-bank refusals
and two pitch/permutation boundary controls. V1 remains readable but exposes
missing used-state as unavailable. Neither version invents live selection
state, handles or readiness. The [cleanup receipt](../analysis/vehicles-tu15-goal-20261023/classic-phantom-all-context-cleanup-v1.json)
verifies removal of all six probe owners/builds/raw copies. The90,192-byte V2
bank is retained as a required input; the earlier bank is preserved. Live
native metadata relocation, original-resource binding and output remain open.

## Source and scope

Original English Xbox TU1.5 supplies gameplay and presentation authority.
The original executable is an offline oracle; production must execute native
compiled kernels. Prepared MCC caches supply imported runtime assets. Unknown
behavior and absent Classic resources stay unresolved until their original
owners or exact source resources are established.

The current census is
[`inventory-v2/inventory.json`](../analysis/vehicles-tu15-goal-20261023/inventory-v2/inventory.json).
It pins the original maps, prepared cache and field schema, inventories all
**37 vehicle definitions**, and records **327 distinct authoring/selection
contexts**: 238 named variants and 89 default constructor contexts. The
default rows include vehicles whose model has no named variant bank. Vehicle
contracts differ between maps, so one passing default vehicle cannot close
every campaign or multiplayer context sharing its name.

The sharded `variants-*.json` files contain the per-context acceptance gates.
Every gate begins open. Script/AI-only objects must retain original eligibility;
human seat admission is required only where the original game permits it.
Authored non-destructibility and absent weapons require positive original
evidence, rather than being treated as missing functionality or skipped.

[`variant-cases-v2/index.json`](../analysis/vehicles-tu15-goal-20261023/variant-cases-v2/index.json)
materializes all 327 constructor input contexts across 31 representative MCC
maps. This is test preparation. `vehicle_variants_world_tu15_tests` invokes the
real request initializer and constructor, reads native model/child state and
uses ordinary deletion. It supplies no admission result. Campaign-only rows
require an authored campaign observer/BSP and remain unresolved when unavailable.

## Required evidence for each context

| Gate | Required acceptance |
| --- | --- |
| Native creation and eligibility | Original constructors, variant/child selection, activation and positive/negative admission behavior |
| Controls, seats and motion | Appropriate player/AI/script controls, entry/exit, occupants, suspension, contacts and motion |
| Weapons and gameplay effects | Every authored barrel/input, aim, timing, projectiles, collisions, damage and attribution |
| Visuals | Models, materials, animation, damage permutations, lights, particles, trails, decals and remote views |
| Audio | Authored events and lifecycle, Classic resources, loops/transitions, spatial playback and actual output |
| Destruction | Body/region responses, occupants, explosion interactions, wrecks, debris and retirement |
| Continuation and replication | Complete source state across restore and real remote gameplay/presentation |
| In-game comparison | Actual input-driven native game behavior compared with original TU1.5 evidence |
| Launcher delivery | Rebuilt qualified runtime, current cache/launcher routing and repeatable in-game acceptance |

## Existing bounded evidence

- [`TURRETS_TU15_GOAL_CHECKLIST.md`](TURRETS_TU15_GOAL_CHECKLIST.md)
  records six native stationary/attached turret cases, actual projectile damage
  and source kill credit, rendered effects and authored PCM voice requests.
  Those cases do not establish complete vehicle or original framebuffer/audio
  parity. The exact Classic Phantom turret flyby resource remains absent from
  the inspected MCC banks. The [exact encoded-resource search](../analysis/vehicles-tu15-goal-20261023/classic-phantom-encoded-bank-search-v1.json)
  finds none of its six original payloads in the selected neutral/English banks.
  The [literal Phantom census](../analysis/vehicles-tu15-goal-20261023/classic-phantom-definition-census-v1.json)
  finds a nearby Classic flyby using48kHz Opus, while the original AP definition
  uses44.1kHz Xbox ADPCM. No resource/metadata alias or decoded waveform equality
  is inferred from these format/presence checks. The original exact bytes remain
  available as offline reference inputs; runtime import/admission stays open.
  A [native waveform comparison](../analysis/vehicles-tu15-goal-20261023/classic-phantom-native-waveforms-v2/report.json)
  decodes all six original and six nearby MCC permutations with unchanged
  production decoder sources. Corresponding sounds correlate strongly after
  a312/313-frame offset at48kHz, but all differ in samples and duration. The
  MCC donor therefore cannot close exact AP waveform parity. The [cleanup](../analysis/vehicles-tu15-goal-20261023/classic-phantom-native-waveform-cleanup-v1.json)
  verifies both private decoder builds and all encoded/PCM copies were removed.
- [`VEHICLE_DESTRUCTION_EXPLOSIVES_GOAL.md`](VEHICLE_DESTRUCTION_EXPLOSIVES_GOAL.md)
  tracks the 37-definition destruction matrix, native component and complete
  continuation comparisons, unresolved original response flags and rendered LAN
  destruction acceptance.
- [`PHYSICS_TU15_REMAINING_CHECKLIST.md`](PHYSICS_TU15_REMAINING_CHECKLIST.md)
  and [`PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md`](PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md)
  track motion, seat, contact and campaign ownership evidence and remaining work.
- [`WEAPON_VEHICLE_AUDIO_20261022.md`](WEAPON_VEHICLE_AUDIO_20261022.md)
  tracks the larger authored audio matrix and the distinction between resource
  decode/request proof and complete audible lifecycle parity.

## Active native cohort

[`VEHICLE_NATIVE_RATE_KERNELS_TU15.md`](VEHICLE_NATIVE_RATE_KERNELS_TU15.md)
records two structured native scalar controller owners and their original
comparison, cursor, refusal and write-fault qualification. Their current
component evidence does not qualify the pending full game build.

[`baseline/report.json`](../analysis/vehicles-tu15-goal-20261023/baseline/report.json)
pins the private frozen source. It is preparation, not acceptance.
[`cohort-v1/prepared.json`](../analysis/vehicles-tu15-goal-20261023/cohort-v1/prepared.json)
records unchanged-body native compilation partitions, the pinned peer
dependency, private authored bundle data and test registration. The official
test worker builds the native game, live multiplayer acceptance, nine vehicle
weapon cases and 21 destruction/interaction cases. Their reports must be read
individually before any scoped qualification.

The first build stopped at the production bundler's rejection of a borrowed
`data` symlink. The owned build now contains verified private regular copies of
the four small authored data files; original data is preserved. The retry is
recorded in
[`baseline-validation-v2/report.json`](../analysis/vehicles-tu15-goal-20261023/baseline-validation-v2/report.json).

That retry later stopped because the unchanged-body partitioner supports
`WorldState` routines, while the death-cycle translation uses `LiveState`.
[`cohort-v2/prepared.json`](../analysis/vehicles-tu15-goal-20261023/cohort-v2/prepared.json)
records restoration of the original whole-file native death-cycle build. No
death-cycle instruction changed. The permanent preparation helper partitions
only the supported AI frame module.

[`native-stage-v1/refresh.json`](../analysis/vehicles-tu15-goal-20261023/native-stage-v1/refresh.json)
authenticates the named kernel/test changes and all 327 copied case inputs.
The resumed
[`native-validation-v1/report.json`](../analysis/vehicles-tu15-goal-20261023/native-validation-v1/report.json)
reuses the existing build and registers 357 cases: 327 constructors, nine
vehicle weapon cases and 21 destruction/interaction cases. Compilation passed.
The first continuation runs exposed an undeclared external-clock difference:
the complete states matched after excluding only the documented clock fields.
Those results remain failures. The corrected
[`native-validation-v2/report.json`](../analysis/vehicles-tu15-goal-20261023/native-validation-v2/report.json)
reruns the complete cohort with `HALO2_DESTRUCTION_FIXED_CLOCK=1`; the
[`clock correction`](../analysis/vehicles-tu15-goal-20261023/clock-correction-v1.json)
records the explicit input and the original failure. The permanent cohort
preparer now declares this input. Registration and compilation do not close
acceptance gates.

The active source/build owner is recorded in
[`session-v1/session.json`](../analysis/vehicles-tu15-goal-20261023/session-v1/session.json).
It is temporary investigation storage under `tools/run_test.py`. Finish that
owner when its build is no longer needed and verify removal. Retain bounded
reports and source hashes. Installed launcher runtimes and original reference
inputs are preserved.

No full-parity gate has been closed by this inventory or preparation, and this
cohort has not yet promoted a launcher runtime.

## Installed-game baseline

The
[`launcher Banshee baseline`](../analysis/combat-presentation-audit-20261022/repair-installed-banshee/vehicles-baseline-20261023/report.json)
did **not** pass. The selected runtime remained hash-pinned and the launcher
selection did not change, but the finite 600-second debugger-driven probe
observed no gameplay tick, seat entry, authored audio request or submitted
frame. Its zero debugger exit status is not acceptance. The owned process and
scratch were removed. A shorter diagnostic rerun interrupts at its deadline
and records native thread backtraces to distinguish scene preparation,
resource waits and observer binding. This baseline does not validate the new
pending native kernels.

The
[`120-second backtrace`](../analysis/combat-presentation-audit-20261022/repair-installed-banshee/vehicles-baseline-backtrace-20261023/report.json)
observed one simulation tick and NVIDIA shader compilation during the first
frame. A
[`private cache-copy retry`](../analysis/combat-presentation-audit-20261022/repair-installed-banshee/vehicles-baseline-warm-20261023/report.json)
then reached original seat entry at tick28 and three firing input ticks. It
submitted both views but did not finish the finite flight/fire/audio capture.
Its final stack was in `vkResetCommandBuffer`, independently identified by
[`linked return-PC disassembly`](../analysis/vehicles-tu15-goal-20261023/installed-warm-graphics-return-pc.txt),
so this later pause must not be described as the original shader-compilation
pause. Both remain failed acceptance results. The copied30,647,544-byte cache
and private processes were removed; the source driver cache was unchanged.

The [`AMD device control`](../analysis/combat-presentation-audit-20261022/repair-installed-banshee/vehicles-baseline-amd-20261023/report.json)
exited at native device admission: the integrated device did not support the
required original D24S8 path with graphics, compute and presentation. No depth
format substitution was used, and this control provides no gameplay pass.

The rebuilt native candidate's
[`Banshee gameplay probe`](../analysis/combat-presentation-audit-20261022/repair-installed-banshee/vehicles-candidate-v1-20261023/report.json)
also failed its 600-second deadline. It reached 22 simulation ticks and the
declared player placement, but did not complete seat entry, firing, audio or
the final capture. Its final thread snapshot showed the render owner waiting
for a presentation copy, the copy worker waiting for work, and an active
NVIDIA shader worker. This single interrupted snapshot does not establish a
copy-worker deadlock. The test runtime and source hashes are pinned in
[`candidate-v1.json`](../analysis/vehicles-tu15-goal-20261023/candidate-v1.json).
The probe owner and private driver-cache copy were removed. The candidate has
not been promoted to the launcher.

## Completed first cohort and current refresh

The authenticated
[`first complete cohort summary`](../analysis/vehicles-tu15-goal-20261023/native-cohort-v2-summary.json)
records **21 destruction passes, nine primary-weapon passes and 298 of 327
constructor passes**. The other 29 cases failed: 20 Old Mombasa observer
surveys and nine Midship world initializations. The latter reach missing
original collision-agent owner `2F18D0` from `315A4C`, before isolated vehicle
construction. These failures remain open and retain their exact reports.

[`The second scoped refresh`](../analysis/vehicles-tu15-goal-20261023/native-stage-v2/refresh.json)
adds the qualified typed instruction mapping and surveys other actual authored
vehicle placements when a scripted aircraft has no clear walkable observer
floor. It supplies no admission or world result. The
[`integrated rebuild and rerun`](../analysis/vehicles-tu15-goal-20261023/native-validation-v3/report.json)
must establish native kernel admission through the real composition. Constructor
processes own independent source worlds and scratch, so this rerun permits four
parallel constructors; destruction and armed-seat cases remain serial.

That rerun completed all 327 constructor cases: **318 passed and nine Midship
cases failed** at the same original BV-agent owner. All 20 Old Mombasa observer
failures are resolved. The owned run was interrupted after that sweep to
integrate the independently qualified missing owner; ten fresh destruction
cases had passed, one was incomplete, and stale earlier reports are excluded by
the [pivot receipt](../analysis/vehicles-tu15-goal-20261023/native-v3-pivot-receipt.json).
This interrupted run is not a full-cohort pass.

The [native BV-agent recovery](VEHICLE_BV_AGENT_NATIVE_TU15.md) records five
complete original bodies, untouched original i386 instruction evidence, strict
native floating-point/memory comparison and a red-capable negative control.
The [third scoped refresh](../analysis/vehicles-tu15-goal-20261023/native-stage-v3/refresh.json)
integrates those owners and their original regression. The focused nine-case
Midship rerun [passes all ten registered checks](../analysis/vehicles-tu15-goal-20261023/native-bv-midship-v1/report.json),
including the strict original component comparison. The complete cohort still
requires a single updated run. The separate recovery owner and its raw banks
have been removed; its bounded original regression fixture is retained.

## Paired-gun damage scenes

The first extended Ghost/Banshee scenes failed before reaching target damage:
the Ghost's low muzzle hit terrain before a camera-clear target. Nearer Ghost
placement and ordinary Banshee departure inputs exposed a second fixture bug:
the aiming correction chased the previous projectile's muzzle while paired
guns alternated. Both weapons then hit only once and failed the unchanged
attribution/kill-credit requirement. The
[retained red diagnosis](../analysis/vehicles-tu15-goal-20261023/weapon-damage-aim-feedback-v1.json)
records the actual muzzle rays and subsequent shot offsets.

The corrected fixture uses the original seated camera as the ordinary look
reference for paired guns. The
[focused rerun](../analysis/vehicles-tu15-goal-20261023/weapon-damage-scene-v3/report.json)
passes both cases: target shields reach zero, health becomes negative and the
source shooter gains one kill. Production weapon/damage behavior and all
acceptance assertions are unchanged. The Banshee's bounded departure inputs
did not establish a takeoff/climb acceptance result; its positions are retained
in the report. Full flight remains a separate input-driven gate.

## Current source and delivery boundary

The [current source refresh](../analysis/vehicles-tu15-goal-20261023/current-snapshot-v1/refresh.json)
replaces the early private snapshot with a coherent current-source freeze,
including 174 changed files. Every one of the 31 variant map files matches the
inventory's exact bytes. The earlier private source copy was removed; original
assets, reference evidence and installed runtimes are preserved.

The [current registrations](../analysis/vehicles-tu15-goal-20261023/current-native-stage-v2/refresh.json)
declare 364 checks: 327 constructor contexts, 24 destruction cases, nine primary
target-damage cases, Banshee/Scorpion secondary inputs, Ghost boost and the strict
original BV-agent comparison. The
[updated native validation](../analysis/vehicles-tu15-goal-20261023/current-native-validation-v2/report.json)
passes all364 checks. The [per-case audit](../analysis/vehicles-tu15-goal-20261023/current-native-summary-v2.json)
checks every retained/live hash and report time against that run. Banshee input2 in multiplayer remains suppressed; a positive
projectile count would fail that original-mode requirement. The full evidence
summary checks report hashes and modification times so stale prior outputs
cannot supply acceptance, and checks actual native rate admission during boost.

The [portal input repair](VEHICLE_LIGHT_PORTAL_LITERAL_TU15.md) recovers the exact
eight-byte readonly comparison operand and four-byte writable bounds alias
required by actual heavy-vehicle lighting. It imports no produced bounds or
lighting result. Both native ownership regressions pass. The
[366-case current rerun](../analysis/vehicles-tu15-goal-20261023/current-native-validation-v3/report.json)
adds those regressions and binds qualification to the exact source receipts.

Before launcher qualification, `prepare_vehicle_goal_delivery_tu15.py` requires
that complete source-bound cohort to pass, then removes only the declared private entry
observers and appended CMake registrations. Every restored production byte must
match the original canonical source freeze. It supplies no gameplay result.
The [delivery preference receipt](../analysis/vehicles-tu15-goal-20261023/launcher-delivery-preferences-v1.json)
pins the cache, map, player count and packaged EOS client configuration at that
checkpoint. Other active work has since advanced launcher/cache selection;
reread it before delivery and preserve its current settings. The component
cohort has no EOS SDK enabled; final launcher
delivery must preserve the installed configuration and qualify that build.

The [rebuilt candidate gameplay probe](../analysis/vehicles-tu15-goal-20261023/current-game-candidate-v1/report.json)
uses a private, hash-verified executable copy while the official test suite runs.
Its temporary executable, raw captures and driver cache belong to a separate
runner. Default presentation and any explicit serial-copy debugger control are
reported separately. Neither a submitted frame nor a control-path pass closes
the original framebuffer, physical audio or full vehicle gates.

## Additional rendered evidence

The [current nine-family native captures](../analysis/vehicles-tu15-goal-20261023/current-model-captures-v4/report.json)
pass actual source lighting/model submission, visible triangle centers and
bit-exact lighting/geometry inputs over real UDP, including Scorpion and Wraith
after their missing native inputs were recovered. The
[image inspection](../analysis/vehicles-tu15-goal-20261023/current-model-frame-inspection-v2.json)
records the exact retained frames and visibility limits. Banshee and both fixed
turrets have partial silhouettes because of base architecture; foreground
weapons overlap parts of the other vehicles. These are single views at one
resolution, without matched Xbox pixels. Their5,580,941 bytes of required PNG
reference evidence remain; raw frames, binaries and the capture owner are gone.

The [rendered firing cohort](../analysis/vehicles-tu15-goal-20261023/current-rendered-firing-v1/report.json)
uses the exact current native GPU executable, ordinary seat admission and SDL
button events for nine primary cases plus the Scorpion secondary barrel. Each
case must complete90 or180 actual frames, original projectile creation,
nonempty authored trail/glow submission where declared and exact source-to-PCM
Classic voice counts. Its per-case results remain authoritative; registration
does not establish a pass. Actual speakers and matched original audio remain
separate acceptance gates.

The
[`LAN Warthog run`](../analysis/vehicles-tu15-goal-20261023/lan-destruction-warthog-v1/reports/lan-vehicle/report.json)
passes actual SDL driving/exit, ordinary weapon/blast input, original death and
debris creation, 1,094 submitted two-view frames and 1,766 complete authority
comparisons. Thirteen debris parts are submitted in each view. No frame was
retained or inspected in this run, so those submission counts do not certify
pixel visibility, original visual comparison or audio. Its scratch and private
pipeline bank were removed; bounded reports and compressed input/body traces
remain.

The Banshee
[`serial-copy debugger control`](../analysis/vehicles-tu15-goal-20261023/serial-copy-banshee-control-v4.json)
completes seat entry, flight, 118 firing-input ticks, projectile publication and
Classic PCM voice submission in 18.48 seconds. It uses the existing serial
immutable-vertex-copy reference control; ordinary defaults were unchanged.
The 254,328-byte top-view PNG is explicitly retained as required visual
reference evidence. Inspection identifies blue projectile glow above the
Banshee nose, with geometry, sky and HUD present. This crop is not an Xbox
framebuffer comparison. The debugger stops after its finite capture, so natural
shutdown is unverified. Raw captures and the probe owner were removed.

The
[`ordinary non-debugger control`](../analysis/vehicles-tu15-goal-20261023/ordinary-game-copy-worker-v1/report.json)
completes 120 frames and natural exit with the default copy worker enabled in
50.73 seconds. It uses neutral input and certifies no vehicle action. These
controls do not establish a production copy-worker deadlock or full parity.
