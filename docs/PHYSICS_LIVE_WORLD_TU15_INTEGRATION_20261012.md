# Live TU1.5 physics integration, updated 2026-10-13

English TU1.5 is the sole executable acceptance target, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Full player, vehicle and object parity remains incomplete. Canonical IDs in
paired modules select TU1.5 implementations; no 1.1 execution is used here.

Current visible checks under `live-integration-v55/v56` pass split-screen
movement and lift travel, plus real loopback UDP movement and lift travel.
Each UDP check retains181 exact authority comparisons, now including every
published WORLD hierarchy, vehicle root and parent/seat identity. V56 movement
delivers one jump, crouch1.0 and maximum upward velocity3.08. V56 lift reaches
z=-6.85961 with upward velocity3.96437. All recordings are unaltered native
X11 window captures. These are bounded live checks, not complete retail parity.

Vehicle WORLD hierarchies now feed both skinning and the authoritative pose
bank, instead of drawing a bind pose at the solved root. Protocol56 retains
the actual signed parent seat as well. The source command adapter writes the
two-byte pickup word without corrupting the adjacent initialized selection.
The visible harness pumps pending backend focus events before inserting keys;
without that ordering, delayed lost-focus events could erase a queued hold or
jump. Its traces retain actual pickup milliseconds, source threshold/buttons
and window focus. Original seat admission now reaches the enter-seat action;
attachment, driving, steering and exit acceptance remain open.

The development diagnostic also passes180 complete source-world ticks with
all16 actual source players,19 Lockout scene bodies and14 pickups. The native
identity ordinal is bounded by the original player-pool capacity rather than
the four local-view limit. This is one source world with16 delivered command
records, not a claim of16 separate network endpoints. All six original
Coagulation vehicles also complete180 source-world ticks with player movement.

`PhysicsLiveWorldTu15` owns one fresh source bank, native process/arena backing,
actual constructors, source world, header/body/component and player pools,
cache resources, BSP and command delivery. Actual B9B70 biped birth,14EBE0
player construction and14F620 binding now return over that same bank.

The complete original14FEC0 player scheduler now consumes a native command
frame, including actions and153070/C92C0, before13B6F0/B8F40/1CC8D0/B9090/
B9180 and the original clock increment. This replaces both direct153070
delivery and the earlier bounded1500C0..1501A8 cluster prefix. Missing owners
are recovered from their original bytes and retained ABI, never replaced with
successful callback returns. The diagnostic with19 Lockout scene bodies,
one source player and its SMG passes120 neutral plus60 forward ticks:
position changes from(7.03357,-0.134619,-8.27901) to
(9.8174,-0.134629,-8.08929). This does not cover all map pickups.

The two-player lift diagnostic also completes180 source-world ticks, with
both actual players at the declared authored lower hull center. The first
body starts at z=-9.96648, reaches z=-6.92542 and has maximum upward velocity
3.96957 before leaving the field and landing. Both players receive neutral
commands for120 ticks, then one receives60 forward commands. This establishes
bounded original force/controller execution; complete retail lift trajectories
and the visible multiplayer lift acceptance remain required.

Authored scene birth now uses source C1120/D75A0, the original scenario type
lookup, raw placement and palette. Source BA3E0/BA490 own deactivation and
retirement; scene lifetime observation occurs afterward. Scene rendering reads
the completed source root, including parent WORLD nodes and handedness.
Current scene root/salt/read-only/interpolation checks pass. The independent
TU1.5 BFC70 comparison passes256/256 exact13-word outputs under Unicorn.
These explicit presentation inputs are not solved-physics evidence.

Complete11EF70 now prepares every resident collision, biped, creature,
physical-model and vehicle definition before birth. Its original11F02C call
passes zero to1F2C70, resolving serialized shape references. Actual startup
row3/148D40 now constructs both random streams, including original CRT
time/rand conversion over explicit native UTC/current-thread inputs. The
literal13A8F2..13A8FB options-seed delivery runs before scene construction.
With these producers, the complete source factory now constructs19 Lockout
scene bodies in one fresh world. This establishes births, not lift forces.

The real151DA0 post-birth sequence B1A70/B1C30/1613B0 now runs after player
binding. Complete1613B0/1A8B50 creates and carries an actual source SMG;
the fresh-world diagnostic reaches this successful return with one weapon.
Native inventory observes its full source identity, selectors and sequences.
Admitted world-weapon births now select complete15D4F0 over the same bank,
instead of reserving a bodyless header. The general vehicle factory is exposed
through that same original owner. All six authored Coagulation vehicle births
now return (two Warthogs, two Ghosts and two Banshees). World updates reach
the original vehicle math, including37CE0C. Driving remains unverified.

The TU1.5 observer callbacks now select actual11E600 location,3F4F0 visibility,
1746BE..174749 fog classification and16C620 complete segment collision over
the same bank. Native death/kill-volume consumers now follow TU1.5 physics
pose readback. Observer live execution is still pending successful world startup.

PCMPGTW and PADDW lowering passed400,392 independent native x86 instruction
comparisons in
`analysis/multiplayer/sandbox-source/physics-live-world-tu15-20261012/mmx-word-hardware`.
The original CRT32CC0C also required real FISTP64/FILD64 lowering. This does
not settle the separately retained whole-world x87/MXCSR flag difference or
Xbox reciprocal-square-root estimate-bit equivalence.

Visible attempts are retained under
`analysis/multiplayer/physics-live-tu15-20261012/live-integration-v2` through
`live-integration-v10`. All use the real multiplayer acceptance executable and
visible desktop. They currently fail before creating the SDL window; none is a
new visible gameplay pass. V6 reached a missing object-looping-sound pool in
18BD80->16E6A0. Source18B5C0 construction and the exact18B660..18B71C storage
prefix have since been wired. The separate2279E0 audio-volume reset and18BB50
profile consumers are not claimed as completed audio composition.
V10 gets through scene construction and stops at the then-unavailable CF4D0
equipment owner. The fresh diagnostic has since completed that equipment path
and now reaches active PVS/physical-world registration. No new visible
gameplay frame was produced by these first ten attempts.

V11 now completes preparation and renders one actual two-player split-screen
frame. Its unaltered window recording retains30 frames over1.1 seconds at
2560x1378; no simulation tick completes in that build. The retained runtime
stops at the then-unavailable320260 world-registration callback. Recovering
that path subsequently exposes serialized instanced-BSP collision vtables on
the first1CC8D0 tick. Complete171120 is now called with the real BSP datum
at original11EA20's boundary, after1255C0 and before BSP binding. The fresh
diagnostic now reaches the real2E9640 collision-query callback through its
resolved vtable. Player motion, lift force and whole-frame acceptance remain RED.

The fresh Lockout diagnostic now completes its first full command/object/world
physics tick with19 original scene bodies, a source-bound player and its actual
SMG. The second tick reaches original resource/texture preparation. Actual
44B308/44B310 now construct/reset the scenario-instance bitmap used by the
controller's instance update. Native FUCOMIP lowering retains extended
operands and unordered comparisons. Texture storage12EBE0 and activation
12ECF0 now run at their original startup/resource boundaries. Their literal
0x404 protection request maps to real host read/write protection under an
explicit host cache policy; Xbox GPU cache coherency is not established.

Visible v12 runs on the real desktop at960x540 retain11 movement-window
frames and18 lift-window frames. Neither completes a simulation tick in
that sealed build. Movement reaches the character-contact owner1FA510;
the lift entrance reaches an additional body-registration callback. Both
paths are now included alongside Coagulation in fresh same-bank recovery.
The development harness's cache archive and headers are sealed together;
a stale layout mismatch exposed by loading the lift metadata was corrected.
Those old build results are retained; they are superseded only by the specific
later checks described here, not by a general parity claim.

Original125354..1253AB physical map reservation now precedes tag delivery;
without it, texture streaming could overwrite the live tag table. The actual
CRT3835F0/3233E0 constructor now initializes the shared Havok deactivator at
48D3D8, with its registered original cleanup callback. Effect-instance and
location pools now use original17E640/17E700 constructors.

Native map vehicle/model-child presentation adopts actual source netgame
parents and constructor-created children. Attached player roots compose the
real parent WORLD node, and parent/seat are read back after source updates.
Normal pickup prompts deliver their typed target and raw button record into
14FEC0; the source action pipeline owns the transfer. Completed source pickup
roots and attachment membership feed presentation. Scene retirement observation
removes only an actually retired full generation; its focused checks also
reject live and duplicate retirement and verify that no source bytes change.
Native combat clocks, full source input-bit delivery, general15D890 netgame-row
lifecycle, source-driven death/respawn, projectiles and dynamic impulses remain
incomplete integration work.

Visible v14b completed12 source physics ticks before a missing locomotion
dependency. V16 reaches split-screen rendering but stops during real map
pickup birth; v17 completes pickup birth and reaches an original B9180 object
update dependency. Their real960x540 X11 window recordings and failed traces
are retained under live-integration-v14b/v16/v17. V18 stops during another
source pickup birth. V19 passes visible lift acceptance:181 source ticks,
849 submitted split-screen frames and199 recorded X11 frames over6.64 seconds
at960x540, peak z=-6.92542 and upward velocity3.969568. Its image preview was
recovered after an optional PIL import failed; pixel equality was verified
through ffmpeg, and the actual native report and recording remain unchanged.
Movement initially reaches66 ticks before a missing B8F40 collision dependency.
V24 passes both visible movement and lift checks. Movement completes181 ticks,
15 forward-command ticks, one jump, full crouch1.0 and upward velocity3.08;
its recording retains199 frames over6.64 seconds at960x540. V34 and V35 pass
visible UDP movement, with181 exact same-tick host/client comparisons of full
player/unit identity, position, velocity and crouch. This is real loopback UDP
authority evidence, not WAN or complete physics parity. Temporary transport
diagnostics are removed from current source; the host worker now retains its
actual exception in failed acceptance logs instead of hiding it behind a join
timeout. Vehicle entry/driving/contact and dynamic impulse acceptance remain
open. All passing recordings are unaltered X11 captures.

The six-vehicle same-bank diagnostic now completes its first world tick; later
ticks recover additional collision, motion, sound and object-update owners.
An unavailable sound-manager pointer at223C66 was traced to original startup
storage. The live composition now supplies12008..12025's actual0x23000 static
arena through native reserve/commit resources and translates the literal
127EF1..12804B storage prefix plus full source cache/pool constructors. It
preserves128039's EAX voice-capacity configuration. The following224EC0 Xbox
DirectSound device activation is a separate audio owner: its unactivated flags
remain original constructor zero bytes, and native audio presentation remains
separate. This does not establish complete original audio initialization.

Native lowering now executes original FYL2X/F2XM1/FSCALE operations under the
retained x87 control word. Full FNSAVE/FRSTOR remain explicit runtime errors:
the ABI does not yet own original FIP/FDP and dead physical register bytes.
Generation manifests identify those instructions and exclude their containing
entry from complete-native-entry claims. A host CPU environment is never
substituted for unavailable original CPU state. These additions do not resolve
the earlier x87/MXCSR differential or Xbox estimate-bit limits.

Required remaining acceptance includes actual moving players, jumping/crouching,
lift membership/force and upward travel, dynamic object/projectile impulses,
source-created/driven vehicles, source retirement/death, and visible multiplayer
plus real UDP authority snapshots. Vehicle inventory alone is insufficient.

## October13 live integration progress

The Warthog now completes real SDL entry, forward movement, steering and exit.
V92/v93 establish local and three-participant UDP sequences; v95 additionally
executes the original TU1.5 vehicle camera and submits that camera to rendering.
The UDP v95 sequence compares303 source states, including parent/seat, complete
vehicle roots and all WORLD matrices. Its seated rendered camera is also checked
against the authority's completed camera. See
`MP_VEHICLE_CAMERA_TU15_20261013.md` for the source owners, recordings and limits.
V96 passes movement/lift and UDP movement/lift against the camera build, with181
authority comparisons in each UDP case. These are bounded live acceptances;
retail solver/contact and complete1.5 parity remain open.

Grenade integration now delivers original command flag04000000 and the retained
signed selector at+22. The source44-byte action component and actual type5 grenade
bodies supply presentation. Native grenade creation, flight, timer advancement
and cancellation are disabled when the TU1.5 world owns physics, avoiding a
parallel bodyless header or a second simulated projectile. Projectile flags,
state, age, timer/arming fields and traveled distance are read from the actual
FD200 layout; rendering adopts the source-composed frame and source velocity.
The creator player/unit/designator are read from the original body+C4/+C8/+C2
fields, independently of attachment; only the native participant index uses an
explicit association with that full source player identity.

This grenade work is **not complete**. Live v97..v104 runs have recovered actual
selection, weapon admission, count and release-event timing owners. The release
event returns the authored eight-tick duration. Further original begin/creation/
release/effect dependencies are being recovered from actual failures. The visible
objects acceptance still fails; no grenade flight or explosion success is claimed.
The harness supports a separate `lan-objects` case and compares complete grenade
body publications when that path becomes available. Temporary instruction tracing
used to distinguish input-delivery failure from incomplete begin execution has
been removed from current source.

## October13 current-root visible checkpoint

`live-root-v151` supersedes the earlier v97..v104 grenade failure status above.
The current checkout builds both production and acceptance targets and passes
all seven selected local/UDP cases, each with a nonempty probed X11 recording.
Actual source grenade release, body motion, detonation, completed effect
locations and salted retirement pass locally and over UDP. Remote authored
scene roots and WORLD nodes are now applied to presentation and checked before
interpolation; UDP vehicle/lift/movement/objects retain301/181/181/181 exact
same-tick authority comparisons. The Warthog uses the actual authored TU1.5
camera locally and on the remote client. Full controller/contact/retail parity,
close-range prop blast response, other vehicle classes and installed runtime
delivery remain open. See `MP_SOURCE_SCENE_REPLICATION_TU15_20261013.md` and
`MP_VEHICLE_CAMERA_TU15_20261013.md` for the bounded proof.
