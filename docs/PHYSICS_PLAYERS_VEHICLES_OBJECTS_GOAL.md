# Player, vehicle and object physics: 1:1 port goal

## Required end state: native kernels with unchanged TU1.5 behavior

Clarified October25,2026: port every player, vehicle, object and shared physics
system to native host kernels while preserving1:1 English OG Xbox TU1.5 logic.
Rewrite existing implementations wherever necessary to meet both requirements.
TU1.1 is outside this goal and cannot qualify TU1.5 acceptance.

- The live world, bodies, controllers, contacts, constraints, solver, integration
  and lifetime paths must execute native semantic kernels. Xbox CPU emulation,
  instruction interpretation, virtual-register/instruction-by-instruction
  execution and fallback to an Xbox execution backend do not qualify. Merely
  compiling lifted instruction machinery as host C++ does not complete this
  migration. Authenticated original constants and compatible data layouts may
  remain inputs to native kernels.
- Preserve original rules, branching, update/callback order, timing, units,
  collision filtering, forces, constraints, state transitions, ownership and
  observable numerical behavior. Retain original precision, rounding, operation
  order and floating-point edge cases wherever they affect subsequent state.
  Do not change tuning, loosen comparisons, skip behavior or supply recorded
  results to make a replacement pass. Unproved behavior stays explicitly open.
- Original Xbox instructions, emulators and executable-backed replays may be
  used only as independent offline references. They must not advance gameplay,
  calculate live physics results or rescue an unported branch in production.
- Qualify each replacement against independently authenticated TU1.5 inputs and
  original outcomes, including relevant state, ordered writes, callbacks,
  numerical edge cases and composed trajectories. Then wire it into the live
  engine and test visibly in multiplayer with prepared imported MCC content,
  actual player/vehicle cameras and host/peer state comparisons. Verify the
  exercised paths used native kernels without lifted/emulated fallback,
  including when required state/write observations are enabled. A nonzero
  process-wide fast-path counter alone is insufficient. Refused, skipped and
  unsupported branches remain open. Host-versus-host original-code agreement
  does not establish Xbox-specific approximation bits; recover and validate
  those independently wherever they affect the required behavior.
- Earlier bounded behavior passes remain reference evidence. They do not prove
  this architecture requirement: every applicable path needs native-backend
  requalification before the goal can be complete. Update the linked checklist
  as conversions, original comparisons and live delivery are completed.

The [native-kernel migration audit](PHYSICS_NATIVE_KERNEL_MIGRATION.md) records
the current lifted owners, partial native kernels and remaining proof gaps.

Native-kernel continuation (October 26): corrected v1459 passes eight official
regressions and ten visible multiplayer cases using the prepared imported MCC
cache. Actual native activity includes 2,736 constraint steps, 2,992 dynamic
accumulator setups, 3,740 motion commits and 8,775 island collision entries.
All three commit kinds execute. Seven strict whole-frame original comparisons
pass; three Cyclotron raw-bank comparisons remain red without waived bytes.
[Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-constraint-original-v1459/iteration-01-summary.json).
The [current vehicle recording](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-06-island-agents-vehicle-camera/requested-video/window.mp4)
was visually inspected and shows the actual chase camera plus independent peer.

The broad-phase refresh completed nine of sixteen planned cases before an
interruption: all nine visible/native gates pass, with 10,438 native broad-phase
calls and 2,414 exact LAN comparisons. Six strict original frames pass; the
same three Cyclotron comparisons remain red. Players and the six extended
fields are incomplete in this refresh. Its owned source/build is removed.
[Interruption receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-constraint-original-v1459/iteration-02-interruption-summary.json).

The scene-owner lift bypass is fixed and independently passes 576 original
parents. Authored Midship/Lockout fields exercise all four native scalar seams.
Expanded v1458 field tests pass all sixteen visible cases, but three additional
raw-bank failures on Gemini/Elongation remain unclassified; the earlier contact
scratch diagnosis does not excuse them. Native-only closure remains open.

The complete typed row solver passes 3,000 physical original cases under
GCC and Clang. Its native initializer passes 113 cases; the three built-in
custom callbacks pass 336 cases with both store-failure phases. The composed
native clock/initializer/solver/custom route passes 143 original cases across
all twelve primary row families. The clock consumes the world's existing
recorded FILETIME input and adjustment. Unknown custom owners remain explicit
refusals. Source wiring and current binding checks pass; the v1460 multiplayer
build is qualifying their activation, including positive vehicle solver rows.

The scoped vehicle-rate route now passes 1,382 physical calls with actual FP
instruction snapshots and 2,230 cases with each retained/current state header.
It is wired with per-world counts and explicit unsupported-context refusals.
Native array reserve now passes 64 physical original cases, including seven
retained-argument cases and a rejected early-header mutation. Remaining collision, controller,
lifetime and platform owners, Xbox numerical identity and a fresh launcher
delivery still require completion. Launcher-selected v1396-r2 stays preserved.

The corrected v1460/01 cohort completes sixteen visible MCC cases: fifteen
acceptance reports pass and ten strict original-frame samples match. The
remaining vehicle acceptance gate is unexercised approach-rate coverage:
acceleration executes121 times, approach0, with no refusal. Its actual camera
and control flow complete, and its selected frame matches89,729,945 original
bytes; zero approach calls are still not counted as a pass. The authored Ghost
supplement also completes ordinary driving/camera flow with83 acceleration
calls and zero approach calls. The separate Warthog reversal-input probe now
passes:239 native acceleration calls,60 approach calls,299 cursor calls and no
refusals. Ordinary240-authority-tick SDL driving exercises the original branch;
no vehicle flags or physics results are supplied. The actual camera renders334
seated frames, the driver enters/exits, and the selected late frame matches
89,729,945 original bytes. The ordinary-rate closure is bounded to this case;
outer vehicle controller conversion and the full goal remain open.
[Latest visible recording](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-06-island-agents-vehicle-camera/requested-video/window.mp4),
[actual report](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-05-vehicle-rate-reversal-retry-vehicle-camera/run/lan-vehicle/report.json).

This cohort records138,352 native solver iterations,296,920 primary rows and
65,544 custom callbacks. All three built-in custom kinds execute, with counts
`[432,432,64680]`, and there are zero custom refusals. Initializer executes16
times; the authenticated client path makes zero clock calls, reported honestly.
All five Cyclotron visible cases pass, including lower0 with58 moving-support
ticks and2.3862369 carry. The earlier zero-carry run remains a failed stimulus:
its delivered input cadence differs from the passing run, so no physics cause
or fix is inferred. Six Cyclotron/Gemini/Elongation raw-bank failures stay red.
[Current full cohort](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-01-summary.json).

The combined v1460/02 cohort retains fifteen visible passes and ten strict
original samples. Native schema14 emission executes15,478 times through all
three original callers, with matching completion/cursor counts and no refusal.
Native array reserve remains zero in every case: the typed broad-phase growth
branch is unexercised, while additional internal helpers still need native
binding. Zero does not qualify reserve activation.
[Combined results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-02-summary.json).

Native island agent phases321D20/321A70 are now wired into the typed world
binding with retained caller-owned contact packets and per-world activity.
Their20-case physical composition proof executes genuine original callback
owners and compares all91,356,057 captured bytes per case. It requires actual
callback-entry GP/private-stack inputs; that dependency is not production
service-context closure. The binding model and110 original world cases pass.
The v1460/06 Warthog live test passes eight official regressions, ordinary
multiplayer/camera gates and a strict89,729,945-byte late-frame comparison.
Native flag-set scheduling executes986 times with1,169 genuine process/finish
calls each and986 balanced buffer lifetimes. Rates are242 acceleration/62
approach/304 cursor calls with no refusal. Flag-clear and minimum-distance
branches stay unexercised here; five contact-time calls still use the explicit
unconverted3205B0 service. The full native-only closure stays open.
[Live phase report](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-06-island-agents-vehicle-camera/run/lan-vehicle/report.json).

Completed v1459 reference captures are losslessly compressed and all 24,226
files hash verified. This frees 759,529,472 allocated bytes after accounting for
the retained archive and receipts. Vehicle-rate proof cleanup removes a further
146,665,472 allocated bytes. The completed v1460 owner is now removed, recovering
1,135,992,832 allocated bytes. Subsequent redundant vehicle recordings recover
28,852,224 bytes and a further11,735,040 bytes after independently reviewing
the current v1460/06 vehicle clip. Latest vehicle/Midship recordings and needed
failed original inputs remain. A fresh, finite v1461 owner built the exact
8,308-file frozen source using the existing imported MCC cache; all eight
regressions pass. Its baseline Warthog shows158 actual source vehicle-camera
frames,254 acceleration/66 approach/320 cursor calls with no refusal,19.09units
of horizontal travel, and twelve still-unconverted contact-time calls. The
strict89,729,945-byte late-frame comparison passes. This does not close the
broader driving/solved-contact gate. Gemini times out at180seconds without a
final activity report; flag-clear coverage is unavailable and its partial raw
frame remains red. The final03 refresh executes912 native endpoint,771 compound
and1,700 compound-child calls,551 box/convex calls each and1,218 centered calls
each. Bounds residuals fall from1,097 to45, so that gate remains red. Its actual
vehicle camera, all three rate families, native contact-time gate and strict
original frame pass. Contact time executes16 native calls,86 motion services,
46 process services and33 paired iterations. These services still require
native conversion. The completed owner removes1,136,771,072 allocated bytes;
its35.108-second in-flight overrun is explicitly recorded without a clock or
deadline mutation. Future owners must clamp work to the remaining budget.
[Baseline vehicle](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-input-original-v1461/iteration-00-contact-time-before-vehicle-camera/run/lan-vehicle/report.json).
[Latest results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-input-original-v1461/iteration-03-summary.json).

The completed v1460 input retirement also verifies all64,599 files through a
lossless archive roundtrip before removing their unpacked copies. Its scoped
net allocated saving is1,705,365,504 bytes, including the replacement archive,
manifests and receipts. The194,532,631-byte archive remains a required reference;
v1460/00 lower0 and all protected v1458 direct inputs remain available.
[Cleanup receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/completed-inputs-retirement-except-lower0/cleanup-verification.json).

Seven native shape-bound owners and their geometry composition now pass1,195
physical original cases under GCC/Clang, with four rejected mutations. Their
world binding and nested caller-storage checks pass; the queued live refresh
requires native leaf work and zero shape-bound residuals. Twelve transient
radius-read x87 fault prefixes remain outside the completed proof. The full
contact producer still has six dynamic-case raw failures; its append/buffer
owners and native exception cleanup are being ported with those failures kept
visible. The31F770/31F860 constraint wrappers now pass922 original calls/prefixes
under GCC/Clang with the genuine native base/emitter composition and three
rejected mutations. Their working-tree binding and real-owner model pass;
world qualification and visible activation are pending. Direct native emissions
have separate attempted/completed counters from the generated32A390 ABI scope.
[Producer proof](../build/test-results/native-constraint-producer-first-tu15/report.json),
[binding](../build/test-results/native-constraint-producers-binding2-tu15/report.json).
The base329720 now passes837 physical calls/prefixes under both compilers,
including all921 source sites and four rejected mutations. Wrapper composition
and live activation remain open.
[Base proof](../build/test-results/native-constraint-base-final-tu15/report.json).
The new native inverse-affine2E4550 kernel passes269 physical original cases
under GCC/Clang with all three arithmetic/read-order mutations rejected. It is
not yet wired; the candidate-update parent2F36F0 and dispatch2F3140 remain open.
[Inverse proof](../build/test-results/native-inverse-geometry-final-tu15/report.json).
The two newly observed bounds owners2E8120/306170 pass334 physical cases,
9,341 ordered reads and2,463 stores under both compilers. Their composed world
binding passes110 original cases; transient private radius faults remain open.
[New bounds](../build/test-results/native-compound-shape-bounds-tu15/report.json).
Native3205B0 now passes eleven genuine-input comparisons under both compilers,
including14 single and22 paired bisection iterations. Each case compares
89,734,533 persistent bytes. Declared velocity/time variants are identified;
no contact result or captured after-state is supplied. Negative-single restore
coverage remains open.
[Contact-time proof](../build/test-results/native-island-contact-advance-loops2-tu15/report.json).
Native2F3140 dispatch passes fourteen actual-input variations with genuine
class/factory services, all112 ordered data reads and strict persistent memory
under both compilers; both wrong-selection controls are rejected. Candidate
update2F36F0's42 bounded completed cases reach160 genuine creations,14 releases
and43 reserve calls per compiler. Its independent source review found additional
read-timing and cleanup-prefix work; those fixes and stronger qualification are
in progress, so neither owner is claimed as live/native-only closure.
[Dispatch](../build/test-results/native-contact-agent-dispatch-read-order-tu15/report.json),
[candidate findings](../analysis/physics/native-contact-candidate-source-review-tu15-20261026/report.json).

Current counted remainder: [TU1.5 remaining checklist](PHYSICS_TU15_REMAINING_CHECKLIST.md).
This checklist is part of the goal's completion criteria. Update it after each
completed build/live acceptance cohort and each material blocker change, with
current MCC evidence, remaining counts and verified scratch cleanup. Keep a
group open until its required live integration and visible multiplayer evidence
pass; historical reference or component tests alone do not close it.
Focused v1124 restores C05 phase0 Cyclotron boarding with58 moving-support
ticks,2.3862369 units of passive carry and243 exact LAN comparisons after the
first-admission facing correction. V01 closes after v1103 passes10/10 actual
vehicle-camera driver/turret cases. **14 goal groups remain open**. C02 also closes in v1124 with2 verified target impulses changing actual
linear/angular motion,0.46190089 body travel, retirement and recreation. Full
v1125 core passes9/9; full
TU1.5 parity remains incomplete.

Current continuation: source-backed Zanzibar switch/script cases pass. Real MCC
Elongation/Colossus conveyor tests expose original282170's missing56E6D0 owner.
Original audits v1395/v1401 authenticate20 mutable loader-zero bytes and the
44-byte readonly constraint interface. Both source fixes are integrated;
v1402 visibly carries neutral players2.25427/3.83574 units with183/182 exact
LAN comparisons. Its exact ELF is immutable v1396-r1. Core9/9, fresh fleet26/26,54 compatibility and3 rendered checks pass.
Wiring v1420 is complete; actual launcher and object-field acceptance follow. The full current v1381 campaign census is34/47 raw
passes,13 unavailable observer starts; private scratch is removed and motion
review stays separate.
The previous private source/build is removed (v1370:1.903GB observed allocations;
measured net free change+870.9MB). Warthog/Ghost wall/contact gates remain red;
ordinary driver/camera qualification is kept separate.

**Current wired physics runtime: v1396-r2.** `Halo2-Physics-TU15` selects
native SHA `bec211bac82541ac2e8192c1152fb3c9a3e21bdfb6e4ff6a2a9233321669ff68`.
Qualification passes36 linked checks, three actual-MCC DXT3/ownership/lifecycle
checks, core v1434 **9/9**, fresh vehicle/seat v1435 **26/26**,54 compatibility
checks (build1396/iteration14), and3 rendered checks v1436. The26 cases include
22 allowed seat/camera cases and four genuine Scorpion rider rejections.
Banshee uses a declared0.3-radian takeoff input with unchanged travel/climb and
exact source-camera gates. Ordinary native v1437 passes120 frames/178 ticks/
two players; actual wired launcher v1439 passes120 frames/160 ticks/two players.
Both opened-file audits contain only imported MCC maps and no foreign caches/XBE.

Occupied Warthog-driver and Spectre restoration v1433 preserves the exact
physics bank/player bindings and continues471/623 exact LAN comparisons with
actual source cameras. Both original post-restore1CC8D0 calls independently
match89,795,481 final source bytes plus captured CPU/FP state. The original
failure was an omitted DXT3 checkpoint format despite existing64-sample
original constructor proof; the actual-MCC regression fails before the fix.
Successful copied banks/replay programs and each completed live scratch are
removed. Earlier deliveries, exact source archives and required shader paths
remain preserved. The active private build is still owned for the final
cross-map contact, vehicle-blast and jump-rearm cases, then will be removed.
Full1:1 parity is incomplete;14 broad completion groups remain open.

Earlier full delivery v1303 passes33/33 linked checks,
core v1306 **9/9**, seats v1308 **16/16**, and drivers/turrets v1313 **10/10**.
The core includes strict nonzero object impulses, Lockout gravity lift and all5
Cyclotron supports. Actual Warthog camera proof:430 submitted seated frames,
0.60 radians look,41 after-exit frames and464 exact LAN comparisons. Lower
Cyclotron phase0:58 support ticks,2.3862386 carry and244 LAN comparisons.
Both Spectre null-HUD regressions are fixed: moving occupied turret travel is
5.446917 units, with438 driver/614 gunner source-camera frames and642 LAN
comparisons. This preserves authored null HUDs as empty weapon interfaces;
genuinely pruned typed HUDs still fail selected submission.

Ordinary MCC startup v1315 passes120 frames/319 ticks/two players. Actual wired
launcher v1317 passes120 frames/295 ticks/two players. Opened-file audits have
zero foreign maps or XBE/disc/Vista inputs. Core, seats, drivers and both startup
scratch trees are removed. Required executable/shader/source packages and
requested recordings remain with explicit retention receipts.

**Campaign V03 stays open: current v1381 has34/47 raw passes and13 unavailable observer starts.**
All47 contexts use one exact v1369-r1 runtime. Its scratch is removed. Latest
v1418 samples show context33 identifiable in both views, with partial late
occlusion;42 still loses a readable first view overhead. Full current motion
review remains open. The earlier v1337 result follows for provenance.
Consolidated v1337 verifies47 distinct contexts from v1330/v1336, actual MCC
inputs, stable per-case runtime/map pins and removed private scratch. Thirteen
contexts lack admitted observer starts; context32 reaches machine355's missing
native PRT caller. These are raw controller/projection/LAN results; motion
recording review is a separate gate. All production source matches v1303;
contexts35/42 explicitly use the v1321 initial-observer fixture. The other45
use the original first-clear-floor fixture. The v1330 fixture-switch hash
assertion was a harness bookkeeping failure after saving case35; v1336 runs
only the remaining11 contexts, preserving every raw receipt.

The bounded floor survey makes context35 readable in both reviewed views.
Context42 still loses the target during overhead motion. Current v1342-b
samples at20/50/85 percent show0/1/2 clearly, but33 becomes substantially
occluded late and stays open. All12 decoded review frames are removed; original
requested recordings and small assessments remain. Prior combined sampled
coverage reached at most32/47 across frozen fixtures; a fully reviewed current
33-context motion cohort is still pending. No raw pass alone closes V03.

Independent original-instruction replay v1311/v1314 matches B8F40, C92C0 and
1CC8D0 on actual context35 inputs:172,426,725 memory bytes per call, retained
register/FP state, zero differences, no substituted services/results. Copied
banks and replay binaries are removed. This is bounded call evidence, not
continuous trajectories or Xbox hardware proof. Machine355 still needs its
actual per-view PRT owner; verified cold BSS zero is not a retained incident.
Raw original/MCC SH audit v1331 confirms both have zero records for FloodZone
BSP0/1/2, seven atBSP3 and25 atBSP4. This is a caller-lifetime integration gap,
not missing MCC SH conversion. Native diagnostic v1341 passes the prior450212
ownership boundary and reaches a duplicate47524E binding inside the authored
camera-effect table. Current shared source already removes that duplicate;
v1343 completes both construction fixes and observes actual23670 arguments
and a real completed one-owner/two-node PRT cache. Actual admission still
excludes this unregistered, globally instantiated machine atZ790. A declared
isolated whole-builder call is component evidence only, never a view admission.
The v1343 private build/source is removed. Candidate v1346 passes the new
actual-MCC frustum/resource regression and existing machine presentation tests,
but its linked cohort is29/35: six checks reach the same writable47C1F8
reaction-name table through an obsolete immutable-scalar declaration. Read-only
debugger v1355 observes matching bits0600008D in the existing16-byte owner;
offline TU1.5 audit v1356 confirms all four names and writable .data flags7.
Both old aliases now borrow the complete owner, and the regression checks all
four writable entries. The v1346 private build/source is removed; no v1346 live
case was attempted or counted. The owned v1357 build now passes factory ownership,34/34 linked gates and
immutable export. Newly reached1EF260 is supplied by the existing shared
19-table branch provider; independent v1361 verifies all577 original readonly
bytes and its unsigned0..4 bound. Base v1359 core passes9/9: actual Warthog
camera320 frames,0.60rad look and481 exact LAN comparisons. Its scratch is gone.
Live v1358 still rejects machine355: the old presenter recomputes a node0 shadow
sphere instead of using the retained object+30/+3C bound. Revision v1357-r1
now consumes that actual bound from the authoritative full-handle WORLD-pose
snapshot for machine frustum and LOD selection. Its34 linked gates/export pass;
core v1365 passes9/9, but focused FloodZone v1364 still fails. Read-only
v1367 shows the real retained sphere enters the ordinary camera frustum during
vehicle look; frustum bounds alone cannot establish source admission. Candidate
v1369 reads actual full-generation membership from both original spatial pools,
transports sorted cluster IDs with each source root under protocol95, and gates
machine submission against the current portal frustums. Its36 linked checks and
immutable export pass. Focused FloodZone v1371 now passes:102 submitted
vehicle frames in both views,16.76 units of causal motion and185 exact LAN
comparisons. v1374 reviews20/50/85-percent recording samples: the Phantom is
clearly visible in both views throughout those samples; all3 decoded PNGs are
removed. This closes the context32 machine355 rejection at this boundary;
admitted machine PRT ownership remains a separate open path. Core v1372 passes9/9. All5 Cyclotron sampled frames visibly retain their
machine geometry; support ticks/carry are71/2.38624,72/2.38624,59/2.38638,
44/2.37445 and15/0.71099. Actual Warthog camera394 frames,0.60rad look and453
exact LAN comparisons pass. The26-case family v1373 is running. Its Banshee
camera passes but the first flight reaches only7.47 horizontal units against
the unchanged10-unit gate. Two unchanged v1378 repeats pass14.71/17.55 units;
the retained v1303 reference passes14.57. The original red trace stays retained.
Declared0.3-radian SDL departure look v1380 passes three times at15.84/15.52/
16.11 horizontal units and18.09/18.19/18.38 climb, without changing flight
thresholds or production physics. Consolidated v1385 qualifies26 selections:
25 unchanged-input family cases plus those three Banshee runs; this is not a
single all-green raw cohort. The observer remains inside the base and loses
the climbing Banshee behind its ceiling. New47-context v1381 runs on one
exact r1 binary; v1375 was superseded before running any live case. Existing teleporter/HUD/flash compatibility
v1376 passes54/54 with no skipped checks before any launcher replacement.
The old full47 census is not relabeled as current-candidate qualification. The expired v1357 private source/build are removed. The new
bounded v1369 source/build owner remains active during visible investigation;
per-case raw outputs retain the managed lifecycle.

**Verified storage:** v1304 removes1,443,106,816 observed allocated private
CMake bytes; net volume free change+398,135,296 bytes. v1322 retires3 failed
v1286 executables:291,561,472 allocated bytes removed, measured net free change0.
Source archive, shader/header dependencies and diagnostics remain. v1312
archives72 completed helpers from359,852 bytes into26,836 bytes and removes
standalone originals. v1345/v1347 retire superseded diagnostic executables:
84,283,392 and84,348,928 allocated bytes respectively; measured net free changes
are0 and+120,582,144 bytes. Exact source archives and all diagnostic reports
remain. Latest required v1343 probe/source retention is documented in its
native-probe/retention.json. All9 v1318 and12 v1342-b review PNGs are removed. v1350 archives43 completed
helpers from248,348 bytes to27,280 bytes; loose originals are removed. v1352
verifies v1346 private build/source removal:906,006,528 allocated build bytes
were observed before cleanup, with measured net free change-20,606,976 bytes
while packages and concurrent work changed the volume. This is an observed
removal amount, not a complete build peak or a claim of physical reclamation. Concurrent
filesystem activity means these allocation/removal observations are not summed
as physical free-space gains. v1377 retires9 superseded v1346/v1357/r1
executables:885,324,672 logical and885,350,400 allocated bytes removed, measured
net volume free change+65,986,560 bytes. Exact source archives, shader inputs,
reports, requested recordings, v1303 reference, current v1369 candidate and the
shared launcher-selected delivery remain preserved. v1389 removes22 redundant
port recordings after preserving current core proof, the original Banshee
negative, one old-input positive and one takeoff positive:135,434,367 logical /
135,487,488 allocated bytes, measured net volume free change0. All reports,
input traces, hashes and original reference evidence remain.

This checklist is part of the goal's completion criteria and is updated after
material results. The goal tool remains `usageLimited`; its API cannot edit the
objective or resume that status. Full1:1 physics parity remains incomplete.

Frozen v1095 passes14/14 linked gates, including actual paired original/MCC
PRT streams and frame sequencing; private build/source trees are removed.
v1100 passes all16 seats again with actual source cameras. Focused v1096 passes
human-turret colored-area tracers in both views, exact camera look/exit, and
visible prop blast/movement/retirement/recreation. Its actual impulse records
are independently verified in v1108; the source fixture is now stricter and
requires nonzero target impulses plus original linear/angular motion response,
travel and retirement; the fresh compiled v1124 strict run passes and closes C02.

Historical v1240 qualification: the dedicated launcher selected v1240 after31/31 linked gates,
fresh core v1242 passes9/9 and ordinary Coagulation v1246 passes. Actual wired
MCC startup v1248 passes with two players and no forbidden runtime inputs;
private scratch is removed. All16 seats v1249 pass on v1240 with actual source cameras and original
rider rejections. All10 driver/turret cases v1250 pass on the same v1240 with actual source
cameras, deliberate look, genuine controls and exit.
The checklist records the active/frozen source and build lifecycle.
Frozen v1123 passes14/14 gates and its private source/build is removed;
v1124–v1134 test the focused cases, full core, seats, drivers,
ordinary launch, dedicated launcher and all47 campaign contexts visibly.
The checklist is refreshed after every completed cohort or material change.
Failed v1109 and all dependent unrun gameplay cases remain recorded; its
private source/build trees are gone. v1135 confirms the current shader-helper
declaration compiles before the replacement coherent build.

## October 19: required MCC runtime inputs and disposable tests

Runtime physics acceptance now requires the existing prepared cache at
`assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b`. Xbox TU1.5 remains
the behavioral and visual authority through offline source/execution/reference
checks. Runtime tests must not load an Xbox executable, disc maps, or Vista UI.
The historical v299 source cohort and v734 runtime used those legacy runtime
inputs; their recordings remain reference evidence, not current MCC runtime
qualification. The queued v758 build/v760 live runs were stopped after this
requirement was made explicit. Their private storage was removed; source edits
and bounded results are preserved. Do not promote v734 as the current runtime.

The original support controller readback is wired into the current root tests:
actual owner at unit+448, PHYS body index at unit+40C, and the original rigid
transform at unit+410. Offline actual captures show a 2.35-unit grounded carry
after a lower-entry body handoff, while the phase0 BSP-floor-only entry remains
red. The current MCC fixture compiles, but moving-support acceptance on MCC
remains red at the native owners described below. Full player/vehicle/object
TU1.5 parity is incomplete.

Use `tools/run_tests.py` for disposable CMake builds and `tools/run_test.py`
for each probe/capture. The visible physics wrapper requires `--mcc-root`,
validates version13 cache headers and rejects maps outside the selected cache.
Raw outputs stay in managed scratch. Keep bounded reports with
`--report-output`; retain requested recordings explicitly with `--retain-video`.

The current-root MCC fixture compiles in `mcc-physics-build-v763`; all nine
actual visible MCC attempts in `mcc-physics-visible-v767` stop before gameplay.
The failures expose the legacy LAN fingerprint's unconditional `default.xbe`
dependency and a missing native placement-selector row at `44EB40`. Offline
English TU1.5 audit `mcc-physics-frontier-v768` verifies both original consumers
and the exact immutable sixteen-byte row. It is now mounted by the native world
factory. LAN identity now uses native TU1.5 MCC identity, v13 shared caches,
actual MCC texture/sound banks and `h2_fonts`, rejects mixed cache versions and
explicit executable injection, and preserves relocation/content checks.
`mcc-lan-identity-v769` records the exact production function failing before and
passing after this fix. Managed current-source build `mcc-physics-build-v770`
passes both linked regressions and exports the ordinary runtime. The private
build/source trees are removed. Initial v771/v773 attempts fail in mirror setup
before game execution because the one-off controller supplied an outdated X11
authority path; their failure logs are preserved. Corrected visible rerun
`mcc-physics-visible-v775` attempts all nine actual MCC cases with uninterrupted
mirrors and removes its scratch. All nine remain red: player shape dispatch at
`461420`, Coagulation configuration branches at `15CFA8`, gravity-lift contact
dispatch at `41CA9C`, and Cyclotron physical branches at `1E0184`. Original
constructors, callers, exact bytes and branch extents are audited in v779/v780;
these four immutable owners and their factory regression checks are now wired.
Managed build v781 stops on a font-variable name collision captured during
concurrent source work; its private tree is removed and no v783/v784 game is
run. The current source already corrects that name collision. Managed build
v788 and fresh visible matrix v789/ordinary startup v790 are pending validation.

Build v788 subsequently passes all three gates and removes its private tree.
The full v789 MCC matrix advances to new failures: movement selector byte
`115E1A`, inline vehicle shape interface `462060`, contact query interface
`41CA8C`, and the physical interface at `41EA3C`. All nine attempts retain their
actual errors and uninterrupted mirror receipts; none is accepted as physics
parity. Ordinary v790 startup passes again (120 frames, 153 ticks, two players,
no forbidden runtime files) and removes scratch. The MCC-only vehicle inventory
finds 209 authored placements, including 47 class-3 placements, without a
reference executable or injected tags.

Offline v786/v792 audits find 104 proved branch/selector tables across the
2,025-function compiled physics graph, with 66 tables/2,191 bytes absent from
the v788 native factory. The native factory now mounts the complete proved
3,541-byte profile through `native_tu15_physics_branch_data.cpp`. Its exporter
authenticates the English TU1.5 executable and exact source graph, exports only
bounded readonly branch data, and initially defers six traces with targets
outside their recovered code. Existing readonly bytes must match; writable or
conflicting owners are rejected before new data is published. Partial owners
are preserved. Real production implementation controls pass in v798/v803.
Audits v796/v797 additionally verify the newly reached interfaces and their
constructor-installed siblings/embedded callbacks; these are wired into the
factory and regression checks. Build v799 passes all three gates and removes
its private tree. Live MCC matrix v800 now passes actual movement/crouch/jump:
181 exact same-tick LAN comparisons, 294 submitted frames, 1.444256 units of
travel, crouch1 and 2.940958 upward velocity. The unaltered recording is retained.
Vehicle/object cases advance to `45F8C4`, the gravity lift to `461F78`, and
Cyclotron to `41E484`; those eight cases remain red. Ordinary v801 passes
120 frames/130 ticks/two players with no forbidden runtime inputs.

Audit v807 identifies the six deferred cases as original literal null targets,
not nonzero cross-function jumps. Export v811 now retains all118 readonly
branch/selector tables (3,915 original bytes), including those null words and
the two proven biased byte-table operands. Original null dispatch is not
replaced with a successful callback. Audits v808-v810 additionally derive
bounded code-reference prefixes from original readonly operands published by
instruction sites in the compiled physics bodies. Export v812 wires140 such
records (4,344 bytes); it does not infer complete C++ class layouts or callback
implementation. The reached physical query at45F8C0 is separately declared
from its original185111 constructor and neighboring interface boundary.
Production sharing controls pass in v815. Build v813 stops at a duplicate
geometry-cache policy owner at48C754; v814/v816 do not execute games. Its private
source/build trees are removed, as verified in v817. The current source checks
the existing writable renderer owner and original enabled value instead of
mounting another copy. Frozen build v819 (four compiler jobs) passes all three
gates and removes its private build/source trees. Live MCC matrix v820 passes
movement again (181 exact LAN states,301 submitted frames), with uninterrupted
mirrors for all nine cases. Vehicle/object cases reachF9083's absent46AAE8
scalar, the gravity lift reachesE892C's46A1F4 scalar, and all five Cyclotron
cases reach31D5A4's41E92C interface slot. Their actual errors are retained and
their scratch is removed. Ordinary v821 passes120 frames/120 ticks/two players
and its opened-file audit finds no Xbox/Vista runtime inputs. Full parity and
vehicle-camera acceptance remain open.

Original audits v825-v828 verify those arithmetic operands and31D68A's
eleven-method41E910 prefix, reached through the existing compiled death/contact
dependencies. Export v829 extends the code-reference profile to146 records /
4,496 bytes. Export v830 adds147 fixed-readonly operand ranges /1,384 bytes,
derived from2,568 authenticated reads in seven pinned native source bodies.
Only original .text sites qualify;466 SDK sites, indexed extents, writable
storage and intervening gaps are excluded. No callbacks, code, tags or produced
state are invented. Shared owners still require exact readonly bytes. The
production sharing controls pass in v831. Frozen build v832 rejects the export's
raw41B774 kernel-clock ordinal because the actual world correctly owns a
relocated clock binding there. Its private tree is removed; v833/v834 and the
queued seat/Coagulation checks do not run games. Corrected export v840 excludes
that loader-resolved input and retains146 ranges /1,380 bytes from2,567 fixed
reads. The permanent ownership test in v841 checks both absence of a fabricated
clock import and preservation of an already supplied clock owner, plus the
existing atomic conflict/reservation controls. Build v842 includes that linked
test and the full native factory; live matrix v843, ordinary v844 startup,
MCC-only seat/camera CLI v846 and ordinary Coagulation v847 follow it in order.

MCC context inventory v818 resolves all47 class-3 object and variant names
against the original reference. Positions, flags and BSP policy correspond;
cache-local name/StringID differences are retained in v805. Future inputs use
actual MCC map paths and tag identities. This does not establish clear observer
starts, activation, controller response or visible acceptance for those cases.

Retirement v804 removes another 277,030,072 bytes of completed v770 test
programs and duplicate shaders, preserving the exact source archive, reports,
recordings, current v788 candidate and launcher-selected runtimes. The v788
private build is removed, including at least 504,840,192 observed allocated
file bytes; some exported files share those extents and concurrent filesystem
activity prevents attributing a matching physical free-space increase.

Retirement v823 removes 555,276,016 bytes of superseded v788 and failed v813
test programs and identical shaders. The exact source archives (31,135,605 and
31,336,006 bytes), pins, reports and recordings remain. The visibly tested v799
baseline and all launcher-selected runtimes are preserved and rehashed. Its
managed scratch is removed. The immediate volume free-space delta is
123,457,536 bytes during concurrent builds, reported separately from removed
file sizes.

Retirement v836 removes another277,418,808 bytes of completed v799 test
programs and identical shaders, retaining its exact source and recordings and
the actual-tested v819 runtime. The immediate volume free-space delta is zero
while other builds write; this is not reported as264.6MiB physically reclaimed.
The v819 private tree is verified absent in v822, including491,999,232 observed
allocated file bytes before automatic removal. Concurrent work again prevents
attributing its net volume delta to that cleanup alone.

The failed v832 private build is verified absent in v835, including953,278,464
observed allocated file bytes; the simultaneous net volume change is
115,179,520 bytes. Retirement v848 removes its unqualified executables and
identical shaders (278,362,480 file bytes; immediate volume delta5,533,696),
preserving its exact source archive and failure diagnostics, v819 and all active
or launcher-selected runtimes.

Ordinary current-candidate MCC startup v776 passes: 120 rendered frames,
163 ticks, two players, 540 opened MCC files, 69 continuous mirror checks and
zero Xbox/Vista runtime inputs. The actual two-view Lockout frame is manually
reviewed in v785. This is startup evidence, not movement/vehicle/lift acceptance.
Its scratch is removed. Retirement v777 removes 183,678,651 bytes of superseded
v763 test programs, duplicate shaders and obsolete helpers; the exact source
archive and failure results remain. Filesystem free-space deltas include
concurrent work and do not equal that removed path size. The original v770
bundle totaled308,133,642 bytes; its programs/shaders are now retired under
v804, with exact source and results preserved.

## October 19: retained shader bundle and live team/world integration (WIP)

Managed builds v722/v730 pass the actual authored child and coupled Match/world
checkpoint regressions. v734 repeats those gates and exports an immutable runtime
with all 54 compiled shaders, then removes its private CMake build. The earlier
v731/v732 runs failed because exported executables still referenced deleted build
shaders; those red packaging reports are preserved. The new retained bundle is
verified independently of its deleted build directory.

Fresh Warthog multiplayer v739 passes 902 exact UDP snapshots, 345 submitted
seated vehicle-camera frames, 0.599999964-radian look response, 21.114706 units of
driver horizontal travel and 300 camera frames after exit. All 118 desktop mirror
checks pass. Actual 8s/26s frames were inspected in `vehicle-camera-review-v742`:
the driving view uses the third-person vehicle camera and exit restores first
person. Lower-edge vehicle cropping and Xbox framing/material parity remain open.

High Charity placement1/BSP6 now passes on v734 through the original team-state
constructor and new-map reset: 2,012 exact UDP snapshots, 1,161 vehicle draw frames
in each view and 16.757002 units of causal source-controller response. The actual
Phantom sample was inspected. Independent checks are recorded in
`integration-execution-verification-v744.json`. The historical aggregate v744 is
now 27 bounded passes/20 incomplete contexts, across mixed runtime versions;
`managed-class3-all-v741` is running all 47 contexts on the single v734 runtime.

Fresh lift bodies0/3 pass moving carry (65/45 ticks and 2.596529/0.778250 units).
Body2 reproduces 66 moving ticks/2.556674 units and native acceptance, but its
desktop mirror stopped, so that visibility gate stays red pending a fresh run.
Body1 at phase0 still has zero moving carry. Retained samples show the player
remaining on the BSP floor while the low paddle moves away. Additional declared
original device-phase entry cases are queued in v743; this is a hypothesis about
the missing entry context, not a changed physics result or a waived phase0 gate.
v746 will repeat body2 and ordinary startup after the serial matrix/phase tests.
The original v740 startup recording is excluded from uninterrupted visual proof
because a mirror-only diagnostic briefly overlapped it.

The visible monitor now tolerates only an unrelated destroyed EWMH window while
still requiring the actual owned capture/player pipe and mapped mirror. The real
X11 regression v738 is red before the fix and green afterward; missing/stopped
mirrors remain red. Diagnostics record which process stopped and the native
process state. Mirror-only probes refuse to open over an existing live mirror.

The launcher remains v678. Coupled validation, a visible native controller pass
and this 47-context vehicle gate do not establish all player/object/vehicle
physics, contacts, lifetimes, all maps/classes, campaign activation/AI/HS or
physical Xbox trajectory/pixel parity. The complete TU1.5 goal remains open.


## October 18: authored marker identities and managed validation (WIP)

`class3-latest-context-matrix-v729.json` now records **26 bounded native passes
and 21 incomplete contexts** across retained runtime versions. All four Floodlab
contexts and seven other formerly ambiguous-child contexts pass on immutable
v698; its twelve-case cohort finishes 11 passes/one new owner failure. The
independent execution check is `model-child-execution-verification-v728.json`.
The retained Floodlab BSP5 and Sentinel Walls samples were manually inspected;
Sentinel Walls hull lighting is visibly dark and full material/pixel parity is
not claimed. This aggregate is not one-current-runtime certification.

Model children now resolve the actual completed C0790/BAF10 parent marker,
child marker, definition and full parent generation. Actual source tests verify
the three Sentinel Walls children, including two with identical definitions,
with exact completed WORLD nodes and UDP state. Checkpoint metadata retains
these original observed identities. A coupled campaign checkpoint exposed two
existing multiplayer-only validators and a bodyless native startup weapon;
those source paths are corrected, with managed CMake regressions v722 still
running. These checkpoint changes are not claimed passed yet.

The remaining High Charity context advances through the newly ported complete
158690 unit-view reset and its five callees (397 recovered instructions).
Its actual whole call matches 95 unchanged original instructions, all 60,084,310
accessible source bytes and CPU/FP state under the documented host IF boundary;
see `unit-view-reset-original-v724/report.json`. This is Linux i386 original-code
execution, not physical Xbox proof. The subsequent fault reads offsetC4 from
an absent team-relationship owner. Actual retained RAM, the original constructor,
new-map reset and registration bytes identify this missing owner. Root already
has the original row18 calls; integration into the physics delivery is queued
after v722 reaches a terminal safe boundary. No zero-filled replacement bit bank
or supplied damage/physics result is used.

Fresh source-owned machine rendering also retains passing rides on authored
`platform`, `platform2` and `platform3`: 65/66/45 moving support ticks and
2.596529/2.556674/0.778292 units carried. Body 1 (`lowest`) remains red with zero
moving carry. Literal original cache names and indices are recorded by v703;
names do not establish retail gameplay roles or justify removing this red gate.

Validation now uses `tools/run_tests.py` and `tools/run_test.py`. v723/v724
scratch cleanup is verified in `managed-storage-verification-v725/report.json`.
The explicitly retained v723 recording, frame, reports and required original-call
input bank total 69,699,616 bytes; replay binaries, objects and generated output
banks were removed. The queued v730 export retains only the immutable deliverable
and two acceptance executables after coupled tests pass, then removes its private
build. v731 will test the affected contexts, actual vehicle camera, all four lift
bodies and ordinary startup; v732 will attempt all 47 authored vehicle contexts on
that same candidate, retaining only requested visual proof and small diagnostics.
The launcher remains v678 until a new candidate completes reviewed delivery gates.

## October 18: machine WORLD integration and original attachment identity (WIP)

The v692 source regression matches **1,095 original WORLD nodes and 131,700
skinned vertices** across Floodlab BSP0/5 and Ivory Tower. Machinery adopts the
completed source node bank and its render interpolation endpoints; it does not
run a second gear controller. Hidden machine resource failures retain their
exact owner/error and reject when that owner becomes visible. This does not
resolve the original NONE cable texture or other future visible-resource owners.
The unbound portal-lifetime regression and three existing machine/LOD checks pass.

Immutable candidate v694 reaches live multiplayer in all four affected Floodlab
contexts: placements0/1/2 pass native two-view/controller/UDP acceptance;
placement8/BSP5 reaches the repeated-child identity gate. The p0 1s/12s footage
was inspected. A fresh actual Warthog drive/look/exit passes 902 exact UDP states,
580 seated-camera frames and 0.5999999642-radian look; inspected 8s/25s frames show
the vehicle camera and return to first person. Ordinary authored startup passes
120 rendered frames/345 source ticks/two players, with gameplay reviewed at
72.10s/78.10s. The 65.10s loading frame is black and excluded from gameplay proof.
The current launcher remains v678 pending the next candidate's delivery gates.

`elevator-source-contact-v696.json` keeps the upper ride green (65 moving
support ticks, 2.596529 units carried, 244 exact UDP states) and the lower ride red
(zero moving support ticks, 243 exact UDP states). Original cache audit v703 maps
bodies 0/1/2/3 to `platform`/`lowest`/`platform2`/`platform3`; names alone do not
prove gameplay roles. Additional platform2/platform3 live tests are queued.
No carrier/contact/velocity or solved output was supplied.

Read-only original BAF10/C0790 observation v693 verifies 24 completed attachment
calls, 19 literal original model records and nine duplicate-definition groups
with distinct authored markers. Exact marker adoption and checkpoint retention
are wired into both available source paths; v698 source/continuation/live
validation is still running and is not claimed passed. Source archive v699
retains 6,590 reconstructable files (14 deltas, 232 inherited archived fixtures).
`class3-latest-context-matrix-v705.json` records 18 bounded native passes/29 red
contexts across retained runtimes; it is not one-current-runtime certification.
Lower/train rides, resource/PRT owners, activation/AI/HS, remaining physics,
all required live contexts and Xbox trajectory/pixel evidence remain open.

## October 18: verified v678 production delivery

`Halo2-Physics-TU15` now selects immutable `tested-runtime-v678/halo2_native`,
SHA256 `618392372d993e38c01cb948f5b19ceff57fb6f9f2c376b2724478fe4da1577d`.
This delivery includes actual parent-generation child adoption and the inherited
first-person packed graph/event/clock/render-key changes. The source regression
matches six packed activities and four original event packets; actual Gasgiant
world/UDP and six existing regressions pass. Two affected Gasgiant placements
pass native visible multiplayer; the p3 recording was manually inspected.

`fp-packed-production-integration-v678.json` passes a fresh Pelican p7 two-view
run (341 exact UDP states), actual Warthog driver/camera run (902 exact UDP
states, 551 seated-camera frames, 0.6000000238-radian look, 36.845570 horizontal
travel, then on-foot camera), and ordinary authored startup (120 frames,
357 source ticks, two local players). Actual 1s/12s Pelican, 8s/25s Warthog and
80s ordinary-startup recording samples were inspected. The 95s ordinary sample
is black after the visible interval and is retained, not used as visual proof.
Some camera-edge occlusion/cropping is visible; full framing and Xbox parity
remain open. Exact root/frozen launcher receipts and reviews are linked by
`physics-production-delivery-v684.json`.

`class3-latest-context-matrix-v677.json` retains the latest bounded native
attempt for all 47 authored class3 identities: 15 passes and 32 failures,
combining explicitly identified immutable runtimes. It does not claim a full
single-current-runtime sweep or complete manual review. Remaining contexts are
13 unavailable observer/activation starts, eight ambiguous authored child
identities, four bitmap-resource failures and seven original PRT caller-state
failures. The bitmap trace now distinguishes three caught/deferred SCEN catalog
exceptions from the later terminal SourceMachineScene placement29 exception;
no replacement texture is supplied. Original attachment caller research remains
read-only, without guessed child ordering or marker identity.

The lower lift/train approaches, broader players/vehicles/objects, original
campaign/AI/HS/session integration, remaining contact/solver/lifetime branches,
and quantitative Xbox acceptance remain required. Full TU1.5 physics parity
is incomplete and the full goal remains active. Earlier delivery records follow.

## Earlier v656 vehicle-child delivery

`Halo2-Physics-TU15` now selects immutable `tested-runtime-v656/halo2_native`,
SHA256 `f6300e1ad19dac8fc00d91210eedd6a84cb7cc7fd843671fb3e28622b44d49b1`.
`launcher-v656-verified-root.json` and `launcher-v656-verified-frozen.json`
verify this exact executable and the owned English TU1.5 reference.
`normal-runtime-delivery-v656/execution.json` passes ordinary authored-spawn
startup: 120 frames, 356 source ticks, two local players, no fixture spawn
overrides. The 95-second recording frame was inspected.

Actual model children now bind the retained parent generation and the original
constructed child. Source-owned child WORLD nodes are no longer replaced by
native marker composition. The actual 03b New Mombasa placement 7 world test
checks its child identity, parent, four exact WORLD nodes, UDP/client adoption,
and three continuation/placement-order regressions. The visible p7 rerun passes
400 exact UDP comparisons, 175 frames per view and 17.167505 units of source
controller travel; inspected 1s/12s images show the Pelican in both road views
and then moving above/out of view. Reports and review are in
`campaign-vehicle-parent-live-v656/lan-campaign/`.

`vehicle-camera-parent-integration-v656/lan-vehicle/report.json` passes actual
Warthog driver entry, steering, look, travel and exit: 902 exact UDP states,
504 original seated-camera frames, 0.5999999642-radian look, 39.115387 horizontal
travel and 543 on-foot camera frames after exit. Its 8s/25s frames were inspected.
This confirms actual use of the vehicle camera in this case. Pelican player
seating/camera and full contact/solver or Xbox trajectory/pixel parity remain open.

The complete 47-placement v644 matrix finished with 11 passes and 36 failures.
Original reports remain red. The other six parent-identity cases have finished
in `class3-parent-visible-cohort-v661/`: Delta Towers Pelican p1 passes 1,572
exact UDP comparisons and both views; its recording was inspected. Five Phantom
placements now fail explicitly because multiple children share a definition and
need the original authored attachment identity. No first-child selection is used.

The Flak Cannon inherited-graph admission failure is reproduced from original
metadata in `fp-inherited-owner-v659/`. The source now retains the full packed
identifier, uses the selected graph for event references, and keys clocks and
render caches by that identifier. `fp-packed-build-v666.json` passes six packed
activities, four original event packets, the actual Gasgiant world/UDP test and
six existing regressions. Renderer v671 and the nine affected visible cases v672
are pending. These changes are not yet included in delivered v656. The lower
lift, source PRT caller context, cinematic observer contexts and broader physics
acceptance gates remain open. Full TU1.5 physics parity remains incomplete.

The active goal is to port **all physics logic for players, vehicles and objects
until it matches English TU1.5 retail 1:1**. TU1.5 is the sole parity target,
per the user's scope correction on 2026-10-11. Existing Map Pack 1.1 evidence
is historical and does not create another parity requirement. Selected arithmetic tests do not complete this
goal. Authority is original executable instructions, original assets and
verified retail behavior. Unknown behavior stays unresolved until evidence is
available; substitute motion, contacts, solver results or tuning cannot satisfy
parity.

The owned English TU1.5 executable is pinned to
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Legacy correspondence IDs remain internal references in retained source modules.
All new parity execution and acceptance validation target TU1.5.

October18 continuation: all five campaign-only class3 definitions now have
bounded source-controller/UDP/rendering evidence, including the actual Pelican
in03b_newmombasa BSP0 placement3. Its v612 live run passes360 exact UDP states,
143 frames per view,17.145758 units of travel and16.924028 causal separation
after30 identical neutral ticks. Inspected1s/12s frames show movement/rise; the
Pelican leaves the upper view. This case does not test Pelican seating or its
vehicle camera. Four complete actual-session calls match268,399 unchanged
original instructions,all60,084,310 accessible bytes per call and compared
CPU/FP state under the recorded host IF boundary.

Actual SCEN/BLOC model regions now reach authority publication,UDP adoption
and resource catalogs without RNG reroll. The complete catalog regression
matches824,952 WORLD vertices and preserves original retirement/recreation;
601 source region banks include299 original BLOC banks alongside275 native
proxy banks. Unavailable resources for three inactive BSP2 cinematic placements
remain catalogued and reject visible requests. Actual borrowed projectile
attachment metadata resolves through its retained source pointer owner.

The dedicated launcher now delivers tested v614. Fresh ordinary authored-spawn
startup passes120 frames,352 source ticks and two local players. The fresh
Warthog enter/drive/steer/look/exit test passes902 exact states,475 original
seated-camera frames,a0.5999999642-radian look turn and486 on-foot camera frames
after exit. The driving,exited and ordinary-startup recording frames were
inspected. Root/frozen launcher receipts verify the same retained tested ELF.
All47 class3 placements,remaining lifts/train riders,broader contact,controller,
lifetime,cloth,corpse/ragdoll,campaign AI/HS and cinematic contexts,canonical
launcher integration and quantitative Xbox TU1.5 reference validation remain
open. See [shared campaign integration](MP_CAMPAIGN_SHARED_PHYSICS_TU15_20261015.md).

Earlier v448 checkpoint: all five campaign-only class3 definitions now respond to declared controller input in the same immutable source runtime. Fifteen complete admission/controller/object/world calls match 405,409 unchanged TU1.5 instructions. The reached helpers and original static contact-agent initializer are wired into both live source trees. The new v448 production launcher passes normal rendered startup; stock Ghost/lift/object multiplayer passes 665 exact UDP observations with actual original vehicle cameras. Visible class3 multiplayer, the inactive intro Pelican cinematic/script context, and the full wider parity gates remain open. See [class3 integration evidence](MP_CLASS3_PHYSICS_INTEGRATION_TU15_20261015.md).

Authority continuation now preserves the original static contact-agent initializer across world restoration. The final v453 visible occupied-Ghost restore passes491 exact UDP states,8.740213 units of travel, original vehicle-camera turn and exit; both complete post-restore original calls match120,561 instructions. The production launcher selects normally tested v451. Visible class3 multiplayer and the full wider physics scope remain open.

Contrail checkpoint: original contrail source inputs now reach multiplayer clients and run the complete original geometry callback. Human turret v426 passes actual authored bullet motion, colored trail triangles in both views,303 exact UDP states, boarding/aim/exit and exact original vehicle camera. Full-bank versus sparse input geometry and actual live GPU-packet pixel checks pass. The normal v427 launcher delivers the tested integration. Human target damage, remaining carrier contexts, campaign-only vehicle classes and the wider full-parity gates stay open. See [live contrail integration](MP_SOURCE_CONTRAIL_MULTIPLAYER_TU15_20261015.md) and [fixed turret integration](MP_FIXED_TURRETS_TU15_20261015.md).

## Current October15 live checkpoint

All43 audited authored phantom placements have bounded native multiplayer
force/motion/render/UDP evidence:32 player candidates and11 player-excluded
object fields. Both Turf springs, both Gemini columns and Midship's suspended
platform now pass visibly. Nine complete original query bodies also close the
normal Gemini grenade path; the real column receives an original point impulse
and rotates. Five same-ELF live cases retain2,476 exact UDP states, including
Spectre travel with both actual original cameras and object recreation/blast/
debris. Original full-call and point-impulse comparisons are retained alongside
the immutable v381 source cut. See
[the current object/query report](MP_OBJECT_ANGULAR_FIELDS_TU15_20261015.md).

Actual stationary players also ride both original conveyor belts. Three complete
world solves during transport/after a grenade impulse match312,562 untouched
TU1.5 instructions and all memory/CPU/FP state. All eight surveyed drivers now
pass fresh visible multiplayer and all16 complete object/world comparisons
(1,540,296 original instructions), using actual original vehicle cameras.
See [vehicle whole-world evidence](MP_VEHICLE_PHYSICS_WORLDS_TU15_20261015.md).

Eight surveyed vehicle choices also pass real driving/destruction/debris
sequences with actual original cameras and 18,750 exact UDP states. All eight
post-death world solves and seven object updates match3,382,575 untouched
original instructions; the Banshee object update reaches an unresolved Xbox
kernel wait in the host replay. The native destruction case remains green.
See the same vehicle report for the retained red fixture trials and exact scope.

Three Ivory Tower elevator paddles also carry a neutral player visibly,
with65/66/45 moving supported ticks and2.60/2.56/0.78 units of carried travel.
Six complete original object/world calls match289,775 instructions. The train
and fourth lower-body approach remain red, despite bounded matching original
phases; their retail rider trajectories and preceding context stay unresolved.
See [mechanical carrier evidence](MP_MACHINE_LIVE_TU15_20261014.md).

The separate `Halo2-Physics-TU15` launcher selects the pinned tested production
runtime and original maps. Normal windowed startup passes with two local
players,120 rendered frames and357 physics ticks, with no physics fixture
controls or declared spawn overrides. Source ownership remains automatic for
TU1.5. See [normal runtime delivery](MP_PHYSICS_RUNTIME_DELIVERY_TU15_20261015.md).

The goal remains active. Broader collisions/physical lifetimes, remaining
player/physical-carrier/shared engine branches, quantitative retail comparisons
and complete canonical production delivery still require completion.

## October14 live integration checkpoint

All32 audited player-flag candidate placements now have bounded visible native
multiplayer force/rise evidence. Eleven authored placements exclude players
and still require their object/body cases. The original wheel carries a
stationary player under actual source support flags and WORLD nodes.

The original automatic/runtime birth caller distinction now reaches live
Scene/Match/body construction. Colossus authored scripts produce three moving
can generations over650 real UDP ticks. Complete355D0/1E1170 surface/query
owners retain their real callees; actual captured calls match untouched
TU1.5 i386 execution across396/102 instructions and60MB of source memory,
within the documented host interrupt-flag limit. Visible conveyor and fresh
post-change vehicle/camera/lifecycle regressions are in progress. See
[the prop integration evidence](MP_RUNTIME_PROP_BIRTH_TU15_20261014.md).

The full goal remains active. Linear carriers, remaining object fields,
vehicle collision/damage/destruction and broader controller/solver branch
coverage plus original quantitative trajectory/visual comparisons remain
required. Historical checkpoints below retain their original evidence scope.

Current conveyor checkpoint v368 closes the original static constraint
prototype wiring gap: both Elongation/Colossus conveyors pass visibly, all four
Colossus object fields deliver real forces, and occupied Spectre travel uses
both actual original cameras. The three same-ELF cases retain2,435 exact UDP
states. Fresh selected checks pass8/8; complete original-call comparisons cover
12 corrected Colossus and6 corrected Elongation controller/object/world calls.
See [the complete evidence and remaining scope](MP_RUNTIME_PROP_BIRTH_TU15_20261014.md).
These gates do not complete the all-player/vehicle/object parity objective.

## October15 object angular field continuation

All11 player-excluded authored fields now have bounded natural object evidence,
including Midship's suspended platform, both Gemini columns and both Turf
spring doors. Together with32 player candidates this covers all43 placements
in the finite authored inventory; complete physics parity remains open. A normal Turf grenade exposed the missing original
2EE660 broadphase query; its complete body and scratch owners are now wired
into the live dispatcher and are undergoing the same visible test. Failed
firing, stationary-door and unreachable-platform runs remain red. See
[object field evidence and remaining gates](MP_OBJECT_ANGULAR_FIELDS_TU15_20261015.md).

## Required coverage

| Area | Required behavior and integration |
| --- | --- |
| Shared engine | Actual body/shape/world/allocator construction and lifetime; authored shapes and collision filters; broadphase, terrain/MOPP and narrowphase; contact generation/retention/removal and game callbacks; material friction/restitution; constraint/Jacobian generation, iterative solver, warm starting, integration/readback; islands, activation/sleep, deferred operations, forces/torques/impulses and retirement. |
| Players and other bipeds | Original controller dispatch and tick ordering; walking, air movement, crouching, jump/rearm/fatigue, slopes, steps, ceilings/ledges and support transitions; moving/rotating supports and carriers; other authored movement modes; physical activation, root-motion/animation history, collision contacts and authoritative object state; death transitions, ragdolls and corpse lifetime. |
| Vehicles | Source-created vehicles and components; each original vehicle/controller and motion class; authored steering, acceleration/deceleration, engine torque/gears, suspension/contact forces and applicable hover/flight behavior; collision, impulses, activation and destruction; seat/driver/boarding transitions that alter physics; controller-to-body force delivery and actual solved readback. |
| Objects | Dynamic crates/props/items, equipment and projectile physical behavior, attached/parented objects, devices and moving machinery; lifts/conveyors and occupancy queries; breakage and destructive state transitions; collision masks, contact/material/impulse delivery and object creation/retirement. Combat effects/damage remain separately owned, with the actual physics events delivered to their owners. |
| Composed runtime | Replace the temporary player terrain mover only after real source contact/solver ownership exists. Preserve identity, state ordering, moving supports, player interactions and multiplayer authority. Verify campaign and multiplayer authored scenarios, maps, split-screen and LAN paths. |

## October14 authored multiplayer lifts and contact-agent closure

All20 authored placements of the nine surveyed gravity-lift definitions pass
visible protocol80 source-world multiplayer acceptance, with3,633 exact UDP
observations and1,053 actual upward force deliveries. Seventeen placements use
ordinary no-jump approaches; all three raised Midship pillar fields use one
normal jump and pass actual-force/post-entry-rise gates. The Midship contact
path includes16 added complete original routines and1,457 instruction sites.
All six moving passenger/turret cases also pass on the current coherent build,
with4,053 exact UDP states and original actor/driver cameras. Fresh movement
and object recreation/blast/debris regressions pass182 states each; all512
captured point-impulse motion bytes match unchanged TU1.5. Moving machinery,
retail quantitative trajectories and full physics scope remain open. See [live lift integration](MP_LIFTS_LIVE_TU15_20261014.md).

## October14 complete driver matrix and activated gate

All eight surveyed driver/controller choices pass visible protocol80 multiplayer
in `current-driver-matrix-v314`: Warthog, Ghost, Banshee, Scorpion, Wraith,
Spectre, Secondary Spectre and Gauss Warthog. The matrix records 6,615 exact
UDP snapshots, ordinary entry/driving/steering/aim/exit and actual original
vehicle cameras. See [vehicle evidence](MP_HEAVY_VEHICLES_CAMERA_TU15_20261014.md).

The Zanzibar gate switch exposes and now closes a further real integration
gap. Complete original `11E100`/`11E180` activation owners run from the normal
control pickup path; completed original device/group readback replaces the
independent live native device clock. Visible `gate-control-live-v319` reaches
full opening in both original and native state, with zero differences across
5,572 paired machinery observations and 902 exact UDP states. See
[machinery integration](MP_MACHINE_LIVE_TU15_20261014.md). Script routing,
supported riders, remaining physics branches and full original behavior
comparison remain required; these matrices do not complete the full goal.

## October14 original player-team birth and cooperative Spectre

Actual native roster delivery now precedes source player allocation; complete
original mask/configuration owners admit the selected team. Actual player
identity/team/configuration inputs also reach original biped creation. Native
LAN protocol78 carries the displayed vehicle action/object/seat into the
original command rather than selecting another target during delivery.

The cooperative Spectre passenger1 case now passes visibly with813 exact UDP
states, source team0 on all three actual unit bodies, normal driver entry,
28.907700 units of occupied travel, passenger aim/exit and698 passenger/576
driver original-camera submissions. Camera position/basis/FOV match authority
packets. The source runtime stays stable and its live desktop mirror is
verified continuously. All six moving passenger/turret combinations pass in `v282`/`v283`;
the release-frame target/duration regression also passes. Team departure/change,
quantitative retail comparisons and full stated physics scope remain open.
See [player-team birth evidence](MP_PLAYER_TEAM_BIRTH_TU15_20261014.md).

## October13 live integration checkpoint

October14 fixed light-option integration: unchanged `15CEF0 ->15CB60` and the
native cold-template loader agree on91 Primary/Secondary Light cases and45
authored child templates. The frontend validates the selected option rather
than Map Default; affected source-free authority stepping remains rejected.
Recorded Spectre `v238` passes17.249096 units of driving and902 exact UDP states;
Gauss Warthog `v243` passes32.557823 units and901 exact states, both with0.6
radians of actual original vehicle-camera response. A consistent isolated
source build was required after a shared attachment-header edit produced
28/56-byte ABI disagreement in the earlier linked candidate. All failed
recordings/logs remain retained. The Gauss path also exposed a legal original
cross-tag attachment block; the isolated runtime adopts the root's containing
owner lookup rather than discarding those authored rows. See
[light vehicle source and live evidence](MP_LIGHT_VEHICLES_TU15_20261014.md).
Latest isolated `v245` repeats Gauss/Spectre with902 exact states each and
checks the actual submitted vehicle definitions and geometry in both views:
1184/1045 submissions per view. Their actual travel is34.480911/18.908596 units,
and both submit0.6 radians of original seated-camera response. Isolated
movement/lift/object regressions also pass, including the two original impulse
comparisons over all512 motion bytes.

The earlier independent client-preference case failed: Spectre host option5 and
client Map Default0 joined, then the client's Warthog hierarchy rejected the
authority's WORLD bank. Native protocol76 now discovers complete ordered host
settings before LAN client world preparation and verifies the resulting asset
fingerprint. Recorded Spectre/Gauss/Wraith cases also override a different saved
starting weapon and pass actual geometry, controls and original cameras. Fixed
Secondary Light parameter77 is admitted after its original selector/loader
integration. Discovery while a LAN host is loading answers on a transport
service; queued gameplay admission still runs on its owner thread. Original
light choices, full native multiplayer, configuration fragmentation and loading
ownership tests pass. See [session configuration evidence](MP_SESSION_CONFIGURATION_TU15_20261014.md).
Actual ONLINE loading configuration adoption, all vehicle/seat, random-birth,
campaign and retail-trajectory coverage remain open.

Additional visible seat coverage now passes Warthog passenger/turret, both
Spectre passengers, Spectre turret and Gauss turret with actual source-selected
seats, ordinary input, original camera response and exact UDP WORLD state.
A second real player also walks around, boards and drives the Warthog with an
occupied passenger:5.340403 units of actual travel,901 exact states,816 passenger
and929 driver original-camera submissions. The original Scorpion rider flags reject player occupants; that restriction is
being verified explicitly rather than allowing unsupported player entry.
NPC rider/controller coverage remains open.
See [vehicle seat integration](MP_VEHICLE_SEATS_TU15_20261014.md).

October14 camera correction: remote seated controlled actors skipped native
look-input retention. Authority/client camera equality checks missed the shared
frozen-look defect. The live binding now updates seated yaw/pitch while retaining
source physics ownership. Recorded `v236` Scorpion, Wraith, Ghost and Banshee
sequences additionally require a real submitted-camera response to mouse aim;
each passes with0.6 radians of camera turn. Scorpion/Wraith also pass ordinary
heavy-option construction, boarding, driving, turning and exit, with901/902 exact
UDP states. Movement/lift/object regressions pass with181 states each and all512
actual point-impulse motion bytes still match unchanged TU1.5. See
[heavy vehicles and seated look evidence](MP_HEAVY_VEHICLES_CAMERA_TU15_20261014.md).
Warthog `v237` also passes its longer drive and deliberate mouse-turn sequence:
36.505600 units,0.862927 radians of vehicle turn,0.6 radians of camera response,
631 original seated camera frames and901 exact UDP states. This does not
complete all vehicle classes, seats, camera modes or retail trajectories.

The current checkout builds `halo2_native` and
`physics_multiplayer_acceptance`. `live-root-v151` passes seven recorded live
cases: local vehicle/lift/objects and UDP vehicle/lift/movement/objects. The
UDP cases compare301 vehicle snapshots or181 snapshots in each other case,
including identities, parent/seat, actual WORLD hierarchies, scene roots and
projectile/effect publications. Every selected case retains a nonempty probed
X11 recording. This checkpoint supersedes earlier red live-status statements
below; historical component seals remain unchanged.

| Live integration | Current evidence and remaining coverage |
| --- | --- |
| Retained TU1.5 world and bipeds | Actual source-created bodies, world stepping, root/WORLD readback and normal delivered SDL inputs run in the match. Visible walking/jump/crouch and lift travel pass; all support/contact/movement modes and retail trajectories still need coverage. |
| Warthog | Normal entry, forward driving, steering and exit pass locally and over UDP. The actual245EA0 authored camera reaches rendering; UDP camera position/basis is checked against authority output. Other vehicle/controller classes, seats and quantitative retail contact/force behavior remain open. |
| Grenades and prop blast | Actual source prepare/create/release/flight/detonation/retirement passes locally and over UDP. Current-root `v230` also renders the actual source-created debris crate and dropped grenade equipment, adopts authored current regions, and passes prop movement/retirement/recreation with181 exact UDP states. Two captured point impulses still compare all512 motion bytes against unchanged TU1.5. Other object types and all destruction/model-region paths remain open. |
| Scene replicas | Complete authored roots/WORLD banks, live query inputs and actual creation ticks reach client presentation. Protocol73 additionally carries source-created equipment/garbage/crate state without allocating client bodies or selecting new model/RNG results. `v230` renders the debris owner174 times in each client view and observes1.591729 units of actual source movement. Arbitrary definitions, late-join resources, attachments and all model-region histories remain open. |
| Delivery | The world constructor automatically selects TU1.5 physics for ordinary authoritative Slayer. Production `halo2_native` and acceptance targets build with the seated camera fix in `v236`; `v237` extends the Warthog fixture. Earlier direct-peer launch/render evidence remains historical: current launcher-info checks select `.local/map-world-shape9-20261014/halo2_native` and report MCC-only assets with native MCC gameplay unavailable. This task's latest TU1.5 physics is verified against original Xbox maps through its explicit native test/runtime paths; current packaged delivery remains unverified. |

Current-root `v164` also passes the Ghost with an actual seated camera and
exact UDP FOV submission checks. Original CB3D0/CB090 seat selection is wired
into the live world and passes the Warthog and Ghost sequences. Banshee `v190`
passes boarding, open-flight travel10.374212 units, climb3.404600 units, turning
and exit, with435 submitted original camera frames and902 exact UDP comparisons.
These bounded native sequences still require quantitative retail comparison
and coverage of the other vehicle classes/seats. Current-root `v217` repeats
Warthog/Ghost camera, movement and lift passes after the prop integration. Its
long Banshee flight reaches an additional rigid-body deactivation callback;
the original gameplay admission and motion checks are wired. Banshee `v219`
passes13.888749 units of travel,5.631904 units of climb, turning and exit with902
exact UDP comparisons and532 original camera submissions. The local/UDP prop
blast also passes again after this deactivation integration. [Prop blast
evidence](MP_PROP_BLAST_TU15_20261014.md) supersedes the
earlier private-only impulse and red post-explosion checkpoints.

Current-root `v221` also passes the scene-generation lifecycle above and repeats
the original point-impulse comparison on its recreated source body: all512
motion bytes match. Its UDP Warthog/lift regressions pass with301/181 exact
snapshots; the Warthog submits292 actual original seated cameras and135 views
after exit. The first recreation recording proves geometry submission;
`live-root-scene-observer-v226` also passes with a surviving observer facing the
recreated prop for58 actual render frames, both generations' geometry in both
views,182 exact UDP states and another512-byte original impulse comparison.
The fixture also exposes and fixes keyboard/mouse routing to an explicitly
assigned second local view. Additional original collision-mask storage and
inverted-contact query routines are joined after their actual live boundaries.
The same `v226` runtime repeats Banshee flight, turning and exit with902 exact
UDP comparisons,562 original seated-camera submissions,13.382885 horizontal
units and5.403625 units of climb. Its UDP lift also passes181 exact comparisons.
The production `halo2_native` target builds with these changes. This does not
verify the separately selected current MCC-only launcher package.

The [source-created object integration](MP_DYNAMIC_OBJECTS_TU15_20261014.md)
adds actual model-region/color readback and dynamic presentation. Its field
readers compare against unchanged TU1.5 BC6C0/BCD80. Current-root `v230` passes
the live debris/prop sequence, Banshee camera/flight, lifts and movement with
181/902/181/181 exact UDP comparisons. These are bounded composed checks;
they do not establish every dynamic definition, controller or retail trajectory.

[Vehicle camera evidence](MP_VEHICLE_CAMERA_TU15_20261013.md) and
[scene replication evidence](MP_SOURCE_SCENE_REPLICATION_TU15_20261013.md)
retain the source owners, recordings and limits. Full1.5 physics parity is
unfinished; these selected successes do not close the required coverage matrix.

## Initial verified boundary (historical)

The preexisting native world component has extensive authenticated bodies, but
the live `Simulation::move_unit` path still uses the temporary host terrain
mover. [World/motion progress](PHYSICS_ENGINE_PORT_20261010.md),
[solver progress](PHYSICS_SOLVER_PORT_20261011.md) and
[world callback work](PHYSICS_WORLD_LISTENER_ABI_RUNTIME_PORT_20261011.md)
record their individual boundaries. Another active chat is extending that shared
world/contact graph; this goal's initial implementation adds a separate vehicle
control component.

`source_vehicle_control_accelerate` ports complete `208D80` / `20D680`.
`source_vehicle_control_approach` ports complete `208E70` / `20D770` and its actual
acceleration callee. Their real vehicle-update/controller callers are separately
authenticated in both executable images. Native execution preserves authored
limits, forward/reverse rates, turn-through-zero interpolation, original x87
multiply/float-store boundaries, SSE comparisons, target clamping, and every
value store, including repeated stores and aliasing with the rate bank.

The initial differential corpus executes **33,696 cases per original image** and
compares each with the native selected-image API: **67,392 native comparisons,
118,824 ordered stores and 4,313,088 full output-bank bytes**. All 94 original
instructions are exercised. Inputs include all four rounding modes, x87 24/53/64
precision, signed zero, finite boundary values, subnormals, infinities, quiet
NaNs, reversal, overshoot and value/rate aliasing. This is declared scalar input
coverage, not that many vehicle-driving scenarios.

The semantic API checks the output bank, store ordering and target result. It
does not preserve an original caller's scratch stack, registers, flags or x87
stack. A future controller integration must supply those owners wherever they
are observable. The x87 backend currently requires GCC/Clang on x86; masked FP
exceptions are required. Complete driving controllers, force/body delivery,
seats, suspension, hover/flight classes, collisions and live vehicle scenarios
remain unfinished.

Evidence is retained under
`analysis/multiplayer/sandbox-source/vehicle-control-20261011/`. The original-only
oracle never executes the native implementation and patches no instructions.
`tools/research_vehicle_control_20261011.py` authenticates the complete caller
chain; `tools/oracle_vehicle_control_20261011.py` independently executes both
original images. `original_source_vehicle_control` runs the native comparisons.
Release and sanitizer evidence must be recorded separately from live parity.

## TU1.5 vehicle remainder and retained FP state

The [TU1.5 remainder component](PHYSICS_VEHICLE_MOTION_MATH_TU15_20261011.md)
adds 16 complete native bodies and 443 distinct instruction sites. Its composed
graph has 96 bodies and 11,056 sites. A test-only Linux i386 harness single-steps
the owned TU1.5 instructions on the host CPU and preserves actual unported-owner
boundaries. Native comparison passes 8,598 cases, including 506,987 ordered
writes, full retained state, active extended values and FP controls. Original
hardware execution, rather than Unicorn's precision-flag and signed-zero
behavior, is the final authority for this component. Only TU1.5 execution is
part of the new acceptance evidence.

The sealed 53-file source cohort passes all four TU1.5 vehicle checks in Release
and under ASan/UBSan. Production passes those four checks and ten TU1.5 world
checks. Supplemental contact checks pass 50 complete cases and two retained
earlier-boundary cases with the new shared headers. The
[validation record](../analysis/multiplayer/sandbox-source/vehicle-motion-math-20261011/validation.json)
pins source, generated code, repeated original hardware fixtures, binaries and
test logs. Its SHA-256 is
`57b1b0db72d685185cbefab2ba9ab269acdf4e1f59eddcd68fa22984105441bb`.

The shared ABI retains FP controls, MXCSR and x87 sticky exception flags through
nested owners. Original source bytes remain outside production execution.
Tank and wheeled staged paths now reach TU1.5 contact-state delivery `20FD70`;
their declared no-solver inputs do not prove suspension or live contacts.
Unported math-error, contact/solver, animation and activation owners, live
movement integration and the rest of the required scope remain open.

## TU1.5 contact-state delivery and reference release

The [contact-state component](PHYSICS_VEHICLE_CONTACT_STATE_TU15_20261011.md)
adds complete TU1.5 `20FD70`, `20FCB0` and `22E1B0`: three bodies and 160 sites.
The composed vehicle graph now contains 99 bodies and 11,216 distinct sites.
All new recovery, generation and parity execution use the owned TU1.5 image.

Native comparison passes 12,518 hardware-backed cases: 10,086 complete calls
and 2,432 actual required-owner prefixes, 3,433,952 observer events, 581,756
ordered writes and 1,865,363 retained bytes. The hardware observer single-steps
3,430,820 original instructions. Repeated recovery, generation, diagnostic
original execution and original hardware execution match exactly. Release,
ASan/UBSan and production checks pass. The sealed source cohort includes 90
native and local Python source files; unchanged parent archives are reused for
standalone checks with their hashes recorded.

All 432 independently computed builder-to-motion cases now return, including
27 tank and 27 wheeled inputs. Their declared component still has no actual
vehicle solver object or constraint list, so this is bounded state/force
calculation evidence. It does not establish suspension, collisions or driving.
151 of the 160 new sites are exercised; nine sites after required cleanup
calls remain unexercised at that stage. That stage identified TU1.5 cleanup
`22E390` (121 instructions), `22FE30` (27) and `22FEA0` (775) as its next
frontier. The following component ports those bodies and their reached
dependencies while preserving deeper required owners.

The [validation record](../analysis/multiplayer/sandbox-source/vehicle-contact-state-tu15-20261011/validation.json)
pins source, tools, repeated original fixtures, native binaries, parent archives
and logs. Its SHA-256 is
`b6c71f4725c803f9e47f4a39b7627699baf6a34327a722f8908bdb4471040430`.
Full player, vehicle and object physics parity remains incomplete.

## TU1.5 constraint cleanup, allocation and force delivery

The [constraint-cleanup component](PHYSICS_VEHICLE_CONSTRAINT_CLEANUP_TU15_20261011.md)
adds 21 complete TU1.5 native bodies and 2,565 distinct sites. Its composed
vehicle graph contains 120 bodies and 13,781 sites. The already compiled
TU1.5 `1D8C80` is reused. All new recovery, generation and parity execution
use only the owned English TU1.5 image.

Native comparison passes 15,141 original hardware cases: 12,689 complete calls
and 2,452 actual required-owner prefixes, 3,566,476 observer events, 603,422
ordered writes and 2,010,369 retained bytes. The hardware observer single-steps
3,563,085 original instructions. Repeated recovery, generation, diagnostic
original execution and hardware execution match exactly. Release, ASan/UBSan
and all six TU1.5 production vehicle checks pass. The sealed source cohort
contains 99 native and local Python files; the unchanged validated parent
archives are reused with their exact hashes recorded.

The new corpus includes 120 real datum-pool allocations, 40 source-created
reference records, 80 force updates, 12 cleanup-to-force calls and 288
contact-basis cases. Both implementations compute their own allocated slots,
force records and linked 13-float transform copies. New floating-point paths
preserve the original defined x87 C1 result as well as retained extended values,
FP controls, MXCSR and sticky exceptions. 1,351 of the 2,565 new sites are
exercised. Object lifecycle `178E30` is compiled but unexercised, and 1,214 new
sites remain uncovered.

The [validation record](../analysis/multiplayer/sandbox-source/vehicle-constraint-cleanup-tu15-20261011/validation.json)
has SHA-256
`41748ed540d156528c696f472011771673027280757c4445787a6eacf2b53f57`.
Deeper object, activation, point/pose-transform and force-cleanup owners remain
required. The 432 staged builder-to-motion inputs still declare no actual
vehicle solver object or constraint list. The live `Simulation::move_unit`
still uses the temporary host terrain mover. World/contact/solver ownership,
live player/vehicle/object integration, complete controller classes and authored
scenario traces remain required. Full physics parity is incomplete.

## TU1.5 environment queries, pose composition and force retirement

The [environment-state component](PHYSICS_VEHICLE_ENVIRONMENT_STATE_TU15_20261011.md)
adds 26 complete TU1.5 bodies and 1,307 distinct sites. The composed vehicle
graph contains 146 bodies and 15,088 sites. New owners cover point/portal and
packed-plane-tree queries, cluster/category and activation metadata, datum and
linked-record retirement, force detachment, scalar delivery and source pose
composition. All new recovery and original execution use English TU1.5 only.

The corpus contains 16,381 original hardware cases: 13,942 complete calls and
2,439 actual required-owner prefixes. It exercises 1,226 of the 1,307 new sites
and 10,048 sites across the composed graph. The hardware observer single-steps
3,648,075 original instructions; the normalized native comparison checks
3,651,499 observer events, 613,055 ordered writes and 2,082,340 retained bytes.
Repeated recovery, generation, diagnostic original execution and hardware
execution match exactly. All 432 staged builder-to-motion cases return with
their declared inputs; these still lack a vehicle solver object and constraint
list and do not establish solved contacts or driving.

The 108-file source cohort pins three shared ABI header changes for explicit
MMX state ownership. The accepted standalone builds compile the full graph
from source, because the earlier parent archives predate that ABI. These
vehicle inputs enter with no active MMX state; mixed MMX/x87 parity remains
outside this corpus. Release, the full ASan/UBSan rebuild and differential
check, and all seven TU1.5 production vehicle checks pass.

The [validation record](../analysis/multiplayer/sandbox-source/vehicle-environment-state-tu15-20261011/validation.json)
pins the 108-file source cohort, repeated original fixtures, full source builds,
native binaries and successful logs. Its SHA-256 is
`239bb07c92de97930be7ccb27400982621915e21e1377e8165fd1c48c8a8c50a`.

81 new sites remain unexercised. Linked-record cleanup `178290`, object
lifecycle and other retained required owners remain open. `223B20` is the
sound scheduler submit owner, not physics activation. Its presence in a
physics callback chain does not make its implementation a physics requirement.
The [semantic correction](PHYSICS_TU15_OWNER_SCOPE_CORRECTION_20261011.md)
supersedes the earlier labels while preserving the sealed evidence.
Existing bounded vehicle cases are not an authored live scenario.

## TU1.5 world startup remains required

The latest shared [world-construction component](PHYSICS_WORLD_CONSTRUCTION_RUNTIME_20261011.md)
contains 421 authenticated bodies and 33,237 sites. Its complete
116,677-instruction construction-prefix scenario still executes the older
image, so it does not satisfy this goal's TU1.5 construction requirement.
Earlier TU1.5 world checks explicitly rebind retained raw inputs; they do not
establish execution of the TU1.5 startup caller or complete game-world
construction.

The next shared-engine acceptance work must establish TU1.5 process/heap,
world/BSP/body and controller construction with original TU1.5 inputs and
composed traces. Actual allocation, lifetime, contact/solver ownership and
solved readback must precede replacement of the temporary live terrain mover.
Player, vehicle and object integration, all authored controller classes,
campaign/multiplayer scenarios, split-screen and LAN remain required.

## Fresh TU1.5 startup record owners

The [TU1.5 startup-record component](PHYSICS_TU15_STARTUP_RECORDS_20261011.md)
adds native ownership of the exact DPC and timer-record initialization calls
reached before process-heap creation. It composes the existing source heap and
shutdown owners, validates the actual TU1.5 return/argument frames and preserves
all fields outside those initializers' ownership. Timer scheduling, DPC dispatch
and hardware/settings outcomes are not supplied by these operations.

24 fresh TU1.5 procedure-entry prefixes use literal image/header storage and
declared standalone stack/CPU inputs. They restore no old world checkpoint and
execute no older image. Independent original execution and compiled primary
record expressions match 504 source instruction observations, 348 source
stores, 360 record stores and 48 registry stores, including their interleaving
and retained CPU/memory state. Another 36 patterned record cases compare 270
independent primary writes and untouched fields. All 169 instructions in the
three retained startup/setup/timer-arm bodies are authenticated; these prefixes
exercise 28 distinct sites. Repeated original recovery, primary compilation
and measurements are identical.

The 108-file cohort passes sealed Release, ASan/UBSan/leak and normal project
target checks. The [validation record](../analysis/multiplayer/sandbox-source/tu15-startup-records-20261011/validation.json)
has SHA-256
`40df49b2c1698e39c63d913bc1f9b275c9ce6270dd8a0a56898097ce77e2eefd`.
This establishes declared native record/void-ABI semantics against the pinned
primary compatibility implementation, not original Xbox kernel clobbers.

That sealed stage reached TU1.5 `2E00E7`'s unresolved hardware-data322 read through
`41B66C`. The encoded import is not a hardware record. Original measurement
stops before consuming it, and native execution refuses the missing bank while
retaining the same preceding state. Hardware data, conditional settings/error
conversion, real timer clock/queue/dispatch ownership and later startup owners
remain required. The process heap has not been installed; the XBE entry point,
full TU1.5 startup/world creation and live physics remain unproven.

## TU1.5 phantom forces and body velocity delivery

The [phantom-force component](PHYSICS_PHANTOM_FORCE_TU15_20261011.md) adds
12 complete TU1.5 bodies and 1,314 instruction sites. The composed dependency
graph contains 158 bodies and 16,402 sites; semantic sound/effect dependencies
in the retained graph do not count as exclusively physical behavior. The new
owners follow the actual vehicle call into phantom updater `1DA750`, physical
membership and force-factor logic, and the real linear/angular velocity methods
selected by the original motion vtables.

528 independently executed TU1.5 cases contain 377 complete calls and 151
required-owner prefixes. Of 193 complete updater calls, 153 reach both motion
setters, 81 change linear velocity and 15 change angular velocity. Original and
native execution compute these results independently from declared raw component,
body, tag/model/physics, membership, motion and island inputs. These records
were not created by a source factory or collision producer in this corpus.
The comparison retains 195,014 normalized instruction observations, 39,156
ordered writes and 153,663 bytes; hardware single-steps 194,838 unchanged
original instructions. Recovery, generation, diagnostic original execution and
original hardware execution repeat identically. 1,214 of 1,314 new sites are
exercised, leaving 100 uncovered sites.

The sealed 118-file source cohort passes all eight TU1.5 vehicle/phantom suites
in Release, ASan/UBSan with leak checks, and the normal project build. Parent
sources are unchanged and every standalone dependency is rebuilt from source.
The [validation record](../analysis/multiplayer/sandbox-source/phantom-force-tu15-20261011/validation.json)
has SHA-256
`9b106d2991f0b2521668c03b360440c731cc869848a13b83c1cc41f29c831a2b`.

CRT power `37CE0C`, world activation `3165D0`, empty-membership allocator
ownership, TU1.5 source-created world/body/contact state and live integration
remain required. The retained world-construction graph already compiles
`3165D0` under its internal `312A90` identifier, and `316D40` under `3131C0`.
Their phantom composition needs valid TU1.5 world state and the actual ABI
binding; duplicate recovery of those bodies is unnecessary. There is no host
power or fabricated allocator/activation result.
This is physical force and velocity-delivery evidence, not a working
live lift, vehicle suspension, solved collision trajectory or authored scenario.
The full goal remains active.

## Acceptance

The user explicitly requires **visible testing in actual multiplayer** so the
physics can be seen working, as of 2026-10-11. Feature acceptance must include
real authored-map match steps, normal input, split-screen and LAN authority
checks, saved tick observations and playable recordings. Recovered components
must own the live bodies/contacts/world; merely linking them or showing motion
from the temporary mover cannot establish parity.

The [first visible TU1.5 acceptance run](PHYSICS_MULTIPLAYER_VISIBLE_TU15_20261012.md)
records native walking/crouching/jumping, a bounded grenade lifecycle and exact
same-tick UDP host/client movement observations. **The authored Lockout lift is
red (zero upward velocity), and Coagulation driving is unavailable (six authored
vehicle candidates, zero live scene vehicle roots).** Other dynamic-object
impulses and the complete original contact/solver integration remain open.
The videos retain these failures; neither a component pass nor this bounded
native gameplay check completes 1:1 physics parity.

Each missing owner must be recovered and integrated rather than supplied by a
recorded output or successful default. Completion requires the full shared
engine, every applicable player/vehicle/object path and real composed original
versus native scenario traces. Builds, scalar fixtures and isolated solver
comparisons provide evidence for their specific contracts; they do not establish
continuous gameplay or Xbox hardware equivalence. The goal stays active until
the entire stated scope has supporting evidence.

## Wiring each completed system

Per the user's explicit requirement, each completed system must be wired into
its actual runtime caller as it is finished. Linking a library or passing an
isolated fixture is not completion. Required evidence includes source-created
input/lifetime owners, original tick ordering and state readback, and actual
multiplayer operation with saved traces and recordings. Until those exist,
record the component as procedure-verified and keep its live integration open.
Prioritize the fresh TU1.5 world/body/contact producers needed to connect the
already ported systems, rather than treating additional isolated bodies as
finished gameplay.

The new activation callbacks are connected to their real source callers inside
the compiled graph: velocity setters enter `315CB0`, world activation enters
`316D40`/`322660`, and installed world/body listeners dispatch to the new native
bodies. All 36 composed wake cases exercise that chain. `Simulation::step` still
has no source-created world/body context to call this chain; the live lift and
vehicle failures remain required integration work.

## TU1.5 phantom/world activation composition

The [activation component](PHYSICS_PHANTOM_WORLD_TU15_20261011.md) joins the
verified TU1.5 vehicle and current world graphs and adds five complete native
callbacks and helpers, 477 sites. The compiled union contains 609 bodies and
51,007 sites. Constructor stores authenticate the installed world/body listeners
and the body owner getter. Entry, instruction and store observations use actual
TU1.5 addresses across component boundaries.

The independent original i386 hardware corpus contains 671 declared-input
procedure cases: 521 complete returns and 150 retained CRT power frontiers.
It records 233,894 hardware instructions, 234,090 normalized observations,
46,208 ordered writes and 191,303 state bytes. All 36 lower velocity-setter
cases wake sleeping islands and run both real installed listener classes before
storing velocity. The full phantom updater skips inactive members, preserved by
20 additional negative cases. Only 249 of the 477 new sites are exercised;
deeper pair-agent and constraint builders remain outside this corpus.

This is raw declared world/body/island input evidence. It does not create live
bodies or contacts and does not resolve the recorded multiplayer lift/vehicle
failures. The live mover still uses host terrain collision. Full TU1.5 factory,
world/contact producers and actual solved readback remain required.

The production build exposed an unexercised parent-world lowering defect:
TU1.5 `2830FE` is a qword x87 load from `46A688`, but the shared world generator
emitted a float load through its four-byte scalar reader. The
[TU1.5 callback correction](PHYSICS_WORLD_QWORD_TU15_20261012.md) now reads the
binary64 value directly into extended precision, and the scalar reader rejects
oversized operands before touching memory. This is installed in the shared
477-body world graph; regeneration changes exactly one lowered instruction.
Exact original callback and shape-constructor bindings were retained in
`analysis/multiplayer/sandbox-source/phantom-world-tu15-20261011/shared-world-qword-load-frontier.json`.
That original frontier record remains historical. The new original TU1.5
hardware corpus completes 288 callback cases, including 240 executions of the
qword load, 31,488 instructions and 4,512 ordered writes. The pre-fix native
binary terminated with stack corruption; the corrected Release, ASAN/UBSAN/leak
and main-project builds each pass all 11 selected TU1.5 checks. The sanitizer
suite completed in 697.62 seconds. Results and the immutable source cohort are retained under
`analysis/multiplayer/sandbox-source/world-qword-tu15-20261012/`.
Its `validation.json` SHA-256 is
`86ee686a0fe715800a528afe8c310ddc0764becd71b2bdbd861d4c9d644f88c3`.
These comparisons consume declared raw pair/shape/transform inputs. Fresh body
and contact creation, live world ownership and the multiplayer lift/vehicle
acceptance failures remain open.

The activation slice is sealed against frozen source-v5 by
`analysis/multiplayer/sandbox-source/phantom-world-tu15-20261011/validation.json`
(SHA-256 `f04a4799672b5e50e43d2a39a26db17dc628d4d36e66e1dbd0bf2737a6ccdab4`).
All nine selected TU1.5 checks pass in Release and ASAN/UBSAN/leak builds.
Production registration separately passes the same nine checks using the
recorded v18 world source. The newer 477-body graph with the qword correction is
verified separately by the callback correction cohort above. The earlier
activation seal and its v18 production record are not rewritten to claim those
later inputs. No new live gameplay parity is claimed by either component slice.

## Fresh TU1.5 kernel data and startup dispatch ownership

The [kernel-data startup component](PHYSICS_TU15_KERNEL_DATA_STARTUP_20261012.md)
adds actual externally owned export322 input to the authenticated native image
loader and public startup join. Two isolated fresh xemu boots and an additional
same-boot read retain identical hardware/kernel-version data and export
metadata. The reference is the user's owned modified Complex4627 BIOS in xemu,
with private EEPROM copies and HDD overlays; it is not physical Xbox or title
execution proof. The observed hardware record differs from the compatibility
library default, so no default hardware flags are substituted.

The loader authenticates the whole TU1.5 image, binds only the actual four-byte
import cell at `41B66C` before mounting its read-only section, and retains the
actual eight-byte record as immutable resource backing. The public startup
wrapper now observes the current generic DPC/timer dispatcher, fixing its
previously unreachable interception. Missing ownership stops explicitly before
the original `2E00D4` load. The older sealed missing-data fixture is preserved;
its regression derives that earlier boundary from the independent timer entry
and declared preserving void ABI.

All 24 fresh original/native startup cases now reach the unchanged settings
callee at actual `2E0FE2`, retaining 744 source instructions, 492 source stores and
408 primary platform stores. Exact call order, pending frame, memory and live
CPU state compare, with no settings result supplied. All three selected TU1.5
checks pass fresh Release and ASAN/UBSAN/leak builds; both startup checks also
pass the main-project build. Final source-v3 contains 76 files; its native inputs
are byte-identical to compiled source-v2. The only v3 change fixes hash-variable
shadowing in the evidence validator. Both root and frozen validators reproduce
the final seal, `validation.json` SHA-256
`e7e36d375d773028bd22f7574bf56d7833892d3f0b1323cf7bf5ae544a4b32b3`, under
`analysis/multiplayer/sandbox-source/tu15-kernel-hardware-20261012/`.

The next authenticated dependency is the original settings wrapper/thunk,
which calls EEPROM query 24 for `XC_MISC` and status conversion 301. The original
startup later invokes timer 149. Static original listings and pinned primary
compatibility references are retained in `settings-owner-research-v2/`; these
owners have not been supplied successful defaults. The full startup caller,
heap installation, world/body/contact producers and actual `Simulation::step`
integration remain open. The multiplayer lift/vehicle failures still require
live acceptance after that integration. This startup component is not a
completed player, vehicle or object physics system.

## TU1.5 settings caller and immutable query owner

The [settings startup component](PHYSICS_TU15_SETTINGS_STARTUP_20261012.md)
now connects the complete original settings wrapper/thunk to the fresh startup
caller. The query consumes the owned EEPROM's actual UserSettings section and
computes its checksum. Invalid data follows the original failure path. No
settings-success return or kernel default is substituted. Independent original
execution uses pinned primary query/checksum/lookup code with explicitly
declared immutable resource, synchronization and byte-copy primitives.

All 48 fresh original/native prefixes compare through the actual status
conversion call: 2,016 source instructions, 1,416 source writes and 264 query
writes, with 24 actual successes and 24 checksum failures. Release and
ASAN/UBSAN/leak builds each pass four selected TU1.5 checks. The main project
passes all three startup checks. Production artifacts are retained in an
immutable snapshot. Root and frozen validators reproduce the evidence seal at
`analysis/multiplayer/sandbox-source/tu15-settings-startup-20261012/validation.json`,
SHA-256 `ba97568822148cee816511208e6f5415ca417a7cd8afcffe37774120bd522ab1`.

Status conversion301 and timer149 remain required. The source process heap,
world/body/contact creation and `Simulation::step` integration are still open;
the visible multiplayer lift and vehicle failures remain acceptance failures.
This component supplies no new live physics parity claim.

## TU1.5 complete settings return and connected status conversion

The [status startup component](PHYSICS_TU15_STATUS_STARTUP_20261012.md) now
connects the complete settings return path into the actual fresh startup
caller. It statically translates the complete owned modified Complex4627
export301 and consumes its exact immutable mapping tables. Two isolated boots
retain identical code/tables. No mapping result, Windows default or rewritten
table is supplied. This reference proves the owned modified BIOS routine,
with physical Xbox behavior explicitly unverified.

All 48 fresh original/native prefixes now complete query, status conversion,
source wrapper return and caller success/failure stores, then reach the actual
timer149 frame. They retain 2,760 title instructions, 1,800 title stores, 8,112
kernel instructions and 144 kernel stores. A separate 786-case corpus covers
every mapped status and range edges through 366,026 original instructions;
58 of 59 sites execute. The remaining jump is unreachable with the preserved
tables and the routine's input-bit early return. Both corpora and native
generation repeat byte-identically. Six selected TU1.5 checks pass fresh
Release and ASAN/UBSAN/leak builds, and all five production startup checks pass.
Root and frozen validators reproduce the immutable source/artifact seal at
`analysis/multiplayer/sandbox-source/tu15-status-startup-20261012/validation.json`,
SHA-256 `3badeb60e3975b150330470a587f453cc81b2c77687d17393ffefea4136c23db`.

Timer149, full process startup and source heap/global installation remain
required. No source world/body/contact result has been supplied. The
`Simulation::step` integration and visible multiplayer lift/vehicle acceptance
failures remain open; this connected startup prerequisite is not a completed
player, vehicle or object physics system.

The next timer owner's original modified-BIOS bytes are retained under
`analysis/multiplayer/sandbox-source/tu15-timer-startup-20261012/`. Static
recovery authenticates complete149/150 functions (26/186 bytes, 7/72 sites),
including the actual call to150 and six remaining kernel helpers. Export
metadata identifies129,163 and119 among those helpers; three are internal.
The actual title frame requests relative time `-216000000000` in100ns units.
Neither scheduler/clock state nor a timer return has been supplied. These
static windows establish the next owner boundary, not completed scheduling.

## Connected TU1.5 timer callers and original queue construction

The [timer prerequisite component](PHYSICS_TU15_TIMER_STARTUP_20261012.md)
now connects original149/150 to the public fresh startup composition. It
recovers nine complete modified-Complex4627 bodies and304 sites, including
actual timer/ready-queue construction, DPC/timer record initialization and
due/index/insertion arithmetic. Production uses native control flow. Only nine
authenticated read-only arithmetic bytes are mounted from the captured text;
no mutable scheduler state, clock value or timer result is substituted.

The original producer corpus has37 fresh cases,15,536 instructions and6,317
stores. It constructs queues and timer records before dependent calls and
compares maximum64-bit arithmetic, carries, absolute/relative time, expiry and
multiple insertions. Clock values are declared procedure inputs, not a live
clock resource. An ADC/SBB lowering defect found by the maximum input case is
fixed; failed evidence and diagnostic traces remain historical.

All48 fresh startup prefixes now execute the actual timer prologues and reach
`8001434A`, which requires the real interrupt-context byte at `80035C00`.
They retain2,760 title instructions,1,800 title stores,8,832 kernel instructions
and720 kernel stores. No context byte is read or injected. Combined new graph
coverage is210/304 sites;94 remain unverified. Original corpora and generation
repeat byte-identically. Eight selected checks pass fresh Release and
ASAN/UBSAN/leak builds; all seven production startup checks pass. Root and
frozen validators reproduce the immutable119-file source/artifact seal at
`analysis/multiplayer/sandbox-source/tu15-timer-startup-20261012/validation.json`,
SHA-256 `11ef159431dbcac9f0a6f3909a1c3519c812b9f3cf97edb0f8f714dd886acc57`.

Actual native interrupt-context/clock construction, resource lifetime and
expiry/DPC servicing remain required before the timer can arm and startup can
return. The process heap/global, source world/body/contact/controller creation,
`Simulation::step` integration and visible multiplayer lift/vehicle acceptance
failures remain open. This is not a completed scheduling or physics system.

## TU1.5 connected interrupt lowering and loader-phase evidence

The [interrupt-lowering component](PHYSICS_TU15_INTERRUPT_LOWERING_20261012.md)
adds two complete original modified-Complex4627 bodies and67 sites. The actual
timer call now dispatches through both bodies in the public startup composition.
Original IRQ masks are authenticated and read-only. Context, clock, scheduler,
dynamic interrupt dispatch and PIC ownership remain explicit; missing callees
do not receive successful returns. Retained CLI/PUSHFD/POPFD preserve guest
flags without affecting host interrupts.

The independent unchanged original corpus has55 cases,5,298 instructions,
2,021 ordered stores and24 PIC outputs. Nine cases execute original queue/timer
constructors and actual149/150/lowering through their returns, using declared
procedure context/clock inputs. Coverage is44/67 new sites, with23 sites after
missing scheduler/dispatch owners unverified. This is not live timer ownership.
The public composition also preserves all48 fresh startup prefixes and their
actual missing context boundary. Original generation and fixtures repeat
byte-identically.

Two isolated owned boots stop at PE entry8005504C before its first instruction,
with an execution breakpoint and no guest memory writes. Identical loader
metadata, INIT bytes and named initial interrupt/clock records are retained.
Loader zeros do not establish the subsequently initialized title-thread
context; kernel initialization and live clock production remain required.

Ten selected checks pass fresh Release and ASAN/UBSAN/leak builds using the
pinned prior source foundation. Both new production targets build from the
actual root CMake project and both checks pass. An initial configure failure
while tag files were absent remains historical evidence; those files later
appeared in the shared checkout. Production artifacts are retained immutably.
Root and frozen validators reproduce the131-file source/artifact seal at
`analysis/multiplayer/sandbox-source/tu15-interrupt-startup-20261012/validation.json`,
SHA-256 `5d3f4dabb425bc6d73034189df76211b6c1ec99f59887cd198f4589ed3d0f4a3`.

Fresh native context/clock construction, timer expiry/DPC servicing, process
heap/global installation, source world/body/contact/controller creation and
`Simulation::step` integration remain open. Visible multiplayer lift and
vehicle acceptance remains red. The complete physics goal is unfinished.


Current additional checkpoint: 28 lift/player-field placements across 13
map-local definitions have bounded visible multiplayer evidence (5,088 exact
UDP comparisons, 1,374 observed upward deliveries). The broader 43-placement
phantom data inventory includes initially absent and non-player fields. Dune
tower/pod behavior and moving/rotating supported riders remain open. Original
device script APIs now have a 127-site closure, 256 original differential cases
and eight real VM/UDP stages. These are incremental integration gates and do
not close the all-player/vehicle/object TU1.5 parity goal.


Further checkpoint: Dune's normal earlier-jump tower entry and two explicitly
created, initially absent Backwash fields bring bounded placement coverage to
31/15 map-local definitions (5,631 exact UDP states, 1,772 upward deliveries).
Unnamed prop birth exposed and fixed a real client geometry preparation gap;
the 21-map regression and fresh visible lifecycle cases pass. Three captured
live Dune force calls match untouched original i386 execution, all accessible
source bytes and CPU state within the documented host interrupt-flag limit.
A current moving Spectre regression after that production repair passes674
exact UDP states, real original driver/passenger cameras and ordinary exit.
Rotating support, remaining field behavior and full stated parity remain open.


The actual Zanzibar rotating-support gate now passes visibly in v350: no
player walking/jump input, 119 moving supported source ticks, 2.340648 units
of verified carried travel and 349 exact UDP snapshots. Actual source biped
support flags and WORLD-node tracking gate the result. See the machinery
report/video. This is one rotating carrier case; linear carriers, all support
branches and full original trajectory parity are still open.

October20 active campaign continuation: v1143 authenticates the original
introduction material's34-record program dependencies with English TU1.5 as
the offline authority. The actual captured MCC input fails before the fix in
v1145 and passes the production conversion plus8 rejection controls in v1146.
The converter is wired into Cache startup and preserves MCC parameter/bitmap
banks. v1147 builds16 gates including actual Cache admission; after current
v1123 vehicle/launcher qualification, v1148 tests introduction live, v1149
attempts all47 campaign contexts, then v1150 reruns the core. Keep V03 open
until the actual controller/body/UDP/rendered cases qualify.

October20 current continuation: qualified v1123 is wired and actually launched
in two-player MCC (v1133), with zero forbidden runtime inputs. The maintained
checklist now leads with its14 remaining groups and current evidence; historical
receipts are retained below it. New introduction material admission passes in
v1147, which exposes a legitimate globals publication shift. Read-only v1160
proves the1,552-byte relocation with all54 class rows unchanged; the exact new
context remains authenticated. Phantom diagnostic v1159 reaches F4D1A's missing
lazy BSS pair; v1162 authenticates its original zero initialization and literal
operand. Native backing/ownership regression are wired without a computed
result. v1163's frozen renderer compile fails during concurrent declarations;
current source already resolves them. v1171 passes frozen-source syntax and
builds20 gates, then v1172 introduction, v1173 all47 live contexts, v1174 core.
No prerequisite pass or unrun campaign case closes V03.

Completed retirement v1169 removes855,427,984 logical program/shader bytes and
measures266,211,328 bytes net free-space increase. v1170 archives40 completed
helpers (188,841 bytes) into23,972 bytes. All source snapshots, reports, failures,
needed videos and launcher-selected v1123 remain. These scratch roots are gone.
