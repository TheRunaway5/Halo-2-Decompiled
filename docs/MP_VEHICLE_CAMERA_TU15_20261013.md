# TU1.5 vehicle camera live integration

The driver view now executes actual English TU1.5 `245EA0`, selected by the
original instruction at `15850D`. Its complete `189FD0` camera-block resolver,
`245C70` pitch-to-track search, `2455F0` authored-track interpolation, marker,
velocity and packet repair dependencies run against the retained physics bank.
`158340` owns the camera-mode admission, including its boarding/exit branches.
There is no invented chase offset or vehicle position supplied by the test.

Reference SHA256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
`tools/research_vehicle_camera_tu15.py` retains the TU1.5 instruction bodies.

`1888A0` constructs the original four-view input storage. `18A0C0` binds the
actual unit; native mouse/gamepad yaw and pitch are explicitly delivered to the
fields the camera reads. This does not assign simulated players to original
Xbox local-user records. The producer's 28-byte state is retained per native
view, with the literal `1584D0..158512` initialization. The original observer
adoption, motion, camera collision, location and visibility owners consume the
result. Native seated rendering includes the player's source WORLD body and
omits the first-person hands.

For UDP peers, the authority retains a separate camera controller for each
peer's local views. Protocol59 carries the completed vehicle view. The client
uses its received position, forward, up and FOV; it does not evaluate the host's
physical world independently. Wire admission requires a live seated owner,
finite coordinates/FOV and a valid orthogonal camera basis.

## Visible evidence

The isolated, coherent `source-v3` build was tested in real X11 windows with
ordinary SDL keyboard/gamepad input and unaltered window recording:

| Run | Result |
| --- | --- |
| `live-integration-v95/vehicle` | PASS: 301 ticks, enter/drive/steer/exit; 4.647826 horizontal units and 2.163720 radians steering; 56 submitted seated frames; 59 submitted frames after exit. |
| `live-integration-v95/lan-vehicle` | PASS: three participants; 303 exact authority comparisons for identities, motion, seats, vehicle roots and all WORLD banks; enter/drive/steer/exit; 199 seated camera frames and 77 frames after exit. |

The UDP test compares the submitted driver's camera position/forward/up
bit-for-bit with the authority view in each seated render frame. Both tests
also require the actual camera to move away from the ordinary first-person eye.
Measured maximum separations are 2.961221 and 2.455765 world units, respectively.
`cameras.json`, `authority-comparisons.json`, `report.json` and `window.mp4` retain
the evidence under `analysis/multiplayer/physics-live-tu15-20261012/`.

The same binary subsequently passes local movement/lift and UDP movement/lift
(`live-integration-v96`); both UDP cases compare all181 observed states.

After grenade creation, explosion and retirement integration, the updated
`live-integration-v143` binary again passes local and UDP vehicle tests. The UDP
run retains301 exact authority comparisons,328 seated-camera frames and206
frames after exit. It observes2.114492 horizontal units of driving and0.594932
radians of steering. The local run repeats4.647826 units and2.163720 radians.
Local/UDP movement and lifts also pass, along with UDP grenade release,
explosion, retirement and exact effect-location transport. These are bounded
live checks; the remaining camera/physics limits below still apply.

The current checkout's `build-multiplayer/physics_multiplayer_acceptance` also
passes both vehicle modes in `live-root-v145`:301 ticks each,301 exact UDP
comparisons and382 seated-camera submissions on the client. These runs include
the current production wire layout and all current source/rendering modules.
They are not merely a private snapshot result. The installed launcher still
selects its existing delivery binary; replacement/default physics delivery
remains separate work.

The later current-root `live-root-v151` production/acceptance build passes
local and UDP vehicle sequences with scene replication integrated. The UDP
run compares301 exact authority snapshots and submits313 seated-camera frames
and160 frames after exit. Driving covers2.303617 horizontal units and0.569623
radians of steering; the local run covers4.611221 units and2.267616 radians.
All seven selected local/UDP physics cases pass and retain nonempty, probed
window recordings. These measurements are bounded native runtime evidence.

The current-root `live-root-ghost-v164/lan-vehicle` run passes three-participant
Ghost entry, driving, turning and exit:301 exact authority comparisons,483
seated-camera submissions,340 frames after exit,6.270763 horizontal units and
0.731575 radians of turning. The exercised UDP camera checks now also compare
horizontal FOV bit-for-bit through the actual submission callback. Its recorded
X11 stream has319 frames. The same updated current-root binary passes Warthog
again in `live-root-warthog-v164`.

Banshee `v164` and the ordinary walking-approach `v173` remain RED: neither
boards. V173 records distance, aim, facing and relative-speed prerequisites;
those gates satisfy the authored limits while the native prompt stays absent.
The live seat path now executes full TU1.5 CB3D0/CB090. Actual153FAD selects CB3D0 with four stack arguments and
RET16; CB7A0 separates its player and AI visibility paths using unit+13C.
`vehicle-camera-seat-tu15-source` retains the19 original bodies.

The isolated-input current-root `live-root-warthog-isolated-v181` run passes
the complete Warthog sequence through that original seat selector. The native
X11 surface is mirrored unaltered to the desktop. Banshee `v183` now boards,
drives4.708907 horizontal units, turns0.647489 radians and submits224 actual
seated-camera frames with302 exact UDP authority comparisons. Its maximum
camera/first-person-eye separation is3.316778 units. This run remains RED:
boarding retries consume the fixed input budget and exit has not completed
when recording ends. A longer sequence with ordinary climb look input is
required before a Banshee flight/exit pass can be claimed.

The extended current-root `live-root-banshee-isolated-v185` run passes boarding,
powered travel, turning,0.769799 units of climb and exit. It retains602 exact
UDP comparisons,266 seated-camera submissions,229 submissions after exit and
669 unaltered window frames. The executed ELF hash is
`08449cdec527b4aae2342ea0bd741b03c2293fed73e867cdec752ef24eb945f3`;
the wrapper verifies that the executable remains unchanged during the run.
This bounded low-altitude climb stays close to the garage roof. The stronger
open-flight `v189` input departs the garage and completes exit, but remains RED
at its explicit10-unit travel/2-unit powered-climb targets:8.314886 and1.916033.
The original camera is submitted417 times and all902 UDP snapshots compare
exactly. The targets are retained for a longer normal-input sequence.

Current-root Warthog and Ghost `v189` both pass the complete UDP sequence through
the original seat selector, with302/301 exact comparisons,233/254 seated camera
submissions and121/155 submissions after exit. Position, forward, up and FOV
are checked at actual driver rendering. Both executed ELF hashes remain stable.

The open-flight current-root `live-root-banshee-isolated-v190` run passes all
retained targets: boarding,301 forward input samples,10.374212 units of travel,
3.404600 units of climb,0.653005 radians of turning and exit. It retains902
exact UDP comparisons,435 original seated-camera submissions and356 submissions
after exit. The unaltered960x540 recording has945 frames. Executed ELF SHA-256
`73a14499a06b233e8c2112c13c4dcf681bdc5bd418c9f59be1d01d9c35ab11aa`
remains unchanged through the test. Test inputs align ordinary look with the
completed vehicle forward vector, depart horizontally, then look up and turn;
no body motion, attachment or camera result is supplied by the fixture.

## Remaining scope

This proves a real authored seated camera in the exercised Warthog, Ghost and
bounded Banshee sequences.
It does not establish complete TU1.5 camera/input or physics parity. Native
input still supplies absolute look angles; the surrounding original look
limits, automatic pitch and full mode-switch transition scheduler need their
own integration and retail comparison. Vehicle types/seats beyond these
drivers, turret visuals, general contacts/impulses, grenade/projectile ownership,
and complete physics parity remain open. The `source-v3` binary is an isolated
test snapshot; the default installed launcher has not been replaced with it.
