# TU1.5 runtime prop birth and conveyor integration

The live Colossus authored spawner test exposed a real source integration defect:
all scenario births used the automatic placement caller's `C1120(...,1,1)` mode.
For type11 this forces creation request bit8 and object+C1 bit0, freezing
script-created gas containers at their authored spawns. Original runtime caller
B15B0 instead uses `C1120(...,0,0)`. D7AA0/D7B80 use1,1 for automatic placement;
D75A0 and B9B70 carry the actual original flags into the body.

`SourceScenarioBirth` now preserves this caller distinction through SceneObjects,
Match, MultiplayerObjectRuntime and PhysicsLiveWorldTu15. Initial/BSP placement
uses automatic mode; explicit/script recreation uses runtime mode. No host
unfreeze, velocity, force, contact or solved pose is supplied. The original
creation owners perform every body/physics operation.

Evidence is retained under `analysis/multiplayer/physics-live-tu15-20261012/`:

- `original-source-birth-context-v357.json`: raw TU1.5 original caller/flag contracts.
- `script-prop-red-v355.log`: real authored-script regression before the repair.
- `script-prop-runtime-v357.log` and `script-prop-runtime-v358.log`: subsequent
  actual dynamic paths exposed missing original surface/query owners.
- `conveyor-source-v360/{root,frozen}`: complete authenticated355D0 and1E1170,
  146 instruction sites. Original13DE70 function evaluation,315C90/315CB0
  activation helpers and the original virtual query call remain responsible.
- `script-prop-runtime-v360.log`: PASS, three moving script-created generations,
  650 real host/client UDP device-bank comparisons with no script faults.
- `prop-actual-entry-capture-v360`: actual live entry/return ABI and all committed
  source bytes, exported read-only while running the real world.
- `conveyor-actual-original-hardware-v360/report.json`: PASS396 original instructions.
- `prop-query-actual-original-hardware-v360/report.json`: PASS102 original instructions.

Each original replay executes unchanged TU1.5 text and real callees, compares
60,084,310 accessible final source bytes and all captured GP/SSE/x87/FP state.
The host user-mode interrupt flag already differs at entry and is explicitly
excluded (mask0xFFFFFDFF); all raw flags are retained. This is host i386 evidence,
not Xbox hardware proof, and one real call per entry is bounded coverage.

Visible v361 validation now passes on one stable acceptance ELF (SHA256
`99cf97bf0501c02eef1cd22c5542ffd4d289bbcd92bc265d3060cce98be96ad8`). Eight cases
retain continuously verified live desktop mirrors and5,220 exact UDP states:
Colossus scripted props; movement; object recreation/blast/debris; actual
Spectre driver/passenger cameras; wheel support; eight device-script stages;
and both explicitly created Backwash HVAC placements. The conveyor fixture
renders three moving prop generations, two with observed falling and horizontal
travel, with1,441 aimed camera observations. Its actual surface shader is
`conveyor_belt_pro`; the captured original function rates are U=-0.5,V=0.
This observation does not itself measure complete retail transport trajectories.

The Colossus field trace observes83 actual velocity-changing original calls in
one of its four authored fields. Its captured member is a type11 gas-container
CBASE garbage fragment. Untouched original i386 execution matches that live
force call across1,719 instructions and60,084,310 source bytes. The other three
fields have no observed members in this run and remain open. See
`colossus-nonplayer-fields-observed-v361.json` and
`colossus-can-force-original-hardware-v361/report.json`.

`runtime-birth-verified-live-v361.json` aggregates the eight live reports/videos.
The refreshed selected device/body/geometry/departure/continuation checks pass;
the first broad selection also listed three not-built test executables and is
retained as such. The two original world/qword checks subsequently build and
pass in `prop-world-oracle-regressions-v361.log`. The legacy mixed-version
contact fixture is outside this TU1.5 validation selection.

Elongation's unchanged authored giant-crate spawners also pass650 real UDP
steps in `elongation-prop-runtime-v362.log`. Birth freeze assertions apply at
first observation; subsequent original sleep/freeze transitions remain owned
by the original world. Complete world/controller/object-update original-call
comparisons and visible Elongation validation are being run. Full player, vehicle, object and all non-player field parity
remain open; this checkpoint does not close the overall goal.


## Full original live-call comparison and x87 repair

Read-only entry/return capture now covers complete14FEC0 controller delivery,
B8F40 object update and1CC8D0 physics world calls. The first two match original
CPU execution in v363. The world matches every captured memory/GP/SSE/FP value
but exposes a C1 difference (0x100 native,0x300 original); single-stepping34,062
original instructions reproduces it. No flag mask is expanded to hide it.

The retained37C5D0 quaternion-to-matrix body has17 FSTP m32real stores in each
of two retained modules. Their float values were correct but C1 was stale.
Eight independent declared quaternion inputs execute unchanged original i386
instructions. They reproduce the defect separately in the map owner (case2)
and TU1.5 core-world composition (case10). Both copies now preserve the actual
hardware rounding direction. The generator retains the repair for this body.
All16 map/world comparisons pass, including complete memory and CPU/FP state.

`world-contact-checkpoints-original-v365/summary.json` passes all12 complete
controller/object/world calls at original clock ticks200,250,400,600. Each
compares60,084,310 accessible source bytes, actual GP/SSE/x87 and FP controls/
status. Hardware execute breakpoints observe authenticated original RET sites
and single-step the outer RET; no original text or callee/result is patched.
This mode reports no instruction count. The documented host IF exception
remains the only excluded EFLAGS bit. This is bounded host CPU evidence,
not Xbox hardware or complete all-map gameplay/trajectory parity.

Elongation's visible v364 run preserves652 exact UDP states, stable ELF and a
continuously verified mirror, but remains RED: crates fall and stack instead
of traveling along the belt. Both falling generations reach actual renderer
submissions. The U surface function returns0.8 and the original cached BSP has
34 conveyor-marked surfaces and7 vector records. The velocity/contact/solver
integration is still under investigation; the travel assertion is retained.
The earlier v363 visible run lost its mirror during loading and remains invalid
as visible proof. Full stated physics parity remains open.


## Conveyor solver prototype integration

The unchanged original Elongation world call at tick600 matches native state
across98,293 single-stepped instructions, including60,084,310 source bytes.
It calls the default325EF0 contact builder four times and never calls the
moving-surface31F770 wrapper. Read-only inspection finds real conveyor contact
flags and body property2003 already holding the original +/-1.2 velocity.
The defect is therefore upstream of that solver callback, not a missing
surface shader or a reason to inject body motion.

Original CRT rows46D7A0/A4/A8 invoke383420/383440/383460 and construct the
prototypes at48CF90/48D0A0/48D1B0. Those objects were all zero in the live
runtime. Whole original31F4E0 compares a contact vtable to48D1B0 before installing
48D0A0's moving-surface vtable. Missing CRT initialization makes that check fail.
`conveyor-prototype-contract-v366.json` retains raw original rows, caller
instructions and the complete upgrade gate.

The original constructors, destructors, native exit registrations and
continuation admissibility are now wired through the real world lifetime.
No host-assigned vtable, velocity, impulse or solved transform is supplied.
The original Elongation spawner regression now requires two generations with
horizontal transport and passes650 actual UDP steps. Two first crates travel
from +/-18 to approximately +/-9.7; later generations also move along the belt.
The first corrected visible v367 run records8.34 units of transport and652
exact UDP states but remains RED because its viewing period ends before the
second newly observed generation completes fall and travel. The viewing period
is extended from650 to900 ticks; the existing two-generation assertions remain.

Colossus exposes the newly reached31F740 size-reporting callback after the
prototype repair. Complete31F740, its mass-scaling sibling31F840 and their
325E90 base callback join the authenticated module. Current original closure
is289 sites; fresh visible and original-call checks are in progress. Full
stated player/vehicle/object parity remains open.


## Current passing live conveyor checkpoint (v368)

Both corrected conveyors now pass visibly through unchanged authored map
scripts and source physics: Elongation and Colossus each retain902 exact UDP
states with stable ELF and continuously verified desktop mirrors. Elongation
renders three moving generations, including two full fall/transport sequences;
maximum observed horizontal travel is11.291181. Colossus renders five moving
and transported generations. All four Colossus fields now observe actual
source velocity changes (444/252/635/280 calls;1,611 total), including intact
full cans and ordinary gas containers. The two Elongation cushion fields also
retain actual crate members in their source force phases. This is composed
world evidence, not isolated retail trajectories.

The same acceptance ELF passes occupied Spectre entry, travel, aiming and exit,
with729 real passenger-camera frames,508 real driver-camera frames and631
exact UDP states. Earlier incomplete camera attempts whose isolated display
stopped are retained as incomplete, not accepted proof. The completed retry
verifies its live mirror and executable throughout.

`conveyor-verified-live-v368.json` aggregates these three same-ELF videos/reports
(2,435 exact UDP states). `conveyor-final-regressions-v368.log` passes all eight
selected device/wheel/prop/departure/continuation/quaternion checks. The complete
original corrected Colossus controller/object/world comparisons pass12 captured
calls; corrected Elongation passes six. All compare complete accessible source
memory and recorded CPU/FP state within the documented host IF limit.

Midship's platform field, both Gemini columns, both Turf door springs, broader
linear carriers, vehicle collision/damage/destruction and full original
quantitative trajectory/visual comparisons remain required by the active goal.
