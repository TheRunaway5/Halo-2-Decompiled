# TU1.5 tested physics runtime delivery

## October16 current delivery

`Halo2-Physics-TU15` now pins `tested-runtime-v566/halo2_native`, SHA256
`a36dc424f5a464508dd0088e393e19f54ced99b033ba22d7ef99f70dc842dbf0`.
`normal-runtime-delivery-v570/execution.json` passes ordinary authored-spawn
startup with two players,120 rendered frames and322 source ticks,without
physics fixture/spawn overrides. `vehicle-camera-unarmed-regression-v569`
passes actual Warthog entry,drive,steering,look and exit with902 exact UDP states,
480 original seated-camera frames,0.6000000238-radian look turn and447 frames
after exit. The seated8s/exited25s frames were inspected; actual game window and
continuous desktop mirror are verified. Root/frozen launcher receipts are
`launcher-v566-verified-*.json`. Source-owned NONE equipment and unarmed body/
unit-HUD presentation,original cinematic RGBA blending and late water binding
are included. No-flare Thronev565 visibly passes382 frames in each view and242
exact UDP states without a replacement weapon. See
[shared campaign integration](MP_CAMPAIGN_SHARED_PHYSICS_TU15_20261015.md).
Full all-player,vehicle and object physics parity remains incomplete. Pelican
SCEN RNG-choice wiring,remaining placements/lifts and all wider gates stay open;
older checkpoints retain their original bounded scope.

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
