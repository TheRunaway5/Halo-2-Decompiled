# Vehicle seat and camera integration, English TU1.5

The driver-camera coverage is recorded in the vehicle-camera, heavy-vehicle and
session-configuration documents. Full vehicle/seat/boarding parity remains open.

## Authored Warthog seat ownership

The first declared seat2 test, `live-seat-warthog-gunner-v256`, is red: the
original live action prompt selects the driver and the test correctly refuses
to force attachment. The fixture's label incorrectly called root seat2 a
gunner seat. `seat-inputs-v257/fixture-inputs.json` retains original authored
seat flags, modes, markers, boarding targets and declared pre-tick input.
Root seat2 has flags6160 and boarding target0; its entry marker is at the same
location as the actual driver entry. Root seat3 boards passenger1. The real
gunner owns a seat on the authored attached turret vehicle.

The acceptance fixture now permits an explicitly selected authored child and
seat. Its pre-tick approach composes original parent/child model markers;
actual source construction, prompt selection, boarding, camera production,
physics and rendering remain runtime-owned. Metadata-only inventory is clearly
separate from visible gameplay acceptance. No completed body, seat assignment,
physics result or camera is supplied by the fixture.

The first rebuild failed because the shared tmpfs `/tmp` was full. The retry
uses an owned temporary directory on the workspace filesystem without deleting
other work. Both build logs are retained.

Evidence root: `analysis/multiplayer/physics-live-tu15-20261012/`.

## Visible Warthog gunner and passenger

`live-seat-warthog-turret-v258` passes normal entry into authored child seat0,
mouse aim and exit with902 exact UDP snapshots. Actual original camera
submission occurs524 times, separates1.524986 world units from the ordinary
eye and turns0.600000 radians. Both views submit the actual turret geometry;
549 post-exit frames are retained. Root seat1 also passes in
`live-seat-warthog-passenger-v258`:901 exact states,655 seated original camera
submissions,0.600000 radians aim and659 views after exit.

These tests verify stationary seat/input/camera integration. Their forward
and lateral inputs are not claimed to drive the root from a passenger/gunner
seat. Moving occupied vehicles, turret firing/aiming mechanics, all boarding
transitions and quantitative retail trajectories remain open.

## Spectre and Gauss seat cameras

The first `v259` batch was interrupted while its first real process was running;
there is no completed verdict for it. The prior process handles and display
were confirmed absent before a fresh owned display and `v260` batch started.
`v260` passes both Spectre passenger seats, the actual Spectre turret child,
and the Gauss turret child, with301/302/302/301 exact UDP states respectively.
Every case requires normal entry/exit, actual original camera turn of0.6 radians,
and original camera position/basis/FOV at submission. Their original seated
camera frame counts are271/249/264/229. Videos and stable executed ELF hashes
are retained in each case directory.

The Scorpion passenger1 fixture then fails before gameplay because it requires
a generic authored entry clip. That fixture-only filter is removed; actual
source seat admission and animation ownership remain unchanged and decide the
live result. The failed input and execution report are retained.

## Moving passenger and second driver

`live-moving-warthog-passenger-v261` passes with three actual UDP participants
and two rendered local views. The second player uses ordinary SDL gamepad
movement/look to walk around the vehicle along declared approach waypoints,
holds actual pickup to board the source-selected driver seat, and delivers300
forward commands. No source player/body/seat/camera result is injected.
The first player occupies the passenger seat, aims and exits normally.

The actual occupied vehicle moves5.340403 horizontal units. All901 observed
LAN states match actual same-tick authority publications, including complete
WORLD hierarchies. The passenger submits816 original cameras and the driver929;
both views compare camera position, basis and horizontal FOV bit-for-bit with
the received authority packet. This supersedes the earlier stationary-only
limit for this specific Warthog passenger/driver combination. Other moving
seat combinations and retail force/contact/trajectory comparison remain open.

`v262` was interrupted during Scorpion loading; its process handle, runtime,
and owned display were confirmed absent before starting `v263`. Incomplete
recordings do not count as passing seat coverage.

`live-seat-scorpion-passenger1-v263` reaches the actual visible runtime after
the fixture filter removal. It remains red: all302 UDP states match but the
original prompt consistently selects root driver0, so no passenger attachment
is forced. Original seat admission and camera are not bypassed. An occupied
driver sequence is the next declared scenario.

`live-moving-scorpion-passenger1-v264` retains901 matching observed snapshots
but remains red: the gamepad approach reaches waypoint2 and is blocked on the
Scorpion hull before boarding. The next approach uses declared waypoints
outside the original vehicle bound and the actual authority tick as its finite
clock. Repeated rendered snapshots cannot shorten that scenario's time budget.
This changes only delivered test input, not collision/seat/controller results.

## Correction: Scorpion rider seats reject player occupants

The earlier Scorpion positive-entry expectation was incorrect. The complete
original278-instruction CB7A0 body is reauthenticated byte-for-byte against
owned English TU1.5 in `scorpion-player-seat-admission-tu15.json`. At CB862 it
reads actor unit+13C. For a player-owned actor, CB899..CB8A1 tests seat flag
bit16 and clears candidate validity. The separate bit17 branch applies to
non-player actors. Raw Coagulation Scorpion seats1..4 all have flags0x10270,
including bit16. The original code therefore prohibits these player seats.

The native restriction is preserved. The acceptance expectation now requires
no prompt for the invalid seat, no player attachment, no seated vehicle camera,
a rendered ordinary camera and exact actual UDP state. It does not enable the
seat, invent an entry clip or relax source admission. NPC rider/controller
physics and boarding transitions remain open. The free-for-all occupied-driver
run also produced the original enemy-boarding prompt; it was stopped as a
retained diagnostic once that behavior was established. Cooperative moving
passenger/gunner tests use explicit original Team Slayer setting91 and team0
for both participants.

`v268` visibly verifies all four Scorpion player restrictions with
302/302/301/301 exact observed UDP states. Every run retains a continuously
verified live desktop mirror, including the actual capture/player pipe and
mapped window. The earlier NUT mirror failed on a raw-frame packet and was
replaced by a direct unaltered raw-video stream with explicit X11 output.
The visible runner now refuses an unrelated/stopped producer/player or an
unmapped mirror, monitors the connection during loading/gameplay, terminates
on lost proof and retains the failure. An unrelated-process negative check
also rejects correctly. Incomplete earlier mirror runs do not establish a
visible demonstration.

The first cooperative moving-turret `v268` case is red because its second
player's test route crosses the vehicle from the rear. Both source seat and
collision admission remain enforced. The next declared route follows an arc
outside the authored vehicle bound before approaching the actual driver-entry
marker; its target is retained with the observed ordinary controller input.

`live-moving-warthog-turret-v269` boards both actual players, drives3.593170
units and submits original cameras in both views with0.6 radians of gunner aim.
It remains red because the finite fixture quits at authority tick360 while its
forward-input interval only ends at326; the declared45-tick exit wait has not
expired. No exit input was delivered. The next version retains the larger
finite bound and exits early after the actual seat exit and post-exit rendering.
This corrects the test lifecycle without changing physics or seat behavior.

## Connected original occupant and event owners

The complete cooperative Warthog gunner sequence passes in
`moving-seat-suite-v271/moving-warthog-turret`:620 exact observed UDP states,
4.137875 units of occupied travel,899 original gunner camera submissions,
617 original driver cameras,0.6 radians of gunner aim, actual exit and a
continuously verified live mirror. The new seat-suite tool owns a fresh
mirror per case and records/cleans up only those processes.

The Spectre sequence then reaches actual missing ED500 from ED705 during
14FEC0 player update. The original complete95-instruction occupant guard is
lowered and connected to the live runtime; object/iterator/relationship/action
results remain with their existing owners. Both acceptance and production
native targets build. Generation repeats byte-identically. The next live run
advances to B2DC0 at ED777. Its complete40-instruction state-gated event owner
is added without a successful-return stub. Every other direct callee in the
complete ED640 path and B2DC0's B8050 event owner already has a registered
native owner. Static bodies, authenticated call bytes and generation manifests
are retained under `seat-transition-source-v272` and `v273`.

These complete function ports do not by themselves prove every branch or
vehicle/boarding behavior. The actual composed Spectre rerun remains required.

The `v273` Spectre run no longer reaches a missing routine and retains2399
matching observed snapshots, but it remains red: the companion never occupies
the driver seat, so motion, aim and exit are not completed. A read-only authority
binding/command trace is being added to distinguish actual unit/player/team
bindings from native reticle timing and the declared approach geometry.
The original caller is ED640; the earlier ED6C0 listing began inside an
instruction and is diagnostic only, not a ported body. Both emitted new
routines start at their authenticated direct-call targets.

The `v274` read-only authority trace confirms full unit/player bindings and
NONE actor identities; raw source parents/seats match native readback. It
records the companion's actual held pickup while the authority prompt is
boarding seat4, then attachment to that seat. The completed-unit team fields
are retained rather than inferred to be a bug. No team/body fields are patched.
This next actual boarding update reaches D3470 at ED964, the original occupant
ejection request. Its complete62-instruction body is added to the same owner;
CB440, E8E50 and CECF0 callees already have registered owners. The module now
contains197 original instructions over three complete routines. Cooperative
driver acceptance remains red independently of these necessary boarding ports.

## Source player-team birth and delivered vehicle intent

The next composed run reaches original154C70 at EDB0F after the new D3470
boarding owner. Complete154C70 and the1AA2B0 lookup reached during team
configuration are now connected; the module totals263 original instructions.
The player-team input investigation and native LAN vehicle-intent integration
are recorded in [player team birth](MP_PLAYER_TEAM_BIRTH_TU15_20261014.md).
`seat-birth-live-v277` and `team-roster-live-v278` remain failed runs with
continuously verified mirrors; neither establishes cooperative acceptance.
The corrected roster delivery occurs before source player allocation.

`roster-notification-live-v282/moving-spectre-passenger1` now passes:813 exact
observed UDP states, actual source team0 on all three participants, normal
driver0 entry,300 ordinary forward commands,28.907700 units of occupied
travel,698 original passenger cameras,576 original driver cameras,0.6 radians
of aim and normal passenger exit. Both source vehicle-camera packets reach
rendering with exact position/basis/FOV comparisons. The runtime is stable and
the desktop mirror is continuously verified. No native team/body/seat result
is supplied. The module now contains327 instructions over seven complete
original bodies, including the two color lookups and team notification reached
by the actual roster/configuration refresh. Full vehicle/physics parity remains
open; the remaining moving-seat combinations are being exercised separately.

## Completed moving-seat matrix

All remaining moving combinations pass in `moving-seat-matrix-v283`, each
with original source team0, normal seat entry, actual companion driving,
0.6-radian camera response, actor exit, a stable ELF and continuously verified
mirror. All observed UDP states match source authority output.

| Seat | Exact observed states | Occupied travel | Actor original camera frames | Driver original camera frames |
| --- | ---: | ---: | ---: | ---: |
| Warthog passenger |700|5.713860|902|585|
| Warthog turret |609|3.837742|813|547|
| Spectre passenger2 |636|9.460221|962|639|
| Spectre turret |697|6.039809|889|558|
| Gauss turret |623|3.594007|712|501|

Spectre passenger1 is the separate passing `v282` case above. This matrix
verifies these composed native seat/camera/input paths, not quantitative retail
trajectories, firing/aim constraints, every boarding branch or full physics.
The protocol78 release-target fix is recorded in the player-team birth document;
its fresh visible regression remains separate from this protocol77 evidence.


Fresh composed checks after the full original player creator/contact extension
and release-intent changes are retained under `current-seats-v296/`.
Moving Spectre passenger1 passes652 exact UDP samples,759 actor camera
submissions,509 driver camera submissions and14.405009 units of travel.
Moving Warthog turret passes618 exact UDP samples,658 actor and463 driver
camera submissions and3.897112 units of travel. Both turn the original vehicle
camera by0.6 radians, enter and exit normally, preserve actual source team0,
and verify the live desktop mirror and stable runtime throughout. This is
protocol78 isolated-runtime evidence; the current protocol80 workspace needs
its own coherent build and runtime check.


`current-moving-seats-v312` repeats all six moving passenger/turret cases on
the coherent current protocol80 build. All pass ordinary entry, companion
driving, original camera aim by0.6 radians, geometry in both views and normal
exit. Every case retains its continuously verified mirror, stable ELF and
unaltered window video. Full seat/boarding/firing/retail-trajectory coverage
remains open; see the exact per-case `verified-seats-summary.json`.

The six cases retain4,053 exact observed UDP state comparisons.

## Further occupied Spectre contact branch

The device-readback regression `device-world-regressions-v320` reached a new
real contact dispatch: `2F62D9 ->2F70D0`, return `2F62E0`. That failing recording
is retained. The complete25-instruction wrapper now belongs to the registered
collision-query module. Original call bytes, collector interface at41CA40,
existing real2F6FC0 query and30B960 collector are preserved. The module now
owns20 complete routines/1,607 instruction sites; root and isolated generated
bodies match exactly.

The fresh `spectre-contact-live-v322` run passes normal passenger and companion
driver entry,20.516548 units of occupied travel, aim/exit,723 exact UDP states,
573 passenger and379 driver original-camera submissions. The passenger aim
response is0.5999999 radians. The source ELF stays stable and the desktop
mirror is verified continuously. This closes the missing owner actually
reached by this live case; it does not establish every possible contact pair.


After the source device script integration, the same coherent v338 acceptance
passes `device-script-vehicle-camera-v338/moving-spectre-passenger1`: 660 exact
UDP comparisons, normal selected-seat entry/exit, 25.528517 units of occupied
vehicle travel, 703 submitted original passenger camera frames and 476 original
companion-driver camera frames. Submitted passenger look turns 0.599999905
radians. Both views submit the actual source vehicle geometry. The executed
ELF is stable and the desktop mirror is continuously verified.

[Current original vehicle-camera recording](../analysis/multiplayer/physics-live-tu15-20261012/device-script-vehicle-camera-v338/moving-spectre-passenger1/lan-vehicle/window.mp4).
This regresses actual original authority camera position/basis/FOV submission
after the new VM service hook; it does not close all collision, firing, boarding
or destruction behavior.
