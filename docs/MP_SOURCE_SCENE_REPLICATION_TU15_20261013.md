# Source scene physics in multiplayer

## October14 generation integration

Protocol72 adds the authority's actual scene-constructor tick. The client now
stages a complete presentation bank using received full object identities,
authored placements/definitions, roots and WORLD nodes. Its initial local
scene generations are retired once; received generations allocate no local
object header or physical body. Vehicles retain their separate authoritative
vehicle presentation bank. Replacement/removal clears the old generation,
name binding and interpolation history. Invalid identities, duplicate pool
slots, changed creation ticks and malformed hierarchies reject the bank before
scene publication.

`native_source_scene_replica` reproduces the previous generation rejection and
now passes unrelated initial allocations, an equal local handle bound to a
different placement, replacement generations, empty snapshots, omission,
unchanged raw root/WORLD words and rejection without partial scene replacement.
The existing `native_source_scene_physics` check also passes.

Current-root `live-root-scene-recreation-v221/lan-objects` passes an actual
source-world lifecycle: the authored Lockout power module is destroyed/recreated
through the retained constructor/destructor before client admission, a normal
SDL grenade then moves and retires that new generation, and another authored
birth follows the blast. The client adopts both source generations and submits
their actual prop geometry in both split-screen views:128 and61 submissions
per view. All181 observed UDP snapshots match the actual authority, including
creation ticks, roots, query inputs and WORLD banks. Two actual point impulses
again match all512 motion bytes from unchanged TU1.5 instructions. The stable
runtime receipt and213-frame X11 video are retained with that case.

This first recording's close-range participants die in the blast; its camera
does not establish a clear framebuffer view of the final recreated prop.
The later surviving-observer fixture `live-root-scene-observer-v226` passes:
both source generations reach both views (114/58 geometry submissions each),
the living observer's actual camera faces the recreated source sphere for58
frames, and all182 UDP states match the authority. The209-frame recording and
the unaltered extracted `recreated-prop-frame.png` show that surviving view.
Its two actual impulses again compare all512 motion bytes exactly against
unchanged TU1.5. The runtime remains stable throughout the recording, SHA256
`323c16f52e406c35c388de19e51f985b5b265bee726dad2790e8979ac8927d16`.

The surviving-observer fixture exposed a native device-routing defect: an
explicit keyboard assignment to view1 still sent mouse look and action edges
to view0. The runtime now routes keyboard movement/look, action consumption,
scope, scoreboard and input labels to the declared keyboard view. Gamepad
scope handling remains active for controller-only bindings. The live fixture
checks that the other view receives no unintended fire/grenade/melee/pickup
actions. Its earlier `v224` failure retains the wrong-view grenade evidence.

The current startup graph additionally reaches actual44B7AC ->219080. Its
complete18-instruction collision-instance mask allocator is wired through the
existing166A80 owner, with original table/call bytes authenticated. The next
real blast (`v225`) reaches inverted contact query2F7480 during corpse physics;
that routine and its complete2F6CA0/2E44D0 query/transform bodies add125 original
instructions. Their actual virtual query remains required. `v226` passes the
live blast, corpse contact, authored recreation and surviving view after this
integration. This is bounded composed runtime evidence, not a claim covering
every contact-query shape combination or complete retail solver parity.

The final `v226` camera/lift regressions also pass: Banshee902 exact states,
562 original seated-camera submissions,446 frames after exit,13.382885 units
of horizontal flight and5.403625 units of climb; lift181 exact states with
3.964367 units/second maximum upward velocity. The production native target
builds with the same source changes. The separately selected current launcher
package remains outside this validation.

`live-root-scene-regression-v221` also passes UDP Warthog entry, driving,
steering and exit with301 exact comparisons,292 original seated-camera
submissions and135 camera submissions after exit. The UDP lift retains181
exact comparisons and3.964367 units/second maximum upward velocity.

These are bounded authored-placement lifecycle checks. Dynamically spawned
debris, arbitrary late-join renderer resources, every model-region transition,
all machinery/vehicle classes and quantitative retail physics coverage remain
open. The older initial-generation rejection and red prop statements below are
historical and superseded for the tested power-module fixture.

## Earlier integration checkpoint

Completed TU1.5 scene roots and WORLD hierarchies now travel with the native
multiplayer snapshot. Each root retains its full source object datum, authored
placement/definition, visibility and exact transform. Objects with no model
retain an actual empty hierarchy. Modeled roots use the existing bounded WORLD
codec and its original signed descriptor extent; all scene owners participate
in pose admission alongside players, corpses and vehicles.

The client validates each placement and generation before applying the received
root and nodes to its existing scene presentation. It captures accepted ticks
for render interpolation and synchronizes prop/scenery collision geometry.
Complete snapshot omissions retire corresponding native scene presentation.
The bridge does not simulate a second physical world, reconstruct rotations
from Euler angles, or supply an impulse/solver result. Initial generation
mismatch and unsupported recreation remain explicit errors; arbitrary late
join/recreation behavior still needs dedicated coverage.

A read-only rendering observer checks every adopted root, visibility value and
WORLD matrix against the received source snapshot before model interpolation.
The multiplayer harness also compares those same roots and all node banks
against the authority's actual same-tick state.

## Evidence

| Run | Result |
| --- | --- |
| `live-integration-v147/lan-objects` | PASS: 19 Lockout roots and181 exact authority comparisons. |
| `live-integration-v149/lan-lift` | PASS: 181 comparisons and518 adopted-scene render frames. |
| `live-integration-v149/lan-movement` | PASS with scene adoption; source-authority comparisons retained. |
| `live-integration-v150/lan-objects` | PASS: 181 comparisons and301 adopted-scene render frames. |
| `live-integration-v150/lan-vehicle` | PASS: 302 comparisons,472 adopted-scene frames,65 forward and25 steering commands, enter/drive/exit and actual seated camera. |

V149's vehicle run is RED because the desktop lost focus, clearing its keyboard
state after46 forward commands. The harness now reacquires the real window and
completes the observed input budget. V149's grenade authority reached a missing
1CD7F0 physical-body velocity/query owner; the complete original body and its
reached dependencies were lowered before V150. Neither failure is suppressed.

The current checkout's production and acceptance targets build successfully.
`live-root-v151` passes all seven selected visible scenarios with protocol66:
local vehicle/lift/objects and UDP vehicle/lift/movement/objects. Each UDP run
retains exact authority comparisons (301 for vehicle,181 for the other cases),
with493/892/805/404 adopted-scene render frames for vehicle/lift/movement/objects,
respectively. Every scenario retains a nonempty, probed X11 window recording.
This current-root evidence includes the production wire fields absent from the
older private pair. Installed-launcher delivery remains a separate open step.

Full TU1.5 physics parity remains incomplete. The next visible fixture throws a
real grenade beside an authored Lockout power module to exercise damage,
physical response and retirement. It currently reaches further original damage
and animation owners; no prop impulse/destruction pass is claimed yet.
