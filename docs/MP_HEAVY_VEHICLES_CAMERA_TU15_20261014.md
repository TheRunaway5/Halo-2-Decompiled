# Heavy vehicles and seated look input, TU1.5

The reference is the owned English retail TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
This work remains part of the unfinished full physics goal.

## Original construction and native replica admission

Actual `15CC70` selects fixed Heavy Vehicle choices1/2 from the original
multiplayer globals at `mulg -> runtime+74 -> references+14/+1C`. Coagulation
contains authored classification3 spawn rows55/56. Their fixed choices resolve
to Scorpion `EEFE0D89` and Wraith `EF9F0E2A`, with variant `0B00078B`.
The visible fixture selects ordinary variant parameter78, then approaches the
actual authored driver entry marker before tick zero. The retained original
netgame factory creates the body, children and controller. No post-step body
position, velocity, contact, attachment or camera result is supplied.

`heavy-choices-original-v231.json` executes unchanged `15CC70` against owned
Coagulation, Waterworks, Zanzibar and Containment metadata:36 fixed/default/
disabled cases pass, covering35 of58 original instruction sites. Random choice
is outside this fixture and remains explicitly unavailable.

The first Scorpion UDP run (`live-heavy-scorpion-v231`) exposed a native client
constructor rejecting every Heavy Vehicle override without a local source
world. Clients need the authored cold templates to display the authority's
vehicles. The source-world requirement now guards authoritative `step`, before
any world-clock mutation. `multiplayer_tests --heavy-replica` checks that fixed
and disabled heavy settings can construct a native display match and still
cannot advance it without the source owner.

## Recorded failures and camera response

Evidence is under `analysis/multiplayer/physics-live-tu15-20261012/`.
`live-heavy-scorpion-v232` boards/drives/turns/exits, with301 exact UDP states
and370 original vehicle-camera submissions, but fails the travel requirement:
65 observed client forward samples cover only26 distinct authority ticks and
0.716700 units of travel. The extended `v234` sequence delivers300 forward
samples over126 distinct authority ticks and travels9.315096 units.

The original turn check sampled only while lateral input was held. The tank
continues turning toward the delivered mouse aim after that key releases.
`heavy-scorpion-red-diagnosis-v235.json` retains the observed0.504591-radian
turn over the remaining driven interval. The revised live check measures from
the actual forward vector when steering input begins until forward releases.
It retains the same minimum travel and turn requirements.

Inspection of the unaltered tank recordings then exposed a separate camera
bug. `vehicle-camera-look-red-v234.json` fails for both Scorpion and Wraith:
delivered yaw spans0.6 radians, but submitted camera yaw spans zero. The old
Wraith acceptance passed because it checked exact authority/client camera
agreement, separation from the ordinary eye, and restoration after exit;
both peers shared the frozen-look defect. That older pass does not establish
working seated camera look.

`Simulation` skipped controlled actors before updating native look input when
they had a source parent. Remote players occupy those controlled actor slots.
Their vehicle commands reached the original controller, but `NativeVehicleCamera`
received stale yaw/pitch from the native presentation unit. Seated controlled
actors now retain the completed delivered look input while continuing to skip
the native movement/weapon path. The actual `245EA0` producer and observer
motion/world routines continue to own camera construction and output.

The live acceptance now also requires the submitted camera to turn by more
than0.05 radians during the deliberate mouse-turn sequence. Ghost, Banshee,
Scorpion and Wraith receive that input in `v236`. The aligned Warthog `v236`
travels5.276381 units but turns only0.010030 radians during its short strafe-only
sequence, so its turn check remains red. `v237` gives Warthog the longer drive
sequence and the same deliberate mouse turn, and requires camera response for
every selected vehicle.

## Verified camera fix

`vehicle-heavy-build-v236.log` builds the production `halo2_native`, acceptance
runtime, native multiplayer tests and original heavy-choice tests. The saved
acceptance executable `build-multiplayer/physics_multiplayer_acceptance-v236`
is used for the following recorded live UDP cases. Each case independently
pins the executed ELF and probes its unaltered X11 recording.

| Vehicle | Exact UDP states | Travel | Vehicle turn | Submitted camera look turn | Original seated camera frames | Frames after exit |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Scorpion, `live-camera-scorpion-v236` |901 |9.857383 |0.508007 |0.600000 |659 |529 |
| Wraith, `live-camera-wraith-v236` |902 |14.277961 |0.671908 |0.600000 |670 |562 |
| Ghost, `live-camera-ghost-v236` |302 |6.276362 |0.720642 |0.600000 |252 |154 |
| Banshee, `live-camera-banshee-v236` |901 |12.919037 |0.641412 |0.600000 |627 |497 |
| Warthog, `live-camera-warthog-v237` |901 |36.505600 |0.862927 |0.600000 |631 |726 |

These cases pass boarding, forward drive, turning, exit, camera response, exact
submitted authority camera position/basis/FOV, and restoration after exit.
Travel is in native world units and angles are radians. A frame extracted
without modification at10 seconds from each tank video is retained as
`vehicle-camera-frame.png` beside `window.mp4`.
The Banshee additionally climbs4.502580 units and its submitted seated camera
pitch spans0.695000 radians during the delivered departure/climb inputs.
`vehicle-camera-build-v237.log` rebuilds the acceptance fixture with Warthog's
longer input sequence. Its saved ELF is
`build-multiplayer/physics_multiplayer_acceptance-v237`; the four other vehicle
recordings and the regression suite use the separately saved `v236` ELF. The
aggregate `vehicle-camera-suite-v237.json` retains each case's executed hash
and original recording path. Both executables include the seated-look fix.

The same saved `v236` executable also passes `live-camera-regression-v236`
movement/lift/objects with181 exact UDP states in each case. Lift upward
velocity reaches3.964367. The object case renders its actual source-created
debris176 times in each local view and observes1.638272 units of source motion.
Its recreated prop receives two actual point impulses; all512 motion bytes
match unchanged TU1.5 in `original-point-impulse-comparison.json`.

`heavy-replica-test-v236.log` passes the constructor/step ownership regression.
`heavy-choices-native-v236.log` matches all36 unchanged original heavy choices
and checks22 authored child templates. These remain bounded construction and
replication checks; they do not replace the recorded live runs above.

## Limits

Recorded native sequences and exact UDP publications do not establish every
retail trajectory, contact/material/constraint mode, all seats or all vehicles.
The complete retail camera scheduler, automatic pitch and all mode transitions
still need coverage. The current installed launcher selects an MCC-only runtime
and reports native MCC gameplay unavailable; these original-Xbox-map acceptance
runs do not establish delivery through that launcher.


`current-driver-matrix-v314` repeats driving, steering, original camera aiming
and normal exit on the coherent current protocol80 build for Warthog, Ghost,
Banshee, Scorpion, Wraith, primary/secondary Spectres and Gauss Warthog.
All eight cases pass, with6,615 exact observed UDP snapshots, independently
stable executables and continuously verified live mirrors. Each submitted
seated camera compares position/basis/FOV to the actual original authority
camera and responds to0.6 radians of delivered look input. Unaltered videos
and exact per-case results are in `verified-driver-summary.json`. This does
not establish all vehicle firing/boarding/contact modes, original quantitative
retail trajectories or complete physics parity.
