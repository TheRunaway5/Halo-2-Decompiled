# TU1.5 tested physics runtime delivery

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

## Earlier v614 delivery

`Halo2-Physics-TU15` now pins `tested-runtime-v614/halo2_native`, SHA256
`8d5b36b8130cfd3bc98feb2f0f96ec4a498a5caa9e8996a60f009e88d9ea5076`.
`normal-runtime-delivery-v617/execution.json` passes ordinary authored-spawn
startup with two players,120 frames and352 source ticks,without physics
fixture/spawn overrides. The actual95s recording frame was inspected.
`vehicle-camera-region-integration-v616/lan-vehicle/report.json` passes actual
Warthog driver entry,drive,steering,look and exit with902 exact UDP states,
475 original seated-camera frames,a0.5999999642-radian look turn,34.086475
horizontal travel and486 on-foot camera frames after exit. The8s driving and
25s exited frames were inspected; actual game window and continuous desktop
mirror are verified. Root/frozen `launcher-v614-verified-*.json` receipts prove
the exact runtime/reference hashes and normal authored-spawn CLI.

Actual SCEN/BLOC model regions now reach source publication,UDP client adoption
and renderer catalogs without rerolling RNG. Three unavailable inactive
cinematic resources retain explicit visible-request rejection. Borrowed original
projectile attachment metadata resolves through its actual retained cache owner.
The actual Pelicanv612 passes143 frames per view,360 exact UDP states and source
controller motion; four sampled complete calls match268,399 original TU1.5
instructions. Pelican player seating/camera and the older inactive intro context
remain open. See [shared campaign integration](MP_CAMPAIGN_SHARED_PHYSICS_TU15_20261015.md).
All-player,vehicle and object physics parity remains incomplete. All47 authored
class3 placements,remaining lift/train rider contexts,broader physics branches,
canonical launcher integration and quantitative Xbox reference gates remain open.
Older checkpoints below retain their original bounded scope.

Earlier v451 delivery: `Halo2-Physics-TU15` selected immutable production v451,
SHA-256 `c3d25c01fad7dbf63d34076168bac78c5bae27bc4714de8b275d3d2de23e5a6f`.
It retains the class3 helpers, actual controller admission and original static
contact-agent startup, and validates the original agent/destructor binding
across authority restoration. See
[class3 source integration and live restoration](MP_CLASS3_PHYSICS_INTEGRATION_TU15_20261015.md).

`normal-runtime-delivery-v451` passes120 rendered frames,357 physics ticks and
two local players, exit0, stable executed hash, mapped960x540 game window and
continuous desktop mirroring without errors. It uses authored player starts
and normal CLI setting `--mp-rule 81 4`, with no acceptance input or spawn/body
replacement. The entire run takes119.525seconds including loading/shutdown;
this is not retail frame-pacing proof. Recorded gameplay is visible at95..116
seconds; the actual106-second frame was inspected. The late direct desktop
snapshot was after shutdown and is retained as such. The current launcher pin
is verified in `launcher-info-v451-verified.json`.

The final v453 acceptance ELF passes live coupled authority restoration while
an occupied Ghost is moving, then ordinary driving, steering, original vehicle
camera and exit:491 exact UDP snapshots and8.740213 units of travel. Complete
post-restore object/world calls match120,561 unchanged original instructions.
Earlier v448 Ghost/lift/object regressions retain665 exact UDP observations;
their source/runtime hashes and original scope remain recorded. Fifteen
class3 admission/controller/object/world calls match405,409 instructions.
These bounded comparisons do not establish visible class3 multiplayer, all
cinematic contexts, Xbox hardware/pixels, or the full physics objective.

The earlier v427 delivery retained the contrail integration and passed120
frames/357 ticks/two players in 148.052 seconds. Its immutable pin remains
`a7825205de802e704ebc74f1dac50fe27415346d3fc6e5ebc4513505d455e4ec`.
Earlier v417/v419 and v381/v412 evidence below remains historical.

`./Halo2-Physics-TU15` launches the pinned tested native production runtime on
original Coagulation, with two local players and normal authored spawns. It
uses normal multiplayer runtime/input handling, with no acceptance fixture,
location overrides, camera replay or supplied physical results. The original
TU1.5 world is selected automatically. The wrapper verifies the retained native
executable and owned English TU1.5 reference hashes before execution.

```sh
./Halo2-Physics-TU15 --launcher-info
./Halo2-Physics-TU15
./Halo2-Physics-TU15 --map assets/disc/maps/cyclotron.map --local-players 1
./Halo2-Physics-TU15 --host 29415
```

Normal `--join ADDRESS:PORT` is also forwarded to the native multiplayer CLI.
The current regular `Halo2-Launcher` and `Halo2-Title-Launcher` still select the
separate MCC-only profile and report gameplay not ready. They are unchanged.
This dedicated wrapper exposes the tested TU1.5 runtime for interactive use;
it does not complete the canonical launcher or all-physics parity objective.
Vista imports supply HUD/pause UI only; original Xbox maps and TU1.5 data remain
the gameplay inputs and authority.

The normal windowed `normal-runtime-delivery-v412` case passes:120 rendered
frames, 357 physics ticks,two local players, exit 0 and a mapped 960x540 native
game window. The desktop mirror is verified throughout, and the executed
`/proc/PID/exe` hash equals the retained tested native hash
`a438f1a9f7c1abf69e7d0c30abdab01c4d4f8eb3f511960cb9d5373e281d1acb`.
The original executable hash is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The unaltered raw window recording and actual execution report are retained.
No scripted movement or pickup was used in this normal-startup case; actual
vehicle control/camera and destruction cases are linked from the vehicle report.

The earlier v407 `--simulate` invocation was headless and its provisional
visible verdict was corrected to red, with the original provisional report
preserved. The v408/v409/v410 infrastructure failures remain retained. The
v411 normal windowed run rendered600 frames and1638 physics ticks but exceeded
its180-second finite deadline; that timing verdict remains red. Its actual
split-screen gameplay has been inspected in the retained video. The successful
shorter v412 startup/capture took105.8seconds. Loading/rendering performance is
still a limitation; these observations do not establish retail frame pacing,
Xbox hardware, full visual parity or every physics trajectory.

The current runtime's source chain begins with the full immutable v381 cut. The
v403 full source archive preserves the newer carrier input/diagnostic fixture;
the v405 source delta and immutable acceptance executable retain the lifetime
observer fix and visible-interval ride gates. The v407 overlay adds this
launcher without changing production physics. These manifests/archives are
under `analysis/multiplayer/physics-live-tu15-20261012/`.

Eight surveyed vehicle destruction cases, original vehicle cameras and the
three successful mechanical-paddle rides are bounded live evidence. Train and
lower-body approaches, the Banshee original object-update kernel-wait boundary,
broader controller/contact/lifetime cases and quantitative retail comparisons
remain open. The full player/vehicle/object physics goal remains active.
